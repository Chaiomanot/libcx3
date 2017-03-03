#include "lzma.hpp"
#include "text.hpp"

// Implementation originally by Igor Pavlov and placed in the public domain

typedef nat2_t prob_t;

void_t init_probs (prob_t* probs, nat8_t probs_len)
{
	const nat8_t num_bit_model_total_bits = 11;
	for (auto i : create_range(probs_len / sizeof(prob_t))) {
		probs[i] = (1 << num_bit_model_total_bits) / 2;
	}
}

struct out_wnd_t
{
	nat1_t*  buf_ptr {};
	nat4_t   buf_i   {};
	nat4_t   buf_len {};

	bool_t  is_full {};
	pad_t<3> padding {};
	nat4_t   total_i {};

	nat1_t*  dst_ptr {};
	nat8_t   dst_len {};
	nat8_t   dst_i   {};
};

void_t put (out_wnd_t& ownd, nat1_t b)
{
	ownd.total_i++;
	ownd.buf_ptr[ownd.buf_i++] = b;
	if (ownd.buf_i == ownd.buf_len) {
		ownd.buf_i = 0;
		ownd.is_full = true;
	}

	if (ownd.dst_i >= ownd.dst_len) {
		ownd.dst_i = ownd.dst_len + 1;
		return;
	}
	ownd.dst_ptr[ownd.dst_i] = b;
	ownd.dst_i++;
}

nat1_t byte (const out_wnd_t& ownd, nat4_t dist)
{
	return ownd.buf_ptr[dist <= ownd.buf_i ? ownd.buf_i - dist :
						ownd.buf_len - dist + ownd.buf_i];
}

void_t cp_match (out_wnd_t& ownd, nat4_t dist, nat4_t len)
{
	for (auto i : create_range(len)) {
		unused(i);
		put(ownd, byte(ownd, dist));
	}
}

bool_t check_dist (const out_wnd_t& ownd, nat4_t dist)
{
	return dist <= ownd.buf_i || ownd.is_full;
}

bool_t is_empty (const out_wnd_t& ownd)
{
	return ownd.buf_i == 0 && !ownd.is_full;
}

struct range_dec_t
{
	nat4_t range {};
	nat4_t code  {};

	const nat1_t* src_ptr {};
	nat8_t       src_len {};
	nat8_t       src_i   {};

	bool_t  corrupt {};
	pad_t<7> padding {};
};

nat1_t read (range_dec_t& rdec)
{
	if (rdec.src_i >= rdec.src_len) {
		rdec.src_i = rdec.src_len + 1;
		return 0;
	}
	auto buf = rdec.src_ptr[rdec.src_i];
	rdec.src_i++;
	return buf;
}

bool_t init (range_dec_t& rdec)
{
	rdec.corrupt = false;
	rdec.range   = 0xFFFFFFFF;
	rdec.code    = 0;

	auto b = read(rdec);
	for (auto i : create_range(4)) {
		unused(i);
		rdec.code = (rdec.code << 8) | read(rdec);
	}
	if (b != 0 || rdec.code == rdec.range) {
		rdec.corrupt = true;
	}
	return b == 0;
}

bool_t is_finished_ok (const range_dec_t& rdec)
{
	return rdec.code == 0;
}

void_t normalize (range_dec_t& rdec)
{
	const decltype(rdec.range) top_val = 1 << 24;
	if (rdec.range < top_val) {
		rdec.range <<= 8;
		rdec.code = (rdec.code << 8) | read(rdec);
	}
}

nat4_t dec_direct_bits (range_dec_t& rdec, nat4_t n_bits)
{
	nat4_t res = 0;
	for (auto i : create_range(n_bits)) {
		unused(i);
		rdec.range >>= 1;
		rdec.code -= rdec.range;
		nat4_t t = 0 - (rdec.code >> 31);
		rdec.code += rdec.range & t;
		if (rdec.code == rdec.range) {
			rdec.corrupt = true;
		}
		normalize(rdec);
		res <<= 1;
		res += t + 1;
	}
	return res;
}

nat4_t dec_bit (range_dec_t& rdec, prob_t& prob)
{
	const auto n_bit_model_total_bits = 11;
	const auto n_mv_bits = 5;

	nat4_t v = prob;
	nat4_t bound = (rdec.range >> n_bit_model_total_bits) * v;
	nat4_t sym;
	if (rdec.code < bound) {
		v += ((1 << n_bit_model_total_bits) - v) >> n_mv_bits;
		rdec.range = bound;
		sym = 0;
	} else {
		v -= v >> n_mv_bits;
		rdec.code -= bound;
		rdec.range -= bound;
		sym = 1;
	}
	prob = static_cast<prob_t>(v);
	normalize(rdec);
	return sym;
}

nat4_t bit_tree_reverse_dec (prob_t* probs, nat4_t n_bits, range_dec_t& rc)
{
	nat4_t m = 1;
	nat4_t sym = 0;
	for (auto i : create_range(n_bits)) {
		auto bit = dec_bit(rc, probs[m]);
		m <<= 1;
		m += bit;
		sym |= (bit << i);
	}
	return sym;
}

template<nat4_t n_bits> struct bit_tree_dec_t
{
	prob_t probs[1U << n_bits] {};
	pad_t<(((1U << n_bits) % 4) + 4) * 2> padding {};
};

template<nat4_t n_bits> void_t init (bit_tree_dec_t<n_bits>& tdec)
{
	return init_probs(tdec.probs, sizeof(tdec.probs));
}

template<nat4_t n_bits> nat4_t dec
					(bit_tree_dec_t<n_bits>& tdec, range_dec_t& rc)
{
	nat4_t m = 1;
	for (auto i : create_range(n_bits)) {
		unused(i);
		m = (m << 1) + dec_bit(rc, tdec.probs[m]);
	}
	return m - (1U << n_bits);
}

template<nat4_t n_bits> nat4_t reverse_dec
					(bit_tree_dec_t<n_bits>& tdec, range_dec_t& rc)
{
	return bit_tree_reverse_dec(tdec.probs, n_bits, rc);
}

const nat8_t n_pos_bits_max      = 4;
const nat8_t n_len_to_pos_states = 4;
const nat8_t n_align_bits        = 4;
const nat8_t end_pos_model_ix    = 14;
const nat8_t n_full_dists        = 1 << (end_pos_model_ix >> 1);

struct len_dec_t
{
	prob_t            choices[2]                     {};
	pad_t<4>          padding                        {};
	bit_tree_dec_t<3> low_coder[1 << n_pos_bits_max] {};
	bit_tree_dec_t<3> mid_coder[1 << n_pos_bits_max] {};
	bit_tree_dec_t<8> high_coder                     {};
};

void_t init (len_dec_t& ldec)
{
	init_probs(ldec.choices, sizeof(ldec.choices));
	init(ldec.high_coder);
	for (auto i : create_range(1 << n_pos_bits_max)) {
		init(ldec.low_coder[i]);
		init(ldec.mid_coder[i]);
	}
}

nat4_t dec (len_dec_t& ldec, range_dec_t& rc, nat4_t pos_state)
{
	if (dec_bit(rc, ldec.choices[0]) == 0) {
		return dec(ldec.low_coder[pos_state], rc);
	}
	if (dec_bit(rc, ldec.choices[1]) == 0) {
		return 8 + dec(ldec.mid_coder[pos_state], rc);
	}
	return 16 + dec(ldec.high_coder, rc);
}

struct lzma_props_t
{
	nat4_t lc      {};
	nat4_t pb      {};
	nat4_t lp      {};
	nat4_t dict_sz {};
	nat8_t buf_sz  {};
};

lzma_props_t dec_lzma_props (const void_t* src_ptr, nat8_t src_len)
{
	if (src_len < 5) {
		return {};
	}
	nat4_t d = static_cast<const nat1_t*>(src_ptr)[0];
	if (d >= (9 * 5 * 5)) {
		return {};
	}
	lzma_props_t props;
	props.lc = d % 9;
	d /= 9;
	props.pb = d / 5;
	props.lp = d % 5;
	props.dict_sz = 0;
	for (auto i : create_range(4)) {
		props.dict_sz |= static_cast<nat4_t>(
					static_cast<const nat1_t*>(src_ptr)[i + 1]) << (8 * i);
	}
	const decltype(props.dict_sz) lzma_dic_min = 1 << 12;
	if (props.dict_sz < lzma_dic_min)
		props.dict_sz = lzma_dic_min;
	props.buf_sz = props.dict_sz +
						((0x300U << (props.lc + props.lp)) * sizeof(prob_t));
	return props;
}

struct lzma_dec_t
{
	static const nat8_t n_states = 12;

	range_dec_t  rdec  {};
	out_wnd_t    ownd  {};
	lzma_props_t props {};

	prob_t* lit_probs {};

	bit_tree_dec_t<6> pos_slot_dec[n_len_to_pos_states] {};
	bit_tree_dec_t<n_align_bits> align_dec {};
	prob_t pos_decs[1 + n_full_dists - end_pos_model_ix] {};
	pad_t<2> padding {};

	prob_t is_match[n_states << n_pos_bits_max] {};
	prob_t is_rep[n_states] {};
	prob_t is_rep_g0[n_states] {};
	prob_t is_rep_g1[n_states] {};
	prob_t is_rep_g2[n_states] {};
	prob_t is_rep_0long[n_states << n_pos_bits_max] {};

	len_dec_t len_dec {};
	len_dec_t rep_len_dec {};
};

bool_t init (lzma_dec_t& dec)
{
	if (!init(dec.rdec)) {
		return false;
	}

	// init literals
	init_probs(dec.lit_probs,
				(0x300U << (dec.props.lc + dec.props.lp)) * sizeof(prob_t));

	// init dist
	for (auto i : create_range(n_len_to_pos_states)) {
		init(dec.pos_slot_dec[i]);
	}
	init(dec.align_dec);
	init_probs(dec.pos_decs, sizeof(dec.pos_decs));

	init_probs(dec.is_match,     sizeof(dec.is_match));
	init_probs(dec.is_rep,       sizeof(dec.is_rep));
	init_probs(dec.is_rep_g0,    sizeof(dec.is_rep_g0));
	init_probs(dec.is_rep_g1,    sizeof(dec.is_rep_g1));
	init_probs(dec.is_rep_g2,    sizeof(dec.is_rep_g2));
	init_probs(dec.is_rep_0long, sizeof(dec.is_rep_0long));

	init(dec.len_dec);
	init(dec.rep_len_dec);

	return true;
}

void_t dec_lit (lzma_dec_t& dec, nat4_t state, nat4_t rep0)
{
	nat4_t prev_byte = 0;
	if (!is_empty(dec.ownd)) {
		prev_byte = byte(dec.ownd, 1);
	}

	nat4_t sym = 1;
	nat4_t lit_state = ((dec.ownd.total_i & ((1 << dec.props.lp) - 1)) <<
							dec.props.lc) + (prev_byte >> (8 - dec.props.lc));
	prob_t* probs = &dec.lit_probs[0x300 * lit_state];

	if (state >= 7) {
		nat4_t match_byte = byte(dec.ownd, rep0 + 1);
		while (sym < 0x100) {
			nat4_t match_bit = (match_byte >> 7) & 1;
			match_byte <<= 1;
			auto bit = dec_bit(dec.rdec, probs[((1 + match_bit) << 8) + sym]);
			sym = (sym << 1) | bit;
			if (match_bit != bit) {
				break;
			}
		}
	}
	while (sym < 0x100) {
		sym = (sym << 1) | dec_bit(dec.rdec, probs[sym]);
	}
	put(dec.ownd, sym & 0xFF);
}

nat4_t dec_dist (lzma_dec_t& dec, nat4_t len)
{
	nat4_t len_state = len;
	if (len_state > n_len_to_pos_states - 1)
		len_state = n_len_to_pos_states - 1;

	nat4_t pos_slot = ::dec(dec.pos_slot_dec[len_state], dec.rdec);
	if (pos_slot < 4)
		return pos_slot;

	nat4_t n_direct_bits = ((pos_slot >> 1) - 1);
	nat4_t dist = ((2 | (pos_slot & 1)) << n_direct_bits);
	if (pos_slot < end_pos_model_ix) {
		dist += bit_tree_reverse_dec(&dec.pos_decs[dist - pos_slot],
									n_direct_bits, dec.rdec);
	} else {
		dist += dec_direct_bits(dec.rdec, n_direct_bits - n_align_bits) <<
																n_align_bits;
		dist += reverse_dec(dec.align_dec, dec.rdec);
	}
	return dist;
}

bool_t dec (lzma_dec_t& dec, nat8_t unpack_sz)
{
	if (!init(dec)) {
		return false;
	}

	nat4_t rep[4] = {};
	nat4_t state = 0;

	while (unpack_sz > 0 || !is_finished_ok(dec.rdec)) {

		const auto pos_state = dec.ownd.total_i & ((1 << dec.props.pb) - 1);
		const auto state_i = (state << n_pos_bits_max) + pos_state;

		if (dec_bit(dec.rdec, dec.is_match[state_i]) == 0) {
			if (unpack_sz == 0) {
				return false;
			}
			dec_lit(dec, state, rep[0]);
			// literal
			if (state < 4) {
				state = 0;
			} else if (state < 10) {
				state = state - 3;
			} else {
				state = state - 6;
			}
			--unpack_sz;
			continue;
		}

		nat4_t len;

		if (dec_bit(dec.rdec, dec.is_rep[state]) != 0) {
			if (unpack_sz == 0) {
				return false;
			}
			if (is_empty(dec.ownd)) {
				return false;
			}
			if (dec_bit(dec.rdec, dec.is_rep_g0[state]) == 0) {
				if (dec_bit(dec.rdec, dec.is_rep_0long[state_i]) == 0) {
					state = state < 7 ? 9 : 11; // short rep
					put(dec.ownd, byte(dec.ownd, rep[0] + 1));
					--unpack_sz;
					continue;
				}
			} else {
				nat4_t dist;
				if (dec_bit(dec.rdec, dec.is_rep_g1[state]) == 0) {
					dist = rep[1];
				} else {
					if (dec_bit(dec.rdec, dec.is_rep_g2[state]) == 0) {
						dist = rep[2];
					} else {
						dist = rep[3];
						rep[3] = rep[2];
					}
					rep[2] = rep[1];
				}
				rep[1] = rep[0];
				rep[0] = dist;
			}
			len = ::dec(dec.rep_len_dec, dec.rdec, pos_state);
			state = state < 7 ? 8 : 11; // rep
		} else {
			rep[3] = rep[2];
			rep[2] = rep[1];
			rep[1] = rep[0];
			len = ::dec(dec.len_dec, dec.rdec, pos_state);
			state = state < 7 ? 7 : 10; // match
			rep[0] = dec_dist(dec, len);
			if (rep[0] == 0xFFFFFFFF) { // end marker
				if (is_finished_ok(dec.rdec)) {
					break;
				}
				return false;
			}
			if (unpack_sz == 0) {
				return false;
			}
			if (rep[0] >= dec.props.dict_sz || !check_dist(dec.ownd, rep[0])) {
				return false;
			}
		}
		const decltype(len) match_min_len = 2;
		len += match_min_len;
		if (unpack_sz < len) {
			return false;
		}
		cp_match(dec.ownd, rep[0] + 1, len);
		unpack_sz -= len;
	}

	return !dec.rdec.corrupt &&
			dec.rdec.src_i == dec.rdec.src_len &&
			dec.ownd.dst_i == dec.ownd.dst_len;
}

nat8_t lzma_dec_buf_len (const void_t* src_ptr, nat8_t src_len)
{
	return dec_lzma_props(src_ptr, src_len).buf_sz;
}

bool_t lzma_dec (void_t* buf_ptr, nat8_t buf_len,
				void_t* dst_ptr, nat8_t dst_len,
				const void_t* src_ptr, nat8_t src_len)
{
	if (!buf_len) { return false; }

	lzma_dec_t dec;
	dec.props = dec_lzma_props(src_ptr, src_len);
	if (dec.props.buf_sz != buf_len) { return false; }

	dec.rdec.src_ptr = static_cast<const nat1_t*>(src_ptr);
	dec.rdec.src_len = src_len;
	dec.rdec.src_i   = 5;
	dec.ownd.dst_ptr = static_cast<nat1_t*>(dst_ptr);
	dec.ownd.dst_len = dst_len;
	dec.ownd.buf_ptr = static_cast<nat1_t*>(buf_ptr);
	dec.ownd.buf_len = dec.props.dict_sz;
	dec.lit_probs = reinterpret_cast<prob_t*>(
						&static_cast<nat1_t*>(buf_ptr)[dec.props.dict_sz]);

	return ::dec(dec, dst_len);
}

str_t lzma_dec (const str_t& src, nat8_t dst_len)
{
	auto buf = create_str(lzma_dec_buf_len(src.ptr, src.len));
	if (!buf) { return {}; }
	auto dst = create_str(dst_len);
	if (!lzma_dec(buf.ptr, buf.len,
				dst.ptr, dst.len, src.ptr, src.len)) { return {}; }
	return dst;
}

const char* prove_lzma_case (const nat1_t* coded_ptr, const nat8_t coded_len,
							const str_t& data)
{
	prove_same(lzma_dec(create_str(coded_ptr, coded_len), data.len), data);
	unused(coded_ptr);
	unused(coded_len);
	unused(data);
	return {};
}

define_test(lzma, "text")
{
	// unfortunately, to do proper testing i'd have try against a large file
	// but i don't wont to wait for that at every debug run,
	// this'll protect against obvious errors, at least

	prove_same(lzma_dec("", 0), "");

	const nat1_t src[] = {
		0x5D,0x00,0x00,0x80,0x00,0x00,0x24,0x19,
		0x49,0x98,0x6F,0x10,0x19,0xC6,0xD7,0x31,
		0xEB,0x36,0x50,0xB2,0x98,0x48,0xFF,0xFE,
		0xA5,0xB0,0x00,
	};
	prove_lzma_case(src, sizeof(src), "Hello world\n");

	return {};
}

