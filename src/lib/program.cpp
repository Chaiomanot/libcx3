#include "program.hpp"
#include "text.hpp"
#include "error.hpp"
#ifdef __unix__
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#endif
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

str_t env_var (const str_t& key)
{
	if (!key) { return {}; }
	#ifdef __unix__
	return getenv(as_strz(key).ptr);
	#endif
	#ifdef _WIN32
	WCHAR buf[32'767] = {};
	if (GetEnvironmentVariable(as_wstr(key).ptr, buf, sizeof(buf)) > 0) {
		return create_str(buf);
	} else {
		assert_eq(GetLastError(), ERROR_ENVVAR_NOT_FOUND);
		return {};
	}
	#endif
}

void_t run_program (const seq_t<str_t>& args, err_t& err)
{
	if (err) { return; }
	if (!args) {
		err = create_err("No arguments");
		return;
	}

	#ifdef __unix__
	auto buf = create_seq<seq_t<char>>(args.len);
	for (auto i : create_range(args.len)) {
		buf[i] = as_strz(args[i]);
	}
	auto c_buf = create_seq<char*>(buf.len + 1);
	for (auto i : create_range(buf.len)) {
		c_buf[i] = buf[i].ptr;
	}

	pid_t pid = fork();
	if (pid == 0) {
		execvp(c_buf[0], c_buf.ptr);
		abort();
	}
	int wstatus = 0;
	pid_t pid2 = waitpid(pid, &wstatus, 0);
	if (pid2 != pid) {
		err = create_err("Error waiting for child execution");
	}
	#endif

	#ifdef _WIN32
	// Leave it to Microsoft to create an insensible encoding
	str_t cmd; {
		nat8_t i = 0;
		for (const auto& arg : args) {
			if (i > 0) { ++i; }
			i += 2;
			nat8_t prev_bs = 0;
			for (const auto g : arg) {
				if (g == '\"') { i += prev_bs + 1; }
				++i;
				prev_bs = (g == '\\' ? prev_bs + 1 : 0);
			}
		}
		cmd = create_str(i);
	}
	nat8_t i = 0;
	for (const auto& arg : args) {
		if (i > 0) { cmd[i++] = ' '; }
		cmd[i++] = '\"';
		nat8_t prev_bs = 0;
		for (const auto g : arg) {
			if (g == '\"') {
				cmd[i++] = '\\';
				while (prev_bs--) { cmd[i++] = '\\'; }
			}
			cmd[i++] = g;
			prev_bs = (g == '\\' ? prev_bs + 1 : 0);
		}
		cmd[i++] = '\"';
	}
	assert_eq(cmd.len, i);

	STARTUPINFO start_info = {};
	start_info.cb = sizeof(start_info);
	PROCESS_INFORMATION proc_info = {};
	if (!CreateProcess(as_wstr(args[0]).ptr, as_wstr(cmd).ptr, NULL, NULL, FALSE,
	                   0, NULL, NULL, &start_info, &proc_info)) {
		err = create_err("Couldn't create process") + decode_os_err(GetLastError());
		return;
	}
	if (WaitForSingleObject(proc_info.hProcess, INFINITE) != WAIT_OBJECT_0) {
		err = create_err("Error waiting for process to finish") + decode_os_err(GetLastError());
	}
	CloseHandle(proc_info.hProcess);
	CloseHandle(proc_info.hThread);
	#endif
}

