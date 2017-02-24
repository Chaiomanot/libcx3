#ifndef libcx3_log_hpp
#define libcx3_log_hpp
#include "prelude.hpp"

enum class log_sev_t
{
	info = 0,
	warn = 1,
	err  = 2,
	crit = 3,
};

void_t log (log_sev_t sev, str_t& msg);

#endif

