#pragma once

void vga_clear_screen();

void vga_print_string(const char* s);

void vga_del_symbol();

void vga_set_cursor(unsigned offset);

unsigned vga_get_cursor();

void vga_set_char(unsigned offset, char c);