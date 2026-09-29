/**
 * @file ui_calendar.c
 *
 */

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
    #include "lvgl.h"
#else
    #include "lvgl/lvgl.h"
#endif

#include "ui_calendar.h"

// Fonts compiled from ui/fonts/
LV_FONT_DECLARE(lv_font_heb_18_bold);
LV_FONT_DECLARE(lv_font_heb_22_bold); // day names, header line, Shabbat box titles
LV_FONT_DECLARE(lv_font_heb_26_bold); // dates in the day tags
LV_FONT_DECLARE(lv_font_heb_96); // Shabbat times (digits and ":" only)

// Define color constants for your 4-color E-Ink display panel
#define EINK_COLOR_WHITE  lv_color_make(0xFF, 0xFF, 0xFF)
#define EINK_COLOR_BLACK  lv_color_make(0x00, 0x00, 0x00)
#define EINK_COLOR_RED    lv_color_make(0xFF, 0x00, 0x00)
#define EINK_COLOR_YELLOW lv_color_make(0xFF, 0xCC, 0x00)
#define EINK_COLOR_GRAY   lv_color_make(0x80, 0x80, 0x80)

// Portrait layout (UI_HOR_RES x UI_VER_RES)
#define UI_MARGIN       20
#define UI_CONTENT_W    (UI_HOR_RES - 2 * UI_MARGIN)
#define HEADER_H        45
#define COL_TITLE_Y     55  // "זריחה" / "שקיעה" line under the header
#define COL_TITLE_H     30
#define ROW_START_Y     (COL_TITLE_Y + COL_TITLE_H)
#define ROW_H           55
#define DAY_TAG_W       100
#define DAY_TAG_PAD     4   // text distance from the tag's left/right edge
#define TIME_GAP        8   // sunrise/sunset distance from the day tags
#define TIME_BOX_H      155
#define TIME_BOX_GAP    10
#define TIME_BOX_W      ((UI_CONTENT_W - TIME_BOX_GAP) / 2)

// Global style structure for your calendar texts
static lv_style_t style_hebrew;
static lv_style_t style_clock_numbers;

// Weekday labels (0 = Sunday ... 6 = Saturday)
static lv_obj_t *lbl_greg_day_nums[7];
static lv_obj_t *lbl_sunrise_times[7];
static lv_obj_t *lbl_sunset_times[7];
static lv_obj_t *lbl_heb_day_nums[7];

// Header labels for dynamic text updating
static lv_obj_t *lbl_header_month;
static lv_obj_t *lbl_header_hebrew_year;

// Large time displays at the bottom
static lv_obj_t *lbl_candle_lighting_time;
static lv_obj_t *lbl_havdalah_time;

static void init_styles(void) {
    lv_style_init(&style_hebrew);
    // Header line + Shabbat box titles: Frank Ruhl Libre Bold 22px
    lv_style_set_text_font(&style_hebrew, &lv_font_heb_22_bold);

    // --- Initialize the clock style globally ---
    lv_style_init(&style_clock_numbers);
    lv_style_set_text_font(&style_clock_numbers, &lv_font_heb_96);
}

void ui_calendar_update(void) {
    // Complete 7-day data arrays (0 = Sunday ... 6 = Saturday)
    const char* test_greg_nums[]  = {"6", "7", "8", "9", "10", "11", "12"};
    const char* test_heb_nums[]   = {"כד", "כה", "כו", "כז", "כח", "כט", "א"};
    const char* test_sunrises[]   = {"6:59", "6:57", "6:56", "6:55", "6:53", "6:52", "6:51"};
    const char* test_sunsets[]    = {"6:24", "6:24", "6:25", "6:25", "6:26", "6:27", "6:27"};

    // 1. Loop through ALL 7 DAYS (0 to 6) smoothly
    for (int i = 0; i < 7; i++) {
        lv_label_set_text(lbl_greg_day_nums[i], test_greg_nums[i]);
        lv_label_set_text(lbl_heb_day_nums[i], test_heb_nums[i]);
        lv_label_set_text(lbl_sunrise_times[i], test_sunrises[i]);
        lv_label_set_text(lbl_sunset_times[i], test_sunsets[i]);
    }

    // 2. Inject the Large Clock Highlights at the bottom
    lv_label_set_text(lbl_candle_lighting_time, "6:30");
    lv_label_set_text(lbl_havdalah_time, "7:28");
}

void ui_calendar_create(void) {
    lv_display_t * disp = lv_display_get_default();
    if(disp == NULL) return;

    lv_obj_t *scr = lv_display_get_screen_active(disp);
    lv_obj_set_style_bg_color(scr, EINK_COLOR_WHITE, 0);
    lv_obj_set_size(scr, UI_HOR_RES, UI_VER_RES);
    lv_obj_set_style_pad_all(scr, 0, 0);
    // Every label without its own font (e.g. English day names, times) inherits this
    lv_obj_set_style_text_font(scr, &lv_font_heb_18_bold, 0);

    // Ensure style properties are loaded
    init_styles();

    // ==========================================
    // HEADER BLOCK (Top Area)
    // ==========================================
    lv_obj_t *header = lv_obj_create(scr);
    lv_obj_set_size(header, UI_HOR_RES, HEADER_H);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 5);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_pad_all(header, 0, 0);
    lv_obj_set_style_bg_opa(header, LV_OPA_TRANSP, 0);

    // חודש לועזי (צד שמאל)
    lbl_header_month = lv_label_create(header);
    lv_obj_add_style(lbl_header_month, &style_hebrew, 0);
    lv_obj_set_style_base_dir(lbl_header_month, LV_BASE_DIR_RTL, 0);
    lv_obj_align(lbl_header_month, LV_ALIGN_LEFT_MID, 20, 0);
    lv_label_set_text(lbl_header_month, "ספטמבר 2026");

    // כיתוב מרכזי: משפחת רוקח (במרכז המדויק)
    lv_obj_t *lbl_header_family = lv_label_create(header);
    lv_obj_add_style(lbl_header_family, &style_hebrew, 0);
    lv_obj_set_style_base_dir(lbl_header_family, LV_BASE_DIR_RTL, 0);
    // מרכז מוחלט בתוך ה-Header, באותו הגובה של השאר
    lv_obj_align(lbl_header_family, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(lbl_header_family, "משפחת רוקח");

    // חודש עברי (צד ימין)
    lbl_header_hebrew_year = lv_label_create(header);
    lv_obj_add_style(lbl_header_hebrew_year, &style_hebrew, 0);
    lv_obj_set_style_base_dir(lbl_header_hebrew_year, LV_BASE_DIR_RTL, 0);
    lv_obj_align(lbl_header_hebrew_year, LV_ALIGN_RIGHT_MID, -20, 0);
    lv_label_set_text(lbl_header_hebrew_year, "חודש-עברי");

    // ==========================================
    // COLUMN TITLES (above the sunrise / sunset columns)
    // ==========================================
    lv_obj_t *col_titles = lv_obj_create(scr);
    lv_obj_set_size(col_titles, UI_CONTENT_W, COL_TITLE_H);
    lv_obj_align(col_titles, LV_ALIGN_TOP_MID, 0, COL_TITLE_Y);
    lv_obj_set_style_border_width(col_titles, 0, 0);
    lv_obj_set_style_pad_all(col_titles, 0, 0);
    lv_obj_set_style_bg_opa(col_titles, LV_OPA_TRANSP, 0);

    // זריחה - above the sunrise column (left)
    lv_obj_t *lbl_sunrise_title = lv_label_create(col_titles);
    lv_obj_set_style_base_dir(lbl_sunrise_title, LV_BASE_DIR_RTL, 0);
    lv_label_set_text(lbl_sunrise_title, "זריחה");
    lv_obj_align(lbl_sunrise_title, LV_ALIGN_LEFT_MID, DAY_TAG_W + TIME_GAP, 0);

    // שקיעה - above the sunset column (right)
    lv_obj_t *lbl_sunset_title = lv_label_create(col_titles);
    lv_obj_set_style_base_dir(lbl_sunset_title, LV_BASE_DIR_RTL, 0);
    lv_label_set_text(lbl_sunset_title, "שקיעה");
    lv_obj_align(lbl_sunset_title, LV_ALIGN_RIGHT_MID, -(DAY_TAG_W + TIME_GAP), 0);

    // ==========================================
    // WEEKLY GRID ROWS (Sunday - Saturday Unified)
    // ==========================================
    const char* greg_days[] = {"Sun", "Mon", "Tues", "Wed", "Thu", "Fri", "Sat"};
    const char* heb_days[] = {"ראשון", "שני", "שלישי", "רביעי", "חמישי", "שישי", "שבת"};

    int row_start_y = ROW_START_Y;
    int row_height = ROW_H;

    // Loop through all 7 days seamlessly
    for (int i = 0; i < 7; i++) {
        lv_obj_t *row = lv_obj_create(scr);
        lv_obj_set_size(row, UI_CONTENT_W, row_height);
        lv_obj_align(row, LV_ALIGN_TOP_MID, 0, row_start_y + (i * row_height));
        lv_obj_set_style_border_side(row, LV_BORDER_SIDE_BOTTOM, 0);
        lv_obj_set_style_border_width(row, 1, 0);
        lv_obj_set_style_border_color(row, EINK_COLOR_BLACK, 0);
        lv_obj_set_style_pad_all(row, 0, 0);
        lv_obj_set_style_radius(row, 0, 0);

        // Left Panel (Gregorian tag)
        lv_obj_t *bg_greg = lv_obj_create(row);
        lv_obj_set_size(bg_greg, DAY_TAG_W, row_height - 2);
        lv_obj_align(bg_greg, LV_ALIGN_LEFT_MID, 0, 0);
        if (i == 6)
        {
         lv_obj_set_style_bg_color(bg_greg, EINK_COLOR_RED, 0);
         lv_obj_set_style_text_color(bg_greg, EINK_COLOR_WHITE, 0);
        }
        else
         lv_obj_set_style_bg_color(bg_greg, EINK_COLOR_YELLOW, 0);
        lv_obj_set_style_border_width(bg_greg, 0, 0);
        lv_obj_set_style_radius(bg_greg, 0, 0);
        lv_obj_set_style_pad_hor(bg_greg, 0, 0); // no theme padding: text sits at the edges
        lv_obj_set_style_text_font(bg_greg, &lv_font_heb_22_bold, 0);

        lv_obj_t *lbl_greg_name = lv_label_create(bg_greg);
        lv_label_set_text(lbl_greg_name, greg_days[i]);
        lv_obj_align(lbl_greg_name, LV_ALIGN_LEFT_MID, DAY_TAG_PAD, 0);

        lbl_greg_day_nums[i] = lv_label_create(bg_greg);
        lv_obj_set_style_text_font(lbl_greg_day_nums[i], &lv_font_heb_26_bold, 0);
        lv_label_set_text(lbl_greg_day_nums[i], "0");
        lv_obj_align(lbl_greg_day_nums[i], LV_ALIGN_RIGHT_MID, -DAY_TAG_PAD, 0);

        // Core Times (Sunrise / Sunset columns)
        lbl_sunrise_times[i] = lv_label_create(row);
        lv_label_set_text(lbl_sunrise_times[i], "--:--");
        lv_obj_align(lbl_sunrise_times[i], LV_ALIGN_LEFT_MID, DAY_TAG_W + TIME_GAP, 0);

        lbl_sunset_times[i] = lv_label_create(row);
        lv_label_set_text(lbl_sunset_times[i], "--:--");
        lv_obj_align(lbl_sunset_times[i], LV_ALIGN_RIGHT_MID, -(DAY_TAG_W + TIME_GAP), 0);

        // Right Panel (Hebrew tag)
        lv_obj_t *bg_heb = lv_obj_create(row);
        lv_obj_set_size(bg_heb, DAY_TAG_W, row_height - 2);
        lv_obj_align(bg_heb, LV_ALIGN_RIGHT_MID, 0, 0);
        if (i == 6)
        {
         lv_obj_set_style_bg_color(bg_heb, EINK_COLOR_RED, 0);
         lv_obj_set_style_text_color(bg_heb, EINK_COLOR_WHITE, 0);
        }
        else
         lv_obj_set_style_bg_color(bg_heb, EINK_COLOR_YELLOW, 0);
        lv_obj_set_style_border_width(bg_heb, 0, 0);
        lv_obj_set_style_radius(bg_heb, 0, 0);
        lv_obj_set_style_pad_hor(bg_heb, 0, 0);
        lv_obj_set_style_text_font(bg_heb, &lv_font_heb_22_bold, 0);

        lv_obj_t *lbl_heb_name = lv_label_create(bg_heb);
        lv_obj_set_style_base_dir(lbl_heb_name, LV_BASE_DIR_RTL, 0);
        lv_label_set_text(lbl_heb_name, heb_days[i]);
        lv_obj_align(lbl_heb_name, LV_ALIGN_RIGHT_MID, -DAY_TAG_PAD, 0);

        lbl_heb_day_nums[i] = lv_label_create(bg_heb);
        lv_obj_set_style_text_font(lbl_heb_day_nums[i], &lv_font_heb_26_bold, 0);
        lv_label_set_text(lbl_heb_day_nums[i], "--");
        lv_obj_align(lbl_heb_day_nums[i], LV_ALIGN_LEFT_MID, DAY_TAG_PAD, 0);
    }

    // ==========================================
    // LARGE BOTTOM CLOCK SECTION (side by side: lighting right, havdalah left; area below left empty)
    // ==========================================
    int clock_section_y = row_start_y + (7 * row_height) + 15;

    lv_obj_t *clock_panel = lv_obj_create(scr);
    lv_obj_set_size(clock_panel, UI_CONTENT_W, TIME_BOX_H);
    lv_obj_align(clock_panel, LV_ALIGN_TOP_MID, 0, clock_section_y);
    lv_obj_set_style_bg_color(clock_panel, EINK_COLOR_WHITE, 0);
    lv_obj_set_style_border_width(clock_panel, 0, 0);
    lv_obj_set_style_pad_all(clock_panel, 0, 0);

    // Main Outer Box (Set background to WHITE)
    lv_obj_t *box_lighting = lv_obj_create(clock_panel);
    lv_obj_set_size(box_lighting, TIME_BOX_W, TIME_BOX_H);
    lv_obj_align(box_lighting, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_set_style_bg_color(box_lighting, EINK_COLOR_WHITE, 0);
    lv_obj_set_style_bg_opa(box_lighting, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(box_lighting, 0, 0); // Remove padding so header fills top width

    // Top Header Container for "כניסת השבת" (Gray background)
    lv_obj_t *header_container = lv_obj_create(box_lighting);
    lv_obj_set_size(header_container, LV_PCT(100), 40); // Full width, 40px height
    lv_obj_align(header_container, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(header_container, EINK_COLOR_BLACK, 0);
    lv_obj_set_style_bg_opa(header_container, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(header_container, 0, 0);
    lv_obj_set_style_radius(header_container, 0, 0); // Square top corners (or match outer box radius)

    // Header Text
    lv_obj_t *lbl_light_header = lv_label_create(header_container);
    lv_obj_add_style(lbl_light_header, &style_hebrew, 0);
    lv_obj_set_style_text_color(lbl_light_header, lv_color_white(), 0);
    lv_obj_set_style_base_dir(lbl_light_header, LV_BASE_DIR_RTL, 0);
    lv_label_set_text(lbl_light_header, "כניסת השבת");
    lv_obj_align(lbl_light_header, LV_ALIGN_CENTER, 0, 0);

    // Time Label (WHITE background section)
    lbl_candle_lighting_time = lv_label_create(box_lighting);
    lv_obj_add_style(lbl_candle_lighting_time, &style_clock_numbers, 0);
    lv_label_set_text(lbl_candle_lighting_time, "00:00");
    lv_obj_align(lbl_candle_lighting_time, LV_ALIGN_BOTTOM_MID, 0, -15);

    // Havdalah (Left Box - Set background to WHITE)
    lv_obj_t *box_havdalah = lv_obj_create(clock_panel);
    lv_obj_set_size(box_havdalah, TIME_BOX_W, TIME_BOX_H);
    lv_obj_align(box_havdalah, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_bg_color(box_havdalah, EINK_COLOR_WHITE, 0);
    lv_obj_set_style_bg_opa(box_havdalah, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(box_havdalah, 0, 0); // Remove padding so header fills top width

    // Top Header Container for "יציאת השבת" (Gray background)
    lv_obj_t *header_container_hav = lv_obj_create(box_havdalah);
    lv_obj_set_size(header_container_hav, LV_PCT(100), 40); // Full width, 40px height
    lv_obj_align(header_container_hav, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(header_container_hav, EINK_COLOR_BLACK, 0);
    lv_obj_set_style_bg_opa(header_container_hav, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(header_container_hav, 0, 0);
    lv_obj_set_style_radius(header_container_hav, 0, 0);

    // Header Text
    lv_obj_t *lbl_hav_header = lv_label_create(header_container_hav);
    lv_obj_add_style(lbl_hav_header, &style_hebrew, 0);
    lv_obj_set_style_text_color(lbl_hav_header, lv_color_white(), 0);
    lv_obj_set_style_base_dir(lbl_hav_header, LV_BASE_DIR_RTL, 0);
    lv_label_set_text(lbl_hav_header, "יציאת השבת");
    lv_obj_align(lbl_hav_header, LV_ALIGN_CENTER, 0, 0);

    // Time Label (WHITE background section)
    lbl_havdalah_time = lv_label_create(box_havdalah);
    lv_obj_add_style(lbl_havdalah_time, &style_clock_numbers, 0);
    lv_label_set_text(lbl_havdalah_time, "00:00");
    lv_obj_align(lbl_havdalah_time, LV_ALIGN_BOTTOM_MID, 0, -15);
}
