#ifndef libcx3_time_hpp
#define libcx3_time_hpp
#include "prelude.hpp"

struct inter_t
{
	nat8_t days  {};
	nat8_t picos {};
};

bool_t operator == (inter_t left, inter_t right);
bool_t operator != (inter_t left, inter_t right);
bool_t operator >  (inter_t left, inter_t right);
bool_t operator <  (inter_t left, inter_t right);
bool_t operator >= (inter_t left, inter_t right);
bool_t operator <= (inter_t left, inter_t right);

inter_t operator + (inter_t inter, inter_t addend);
inter_t operator - (inter_t inter, inter_t subtrahend);

nat8_t get_secs      (inter_t inter);
nat8_t get_millisecs (inter_t inter);
nat8_t get_clunks    (inter_t inter);
nat8_t get_sec_millisecs (inter_t inter);
nat8_t get_sec_nanosecs  (inter_t inter);

inter_t create_inter_of_secs      (nat8_t secs);
inter_t create_inter_of_millisecs (nat8_t millisecs);
inter_t create_inter_of_clunks    (nat8_t clunks);
inter_t create_inter_of_nanosecs  (nat8_t nanosecs);

struct date_t
{
	inter_t val;
};

bool_t operator == (date_t left, date_t right);
bool_t operator != (date_t left, date_t right);
bool_t operator >  (date_t left, date_t right);
bool_t operator <  (date_t left, date_t right);
bool_t operator >= (date_t left, date_t right);
bool_t operator <= (date_t left, date_t right);

date_t operator + (date_t date, inter_t addend);
date_t operator - (date_t date, inter_t subtrahend);
inter_t operator - (date_t date, date_t subtrahend);

inter_t get_current_inter ();
date_t get_current_date ();

struct err_t;
str_t as_text (date_t date, err_t& err);

struct metronome_t
{
	opaque_t opaq {};

	metronome_t ();
	~metronome_t ();
	metronome_t (metronome_t&& ori);
	metronome_t (const metronome_t& ori) = delete;
	metronome_t& operator = (metronome_t&& ori);
	metronome_t& operator = (const metronome_t& ori) = delete;

	explicit operator bool_t () const;
};

metronome_t create_metronome (err_t& err);
void_t set_freq (metronome_t& met, inter_t inter, err_t& err);
void_t recv (metronome_t& met, err_t& err);

#endif

