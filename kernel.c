#include "console.h"
#include "cpu/isr.h"
#include "cpu/gdt.h"
#include "cpu/memlayout.h"
#include "drivers/keyboard.h"
#include "drivers/vga.h"
#include "drivers/ata.h"
#include "drivers/misc.h"
#include "drivers/pit.h"
#include "drivers/uart.h"
#include "drivers/cursor.h"
#include "fs/fs.h"
#include "lib/string.h"
#include "proc.h"
#include "kernel/mem.h"
#include "drivers/sheduler.c"


void kmain() {
    freerange(P2V(1u<<20), P2V(2u<<20)); // 1MB - 2MB
    kvmalloc();  // map all of physical memory at KERNBASE
    freerange(P2V(2u<<20), P2V(PHYSTOP));

    load_gdt();
    init_keyboard();
    init_pit();
    uartinit();
    load_idt();
    sti();

    vga_clear_screen();
    printk("YABLOKO\n");
    init_cursor();
    init_sheduler();
    printk("\n> ");
    while (1) {
        if (kbd_buf_size > 0 && kbd_buf[kbd_buf_size-1] == '\n') {
            if (!strncmp("halt\n", kbd_buf, kbd_buf_size)) {
                qemu_shutdown();
            } else if (!strncmp("work\n", kbd_buf, kbd_buf_size)) {
                for (int i = 0; i < 5; ++i) {
                    msleep(1000);
                    printk(".");
                }
            }
            else if (!strncmp("run ", kbd_buf, 4)) {
                kbd_buf[kbd_buf_size - 1] = '\0';
                char* cmd = kbd_buf + 4;

                // Ручной поиск '&' в конце строки
                int len = 0;
                while (cmd[len] != '\0') len++;
                int is_background = 0;

                if (len > 0 && cmd[len - 1] == '&') {
                    is_background = 1;
                    cmd[len - 1] = '\0'; // Удаляем '&'
                    len--;
                }

                if (len == 0) {
                    printk("empty command\n");
                }
                else {
                    if (is_background == 0)
                    {
                        run_ellf(cmd);
                    }
                    else {
                        run_elf(cmd);
                    }
                }
            }
            else if (!strncmp("clear\n", kbd_buf, kbd_buf_size)) {
                vga_clear_screen();
            } else {
                printk("unknown command, try: halt | run CMD");
            }
            kbd_buf_size = 0;
            printk("\n> ");
        }
        asm("hlt");
    }
}
