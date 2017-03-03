#ifndef libcx3_raster_hpp
#define libcx3_raster_hpp
#include "prelude.hpp"

struct img_raster_t
{
	nat8_t        width  {};
	nat8_t        height {};
	seq_t<nat4_t> data   {};

	explicit operator bool_t () const;
};

img_raster_t get_placeholder_raster_img ();

struct path_t;
struct err_t;
img_raster_t load_png (const path_t& path, err_t& err);

#endif

