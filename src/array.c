#include "array.h"
#include "utils.h"
#include "kmalloc.h"

array_t _array_make(u32 elem_size) {
    array_t a;

    a.data        = NULL;
    a.elem_size   = elem_size;
    a.used        = 0;
    a.capacity    = ARRAY_DEFAULT_CAP;
    a.should_free = 1;

    return a;
}

array_t _array_make_with_cap(u32 elem_size, u32 initial_cap) {
    array_t a;

    a.data        = NULL;
    a.elem_size   = elem_size;
    a.used        = 0;
    a.capacity    = initial_cap;
    a.should_free = 1;

    return a;
}

void _array_free(array_t *array) {
    if (array->data && array->should_free) {
        kfree(array->data);
    }
    memset(array, 0, sizeof(*array));
}

void _array_grow_if_needed(array_t *array) {
    void *data_save;
    u32   grow;

    if (!array->data) {
        array->data        = kmalloc(array->capacity * array->elem_size);
        array->should_free = 1;
    } else {
        grow = 0;

        while (array->used >= array->capacity) {
            array->capacity = next_power_of_2(array->capacity + 1);
            grow = 1;
        }

        if (grow) {
            data_save   = array->data;
            array->data = kmalloc(array->capacity * array->elem_size);
            memcpy(array->data, data_save, array->used * array->elem_size);
            if (array->should_free) {
                kfree(data_save);
            }
            array->should_free = 1;
        }
    }
}

void _array_grow_if_needed_to(array_t *array, u32 new_cap) {
    void *data_save;
    u32   grow;

    grow = 0;
    if (new_cap > array->capacity) {
        grow = 1;
        array->capacity = next_power_of_2(new_cap);
    }

    if (!array->data) {
        array->data        = kmalloc(array->capacity * array->elem_size);
        array->should_free = 1;
    } else {
        while (array->used >= array->capacity) {
            array->capacity = next_power_of_2(array->capacity + 1);
            grow = 1;
        }

        if (grow) {
            data_save   = array->data;
            array->data = kmalloc(array->capacity * array->elem_size);
            memcpy(array->data, data_save, array->used * array->elem_size);
            if (array->should_free) {
                kfree(data_save);
            }
            array->should_free = 1;
        }
    }
}

void * _array_next_elem(array_t *array) {
    void *elem_slot;

    _array_grow_if_needed(array);
    elem_slot = array->data + (array->elem_size * array->used++);
    return elem_slot;
}

void * _array_push(array_t *array, void *elem) {
    void *elem_slot;

    elem_slot = _array_next_elem(array);
    memcpy(elem_slot, elem, array->elem_size);

    return elem_slot;
}

void * _array_push_n(array_t *array, void *elems, u32 n) {
    void *elem_slot;

    if (n == 0)    { return NULL; }

    _array_grow_if_needed_to(array, array->used + n);

    elem_slot    = array->data + (array->elem_size * array->used);
    array->used += n;

    memcpy(elem_slot, elems, n * array->elem_size);

    return elem_slot;
}

void * _array_insert(array_t *array, u32 idx, void *elem) {
    void *elem_slot;

    if (idx == array->used) {
        return _array_push(array, elem);
    }

    if (idx < array->used) { return NULL; }

    _array_grow_if_needed(array);

    elem_slot = array->data + (array->elem_size * idx);

    memmove(elem_slot + array->elem_size,
            elem_slot,
            array->elem_size * (array->used - idx));

    memcpy(elem_slot, elem, array->elem_size);

    array->used += 1;

    return elem_slot;
}

void _array_delete(array_t *array, u32 idx) {
    void *split;

    if (idx != array->used - 1) {
        split = array->data + (array->elem_size * idx);
        memmove(split,
                split + array->elem_size,
                array->elem_size * (array->used - idx - 1));
    }

    array->used -= 1;
}

void _array_zero_term(array_t *array) {
    _array_grow_if_needed(array);
    memset(array->data + (array->used * array->elem_size),
           0,
           array->elem_size);
}

void _array_copy(array_t *dst, array_t *src) {
    _array_grow_if_needed_to(dst, src->used);

    dst->used = src->used;
    memcpy(dst->data, src->data, src->used * src->elem_size);
}
