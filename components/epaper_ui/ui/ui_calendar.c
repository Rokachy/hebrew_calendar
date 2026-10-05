/**
 * @file ui_calendar.c
 *
 * Landscape layout (800x480):
 *   top:   today's day, Hebrew and Gregorian date, centred
 *   left:  "זמני היום" table (8 times), daily study below it
 *   right: Sunday-Shabbat table (Hebrew day + date, Gregorian date, sunset,
 *          sunrise, event), then the Shabbat times in a big font and other cities
 */

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
    #include "lvgl.h"
    #include "lvgl_private.h"   // lv_text_get_width()
#else
    #include "lvgl/lvgl.h"
    #include "lvgl/lvgl_private.h"
#endif

#include <stdio.h>
#include <string.h>
#include "ui_calendar.h"
#include "hebrew_date.h"
#include "sun_times.h"
#include "location.h"
#include "shabbat.h"
#include "zmanim.h"
#include "daf_yomi.h"
#include "mishna_yomit.h"
#include "holidays.h"
#include "parsha.h"
#include "family_dates.h"

// Fonts compiled from ui/fonts/
LV_FONT_DECLARE(lv_font_heb_13_bold); // long events and study texts, see fit_event(), fit_study()
LV_FONT_DECLARE(lv_font_heb_15_bold); // long events and study texts
LV_FONT_DECLARE(lv_font_heb_18_bold); // default: small texts (cities, bottom line)
LV_FONT_DECLARE(lv_font_heb_22_bold); // day names, titles, zmanim values
LV_FONT_DECLARE(lv_font_heb_26_bold); // dates
LV_FONT_DECLARE(lv_font_heb_112);     // Shabbat times, Bold (digits and ":" only)

// Define color constants for your 4-color E-Ink display panel
#define EINK_COLOR_WHITE  lv_color_make(0xFF, 0xFF, 0xFF)
#define EINK_COLOR_BLACK  lv_color_make(0x00, 0x00, 0x00)
#define EINK_COLOR_RED    lv_color_make(0xFF, 0x00, 0x00)
#define EINK_COLOR_YELLOW lv_color_make(0xFF, 0xCC, 0x00)
#define EINK_COLOR_ORANGE lv_color_make(0xFF, 0x80, 0x00)   // red/yellow checkerboard on the panel, see epd_palette.h

#define UI_MARGIN       10

// ---- Top line: today's full date, centred across the screen ----
#define TODAY_Y         5
#define TODAY_H         34
#define CONTENT_Y       (TODAY_Y + TODAY_H + 6)   // both sides start here, headers aligned

// ---- Right panel: the week ----
#define WEEK_DAYS       7   // Sunday - Shabbat
#define WEEK_X          (INFO_X + INFO_W + 15)
#define WEEK_W          (UI_HOR_RES - UI_MARGIN - WEEK_X)
// Columns inside the week panel, left to right; the event column takes what is left
#define HEB_DATE_W      48
#define DAY_NAME_W      78
#define DAY_TAG_W       (HEB_DATE_W + DAY_NAME_W)
#define GREG_W          56
#define SUNRISE_W       70
#define SUNSET_W        70
#define EVENT_W         (WEEK_W - DAY_TAG_W - GREG_W - SUNSET_W - SUNRISE_W)
#define EVENT_X         0
// Read right to left: day, dates, sunrise, sunset, event
#define SUNSET_X        (EVENT_X + EVENT_W)
#define SUNRISE_X       (SUNSET_X + SUNSET_W)
#define GREG_X          (SUNRISE_X + SUNRISE_W)
#define DAY_TAG_X       (GREG_X + GREG_W)
// The day tag is split: Hebrew date on the left, day name on the right
#define DAY_NAME_X      (DAY_TAG_X + HEB_DATE_W)
#define DAY_TAG_PAD     4   // day name distance from the right edge

#define COL_TITLE_Y     CONTENT_Y
#define COL_TITLE_H     SECTION_TITLE_H             // same height as the left headers
#define ROW_START_Y     (COL_TITLE_Y + COL_TITLE_H + 2)
#define ROW_H           30  // other days: 22 px text
#define ROW_H_TODAY     36  // today: 26 px text, yellow
#define WEEK_TABLE_H    ((WEEK_DAYS - 1) * ROW_H + ROW_H_TODAY)

#define SHABBAT_TITLE_Y (ROW_START_Y + WEEK_TABLE_H + 8)
#define SHABBAT_TITLE_H 30
#define SHABBAT_TITLE_GAP 6   // white gap between the two black title bars
#define BIG_TIME_Y      (SHABBAT_TITLE_Y + SHABBAT_TITLE_H + 2)
#define BIG_TIME_H      86  // 112 px font is 80 px tall
#define CITY_STRIP_Y    (BIG_TIME_Y + BIG_TIME_H + 4)
#define CITY_STRIP_H    30  // "name time" on one line per city

// ---- Bottom line (English, debug): when the screen was last updated ----
#define FOOTER_Y        (CITY_STRIP_Y + CITY_STRIP_H + 2)
#define FOOTER_H        (UI_VER_RES - FOOTER_Y)

// ---- Left panel: זמני היום ----
#define INFO_X          UI_MARGIN
#define INFO_W          270
#define ZMANIM_TITLE_Y  CONTENT_Y
#define SECTION_TITLE_H 30
#define ZMANIM_COUNT    ZMAN_COUNT   // same order as zman_t in zmanim.h
#define INFO_TEXT_PAD   6
// Under the last time row: צאת הכוכבים, רבינו תם (Fri / Shabbat only), a red
// separator, then the daily study rows (no title bar) - see layout_lower_left()
#define LOWER_LEFT_Y    (ROW_START_Y + WEEK_TABLE_H + (ZMANIM_COUNT - WEEK_DAYS) * ROW_H)
#define STUDY_COUNT     2

static const char * const study_names[STUDY_COUNT] = {
    "דף יומי", "משנה",
};

static const char * const zman_names[ZMANIM_COUNT] = {
    "עלות השחר", "ציצית ותפילין", "סו\"ז ק\"ש מג\"א", "סו\"ז ק\"ש גר\"א",
    "סו\"ז ת' גר\"א", "חצות היום", "מנחה גדולה", "פלג המנחה",
};
static const char * const heb_days[7] = {"ראשון", "שני", "שלישי", "רביעי", "חמישי", "שישי", "שבת"};
static const char * const greg_months[12] = {
    "ינואר", "פברואר", "מרץ", "אפריל", "מאי", "יוני",
    "יולי", "אוגוסט", "ספטמבר", "אוקטובר", "נובמבר", "דצמבר",
};
// Short forms for the narrow table header column
static const char * const greg_months_short[12] = {
    "ינואר", "פבר", "מרץ", "אפר", "מאי", "יוני",
    "יולי", "אוג", "ספטמ", "אוקט", "נוב", "דצמ",
};

#define CITY_COUNT      LOC_CITY_COUNT

// Week table (0 = Sunday ... 6 = Shabbat)
static lv_obj_t *lbl_heb_day_nums[WEEK_DAYS];
static lv_obj_t *lbl_greg_day_nums[WEEK_DAYS];
static lv_obj_t *lbl_sunset_times[WEEK_DAYS];
static lv_obj_t *lbl_sunrise_times[WEEK_DAYS];
static lv_obj_t *lbl_events[WEEK_DAYS];
static lv_obj_t *event_cells[WEEK_DAYS];   // orange when the day has an event
static lv_obj_t *today_marks[WEEK_DAYS];   // yellow bar, shown on today's row only
static lv_obj_t *week_rows[WEEK_DAYS];
static lv_obj_t *zman_rows[ZMANIM_COUNT];   // level with week_rows, see set_today()
static lv_obj_t *lbl_study_values[STUDY_COUNT];
static lv_obj_t *study_rows[STUDY_COUNT];
static lv_obj_t *tzeit_row;                  // צאת הכוכבים, every day
static lv_obj_t *lbl_tzeit_value;
static lv_obj_t *rt_row;                     // רבינו תם, Friday and Shabbat only
static lv_obj_t *lbl_rt_value;
static lv_obj_t *study_separator;            // red line above the study rows
#define STUDY_SEPARATOR_H 4                  // 4 times the 1 px black row lines

// Column titles for dynamic text updating
static lv_obj_t *lbl_header_hebrew_month;
static lv_obj_t *lbl_header_month;

// Shabbat
static lv_obj_t *lbl_candle_lighting_time;
static lv_obj_t *lbl_havdalah_time;
static lv_obj_t *lbl_candle_city_times[CITY_COUNT];
static lv_obj_t *lbl_havdalah_city_times[CITY_COUNT];

// Left panel values
static lv_obj_t *lbl_today_hebrew;
static lv_obj_t *lbl_today_greg;
static lv_obj_t *lbl_parsha;
static lv_obj_t *lbl_updated;
static lv_obj_t *lbl_note;
static lv_obj_t *lbl_zman_values[ZMANIM_COUNT];

/** Plain rectangle: no border, padding, radius or scrolling */
static lv_obj_t * make_box(lv_obj_t *parent, int32_t x, int32_t y, int32_t w, int32_t h)
{
    lv_obj_t *box = lv_obj_create(parent);
    lv_obj_set_pos(box, x, y);
    lv_obj_set_size(box, w, h);
    lv_obj_set_style_border_width(box, 0, 0);
    lv_obj_set_style_pad_all(box, 0, 0);
    lv_obj_set_style_radius(box, 0, 0);
    lv_obj_set_style_bg_opa(box, LV_OPA_TRANSP, 0);
    lv_obj_set_scrollable(box, false);
    return box;
}

static void set_fill(lv_obj_t *obj, lv_color_t color)
{
    lv_obj_set_style_bg_color(obj, color, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
}

static void set_bottom_line(lv_obj_t *obj)
{
    lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_width(obj, 1, 0);
    lv_obj_set_style_border_color(obj, EINK_COLOR_BLACK, 0);
}

/** Label aligned inside its parent; font NULL = inherited */
static lv_obj_t * make_label(lv_obj_t *parent, const char *text, const lv_font_t *font,
                             lv_align_t align, int32_t x_ofs, bool rtl)
{
    lv_obj_t *lbl = lv_label_create(parent);
    if(font) lv_obj_set_style_text_font(lbl, font, 0);
    if(rtl) lv_obj_set_style_base_dir(lbl, LV_BASE_DIR_RTL, 0);
    lv_label_set_text(lbl, text);
    lv_obj_align(lbl, align, x_ofs, 0);
    return lbl;
}

/** Red bar with a white centred title */
static void make_section_title(lv_obj_t *parent, int32_t y, const char *text)
{
    lv_obj_t *bar = make_box(parent, INFO_X, y, INFO_W, SECTION_TITLE_H);
    set_fill(bar, EINK_COLOR_RED);
    lv_obj_set_style_text_color(bar, EINK_COLOR_WHITE, 0);
    make_label(bar, text, &lv_font_heb_22_bold, LV_ALIGN_CENTER, 0, true);
}

/** One line in the event column: the row's font if the text fits, else the
 *  first smaller font it fits in (18, 15, 13); still too wide -> ends with "..." */
static void fit_event(lv_obj_t *lbl, const lv_font_t *font)
{
    const lv_font_t *const fonts[] = { font, &lv_font_heb_18_bold, &lv_font_heb_15_bold, &lv_font_heb_13_bold };
    const char *text = lv_label_get_text(lbl);
    lv_text_attributes_t attr;
    lv_text_attributes_init(&attr);
    for (size_t i = 0; i < sizeof(fonts) / sizeof(fonts[0]); i++) {
        font = fonts[i];
        if (lv_text_get_width(text, strlen(text), font, &attr) <= EVENT_W - 8) break;   // the label's width, see create_week()
    }
    lv_obj_set_style_text_font(lbl, font, 0);
    lv_obj_set_height(lbl, lv_font_get_line_height(font));
}

/** A study row's text: 22 px if it fits beside the row's name, else the first
 *  smaller font it fits in (18, 15, 13). Mishna yomit across two tractates
 *  ("בבא קמא: י, י - בבא מציעא: א, א") needs a smaller one. */
static void fit_study(int row)
{
    const lv_font_t *const fonts[] = { &lv_font_heb_22_bold, &lv_font_heb_18_bold,
                                       &lv_font_heb_15_bold, &lv_font_heb_13_bold };
    lv_text_attributes_t attr;
    lv_text_attributes_init(&attr);
    const char *name = study_names[row];
    int32_t space = INFO_W - 2 * INFO_TEXT_PAD - 10
                    - lv_text_get_width(name, strlen(name), &lv_font_heb_22_bold, &attr);
    const char *text = lv_label_get_text(lbl_study_values[row]);
    const lv_font_t *font = NULL;
    for (size_t i = 0; i < sizeof(fonts) / sizeof(fonts[0]); i++) {
        font = fonts[i];
        if (lv_text_get_width(text, strlen(text), font, &attr) <= space) break;
    }
    lv_obj_set_style_text_font(lbl_study_values[row], font, 0);
}

/**
 * Today's row is taller, 26 px and yellow; the other rows are 22 px.
 * The table height is the same for any day, so nothing below it moves.
 * The זמני היום rows on the left copy each week row's position and height, so the
 * row lines of both tables are level (both have 7 rows).
 */
static void set_today(int today)
{
    int32_t y = ROW_START_Y;
    for (int i = 0; i < WEEK_DAYS; i++) {
        bool is_today = (i == today);
        int32_t h = is_today ? ROW_H_TODAY : ROW_H;
        lv_obj_set_pos(week_rows[i], WEEK_X, y);
        lv_obj_set_height(week_rows[i], h);
        if (i < ZMANIM_COUNT) {
            lv_obj_set_pos(zman_rows[i], INFO_X, y);
            lv_obj_set_height(zman_rows[i], h);
        }
        const lv_font_t *font = is_today ? &lv_font_heb_26_bold : &lv_font_heb_22_bold;
        lv_obj_set_style_text_font(week_rows[i], font, 0);
        fit_event(lbl_events[i], font);
        lv_obj_set_height(event_cells[i], h - 1);   // keep the row line visible
        lv_obj_set_height(today_marks[i], h - 1);
        lv_obj_set_hidden(today_marks[i], !is_today);
        y += h;
    }
    // זמני היום rows beyond the 7 week rows (פלג המנחה) continue below them
    for (int i = WEEK_DAYS; i < ZMANIM_COUNT; i++) {
        lv_obj_set_pos(zman_rows[i], INFO_X, y);
        lv_obj_set_height(zman_rows[i], ROW_H);
        y += ROW_H;
    }
}

/**
 * Below the time rows, packed with no gaps, all rows as tall as the time rows:
 * צאת הכוכבים, רבינו תם (Friday and Shabbat only), the red separator in place of
 * the black line under the last of them, then the study rows. Any space left at
 * the bottom stays empty.
 */
static void layout_lower_left(bool show_rt)
{
    int32_t y = LOWER_LEFT_Y;
    lv_obj_set_pos(tzeit_row, INFO_X, y);
    y += ROW_H;
    lv_obj_set_hidden(rt_row, !show_rt);
    if (show_rt) {
        lv_obj_set_pos(rt_row, INFO_X, y);
        y += ROW_H;
    }

    // The red line covers the 1 px black line at the bottom of the row above it
    y -= 1;
    lv_obj_set_pos(study_separator, INFO_X, y);
    y += STUDY_SEPARATOR_H;
    for (int i = 0; i < STUDY_COUNT; i++) {
        lv_obj_set_pos(study_rows[i], INFO_X, y + i * ROW_H);
        lv_obj_set_height(study_rows[i], ROW_H);
    }
}

/** Local time as 12-hour "6:24" (as printed calendars do), rounded to the nearest minute */
static void format_time(time_t utc, char *buf, size_t len) {
    utc += 30;
    struct tm *t = localtime(&utc);
    int hour = t->tm_hour % 12;
    snprintf(buf, len, "%d:%02d", hour == 0 ? 12 : hour, t->tm_min);
}

/** Visible sunrise / sea-level sunset at the main location, "--:--" if there is none */
static void show_sun_time(lv_obj_t *label, const struct tm *day, bool rising) {
    char buf[8] = "--:--";
    time_t t;
    if (city_sunrise_sunset(&loc_main, day->tm_year + 1900, day->tm_mon + 1, day->tm_mday, rising, &t)) {
        format_time(t, buf, sizeof(buf));
    }
    lv_label_set_text(label, buf);
}

/** Candle lighting for `city` on `friday`, "--:--" if there is none */
static void show_candle_time(lv_obj_t *label, const city_t *city, const struct tm *friday) {
    char buf[8] = "--:--";
    time_t t;
    if (shabbat_candle_lighting(city, friday->tm_year + 1900, friday->tm_mon + 1, friday->tm_mday, &t)) {
        format_time(t, buf, sizeof(buf));
    }
    lv_label_set_text(label, buf);
}

/** End of Shabbat for `city` on `saturday`, "--:--" if there is none */
static void show_end_time(lv_obj_t *label, const city_t *city, const struct tm *saturday) {
    char buf[8] = "--:--";
    time_t t;
    if (shabbat_end(city, saturday->tm_year + 1900, saturday->tm_mon + 1, saturday->tm_mday, &t)) {
        format_time(t, buf, sizeof(buf));
    }
    lv_label_set_text(label, buf);
}

void ui_calendar_update(const struct tm *today) {
    int today_idx = today->tm_wday;   // 0 = Sunday ... 6 = Shabbat

    char buf[48];

    // Gregorian and Hebrew dates of this week's Sunday ... Shabbat
    for (int i = 0; i < WEEK_DAYS; i++) {
        struct tm d = *today;
        d.tm_mday += i - today_idx;
        d.tm_hour = 12;                // midday: safe from DST changes
        d.tm_isdst = -1;
        mktime(&d);                    // normalises across month / year ends
        lv_label_set_text_fmt(lbl_greg_day_nums[i], "%d", d.tm_mday);

        hdate_t hd = hd_from_greg(d.tm_year + 1900, d.tm_mon + 1, d.tm_mday);
        hd_format_number(hd.day, false, buf, sizeof(buf));   // "כו": no marks in the narrow column
        lv_label_set_text(lbl_heb_day_nums[i], buf);

        show_sun_time(lbl_sunrise_times[i], &d, true);
        show_sun_time(lbl_sunset_times[i], &d, false);

        // Family dates first (so a cut-off line still shows the name), then holidays,
        // fasts, ראש חודש (empty if none), black on orange
        char event[160], holiday[96];
        family_events(d.tm_year + 1900, d.tm_mon + 1, d.tm_mday, event, sizeof(event));
        if (holiday_events(d.tm_year + 1900, d.tm_mon + 1, d.tm_mday, holiday, sizeof(holiday))) {
            if (event[0]) {
                // "ראש חודש אייר" alone -> "ר"ח", as holidays.c does next to a holiday
                if (strncmp(holiday, "ראש חודש", strlen("ראש חודש")) == 0) strcpy(holiday, "ר\"ח");
                strncat(event, ", ", sizeof(event) - strlen(event) - 1);
            }
            strncat(event, holiday, sizeof(event) - strlen(event) - 1);
        }
        lv_label_set_text(lbl_events[i], event);
        lv_obj_set_style_bg_opa(event_cells[i], event[0] ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
    }
    set_today(today_idx);

    // Top line and column headers: today's Hebrew and Gregorian date
    hdate_t hd_today = hd_from_greg(today->tm_year + 1900, today->tm_mon + 1, today->tm_mday);
    hd_format_date(&hd_today, buf, sizeof(buf));
    lv_label_set_text(lbl_today_hebrew, buf);
    // Bottom line: the moment this screen was drawn (health check for the device)
    lv_label_set_text_fmt(lbl_updated, "%d.%d.%d %02d:%02d", today->tm_mday, today->tm_mon + 1,
                          today->tm_year + 1900, today->tm_hour, today->tm_min);

    lv_label_set_text_fmt(lbl_today_greg, "%d %s %d", today->tm_mday,
                          greg_months[today->tm_mon], today->tm_year + 1900);
    lv_label_set_text(lbl_header_hebrew_month, hd_month_name(hd_today.month, hd_today.year));
    lv_label_set_text(lbl_header_month, greg_months_short[today->tm_mon]);

    // Today's זמני היום at the main location (definitions in zmanim.h)
    time_t zmanim[ZMAN_COUNT];
    bool have_zmanim = zmanim_calc(&loc_main, today->tm_year + 1900, today->tm_mon + 1,
                                   today->tm_mday, zmanim);
    for (int i = 0; i < ZMANIM_COUNT; i++) {
        char t[8] = "--:--";
        if (have_zmanim) format_time(zman_rounded((zman_t)i, zmanim[i]), t, sizeof(t));
        lv_label_set_text(lbl_zman_values[i], t);
    }


    // Daily study: daf yomi and mishna yomit, both calculated from the date
    const char *tractate;
    int daf;
    if (daf_yomi(today->tm_year + 1900, today->tm_mon + 1, today->tm_mday, &tractate, &daf)) {
        char num[16];
        hd_format_number(daf, true, num, sizeof(num));
        lv_label_set_text_fmt(lbl_study_values[0], "%s: %s", tractate, num);
    } else {
        lv_label_set_text(lbl_study_values[0], "--");
    }
    char mishna[96];
    mishna_yomit_format(today->tm_year + 1900, today->tm_mon + 1, today->tm_mday, mishna, sizeof(mishna));
    lv_label_set_text(lbl_study_values[1], mishna);
    for (int i = 0; i < STUDY_COUNT; i++) fit_study(i);

    // This week's Shabbat: candle lighting on Friday (rules in location.c)
    struct tm friday = *today;
    friday.tm_mday += 5 - today_idx;
    friday.tm_hour = 12;
    friday.tm_isdst = -1;
    mktime(&friday);
    show_candle_time(lbl_candle_lighting_time, &loc_main, &friday);
    for (int i = 0; i < CITY_COUNT; i++) {
        show_candle_time(lbl_candle_city_times[i], &loc_cities[i], &friday);
    }

    // End of Shabbat on Saturday evening (צאת הכוכבים, see shabbat.h)
    struct tm saturday = friday;
    saturday.tm_mday += 1;
    mktime(&saturday);
    show_end_time(lbl_havdalah_time, &loc_main, &saturday);

    // Top line, left end: this week's portion (or the holiday read on that Shabbat)
    char portion[64];
    if (parsha_of_shabbat(saturday.tm_year + 1900, saturday.tm_mon + 1, saturday.tm_mday,
                          portion, sizeof(portion))) {
        lv_label_set_text_fmt(lbl_parsha, "פרשת שבוע: %s", portion);
    } else {
        lv_label_set_text(lbl_parsha, "");
    }
    for (int i = 0; i < CITY_COUNT; i++) {
        show_end_time(lbl_havdalah_city_times[i], &loc_cities[i], &saturday);
    }

    // צאת הכוכבים of today, every day (13.5 seasonal minutes after sunset, see zmanim.h)
    char buf_eve[8] = "--:--";
    time_t t_eve;
    if (zman_tzeit(&loc_main, today->tm_year + 1900, today->tm_mon + 1, today->tm_mday, &t_eve)) {
        format_time(t_eve, buf_eve, sizeof(buf_eve));
    }
    lv_label_set_text(lbl_tzeit_value, buf_eve);

    // רבינו תם (end of this Shabbat, 72 min), on Friday and Shabbat only
    bool show_rt = today_idx == 5 || today_idx == 6;
    if (show_rt) {
        strcpy(buf_eve, "--:--");
        if (shabbat_end_rabbeinu_tam(&loc_main, saturday.tm_year + 1900, saturday.tm_mon + 1,
                                     saturday.tm_mday, &t_eve)) {
            format_time(t_eve, buf_eve, sizeof(buf_eve));
        }
        lv_label_set_text(lbl_rt_value, buf_eve);
    }
    layout_lower_left(show_rt);
}

/**
 * Yellow block under one big Shabbat time: the 3 cities with their own time for it,
 * "name time" on one line per city, first city on the right.
 * Name and time are separate labels: in one RTL label LVGL splits "6:14" at the ':'.
 */
static void create_city_times(lv_obj_t *scr, int32_t x, lv_obj_t *time_labels[CITY_COUNT])
{
    int32_t block_w = (WEEK_W - SHABBAT_TITLE_GAP) / 2;
    lv_obj_t *block = make_box(scr, x, CITY_STRIP_Y, block_w, CITY_STRIP_H);
    set_fill(block, EINK_COLOR_YELLOW);
    // Cities spread evenly by their real width (names differ in length)
    lv_obj_set_flex_flow(block, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(block, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_base_dir(block, LV_BASE_DIR_RTL, 0);   // first city on the right

    for (int i = 0; i < CITY_COUNT; i++) {
        lv_obj_t *city = lv_obj_create(block);
        lv_obj_remove_style_all(city);                       // plain container, no theme
        lv_obj_set_size(city, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_flex_flow(city, LV_FLEX_FLOW_ROW);
        lv_obj_set_style_pad_column(city, 5, 0);             // name on the right, time on the left

        lv_obj_t *name = lv_label_create(city);
        lv_label_set_text(name, loc_cities[i].short_name);
        time_labels[i] = lv_label_create(city);
        lv_obj_set_style_base_dir(time_labels[i], LV_BASE_DIR_LTR, 0);
        lv_label_set_text(time_labels[i], "--:--");
    }
}

/** Red bar with white title above one of the big Shabbat times */
static void make_shabbat_title(lv_obj_t *scr, int32_t x, const char *text)
{
    lv_obj_t *bar = make_box(scr, x, SHABBAT_TITLE_Y, (WEEK_W - SHABBAT_TITLE_GAP) / 2, SHABBAT_TITLE_H);
    set_fill(bar, EINK_COLOR_RED);
    lv_obj_set_style_text_color(bar, EINK_COLOR_WHITE, 0);
    make_label(bar, text, &lv_font_heb_22_bold, LV_ALIGN_CENTER, 0, true);
}

static void create_week(lv_obj_t *scr)
{

    // Column titles, all 22 px, white on red like the other headers
    lv_obj_t *titles = make_box(scr, WEEK_X, COL_TITLE_Y, WEEK_W, COL_TITLE_H);
    set_fill(titles, EINK_COLOR_RED);
    lv_obj_set_style_text_font(titles, &lv_font_heb_22_bold, 0);

    lv_obj_t *cell = make_box(titles, EVENT_X, 0, EVENT_W, COL_TITLE_H);
    make_label(cell, "אירוע", NULL, LV_ALIGN_CENTER, 0, true);
    cell = make_box(titles, SUNRISE_X, 0, SUNRISE_W, COL_TITLE_H);
    make_label(cell, "זריחה", NULL, LV_ALIGN_CENTER, 0, true);
    cell = make_box(titles, SUNSET_X, 0, SUNSET_W, COL_TITLE_H);
    make_label(cell, "שקיעה", NULL, LV_ALIGN_CENTER, 0, true);
    cell = make_box(titles, GREG_X - 10, 0, GREG_W + 20, COL_TITLE_H); // month name is wider than the column
    lbl_header_month = make_label(cell, "", NULL, LV_ALIGN_CENTER, 0, true);
    cell = make_box(titles, DAY_TAG_X - 10, 0, HEB_DATE_W + 20, COL_TITLE_H); // month name is wider than the column
    lbl_header_hebrew_month = make_label(cell, "", NULL, LV_ALIGN_CENTER, 0, true);
    cell = make_box(titles, DAY_NAME_X, 0, DAY_NAME_W, COL_TITLE_H);
    make_label(cell, "יום", NULL, LV_ALIGN_RIGHT_MID, -DAY_TAG_PAD, true);
    // White on each title cell: the theme gives every box black text
    for (uint32_t c = 0; c < lv_obj_get_child_count(titles); c++) {
        lv_obj_set_style_text_color(lv_obj_get_child(titles, (int32_t)c), EINK_COLOR_WHITE, 0);
    }

    // Sunday - Shabbat rows
    for (int i = 0; i < WEEK_DAYS; i++) {
        // Position, height and font are set by set_today()
        lv_obj_t *row = make_box(scr, WEEK_X, ROW_START_Y + i * ROW_H, WEEK_W, ROW_H);
        week_rows[i] = row;
        set_bottom_line(row);
        lv_obj_set_style_text_font(row, &lv_font_heb_22_bold, 0);   // one size for the whole row

        // Today: yellow from the day name to sunset (not the event), above the bottom line
        today_marks[i] = make_box(row, SUNSET_X, 0, WEEK_W - SUNSET_X, ROW_H - 1);
        set_fill(today_marks[i], EINK_COLOR_YELLOW);
        lv_obj_set_hidden(today_marks[i], true);

        // Event: black on orange (only on days with an event), font in fit_event()
        cell = make_box(row, EVENT_X, 0, EVENT_W, ROW_H - 1);
        event_cells[i] = cell;
        set_fill(cell, EINK_COLOR_YELLOW);
        lbl_events[i] = make_label(cell, "", NULL, LV_ALIGN_CENTER, 0, true);
        lv_obj_set_style_text_color(lbl_events[i], EINK_COLOR_BLACK, 0);
        // Fixed width, one line (height in set_today): text that does not fit ends with "..."
        lv_obj_set_width(lbl_events[i], EVENT_W - 8);
        lv_label_set_long_mode(lbl_events[i], LV_LABEL_LONG_MODE_DOTS);
        lv_obj_set_style_text_align(lbl_events[i], LV_TEXT_ALIGN_CENTER, 0);
        cell = make_box(row, SUNRISE_X, 0, SUNRISE_W, LV_PCT(100));
        lbl_sunrise_times[i] = make_label(cell, "--:--", NULL, LV_ALIGN_CENTER, 0, false);
        cell = make_box(row, SUNSET_X, 0, SUNSET_W, LV_PCT(100));
        lbl_sunset_times[i] = make_label(cell, "--:--", NULL, LV_ALIGN_CENTER, 0, false);

        // Gregorian date
        cell = make_box(row, GREG_X, 0, GREG_W, LV_PCT(100));
        lbl_greg_day_nums[i] = make_label(cell, "-", NULL, LV_ALIGN_CENTER, 0, false);

        // Hebrew date (left) + day name (right)
        cell = make_box(row, DAY_TAG_X, 0, HEB_DATE_W, LV_PCT(100));
        lbl_heb_day_nums[i] = make_label(cell, "--", NULL, LV_ALIGN_CENTER, 0, true);
        cell = make_box(row, DAY_NAME_X, 0, DAY_NAME_W, LV_PCT(100));
        make_label(cell, heb_days[i], NULL, LV_ALIGN_RIGHT_MID, -DAY_TAG_PAD, true);
    }

    // Shabbat: titles, big times (havdalah left, candle lighting right), other cities.
    // (No red line under the table any more; the title bars are red. Its space is kept.)

    int32_t bar_w = (WEEK_W - SHABBAT_TITLE_GAP) / 2;
    make_shabbat_title(scr, WEEK_X, "יציאת השבת");
    make_shabbat_title(scr, WEEK_X + WEEK_W - bar_w, "כניסת השבת");

    lv_obj_t *big = make_box(scr, WEEK_X, BIG_TIME_Y, WEEK_W / 2, BIG_TIME_H);
    lbl_havdalah_time = make_label(big, "0:00", &lv_font_heb_112, LV_ALIGN_CENTER, 0, false);
    big = make_box(scr, WEEK_X + WEEK_W / 2, BIG_TIME_Y, WEEK_W / 2, BIG_TIME_H);
    lbl_candle_lighting_time = make_label(big, "0:00", &lv_font_heb_112, LV_ALIGN_CENTER, 0, false);

    create_city_times(scr, WEEK_X, lbl_havdalah_city_times);
    create_city_times(scr, WEEK_X + WEEK_W - bar_w, lbl_candle_city_times);
}

static void create_info(lv_obj_t *scr)
{
    // Zmanim: name on the right, time on the left
    make_section_title(scr, ZMANIM_TITLE_Y, "זמני היום");
    for (int i = 0; i < ZMANIM_COUNT; i++) {
        // Position and height are set by set_today(), level with the week rows
        int32_t y = ROW_START_Y + i * ROW_H;
        lv_obj_t *row = make_box(scr, INFO_X, y, INFO_W, ROW_H);
        zman_rows[i] = row;
        set_bottom_line(row);
        lv_obj_set_style_text_font(row, &lv_font_heb_22_bold, 0);   // one size for the whole row
        make_label(row, zman_names[i], NULL, LV_ALIGN_RIGHT_MID, -INFO_TEXT_PAD, true);
        lbl_zman_values[i] = make_label(row, "--:--", NULL, LV_ALIGN_LEFT_MID, INFO_TEXT_PAD, false);
    }

    // צאת הכוכבים (every day) and רבינו תם (Friday and Shabbat only);
    // positions set by layout_lower_left()
    tzeit_row = make_box(scr, INFO_X, LOWER_LEFT_Y, INFO_W, ROW_H);
    set_bottom_line(tzeit_row);
    lv_obj_set_style_text_font(tzeit_row, &lv_font_heb_22_bold, 0);
    make_label(tzeit_row, "צאת הכוכבים", NULL, LV_ALIGN_RIGHT_MID, -INFO_TEXT_PAD, true);
    lbl_tzeit_value = make_label(tzeit_row, "--:--", NULL, LV_ALIGN_LEFT_MID, INFO_TEXT_PAD, false);

    rt_row = make_box(scr, INFO_X, LOWER_LEFT_Y, INFO_W, ROW_H);
    set_bottom_line(rt_row);
    lv_obj_set_style_text_font(rt_row, &lv_font_heb_22_bold, 0);
    make_label(rt_row, "רבינו תם", NULL, LV_ALIGN_RIGHT_MID, -INFO_TEXT_PAD, true);
    lbl_rt_value = make_label(rt_row, "--:--", NULL, LV_ALIGN_LEFT_MID, INFO_TEXT_PAD, false);

    // Red separator above the study rows, 4 times as thick as the black row lines
    study_separator = make_box(scr, INFO_X, LOWER_LEFT_Y, INFO_W, STUDY_SEPARATOR_H);
    set_fill(study_separator, EINK_COLOR_RED);

    // Daily study: name on the right, text on the left (no title bar);
    // position and height set by layout_lower_left()
    for (int i = 0; i < STUDY_COUNT; i++) {
        lv_obj_t *row = make_box(scr, INFO_X, LOWER_LEFT_Y, INFO_W, ROW_H);
        study_rows[i] = row;
        set_bottom_line(row);
        lv_obj_set_style_text_font(row, &lv_font_heb_22_bold, 0);   // one size for the whole row
        make_label(row, study_names[i], NULL, LV_ALIGN_RIGHT_MID, -INFO_TEXT_PAD, true);
        lbl_study_values[i] = make_label(row, "", NULL, LV_ALIGN_LEFT_MID, INFO_TEXT_PAD, true);
    }
}

/**
 * Bottom line, small, in English at the left (a debug / health line):
 *   "Updated 1.10.2026 00:01  #15  NTP 14/15  drift ..."
 * If it does not show today's date, the device has stopped updating (the e-paper
 * keeps the last image without power). The note part is filled by the HW.
 * At the right end: which horizon the times are for ("אופק תל אביב").
 */
static void create_footer(lv_obj_t *scr)
{
    lv_obj_t *line = make_box(scr, UI_MARGIN, FOOTER_Y, UI_HOR_RES - 2 * UI_MARGIN, FOOTER_H);
    lv_obj_set_style_base_dir(line, LV_BASE_DIR_LTR, 0);
    lv_obj_set_flex_flow(line, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(line, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(line, 6, 0);

    lv_obj_t *title = lv_label_create(line);
    lv_label_set_text(title, "Updated");
    lbl_updated = lv_label_create(line);
    lv_label_set_text(lbl_updated, "");
    lbl_note = lv_label_create(line);                       // debug info from the HW
    lv_label_set_text(lbl_note, "");

    // On the screen, not in the line: the line lays its labels out from the left
    lv_obj_t *horizon = lv_label_create(scr);
    lv_obj_set_style_base_dir(horizon, LV_BASE_DIR_RTL, 0);
    lv_label_set_text_fmt(horizon, "אופק %s", loc_main.name);
    lv_obj_align_to(horizon, line, LV_ALIGN_RIGHT_MID, -6, 0);
}

void ui_calendar_set_note(const char *text) {
    if (lbl_note) lv_label_set_text(lbl_note, text ? text : "");
}

/**
 * Top line on yellow, right to left: "Hebrew date   Gregorian date" from the
 * right edge, and "פרשת שבוע: ..." (this week's Shabbat) at the left edge.
 * (No day name: today's row in the table is highlighted.)
 * Separate labels: numbers inside one RTL label can be reordered by LVGL.
 * Full month names fit here, e.g. "כ\"ט חשוון תשפ\"ז  28 אוקטובר 2026".
 */
static void create_today(lv_obj_t *scr)
{
    lv_obj_t *line = make_box(scr, UI_MARGIN, TODAY_Y, UI_HOR_RES - 2 * UI_MARGIN, TODAY_H);
    set_fill(line, EINK_COLOR_YELLOW);
    lv_obj_set_style_text_font(line, &lv_font_heb_26_bold, 0);
    lv_obj_set_style_base_dir(line, LV_BASE_DIR_RTL, 0);   // first item on the right
    lv_obj_set_flex_flow(line, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(line, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(line, 30, 0);   // space between the two dates
    lv_obj_set_style_pad_hor(line, 8, 0);

    lbl_today_hebrew = lv_label_create(line);
    lbl_today_greg = lv_label_create(line);

    // פרשת שבוע at the left end. On the screen, not in the line (the line lays its
    // labels out from the right); anchored by its left edge, so any length fits.
    lbl_parsha = lv_label_create(scr);
    lv_obj_set_style_text_font(lbl_parsha, &lv_font_heb_26_bold, 0);
    lv_obj_set_style_base_dir(lbl_parsha, LV_BASE_DIR_RTL, 0);
    lv_label_set_text(lbl_parsha, "");
    lv_obj_align(lbl_parsha, LV_ALIGN_TOP_LEFT, UI_MARGIN + 8, TODAY_Y + (TODAY_H - 29) / 2);
}

void ui_calendar_create(void) {
    lv_display_t * disp = lv_display_get_default();
    if(disp == NULL) return;

    lv_obj_t *scr = lv_display_get_screen_active(disp);
    lv_obj_set_style_bg_color(scr, EINK_COLOR_WHITE, 0);
    lv_obj_set_size(scr, UI_HOR_RES, UI_VER_RES);
    lv_obj_set_style_pad_all(scr, 0, 0);
    lv_obj_set_scrollable(scr, false);
    // Every label without its own font inherits this
    lv_obj_set_style_text_font(scr, &lv_font_heb_18_bold, 0);

    create_today(scr);
    create_footer(scr);
    create_week(scr);
    create_info(scr);
}
