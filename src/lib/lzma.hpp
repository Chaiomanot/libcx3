#ifndef libcx3_lzma_hpp
#define libcx3_lzma_hpp
#include "prelude.hpp"

str_t lzma_dec (const str_t& src, nat8_t dst_len);
nat8_t lzma_dec_buf_len (const void_t* src_ptr, nat8_t src_len);
bool_t lzma_dec (void_t* buf_ptr, nat8_t buf_len,
					void_t* dst_ptr, nat8_t dst_len,
					const void_t* src_ptr, nat8_t src_len);

#endif

