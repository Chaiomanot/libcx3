#include "prelude.hpp"
#include "text.hpp"
#include "thread.hpp"
#include <cstdlib>
#include <cstdio>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

static_assert(max<signed char>() == 0x7FULL);
static_assert(max<unsigned short>() == 0xFFFFULL);

nat8_t clamp (nat8_t val, nat8_t lo, nat8_t hi)
{
	assert_lteq(lo, hi);
	return val < lo ? lo : val > hi ? hi : val;
}

rat4_t clamp (rat4_t val, rat4_t lo, rat4_t hi)
{
	assert_lteq(lo, hi);
	return val < lo ? lo : val > hi ? hi : val;
}

rat8_t clamp (rat8_t val, rat8_t lo, rat8_t hi)
{
	assert_lteq(lo, hi);
	return val < lo ? lo : val > hi ? hi : val;
}

nat8_t range_iter_t::operator * () { return val; }
range_iter_t& range_iter_t::operator ++ () { ++val; return *this; }

range_iter_t begin (const range_t& range) { unused(range); return {}; }
range_iter_t end (const range_t& range) { return range.it; }
bool_t operator != (const range_iter_t& left, const range_iter_t& right) { return left.val != right.val; }

range_t create_range (nat8_t end)
{
	range_t iter;
	iter.it.val = end;
	return iter;
}

opaque_t::operator bool_t () const { return val != 0; }

#ifdef __unix__
int get_fd (opaque_t opaq) { return static_cast<int>(opaq.val) - 1; }
opaque_t create_opaque_fd (int fd)
{
	opaque_t opaq;
	opaq.val = static_cast<nat8_t>(fd + 1);
	return opaq;
}
#endif

#ifdef _WIN32
HANDLE get_handle (opaque_t opaq) { return reinterpret_cast<HANDLE>(opaq.val); }
opaque_t create_opaque_handle (HANDLE h)
{
	opaque_t opaq;
	opaq.val = reinterpret_cast<nat8_t>(h);
	return opaq;
}
#endif

void_t* get_ptr (opaque_t opaq) { return reinterpret_cast<void_t*>(opaq.val); }
opaque_t create_opaque_ptr (void_t* ptr)
{
	opaque_t opaq;
	opaq.val = reinterpret_cast<nat8_t>(ptr);
	return opaq;
}

const char*  prog_name;
nat8_t       args_n;
const char** args_ptr;

const char* get_program_name () { return prog_name ? prog_name : ""; }

void_t begin_main (const char* name, int argc, const char** argv)
{
	assert_gt(argc, 0);
	prog_name = name;
	args_n    = static_cast<nat8_t>(argc);
	args_ptr  = argv;

	#ifndef NTEST
	void_t run_tests ();
	run_tests();
	#endif

	void_t log_open (const char* prog_name);
	log_open(prog_name);
}

void_t end_main ()
{
	wait_for_threads();

	void_t log_close ();
	log_close();
}

