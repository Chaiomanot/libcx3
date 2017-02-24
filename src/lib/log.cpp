#include "log.hpp"
#include "text.hpp"
#ifdef __unix__
#include <syslog.h>
#endif
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#ifdef __unix__
int unix (log_sev_t sev)
{
	switch (sev) {
		case log_sev_t::info: return LOG_INFO;
		case log_sev_t::warn: return LOG_WARNING;
		case log_sev_t::err:  return LOG_ERR;
		case log_sev_t::crit: return LOG_ALERT;
	}
}
#endif

#ifdef _WIN32
WORD win (log_sev_t sev)
{
	switch (sev) {
		case log_sev_t::info: return EVENTLOG_INFORMATION_TYPE;
		case log_sev_t::warn: return EVENTLOG_WARNING_TYPE;
		case log_sev_t::err:  return EVENTLOG_ERROR_TYPE;
		case log_sev_t::crit: return EVENTLOG_ERROR_TYPE;
	}
}
#endif

#ifdef _WIN32
HANDLE ev_src;
#endif

void_t log (log_sev_t sev, str_t& msg)
{
	#ifdef __unix__
	syslog(unix(sev), "%s", as_strz(msg).ptr);
	#endif

	#ifdef _WIN32
	LPCTSTR strs[2] = {wstr(msg).ptr, NULL};
	const auto succ = ReportEvent(ev_src, win(sev), 0, 0,
									NULL, 1, 0, strs, NULL);
	assert_true(succ);
	unused(succ);
	#endif
}

void_t log_open (const char* prog_name)
{
	#ifdef __unix__
	openlog(prog_name, LOG_CONS, LOG_USER);
	#endif

	#ifdef _WIN32
	ev_src = RegisterEventSource(NULL, wstr(prog_name).ptr);
	assert_true(ev_src);
	#endif
}

void_t log_close ()
{
	#ifdef __unix__
	closelog();
	#endif

	#ifdef _WIN32
	DeregisterEventSource(ev_src);
	#endif
}

