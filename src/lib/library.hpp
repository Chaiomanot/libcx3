#ifndef libcx3_library_hpp
#define libcx3_library_hpp
#include "prelude.hpp"

struct lib_t
{
	str_t  name {};
	nat8_t h    {};

	lib_t ();
	~lib_t ();
	lib_t (lib_t&& src);
	lib_t (const lib_t& src) = delete;
	lib_t& operator = (lib_t&& src);
	lib_t& operator = (const lib_t& src) = delete;

	explicit operator bool () const;
};

struct err_t;
void_t open (lib_t& lib, const str_t& lib_name,
				const str_t& env_var_name, err_t& e);
void_t* sym (lib_t& lib, const str_t& sym_name, err_t& e);

template<typename T> void_t link (lib_t& lib, T& ptr,
									const str_t& sym_name, err_t& e)
{
	ptr = reinterpret_cast<T>(sym(lib, sym_name, e));
}

#endif
