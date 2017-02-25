#include "time.hpp"
#include "error.hpp"
#include "text.hpp"
#include "raw.hpp"
#ifdef __unix__
#include <unistd.h>
#include <sys/timerfd.h>
#include <fcntl.h>
#include <errno.h>
#endif
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreserved-id-macro"
#define _WIN32_WINNT 0x0601
#pragma clang diagnostic pop
#include <windows.h>
#endif
#include <time.h>

const nat8_t nanosec_mult  =                  1'000;
const nat8_t clunk_mult    = nanosec_mult  *    100;
const nat8_t microsec_mult = nanosec_mult  *  1'000;
const nat8_t millisec_mult = microsec_mult *  1'000;
const nat8_t sec_mult      = millisec_mult *  1'000;
const nat8_t day_mult      = sec_mult      * 86'400;

#ifndef NDEBUG
bool_t valid (inter_t inter)
{
	return inter.picos < day_mult;
}
#endif

nat8_t divide (inter_t inter, nat8_t divisor)
{
	assert_true(valid(inter));

	nat8_t val_days = inter.days * (day_mult / divisor);
	if (val_days / (day_mult / divisor) != inter.days) { return max<nat8_t>(); }
	nat8_t val_picos = (inter.picos + ((divisor / 2) - 1)) / divisor;

	if (val_days + val_picos < val_days) { return max<nat8_t>(); }
	return val_days + val_picos;
}

nat8_t divide_day (inter_t inter, nat8_t modulor, nat8_t divisor)
{
	assert_true(valid(inter));

	return ((inter.picos % modulor) + ((divisor / 2) - 1)) / divisor;
}

inter_t create_inter_of_mult (nat8_t val, nat8_t mult)
{
	inter_t inter;
	inter.days  =  val / (day_mult / mult);
	inter.picos = (val % (day_mult / mult)) * mult;
	return inter;
}

nat1_t get_relation (inter_t left, inter_t right)
{
	if (left.days > right.days) { return 2; }
	if (left.days < right.days) { return 0; }
	if (left.picos > right.picos) { return 2; }
	if (left.picos < right.picos) { return 0; }
	return 1;
}

bool_t operator == (inter_t left, inter_t right) { return get_relation(left, right) == 1; }
bool_t operator != (inter_t left, inter_t right) { return get_relation(left, right) != 1; }
bool_t operator >  (inter_t left, inter_t right) { return get_relation(left, right) >  1; }
bool_t operator <  (inter_t left, inter_t right) { return get_relation(left, right) <  1; }
bool_t operator >= (inter_t left, inter_t right) { return get_relation(left, right) >= 1; }
bool_t operator <= (inter_t left, inter_t right) { return get_relation(left, right) <= 1; }

inter_t operator + (inter_t inter, inter_t addend)
{
	assert_true(valid(inter));
	assert_true(valid(addend));

	inter_t prod;
	prod.days  = inter.days  + addend.days;
	prod.picos = inter.picos + addend.picos;
	if (prod.picos >= day_mult) { // overflow
		prod.picos -= day_mult;
		++prod.days;
	}
	return prod;
}

inter_t operator - (inter_t inter, inter_t subtrahend)
{
	assert_true(valid(inter));
	assert_true(valid(subtrahend));
	assert_true(inter >= subtrahend);

	inter_t prod;
	if (inter > subtrahend) {
		prod.days  = inter.days  - subtrahend.days;
		prod.picos = inter.picos - subtrahend.picos;
		if (prod.picos >= day_mult) { // underflow
			prod.picos += day_mult;
			--prod.days;
		}
	}
	return prod;
}

nat8_t get_secs      (inter_t inter) { return divide(inter, sec_mult);      }
nat8_t get_millisecs (inter_t inter) { return divide(inter, millisec_mult); }
nat8_t get_clunks    (inter_t inter) { return divide(inter, clunk_mult);    }
nat8_t get_nanosecs  (inter_t inter) { return divide(inter, nanosec_mult);  }

nat8_t get_sec_millisecs (inter_t inter) { return divide_day(inter, sec_mult, millisec_mult); }
nat8_t get_sec_nanosecs  (inter_t inter) { return divide_day(inter, sec_mult, nanosec_mult);  }

inter_t create_inter_of_secs      (nat8_t secs)      { return create_inter_of_mult(secs,      sec_mult);      }
inter_t create_inter_of_millisecs (nat8_t millisecs) { return create_inter_of_mult(millisecs, millisec_mult); }
inter_t create_inter_of_clunks    (nat8_t clunks)    { return create_inter_of_mult(clunks,    clunk_mult);    }
inter_t create_inter_of_nanosecs  (nat8_t nanosecs)  { return create_inter_of_mult(nanosecs,  nanosec_mult);  }

bool_t operator == (date_t left, date_t right) { return left.val == right.val; }
bool_t operator != (date_t left, date_t right) { return left.val != right.val; }
bool_t operator >  (date_t left, date_t right) { return left.val >  right.val; }
bool_t operator <  (date_t left, date_t right) { return left.val <  right.val; }
bool_t operator >= (date_t left, date_t right) { return left.val >= right.val; }
bool_t operator <= (date_t left, date_t right) { return left.val <= right.val; }

date_t operator + (date_t date, inter_t addend)
{
	date_t prod;
	prod.val = date.val + addend;
	return prod;
}

date_t operator - (date_t date, inter_t subtrahend)
{
	date_t prod;
	prod.val = date.val - subtrahend;
	return prod;
}

inter_t operator - (date_t date, date_t subtrahend)
{
	return date.val - subtrahend.val;
}

static const date_t unix_epoch = {{2'440'587, 0}};
#ifdef _WIN32
static const date_t win_epoch  = {{2'305'813, 0}};
#endif

date_t create_date_of_unix_time (nat8_t unix)
{
	return unix_epoch + create_inter_of_secs(unix);
}

nat8_t as_unix_time (date_t date)
{
	if (date < unix_epoch) { return {}; }
	const auto secs = get_secs(date - unix_epoch);
	if (secs > max<time_t>()) { return {}; }
	return secs;
}

#ifdef __unix__
inter_t create_inter (const timespec& ts)
{
	assert_gteq(ts.tv_nsec, 0);
	if (ts.tv_sec < 0) { return {}; }

	return create_inter_of_secs(static_cast<nat8_t>(ts.tv_sec)) +
	       create_inter_of_nanosecs(static_cast<nat8_t>(ts.tv_nsec));
}

date_t create_date (const timespec& ts)
{
	return unix_epoch + create_inter(ts);
}

timespec as_timespec (inter_t inter)
{
	auto secs  = get_secs(inter);
	auto nanos = get_sec_nanosecs(inter);
	if (secs > max<time_t>()) { secs = max<time_t>(); nanos = 999'999'999; }

	timespec ts;
	ts.tv_sec  = static_cast<time_t>(secs);
	ts.tv_nsec = static_cast<long>(nanos);
	return ts;
}

timespec as_timespec (date_t date)
{
	return as_timespec(date - unix_epoch);
}

timespec create_omission_timespec ()
{
	timespec ts;
	ts.tv_sec  = 0;
	ts.tv_nsec = UTIME_OMIT;
	return ts;
}
#endif

#ifdef _WIN32
inter_t create_inter (const FILETIME& file)
{
	ULARGE_INTEGER large;
	large.LowPart  = file.dwLowDateTime;
	large.HighPart = file.dwHighDateTime;
	if (large.QuadPart >= max<LONGLONG>()) { large.QuadPart = max<LONGLONG>(); }
	return create_inter_of_clunks(large.QuadPart);
}

date_t create_date (const FILETIME& file)
{
	return win_epoch + create_inter(file);
}

FILETIME as_filetime (inter_t inter)
{
	ULARGE_INTEGER large;
	large.QuadPart = get_clunks(inter);

	FILETIME file;
	file.dwLowDateTime  = large.LowPart;
	file.dwHighDateTime = large.HighPart;
	return file;
}

FILETIME as_filetime (date_t date)
{
	if (date < win_epoch) { return {}; }

	return as_filetime(date - win_epoch);
}
#endif

inter_t get_current_inter ()
{
	#ifdef __unix__
	#ifdef __linux__
	const clockid_t clock = CLOCK_MONOTONIC_RAW;
	#else
	const clockid_t clock = CLOCK_MONOTONIC; // CLOCK_MONOTONIC_COARSE
	#endif
	timespec ts = {};
	auto stat = clock_gettime(clock, &ts);
	assert_eq(stat, 0);
	unused(stat);
	return create_inter(ts);
	#endif

	#ifdef _WIN32
	ULONGLONG intr = 0;
	QueryUnbiasedInterruptTime(&intr); // QueryUnbiasedInterruptTimePrecise
	assert_lt(intr, max<LONGLONG>());
	return create_inter_of_clunks(intr);
	#endif
}

date_t get_current_date ()
{
	#ifdef __unix__
	timespec ts = {};
	auto stat = clock_gettime(CLOCK_REALTIME, &ts);
	assert_eq(stat, 0);
	unused(stat);
	return create_date(ts);
	#endif

	#ifdef _WIN32
	SYSTEMTIME sys = {};
	GetSystemTime(&sys);
	FILETIME file = {};
	auto succ = SystemTimeToFileTime(&sys, &file);
	assert_true(succ);
	unused(succ);
	return create_date(file);
	#endif
}

str_t as_text (date_t date, err_t& err)
{
	const auto secs = as_unix_time(date);
	if (create_date_of_unix_time(secs) != date) {
		err = create_err("Out of Unix time integer range");
		return "0000 ---- 00 --- 00:00:00 --";
	}

	tm calendar_time = {};
	const auto unix = static_cast<time_t>(secs);
	#ifdef __unix__
	gmtime_r(&unix, &calendar_time);
	#endif
	#ifdef _WIN32
	gmtime_s(&calendar_time, &unix);
	#endif

	char buf[64] = {};
	strftime(buf, 64, "%Y %b %d %a %I:%M:%S %p", &calendar_time);
	if (!buf[0]) {
		err = create_err("Unable to format time");
	}
	return buf;
}

#ifdef __unix__
int get_fd (opaque_t opaq);
opaque_t create_opaque_fd (int fd);
#endif
#ifdef _WIN32
HANDLE get_handle (opaque_t opaq);
opaque_t create_opaque_handle (HANDLE h);
#endif

metronome_t::metronome_t () { }

metronome_t::~metronome_t ()
{
	if (!opaq) { return; }
	#ifdef __unix__
	close(get_fd(opaq));
	#endif
	#ifdef _WIN32
	CloseHandle(get_handle(opaq));
	#endif
}

metronome_t::metronome_t (metronome_t&& ori) { *this = move(ori); }
metronome_t& metronome_t::operator = (metronome_t&& ori)
{
	if (&ori != this) {
		this->~metronome_t();
		opaq = ori.opaq;
		ori.opaq = {};
	}
	return *this;
}

metronome_t::operator bool_t () const
{
	return bool_t(opaq);
}

metronome_t metronome_create (err_t& err)
{
	if (err) { return {}; }

	#ifdef __unix__
	if (const auto fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK); fd >= 0) {
		metronome_t met;
		met.opaq = create_opaque_fd(fd);
		return met;
	} else {
		err = decode_os_err(errno);
		return {};
	}
	#endif

	#ifdef _WIN32
	if (auto handle = CreateWaitableTimer(NULL, FALSE, NULL)) {
		metronome_t met;
		met.opaq = create_opaque_handle(handle);
		return met;
	} else {
		err = decode_os_err(GetLastError());
		return {};
	}
	#endif
}

void_t set_freq (metronome_t& met, inter_t inter, err_t& err)
{
	if (err) { return; }

	if (inter > inter_t{}) {

		#ifdef __unix__
		itimerspec its;
		its.it_value = as_timespec(inter);
		if (its.it_value.tv_sec == 0 && its.it_value.tv_nsec == 0) { its.it_value.tv_nsec = 1; }
		its.it_interval = its.it_value;

		if (timerfd_settime(get_fd(met.opaq), 0, &its, nullptr) != 0) {
			err = decode_os_err(errno);
		}
		#endif

		#ifdef _WIN32
		const auto clunks = get_clunks(inter);
		const auto ms     = get_millisecs(inter);
		if (clunks > max<LONGLONG>() || max<LONG>()) {
			err = create_err("Interval too large");
			return;
		}
		LARGE_INTEGER due;
		due.QuadPart = -static_cast<LONGLONG>(clunks);
		if (due.QuadPart == 0) { due.QuadPart = -1; }
		auto period = static_cast<LONG>(ms);
		if (period == 0 && inter > inter_t{}) { period = 1; }

		if (!SetWaitableTimer(get_handle(met.opaq), &due, period, NULL, NULL, FALSE)) {
			err = decode_os_err(GetLastError());
		}
		#endif

	} else if (inter == inter_t{}) {

		#ifdef __unix__
		itimerspec its = {};
		if (timerfd_settime(get_fd(met.opaq), 0, &its, nullptr) != 0) {
			err = decode_os_err(errno);
		}
		#endif

		#ifdef _WIN32
		if (!CancelWaitableTimer(get_handle(met.opaq))) {
			err = decode_os_err(GetLastError());
		}
		#endif

	} else {
		err = create_err("Interval is negative");
	}
}

void_t recv (metronome_t& met, err_t& err)
{
	if (err) { return; }

	#ifdef __unix__
	nat8_t buf = 0;
	const auto stat = read(get_fd(met.opaq), &buf, sizeof(buf));
	if (stat >= 0) {
		assert_eq(stat, sizeof(buf));
	} else {
		assert_eq(stat, -1);
		static_assert(EAGAIN == EWOULDBLOCK);
		if (errno != EAGAIN) {
			err = decode_os_err(errno);
		}
	}
	#endif

	#ifdef _WIN32
	// we don't have to read from timers on windows
	unused(met);
	#endif
}

define_test(time, "text,error")
{
	err_t err;
	prove_same(as_text(unix_epoch, err),                                    "1970 Jan 01 Thu 12:00:00 AM");
	prove_same(as_text(unix_epoch + create_inter_of_secs(1), err),          "1970 Jan 01 Thu 12:00:01 AM");
	prove_same(as_text(unix_epoch + create_inter_of_secs(1234567890), err), "2009 Feb 13 Fri 11:31:30 PM");
	prove_false(err);
	prove_same(as_text(date_t{}, err), "");
	prove_true(err);

	return {};
}

