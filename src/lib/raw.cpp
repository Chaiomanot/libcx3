#include "raw.hpp"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

void_t* alloc_mem (nat8_t len)
{
	assert_gt(len, 0);

	if (auto ptr = calloc(len, 1); ptr) {
		return ptr;
	} else {
		fprintf(stderr, "Couldn't allocate %llu bytes on the heap\n",
				static_cast<unsigned long long int>(len));
		abort();
	}
}

void_t free_mem (void_t* ptr, nat8_t len)
{
	assert_true(ptr);
	assert_gt(len, 0);

	unused(len);
	free(ptr);
}

void_t copy_mem (void_t* dst, const void_t* src, nat8_t len)
{
	if (len == 0) {
		return;
	}
	assert_true(dst);
	assert_true(src);
	memmove(dst, src, len);
}

bool_t is_mem_eq (const void_t* left_ptr, nat8_t left_len, const void_t* right_ptr, nat8_t right_len)
{
	if (left_len != right_len) { return false; }
	return memcmp(left_ptr, right_ptr, left_len) == 0;
}

