#include <stdio.h>
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <timers.h>
#include <semphr.h>

#include "lvgl.h"
#include "examples/lv_examples.h"
#include "demos/lv_demos.h"
#include "player.h"

using namespace std;

static lv_display_t* hal_init(int32_t w, int32_t h) {
    lv_group_set_default(lv_group_create());

    lv_display_t* disp = lv_sdl_window_create(w, h);

    lv_indev_t* mouse = lv_sdl_mouse_create();
    lv_indev_set_group(mouse, lv_group_get_default());
    lv_indev_set_display(mouse, disp);
    lv_display_set_default(disp);

    lv_indev_t* mousewheel = lv_sdl_mousewheel_create();
    lv_indev_set_display(mousewheel, disp);
    lv_indev_set_group(mousewheel, lv_group_get_default());

    lv_indev_t* kb = lv_sdl_keyboard_create();
    lv_indev_set_display(kb, disp);
    lv_indev_set_group(kb, lv_group_get_default());

    return disp;
}

static lv_obj_t* detail_page = NULL;
static lv_obj_t* new_page = NULL;

static void delete_detail_page(lv_anim_t* anim) {
    lv_obj_t* page = (lv_obj_t*)anim->var;
    lv_obj_delete(page);
    detail_page = NULL;
}

static void start_bounce_animation(lv_obj_t* page, lv_coord_t start_x) {
    lv_anim_t anim{};
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, page);
    lv_anim_set_exec_cb(&anim, (lv_anim_exec_xcb_t)lv_obj_set_x);
    lv_anim_set_values(&anim, start_x, 0);
    lv_anim_set_duration(&anim, 300);
    lv_anim_start(&anim);
}

static void start_exit_animation(lv_obj_t* page, lv_coord_t start_x) {
    lv_anim_t anim{};
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, page);
    lv_anim_set_exec_cb(&anim, (lv_anim_exec_xcb_t)lv_obj_set_x);
    lv_coord_t target = lv_display_get_horizontal_resolution(NULL);
    lv_anim_set_values(&anim, start_x, target);
    lv_anim_set_duration(&anim, 300);
    lv_anim_set_deleted_cb(&anim, delete_detail_page);
    lv_anim_start(&anim);
}

static void drag_event_handler(lv_event_t* e) {
    lv_obj_t* page = lv_event_get_target_obj(e);
    lv_indev_t* indev = lv_indev_get_act();
    if (!indev) {
        return;
    }
    lv_point_t vect{};
    lv_indev_get_vect(indev, &vect);
    lv_coord_t x = lv_obj_get_x(page) + vect.x;
    if (x < 0) {
        x = 0;
    }
    lv_obj_set_x(page, x);
    if (lv_indev_get_state(indev) == LV_INDEV_STATE_RELEASED) {
        lv_coord_t threshold = lv_display_get_horizontal_resolution(NULL) / 4;
        if (x > threshold) {
            start_exit_animation(page, x);
        } else {
            start_bounce_animation(page, x);
        }
    }
}

static void close_detail_cb(lv_event_t* e) {
    lv_obj_t* btn = lv_event_get_target_obj(e);
    lv_obj_t* page = lv_obj_get_parent(btn);
    lv_coord_t cur_x = lv_obj_get_x(page);
    start_exit_animation(page, cur_x);
}

static void open_new_page_cb(lv_event_t* e) {
    if (new_page) {
        lv_obj_delete(new_page);
        new_page = NULL;
    }
    lv_obj_t* screen = lv_screen_active();
    lv_obj_t* page = lv_obj_create(screen);
    lv_obj_set_size(page, LV_PCT(100), LV_PCT(100));
    lv_obj_set_pos(page, 0, 0);
#if 0
    lv_obj_set_style_bg_color(page, lv_color_white(), 0);
#else
    lv_obj_set_style_bg_color(page, lv_color_hex(0xE8EAF6), 0);
    lv_obj_set_style_bg_grad_color(page, lv_color_hex(0xBBDEFB), 0);
    lv_obj_set_style_bg_grad_dir(page, LV_GRAD_DIR_VER, 0);
#endif
    lv_obj_set_style_border_width(page, 0, 0);
    lv_obj_set_style_bg_opa(page, LV_OPA_100, 0);
    /* 边缘效果 */
    lv_obj_set_style_radius(page, 20, 0);
    lv_obj_set_style_shadow_width(page, 30, 0);
    lv_obj_set_style_shadow_spread(page, 10, 0);
    lv_obj_set_style_shadow_color(page, lv_color_hex(0x888888), 0);
    lv_obj_set_style_shadow_opa(page, LV_OPA_60, 0);
    /* 绑定拖动事件 */
    lv_obj_add_event_cb(page, drag_event_handler, LV_EVENT_PRESSING, NULL);
    lv_obj_add_event_cb(page, drag_event_handler, LV_EVENT_RELEASED, NULL);

    new_page = page;
}

static void open_detail_cb(lv_event_t* e) {
    lv_obj_t* btn = lv_event_get_target_obj(e);
    lv_obj_t* label = lv_obj_get_child(btn, 0);
    const char* text = lv_label_get_text(label);

    if (detail_page) {
        lv_obj_delete(detail_page);
        detail_page = NULL;
    }

    lv_obj_t* screen = lv_screen_active();
    lv_obj_t* page = lv_obj_create(screen);
    lv_obj_set_size(page, LV_PCT(100), LV_PCT(100));
    lv_obj_set_pos(page, 0, 0);
#if 0
    lv_obj_set_style_bg_color(page, lv_color_white(), 0);
#else
    lv_obj_set_style_bg_color(page, lv_color_hex(0xE8EAF6), 0);
    lv_obj_set_style_bg_grad_color(page, lv_color_hex(0xBBDEFB), 0);
    lv_obj_set_style_bg_grad_dir(page, LV_GRAD_DIR_VER, 0);
#endif
    lv_obj_set_style_border_width(page, 0, 0);
    lv_obj_set_style_bg_opa(page, LV_OPA_100, 0);
    /* 边缘效果 */
    lv_obj_set_style_radius(page, 20, 0);
    lv_obj_set_style_shadow_width(page, 30, 0);
    lv_obj_set_style_shadow_spread(page, 10, 0);
    lv_obj_set_style_shadow_color(page, lv_color_hex(0x888888), 0);
    lv_obj_set_style_shadow_opa(page, LV_OPA_60, 0);

    /* 添加标题 */
    lv_obj_t* title = lv_label_create(page);
    lv_label_set_text_fmt(title, "%s", text);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

    /* 添加内容 */
    lv_obj_t* content = lv_label_create(page);
    lv_label_set_text_fmt(content, "%s", text);
    lv_obj_set_width(content, lv_pct(80));
    lv_obj_set_style_text_align(content, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(content, LV_ALIGN_CENTER, 0, 0);

    /* 返回按钮 */
    lv_obj_t* close_btn = lv_button_create(page);
    lv_obj_set_size(close_btn, 120, 40);
    lv_obj_align(close_btn, LV_ALIGN_BOTTOM_MID, 0, -30);
    lv_obj_t* btn_label = lv_label_create(close_btn);
    lv_label_set_text(btn_label, "return");
    lv_obj_center(btn_label);
    lv_obj_add_event_cb(close_btn, close_detail_cb, LV_EVENT_CLICKED, NULL);

    /* 跳转按钮 */
    lv_obj_t* new_btn = lv_button_create(page);
    lv_obj_set_size(new_btn, 120, 40);
    lv_obj_align(new_btn, LV_ALIGN_BOTTOM_MID, 0, -90);
    lv_obj_t* new_label = lv_label_create(new_btn);
    lv_label_set_text(new_label, "start");
    lv_obj_center(new_label);
    lv_obj_add_event_cb(new_btn, open_new_page_cb, LV_EVENT_CLICKED, NULL);

    /* 绑定拖动事件 */
    lv_obj_add_event_cb(page, drag_event_handler, LV_EVENT_PRESSING, NULL);
    lv_obj_add_event_cb(page, drag_event_handler, LV_EVENT_RELEASED, NULL);
    detail_page = page;
}

void list_page(void) {
    lv_obj_t* screen = lv_screen_active();
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* main = lv_obj_create(screen);
    lv_obj_set_size(main, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(main, lv_color_hex(0xf0f0f0), 0);
    lv_obj_set_style_border_width(main, 0, 0);
    lv_obj_set_style_radius(main, 0, 0);
    /* 列表控件 */
    lv_obj_t* list = lv_list_create(screen);
    lv_obj_set_size(list, 300, 200);
    lv_obj_center(list);
    const char* items[] = { "Item 1", "Item 2", "Item 3", "Item 4", "Item 5" };
    for (int i = 0; i < 5; i++) {
        lv_obj_t* button = lv_list_add_button(list, NULL, items[i]);
        lv_obj_add_event_cb(button, open_detail_cb, LV_EVENT_CLICKED, NULL);
    }
}

static void playerTask(void* parameters) {
    lv_init();
    hal_init(600, 400);
    list_page();
    while (1) {
        uint32_t time_till_next = lv_timer_handler();
        lv_delay_ms(time_till_next);
    }
}

static void playerTaskInit(void) {
    static StaticTask_t playerTaskTCB;
    static StackType_t playerTaskStack[256];
    xTaskCreateStatic(playerTask, "player", 256, NULL, configMAX_PRIORITIES - 1U, playerTaskStack, &playerTaskTCB);
}

int main_player(void) {
    playerTaskInit();
    return 0;
}
