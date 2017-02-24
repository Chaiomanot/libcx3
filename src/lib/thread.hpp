#ifndef libcx3_thread_hpp
#define libcx3_thread_hpp
#include "prelude.hpp"

struct sem_t
{
	opaque_t opaq {};

	sem_t ();
	~sem_t ();
	sem_t (sem_t&& ori);
	sem_t (const sem_t& ori) = delete;
	sem_t& operator = (sem_t&& ori);
	sem_t& operator = (const sem_t& ori) = delete;
};

void_t signal (sem_t& sem);
void_t wait (sem_t& sem);

struct mutex_t
{
	opaque_t opaq {};

	mutex_t ();
	~mutex_t ();
	mutex_t (mutex_t&& ori);
	mutex_t (const mutex_t& ori) = delete;
	mutex_t& operator = (mutex_t&& ori);
	mutex_t& operator = (const mutex_t& ori) = delete;
};

struct mutex_lock_t
{
	mutex_t* mutex {};

	mutex_lock_t ();
	~mutex_lock_t ();
	mutex_lock_t (mutex_lock_t&& ori);
	mutex_lock_t (const mutex_lock_t& ori) = delete;
	mutex_lock_t& operator = (mutex_lock_t&& ori);
	mutex_lock_t& operator = (const mutex_lock_t& ori) = delete;
};

mutex_lock_t acquire (mutex_t& mutex);

struct err_t;
void_t spawn_thread_raw_param (void_t (*entry) (void_t* param), void_t* arg, err_t& err);
void_t wait_for_threads ();

template<typename T> void_t spawn_thread (void_t (*entry) (T& param), T& arg, err_t& err)
{
	return spawn_thread_raw_param(reinterpret_cast<void_t (*) (void_t* param)>(entry), &arg, err);
}

#endif

