// Подключение заголовочных файлов для работы с памятью, макросами процессора и консольным выводом
#include "mem.h"
#include "cpu/memlayout.h"
#include "cpu/x86.h"
#include "console.h"

// Глобальная переменная: корневая страничная директория (Page Directory) ядра
pde_t* kvm;

// Создает и настраивает страничную директорию ядра
pde_t* setupkvm(void) {
    // Выделяем страницу памяти под Page Directory
    pde_t* kvm = kalloc();
    // Обнуляем выделенную память
    memset(kvm, 0, PGSIZE);

    // Маппинг физической памяти в виртуальное адресное пространство ядра
    // KERNBASE..KERNBASE+PHYSTOP с использованием 4MB страниц (PTE_PS)
    for (uintptr_t pa = 0; pa < PHYSTOP; pa += 4 << 20) {
        // Вычисляем виртуальный адрес для текущего физического
        uintptr_t va = KERNBASE + pa;
        // Заполняем PDE: физический адрес + флаги (Present, Writable, Page Size)
        kvm[PDX(va)] = pa | PTE_P | PTE_W | PTE_PS;
    }
    return kvm;
}

// Инициализирует и активирует страничную таблицу ядра
void kvmalloc() {
    // Создаем страничную директорию
    kvm = setupkvm();
    // Переключаемся на новую таблицу
    switchkvm();
}

// Переключает CR3 на физический адрес страничной директории ядра
void switchkvm(void) {
    // Загружаем физический адрес таблицы в регистр CR3
    lcr3(V2P(kvm));
}

// Возвращает PTE для виртуального адреса va. Если страничной таблицы нет и alloc=1 — создает ее
static pte_t* walkpgdir(pde_t* pgdir, const void* va, int alloc) {
    pde_t* pde;
    pte_t* pgtab;

    // Получаем PDE из Page Directory по индексу PDX(va)
    pde = &pgdir[PDX(va)];
    // Если страничная таблица существует (флаг PTE_P)
    if (*pde & PTE_P) {
        // Преобразуем физический адрес таблицы в виртуальный
        pgtab = (pte_t*)P2V(PTE_ADDR(*pde));
    }
    else {
        // Если создание запрещено или не удалось выделить память — возвращаем 0
        if (!alloc || (pgtab = (pte_t*)kalloc()) == 0)
            return 0;
        // Обнуляем новую страничную таблицу
        memset(pgtab, 0, PGSIZE);
        // Записываем в PDE физический адрес таблицы + флаги (Present, Writable, User)
        *pde = V2P(pgtab) | PTE_P | PTE_W | PTE_U;
    }
    // Возвращаем PTE по индексу PTX(va)
    return &pgtab[PTX(va)];
}

// Создает отображение виртуального диапазона [va, va+size) на физический [pa, pa+size)
static int mappages(pde_t* pgdir, void* va, uintptr_t size, uintptr_t pa, int perm) {
    char* a, * last;
    pte_t* pte;

    // Выравниваем виртуальный адрес вниз до границы страницы
    a = (char*)PGROUNDDOWN((uintptr_t)va);
    // Вычисляем конец диапазона (последняя страница)
    last = (char*)PGROUNDDOWN(((uintptr_t)va) + size - 1);
    for (;;) {
        // Получаем PTE для текущего адреса
        if ((pte = walkpgdir(pgdir, a, 1)) == 0)
            return -1;
        // Если страница уже занята — паника
        if (*pte & PTE_P)
            panic("remap");
        // Записываем физический адрес и флаги в PTE
        *pte = pa | perm | PTE_P;
        // Выходим, если достигли конца диапазона
        if (a == last)
            break;
        // Переходим к следующей странице
        a += PGSIZE;
        pa += PGSIZE;
    }
    return 0;
}

// Выделяет виртуальную память в диапазоне [base, top)
int allocuvm(pde_t* pgdir, uintptr_t base, uintptr_t top) {
    // Проходим по всем страницам в диапазоне
    for (uintptr_t a = PGROUNDUP(base); a < top; a += PGSIZE) {
        // Выделяем физическую страницу
        char* pa = kalloc();
        if (pa == 0) {
            return -1;
        }
        // Обнуляем страницу
        memset(pa, 0, PGSIZE);
        // Маппим виртуальный адрес на физический с флагами (Writable, User)
        if (mappages(pgdir, (void*)a, PGSIZE, V2P(pa), PTE_W | PTE_U) < 0) {
            kfree(pa);
            return -1;
        }
    }
    return 0;
}

// Освобождает страничную таблицу и все связанные страницы
static void freept(pte_t* pt) {
    // Для каждой записи в таблице
    for (int i = 0; i < NPTENTRIES; i++) {
        // Если страница существует — освобождаем её
        if (pt[i] & PTE_P) {
            kfree((char*)P2V(PTE_ADDR(pt[i])));
        }
    }
    // Освобождаем саму таблицу
    kfree((char*)pt);
}

// Освобождает всю память, связанную с Page Directory
void freevm(pde_t* pgdir) {
    // Проходим по всем PDE (только первые половины записей)
    for (int i = 0; i < NPDENTRIES / 2; i++) {
        // Если таблица существует — освобождаем её
        if (pgdir[i] & PTE_P) {
            freept((pte_t*)P2V(PTE_ADDR(pgdir[i])));
        }
    }
    // Освобождаем саму директорию
    kfree(pgdir);
}