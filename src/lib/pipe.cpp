#include "pipe.hpp"
#include "error.hpp"
#include "text.hpp"
#ifdef __unix__
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#endif
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#ifdef __unix__
int get_fd (opaque_t opaq);
opaque_t create_opaque_fd (int fd);
#endif
#ifdef _WIN32
HANDLE get_handle (opaque_t opaq);
opaque_t create_opaque_handle (HANDLE h);
#endif

pipe_t::pipe_t () { }

pipe_t::~pipe_t ()
{
	if (!h_in && !h_out) { return; }
	#ifdef __unix__
	close(get_fd(h_in));
	close(get_fd(h_out));
	#endif
	#ifdef _WIN32
	CloseHandle(get_handle(h_in));
	CloseHandle(get_handle(h_out));
	#endif
}

pipe_t::pipe_t (pipe_t&& src) { *this = move(src); }
pipe_t& pipe_t::operator = (pipe_t&& src)
{
	if (&src != this) {
		this->~pipe_t();
		h_in  = src.h_in;
		h_out = src.h_out;
		src.h_in  = {};
		src.h_out = {};
	}
	return *this;
}

pipe_t::operator bool () const
{
	assert_eq(bool(h_in), bool(h_out));
	return bool(h_in);
}

pipe_pair_t::operator bool () const
{
	assert_eq(bool(left), bool(right));
	return bool(left);
}

pipe_pair_t pipe_create (err_t& e)
{
	#ifdef __unix__
	if (int fds[4] = {-1}; pipe(fds) == 0) {
		if (pipe(&fds[2]) == 0) {
			for (auto i : create_range(sizeof(fds) / sizeof(int))) {
				auto stat = fcntl(fds[i], F_SETFL, O_NONBLOCK);
				assert_gteq(stat, 0);
				unused(stat);
			}
			pipe_pair_t pp;
			pp.left.h_in   = create_opaque_fd(fds[0]);
			pp.left.h_out  = create_opaque_fd(fds[3]);
			pp.right.h_in  = create_opaque_fd(fds[2]);
			pp.right.h_out = create_opaque_fd(fds[1]);
			return pp;
		} else {
			e = decode_os_err(errno);
			close(fds[0]);
			close(fds[1]);
			return {};
		}
	} else {
		e = decode_os_err(errno);
		return {};
	}
	#endif

	#ifdef _WIN32
	pipe_pair_t pp;
	if (CreatePipe(reinterpret_cast<HANDLE*>(&pp.left.h_in),
					reinterpret_cast<HANDLE*>(&pp.right.h_out), NULL, 0) &&
		CreatePipe(reinterpret_cast<HANDLE*>(&pp.right.h_in),
					reinterpret_cast<HANDLE*>(&pp.left.h_out), NULL, 0)) {
		return pp;
	} else {
		e = decode_os_err(GetLastError());
		return {};
	}
	#endif
}

str_t recv (pipe_t& p, err_t& e)
{
	if (e) { return {}; }

	#ifdef __unix__
	str_t buf = create_str(65536);
	if (const auto stat = read(get_fd(p.h_in), buf.ptr, buf.len); stat >= 0) {
		const auto red = static_cast<decltype(buf.len)>(stat);
		assert_lteq(red, buf.len);
		shrink(buf, red, buf.len - red);
		return buf;
	} else {
		static_assert(EAGAIN == EWOULDBLOCK);
		if (errno == EAGAIN) {
			return {}; // come back later
		}
		e = decode_os_err(errno);
		return {};
	}
	#endif

	#ifdef _WIN32
	unused(p); // TODO
	err = create_err("Not yet implemented");
	return {};
	#endif
}

nat8_t send (pipe_t& p, const str_t& data, err_t& e)
{
	if (e) { return 0; }

	#ifdef __unix__
	if (auto stat = write(get_fd(p.h_out), data.ptr, data.len); stat > 0) {
		return static_cast<decltype(data.len)>(stat);
	} else {
		assert_uneq(stat, 0);
		if (stat == 0) { // this should never happen...
			errno = ECONNRESET; // ...but it's an easy case to handle
		}
		static_assert(EAGAIN == EWOULDBLOCK);
		if (errno == EAGAIN) {
			return 0; // come back later
		}
		e = decode_os_err(errno);
		return 0;
	}
	#endif

	#ifdef _WIN32
	unused(p); // TODO
	unused(data);
	err = create_err("Not yet implemented");
	return 0;
	#endif
}

