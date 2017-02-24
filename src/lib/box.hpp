#ifndef libcx3_box_hpp
#define libcx3_box_hpp
#include "prelude.hpp"

template<typename el_t> struct box_t
{
	el_t* ptr {};

	box_t () { }
	~box_t ()
	{
		void_t free_mem (void_t* ptr, nat8_t len);

		if (!ptr) { return; }

		ptr->~el_t();
		free_mem(ptr, sizeof(el_t));
		ptr = nullptr;
	}

	box_t (const box_t<el_t>& ori) = delete;
	box_t<el_t>& operator = (const box_t<el_t>& ori) = delete;
	box_t (box_t<el_t>&& ori) { *this = move(ori); }
	box_t<el_t>& operator = (box_t<el_t>&& ori)
	{
		if (&ori != this) {
			this->~box_t();
			ptr = ori.ptr;
			ori.ptr = nullptr;
		}
		return *this;
	}

	el_t* operator * ()
	{
		void_t* alloc_mem (nat8_t len);

		if (ptr) { return ptr; }

		assert_init_zero<el_t>();
		ptr = static_cast<el_t*>(alloc_mem(sizeof(el_t)));
		return ptr;
	}

	el_t* operator -> ()
	{
		return **this;
	}

	explicit operator bool_t () const
	{
		return ptr;
	}
};

template<typename el_t> void_t acquire (box_t<el_t>& box, el_t* ptr)
{
	box.~box_t();
	box.ptr = ptr;
}

template<typename el_t> el_t* release (box_t<el_t>& box)
{
	auto ptr = *box;
	box.ptr = nullptr;
	return ptr;
}

#endif

