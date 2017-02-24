#include "prelude.hpp"

enum class plat_kernel_t : nat2_t
{
	unknown = 0,
	linux   = 1,
	winnt   = 2,
};

struct plat_t
{
	plat_kernel_t kernel       {};
	nat2_t      kernel_major {};
	nat2_t      kernel_minor {};
	nat2_t      kernel_rev   {};

	explicit operator bool () const;
};

plat_t get_plat ();
str_t as_text (const plat_kernel_t& kernel);
str_t as_text (const plat_t& plat);

