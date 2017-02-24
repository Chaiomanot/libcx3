#ifndef libcx3_debug_hpp
#define libcx3_debug_hpp
#ifndef libcx3_prelude_hpp
#error "Don't include debug.hpp directly"
#endif

#define laze_stmt() static_assert(true)

typedef const char* test_result_t;

#if !defined(NDEBUG) || !defined(NTEST)

const char* describe_bug (const char* file, int line, const char* op,
                          nat8_t left_val, nat8_t right_val, const char* left_expr, const char* right_expr);

#define fmt_bug_loc(op, left_val, right_val, left_expr, right_expr) \
	describe_bug(__FILE__, __LINE__, op, \
	             static_cast<nat8_t>(left_val), static_cast<nat8_t>(right_val), left_expr, right_expr)

#endif
#ifndef NDEBUG

[[noreturn]] void_t fail_assertion (const char* msg);

#define assert_eq(   left, right) \
	if (!((left) == (right))) { fail_assertion(fmt_bug_loc("==", left, right, #left, #right)); } laze_stmt()
#define assert_uneq( left, right) \
	if (!((left) != (right))) { fail_assertion(fmt_bug_loc("!=", left, right, #left, #right)); } laze_stmt()
#define assert_gt(   left, right) \
	if (!((left) > (right)))  { fail_assertion(fmt_bug_loc(">",  left, right, #left, #right)); } laze_stmt()
#define assert_lt(   left, right) \
	if (!((left) < (right)))  { fail_assertion(fmt_bug_loc("<",  left, right, #left, #right)); } laze_stmt()
#define assert_gteq( left, right) \
	if (!((left) >= (right))) { fail_assertion(fmt_bug_loc(">=", left, right, #left, #right)); } laze_stmt()
#define assert_lteq( left, right) \
	if (!((left) <= (right))) { fail_assertion(fmt_bug_loc("<=", left, right, #left, #right)); } laze_stmt()
#define assert_true( val        ) \
	if (!(val))        { fail_assertion(fmt_bug_loc("T",  bool(val), 0, #val, ""    )); } laze_stmt()
#define assert_false(val        ) \
	if (!(!(val)))     { fail_assertion(fmt_bug_loc("F",  bool(val), 0, #val, ""    )); } laze_stmt()
#define assert_same( left, right) \
	if (!((left) == (right))) { \
		fail_assertion(fmt_bug_loc("*", reinterpret_cast<nat8_t>(as_strz(left).ptr), \
		                                reinterpret_cast<nat8_t>(as_strz(right).ptr), #left, #right)); } laze_stmt()

void_t* operator new (unsigned long size, void_t* ptr);

template<typename new_t> void_t assert_init_zero ()
{
	nat1_t buf[sizeof(new_t)];
	for (nat8_t i = 0; i < sizeof(buf); ++i) { buf[i] = max<nat1_t>(); }
	new(buf) new_t{};

	for (nat8_t i = 0; i < sizeof(buf); ++i) {
		if (buf[i]) {
			fail_assertion(describe_bug("", 0, "0", reinterpret_cast<nat8_t>(&buf), sizeof(buf), "", ""));
		}
	}
}

#else

#define assert_eq(   left, right) laze_stmt()
#define assert_uneq( left, right) laze_stmt()
#define assert_gt(   left, right) laze_stmt()
#define assert_lt(   left, right) laze_stmt()
#define assert_gteq( left, right) laze_stmt()
#define assert_lteq( left, right) laze_stmt()
#define assert_true( val        ) laze_stmt()
#define assert_false(val        ) laze_stmt()
#define assert_same( left, right) laze_stmt()

#endif
#ifndef NTEST

test_result_t fail_proof (const char* msg);

void_t install_test (test_result_t (*func) (), const char* name, const char* deps);
#pragma clang diagnostic ignored "-Wglobal-constructors"
#define define_test(func, deps) \
	test_result_t test_##func (); \
	namespace { nullptr_t dummy = (install_test(&test_##func, #func, deps), nullptr); } \
	test_result_t test_##func ()

#define prove_eq(   left, right) \
	if (!((left) == (right))) { return fail_proof(fmt_bug_loc("==", left, right, #left, #right)); } laze_stmt()
#define prove_uneq( left, right) \
	if (!((left) != (right))) { return fail_proof(fmt_bug_loc("!=", left, right, #left, #right)); } laze_stmt()
#define prove_gt(   left, right) \
	if (!((left) >  (right)))  { return fail_proof(fmt_bug_loc(">",  left, right, #left, #right)); } laze_stmt()
#define prove_lt(   left, right) \
	if (!((left) <  (right)))  { return fail_proof(fmt_bug_loc("<",  left, right, #left, #right)); } laze_stmt()
#define prove_gteq( left, right) \
	if (!((left) >= (right))) { return fail_proof(fmt_bug_loc(">=", left, right, #left, #right)); } laze_stmt()
#define prove_lteq( left, right) \
	if (!((left) <= (right))) { return fail_proof(fmt_bug_loc("<=", left, right, #left, #right)); } laze_stmt()
#define prove_true( val        ) \
	if (!(val))        { return fail_proof(fmt_bug_loc("T",  bool(val), 0, #val, ""    )); } laze_stmt()
#define prove_false(val        ) \
	if (!(!(val)))     { return fail_proof(fmt_bug_loc("F",  bool(val), 0, #val, ""    )); } laze_stmt()
#define prove_same(left, right) \
	if (!((left) == (right))) { \
		fail_proof(fmt_bug_loc("*", reinterpret_cast<nat8_t>(as_strz(left).ptr), \
		                            reinterpret_cast<nat8_t>(as_strz(right).ptr), #left, #right)); } laze_stmt()

#else

#define define_test(func, deps) test_result_t test_##func ()

#define prove_eq(   left, right) laze_stmt()
#define prove_uneq( left, right) laze_stmt()
#define prove_gt(   left, right) laze_stmt()
#define prove_lt(   left, right) laze_stmt()
#define prove_gteq( left, right) laze_stmt()
#define prove_lteq( left, right) laze_stmt()
#define prove_true( val        ) laze_stmt()
#define prove_false(val        ) laze_stmt()
#define prove_same( left, right) laze_stmt()

#endif

#endif

