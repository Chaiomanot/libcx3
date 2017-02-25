#include "text.hpp"
#include "raw.hpp"
#include "pipe.hpp"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

nat8_t get_len (const char* str)
{
	return str ? strlen(str) : 0;
}

str_t create_str (nat8_t len)
{
	return create_seq<nat1_t>(len);
}

str_t create_str (const void_t* ptr, nat8_t len)
{
	return create_seq<nat1_t>(static_cast<const nat1_t*>(ptr), len);
}

str_t clone (const str_t& text)
{
	auto neo = create_seq<nat1_t>(text.len);
	copy_mem(neo.ptr, text.ptr, text.len);
	return neo;
}

str_t concat (const void_t* left_ptr,  nat8_t left_len, const void_t* right_ptr, nat8_t right_len)
{
	auto prod = create_seq<nat1_t>(left_len + right_len);
	copy_mem(&prod[0],        left_ptr,  left_len );
	copy_mem(&prod[left_len], right_ptr, right_len);
	return prod;
}

bool_t operator == (const str_t& left, const str_t& right) { return is_mem_eq(left.ptr, left.len, right.ptr, right.len); }
bool_t operator == (const str_t& left, const char*  right) { return is_mem_eq(left.ptr, left.len, right, get_len(right)); }
bool_t operator == (const char*  left, const str_t& right) { return is_mem_eq(left, get_len(left), right.ptr, right.len); }

bool_t operator != (const str_t& left, const str_t& right) { return !(left == right); }
bool_t operator != (const str_t& left, const char*  right) { return !(left == right); }
bool_t operator != (const char*  left, const str_t& right) { return !(left == right); }

str_t operator + (const str_t& left, const str_t& right) { return concat(left.ptr, left.len, right.ptr, right.len); }
str_t operator + (const str_t& left, const char*  right) { return concat(left.ptr, left.len, right, get_len(right)); }
str_t operator + (const char*  left, const str_t& right) { return concat(left, get_len(left), right.ptr, right.len); }

str_t quote (const str_t& text)
{
	return "\u201C" + text + "\u201D";
}

str_t as_text (nat8_t n)
{
	char buf[16 * 3];
	auto i = sizeof(buf);
	buf[--i] = '\0';

	if (n != 0) {
		while (n != 0) {
			if (i % 4 == 0) {
				buf[--i] = ',';
			}
			buf[--i] = "0123456789"[n % 10];
			n /= 10;
		}
	} else {
		buf[--i] = '0';
	}
	return &buf[i];
}

str_t as_text (rat8_t n, bool_t fmt_wide)
{
	char buf[64];
	snprintf(buf, sizeof(buf), fmt_wide ? "%.16g" : "%.7g", n); // inf = \u221e, nan = NaN

	nat8_t exp_i = 0;
	for (auto i : create_range(max<nat8_t>())) {
		if (!buf[i]) { break; }
		// we don't have to check for inf or nan because they don't contain an 'e'
		if (buf[i] == 'e') {
			assert_eq(exp_i, 0);
			buf[i] = '\0';
			exp_i = i + 1;
			break;
		}
	}

	if (exp_i == 0) {
		// i'm not sure what the spec has to say about it,
		// but in my experience the sign bit is undefined for NaNs & Infs
		// so for cleanliness and consistency, we'll strip it for them
		#ifdef _WIN32
		nat8_t i = 0;
		if (buf[i] == '-' || buf[i] == '+') { i += 1; }
		if (buf[i] == '1' && buf[i + 1] == '.') { i += 2; }
		if (buf[i] == '#') {
			if (buf[i + 1] == 'I') { return "inf"; }
			if (buf[i + 1] == 'Q' || buf[i + 1] == 'S' || buf[i + 1] == 'N') { return "nan"; }
			assert_same(create_str(buf), "");
		}
		#endif
		if (buf[0] == '-') {
			if (buf[1] == 'i') { return "inf"; }
			if (buf[1] == 'n') { return "nan"; }
		}
		return buf;
	}

	nat8_t radix_i = 0;
	nat8_t i = 0;
	while (buf[i]) {
		if (buf[i] == '.') {
			assert_eq(radix_i, 0);
			radix_i = i;
			break;
		}
		++i;
	}
	if (buf[i] == '.') {
		while (buf[i + 1]) {
			buf[i] = buf[i + 1];
			++i;
		}
	}
	assert_gt(radix_i, 0);

	char* expr_val_end_ptr = nullptr;
	auto exp_val = strtol(&buf[exp_i], &expr_val_end_ptr, 10);
	assert_true(expr_val_end_ptr);
	assert_false(*expr_val_end_ptr);
	assert_uneq(exp_val, 0);
	if (exp_val >= 0) {
		// decrease the exp for every digit we move the radix pt rightward
		exp_val -= i - radix_i;
	} else {
		// increase the exp for every digit we move the radix pt leftward
		exp_val += radix_i;
	}

	if (exp_val >= 3 || exp_val <= -4) {
		snprintf(&buf[i], sizeof(buf) - i, "e%ld", exp_val); // e = \u23E8
	} else {
		for (auto exp_val_i : create_range(static_cast<nat8_t>(exp_val))) {
			unused(exp_val_i);
			buf[i] = '0';
			++i;
		}
		buf[i] = '\0';
	}
	if (exp_val < 0) {
		copy_mem(&buf[2], buf, get_len(buf) + 1);
		buf[0] = '0';
		buf[1] = '.';
	}
	return buf;
}

str_t as_text (nat4_t n)
{
	return as_text(static_cast<nat8_t>(n));
}

str_t as_text (nat2_t n)
{
	return as_text(static_cast<nat8_t>(n));
}

str_t as_text (nat1_t n)
{
	return as_text(static_cast<nat8_t>(n));
}

str_t as_text (rat8_t n)
{
	return as_text(n, true);
}

str_t as_text (rat4_t n)
{
	return as_text(static_cast<rat8_t>(n), false);
}

nat8_t decode_nat (const str_t& str, nat8_t* i)
{
	if (!i) {
		nat8_t dummy_i = 0;
		return decode_nat(str, &dummy_i);
	}
	if (*i >= str.len) { return 0; }

	auto buf = as_strz(str);
	char* end_ptr = nullptr;
	nat8_t val = strtoul(&buf[*i], &end_ptr, 0);
	*i = static_cast<nat8_t>(static_cast<decltype(buf.ptr)>(end_ptr) - buf.ptr);
	return val;
}

rat8_t decode_rat (const str_t& str, nat8_t* i)
{
	if (!i) {
		nat8_t dummy_i = 0;
		return decode_rat(str, &dummy_i);
	}
	if (*i >= str.len) { return 0; }

	auto buf = as_strz(str);
	char* end_ptr = nullptr;
	rat8_t val = strtod(&buf[*i], &end_ptr);
	*i = static_cast<nat8_t>(static_cast<decltype(buf.ptr)>(end_ptr) - buf.ptr);
	return val;
}

str_t create_str (const char* src)
{
	return create_str(src, get_len(src));
}

seq_t<char> as_strz (const str_t& str)
{
	auto prod = create_seq<char>(str.len + 1);
	copy_mem(prod.ptr, str.ptr, str.len);
	return prod;
}

#ifdef _WIN32
str_t create_str (const wchar_t* wstr)
{
	if (!wstr) { return {}; }

	const auto wstr_len = wcslen(wstr);
	if (!wstr_len) { return {}; }

	assert_lt(wstr_len, max<nat4_t>() / 2);
	auto str_len = WideCharToMultiByte(CP_UTF8, 0, wstr, static_cast<int>(wstr_len), NULL, 0, NULL, NULL);
	if (str_len <= 0) { return {}; }

	auto str = create_str(static_cast<nat8_t>(str_len));
	auto tr_len = WideCharToMultiByte(CP_UTF8, 0, wstr, static_cast<int>(wstr_len),
	                                  reinterpret_cast<char*>(str.ptr), str_len, NULL, NULL);
	if (tr_len <= 0) { return {}; }

	return str;
}

seq_t<wchar_t> as_wstr (const str_t& str)
{
	if (!str) { return {}; }

	assert_lt(str.len, max<nat4_t>() / 2);
	auto wstr_len = MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(str.ptr),
	                                    static_cast<int>(str.len), NULL, 0);
	if (wstr_len <= 0) { return {}; }

	// +1 because MultiByteToWideChar won't give a null term since we didnt give it one
	auto wstr = create_seq<WCHAR>(static_cast<nat8_t>(wstr_len) + 1);
	auto tr_len = MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(str.ptr),
	                                  static_cast<int>(str.len), wstr.ptr, wstr_len);
	if (tr_len <= 0) { return {}; }

	return wstr;
}
#endif

str_t get_line_sep ()
{
	#ifdef __unix__
	return "\n";
	#endif
	#ifdef _WIN32
	return "\r\n";
	#endif
}

define_test(text, "")
{
	{ auto s = as_strz("(0_0)");
		prove_eq(s.len, 6);
		prove_false(s[5]);
		prove_true(strcmp(s.ptr, "(0_0)") == 0);
	}

	prove_true(create_str("Space") != create_str("Slash"));

	prove_true(create_str("left<") + ">right" == "left<>right");
	prove_true("left<" + create_str(">right") == "left<>right");
	prove_true(create_str("left<") + ">right" != "right<>left");
	prove_true("left<" + create_str(">right") != "right<>left");

	prove_same(as_text(0U), "0");
	prove_same(as_text(1234567890U), "1,234,567,890");
	prove_same(as_text( 0.0),  "0");
	prove_same(as_text(-0.0), "-0");
	prove_same(as_text(12345.0f), "12345");
	prove_same(as_text(12345.0 ), "12345");
	prove_same(as_text(1234.4321012f      ), "1234.432");
	prove_same(as_text(1234.43210123456789), "1234.432101234568");
	prove_same(as_text(123443210.12f      ), "123443200");
	prove_same(as_text(123443210.123456789), "123443210.1234568");
	prove_same(as_text(12344321012.0f     ), "1234432e4");
	prove_same(as_text(123443210123456.789), "123443210123456.8");
	prove_same(as_text(1234432101200000.0f   ), "1234432e9");
	prove_same(as_text(12344321012345678900.0), "1234432101234568e4");
	prove_same(as_text(80085.0f), "80085");
	prove_same(as_text(80085.0 ), "80085");
	prove_same(as_text(-8008.135f), "-8008.135");
	prove_same(as_text(-8008.135 ), "-8008.135");
	prove_same(as_text(-8008135.7175f), "-8008136");
	prove_same(as_text(-8008135.7175 ), "-8008135.7175");
	prove_same(as_text(-80081357175.455f), "-8008135e4");
	prove_same(as_text(-80081357175.455 ), "-80081357175.455");
	prove_same(as_text(-80081357177135.45535f), "-8008136e7");
	prove_same(as_text(-80081357177135.45535 ), "-80081357177135.45");
	prove_same(as_text(0.0012345789f), "0.001234579");
	prove_same(as_text(0.0012345789 ), "0.0012345789");
	prove_same(as_text(0.000000000012345789f), "0.1234579e-10");
	prove_same(as_text(0.000000000012345789 ), "0.12345789e-10");
	prove_same(as_text(0.0000123457898765432123f), "0.1234579e-4");
	prove_same(as_text(0.0000123457898765432123 ), "0.1234578987654321e-4");
	prove_same(as_text( 1.0 / 0.0), "inf");
	prove_same(as_text(-1.0 / 0.0), "inf");
	prove_same(as_text( 0.0 / 0.0), "nan");
	prove_same(as_text(-0.0 / 0.0), "nan");

	prove_eq(decode_nat("12345", nullptr), 12345);
	{ auto v = decode_rat("-8008.135", nullptr);
		prove_gteq(v, -8008.135 - 0.00001);
		prove_lteq(v, -8008.135 + 0.00001);
	}

	#ifdef _WIN32
	{ auto s = as_wstr("Hello");
		assert_eq(s.len, 6);
		assert_eq(s[0], 'H');
		assert_eq(s[4], 'o');
		assert_eq(s[5], '\0');
	}
	#endif

	return {};
}

