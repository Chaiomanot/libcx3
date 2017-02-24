#ifndef libcx3_pipe_hpp
#define libcx3_pipe_hpp
#include "prelude.hpp"

struct pipe_t
{
	opaque_t h_in  {};
	opaque_t h_out {};

	pipe_t ();
	~pipe_t ();
	pipe_t (pipe_t&& src);
	pipe_t (const pipe_t& src) = delete;
	pipe_t& operator = (pipe_t&& src);
	pipe_t& operator = (const pipe_t& src) = delete;

	explicit operator bool () const;
};

struct pipe_pair_t
{
	pipe_t left  {};
	pipe_t right {};

	explicit operator bool () const;
};

struct err_t;
pipe_pair_t pipe_create (err_t& e);
str_t recv (pipe_t& p, err_t& e);
nat8_t send (pipe_t& p, const str_t& data, err_t& e);

#endif

