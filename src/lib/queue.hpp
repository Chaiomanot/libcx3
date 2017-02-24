#ifndef libcx3_queue_hpp
#define libcx3_queue_hpp
#include "prelude.hpp"
#include "thread.hpp"

struct queue_t
{
	sem_t        sem       {};
	mutex_t      read_mut  {};
	mutex_t      write_mut {};
	seq_t<str_t> buf       {};
	nat8_t     read_at   {};
	nat8_t     write_at  {};

	explicit operator bool () const;
};

struct queue_pair_t
{
	queue_t left  {};
	queue_t right {};
};

str_t recv_waited (queue_t& q);
str_t recv (queue_t& q);
void_t send (queue_t& q, str_t msg);

#endif

