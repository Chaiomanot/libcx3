#ifndef libcx3_prelude_hpp
#define libcx3_prelude_hpp

typedef void              void_t;
typedef decltype(nullptr) nullptr_t;
static_assert(sizeof(void_t*) == 8);

typedef unsigned char          nat1_t;
typedef unsigned short int     nat2_t;
typedef unsigned int           nat4_t;
typedef unsigned long long int nat8_t;
static_assert(sizeof(nat1_t) == 1);
static_assert(sizeof(nat2_t) == 2);
static_assert(sizeof(nat4_t) == 4);
static_assert(sizeof(nat8_t) == 8);

typedef bool bool_t;
static_assert(sizeof(bool_t) == 1);

typedef float  rat4_t;
typedef double rat8_t;
static_assert(sizeof(rat4_t) == 4);
static_assert(sizeof(rat8_t) == 8);

template<int N> struct pad_t { nat1_t pad[N] {}; };

template<int N> struct nat_wrap_t { };
template<> struct nat_wrap_t<1> { nat1_t u; };
template<> struct nat_wrap_t<2> { nat2_t u; };
template<> struct nat_wrap_t<4> { nat4_t u; };
template<> struct nat_wrap_t<8> { nat8_t u; };

template<typename val_t> constexpr nat8_t max ()
{
	const auto nat = static_cast<decltype(nat_wrap_t<sizeof(val_t)>::u)>(-1ULL);
	const auto div = static_cast<val_t>(-1ULL) < 0 ? 2ULL : 1ULL;
	return static_cast<nat8_t>(nat) / div;
}

#include "debug.hpp"

template<typename val_t> struct remove_reference_t         { typedef val_t type; };
template<typename val_t> struct remove_reference_t<val_t&> { typedef val_t type; };

template<typename val_t> typename remove_reference_t<val_t>::type&& move (val_t&& val)
{
	return static_cast<typename remove_reference_t<val_t>::type&&>(val);
}

template<typename val_t> void_t unused (const val_t& val)
{
	static_cast<void_t>(val);
}

struct opaque_t
{
	nat8_t val {};

	explicit operator bool_t () const;
};

nat8_t clamp (nat8_t val, nat8_t lo, nat8_t hi);
rat4_t clamp (rat4_t val, rat4_t lo, rat4_t hi);
rat8_t clamp (rat8_t val, rat8_t lo, rat8_t hi);

struct range_iter_t
{
	nat8_t val {};

	nat8_t operator * ();
	range_iter_t& operator ++ ();
};

struct range_t
{
	range_iter_t it {};
};

range_iter_t begin (const range_t& range);
range_iter_t end (const range_t& range);
bool_t operator != (const range_iter_t& left, const range_iter_t& right);

range_t create_range (nat8_t n);

template<typename el_t> struct seq_t
{
	el_t*  ptr {};
	nat8_t len {};

	seq_t () { }

	~seq_t ()
	{
		void_t free_mem (void_t* ptr, nat8_t len);

		if (ptr) {
			for (auto& el : *this) {
				el.~el_t();
			}
			free_mem(ptr, len * sizeof(el_t));
			ptr = nullptr;
		}
		len = 0;
	}

	seq_t (const seq_t<el_t>& src) = delete;
	seq_t (seq_t<el_t>&& src) { *this = move(src); }
	seq_t<el_t>& operator = (const seq_t<el_t>& src) = delete;
	seq_t<el_t>& operator = (seq_t<el_t>&& src)
	{
		if (&src != this) {
			this->~seq_t();
			ptr = src.ptr;
			len = src.len;
			src.ptr = nullptr;
			src.len = 0;
		}
		return *this;
	}

	el_t& operator [] (decltype(len) i)
	{
		assert_lteq(i, len);
		return ptr[i];
	}

	const el_t& operator [] (decltype(len) i) const
	{
		assert_lteq(i, len);
		return ptr[i];
	}

	explicit operator bool () const
	{
		assert_eq(!!ptr, (len > 0));
		return len > 0;
	}

	seq_t (const char* src) // compile will succeed if the type is nat1_t
	{
		seq_t<nat1_t> create_str (const char* src);

		*this = create_str(src);
	}
};

template<typename el_t>       el_t* begin (      seq_t<el_t>& seq) { return  seq.ptr;      }
template<typename el_t>       el_t* end   (      seq_t<el_t>& seq) { return &seq[seq.len]; }
template<typename el_t> const el_t* begin (const seq_t<el_t>& seq) { return  seq.ptr;      }
template<typename el_t> const el_t* end   (const seq_t<el_t>& seq) { return &seq[seq.len]; }

template<typename el_t> void_t grow (seq_t<el_t>& seq, nat8_t ins_at, nat8_t ins_len)
{
	void_t* alloc_mem (nat8_t len);

	assert_lteq(ins_at, seq.len);
	if (!ins_len) { return; }

	assert_init_zero<el_t>();
	auto old = move(seq);

	seq.len = old.len + ins_len;
	seq.ptr = static_cast<el_t*>(alloc_mem(seq.len * sizeof(el_t)));

	for (auto i : create_range(ins_at)) {
		seq[i] = move(old[i]);
	}
	for (auto i : create_range(old.len - ins_at)) {
		seq[i + ins_at + ins_len] = move(old[i + ins_at]);
	}
}

template<typename el_t> void_t shrink (seq_t<el_t>& seq, nat8_t rm_at, nat8_t rm_len)
{
	assert_lteq(rm_at, seq.len);
	assert_lteq(rm_at + rm_len, seq.len);
	if (!rm_len) { return; }

	void_t* alloc_mem (nat8_t len);

	auto old = move(seq);

	seq.len = old.len - rm_len;
	seq.ptr = seq.len > 0 ? static_cast<el_t*>(alloc_mem(seq.len * sizeof(el_t))) : nullptr;

	for (auto ix : create_range(rm_at)) {
		seq[ix] = move(old[ix]);
	}
	for (auto ix : create_range(seq.len - rm_at)) {
		seq[ix + rm_at] = move(old[ix + rm_at + rm_len]);
	}
}

template<typename el_t> seq_t<el_t> create_seq (nat8_t len)
{
	seq_t<el_t> seq;
	grow(seq, 0, len);
	return seq;
}

template<typename el_t> seq_t<el_t> create_seq (const el_t* ptr, nat8_t len)
{
	seq_t<el_t> seq;
	grow(seq, 0, len);
	for (auto ix : create_range(len)) {
		seq.ptr[ix] = ptr[ix];
	}
	return seq;
}

using str_t = seq_t<nat1_t>;

void_t begin_main (const char* name, int argc, const char** argv);
void_t end_main ();

#endif

