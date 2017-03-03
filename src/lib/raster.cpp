#include "raster.hpp"
#include "text.hpp"
#include "error.hpp"
#include "file.hpp"
#include "library.hpp"

img_raster_t::operator bool_t () const
{
	return bool(data);
}

img_raster_t get_placeholder_raster_img ()
{
	const nat4_t light = 0x3F3F3FFF;
	const nat4_t dark  = 0xBFBFBFFF;
	const nat4_t data[] = {
		light, light, light, light, dark,  dark,  dark,  dark,
		light, light, light, light, dark,  dark,  dark,  dark,
		light, light, light, light, dark,  dark,  dark,  dark,
		light, light, light, light, dark,  dark,  dark,  dark,
		dark,  dark,  dark,  dark,  light, light, light, light,
		dark,  dark,  dark,  dark,  light, light, light, light,
		dark,  dark,  dark,  dark,  light, light, light, light,
		dark,  dark,  dark,  dark,  light, light, light, light,
	};
	static_assert(sizeof(data) == 8 * 8 * 4);

	img_raster_t img;
	img.width  = 8;
	img.height = 8;
	img.data   = create_seq(data, sizeof(data));
	return img;
}

#include <stdio.h>

typedef FILE*             png_FILE_p;
typedef void*             png_voidp;
typedef const char*       png_const_charp;
typedef unsigned char     png_byte;
typedef png_byte*         png_bytep;
typedef png_bytep*        png_bytepp;
typedef unsigned long int png_uint_32;
typedef size_t            png_size_t;
typedef void*             png_structp;
typedef png_structp*      png_structpp;
typedef void*             png_infop;
typedef png_infop*        png_infopp;

typedef void (*png_error_ptr) (png_structp, png_const_charp);

#include "thread.hpp"

typedef png_structp (*png_create_read_struct_t) (png_const_charp user_png_ver, png_voidp error_ptr,
                                                 png_error_ptr error_fn, png_error_ptr warn_fn);
typedef png_infop (*png_create_info_struct_t) (png_structp png_ptr);
typedef void_t (*png_destroy_read_struct_t) (png_structpp png_ptr_ptr, png_infopp info_ptr_ptr,
                                             png_infopp end_info_ptr_ptr);
typedef void_t (*png_destroy_info_struct_t) (png_structp png_ptr, png_infopp info_ptr_ptr);
typedef png_uint_32 (*png_get_ihdr_t) (png_structp png_ptr, png_infop info_ptr,
                                       png_uint_32* width, png_uint_32* height, int* bit_depth, int* color_type,
                                       int* interlace_method, int* compression_method, int* filter_method);
typedef png_uint_32 (*png_get_rowbytes_t) (png_structp png_ptr, png_infop info_ptr);
typedef void_t (*png_init_io_t) (png_structp png_ptr, png_FILE_p fp);
typedef void_t (*png_read_image_t) (png_structp png_ptr, png_bytepp image);
typedef void_t (*png_read_info_t) (png_structp png_ptr, png_infop info_ptr);
typedef void_t (*png_set_sig_bytes_t) (png_structp png_ptr, int num_bytes);
typedef int (*png_sig_cmp_t) (png_bytep sig, png_size_t start, png_size_t num_to_check);

struct lib_png_t
{
	lib_t lib {};

	png_create_read_struct_t  create_read_struct  {};
	png_create_info_struct_t  create_info_struct  {};
	png_destroy_read_struct_t destroy_read_struct {};
	png_destroy_info_struct_t destroy_info_struct {};
	png_get_ihdr_t            get_ihdr            {};
	png_get_rowbytes_t        get_rowbytes        {};
	png_init_io_t             init_io             {};
	png_read_image_t          read_image          {};
	png_read_info_t           read_info           {};
	png_set_sig_bytes_t       set_sig_bytes       {};
	png_sig_cmp_t             sig_cmp             {};
};

void_t init (lib_png_t& lib, err_t& err)
{
	open(lib.lib, "libpng12", "PNG", err);

	link(lib.lib, lib.create_read_struct,  "png_create_read_struct",  err);
	link(lib.lib, lib.create_info_struct,  "png_create_info_struct",  err);
	link(lib.lib, lib.destroy_read_struct, "png_destroy_read_struct", err);
	link(lib.lib, lib.destroy_info_struct, "png_destroy_info_struct", err);
	link(lib.lib, lib.get_ihdr,            "png_get_IHDR",            err);
	link(lib.lib, lib.get_rowbytes,        "png_get_rowbytes",        err);
	link(lib.lib, lib.init_io,             "png_init_io",             err);
	link(lib.lib, lib.read_image,          "png_read_image",          err);
	link(lib.lib, lib.read_info,           "png_read_info",           err);
	link(lib.lib, lib.set_sig_bytes,       "png_set_sig_bytes",       err);
	link(lib.lib, lib.sig_cmp,             "png_sig_cmp",             err);

	if (err) { err = create_err("Couldn't load PNG library") + err; }
}

img_raster_t load_png (const path_t& path, err_t& err)
{
	png_const_charp PNG_LIBPNG_VER_STRING = "1.2.50";
	const int PNG_COLOR_MASK_PALETTE = 1;
	const int PNG_COLOR_MASK_COLOR   = 2;
	const int PNG_COLOR_MASK_ALPHA   = 4;
	const int PNG_COLOR_TYPE_GRAY    = 0;
	const int PNG_COLOR_TYPE_PALETTE    = PNG_COLOR_MASK_COLOR | PNG_COLOR_MASK_PALETTE;
	const int PNG_COLOR_TYPE_RGB        = PNG_COLOR_MASK_COLOR;
	const int PNG_COLOR_TYPE_RGB_ALPHA  = PNG_COLOR_MASK_COLOR | PNG_COLOR_MASK_ALPHA;
	const int PNG_COLOR_TYPE_GRAY_ALPHA = PNG_COLOR_MASK_ALPHA;

	static lib_png_t png;
	init(png, err);
	if (err) { return {}; }

	FILE *fp = fopen(as_strz(as_text(path)).ptr, "rb");
	if (!fp) {
		err = create_err("Couldn't open file");
		return {};
	}

	png_byte header[8] = {};
	fread(header, 1, 8, fp);
	if (png.sig_cmp(header, 0, 8) != 0) {
		err = create_err("Not a PNG");
		fclose(fp);
		return {};
	}

	png_structp reader = png.create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	if (!reader) {
		err = create_err("Couldn't create PNG read context");
		fclose(fp);
		return {};
	}

	png_infop info = png.create_info_struct(reader);
	if (!info) {
		err = create_err("Couldn't create information context");
		png.destroy_read_struct(&reader, nullptr, nullptr);
		fclose(fp);
		return {};
	}

	png.init_io(reader, fp);
	png.set_sig_bytes(reader, 8);
	png.read_info(reader, info);

	png_uint_32 tex_width  = 0;
	png_uint_32 tex_height = 0;
	int bit_depth  = 0;
	int color_type = 0;
	if (!png.get_ihdr(reader, info, &tex_width, &tex_height, &bit_depth, &color_type, nullptr, nullptr, nullptr)) {
		err = create_err("Couldn't get PNG image metadata");
		png.destroy_read_struct(&reader, nullptr, nullptr);
		fclose(fp);
		return {};
	}

	assert_gt(tex_width,  0);
	assert_gt(tex_height, 0);

	if (bit_depth != 8) {
		err = create_err("Unsupported bit depth, " + as_text(static_cast<nat4_t>(bit_depth)) + ", but must be 8");
		png.destroy_read_struct(&reader, nullptr, nullptr);
		fclose(fp);
		return {};
	}

	if (color_type != PNG_COLOR_TYPE_RGB_ALPHA) {
		png.destroy_read_struct(&reader, nullptr, nullptr);
		str_t clr_str;
		switch (color_type) {
			case PNG_COLOR_TYPE_RGB:        clr_str = "RGB";             break;
			case PNG_COLOR_TYPE_PALETTE:    clr_str = "paletted";        break;
			case PNG_COLOR_TYPE_GRAY:       clr_str = "grayscale";       break;
			case PNG_COLOR_TYPE_GRAY_ALPHA: clr_str = "grayscale-alpha"; break;
			default:                        clr_str = "unknown (" + as_text(static_cast<nat4_t>(color_type)) + ")"; break;
		}
		err = create_err("Image's color type is " + clr_str + ", but must be RGBA");
		return {};
	}

	assert_eq(png.get_rowbytes(reader, info), tex_width * 4);

	auto img_data = create_seq<png_byte>(tex_width * tex_height * 4);
	auto row_ptrs = create_seq<png_byte*>(tex_height);
	for (auto i : create_range(tex_height)) {
		row_ptrs[i] = &img_data[i * (tex_width * 4)];
	}

	png.read_image(reader, row_ptrs.ptr);

	png.destroy_read_struct(&reader, &info, nullptr);

	img_raster_t img;
	img.width    = tex_width;
	img.height   = tex_height;
	img.data.ptr = reinterpret_cast<nat4_t*>(img_data.ptr);
	img.data.len = img_data.len / 4;
	img_data.ptr = nullptr;
	img_data.len = 0;
	return img;
}

