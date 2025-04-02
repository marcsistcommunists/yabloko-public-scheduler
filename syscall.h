#pragma once

enum {
	T_SYSCALL = 0x84,
	SYS_exit = 0,
	SYS_greet = 1,
	SYS_putc = 2,
	SYS_puts = 3,
	SYS_msleep = 4,
};

int syscall(int call, int arg);
