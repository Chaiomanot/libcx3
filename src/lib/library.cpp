#include "library.hpp"
#include "text.hpp"
#include "error.hpp"
#include "file.hpp"
#include "program.hpp"
#ifdef __unix__
#include <dlfcn.h>
#endif
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

lib_t::operator bool () const
{
	return h;
}

void_t open (lib_t& lib, const str_t& lib_name,
				const str_t& env_var_name, err_t& err)
{
	assert_true(lib_name);
	if (err) { return; }

	path_t path;
	if (auto ev = env_var(env_var_name); ev) {
		path = create_path(ev);
	} else {
		path = create_path(lib_name);
	}

	#ifdef __unix__
	if (path.cos.len == 1 && !get_ext(path)) {
		set_ext(path, "so");
	}
	if (auto h = dlopen(as_strz(as_text(path)).ptr, RTLD_LAZY); h) {
		lib.h = reinterpret_cast<decltype(lib.h)>(h);
	} else {
		err = create_err(dlerror());
	}
	#endif

	#ifdef _WIN32
	// LoadLibrary takes care of the extension for us
	if (auto h = LoadLibrary(as_wstr(as_text(path)).ptr); h) {
		lib.h = reinterpret_cast<decltype(lib.h)>(h);
	} else {
		err = decode_os_err(GetLastError());
	}
	#endif
}

void_t* sym (lib_t& lib, const str_t& sym_name, err_t& err)
{
	if (err) { return nullptr; }

	#ifdef __unix__
	if (void_t* sym = dlsym(reinterpret_cast<void_t*>(lib.h), as_strz(sym_name).ptr); sym) {
		return sym;
	} else {
		err = create_err(dlerror());
		return {};
	}
	#endif

	#ifdef _WIN32
	if (FARPROC sym = GetProcAddress(reinterpret_cast<HMODULE>(lib.h),
										strz(sym_name).ptr)) {
		return reinterpret_cast<void_t*>(sym);
	} else {
		err = decode_os_err(GetLastError());
		return {};
	}
	#endif
}

lib_t::lib_t () { }
lib_t::~lib_t ()
{
	if (h) {
		#ifdef __unix__
		dlclose(reinterpret_cast<void_t*>(h));
		#endif
		#ifdef _WIN32
		CloseHandle(reinterpret_cast<HMODULE>(h));
		#endif
	}
}

lib_t::lib_t (lib_t&& src) { *this = move(src); }
lib_t& lib_t::operator = (lib_t&& src)
{
	if (&src != this) {
		this->~lib_t();
		name = move(src.name);
		src.name = {};
		h = src.h;
		src.h = {};
	}
	return *this;
}

