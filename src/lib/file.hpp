#ifndef libcx3_file_hpp
#define libcx3_file_hpp
#include "prelude.hpp"

struct path_t
{
	seq_t<str_t> cos {};

	explicit operator bool_t () const;
};

path_t create_path (const str_t& text);
path_t clone (const path_t& path);
path_t operator + (const path_t& path, const str_t& right);
str_t as_text (const path_t& path);
str_t as_text (const path_t& path, const str_t& delim);
path_t get_dir (const path_t& path);
str_t get_ext (const path_t& path);
void_t set_ext (path_t& path, const str_t& ext);

struct err_t;
path_t get_working_dir (err_t& err);
void_t set_working_dir (const path_t& path, err_t& err);
path_t create_temp_path (err_t& err);

struct file_t
{
	opaque_t opaq {};

	file_t ();
	~file_t ();
	file_t (const file_t& ori) = delete;
	file_t& operator = (const file_t& ori) = delete;
	file_t (file_t&& ori);
	file_t& operator = (file_t&& ori);

	explicit operator bool_t () const;
};

file_t open_file (const path_t& path, bool_t writing, err_t& err);
str_t read (file_t& file, nat8_t len, err_t& err);
void_t write (file_t& file, const str_t& data, err_t& err);
void_t set_cursor (file_t& file, nat8_t at, err_t& err);

struct date_t;
date_t read_last_mod (file_t& file, err_t& err);
void_t write_last_mod (file_t& file, const date_t& date, err_t& err);

#endif

