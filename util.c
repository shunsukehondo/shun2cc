#include "shun2cc.h"

noreturn void error(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

char *format(char *fmt, ...) {
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    return strdup(buf);
}

Vector *new_vec()
{
    Vector *v = malloc(sizeof(Vector));
    // The size of a void pointer depends on platform.
    v->data = malloc(sizeof(void *) * 16);
    v->capacity = 16;
    v->len = 0;
    return v;
}

void vec_push(Vector *v, void *elem)
{
    if (v->len == v->capacity) {
        v->capacity *= 2;
        v->data = realloc(v->data, sizeof(void *) * v->capacity);
    }

    v->data[v->len++] = elem;
}

Map *new_map(void)
{
    Map *map = malloc(sizeof(Map));
    map->keys = new_vec();
    map->vals = new_vec();
    return map;
}

void map_put(Map *map, char *key, void *val)
{
    vec_push(map->keys, key);
    vec_push(map->vals, val);
}

void *map_get(Map *map, char *key)
{
    for (int i=map->keys->len - 1; i >= 0; i--) {
        if (!strcmp(map->keys->data[i], key)) {
            return map->vals->data[i];
        }
    }

    return NULL;
}

bool map_exists(Map *map, char *key)
{
    for (int i=0; i<map->keys->len; i++) {
        if (!strcmp(map->keys->data[i], key)) {
            return true;
        }
    }

    return false;
}

StringBuilder *new_sb(void)
{
    StringBuilder *sb = calloc(1, sizeof(StringBuilder));
    sb->data = malloc(8);
    sb->capacity = 8;
    sb->len = 0;
    return sb;
}

static void sb_grow(StringBuilder *sb, int len)
{
    if (sb->len + len <= sb->capacity) {
        return;
    }

    while (sb->len + len > sb->capacity) {
        sb->capacity *= 2;
    }

    sb->data = realloc(sb->data, sb->capacity);
}

void sb_append(StringBuilder *sb, char *s) {
    int len = strlen(s);
    sb_grow(sb, len);

    // 既存データの末尾にsをコピー
    memcpy(sb->data + sb->len, s, len);
    sb->len += len;
}

char *sb_get(StringBuilder *sb) {
    sb_grow(sb, 1);
    sb->data[sb->len] = '\0';
    return sb->data;
}
