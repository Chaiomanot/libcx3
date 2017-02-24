#include "queue.hpp"
#include "raw.hpp"
#include "box.hpp"

str_t recv_waited (queue_t& q)
{
	auto lock = acquire(q.read_mut);
	assert_true(q.buf);
	assert_uneq(q.read_at, q.write_at);
	auto next = (q.read_at + 1 < q.buf.len ? q.read_at + 1 : 0);
	auto val = move(q.buf[q.read_at]);
	q.read_at = next;
	return val;
}

str_t recv (queue_t& q)
{
	wait(q.sem);
	return recv_waited(q);
}

void_t send (queue_t& q, str_t msg)
{
	{ mutex_lock_t lock = acquire(q.write_mut);
		auto next = (q.write_at + 1 < q.buf.len ? q.write_at + 1 : 0);
		if (next == q.read_at) {
			auto inc_len = q.buf.len > 4 ? q.buf.len : 4;
			mutex_lock_t read_lock = acquire(q.read_mut);
			if (q.read_at > q.write_at) {
				q.read_at += inc_len;
			}
			grow(q.buf, q.write_at, inc_len);
			next = (q.write_at + 1 < q.buf.len ? q.write_at + 1 : 0);
		}
		q.buf[q.write_at] = move(msg);
		q.write_at = next;
	}
	signal(q.sem);
}

#include "text.hpp"
#include "error.hpp"

void_t test_queue_thread_entry (queue_t& q)
{
	for (auto i : create_range(10)) {
		send(q, "<pineapple>");
	}
	for (auto i : create_range(100)) {
		send(q, "<cherry>");
		recv(q);
	}
	for (auto i : create_range(10)) {
		recv(q);
	}
}

define_test(queue, "thread")
{
	queue_t q;

	err_t e;
	for (auto i : create_range(4)) {
		spawn_thread(&test_queue_thread_entry, q, e);
	}
	prove_same(as_text(e), "");

	wait_for_threads();
	return {};
}

