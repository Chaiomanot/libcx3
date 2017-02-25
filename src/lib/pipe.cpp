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

pipe_pair_t pipe_create (err_t& err)
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
			err = decode_os_err(errno);
			close(fds[0]);
			close(fds[1]);
			return {};
		}
	} else {
		err = decode_os_err(errno);
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
		err = decode_os_err(GetLastError());
		return {};
	}
	#endif
}

nat8_t recv (pipe_t& pipe, void_t* buf_ptr, nat8_t buf_len, err_t& err)
{
	if (err) { return 0; }

	#ifdef __unix__
	if (const auto stat = read(get_fd(pipe.h_in), buf_ptr, buf_len); stat >= 0) {
		assert_lteq(static_cast<decltype(buf_len)>(stat), buf_len);
		return static_cast<decltype(buf_len)>(stat);
	} else {
		static_assert(EAGAIN == EWOULDBLOCK);
		if (errno == EAGAIN) { return 0; } // come back later
		err = decode_os_err(errno);
		return 0;
	}
	#endif

	#ifdef _WIN32
	DWORD red = 0;
	if (!ReadFile(get_handle(pipe.h_out), buf_ptr, static_cast<DWORD>(buf_len), &red, NULL)) {
		err = decode_os_err(GetLastError());
		return 0;
	}
	assert_lteq(static_cast<decltype(buf_len)>(red), buf_len);
	return static_cast<decltype(buf_len)>(red);
	#endif
}

nat8_t send (pipe_t& pipe, const void_t* data_ptr, nat8_t data_len, err_t& err)
{
	if (err) { return 0; }

	#ifdef __unix__
	if (auto stat = write(get_fd(pipe.h_out), data_ptr, data_len); stat > 0) {
		return static_cast<decltype(data_len)>(stat);
	} else {
		assert_uneq(stat, 0);
		if (stat == 0) { // this should never happen...
			errno = ECONNRESET; // ...but it's an easy case to handle
		}
		static_assert(EAGAIN == EWOULDBLOCK);
		if (errno == EAGAIN) {
			return 0; // come back later
		}
		err = decode_os_err(errno);
		return 0;
	}
	#endif

	#ifdef _WIN32
	if (data_len > max<DWORD>()) { data_len = max<DWORD>(); }
	DWORD written = 0;
	if (!WriteFile(get_handle(pipe.h_out), data_ptr, static_cast<DWORD>(data_len), &written, NULL)) {
		err = decode_os_err(GetLastError());
		return 0;
	}
	return written;
	#endif
}

str_t recv (pipe_t& pipe, err_t& err)
{
	if (err) { return {}; }
	str_t buf = create_str(65536);
	const auto red = recv(pipe, buf.ptr, buf.len, err);
	shrink(buf, red, buf.len - red);
	return buf;
}

void_t send (pipe_t& pipe, const str_t& data, err_t& err)
{
	if (err) { return; }
	for (nat8_t i = 0; i < data.len; ) {
		const auto written = send(pipe, &static_cast<const nat1_t*>(data.ptr)[i], data.len - i, err);
		if (written == 0) {
			if (!err) { err = create_err("Couldn't send message before connection was closed"); }
			return;
		}
		i += written;
	}
}

