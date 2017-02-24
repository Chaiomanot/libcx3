#include "error.hpp"
#include "text.hpp"
#ifdef __unix__
#include "library.hpp"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreserved-id-macro"
#define _XOPEN_SOURCE 700
#define _POSIX_C_SOURCE 200809L
#undef _GNU_SOURCE
#pragma clang diagnostic pop
#endif
#include <stdlib.h>
#include <stdio.h>
#ifdef __unix__
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <dlfcn.h>
#endif
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

err_t::operator bool_t () const
{
	return bool_t(msg);
}

err_t create_err (str_t msg)
{
	err_t err;
	err.msg = move(msg);
	return err;
}

str_t as_text (const err_t& err)
{
	return clone(err.msg);
}

err_t operator + (const err_t& left, const err_t& right)
{
	assert_true(left);
	assert_true(right);

	return create_err(left.msg + ": " + right.msg);
}

#ifdef __unix__
err_t decode_os_err (int code)
{
	if (code < 0) { code = -code; }

	char buf[512] = {};
	const int stat = strerror_r(code, buf, sizeof(buf) - 1);
	if (stat == 0 && buf[0]) {
		return create_err(buf);
	} else {
		return create_err("System error code " + as_text(static_cast<nat4_t>(code)));
	}
}
#endif

#ifdef _WIN32
err_t decode_os_err (unsigned long code)
{
	str_t buf;
	WCHAR* wbuf = nullptr;
	const DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
	const DWORD lang = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);
	const auto buf_len = FormatMessage(flags, NULL, code, lang, reinterpret_cast<WCHAR*>(&wbuf), 0, NULL);
	if (wbuf && buf_len > 0) {
		buf = create_text(wbuf);
		while (buf) {
			const auto g = buf[buf.len - 1];
			if (g == '.' || g == '\r' || g == '\n') {
				shrink(buf, buf.len - 1, 1);
			} else {
				break;
			}
		}
	} else {
		assert_eq(GetLastError(), ERROR_MR_MID_NOT_FOUND);
		buf = "System error code " + enc(code);
	}
	if (wbuf) {
		LocalFree(wbuf);
	}
	return create_err(move(buf));
}
#endif

err_t get_last_os_err ()
{
	#ifdef __unix__
	return decode_os_err(errno);
	#endif
	#ifdef _WIN32
	return decode_os_err(GetLastError());
	#endif
}

[[noreturn]] void_t terminate_prog ()
{
	#ifdef __unix__
	abort();
	#endif
	#ifdef _WIN32
	ExitProcess(0); // DebugBreak
	#endif
}

#ifdef __unix__
typedef char   gchar;
typedef int    gint;
typedef gint   gboolean;
typedef void*  gobject;
typedef nat4_t GQuark;
struct GError {
	GQuark domain;
	gint   code;
	gchar* message;
};
struct NotifyNotification;

typedef gboolean (*notify_init_t) (const char* app_name);
typedef void (*notify_uninit_t) ();
typedef NotifyNotification* (*notify_notification_new_t) (const char* summary, const char* body, const char* icon);
typedef void (*notify_notification_set_timeout_t) (NotifyNotification* notification, gint timeout);
typedef gboolean (*notify_notification_show_t) (NotifyNotification* notification, GError** error);
typedef void (*gobject_unref_t) (gobject object);

gobject G_OBJECT (gobject object) { return object; }
static const gint NOTIFY_EXPIRES_NEVER = 0;
#endif

void_t display_err_msg (const str_t& title, const str_t& msg, err_t& err)
{
	if (err) { return; }

	#ifdef __unix__
	if (isatty(STDIN_FILENO)) {
		if (const auto stat = fprintf(stderr, "\x1B[31m%s\x1B[0m %s\n", as_strz(title).ptr, as_strz(msg).ptr); stat < 0) {
			err = create_err("Error printing to standard error stream");
		}
		return;
	}
	#endif

	#ifdef _WIN32
	if (GetConsoleWindow()) {
		HANDLE term = GetStdHandle(STD_ERROR_HANDLE);
		CONSOLE_SCREEN_BUFFER_INFO info = {};
		GetConsoleScreenBufferInfo(term, &info);
		SetConsoleTextAttribute(term, FOREGROUND_RED);
		fprintf(stderr, "[%s]", create_strz(title).ptr);
		SetConsoleTextAttribute(term, info.wAttributes);
		if (const auto stat = fprintf(stderr, "%s\r\n", as_strz(msg).ptr); stat < 0) {
			err = create_err("Error printing to standard error stream");
		}
		return;
	}
	#endif

	#ifdef __unix__
	notify_init_t                     notify_init                     = {};
	notify_uninit_t                   notify_uninit                   = {};
	notify_notification_new_t         notify_notification_new         = {};
	notify_notification_set_timeout_t notify_notification_set_timeout = {};
	notify_notification_show_t        notify_notification_show        = {};
	gobject_unref_t                   g_object_unref                  = {};

	lib_t lib;
	open(lib, "libnotify.so.4", {}, err);

	link(lib, notify_init,                     "notify_init",                     err);
	link(lib, notify_uninit,                   "notify_uninit",                   err);
	link(lib, notify_notification_new,         "notify_notification_new",         err);
	link(lib, notify_notification_set_timeout, "notify_notification_set_timeout", err);
	link(lib, notify_notification_show,        "notify_notification_show",        err);
	link(lib, g_object_unref,                  "g_object_unref",                  err);

	if (err) { return; }

	const char* get_program_name (void);
	notify_init(get_program_name());
	auto nf = notify_notification_new(as_strz(title).ptr, as_strz(msg).ptr, "dialog-error");
	notify_notification_set_timeout(nf, NOTIFY_EXPIRES_NEVER);
	if (GError* nf_err = {}; notify_notification_show(nf, &nf_err)) {
		err = create_err("Error showing notification");
		if (nf_err) {
			if (nf_err->message) {
				err = err + create_err(nf_err->message);
			}
		}
	}
	g_object_unref(G_OBJECT(nf));
	notify_uninit();
	#endif

	#ifdef _WIN32
	if (MessageBox(NULL, as_wstr(msg).ptr, as_wstr(title).ptr, MB_OK | MB_ICONERROR | MB_TASKMODAL) == 0) {
		err = create_err("Displaying message box") + get_last_os_err();
	}
	#endif
}

define_test(error, "text")
{
	#ifdef __unix__
	prove_same(decode_os_err(EPERM    ).msg, "Operation not permitted");
	prove_same(decode_os_err(EHWPOISON).msg, "Memory page has hardware error");
	#endif

	#ifdef _WIN32
	prove_true(strstr(as_strz(decode_os_err(DWORD(ERROR_INVALID_FUNCTION    )).msg).ptr, "unction"));
	prove_true(strstr(as_strz(decode_os_err(DWORD(ERROR_FILE_NOT_FOUND      )).msg).ptr, "ile"));
	prove_true(strstr(as_strz(decode_os_err(DWORD(ERROR_CLASS_DOES_NOT_EXIST)).msg).ptr, "lass"));
	#endif

	return {};
}

