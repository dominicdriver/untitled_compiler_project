#include "strings.h"

#include <assert.h>
#include <string.h>

bool string_cat(string *dest, const string *src) {
    if (dest->cap <= dest->len + src->len) {
        assert(0);
    }

    memcpy(dest->data+dest->len, src->data, src->len);

    dest->len += src->len;
    dest->data[dest->len] = 0;

    return true;
}

bool string_cat_c(string *dest, char c) {
    if (dest->cap <= dest->len + 1) {
        assert(0);
    }

    dest->data[dest->len] = c;
    dest->len++;
    dest->data[dest->len] = 0;

    return true;
}

int16_t string_cmp(const string *s1, const string *s2) {

    if (s1->len == 0 && s2->len == 0) {
        return 0;
    }

    if (s1->len == 0 || s2->len == 0) {
        return ((s2->len == 0) * 2) - 1;
    }

    if (s1->len < s2->len) {
        return s1->data[s1->len] - s2->data[s1->len];
    }

    if (s1->len > s2->len) {
        return s1->data[s2->len] - s2->data[s2->len];
    }

    for (uint16_t i = 0; i < s1->len; i++) {
        if (s1->data[i] != s2->data[i]) {
            return s1->data[i] - s2->data[i];
        }
    }

    return 0;
}

bool string_copy(string *dest, const string *src) {
    if (dest->cap < src->len) {
        assert(0);
    }

    dest->len = src->len;

    memcpy(dest->data, src->data, src->len);

    dest->data[dest->len] = 0;

    return true;
}

string string_rstr(const string *s, uint8_t c) {
    string str = {0};
    uint16_t index = s->len - 1;

    while (index > 0 && s->data[index] != c) index--;

    str.data = &s->data[index];
    str.cap = s->cap - index;
    str.len = s->len - index;

    // char not found
    if (str.data[0] != c) {
        str.data = NULL;
        str.cap = 0;
        str.len = 0;
    }

    return str;
}

// Returns a new string containing a slice of s,
// from s->data[start] to s->data[end] (inclusive)
// The resulting string may NOT be NULL terminated
string string_slice(const string *s, uint16_t start, uint16_t end) {
    string str = {0};

    // Return a blank string if start or end is invalid
    if (start > s->len || end > s->len) {
        return str;
    }

    str.len = end - start;
    str.cap = str.len + 1;
    str.data = &s->data[start];

    return str;
}
