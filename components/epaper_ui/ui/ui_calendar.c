/**
 * @file ui_calendar.c
 *
 * Landscape layout (800x480):
 *   top:   today's day, Hebrew and Gregorian date, centred
 *   left:  "זמני היום" table, "לימוד יומי" table
 *   right: Sunday-Shabbat table (Hebrew day + date, Gregorian date, sunset,
 *          sunrise, event), then the Shabbat times in a big font and other cities
 */

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
    #include "lvgl.h"
#else
    #include "lvgl/lvgl.h"
#endif

#include "ui_calendar.h"

// Fonts compiled from ui/fonts/
LV_FONT_DECLARE(lv_font_heb_18_bold); // default: times, zmanim names, study texts
LV_FONT_DECLARE(lv_font_heb_22_bold); // day names, titles, zmanim values
LV_FONT_DECLARE(lv_font_heb_26_bold); // dates
LV_FONT_DECLARE(lv_font_heb_112);     // Shabbat times, Bold (digits and ":" only)

// Define color constants for your 4-color E-Ink display panel
#define EINK_COLOR_WHITE  lv_color_make(0xFF, 0xFF, 0xFF)
#define EINK_COLOR_BLACK  lv_color_make(0x00, 0x00, 0x00)
#define EINK_COLOR_RED    lv_color_make(0xFF, 0x00, 0x00)
#define EINK_COLOR_YELLOW lv_color_make(0xFF, 0xCC, 0x00)

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
#define SUNRISE_X       (EVENT_X + EVENT_W)
#define SUNSET_X        (SUNRISE_X + SUNRISE_W)
#define GREG_X          (SUNSET_X + SUNSET_W)
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

// ---- Left panel: zmanim and daily study ----
#define INFO_X          UI_MARGIN
#define INFO_W          270
#define ZMANIM_TITLE_Y  CONTENT_Y
#define SECTION_TITLE_H 30
#define ZMANIM_COUNT    7
#define ZMAN_ROW_H      31
// "לימוד יומי" lines up with the Shabbat titles on the right, and its rows end
// level with the city blocks
#define STUDY_TITLE_Y   SHABBAT_TITLE_Y
#define STUDY_COUNT     3
#define STUDY_ROW_H     ((CITY_STRIP_Y + CITY_STRIP_H - STUDY_TITLE_Y - SECTION_TITLE_H) / STUDY_COUNT)
#define INFO_TEXT_PAD   6

static const char * const zman_names[ZMANIM_COUNT] = {
    "עלות השחר", "ציצית ותפילין", "סו\"ז ק\"ש מג\"א", "סו\"ז ק\"ש גר\"א",
    "סו\"ז ת' גר\"א", "חצות היום", "מנחה גדולה",
};
#define CITY_COUNT      3
static const char * const city_names[CITY_COUNT] = { "י-ם", "חיפה", "ב\"ש" };

static const char * const study_names[STUDY_COUNT] = {
    "דף יומי בבלי", "הלכה יומית", "משנה יומית",
};

// Week table (0 = Sunday ... 6 = Shabbat)
static lv_obj_t *lbl_heb_day_nums[WEEK_DAYS];
static lv_obj_t *lbl_greg_day_nums[WEEK_DAYS];
static lv_obj_t *lbl_sunset_times[WEEK_DAYS];
static lv_obj_t *lbl_sunrise_times[WEEK_DAYS];
static lv_obj_t *lbl_events[WEEK_DAYS];
static lv_obj_t *today_marks[WEEK_DAYS];   // yellow bar, shown on today's row only
static lv_obj_t *week_rows[WEEK_DAYS];

// Column titles for dynamic text updating
static lv_obj_t *lbl_header_hebrew_month;
static lv_obj_t *lbl_header_month;

// Shabbat
static lv_obj_t *lbl_candle_lighting_time;
static lv_obj_t *lbl_havdalah_time;
static lv_obj_t *lbl_candle_city_times[CITY_COUNT];
static lv_obj_t *lbl_havdalah_city_times[CITY_COUNT];

// Left panel values
static lv_obj_t *lbl_today_day;
static lv_obj_t *lbl_today_hebrew;
static lv_obj_t *lbl_today_greg;
static lv_obj_t *lbl_zman_values[ZMANIM_COUNT];
static lv_obj_t *lbl_study_values[STUDY_COUNT];

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

/**
 * Today's row is taller, 26 px and yellow; the other rows are 22 px.
 * The table height is the same for any day, so nothing below it moves.
 */
static void set_today(int today)
{
    int32_t y = ROW_START_Y;
    for (int i = 0; i < WEEK_DAYS; i++) {
        bool is_today = (i == today);
        int32_t h = is_today ? ROW_H_TODAY : ROW_H;
        lv_obj_set_pos(week_rows[i], WEEK_X, y);
        lv_obj_set_height(week_rows[i], h);
        lv_obj_set_style_text_font(week_rows[i], is_today ? &lv_font_heb_26_bold : &lv_font_heb_22_bold, 0);
        lv_obj_set_height(today_marks[i], h - 1);   // keep the row line visible
        lv_obj_set_hidden(today_marks[i], !is_today);
        y += h;
    }
}

void ui_calendar_update(void) {
    // Test data (0 = Sunday ... 6 = Shabbat)
    const char* test_greg_nums[]  = {"6", "7", "8", "9", "10", "11", "12"};
    const char* test_heb_nums[]   = {"כד", "כה", "כו", "כז", "כח", "כט", "א"};
    const char* test_sunrises[]   = {"6:59", "6:57", "6:56", "6:55", "6:53", "6:52", "6:51"};
    const char* test_sunsets[]    = {"6:24", "6:24", "6:25", "6:25", "6:26", "6:27", "6:27"};
    const char* test_events[]     = {"", "", "", "", "", "", "ראש השנה"};
    int test_today = 2;   // Tuesday
    const char* test_zmanim[]     = {"5:03-08", "5:10-15", "8:51-52", "9:29-30",
                                     "10:32", "12:40-38", "1:10-08"};
    // Today's portion only
    const char* test_study[]      = {"חולין: קל\"ז", "או\"ח: רס\"א, ג", "כלים: כ\"ט, ב"};

    for (int i = 0; i < WEEK_DAYS; i++) {
        lv_label_set_text(lbl_greg_day_nums[i], test_greg_nums[i]);
        lv_label_set_text(lbl_heb_day_nums[i], test_heb_nums[i]);
        lv_label_set_text(lbl_sunrise_times[i], test_sunrises[i]);
        lv_label_set_text(lbl_sunset_times[i], test_sunsets[i]);
        lv_label_set_text(lbl_events[i], test_events[i]);
    }
    set_today(test_today);

    for (int i = 0; i < ZMANIM_COUNT; i++) {
        lv_label_set_text(lbl_zman_values[i], test_zmanim[i]);
    }
    for (int i = 0; i < STUDY_COUNT; i++) {
        lv_label_set_text(lbl_study_values[i], test_study[i]);
    }

    lv_label_set_text(lbl_today_day, "יום שלישי");
    lv_label_set_text(lbl_today_hebrew, "כ\"ו אלול תשפ\"ו");
    lv_label_set_text(lbl_today_greg, "8 ספטמבר 2026");

    lv_label_set_text(lbl_header_hebrew_month, "תשרי");
    lv_label_set_text(lbl_header_month, "ספטמ");

    lv_label_set_text(lbl_candle_lighting_time, "6:30");
    lv_label_set_text(lbl_havdalah_time, "7:28");
    const char* test_candle_cities[]   = {"6:14", "6:21", "6:32"};
    const char* test_havdalah_cities[] = {"7:26", "7:28", "7:27"};
    for (int i = 0; i < CITY_COUNT; i++) {
        lv_label_set_text(lbl_candle_city_times[i], test_candle_cities[i]);
        lv_label_set_text(lbl_havdalah_city_times[i], test_havdalah_cities[i]);
    }
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
        lv_label_set_text(name, city_names[i]);
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
    const char* heb_days[WEEK_DAYS] = {"ראשון", "שני", "שלישי", "רביעי", "חמישי", "שישי", "שבת"};

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

        // Today: yellow from the day name to sunrise (not the event), above the bottom line
        today_marks[i] = make_box(row, SUNRISE_X, 0, WEEK_W - SUNRISE_X, ROW_H - 1);
        set_fill(today_marks[i], EINK_COLOR_YELLOW);
        lv_obj_set_hidden(today_marks[i], true);

        cell = make_box(row, EVENT_X, 0, EVENT_W, LV_PCT(100));
        lbl_events[i] = make_label(cell, "", NULL, LV_ALIGN_CENTER, 0, true);
        lv_obj_set_style_text_color(lbl_events[i], EINK_COLOR_RED, 0);
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
        lv_obj_t *day_name = make_label(cell, heb_days[i], NULL, LV_ALIGN_RIGHT_MID, -DAY_TAG_PAD, true);

        if (i == WEEK_DAYS - 1) {   // Shabbat: day name and dates in red
            lv_obj_set_style_text_color(day_name, EINK_COLOR_RED, 0);
            lv_obj_set_style_text_color(lbl_heb_day_nums[i], EINK_COLOR_RED, 0);
            lv_obj_set_style_text_color(lbl_greg_day_nums[i], EINK_COLOR_RED, 0);
        }
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
        int32_t y = ZMANIM_TITLE_Y + SECTION_TITLE_H + i * ZMAN_ROW_H;
        lv_obj_t *row = make_box(scr, INFO_X, y, INFO_W, ZMAN_ROW_H);
        set_bottom_line(row);
        lv_obj_set_style_text_font(row, &lv_font_heb_22_bold, 0);   // one size for the whole row
        make_label(row, zman_names[i], NULL, LV_ALIGN_RIGHT_MID, -INFO_TEXT_PAD, true);
        lbl_zman_values[i] = make_label(row, "--:--", NULL, LV_ALIGN_LEFT_MID, INFO_TEXT_PAD, false);
    }

    // Daily study: name on the right, text on the left
    make_section_title(scr, STUDY_TITLE_Y, "לימוד יומי");
    for (int i = 0; i < STUDY_COUNT; i++) {
        int32_t y = STUDY_TITLE_Y + SECTION_TITLE_H + i * STUDY_ROW_H;
        lv_obj_t *row = make_box(scr, INFO_X, y, INFO_W, STUDY_ROW_H);
        set_bottom_line(row);
        lv_obj_set_style_text_font(row, &lv_font_heb_22_bold, 0);   // one size for the whole row
        make_label(row, study_names[i], NULL, LV_ALIGN_RIGHT_MID, -INFO_TEXT_PAD, true);
        lbl_study_values[i] = make_label(row, "", NULL, LV_ALIGN_LEFT_MID, INFO_TEXT_PAD, true);
    }
}

/**
 * Top line on yellow: "day   Hebrew date   Gregorian date", centred, right to left.
 * Separate labels: numbers inside one RTL label can be reordered by LVGL.
 * Full month names fit here (the line spans the whole screen), e.g.
 * "יום חמישי  כ\"ט מרחשוון תשפ\"ז  28 אוקטובר 2026".
 */
static void create_today(lv_obj_t *scr)
{
    lv_obj_t *line = make_box(scr, UI_MARGIN, TODAY_Y, UI_HOR_RES - 2 * UI_MARGIN, TODAY_H);
    set_fill(line, EINK_COLOR_YELLOW);
    lv_obj_set_style_text_font(line, &lv_font_heb_26_bold, 0);
    lv_obj_set_style_base_dir(line, LV_BASE_DIR_RTL, 0);   // first item on the right
    lv_obj_set_flex_flow(line, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(line, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(line, 30, 0);   // space between the three parts

    lbl_today_day = lv_label_create(line);
    lbl_today_hebrew = lv_label_create(line);
    lbl_today_greg = lv_label_create(line);
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
    create_week(scr);
    create_info(scr);
}
