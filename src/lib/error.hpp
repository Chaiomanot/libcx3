#ifndef libcx3_error_hpp
#define libcx3_error_hpp
#include "prelude.hpp"

struct err_t
{
	str_t msg {};

	explicit operator bool_t () const;
};

err_t create_err (str_t msg);

str_t as_text (const err_t& err);
err_t operator + (const err_t& left, const err_t& right);

err_t decode_os_err (int code);
err_t decode_os_err (unsigned long code);

[[noreturn]] void_t terminate_prog ();
void_t display_err_msg (const str_t& title, const str_t& msg, err_t& err);

#endif

