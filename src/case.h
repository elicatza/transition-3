#ifndef CASE_H
#define CASE_H

#include <stddef.h>


typedef struct CaseBase CaseBase;
struct CaseBase {
    size_t capacity;
    size_t len;
    size_t size;
};

#define GROW_FACTOR 2

#define FIELD_OFFSET(type, field) (unsigned long) &(((type *) 0)->field)
#define case_cap(arr) *(size_t *) ((char *) (arr) - sizeof(CaseBase) + FIELD_OFFSET(CaseBase, capacity))
#define case_len(arr) *(size_t *) ((char *) (arr) - sizeof(CaseBase) + FIELD_OFFSET(CaseBase, len))
#define case_size(arr) *(size_t *) ((char *) (arr) - sizeof(CaseBase) + FIELD_OFFSET(CaseBase, size))
#define case_base(arr) (*(CaseBase *) ((char *) (arr) - sizeof(CaseBase)))
#define case_base_to_arr(base) ((char *) (base) + sizeof(CaseBase))

#define case_pop(arr) (assert(case_len(arr) > 0), --case_len(arr), arr[case_len(arr)])
#define case_get(arr, i) (assert(i >= 0), assert(i < case_len(arr)), arr[(i)])
#define case_push(arr, item) \
    do { \
        if (case_len(arr) >= case_cap(arr)) { \
            case_resize((void *) &(arr), case_cap(arr) * GROW_FACTOR); \
        } \
        (arr)[case_len((arr))] = (item); \
        ++case_len(arr); \
    } while (0)

#define case_remove(arr, idx) (case_shift_left(arr, idx), case_pop(arr))
#define case_insert(arr, item, idx) do { case_push(arr, item); case_shift_right(arr, idx); } while (0)
#define case_print(arr, fmt) \
    do { \
        size_t __i; \
        printf("%s = [ ", #arr); \
        for (__i = 0; __i < case_len((arr)); ++__i) { \
            printf(fmt" ", (arr)[__i]); \
        } \
        printf("]\n"); \
    } while (0)

void *case_init(size_t capacity, size_t item_size);
void case_clear(void *arr);
void case_resize(void **arr, size_t size);
void case_append(void **dest, void *src);
void case_shift_left(void *arr, size_t start);
void case_shift_right(void *arr, size_t start);
void case_free(void *arr);

#endif // CASE_H
#ifdef CASE_IMPLEMENTATION

#include <assert.h>
#include <stdlib.h>
#include <string.h>

void *case_init(size_t capacity, size_t item_size)
{
    void *base = malloc(item_size * capacity + sizeof(CaseBase));
    assert(base != NULL && "Malloc failed: buy more ram");
    void *arr = case_base_to_arr(base);

    case_len(arr) = 0;
    case_cap(arr) = capacity;
    case_size(arr) = item_size;
    return arr;
}

void case_resize(void **arr, size_t size)
{
    printf("Current cap: %zu\n", case_cap(*arr));
    case_cap(*arr) = size;
    void *mem = realloc(&case_base(*arr), case_size(*arr) * case_cap(*arr) + sizeof(CaseBase));
    assert(mem != NULL && "ERROR: realloc failed. buy more ram");
    *arr = case_base_to_arr(mem);

    printf("Changed cap: %zu\n", case_cap(*arr));
}

void case_append(void **dest, void *src)
{
    assert(case_size(*dest) == case_size(src));
    size_t new_len = case_len(*dest) + case_len(src);

    if (new_len > case_cap(*dest)) {
        case_resize(dest, new_len * GROW_FACTOR);
    }

    void *rv = memcpy((char *) *dest + case_len(*dest) * case_size(*dest), src, case_len(src) * case_size(src));
    assert(rv != NULL);
    case_len(*dest) = new_len;
}

void case_shift_left(void *arr, size_t start)
{
    long int diff = case_len(arr) - start;
    assert(diff > 0);

    void *first = malloc(case_size(arr));
    memcpy(first, (char *) arr + start * case_size(arr), case_size(arr));

    memmove((char *) arr + start * case_size(arr), (char *) arr + (start + 1) * case_size(arr), diff * case_size(arr));

    memcpy((char *) arr + (case_len(arr) - 1) * case_size(arr), first, case_size(arr));
    free(first);
}

void case_shift_right(void *arr, size_t start)
{
    long int diff = case_len(arr) - start;
    assert(diff > 0);

    void *last = malloc(case_size(arr));
    memcpy(last, (char *) arr + (case_len(arr) - 1) * case_size(arr), case_size(arr));

    memmove((char *) arr + (start + 1) * case_size(arr), (char *) arr + start * case_size(arr), diff * case_size(arr));

    memcpy((char *) arr + start * case_size(arr), last, case_size(arr));
    free(last);
}

void case_clear(void *arr)
{
    case_len(arr) = 0;
}

void case_free(void *arr)
{
    free(&case_base(arr));
}

#endif // CASE_IMPLEMENTATION


/* LICENSE
The MIT License (MIT)

Copyright (c) 2023 Eliza Clausen

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
 */
