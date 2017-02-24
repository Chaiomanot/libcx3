#include "thread.hpp"
#include "error.hpp"
#include "raw.hpp"
#include "box.hpp"
#include "bag.hpp"
#ifdef __unix__
#include <pthread.h>
#include <signal.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <errno.h>
#endif
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

bool_t set_atomic_cmp (nat8_t& datum, nat8_t cond, nat8_t val)
{
	#ifdef __unix__
	return __sync_bool_compare_and_swap(&datum, cond, val);
	#endif
	#ifdef _WIN32
	const auto long_cond = static_cast<LONG64>(cond);
	auto ptr = const_cast<volatile LONG64*>(reinterpret_cast<LONG64*>(&datum));
	return InterlockedCompareExchange64(ptr, static_cast<LONG64>(val), long_cond) == long_cond;
	#endif
}

#ifdef __unix__
int get_fd (opaque_t opaq);
opaque_t create_opaque_fd (int fd);
#endif
#ifdef _WIN32
HANDLE get_handle (opaque_t opaq);
opaque_t create_opaque_handle (HANDLE h);
#endif
void_t* get_ptr (opaque_t opaq);
opaque_t create_opaque_ptr (void_t* ptr);

sem_t::sem_t () { }
sem_t::~sem_t ()
{
	if (!opaq) { return; }
	#ifdef __unix__
	close(get_fd(opaq));
	#endif
	#ifdef _WIN32
	CloseHandle(get_handle(opaq));
	#endif
}

sem_t::sem_t (sem_t&& ori) { *this = move(ori); }
sem_t& sem_t::operator = (sem_t&& ori)
{
	if (&ori != this) {
		this->~sem_t();
		opaq = ori.opaq;
		ori.opaq = {};
	}
	return *this;
}

void_t init (sem_t& sem)
{
	#ifdef __unix__
	const auto fd = eventfd(0, EFD_SEMAPHORE);
	assert_gteq(fd, 0);
	const auto opaq = create_opaque_fd(fd);
	#endif

	#ifdef _WIN32
	const auto handle = CreateSemaphore(NULL, 0, max<LONG>(), NULL);
	assert_true(handle);
	const auto opaq = create_opaque_handle(handle);
	#endif

	if (!set_atomic_cmp(sem.opaq.val, {}, opaq.val)) {
		sem_t cleanup;
		cleanup.opaq = opaq;
	}
}

void_t signal (sem_t& sem)
{
	if (!sem.opaq) { init(sem); }

	const nat8_t step = 1;

	#ifdef __unix__
	const auto stat = write(get_fd(sem.opaq), &step, sizeof(step));
	assert_eq(stat, sizeof(step));
	unused(stat);
	#endif

	#ifdef _WIN32
	const auto suc = ReleaseSemaphore(get_handle(sem.opaq), step, NULL);
	assert_true(suc);
	unused(suc);
	#endif
}

void_t wait (sem_t& sem)
{
	if (!sem.opaq) { init(sem); }

	#ifdef __unix__
	nat8_t counter = {};
	const auto stat = read(get_fd(sem.opaq), &counter, sizeof(counter));
	assert_eq(stat, sizeof(counter));
	unused(stat);
	assert_eq(counter, 1);
	#endif

	#ifdef _WIN32
	const auto stat = WaitForSingleObject(get_handle(sem.opaq), INFINITE);
	assert_eq(stat, WAIT_OBJECT_0);
	unused(stat);
	#endif
}

#ifdef __unix__
pthread_mutex_t* get_handle (mutex_t& mutex) { return static_cast<pthread_mutex_t*>(get_ptr(mutex.opaq)); }
#endif
#ifdef _WIN32
CRITICAL_SECTION* get_handle (mutex_t& mutex) { return static_cast<CRITICAL_SECTION*>(get_ptr(mutex.opaq)); }
#endif

mutex_t::mutex_t () { }
mutex_t::~mutex_t ()
{
	if (!opaq) { return; }

	#ifdef __unix__
	const auto stat = pthread_mutex_destroy(get_handle(*this));
	assert_eq(stat, 0);
	unused(stat);
	box_t<pthread_mutex_t> box;
	acquire(box, get_handle(*this));
	#endif

	#ifdef _WIN32
	DeleteCriticalSection(get_handle(*this));
	box_t<CRITICAL_SECTION> box;
	acquire(box, get_handle(*this));
	#endif
}

mutex_t::mutex_t (mutex_t&& ori) { *this = move(ori); }
mutex_t& mutex_t::operator = (mutex_t&& ori)
{
	if (&ori != this) {
		this->~mutex_t();
		opaq = ori.opaq;
		ori.opaq = {};
	}
	return *this;
}

void_t init (mutex_t& mutex)
{
	#ifdef __unix__
	box_t<pthread_mutex_t> box;
	auto stat = pthread_mutex_init(*box, nullptr);
	assert_eq(stat, 0);
	unused(stat);
	#endif

	#ifdef _WIN32
	box_t<CRITICAL_SECTION> box;
	InitializeCriticalSection(*box);
	#endif

	auto opaq = create_opaque_ptr(release(box));
	if (!set_atomic_cmp(mutex.opaq.val, {}, opaq.val)) {
		mutex_t cleanup;
		cleanup.opaq = opaq;
	}
}

mutex_lock_t acquire (mutex_t& mutex)
{
	if (!mutex.opaq) { init(mutex); }

	#ifdef __unix__
	auto stat = pthread_mutex_lock(get_handle(mutex));
	assert_eq(stat, 0);
	unused(stat);
	#endif

	#ifdef _WIN32
	EnterCriticalSection(get_handle(mutex));
	#endif

	mutex_lock_t lock;
	lock.mutex = &mutex;
	return lock;
}

void_t release (mutex_t& mutex)
{
	assert_true(mutex.opaq);

	#ifdef __unix__
	const auto stat = pthread_mutex_unlock(get_handle(mutex));
	assert_eq(stat, 0);
	unused(stat);
	#endif

	#ifdef _WIN32
	LeaveCriticalSection(get_handle(mutex));
	#endif
}

mutex_lock_t::mutex_lock_t () { }
mutex_lock_t::~mutex_lock_t () { assert_true(mutex); release(*mutex); }

mutex_lock_t::mutex_lock_t (mutex_lock_t&& ori) { *this = move(ori); }
mutex_lock_t& mutex_lock_t::operator = (mutex_lock_t&& ori)
{
	if (&ori != this) {
		this->~mutex_lock_t();
		mutex = ori.mutex;
		ori.mutex = nullptr;
	}
	return *this;
}

struct thread_ctx_t
{
	void_t (*entry) (void_t* param) {};
	void_t* arg {};
};

#ifdef __unix__
void_t* thread_entry (void_t* arg)
#endif
#ifdef _WIN32
DWORD WINAPI thread_entry (LPVOID arg)
#endif
{
	assert_true(arg);
	box_t<thread_ctx_t> ctx_box;
	acquire(ctx_box, static_cast<thread_ctx_t*>(arg));
	auto ctx = *release(ctx_box);
	ctx.entry(ctx.arg);
	return {};
}

#ifdef __unix__
typedef pthread_t thread_os_t;
#endif
#ifdef _WIN32
typedef HANDLE thread_os_t;
#endif
mutex_t            threads_mutex;
bag_t<thread_os_t> threads;

void_t install_thread (thread_os_t thread)
{
	auto lock = acquire(threads_mutex);
	insert(threads, thread);
}

void_t join_ready_threads ()
{
	auto lock = acquire(threads_mutex);
	for (auto& thread : threads) {
		#ifdef __unix__
		if (const auto stat = pthread_kill(thread, 0); stat != 0) {
			assert_eq(stat, ESRCH);
			const auto join_stat = pthread_join(thread, nullptr);
			assert_eq(join_stat, 0);
			unused(join_stat);
			remove(threads, thread);
		}
		#endif
		#ifdef _WIN32
		if (const auto stat = WaitForSingleObject(thread, 0); stat == WAIT_OBJECT_0) {
			CloseHandle(thread);
			remove(threads, thread);
		}
		#endif
	}
}

void_t spawn_thread_raw_param (void_t (*entry) (void_t* param), void_t* arg, err_t& err)
{
	assert_true(entry);
	if (err) { return; }
	join_ready_threads();

	box_t<thread_ctx_t> ctx;
	ctx->entry = entry;
	ctx->arg   = arg;

	#ifdef __unix__
	pthread_t handle = 0;
	if (const auto stat = pthread_create(&handle, nullptr, &thread_entry, *ctx); stat == 0) {
		release(ctx);
		install_thread(handle);
	} else {
		err = decode_os_err(stat);
	}
	#endif

	#ifdef _WIN32
	if (auto handle = CreateThread(NULL, 0, &thread_entry, *ctx, 0, NULL); handle) {
		release(ctx);
		install_thread(handle);
	} else {
		err = decode_os_err(GetLastError());
	}
	#endif
}

void_t wait_for_threads ()
{
	auto lock = acquire(threads_mutex);
	for (auto& thread : threads) {

		#ifdef __unix__
		const auto stat = pthread_join(thread, nullptr);
		assert_eq(stat, 0);
		unused(stat);
		#endif

		#ifdef _WIN32
		const auto stat = WaitForSingleObject(thread, INFINITE);
		assert_eq(stat, WAIT_OBJECT_0);
		unused(stat);
		CloseHandle(thread);
		#endif

		remove(threads, thread);
	}
}

#include "text.hpp"

void_t test_thread_thread_entry (nat8_t& val)
{
	if (val == 1) { val = 2; }
}

define_test(thread, "error")
{
	nat8_t val = 1;

	err_t err;
	spawn_thread(&test_thread_thread_entry, val, err);
	prove_same(as_text(err), "");
	wait_for_threads();

	prove_eq(val, 2);

	return {};
}

