#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <locale.h>
#include <windows.h>

#define CAFE_LEN 15
#define ADDR_LEN 50
#define ADDR_COL_WIDTH 28
#define PAGE_SIZE 10
#define TOP_N 5
#define MENU_ITEMS 11
#define DEFAULT_SAVE_FILE "orders.txt"

typedef struct {
    int num;
    char name_cafe[CAFE_LEN];
    char address[ADDR_LEN];
    int order_amount;
    int deliver_amount;
    int time_cook;
    int time_deliver;
    int tips;
} Order;

typedef struct Node {
    Order inf;
    struct Node *next;
    struct Node *prev;
} Node;

Node *list_find(Node *beg, int key);
Node *file_load(Node *old, const char *filename, int from_binary, int interactive);

/* Настройка кодировки консоли */
void setup_console(void) {
#ifdef LEGACY_CP1251
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);
    setlocale(LC_ALL, ".1251");
#else
    SetConsoleCP(65001);
    SetConsoleOutputCP(65001);
    setlocale(LC_ALL, ".UTF-8");
#endif
}

/* Пауза перед возвратом в главное меню */
void ui_pause_menu(void) {
    printf("\n  Нажмите любую клавишу для возврата в главное меню...\n");
    _getch();
}

/* Сообщение об ошибке и пауза */
void ui_show_error(const char *msg) {
    printf("\n  Ошибка: %s\n", msg);
    ui_pause_menu();
}

/* Ввод целого числа, Esc — отмена */
int read_int_esc(const char *prompt, int *value) {
    char buf[32];
    int i = 0;
    int ch;

    printf("  %s", prompt);
    while (1) {
        ch = _getch();
        if (ch == 27) {
            printf("\n");
            return -1;
        }
        if (ch == '\r') {
            buf[i] = '\0';
            if (i == 0) {
                continue;
            }
            if (sscanf(buf, "%d", value) == 1) {
                printf("\n");
                return 0;
            }
            return -2;
        }
        if (ch == '\b' || ch == 127) {
            if (i > 0) {
                i--;
                buf[i] = '\0';
                printf("\b \b");
            }
            continue;
        }
        if (ch >= '0' && ch <= '9' && i < 31) {
            buf[i++] = (char)ch;
            buf[i] = '\0';
            putchar(ch);
        }
    }
}

/* Ввод строки, Esc — отмена */
int read_str_esc(const char *prompt, char *buf, int size) {
    int i = 0;
    int ch;

    printf("  %s", prompt);
    buf[0] = '\0';
    while (1) {
        ch = _getch();
        if (ch == 27) {
            printf("\n");
            return -1;
        }
        if (ch == '\r') {
            buf[i] = '\0';
            printf("\n");
            return 0;
        }
        if (ch == '\b' || ch == 127) {
            if (i > 0) {
                i--;
                buf[i] = '\0';
                printf("\b \b");
            }
            continue;
        }
        if (i < size - 1 && ch >= 32 && ch != 127) {
            buf[i++] = (char)ch;
            buf[i] = '\0';
            putchar(ch);
        }
    }
}

/* Экран ввода с заголовком раздела */
void print_input_screen(const char *title) {
    system("cls");
    printf("\n  Esc - возврат в главное меню\n\n");
    printf("   ================  %s  ================\n\n", title);
}

/* Ввод одной записи заказа с проверкой полей */
int read_order_record(Node *beg, Order *x, int check_unique_num, int retry_on_error) {
    int rc;
    int fail;

    while (1) {
        rc = read_int_esc("1. Номер заказа: ", &x->num);
        if (rc == -1) {
            return 0;
        }
        if (rc == 0 && x->num > 0) {
            if (!check_unique_num || list_find(beg, x->num) == NULL) {
                break;
            }
            printf("\n  Ошибка: заказ с таким номером уже существует\n");
            if (!retry_on_error) {
                ui_pause_menu();
                return 0;
            }
        } else {
            printf("\n  Ошибка: номер заказа должен быть положительным целым\n");
            if (!retry_on_error) {
                ui_pause_menu();
                return 0;
            }
        }
    }

    while (1) {
        rc = read_str_esc("2. Ресторан (без пробелов): ", x->name_cafe, CAFE_LEN);
        if (rc == -1) {
            return 0;
        }
        if (x->name_cafe[0] != '\0' && strchr(x->name_cafe, ' ') == NULL) {
            break;
        }
        printf("\n  Ошибка: название ресторана не должно быть пустым и без пробелов\n");
        fail = retry_on_error ? 0 : -1;
        if (fail < 0) {
            return 0;
        }
    }

    while (1) {
        rc = read_str_esc("3. Адрес доставки (без пробелов): ", x->address, ADDR_LEN);
        if (rc == -1) {
            return 0;
        }
        if (x->address[0] != '\0' && strchr(x->address, ' ') == NULL) {
            break;
        }
        printf("\n  Ошибка: адрес не должен быть пустым и без пробелов\n");
        if (!retry_on_error) {
            ui_pause_menu();
            return 0;
        }
    }

    while (1) {
        rc = read_int_esc("4. Сумма заказа: ", &x->order_amount);
        if (rc == -1) {
            return 0;
        }
        if (rc == 0 && x->order_amount > 0) {
            break;
        }
        printf("\n  Ошибка: сумма заказа должна быть больше 0\n");
        if (!retry_on_error) {
            ui_pause_menu();
            return 0;
        }
    }

    while (1) {
        rc = read_int_esc("5. Стоимость доставки: ", &x->deliver_amount);
        if (rc == -1) {
            return 0;
        }
        if (rc == 0 && x->deliver_amount >= 0) {
            break;
        }
        printf("\n  Ошибка: стоимость доставки не может быть отрицательной\n");
        if (!retry_on_error) {
            ui_pause_menu();
            return 0;
        }
    }

    while (1) {
        rc = read_int_esc("6. Время приготовления (мин): ", &x->time_cook);
        if (rc == -1) {
            return 0;
        }
        if (rc == 0 && x->time_cook >= 0) {
            break;
        }
        printf("\n  Ошибка: время приготовления не может быть отрицательным\n");
        if (!retry_on_error) {
            ui_pause_menu();
            return 0;
        }
    }

    while (1) {
        rc = read_int_esc("7. Время в пути курьера (мин): ", &x->time_deliver);
        if (rc == -1) {
            return 0;
        }
        if (rc == 0 && x->time_deliver >= 0) {
            break;
        }
        printf("\n  Ошибка: время в пути не может быть отрицательным\n");
        if (!retry_on_error) {
            ui_pause_menu();
            return 0;
        }
    }

    while (1) {
        rc = read_int_esc("8. Чаевые курьеру: ", &x->tips);
        if (rc == -1) {
            return 0;
        }
        if (rc == 0 && x->tips >= 0) {
            break;
        }
        printf("\n  Ошибка: чаевые не могут быть отрицательными\n");
        if (!retry_on_error) {
            ui_pause_menu();
            return 0;
        }
    }

    return 1;
}

/* Подсчет числа записей в списке */
int list_count(Node *beg) {
    int n = 0;
    while (beg != NULL) {
        n++;
        beg = beg->next;
    }
    return n;
}

/* Добавление записи в конец списка */
Node *list_add_last(Node *beg, Order x) {
    Node *n = (Node *)malloc(sizeof(Node));
    Node *p;

    if (n == NULL) {
        printf("\n  Ошибка выделения памяти\n");
        return beg;
    }
    n->inf = x;
    n->next = NULL;
    n->prev = NULL;
    if (beg == NULL) {
        return n;
    }
    p = beg;
    while (p->next != NULL) {
        p = p->next;
    }
    p->next = n;
    n->prev = p;
    return beg;
}

/* Освобождение всего списка */
void list_free(Node *beg) {
    while (beg != NULL) {
        Node *t = beg;
        beg = beg->next;
        free(t);
    }
}

/* Поиск узла по номеру заказа */
Node *list_find(Node *beg, int key) {
    while (beg != NULL) {
        if (beg->inf.num == key) {
            return beg;
        }
        beg = beg->next;
    }
    return NULL;
}

/* Удаление записи по номеру заказа */
Node *list_delete(Node *beg, int key) {
    Node *p;

    if (beg == NULL) {
        printf("  Список пуст\n");
        return beg;
    }
    p = beg;
    while (p != NULL && p->inf.num != key) {
        p = p->next;
    }
    if (p == NULL) {
        printf("  Запись с номером %d не найдена\n", key);
        return beg;
    }
    if (p->prev == NULL) {
        beg = p->next;
        if (beg != NULL) {
            beg->prev = NULL;
        }
    } else {
        p->prev->next = p->next;
    }
    if (p->next != NULL) {
        p->next->prev = p->prev;
    }
    free(p);
    printf("\n  Запись удалена\n");
    return beg;
}

/* Сортировка списка пузырьком по номеру заказа */
Node *list_sort(Node *beg, int ascending) {
    int swapped;
    int cmp;
    Node *p;
    Order tmp;

    if (beg == NULL || beg->next == NULL) {
        return beg;
    }
    do {
        swapped = 0;
        p = beg;
        while (p->next != NULL) {
            cmp = 0;
            if (p->inf.num != p->next->inf.num) {
                if (ascending) {
                    cmp = (p->inf.num > p->next->inf.num) ? 1 : -1;
                } else {
                    cmp = (p->inf.num < p->next->inf.num) ? 1 : -1;
                }
            }
            if (cmp > 0) {
                tmp = p->inf;
                p->inf = p->next->inf;
                p->next->inf = tmp;
                swapped = 1;
            }
            p = p->next;
        }
    } while (swapped);
    printf("\n  Сортировка выполнена\n");
    return beg;
}

/* Сохранение в текстовый или бинарный файл (bin_flag: 0 — текст, 1 — бинарный) */
int file_save(Node *beg, const char *filename, int bin_flag) {
    FILE *f;
    Node *p;
    int n;

    if (bin_flag) {
        f = fopen(filename, "wb");
        if (f == NULL) {
            return 0;
        }
        n = list_count(beg);
        if (fwrite(&n, sizeof(int), 1, f) != 1) {
            fclose(f);
            return 0;
        }
        p = beg;
        while (p != NULL) {
            if (fwrite(&p->inf, sizeof(Order), 1, f) != 1) {
                fclose(f);
                return 0;
            }
            p = p->next;
        }
        fclose(f);
        return 1;
    }

    f = fopen(filename, "wt");
    if (f == NULL) {
        return 0;
    }
    p = beg;
    while (p != NULL) {
        fprintf(f, "%d;%s;%s;%d;%d;%d;%d;%d\n",
                p->inf.num, p->inf.name_cafe, p->inf.address,
                p->inf.order_amount, p->inf.deliver_amount,
                p->inf.time_cook, p->inf.time_deliver, p->inf.tips);
        p = p->next;
    }
    fclose(f);
    return 1;
}

/* Загрузка из текстового или бинарного файла, создается новый список */
Node *file_load(Node *old, const char *filename, int from_binary, int interactive) {
    FILE *f;
    Node *beg;
    Order x;
    int count;
    int i;

    if (from_binary) {
        f = fopen(filename, "rb");
        if (f == NULL) {
            if (interactive) {
                ui_show_error("не удалось открыть бинарный файл");
            }
            return old;
        }
        if (fread(&count, sizeof(int), 1, f) != 1 || count < 0) {
            fclose(f);
            if (interactive) {
                ui_show_error("ошибка чтения бинарного файла");
            }
            return old;
        }
        beg = NULL;
        for (i = 0; i < count; i++) {
            if (fread(&x, sizeof(Order), 1, f) != 1) {
                break;
            }
            beg = list_add_last(beg, x);
        }
        fclose(f);
        list_free(old);
        if (interactive) {
            printf("  Новый список загружен из бинарного файла\n");
            ui_pause_menu();
        }
        return beg;
    }

    f = fopen(filename, "rt");
    if (f == NULL) {
        if (interactive) {
            ui_show_error("не удалось открыть текстовый файл");
        }
        return old;
    }
    beg = NULL;
    while (fscanf(f, "%d;%14[^;];%49[^;];%d;%d;%d;%d;%d",
                  &x.num, x.name_cafe, x.address, &x.order_amount,
                  &x.deliver_amount, &x.time_cook, &x.time_deliver, &x.tips) == 8) {
        beg = list_add_last(beg, x);
    }
    fclose(f);
    list_free(old);
    if (interactive) {
        printf("  Новый список загружен из текстового файла\n");
        ui_pause_menu();
    }
    return beg;
}

/* Постраничный просмотр таблицы заказов */
void view_table(Node *beg) {
    int total;
    int total_pages;
    int page;
    int key;
    int nav;
    int arrow;
    int shown;
    int skip;
    Node *p;

    if (beg == NULL) {
        printf("  Список пуст\n");
        return;
    }

    total = list_count(beg);
    total_pages = (total + PAGE_SIZE - 1) / PAGE_SIZE;
    page = 0;

    while (1) {
        system("cls");
        if (total_pages <= 1) {
            printf("\n  Esc - выход в меню\n\n");
        } else {
            printf("\n  <- пред. стр. | -> след. стр. | Esc - выход в меню\n");
        }

        printf("\n| № заказа | Ресторан       | Адрес доставки               | Сумма | Дост. | Готовка | В пути | Чаевые |\n");
        printf("|----------|----------------|------------------------------|-------|-------|---------|--------|--------|\n");

        skip = page * PAGE_SIZE;
        p = beg;
        while (p != NULL && skip > 0) {
            p = p->next;
            skip--;
        }
        shown = 0;
        while (p != NULL && shown < PAGE_SIZE) {
            printf("| %8d | %-14.14s | %-*.*s | %5d | %5d | %7d | %6d | %6d |\n",
                   p->inf.num, p->inf.name_cafe, ADDR_COL_WIDTH, ADDR_COL_WIDTH, p->inf.address,
                   p->inf.order_amount, p->inf.deliver_amount, p->inf.time_cook, p->inf.time_deliver, p->inf.tips);
            p = p->next;
            shown++;
        }

        printf("\n  Страница %d из %d\n", page + 1, total_pages);

        while (1) {
            key = _getch();
            if (key == 27) {
                printf("  \nПросмотр прерван, возврат в меню\n");
                return;
            }
            if (total_pages <= 1) {
                continue;
            }
            if (key != 224 && key != 0) {
                continue;
            }
            arrow = _getch();
            nav = 0;
            if (arrow == 77 && page < total_pages - 1) {
                page++;
                nav = 1;
            }
            if (arrow == 75 && page > 0) {
                page--;
                nav = 1;
            }
            if (nav) {
                break;
            }
        }
    }
}

/* Выбор пункта подменю стрелками (^/v, Enter, Esc) */
int menu_pick(const char *title, const char *options[], int count) {
    int selected = 0;
    int key;
    int i;
    HANDLE h;

    h = GetStdHandle(STD_OUTPUT_HANDLE);
    while (1) {
        system("cls");
        printf("\n  Управление: ^/v - выбор | Enter - подтвердить | Esc - в меню\n\n");
        printf("   ================  %s  ================\n\n", title);
        for (i = 0; i < count; i++) {
            if (i == selected) {
                SetConsoleTextAttribute(h, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
                printf(" > %s\n", options[i]);
                SetConsoleTextAttribute(h, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
            } else {
                printf("   %s\n", options[i]);
            }
        }
        key = _getch();
        if (key == 224 || key == 0) {
            key = _getch();
            if (key == 72) {
                selected--;
                if (selected < 0) {
                    selected = count - 1;
                }
            } else if (key == 80) {
                selected++;
                if (selected >= count) {
                    selected = 0;
                }
            }
        } else if (key == 13) {
            return selected;
        } else if (key == 27) {
            return -1;
        }
    }
}

/* Чтение номера команды главного меню */
int menu_read_command(void) {
    const char *items[MENU_ITEMS] = {
        "1. Начальное создание таблицы",
        "2. Просмотр таблицы",
        "3. Добавление новой записи",
        "4. Удаление записи по ключу (номер заказа)",
        "5. Корректировка записи по ключу",
        "6. Сортировка таблицы по ключу (возр./убыв.)",
        "7. Поиск записи по неключевому полю (ресторан)",
        "8. Сохранение таблицы в текстовый и бинарный файл",
        "9. Чтение данных из текстового или бинарного файла",
        "10. Обработка и сохранение результатов в файл",
        "0. Выход"
    };
    const int commands[MENU_ITEMS] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 0};
    int selected = 0;
    int key;
    int i;
    HANDLE h;

    h = GetStdHandle(STD_OUTPUT_HANDLE);
    while (1) {
        system("cls");
        printf("\n  Управление: стрелки ВВЕРХ/ВНИЗ | Enter - выбрать\n\n");
        printf("   ================      МЕНЮ      ================\n\n");
        for (i = 0; i < MENU_ITEMS; i++) {
            if (i == selected) {
                SetConsoleTextAttribute(h, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
                printf(" > %s\n", items[i]);
                SetConsoleTextAttribute(h, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
            } else {
                printf("   %s\n", items[i]);
            }
        }
        key = _getch();
        if (key == 224 || key == 0) {
            key = _getch();
            if (key == 72) {
                selected--;
                if (selected < 0) {
                    selected = MENU_ITEMS - 1;
                }
            } else if (key == 80) {
                selected++;
                if (selected >= MENU_ITEMS) {
                    selected = 0;
                }
            }
        } else if (key == 13) {
            return commands[selected];
        }
    }
}

/* Начальное создание таблицы */
Node *create_table(Node *beg) {
    int n;
    int i;
    int src;
    int rc;
    int recreate;
    Order x;
    char filename[260];
    const char *src_items[] = {
        "1. Ввод с клавиатуры",
        "2. Загрузка из текстового файла",
        "3. Загрузка из бинарного файла"
    };
    const char *confirm_items[] = {
        "1. Да, пересоздать таблицу",
        "2. Нет, оставить текущую"
    };

    if (beg != NULL) {
        recreate = menu_pick("СОЗДАНИЕ ТАБЛИЦЫ", confirm_items, 2);
        if (recreate < 0 || recreate == 1) {
            return beg;
        }
        list_free(beg);
        beg = NULL;
    }

    src = menu_pick("СОЗДАНИЕ ТАБЛИЦЫ", src_items, 3);
    if (src < 0) {
        return beg;
    }

    if (src == 1) {
char *ext;
        print_input_screen("СОЗДАНИЕ ТАБЛИЦЫ");
        if (read_str_esc("Имя текстового файла: ", filename, sizeof(filename)) < 0) {
            return beg;
        }
        if (filename[0] == '\0') {
            ui_show_error("имя файла не может быть пустым");
            return beg;
        }
ext = strrchr(filename, '.');
if (ext != NULL && (_stricmp(ext, ".bin") == 0 || _stricmp(ext, ".dat") == 0)) {
ui_show_error("указан бинарный файл, выберите текстовый");
return beg;
}
        return file_load(beg, filename, 0, 1);
    }

    if (src == 2) {
char *ext;
        print_input_screen("СОЗДАНИЕ ТАБЛИЦЫ");
        if (read_str_esc("Имя бинарного файла: ", filename, sizeof(filename)) < 0) {
            return beg;
        }
        if (filename[0] == '\0') {
            ui_show_error("имя файла не может быть пустым");
            return beg;
        }
ext = strrchr(filename, '.');
if (ext != NULL && _stricmp(ext, ".txt") == 0) {
ui_show_error("указан текстовый файл, выберите бинарный");
return beg;
}
        return file_load(beg, filename, 1, 1);
    }

    print_input_screen("СОЗДАНИЕ ТАБЛИЦЫ");
    while (1) {
        rc = read_int_esc("Сколько записей ввести: ", &n);
        if (rc == -1) {
            return beg;
        }
        if (rc == 0 && n >= 0) {
            break;
        }
        printf("  Ошибка: число записей должно быть целым неотрицательным\n");
    }

    list_free(beg);
    beg = NULL;
    for (i = 0; i < n; i++) {
        printf("  \nВведите запись %d из %d:\n", i + 1, n);
        if (!read_order_record(beg, &x, 1, 1)) {
            list_free(beg);
            return NULL;
        }
        beg = list_add_last(beg, x);
    }
    printf("\n  Таблица создана\n");
    ui_pause_menu();
    return beg;
}

/* Отбор ТОП-5 заказов ресторана по чаевым (%) и по полному времени */
void collect(Node *beg, const char *cafe,
             Order top_tips[], double top_tips_val[], int *top_tips_count,
             Order top_time[], double top_time_val[], int *top_time_count) {
    Node *p;
    double pct;
    double tval;
    double tv;
    Order to;
    int limit;
    int pos;
    int k;

    *top_tips_count = 0;
    *top_time_count = 0;
    p = beg;
    while (p != NULL) {
        if (strcmp(p->inf.name_cafe, cafe) != 0) {
            p = p->next;
            continue;
        }

        pct = (p->inf.order_amount <= 0) ? 0.0
             : ((double)p->inf.tips * 100.0) / (double)p->inf.order_amount;
        tval = (double)(p->inf.time_cook + p->inf.time_deliver);

        pos = *top_tips_count;
        if (pos < TOP_N) {
            top_tips[pos] = p->inf;
            top_tips_val[pos] = pct;
            (*top_tips_count)++;
        } else if (pct > top_tips_val[TOP_N - 1]) {
            top_tips[TOP_N - 1] = p->inf;
            top_tips_val[TOP_N - 1] = pct;
        }
        limit = (*top_tips_count < TOP_N) ? *top_tips_count : TOP_N;
        for (k = limit - 1; k > 0; k--) {
            if (top_tips_val[k] > top_tips_val[k - 1]) {
                tv = top_tips_val[k];
                top_tips_val[k] = top_tips_val[k - 1];
                top_tips_val[k - 1] = tv;
                to = top_tips[k];
                top_tips[k] = top_tips[k - 1];
                top_tips[k - 1] = to;
            } else {
                break;
            }
        }

        pos = *top_time_count;
        if (pos < TOP_N) {
            top_time[pos] = p->inf;
            top_time_val[pos] = tval;
            (*top_time_count)++;
        } else if (tval > top_time_val[TOP_N - 1]) {
            top_time[TOP_N - 1] = p->inf;
            top_time_val[TOP_N - 1] = tval;
        }
        limit = (*top_time_count < TOP_N) ? *top_time_count : TOP_N;
        for (k = limit - 1; k > 0; k--) {
            if (top_time_val[k] > top_time_val[k - 1]) {
                tv = top_time_val[k];
                top_time_val[k] = top_time_val[k - 1];
                top_time_val[k - 1] = tv;
                to = top_time[k];
                top_time[k] = top_time[k - 1];
                top_time[k - 1] = to;
            } else {
                break;
            }
        }
        p = p->next;
    }
}

/* Сохранение отчета обработки в текстовый файл */
int save_to_file(Node *beg, const char *filename, char cafes[][CAFE_LEN], int cafe_count) {
    FILE *f;
    int i;
    int k;
    Order top_tips[TOP_N];
    double top_tips_val[TOP_N];
    int top_tips_count;
    Order top_time[TOP_N];
    double top_time_val[TOP_N];
    int top_time_count;

    f = fopen(filename, "wt");
    if (f == NULL) {
        return 0;
    }

    fprintf(f, "Результаты обработки заказов службы доставки еды\n\n");

    for (i = 0; i < cafe_count; i++) {
        collect(beg, cafes[i], top_tips, top_tips_val, &top_tips_count,
                top_time, top_time_val, &top_time_count);

        fprintf(f, "Ресторан: %s\n", cafes[i]);
        fprintf(f, "ТОП-5 самых щедрых заказов (процент чаевых от суммы):\n");
        for (k = 0; k < top_tips_count; k++) {
            fprintf(f, "  #%d | заказ %d | чаевые %d | сумма %d | %.2f%%\n",
                    k + 1, top_tips[k].num, top_tips[k].tips, top_tips[k].order_amount, top_tips_val[k]);
        }
        if (top_tips_count == 0) {
            fprintf(f, "  Нет данных!\n");
        }

        fprintf(f, "ТОП-5 самых долгих заказов (готовка + путь):\n");
        for (k = 0; k < top_time_count; k++) {
            fprintf(f, "  #%d | заказ %d | готовка %d | путь %d | всего %d мин\n",
                    k + 1, top_time[k].num, top_time[k].time_cook, top_time[k].time_deliver, (int)top_time_val[k]);
        }
        if (top_time_count == 0) {
            fprintf(f, "  Нет данных!\n");
        }
        fprintf(f, "\n");
    }

    fclose(f);
    return 1;
}

/* Просмотр результатов обработки на экране (по ресторанам, со скроллингом) */
void show_result(Node *beg, char cafes[][CAFE_LEN], int cafe_count) {
    int page;
    int key;
    int nav;
    int arrow;
    int k;
    Order top_tips[TOP_N];
    double top_tips_val[TOP_N];
    int top_tips_count;
    Order top_time[TOP_N];
    double top_time_val[TOP_N];
    int top_time_count;

    if (cafe_count <= 0) {
        printf("  Нет данных для отображения!\n");
        return;
    }

    page = 0;
    while (1) {
        system("cls");
        if (cafe_count <= 1) {
            printf("\n  Esc - выход в меню\n\n");
        } else {
            printf("\n  <- пред. стр. | -> след. стр. | Esc - выход в меню\n");
        }

        collect(beg, cafes[page], top_tips, top_tips_val, &top_tips_count,
                top_time, top_time_val, &top_time_count);

        printf("\n  Результаты обработки заказов службы доставки еды\n\n");
        printf("  Ресторан: %s\n\n", cafes[page]);
        printf("  ТОП-5 самых щедрых заказов (процент чаевых от суммы):\n");
        if (top_tips_count == 0) {
            printf("  Нет данных!\n");
        } else {
            printf("\n| № заказа | Ресторан       | Адрес доставки               | Сумма | Дост. | Готовка | В пути | Чаевые |\n");
            printf("|----------|----------------|------------------------------|-------|-------|---------|--------|--------|\n");
            for (k = 0; k < top_tips_count; k++) {
                printf("| %8d | %-14.14s | %-*.*s | %5d | %5d | %7d | %6d | %6d |\n",
                       top_tips[k].num, top_tips[k].name_cafe, ADDR_COL_WIDTH, ADDR_COL_WIDTH, top_tips[k].address,
                       top_tips[k].order_amount, top_tips[k].deliver_amount,
                       top_tips[k].time_cook, top_tips[k].time_deliver, top_tips[k].tips);
            }
        }

        printf("\n  ТОП-5 самых долгих заказов (готовка + путь):\n");
        if (top_time_count == 0) {
            printf("  Нет данных!\n");
        } else {
            printf("\n| № заказа | Ресторан       | Адрес доставки               | Сумма | Дост. | Готовка | В пути | Чаевые |\n");
            printf("|----------|----------------|------------------------------|-------|-------|---------|--------|--------|\n");
            for (k = 0; k < top_time_count; k++) {
                printf("| %8d | %-14.14s | %-*.*s | %5d | %5d | %7d | %6d | %6d |\n",
                       top_time[k].num, top_time[k].name_cafe, ADDR_COL_WIDTH, ADDR_COL_WIDTH, top_time[k].address,
                       top_time[k].order_amount, top_time[k].deliver_amount,
                       top_time[k].time_cook, top_time[k].time_deliver, top_time[k].tips);
            }
        }
        printf("\n  Страница %d из %d\n", page + 1, cafe_count);

        while (1) {
            key = _getch();
            if (key == 27) {
                return;
            }
            if (cafe_count <= 1) {
                continue;
            }
            if (key != 224 && key != 0) {
                continue;
            }
            arrow = _getch();
            nav = 0;
            if (arrow == 77 && page < cafe_count - 1) {
                page++;
                nav = 1;
            }
            if (arrow == 75 && page > 0) {
                page--;
                nav = 1;
            }
            if (nav) {
                break;
            }
        }
    }
}

/* Обработка таблицы: координация отбора, записи в файл и вывода на экран */
int process_data(Node *beg, const char *filename) {
    char cafes[300][CAFE_LEN];
    int cafe_count;
    int found_cafe;
    int c;
    Node *p;

    if (beg == NULL) {
        printf("  Список пуст, обработка невозможна!\n");
        return 0;
    }

    cafe_count = 0;
    p = beg;
    while (p != NULL && cafe_count < 300) {
        found_cafe = 0;
        for (c = 0; c < cafe_count; c++) {
            if (strcmp(cafes[c], p->inf.name_cafe) == 0) {
                found_cafe = 1;
                break;
            }
        }
        if (!found_cafe) {
            strcpy(cafes[cafe_count], p->inf.name_cafe);
            cafe_count++;
        }
        p = p->next;
    }

    if (!save_to_file(beg, filename, cafes, cafe_count)) {
        return 0;
    }

    show_result(beg, cafes, cafe_count);
    return 1;
}

/* Поиск записей по названию ресторана */
void search_by_restaurant(Node *beg, const char *cafe) {
    int found = 0;
    Node *p = beg;
    while (p != NULL) {
        if (strcmp(p->inf.name_cafe, cafe) == 0) {
            if (!found) {
                printf("\n| № заказа | Ресторан       | Адрес доставки               | Сумма | Дост. | Готовка | В пути | Чаевые |\n");
                printf("|----------|----------------|------------------------------|-------|-------|---------|--------|--------|\n");
            }
            printf("| %8d | %-14.14s | %-*.*s | %5d | %5d | %7d | %6d | %6d |\n",
                   p->inf.num, p->inf.name_cafe, ADDR_COL_WIDTH, ADDR_COL_WIDTH, p->inf.address,
                   p->inf.order_amount, p->inf.deliver_amount,
                   p->inf.time_cook, p->inf.time_deliver, p->inf.tips);
            found = 1;
        }
        p = p->next;
    }
    if (!found)
        printf("  Записи по ресторану \"%s\" не найдены!\n", cafe);
}

/* Корректировка записи */
Node *edit_record(Node *beg) {
    Node *edit_node;
    int key, rc;
    print_input_screen("Корректировка записи");
    rc = read_int_esc("Введите номер заказа для корректировки: ", &key);
    if (rc == -1) return beg;
    if (rc != 0 || key <= 0) {
        ui_show_error("номер заказа должен быть положительным целым");
        return beg;
    }
    edit_node = list_find(beg, key);
    if (edit_node == NULL) {
        ui_show_error("запись с таким номером не найдена");
        return beg;
    }
    system("cls");
    printf("\n  Текущая запись:\n\n");
    printf("| № заказа | Ресторан       | Адрес доставки               | Сумма | Дост. | Готовка | В пути | Чаевые |\n");
    printf("|----------|----------------|------------------------------|-------|-------|---------|--------|--------|\n");
    printf("| %8d | %-14.14s | %-*.*s | %5d | %5d | %7d | %6d | %6d |\n",
           edit_node->inf.num, edit_node->inf.name_cafe,
           ADDR_COL_WIDTH, ADDR_COL_WIDTH, edit_node->inf.address,
           edit_node->inf.order_amount, edit_node->inf.deliver_amount,
           edit_node->inf.time_cook, edit_node->inf.time_deliver, edit_node->inf.tips);
    printf("\nВведите новые данные:\n");
    if (!read_order_record(beg, &edit_node->inf, 0, 0))
        printf("  Корректировка отменена\n");
    else
        printf("  Запись обновлена\n");
    ui_pause_menu();
    return beg;
}

/* Выполнение выбранного пункта меню */
Node *menu_execute(int cmd, Node *beg) {
    Order x;
    int key;
    int rc;
    int mode;
    char cafe[CAFE_LEN];
    char text_file[260];
    char bin_file[260];
    char filename[260];
    char result_file[260];
    const char *sort_items[] = {
        "1. По возрастанию номера заказа",
        "2. По убыванию номера заказа"
    };
    const char *read_items[] = {
        "1. Текстовый файл",
        "2. Бинарный файл"
    };

    switch (cmd) {
        case 1:
            return create_table(beg);
        case 2:
            view_table(beg);
            return beg;
        case 3:
            print_input_screen("ДОБАВЛЕНИЕ ЗАПИСИ");
            if (!read_order_record(beg, &x, 1, 0)) {
                return beg;
            }
            beg = list_add_last(beg, x);
            printf("  Запись добавлена\n");
            ui_pause_menu();
            return beg;
        case 4:
            print_input_screen("УДАЛЕНИЕ ЗАПИСИ");
            rc = read_int_esc("Введите номер заказа для удаления: ", &key);
            if (rc == -1) {
                return beg;
            }
            if (rc != 0 || key <= 0) {
                ui_show_error("номер заказа должен быть положительным целым");
                return beg;
            }
            if (list_find(beg, key) == NULL) {
                ui_show_error("запись с таким номером не найдена");
                return beg;
            }
            beg = list_delete(beg, key);
            ui_pause_menu();
            return beg;
        case 5:
            return edit_record(beg);
        case 6:
            mode = menu_pick("СОРТИРОВКА", sort_items, 2);
            if (mode < 0) {
                return beg;
            }
            beg = list_sort(beg, mode == 0);
            ui_pause_menu();
            return beg;
        case 7:
            print_input_screen("ПОИСК ПО РЕСТОРАНУ");
            if (read_str_esc("Название ресторана: ", cafe, CAFE_LEN) < 0)
                return beg;
            if (cafe[0] == '\0' || strchr(cafe, ' ') != NULL) {
                ui_show_error("название ресторана не должно быть пустым");
                return beg;
            }
            system("cls");
            printf("\n  Результаты поиска:\n");
            search_by_restaurant(beg, cafe);
            ui_pause_menu();
            return beg;
        case 8:
            print_input_screen("СОХРАНЕНИЕ В ФАЙЛ");
            if (read_str_esc("Имя текстового файла: ", text_file, sizeof(text_file)) < 0) {
                return beg;
            }
            if (text_file[0] == '\0') {
                ui_show_error("имя текстового файла не может быть пустым");
                return beg;
            }
            if (read_str_esc("Имя бинарного файла: ", bin_file, sizeof(bin_file)) < 0) {
                return beg;
            }
            if (bin_file[0] == '\0') {
                ui_show_error("имя бинарного файла не может быть пустым");
                return beg;
            }
            if (beg == NULL) {
                ui_show_error("список пуст, сохранять нечего");
                return beg;
            }
            if (!file_save(beg, text_file, 0)) {
                ui_show_error("ошибка сохранения текстового файла");
                return beg;
            }
            printf("  Текстовый файл сохранен\n");
            if (!file_save(beg, bin_file, 1)) {
                ui_show_error("ошибка сохранения бинарного файла");
                return beg;
            }
            printf("  Бинарный файл сохранен\n");
            ui_pause_menu();
            return beg;
        case 9:
            mode = menu_pick("ЧТЕНИЕ ИЗ ФАЙЛА", read_items, 2);
            if (mode < 0) {
                return beg;
            }
            print_input_screen("ЧТЕНИЕ ИЗ ФАЙЛА");
            if (read_str_esc("Имя файла: ", filename, sizeof(filename)) < 0) {
                return beg;
            }
            if (filename[0] == '\0') {
                ui_show_error("имя файла не может быть пустым");
                return beg;
            }
            return file_load(beg, filename, mode == 1, 1);
        case 10:
            print_input_screen("ОБРАБОТКА ДАННЫХ");
            if (read_str_esc("Имя файла отчета: ", result_file, sizeof(result_file)) < 0) {
                return beg;
            }
            if (result_file[0] == '\0') {
                ui_show_error("имя файла отчета не может быть пустым");
                return beg;
            }
            if (!process_data(beg, result_file)) {
                ui_show_error("ошибка обработки или сохранения");
                return beg;
            }
            printf("  Результаты выведены на экран и сохранены в файл\n");
            ui_pause_menu();
            return beg;
        default:
            printf("  Неизвестный пункт меню!\n");
            return beg;
    }
}

/* Главная функция программы */
int main(void) {
    Node *beg;
    int cmd;

    setup_console();
    beg = NULL;

    while (1) {
        cmd = menu_read_command();
        if (cmd == 0) {
            if (beg != NULL) {
                if (file_save(beg, DEFAULT_SAVE_FILE, 0)) {
                    printf("  Данные сохранены в %s\n", DEFAULT_SAVE_FILE);
                }
            }
            list_free(beg);
            printf("\n  Программа завершена!\n");
            return 0;
        }
        system("cls");
        beg = menu_execute(cmd, beg);
    }
}
