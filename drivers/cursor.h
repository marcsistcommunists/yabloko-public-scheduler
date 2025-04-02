#include "pit.h"
#include "vga.h"
volatile uint32_t timer_ticks = 0;
volatile uint32_t timer_ofset = 0;
volatile uint8_t cursor_char = '/';
void cursor_change() {
	if (timer_ticks < 300) {
		timer_ticks++;
	}
	else {
		vga_set_char(timer_ofset, cursor_char);
		timer_ticks = 0;
		if (cursor_char == '/') {
			cursor_char = '\\';
		}
		else {
			cursor_char = '/';
		}
	}
}
void init_cursor() {
	add_timer_callback(cursor_change);
	timer_ofset = vga_get_cursor();
	vga_set_char(timer_ofset, cursor_char);
}