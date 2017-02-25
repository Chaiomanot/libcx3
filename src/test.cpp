#include "lib/prelude.hpp"
#include "lib/text.hpp"
#include "lib/error.hpp"
#include "lib/pipe.hpp"
#include "lib/platform.hpp"

int main (int args_len, const char** args_ptr)
{
	pipe_t std_out;
	begin_main("Test", args_len, args_ptr, nullptr, &std_out, nullptr);

	err_t err;
	send(std_out, "Running on " + as_text(get_plat()) + get_line_sep(), err);

	end_main();
}

