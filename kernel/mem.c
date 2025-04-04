// Заголовочные файлы для работы с типами данных, памятью, консолью и архитектурой x86
#include <stdint.h>
#include "mem.h"
#include "console.h"
#include "cpu/memlayout.h"
#include "cpu/x86.h"

// Структура элемента списка свободных страниц
struct run {
    struct run* next;  // Указатель на следующий элемент в списке
};

// Глобальная структура для управления свободной памятью
struct {
    struct run* freelist;  // Голова односвязного списка свободных страниц
} kmem;

// Инициализирует аллокатор, добавляя диапазон памяти [vstart, vend] в свободный список
void freerange(void* vstart, void* vend) {
    char* p;
    // Выравниваем начальный адрес вверх до границы страницы (4096 байт)
    p = (char*)PGROUNDUP((uintptr_t)vstart);

    // Добавляем каждую страницу в диапазоне в свободный список
    for (; p + PGSIZE <= (char*)vend; p += PGSIZE)
        kfree(p);  // Освобождаем страницу (добавляем в kmem.freelist)
}

void* memset(void* dst, unsigned c, uint64_t n) {
    // Если адрес и размер выровнены по 4 байтам, используем 32-битные операции
    if ((uintptr_t)dst % 4 == 0 && n % 4 == 0) {
        c &= 0xFF;  // Оставляем младший байт
        stosl(dst, (c << 24) | (c << 16) | (c << 8) | c, n / 4);  // Заполняем 32-битными блоками
    }
    else
        stosb(dst, c, n);  // Байтовая запись для невыровненных данных
    return dst;
}

// Освобождает физическую страницу памяти
void kfree(void* v) {
    struct run* r;

    // Проверки корректности адреса:
    if ((uintptr_t)v % PGSIZE || V2P(v) >= PHYSTOP) {        // Физический адрес должен быть в допустимом диапазоне
        printk("address fail\n");
        panic("kfree");            // Аварийное завершение при ошибке
    }
      // Заполняем страницу "мусором" (0x01), чтобы обнаружить использование после освобождения
    memset(v, 1, PGSIZE);

    // Добавляем страницу в начало списка свободных
    r = v;                   // Преобразуем указатель в структуру run
    r->next = kmem.freelist; // Новый элемент указывает на текущую голову списка
    kmem.freelist = r;       // Обновляем голову списка
}

// Выделяет одну страницу физической памяти (4096 байт)
void* kalloc(void) {
    struct run* r;

    // Извлекаем первую страницу из списка свободных
    r = kmem.freelist;
    if (r)
        kmem.freelist = r->next;  // Обновляем голову списка

    return (char*)r;  // Возвращаем адрес страницы (или NULL, если память закончилась)
}