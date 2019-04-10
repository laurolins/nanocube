/*
 * UTC Coordinated Universal Time
 * TAI Temps Atomique International (ahead of UTC by 36 secs)
 */

#ifdef time_UNIT_TEST
#include "platform.c"
#endif

/*
UTC Leap Seconds
2272060800	10	# 1 Jan 1972
2287785600	11	# 1 Jul 1972
2303683200	12	# 1 Jan 1973
2335219200	13	# 1 Jan 1974
2366755200	14	# 1 Jan 1975
2398291200	15	# 1 Jan 1976
2429913600	16	# 1 Jan 1977
2461449600	17	# 1 Jan 1978
2492985600	18	# 1 Jan 1979
2524521600	19	# 1 Jan 1980
2571782400	20	# 1 Jul 1981
2603318400	21	# 1 Jul 1982
2634854400	22	# 1 Jul 1983
2698012800	23	# 1 Jul 1985
2776982400	24	# 1 Jan 1988
2840140800	25	# 1 Jan 1990
2871676800	26	# 1 Jan 1991
2918937600	27	# 1 Jul 1992
2950473600	28	# 1 Jul 1993
2982009600	29	# 1 Jul 1994
3029443200	30	# 1 Jan 1996
3076704000	31	# 1 Jul 1997
3124137600	32	# 1 Jan 1999
3345062400	33	# 1 Jan 2006
3439756800	34	# 1 Jan 2009
3550089600	35	# 1 Jul 2012
3644697600	36	# 1 Jul 2015
3692217600	37	# 1 Jan 2017
*/

typedef struct {

	struct {
		char *begin;
		char *end;
	} abbreviation;

	struct {
		char *begin;
		char *end;
	} name;

	s32  offset;
	b8   daylight_savings;

} tm_TimeZone;

/* UTC labeling of a tm_Time */
/* Assumes tm_Time.time is an offset in seconds since 1970-01-01T00:00:00Z */
/* leap seconds should be included */
typedef struct {

	s32 year;
	s32 month;
	s32 day;
	s32 hour;
	s32 minute;
	s32 second;

	/* offset in minutes from the UTC Zulu timezone */
	/* if an official timezone is specified, offset should match */
	s32 offset_minutes;

	/* official timezones including daylight savings info etc */
	tm_TimeZone *timezone;

} tm_Label;


typedef struct {
	s64 time;
} tm_Time;

// 1475778758


static char *tm_timezones_src[] = {
"NST",	"Newfoundland Standard Time",	"UTC-03:30", "0",
"NDT",	"Newfoundland Daylight Time",	"UTC-02:30", "1",
"AST",	"Atlantic Standard Time",       "UTC-04:00", "0",
"ADT",	"Atlantic Daylight Time",       "UTC-03:00", "1",
"EST",	"Eastern Standard Time",	"UTC-05:00", "0",
"EDT",	"Eastern Daylight Time",	"UTC-04:00", "1",
"CST",	"Central Standard Time",	"UTC-06:00", "0",
"CDT",	"Central Daylight Time",	"UTC-05:00", "1",
"MST",	"Mountain Standard Time",	"UTC-07:00", "0",
"MDT",	"Mountain Daylight Time",	"UTC-06:00", "1",
"PST",	"Pacific Standard Time",	"UTC-08:00", "0",
"PDT",	"Pacific Daylight Time",	"UTC-07:00", "1",
"AKST",	"Alaska Standard Time",		"UTC-09:00", "0",
"AKDT",	"Alaska Daylight Time",		"UTC-08:00", "1",
"HAST",	"Hawaii-Aleutian Standard",	"UTC-10:00", "0",
"HADT",	"Hawaii-Aleutian Daylight",	"UTC-09:00", "1",
"Z",	"Zulu Time",                    "UTC+00:00", "0",
"BRST",	"Brasilia Summer Time",	        "UTC-02:00", "1",
"BRT",	"Brasilia Time",	        "UTC-03:00", "0"
};

#define tm_NUM_TIMEZONES sizeof(tm_timezones_src)/sizeof(char*)/4

static b8 tm_timezones_initialized = 0;
static tm_TimeZone tm_timezones[tm_NUM_TIMEZONES];

/*                                                Jan  Feb  Mar  Apr  May  Jun  Jul  Aug  Sep  Oct  Nov  Dec */
static s32 tm_month_days[]               = {   0,  31,  28,  31,  30,  31,  30,  31,  31,  30,  31,  30,  31 };
// static s32 tm_month_days_leap[]          = {   0,  31,  29,  31,  30,  31,  30,  31,  31,  30,  31,  30,  31 };
static s32 tm_accum_month_days[]         = {   0,  31,  59,  90, 120, 151, 181, 212, 243, 273, 304, 334, 365 };
static s32 tm_accum_month_days_leap[]    = {   0,  31,  60,  91, 121, 152, 182, 213, 244, 274, 305, 335, 366 };
/*                                                Dec  Nov  Oct  Sep  Aug  Jul  Jun  May  Apr  Mar  Feb  Jan */
static s32 tm_accum_reverse_month_days[] = {  0,   31,  61,  92, 122, 153, 184, 214, 245, 275, 306, 334, 365 };

static s32 tm_LENGTH_MONTH_DAYS          = sizeof(tm_month_days) / sizeof(s32);


/* initialize tm_epoch label */
static tm_Label tm_epoch      = {.year=1,    .month=1, .day=1, .hour=0, .minute=0, .second=0, .offset_minutes=0, .timezone = 0};
static tm_Label tm_unix_epoch = {.year=1970, .month=1, .day=1, .hour=0, .minute=0, .second=0, .offset_minutes=0, .timezone = 0};

/* used to convert unix time to tm time */
static tm_Time tm_unix_epoch_tm_time = { .time = 62135596800 };

/* seconds in a day */
#define tm_SECONDS_PER_DAY       86400ll
#define tm_MINUTES_PER_DAY       1440ll
#define tm_SECONDS_PER_HOUR      3600ll
#define tm_SECONDS_PER_MINUTE    60ll
#define tm_MINUTES_PER_HOUR      60ll

/* non-leap year */
#define tm_DAYS_PER_YEAR         365ll
#define tm_DAYS_PER_LEAP_YEAR    366ll

/* number of divisors of num in { x : begin <= x < end, x: integer } */
static s32
tm_divisors(s32 begin, s32 end, s32 num)
{
	Assert(num > 0);
	s32 diff = end - begin;
	if (diff == 0) return 0;
	s32 q    = diff  / num;
	s32 r    = diff  % num;
	s32 off  = begin % num;
	return q + (((off == 0 && r>0) || (off + r > num)) ? 1 : 0);
}

static s32
tm_leap_years_between(s32 begin, s32 end)
{
	Assert(begin <= end);
	s32 d4   = tm_divisors(begin, end, 4);
	s32 d100 = tm_divisors(begin, end, 100);
	s32 d400 = tm_divisors(begin, end, 400);
	return d4 - d100 + d400;
}

static s32
tm_is_leap_year(s32 year)
{
	return tm_leap_years_between(year, year + 1);
}

static s32
tm_days_between_years(s32 begin, s32 end)
{
	Assert(begin <= end);
	return (end - begin) * tm_DAYS_PER_YEAR + tm_leap_years_between(begin, end);
}


static void
tm_initialize_timezones()
{
	if (tm_timezones_initialized)
		return;

	for (int i=0;i<tm_NUM_TIMEZONES;++i) {
		tm_TimeZone *tz = tm_timezones + i;

		char *abbr      = tm_timezones_src[4*i + 0];
		char *name      = tm_timezones_src[4*i + 1];
		char *off       = tm_timezones_src[4*i + 2];
		char *daylight  = tm_timezones_src[4*i + 3];

		Assert(*daylight == '0' || *daylight == '1');
		Assert(cstr_end(off) - off == 9);

		tz->abbreviation.begin = abbr;
		tz->abbreviation.end   = cstr_end(abbr);

		tz->name.begin = name;
		tz->name.end   = cstr_end(name);

		s32 hours=0, minutes=0;
		b8 okh = pt_parse_s32(off + 3, off + 6, &hours);
		b8 okm = pt_parse_s32(off + 7, off + 9, &minutes);

		if (!okh || !okm) {
			Assert(cstr_end(off) - off == 9);
		}

		tz->offset  = hours * tm_MINUTES_PER_HOUR + minutes;

		tz->daylight_savings = *daylight == '0' ? 0 : 1;
	}

	tm_timezones_initialized = 1;
}

static s32
tm_year_days_elapsed(s32 year, s32 month, s32 day)
{
	return tm_accum_month_days[month - 1] + (day - 1) + ((month > 2) ? tm_is_leap_year(year) : 0);
}

/* doesn't include the current date */
static s32
tm_year_days_remaining(s32 year, s32 month, s32 day)
{
	if (month >= 3 || !tm_is_leap_year(year)) {
		return tm_accum_reverse_month_days[12 - month] + (tm_month_days[month] - day);
	} else {
		// it is a leap year and month <= 2
		if (month == 2) {
			return tm_accum_reverse_month_days[12 - month] + (29 - day);
		} else {
			return tm_accum_reverse_month_days[12 - month] + (tm_month_days[month] - day) + 1;
		}
	}
}

static s32
tm_seconds_until_end_of_day(s32 hour, s32 minute, s32 second)
{
	return tm_SECONDS_PER_DAY - (hour * tm_SECONDS_PER_HOUR + minute * tm_SECONDS_PER_MINUTE + second);
}

static void
tm_Time_init_from_unix_time(tm_Time *self, s64 unix_time)
{
	self->time = unix_time + tm_unix_epoch_tm_time.time;
}

static s64
tm_Time_unix_time(tm_Time *self)
{
	return self->time - tm_unix_epoch_tm_time.time;
}

static b8
tm_Time_init_from_label(tm_Time *self, tm_Label *label)
{
	Assert(label);
	if (label->year >= tm_epoch.year) {
		s32 days = tm_year_days_elapsed(label->year, label->month, label->day);
#if 0
		Print *print = &debug_request->print;
		Print_clear(print);
		Print_cstr(print,"....days: ");
		Print_u64(print, days);
		Print_cstr(print,"\n");
		Print_cstr(print,"....is_leap_year: ");
		Print_s64(print, tm_is_leap_year(label->year));
		Print_cstr(print,"\n");
		s32 d4   = tm_divisors(label->year, label->year+1, 4);
		s32 d100 = tm_divisors(label->year, label->year+1, 100);
		s32 d400 = tm_divisors(label->year, label->year+1, 400);
		Print_cstr(print,"....d4 d100 d400: ");
		Print_s64(print, d4);
		Print_cstr(print," ");
		Print_s64(print, d100);
		Print_cstr(print," ");
		Print_s64(print, d400);
		Print_cstr(print,"\n");
		Request_print(debug_request, print);
#endif
		days += tm_days_between_years(tm_epoch.year, label->year);
		self->time = days * tm_SECONDS_PER_DAY
			+ label->hour * tm_SECONDS_PER_HOUR
			+ label->minute * tm_SECONDS_PER_MINUTE
			+ label->second
		        - label->offset_minutes * tm_SECONDS_PER_MINUTE;
	} else {
		s32 days = tm_days_between_years(label->year + 1, tm_epoch.year) +
			tm_year_days_remaining(label->year, label->month, label->day);
		self->time = days * tm_SECONDS_PER_DAY
			+ tm_seconds_until_end_of_day(label->hour, label->minute, label->second)
			- label->offset_minutes * tm_SECONDS_PER_MINUTE;
	}
	return 1;
}

static s32*
tm_lower_bound(s32 *begin, s32 *end, s32 num)
{
	/* while there are three or more elements */
	/* begin and end will always change in each iteration */
	while (begin + 2 < end) {
		/* "it" is > begin */
		/* "it" is < end */
		s32 *it = begin + (end - begin)/2;
		if (num <= *it) {
			end = it + 1;
		} else {
			begin = it;
		}
	}

	s32 len = end - begin;
	if (len == 2) {
		if (num <= *begin) {
			return begin;
		} else if (num <= *(begin + 1)) {
			return begin + 1;
		} else {
			return begin + 2;
		}
	} else if (len == 1) {
		if (num <= *begin) {
			return begin;
		} else {
			return begin + 1;
		}
	} else {
		return begin;
	}
}

#define tm_MONDAY   0
#define tm_TUESDAY  1
#define tm_WEDNESAY 2
#define tm_THURSDAY 3
#define tm_FRIDAY   4
#define tm_SATURDAY 5
#define tm_SUNDAY   6

static u8
tm_weekday(s32 year, s32 month, s32 day)
{
	s32 days = tm_days_between_years(tm_epoch.year, year) +
		tm_year_days_elapsed(year, month, day);
	return (u8) (days % 7);
}

static b8
tm_Label_init(tm_Label *self, tm_Time time)
{
	/* only stuff after epoch */
	Assert(time.time >= 0);
	s32 days    = (s32) (time.time / tm_SECONDS_PER_DAY);
	s32 seconds = (s32) (time.time % tm_SECONDS_PER_DAY);
//
//
#if 0
	{
		Print *print = &debug_request->print;
		Print_clear(print);
		Print_cstr(print,"....days: ");
		Print_u64(print, days);
		Print_cstr(print,"\n");
		Request_print(debug_request, print);
	}
#endif
//
	/* find year defined */
	s32 year      = tm_epoch.year + (days / tm_DAYS_PER_YEAR);
	s32 year_days = tm_days_between_years(tm_epoch.year, year);
	while (year_days > days) {
		--year;
		year_days = tm_days_between_years(tm_epoch.year, year);
	}
	self->year = year;
//
	/* find month */
	days -= year_days;
//
	s32 *begin = (!tm_is_leap_year(year))
		? tm_accum_month_days
		: tm_accum_month_days_leap;
	s32 *end   = begin + tm_LENGTH_MONTH_DAYS;
	s32 *it    = tm_lower_bound(begin, end, days);
	Assert(it < end);
	if (*it == days) {
		self->month = (it - begin) + 1;
		days -= *it;
	} else {
		Assert(it > begin);
		self->month = (it - begin);
		days -= *(it-1);
	}
//
	/* find day */
	self->day = 1 + days;
//
	/* find hour */
	self->hour   = seconds / tm_SECONDS_PER_HOUR;
	seconds      = seconds % tm_SECONDS_PER_HOUR;
	self->minute = seconds / tm_SECONDS_PER_MINUTE;
	self->second = seconds % tm_SECONDS_PER_MINUTE;
	self->offset_minutes = 0;
	self->timezone = 0;
//
	return 1;
//
}

/* same time different offset minutes */
static void
tm_Label_adjust_offset(tm_Label *self, s32 new_offset_minutes)
{
	tm_Time t;
	tm_Time_init_from_label(&t, self);

	t.time += new_offset_minutes * tm_SECONDS_PER_MINUTE;

	tm_Label_init(self, t);
	self->offset_minutes = new_offset_minutes;
	self->timezone = 0;
}

static void
tm_Label_print(tm_Label* self, Print *print)
{
	print_s64(print, self->year);
	print_align(print, 4, 1, ' ');
	print_cstr(print,"-");
	print_s64(print, self->month);
	print_align(print, 2, 1, '0');
	print_cstr(print,"-");
	print_s64(print, self->day);
	print_align(print, 2, 1, '0');
	print_cstr(print,"T");
	print_s64(print, self->hour);
	print_align(print, 2, 1, '0');
	print_char(print,':');
	print_s64(print, self->minute);
	print_align(print, 2, 1, '0');
	print_char(print,':');
	print_s64(print, self->second);
	print_align(print, 2, 1, '0');
	if (self->offset_minutes == 0) {
		print_char(print,'Z');
	} else {
		s32 mins = self->offset_minutes;
		b8  neg  = mins < 0;
		if (neg) {
			print_char(print,'-');
			mins = -mins;
		} else {
			print_char(print,'+');
		}
		print_s64(print, mins / tm_MINUTES_PER_HOUR);
		print_align(print, 2, 1, '0');
		print_char(print,':');
		print_s64(print, mins % tm_MINUTES_PER_HOUR);
		print_align(print, 2, 1, '0');
	}
}



#ifdef time_UNIT_TEST

#include <stdio.h>
#include <stdlib.h>

int main()
{
	// unix epoch
	tm_Label unix_epoch_label = {.year=1970, .month=1, .day=1, .hour=0, .minute=0, .second=0, .offset_minutes=0, .timezone = 0};
	tm_Label year_2017_label  = {.year=2017, .month=1, .day=1, .hour=0, .minute=0, .second=0, .offset_minutes=0, .timezone = 0};

	tm_Time unix_epoch_time;
	tm_Time year_2017_time;

 	tm_Time_init_from_label(&unix_epoch_time, &unix_epoch_label);
	tm_Time_init_from_label(&year_2017_time, &year_2017_label);

	// query on wolfram alpha: "convert 2017-01-01 UTC to unix time"
	printf("A: 1970-01-01T00:00:00Z (unix epoch)    tm_Time: %ld\n",unix_epoch_time.time);
	printf("B: 2017-01-01T00:00:00Z (start of 2017) tm_Time: %ld\n",year_2017_time.time);
	printf("B-A: %ld\n", year_2017_time.time - unix_epoch_time.time);

	// output
	//     A: 1970-01-01T00:00:00Z (unix epoch)    tm_Time: 62135596800
	//     B: 2017-01-01T00:00:00Z (start of 2017) tm_Time: 63618825600
	//     B-A: 1483228800

	return 0;
}


#endif





















//
// ACDT	Australian Central Daylight Savings Time	UTC+10:30
// ACST	Australian Central Standard Time	UTC+09:30
// ACST	Central Standard Time (Australia)	UTC+09:30
// ACT	Acre Time	UTC-05:00
// ADT	Atlantic Daylight Time	UTC-03:00
// AEDT	Australian Eastern Daylight Savings Time	UTC+11:00
// AEST	Australian Eastern Standard Time	UTC+10:00
// AFT	Afghanistan Time	UTC+04:30
// KDT	Alaska Daylight Time	UTC-08:00
// AKST	Alaska Standard Time	UTC-09:00
// AMST	Amazon Summer Time (Brazil)[1]	UTC-03:00
// AMT	Amazon Time (Brazil)[2]	UTC-04:00
// ART	Argentina Time	UTC-03:00
// AST	Atlantic Standard Time	UTC-04:00
// AWST	Australian Western Standard Time	UTC+08:00
// AZOST	Azores Summer Time	UTC+00:00
// AZOT	Azores Standard Time	UTC-01:00
// AZT	Azerbaijan Time	UTC+04:00
// BDT	Brunei Time	UTC+08:00
// BIOT	British Indian Ocean Time	UTC+06:00
// BIT	Baker Island Time	UTC-12:00
// BOT	Bolivia Time	UTC-04:00
// BRST	Brasilia Summer Time	UTC-02:00
// BRT	Brasilia Time	UTC-03:00
// BST	British Summer Time (British Standard Time from Feb 1968 to Oct 1971)	UTC+01:00
// BTT	Bhutan Time	UTC+06:00
// CAT	Central Africa Time	UTC+02:00
// CCT	Cocos Islands Time	UTC+06:30
// CDT	Central Daylight Time (North America)	UTC-05:00
// CEST	Central European Summer Time (Cf. HAEC)	UTC+02:00
// CET	Central European Time	UTC+01:00
// CHADT	Chatham Daylight Time	UTC+13:45
// CHAST	Chatham Standard Time	UTC+12:45
// CHOST	Choibalsan Summer Time	UTC+09:00
// CHOT	Choibalsan Standard Time	UTC+08:00
// CHST	Chamorro Standard Time	UTC+10:00
// CHUT	Chuuk Time	UTC+10:00
// CIST	Clipperton Island Standard Time	UTC-08:00
// CIT	Central Indonesia Time	UTC+08:00
// CKT	Cook Island Time	UTC-10:00
// CLST	Chile Summer Time	UTC-03:00
// CLT	Chile Standard Time	UTC-04:00
// COST	Colombia Summer Time	UTC-04:00
// COT	Colombia Time	UTC-05:00
// CST	Central Standard Time (North America)	UTC-06:00
// CT	China time	UTC+08:00
// CVT	Cape Verde Time	UTC-01:00
// CWST	Central Western Standard Time (Australia) unofficial	UTC+08:45
// CXT	Christmas Island Time	UTC+07:00
// DAVT	Davis Time	UTC+07:00
// DDUT	Dumont d'Urville Time	UTC+10:00
// DFT	AIX specific equivalent of Central European Time[5]	UTC+01:00
// EASST	Easter Island Summer Time	UTC-05:00
// EAST	Easter Island Standard Time	UTC-06:00
// EAT	East Africa Time	UTC+03:00
// ECT	Ecuador Time	UTC-05:00
// EDT	Eastern Daylight Time (North America)	UTC-04:00
// EEST	Eastern European Summer Time	UTC+03:00
// EET	Eastern European Time	UTC+02:00
// EGST	Eastern Greenland Summer Time	UTC+00:00
// EGT	Eastern Greenland Time	UTC-01:00
// EIT	Eastern Indonesian Time	UTC+09:00
// EST	Eastern Standard Time (North America)	UTC-05:00
// FET	Further-eastern European Time	UTC+03:00
// FJT	Fiji Time	UTC+12:00
// FKST	Falkland Islands Summer Time	UTC-03:00
// FKT	Falkland Islands Time	UTC-04:00
// FNT	Fernando de Noronha Time	UTC-02:00
// GALT	Galapagos Time	UTC-06:00
// GAMT	Gambier Islands	UTC-09:00
// GET	Georgia Standard Time	UTC+04:00
// GFT	French Guiana Time	UTC-03:00
// GILT	Gilbert Island Time	UTC+12:00
// GIT	Gambier Island Time	UTC-09:00
// GMT	Greenwich Mean Time	UTC+00:00
// GST	South Georgia and the South Sandwich Islands	UTC-02:00
// GST	Gulf Standard Time	UTC+04:00
// GYT	Guyana Time	UTC-04:00
// HADT	Hawaii-Aleutian Daylight Time	UTC-09:00
// HAEC	Heure Avancée d'Europe Centrale francised name for CEST	UTC+02:00
// HAST	Hawaii-Aleutian Standard Time	UTC-10:00
// HKT	Hong Kong Time	UTC+08:00
// HMT	Heard and McDonald Islands Time	UTC+05:00
// HOVST	Khovd Summer Time	UTC+08:00
// HOVT	Khovd Standard Time	UTC+07:00
// ICT	Indochina Time	UTC+07:00
// IDTG	Israel Daylight Time	UTC+03:00
// IOT	Indian Ocean Time	UTC+03:00
// IRDT	Iran Daylight Time	UTC+04:30
// IRKT	Irkutsk Time	UTC+08:00
// IRST	Iran Standard Time	UTC+03:30
// IST	Indian Standard Time	UTC+05:30
// IST	Israel Standard Time	UTC+02:00
// JST	Japan Standard Time	UTC+09:00
// KGT	Kyrgyzstan time	UTC+06:00
// KOST	Kosrae Time	UTC+11:00
// KRAT	Krasnoyarsk Time	UTC+07:00
// KST	Korea Standard Time	UTC+09:00
// LHST	Lord Howe Standard Time	UTC+10:30
// LHST	Lord Howe Summer Time	UTC+11:00
// LINT	Line Islands Time	UTC+14:00
// MAGT	Magadan Time	UTC+12:00
// MART	Marquesas Islands Time	UTC-09:30
// MAWT	Mawson Station Time	UTC+05:00
// MDT	Mountain Daylight Time (North America)	UTC-06:00
// MEST	Middle European Summer Time Same zone as CEST	UTC+02:00
// MET	Middle European Time Same zone as CET	UTC+01:00
// MHT	Marshall Islands	UTC+12:00
// MIST	Macquarie Island Station Time	UTC+11:00
// MIT	Marquesas Islands Time	UTC-09:30
// MMT	Myanmar Standard Time	UTC+06:30
// MSK	Moscow Time	UTC+03:00
// MST	Malaysia Standard Time	UTC+08:00
// MST	Mountain Standard Time (North America)	UTC-07:00
// MUT	Mauritius Time	UTC+04:00
// MVT	Maldives Time	UTC+05:00
// MYT	Malaysia Time	UTC+08:00
// NCT	New Caledonia Time	UTC+11:00
// NDT	Newfoundland Daylight Time	UTC-02:30
// NFT	Norfolk Time	UTC+11:00
// NPT	Nepal Time	UTC+05:45
// NST	Newfoundland Standard Time	UTC-03:30
// NT	Newfoundland Time	UTC-03:30
// NUT	Niue Time	UTC-11:00
// NZDT	New Zealand Daylight Time	UTC+13:00
// NZST	New Zealand Standard Time	UTC+12:00
// OMST	Omsk Time	UTC+06:00
// ORAT	Oral Time	UTC+05:00
// PDT	Pacific Daylight Time (North America)	UTC-07:00
// PET	Peru Time	UTC-05:00
// PETT	Kamchatka Time	UTC+12:00
// PGT	Papua New Guinea Time	UTC+10:00
// PHOT	Phoenix Island Time	UTC+13:00
// PKT	Pakistan Standard Time	UTC+05:00
// PMDT	Saint Pierre and Miquelon Daylight time	UTC-02:00
// PMST	Saint Pierre and Miquelon Standard Time	UTC-03:00
// PONT	Pohnpei Standard Time	UTC+11:00
// PST	Pacific Standard Time (North America)	UTC-08:00
// PST	Philippine Standard Time	UTC+08:00
// PYST	Paraguay Summer Time (South America)[7]	UTC-03:00
// PYT	Paraguay Time (South America)[8]	UTC-04:00
// RET	Réunion Time	UTC+04:00
// ROTT	Rothera Research Station Time	UTC-03:00
// SAKT	Sakhalin Island time	UTC+11:00
// SAMT	Samara Time	UTC+04:00
// SAST	South African Standard Time	UTC+02:00
// SBT	Solomon Islands Time	UTC+11:00
// SCT	Seychelles Time	UTC+04:00
// SGT	Singapore Time	UTC+08:00
// SLST	Sri Lanka Standard Time	UTC+05:30
// SRET	Srednekolymsk Time	UTC+11:00
// SRT	Suriname Time	UTC-03:00
// SST	Samoa Standard Time	UTC-11:00
// SST	Singapore Standard Time	UTC+08:00
// SYOT	Showa Station Time	UTC+03:00
// TAHT	Tahiti Time	UTC-10:00
// TFT	Indian/Kerguelen	UTC+05:00
// THA	Thailand Standard Time	UTC+07:00
// TJT	Tajikistan Time	UTC+05:00
// TKT	Tokelau Time	UTC+13:00
// TLT	Timor Leste Time	UTC+09:00
// TMT	Turkmenistan Time	UTC+05:00
// TOT	Tonga Time	UTC+13:00
// TRT	Turkey Time	UTC+03:00
// TVT	Tuvalu Time	UTC+12:00
// ULAST	Ulaanbaatar Summer Time	UTC+09:00
// ULAT	Ulaanbaatar Standard Time	UTC+08:00
// USZ1	Kaliningrad Time	UTC+02:00
// UTC	Coordinated Universal Time	UTC+00:00
// UYST	Uruguay Summer Time	UTC-02:00
// UYT	Uruguay Standard Time	UTC-03:00
// UZT	Uzbekistan Time	UTC+05:00
// VET	Venezuelan Standard Time	UTC-04:00
// VLAT	Vladivostok Time	UTC+10:00
// VOLT	Volgograd Time	UTC+04:00
// VOST	Vostok Station Time	UTC+06:00
// VUT	Vanuatu Time	UTC+11:00
// WAKT	Wake Island Time	UTC+12:00
// WAST	West Africa Summer Time	UTC+02:00
// WAT	West Africa Time	UTC+01:00
// WEST	Western European Summer Time	UTC+01:00
// WET	Western European Time	UTC+00:00
// WIT	Western Indonesian Time	UTC+07:00
// WST	Western Standard Time	UTC+08:00
// YAKT	Yakutsk Time	UTC+09:00
// YEKT	Yekaterinburg Time	UTC+05:00
//
