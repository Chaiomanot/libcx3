#ifndef libcx3_raw_hpp
#define libcx3_raw_hpp
#include "prelude.hpp"

void_t* alloc_mem (nat8_t len);
void_t free_mem (void_t* ptr, nat8_t len);

void_t copy_mem (void_t* dst, const void_t* src, nat8_t len);

template<typename T> seq_t<T> clone_mem (const seq_t<T>& src)
{
	auto prod = create_seq<T>(src.len);
	copy_mem(prod.ptr, src.ptr, src.len);
	return prod;
}

bool_t is_mem_eq (const void_t* left_ptr, nat8_t left_len, const void_t* right_ptr, nat8_t right_len);

#endif

