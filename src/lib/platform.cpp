#include "platform.hpp"
#include "text.hpp"
#ifdef __linux__
#include <sys/utsname.h>
#endif
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <versionhelpers.h>
#ifndef _WIN32_WINNT_WIN10
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreserved-id-macro"
#define _WIN32_WINNT_WIN10 0x0A00
#pragma clang diagnostic pop
VERSIONHELPERAPI IsWindows10OrGreater () {
	return IsWindowsVersionOrGreater(HIBYTE(_WIN32_WINNT_WIN10), LOBYTE(_WIN32_WINNT_WIN10), 0); }
#endif
#endif

plat_t::operator bool () const
{
	return kernel != plat_kernel_t::unknown;
}

plat_t get_plat (nat2_t major, nat2_t minor, nat2_t rev)
{
	plat_t plat;
	#ifdef __linux__
	plat.kernel = plat_kernel_t::linux;
	#endif
	#ifdef _WIN32
	plat.kernel = plat_kernel_t::winnt;
	#endif
	plat.kernel_major = major;
	plat.kernel_minor = minor;
	plat.kernel_rev   = rev;
	return plat;
}

plat_t get_plat (const str_t& ver_str)
{
	nat8_t n[3] = {};
	nat8_t old_at = 0;
	for (auto i : create_range(sizeof(n) / sizeof(*n))) {
		nat8_t new_at = old_at;
		n[i] = clamp(decode_nat(ver_str, new_at), 0, max<nat2_t>());
		if (new_at == old_at || new_at >= ver_str.len) {
			break;
		}
		if (ver_str[new_at] != '.') {
			break;
		}
		old_at = new_at + 1;
	}
	return get_plat(static_cast<nat2_t>(n[0]), static_cast<nat2_t>(n[1]), static_cast<nat2_t>(n[2]));
}

plat_t get_plat ()
{
	#ifdef __linux__
	utsname buf = {};
	int stat = uname(&buf);
	assert_eq(stat, 0);
	unused(stat);
	return get_plat(buf.release);
	#endif

	#ifdef _WIN32
	if (const auto ntdll = GetModuleHandle(TEXT("ntdll.dll")); ntdll) {
		if (const auto ver_func = GetProcAddress(ntdll, "wine_get_version"); ver_func) {
			return get_plat(reinterpret_cast<const char*(*)()>(ver_func)());
		}
	}
	if (IsWindows10OrGreater())       { return get_plat(10, 0, 0); }
	if (IsWindows8Point1OrGreater())  { return get_plat( 6, 3, 0); }
	if (IsWindows8OrGreater())        { return get_plat( 6, 2, 0); }
	if (IsWindows7SP1OrGreater())     { return get_plat( 6, 1, 1); }
	if (IsWindows7OrGreater())        { return get_plat( 6, 1, 0); }
	if (IsWindowsVistaSP2OrGreater()) { return get_plat( 6, 0, 2); }
	if (IsWindowsVistaSP1OrGreater()) { return get_plat( 6, 0, 1); }
	if (IsWindowsVistaOrGreater())    { return get_plat( 6, 0, 0); }
	if (IsWindowsXPSP3OrGreater())    { return get_plat( 5, 2, 3); }
	if (IsWindowsXPSP2OrGreater())    { return get_plat( 5, 2, 2); }
	if (IsWindowsXPSP1OrGreater())    { return get_plat( 5, 2, 1); }
	if (IsWindowsXPOrGreater())       { return get_plat( 5, 2, 0); }
	return                                     get_plat( 5, 0, 0);
	#endif
}

str_t as_text (const plat_kernel_t& plat)
{
	switch (plat) {
		case plat_kernel_t::unknown: return "Unknown";
		case plat_kernel_t::linux:   return "Linux";
		case plat_kernel_t::winnt:   return "Windows";
	}
	return "Unknown";
}

str_t as_text (const plat_t& plat)
{
	str_t str = as_text(plat.kernel);
	if (plat.kernel_major || plat.kernel_minor || plat.kernel_rev) {
		str = str + " " + as_text(plat.kernel_major);
		if (plat.kernel_minor || plat.kernel_rev) {
			str = str + "." + as_text(plat.kernel_minor);
			if (plat.kernel_rev) {
				str = str + "." + as_text(plat.kernel_rev);
			}
		}
	}
	return str;
}

