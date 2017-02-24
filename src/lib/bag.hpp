#ifndef libcx3_bag_hpp
#define libcx3_bag_hpp
#include "prelude.hpp"

template<typename el_t> struct bag_t
{
	// bag_t is functionally a multiset
	// that always contains an infinite number of null elements

	seq_t<el_t> cells {};

	operator bool_t () const
	{
		for (auto& cell : cells) {
			if (cell) {
				return true;
			}
		}
		return false;
	}
};

template<typename el_t> void_t insert (bag_t<el_t>& bag, el_t el)
{
	for (auto& cell : bag.cells) {
		if (!cell) {
			cell = move(el);
			return;
		}
	}
	const auto dest_i = bag.cells.len;
	grow(bag.cells, bag.cells.len, clamp(bag.cells.len, 1, max<nat8_t>()));
	bag.cells[dest_i] = move(el);
}

template<typename el_t> void_t remove (bag_t<el_t>& bag, el_t& el)
{
	assert_gteq(reinterpret_cast<nat8_t>(&el), reinterpret_cast<nat8_t>(bag.cells.ptr));
	assert_lt(  reinterpret_cast<nat8_t>(&el), reinterpret_cast<nat8_t>(&bag.cells[bag.cells.len]));

	unused(bag);
	el = {};
}

template<typename el_t> struct bag_iter_t
{
	el_t* at  {};
	el_t* end {};

	el_t& operator * ()
	{
		assert_true(at);
		assert_true(end);

		return *at;
	}

	bag_iter_t<el_t>& operator ++ ()
	{
		assert_true(at);
		assert_true(end);

		if (at != end) { ++at; }
		skip_unoccupied_cells(*this);

		return *this;
	}
};

template<typename el_t> void skip_unoccupied_cells (bag_iter_t<el_t>& iter)
{
	while (iter.at != iter.end) {
		if (*iter.at) {
			break;
		} else {
			++iter.at;
		}
	}
}

template<typename el_t> bag_iter_t<el_t> begin_bag_iter (el_t* ptr, nat8_t len)
{
	bag_iter_t<el_t> iter;
	iter.at  =  ptr;
	iter.end = &ptr[len];
	skip_unoccupied_cells(iter);
	return iter;
}

template<typename el_t> bag_iter_t<el_t> end_bag_iter (el_t* ptr, nat8_t len)
{
	bag_iter_t<el_t> iter;
	iter.at = iter.end = &ptr[len];
	return iter;
}

template<typename el_t> bag_iter_t<el_t> begin (bag_t<el_t>& bag)
{
	return begin_bag_iter(bag.cells.ptr, bag.cells.len);
}

template<typename el_t> bag_iter_t<el_t> end (bag_t<el_t>& bag)
{
	return end_bag_iter(bag.cells.ptr, bag.cells.len);
}

template<typename el_t> bag_iter_t<const el_t> begin (const bag_t<el_t>& bag)
{
	return begin_bag_iter(bag.cells.ptr, bag.cells.len);
}

template<typename el_t> bag_iter_t<const el_t> end (const bag_t<el_t>& bag)
{
	return end_bag_iter(bag.cells.ptr, bag.cells.len);
}

template<typename el_t> bool_t operator != (const bag_iter_t<el_t>& left, const bag_iter_t<el_t>& right)
{
	assert_eq(reinterpret_cast<nat8_t>(left.end), reinterpret_cast<nat8_t>(right.end));

	return left.at != right.at;
}

#endif

