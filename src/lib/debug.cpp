#if !defined(NDEBUG) || !defined(NTEST)
#include "prelude.hpp"
#include "text.hpp"
#include "time.hpp"
#include "error.hpp"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#ifndef NTEST
struct dev_test_t
{
	const char* (*func) () {};
	const char*   name     {};
	const char*   deps     {};
	bool_t        ran      {};
	bool_t        passed   {};
	pad_t<6>      padding  {};
};

nat8_t      tests_len;
dev_test_t* tests_ptr;
bool_t      test_failure;
#endif

const char* describe_bug (const char* file, int line, const char* op,
                          nat8_t left_val, nat8_t right_val, const char* left_expr, const char* right_expr)
{
	static char buf[2048];
	auto buf_i = snprintf(buf, sizeof(buf), " at %s line %d: ", file, line);

	if (strcmp(op, "0") == 0) {
		const auto ptr = reinterpret_cast<const nat1_t*>(left_val);
		const auto len = static_cast<int>(right_val);
		buf_i += snprintf(&buf[buf_i], sizeof(buf) - static_cast<nat8_t>(buf_i),
		                   "%d%s", len, "-byte datum isn't properly initialized:");
		for (int i = 0; i < len; ++i) {
			#ifdef __unix__
			const char* line_sep = "\n";
			#endif
			#ifdef _WIN32
			const char* line_sep = "\r\n";
			#endif
			if (static_cast<nat8_t>(buf_i) + 6 + 7 > sizeof(buf)) {
				buf_i += snprintf(&buf[buf_i], sizeof(buf) - static_cast<nat8_t>(buf_i), "%s%s", line_sep, "...");
				break;
			}
			if (i % 8 == 0) {
				buf_i += snprintf(&buf[buf_i], sizeof(buf) - static_cast<nat8_t>(buf_i), "%s%4d. ",
				                   line_sep, (i / 8) + 1);
			}
			buf_i += snprintf(&buf[buf_i], sizeof(buf) - static_cast<nat8_t>(buf_i), "0x%02X,", ptr[i]);
		}

	} else if (strcmp(op, "*") == 0) {
		snprintf(&buf[buf_i], sizeof(buf) - static_cast<nat8_t>(buf_i),
		         "\"%s\" isn't the same as \"%s\" in (%s == %s)",
		         reinterpret_cast<const char*>(left_val), reinterpret_cast<const char*>(right_val),
		         left_expr, right_expr);

	} else if (strcmp(op, "") == 0) {
		snprintf(&buf[buf_i], sizeof(buf) - static_cast<nat8_t>(buf_i),
		         "%lld/0x%llx in (%s) is invalid",
		         static_cast<long long>(left_val), static_cast<unsigned long long>(left_val), left_expr);

	} else if (strcmp(op, "T") == 0) {
		snprintf(&buf[buf_i], sizeof(buf) - static_cast<nat8_t>(buf_i), "(%s) isn't true", left_expr);

	} else if (strcmp(op, "F") == 0) {
		snprintf(&buf[buf_i], sizeof(buf) - static_cast<nat8_t>(buf_i), "(%s) isn't false", left_expr);

	} else {
		snprintf(&buf[buf_i], sizeof(buf) - static_cast<nat8_t>(buf_i),
		         "%lld/0x%llx %s %lld/0x%llx isn't true in (%s %s %s)",
		         static_cast<long long>(left_val),  static_cast<unsigned long long>(left_val), op,
		         static_cast<long long>(right_val), static_cast<unsigned long long>(right_val),
		         left_expr, op, right_expr);
	}
	return buf;
}

nat2_t nativize_term_color (const char* color)
{
	#ifdef __unix__
	#define match(name, uni_code, win_code) \
		if (strcmp(color, name) == 0) { return uni_code; } laze_stmt()
	#endif
	#ifdef _WIN32
	#define match(name, uni_code, win_code) \
		if (strcmp(color, name) == 0) { return win_code; } laze_stmt()
	#endif

	match("red",    31, FOREGROUND_RED);
	match("green",  32, FOREGROUND_GREEN);
	match("yellow", 33, FOREGROUND_RED | FOREGROUND_GREEN);

	#undef match
	return 0;
}

void_t echo_debug (const char* head, const char* body, const char* color)
{
	#ifdef __unix__
	const auto color_code = static_cast<unsigned int>(nativize_term_color(color));
	fprintf(stderr, "\x1B[1;%um%s\x1B[0m%s\n", color_code, head, body);
	#endif

	#ifdef _WIN32
	HANDLE term = GetStdHandle(STD_ERROR_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO info = {};
	GetConsoleScreenBufferInfo(term, &info);
	SetConsoleTextAttribute(term, nativize_term_color(color));
	fprintf(stderr, "%s", head);
	SetConsoleTextAttribute(term, info.wAttributes);
	fprintf(stderr, "%s\r\n", body);
	#endif
}

#ifndef NDEBUG

#ifdef __unix__
void_t* operator new (unsigned long size, void_t* ptr) { unused(size); return ptr; }
#endif
#ifdef _WIN32
void_t* operator new (unsigned long long size, void_t* ptr) { unused(size); return ptr; }
#endif

[[noreturn]] void_t fail_assertion (const char* msg)
{
	const char* title = "Assertion failure";

	#ifndef NTEST
	if (tests_ptr) {
		echo_debug(title, msg, "red");
		terminate_prog();
	}
	#endif
	char buf[2048];
	snprintf(buf, sizeof(buf), "%s%s", title, msg);
	err_t err;
	display_err_msg(title, buf, err);

	terminate_prog();
}

#endif
#ifndef NTEST

test_result_t fail_proof (const char* msg)
{
	return msg;
}

void_t install_test (test_result_t (*func) (), const char* name, const char* deps)
{
	dev_test_t test;
	test.func = func;
	test.name = name;
	test.deps = deps;

	const auto new_sz = (tests_len + 1) * sizeof(dev_test_t);
	tests_ptr = static_cast<dev_test_t*>(realloc(tests_ptr, new_sz));
	if (!tests_ptr) {
		echo_debug("Realloc failed", "", "red");
		terminate_prog();
	}
	tests_ptr[tests_len++] = move(test);
}

nat8_t find_test (const dev_test_t& parent, const char* name, nat8_t name_len)
{
	for (nat8_t i = 0; i < tests_len; ++i) {
		if (strlen(tests_ptr[i].name) == name_len) {
			if (memcmp(tests_ptr[i].name, name, name_len) == 0) {
				return i;
			}
		}
	}

	char buf[2048];
	snprintf(buf, sizeof(buf), ", \"%.*s\", dependency of \"%s\"", static_cast<int>(name_len), name, parent.name);
	echo_debug("Couldn't find test", buf, "red");
	terminate_prog();
}

void_t assert_no_loop (const dev_test_t& test, const char* srcs[], nat8_t src_i)
{
	for (decltype(src_i) i = 0; i < src_i; ++i) {
		if (strcmp(srcs[i], test.name) == 0) {

			char buf[2048];
			auto buf_i = snprintf(buf, sizeof(buf), ": ");
			for (; i < src_i; ++i) {
				snprintf(&buf[buf_i], sizeof(buf) - static_cast<nat8_t>(buf_i), "%s -> ", srcs[i]);
			}
			snprintf(&buf[buf_i], sizeof(buf) - static_cast<nat8_t>(buf_i), "%s", test.name);

			echo_debug("Test dependency loop", buf, "red");
			terminate_prog();
		}
	}
}

void_t run (dev_test_t& test, const char* srcs[], nat8_t src_i)
{
	if (test.ran) { return; }

	// push
	assert_no_loop(test, srcs, src_i);
	srcs[src_i++] = test.name;

	// look for and run dependencies first
	auto dep_fail = false;
	nat8_t last_i = 0;
	for (nat8_t i = 0; ; ++i) {
		if (test.deps[i] != ',' && test.deps[i] != '\0') { continue; }

		if (const auto dep_name_len = i - last_i; dep_name_len > 0) {
			const auto dep_i = find_test(test, &test.deps[last_i], dep_name_len);
			auto& dep = tests_ptr[dep_i];

			run(dep, srcs, src_i);
			if (!dep.passed) { dep_fail = true; break; }
		}

		last_i = i + 1;
		if (test.deps[i] == '\0') { break; }
	}

	// pop
	--src_i;

	char buf[2048];
	if (!dep_fail) {
		// run the test
		auto fail_msg = test.func();
		test.ran = true;
		if (!fail_msg) {
			test.passed = true;
			snprintf(buf, sizeof(buf), " %s", test.name);
			echo_debug("[-- PASSED --]", buf, "green");
		} else {
			snprintf(buf, sizeof(buf), " %s%s", test.name, fail_msg);
			echo_debug("[== FAILED ==]", buf, "red");
		}
	} else {
		snprintf(buf, sizeof(buf), " %s", test.name);
		echo_debug("[== NOTRUN ==]", buf, "yellow");
	}
}

void_t run_tests ()
{
	const auto begin_time = get_current_inter();

	auto srcs = static_cast<const char**>(malloc(tests_len * sizeof(char*)));
	if (!srcs) {
		echo_debug("Malloc failed", "", "red");
		terminate_prog();
	}
	nat8_t src_i = 0;
	for (decltype(tests_len) i = 0; i < tests_len; ++i) {
		run(tests_ptr[i], srcs, src_i);
	}
	free(srcs);

	free(tests_ptr);
	tests_ptr = {};

	if (!test_failure) {
		const auto end_time = get_current_inter();

		const auto secs    = get_secs(end_time - begin_time);
		const auto ms_secs = get_sec_millisecs(end_time - begin_time);

		char buf[256];
		snprintf(buf, sizeof(buf), "%llu %s in %lld.%03llds",
		         static_cast<unsigned long long>(tests_len), "developer unit tests passed",
		         static_cast<unsigned long long>(secs), static_cast<unsigned long long>(ms_secs));
		echo_debug("", buf, "");
	}
}

#endif
#endif

