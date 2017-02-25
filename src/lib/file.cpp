#include "file.hpp"
#include "error.hpp"
#include "text.hpp"
#include "raw.hpp"
#include "time.hpp"
#ifdef __unix__
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>
#endif
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#ifdef __unix__
int fd (nat8_t h);
#endif
#ifdef _WIN32
HANDLE handle (nat8_t h);
#endif

path_t::operator bool_t () const
{
	return bool_t(cos);
}

path_t create_path (const str_t& text)
{
	if (!text) { return {}; }

	auto parse_back = true;
	for (auto g : text) {
		if (g == '/') {
			parse_back = false;
		}
	}

	path_t path; {
		nat8_t n_cos = 0;
		nat8_t front = 0;
		for (auto i : create_range(text.len)) {
			if (text[i] == '/' || (text[i] == '\\' && parse_back)) {
				if (i != front || n_cos == 0) { ++n_cos; }
				front = i + 1;
			}
		}
		if (text.len != front) { ++n_cos; }
		path.cos = create_seq<str_t>(n_cos);
	}

	nat8_t seg_i = 0;
	nat8_t front = 0;
	for (auto i : create_range(text.len)) {
		if (text[i] == '/' || (text[i] == '\\' && parse_back)) {
			if (i != front || seg_i == 0) {
				path.cos[seg_i] = create_str(&text[front], i - front);
				++seg_i;
			}
			front = i + 1;
		}
	}
	if (text.len != front) {
		path.cos[seg_i] = create_str(&text[front], text.len - front);
		++seg_i;
	}

	assert_eq(seg_i, path.cos.len);
	return path;
}

path_t clone (const path_t& path)
{
	path_t prod;
	prod.cos = create_seq<str_t>(path.cos.len);
	for (auto i : create_range(path.cos.len)) {
		prod.cos[i] = clone(path.cos[i]);
	}
	return prod;
}

path_t operator + (const path_t& path, const str_t& right)
{
	path_t prod = clone(path);
	if (right) {
		grow(prod.cos, prod.cos.len, 1);
		prod.cos[prod.cos.len - 1] = clone(right);
	}
	return prod;
}

str_t as_text (const path_t& path)
{
	#ifdef __unix__
	return as_text(path, "/");
	#endif
	#ifdef _WIN32
	return as_text(path, "\\");
	#endif
}

str_t as_text (const path_t& path, const str_t& delim)
{
	if (path.cos.len == 1 && !path.cos[0]) { return clone(delim); }

	str_t text; {
		nat8_t len = 0;
		for (auto& seg : path.cos) {
			if (&seg != &path.cos[0]) { len += delim.len; }
			len += seg.len;
		}
		text = create_str(len);
	}

	nat8_t buf_i = 0;
	for (auto& seg : path.cos) {
		if (&seg != &path.cos[0]) {
			copy_mem(&text[buf_i], delim.ptr, delim.len);
			buf_i += delim.len;
		}
		if (seg.len > 0) {
			copy_mem(&text[buf_i], seg.ptr, seg.len);
			buf_i += seg.len;
		}
	}
	assert_eq(buf_i, text.len);
	return text;
}

path_t get_working_dir (err_t& err)
{
	if (err) { return {}; }

	#ifdef __unix__
	auto buf = create_seq<char>(256);
	while (true) {
		if (getcwd(buf.ptr, buf.len)) {
			assert_false(buf[buf.len - 1]);
			return create_path(create_str(buf.ptr));
		} else if (errno == ERANGE) {
			grow(buf, buf.len, buf.len);
		} else {
			err = decode_os_err(errno);
			return {};
		}
	}
	#endif

	#ifdef _WIN32
	auto len = GetCurrentDirectory(0, NULL);
	if (len <= 0) {
		err = decode_os_err(GetLastError());
		return {};
	}
	auto buf = create_seq<WCHAR>(len);
	if (GetCurrentDirectory(len, buf.ptr) > 0) {
		return create_path(create_str(buf.ptr));
	}
	err = decode_os_err(GetLastError());
	return {};
	#endif
}

void_t set_working_dir (const path_t& path, err_t& err)
{
	if (err) { return; }

	#ifdef __unix__
	if (chdir(as_strz(as_text(path)).ptr) == 0) { return; }
	err = decode_os_err(errno);
	#endif

	#ifdef _WIN32
	if (SetCurrentDirectory(as_wstr(as_text(path)).ptr)) { return; }
	err = decode_os_err(GetLastError());
	#endif
}

path_t create_temp_path (err_t& err)
{
	if (err) { return {}; }

	#ifdef __unix__
	char buf[12];
	copy_mem(buf, "/tmp/XXXXXX", sizeof(buf));
	if (int fd = mkstemp(buf); fd >= 0) {
		close(fd);
		return create_path(buf);
	}
	err = decode_os_err(errno);
	return {};
	#endif

	#ifdef _WIN32
	if (WCHAR tmp_dir[MAX_PATH + 1] = {}; GetTempPath(sizeof(tmp_dir), tmp_dir)) {
		if (WCHAR tmp_path[MAX_PATH + 1] = {}; GetTempFileName(tmp_dir, TEXT("TMP"), 0, tmp_path)) {
			return create_path(create_str(tmp_path));
		}
	}
	err = decode_os_err(GetLastError());
	return {};
	#endif
}

path_t get_dir (const path_t& path)
{
	if (path.cos.len <= 1) { return {}; }

	path_t prod;
	prod.cos = create_seq<str_t>(path.cos.len - 1);
	for (auto i : create_range(path.cos.len - 1)) {
		prod.cos[i] = clone(path.cos[i]);
	}
	return prod;
}

str_t get_ext (const path_t& path)
{
	if (!path) { return {}; }

	const auto& leaf = path.cos[path.cos.len - 1];
	for (auto i : create_range(leaf.len)) {
		i = leaf.len - i;
		if (leaf[i - 1] == '.') {
			return create_str(&leaf[i], leaf.len - i);
		}
	}
	return {};
}

void_t set_ext (path_t& path, const str_t& ext)
{
	if (!path) { return; }

	auto& leaf = path.cos[path.cos.len - 1];
	for (auto i : create_range(leaf.len)) {
		i = leaf.len - i;
		if (leaf[i - 1] == '.') {
			if (ext) {
				leaf = create_str(leaf.ptr, i) + ext;
			} else {
				if (i - 1 > 0) {
					leaf = create_str(leaf.ptr, i - 1);
				} else {
					path = get_dir(path);
				}
			}
			return;
		}
	}
	if (ext) {
		leaf = leaf + "." + ext;
	}
}

#ifdef __unix__
int get_fd (opaque_t opaq);
opaque_t create_opaque_fd (int fd);
#endif
#ifdef _WIN32
HANDLE get_handle (opaque_t opaq);
opaque_t create_opaque_handle (HANDLE h);
#endif

file_t::file_t () { }
file_t::~file_t ()
{
	if (!opaq) { return; }
	#ifdef __unix__
	close(get_fd(opaq));
	#endif
	#ifdef _WIN32
	CloseHandle(get_handle(opaq));
	#endif
}

file_t::file_t (file_t&& ori) { *this = move(ori); }
file_t& file_t::operator = (file_t&& ori)
{
	if (&ori != this) {
		this->~file_t();
		opaq = ori.opaq;
		ori.opaq = {};
	}
	return *this;
}

file_t::operator bool_t () const
{
	return bool_t(opaq);
}

file_t open_file (const path_t& path, bool_t writing, err_t& err)
{
	if (err) { return {}; }

	#ifdef __unix__
	const mode_t mode = S_IRWXU | S_IRGRP | S_IROTH;
	if (int fd = open(as_strz(as_text(path)).ptr, writing ? O_RDWR | O_CREAT : O_RDONLY, mode); fd >= 0) {
		file_t file;
		file.opaq = create_opaque_fd(fd);
		return file;
	}
	err = decode_os_err(errno);
	return {};
	#endif

	#ifdef _WIN32
	const DWORD access = writing ? GENERIC_READ | GENERIC_WRITE : GENERIC_READ;
	if (HANDLE handle = CreateFile(as_wstr(as_text(path)).ptr, access, writing ? 0 : FILE_SHARE_READ, NULL,
	                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL); handle != INVALID_HANDLE_VALUE) {
		assert_true(handle);
		file_t file;
		file.opaq = create_opaque_handle(handle);
		return file;
	}
	err = decode_os_err(GetLastError());
	return {};
	#endif
}

str_t read (file_t& file, nat8_t len, err_t& err)
{
	if (err) { return {}; }

	str_t buf = create_str(len != max<decltype(len)>() ? len : 1024 * 4);
	decltype(buf.len) i = 0;
	while (i < buf.len) {

		#ifdef __unix__
		const auto stat = read(get_fd(file.opaq), &buf[i], buf.len - i);
		if (stat < 0) {
			err = decode_os_err(errno);
			return {};
		}
		const auto red = static_cast<nat8_t>(stat);
		#endif

		#ifdef _WIN32
		const auto rd_len = static_cast<DWORD>(clamp(buf.len - i, 0, max<DWORD>()));
		DWORD stat = 0;
		if (!ReadFile(get_handle(file.opaq), &buf[i], rd_len, &stat, NULL)) {
			err = decode_os_err(GetLastError());
			return {};
		}
		const auto red = static_cast<nat8_t>(rd_len);
		#endif

		assert_lteq(red, buf.len - i);
		i += red;

		if (len != max<decltype(len)>()) {
			if (red == 0) {
				err = create_err("Unexpected end of file");
				return {};
			}
		} else {
			if (red == 0) {
				shrink(buf, i, buf.len - i);
			} else if (i == buf.len) {
				grow(buf, buf.len, buf.len);
			}
		}
	}
	return buf;
}

void_t write (file_t& file, const str_t& data, err_t& err)
{
	if (err) { return; }

	decltype(data.len) i = 0;
	while (i < data.len) {

		#ifdef __unix__
		const auto stat = write(get_fd(file.opaq), &data[i], data.len - i);
		if (stat < 0) {
			err = decode_os_err(errno);
			return;
		}
		const auto written = static_cast<nat8_t>(stat);
		#endif

		#ifdef _WIN32
		const auto wr_len = static_cast<DWORD>(clamp(data.len - i, 0, max<DWORD>()));
		DWORD stat = 0;
		if (!WriteFile(get_handle(file.opaq), &data[i], wr_len, &stat, NULL)) {
			err = decode_os_err(GetLastError());
			return;
		}
		const auto written = static_cast<nat8_t>(wr_len);
		#endif

		assert_lteq(written, data.len - i);
		i += written;
		assert_gt(written, 0);
		if (written == 0) {
			err = create_err("End of file");
			return;
		}
	}
}

void_t set_cursor (file_t& file, nat8_t at, err_t& err)
{
	if (err) { return; }

	#ifdef __unix__
	assert_lteq(at, max<off_t>());
	if (lseek(get_fd(file.opaq), static_cast<off_t>(at), SEEK_SET) >= 0) { return; }
	err = decode_os_err(errno);
	#endif

	#ifdef _WIN32
	assert_lteq(at, max<LONGLONG>());
	LARGE_INTEGER win_at = {};
	win_at.QuadPart = static_cast<LONGLONG>(at);
	if (SetFilePointerEx(get_handle(file.opaq), win_at, NULL, FILE_BEGIN)) { return; }
	err = decode_os_err(GetLastError());
	#endif
}

date_t read_last_mod (file_t& file, err_t& err)
{
	#ifdef __unix__
	date_t create_date (const timespec& ts);

	if (struct stat st = {}; fstat(get_fd(file.opaq), &st) == 0) { return create_date(st.st_mtim); }

	err = decode_os_err(errno);
	return {};
	#endif

	#ifdef _WIN32
	date_t create_date (const FILETIME& file);

	if (FILETIME ft = {}; GetFileTime(get_handle(file.opaq), NULL, NULL, &ft)) { return create_date(ft); }

	err = decode_os_err(GetLastError());
	return {};
	#endif
}

void_t write_last_mod (file_t& file, const date_t& date, err_t& err)
{
	#ifdef __unix__
	timespec as_timespec (date_t date);
	timespec create_omission_timespec (void);

	if (timespec times[2] = {create_omission_timespec(), as_timespec(date)};
	    futimens(get_fd(file.opaq), times) == 0) { return; }

	err = decode_os_err(errno);
	#endif

	#ifdef _WIN32
	FILETIME as_filetime (date_t date);

	if (FILETIME fn = as_filetime(date); SetFileTime(get_handle(file.opaq), NULL, NULL, &fn)) { return; }

	err = decode_os_err(GetLastError());
	#endif
}

define_test(path, "text")
{
	{ auto p = create_path("");
		prove_eq(p.cos.len, 0);
		prove_same(as_text(p), "");
	}
	{ auto p = create_path("/");
		prove_eq(p.cos.len, 1);
		prove_same(p.cos[0], "");
		prove_same(as_text(p, "/"), "/");
	}
	{ auto p = create_path("/etc/motd");
		prove_eq(p.cos.len, 3);
		prove_same(p.cos[0], "");
		prove_same(p.cos[1], "etc");
		prove_same(p.cos[2], "motd");
		prove_same(as_text(p, "+"), "+etc+motd");
	}
	{ auto p = create_path("./..");
		prove_eq(p.cos.len, 2);
		prove_same(p.cos[0], ".");
		prove_same(p.cos[1], "..");
		prove_same(as_text(p, "/"), "./..");
	}
	{ auto p = create_path("C:\\Windows\\\\NSA_Backdoor\\");
		prove_eq(p.cos.len, 3);
		prove_same(p.cos[0], "C:");
		prove_same(p.cos[1], "Windows");
		prove_same(p.cos[2], "NSA_Backdoor");
		prove_same(as_text(p, "\\"), "C:\\Windows\\NSA_Backdoor");
	}
	{ auto p = create_path("But\\What/Am\\I");
		prove_eq(p.cos.len, 2);
		prove_same(p.cos[0], "But\\What");
		prove_same(p.cos[1], "Am\\I");
		prove_same(as_text(p, ""), "But\\WhatAm\\I");
	}
	{ auto p = create_path("/food/.cookies/raisin");
		prove_eq(p.cos.len, 4);
		prove_same(p.cos[0], "");
		prove_same(p.cos[1], "food");
		prove_same(p.cos[2], ".cookies");
		prove_same(p.cos[3], "raisin");
		prove_same(as_text(p, "/"), "/food/.cookies/raisin");
		prove_same(get_ext(p), "");
		set_ext(p, "bits");
		prove_same(get_ext(p), "bits");
		prove_same(as_text(p, "/"), "/food/.cookies/raisin.bits");
		set_ext(p, "");
		prove_same(get_ext(p), "");
		prove_same(as_text(p, "/"), "/food/.cookies/raisin");
		p = get_dir(p);
		prove_same(as_text(p, "/"), "/food/.cookies");
		prove_same(get_ext(p), "cookies");
		set_ext(p, "cakes");
		prove_same(get_ext(p), "cakes");
		prove_same(as_text(p, "/"), "/food/.cakes");
		set_ext(p, "");
		prove_same(as_text(p, "/"), "/food");
		prove_same(get_ext(p), "");
		set_ext(p, "stuffs");
		prove_same(get_ext(p), "stuffs");
		prove_same(as_text(p, "/"), "/food.stuffs");
		p = get_dir(p);
		prove_same(as_text(p, "/"), "/");
		prove_true(p);
		p = get_dir(p);
		prove_same(as_text(p, "/"), "");
		prove_false(p);
	}

	return {};
}

