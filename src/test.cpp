#include "lib/prelude.hpp"
#include "lib/text.hpp"
#include "lib/error.hpp"
#include "lib/platform.hpp"

int main (int args_len, const char** args_ptr)
{
	begin_main("Test", args_len, args_ptr);

	err_t err;
	print("Running on " + as_text(get_plat()) + get_line_sep(), err);

	end_main();
}

