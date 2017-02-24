#ifndef libcx3_text_hpp
#define libcx3_text_hpp
#include "prelude.hpp"

str_t create_str (nat8_t len);
str_t create_str (const void_t* ptr, nat8_t len);

str_t clone (const str_t& text);

bool_t operator == (const str_t& left, const str_t& right);
bool_t operator == (const str_t& left, const char*  right);
bool_t operator == (const char*  left, const str_t& right);
bool_t operator != (const str_t& left, const str_t& right);
bool_t operator != (const str_t& left, const char*  right);
bool_t operator != (const char*  left, const str_t& right);

str_t operator + (const str_t& left, const str_t& right);
str_t operator + (const str_t& left, const char*  right);
str_t operator + (const char*  left, const str_t& right);

str_t quote (const str_t& text);

str_t as_text (nat8_t n);
str_t as_text (nat4_t n);
str_t as_text (nat2_t n);
str_t as_text (nat1_t n);
str_t as_text (rat8_t n);
str_t as_text (rat4_t n);

nat8_t decode_nat (const str_t& str, nat8_t& i);
rat8_t decode_rat (const str_t& str, nat8_t& i);

str_t create_str (const char* src);
seq_t<char> as_strz (const str_t& text);

str_t as_str (const wchar_t* wstr);
seq_t<wchar_t> as_wstr (const str_t& str);

str_t get_line_sep ();

struct err_t;
void_t print (const str_t& text, err_t& err);

#endif

