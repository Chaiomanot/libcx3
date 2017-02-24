#ifndef libcx3_program_hpp
#define libcx3_program_hpp
#include "prelude.hpp"

str_t env_var (const str_t& key);

struct err_t;
void_t run_program (const seq_t<str_t>& args, err_t& e);

#endif

