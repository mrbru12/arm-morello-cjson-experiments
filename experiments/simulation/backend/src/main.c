#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

#include "cJSON.h"

// #define PARSE_WHOLE_SRC_JSON

static volatile int    g_tracking        = 0;
static size_t          g_malloc_calls    = 0;
static size_t          g_realloc_calls   = 0;
static size_t          g_calloc_calls    = 0;
static size_t          g_free_calls      = 0;
static size_t          g_bytes_requested = 0;

extern void *__real_malloc(size_t size);
extern void *__real_realloc(void *ptr, size_t size);
extern void *__real_calloc(size_t nmemb, size_t size);
extern void  __real_free(void *ptr);

void *__wrap_malloc(size_t size) {
    if (g_tracking) {
        g_malloc_calls++;
        g_bytes_requested += size;
    }
    return __real_malloc(size);
}

void *__wrap_realloc(void *ptr, size_t size) {
    if (g_tracking) {
        g_realloc_calls++;
        g_bytes_requested += size;
    }
    return __real_realloc(ptr, size);
}

void *__wrap_calloc(size_t nmemb, size_t size) {
    if (g_tracking) {
        g_calloc_calls++;
        g_bytes_requested += nmemb * size;
    }
    return __real_calloc(nmemb, size);
}

void __wrap_free(void *ptr) {
    if (g_tracking && ptr) {
        g_free_calls++;
    }
    __real_free(ptr);
}

static void reset_counters(void) {
    g_malloc_calls    = 0;
    g_realloc_calls   = 0;
    g_calloc_calls    = 0;
    g_free_calls      = 0;
    g_bytes_requested = 0;
}

static struct timespec ts_start, ts_end;

static void timer_start(void) {
    clock_gettime(CLOCK_MONOTONIC, &ts_start);
}

static double timer_stop_us(void) {
    clock_gettime(CLOCK_MONOTONIC, &ts_end);
    double s  = (double)(ts_end.tv_sec  - ts_start.tv_sec);
    double ns = (double)(ts_end.tv_nsec - ts_start.tv_nsec);
    return s * 1e6 + ns / 1e3;
}

#define INITIAL_BUF 65536

static char *read_stdin(size_t *out_len) {
    size_t cap  = INITIAL_BUF;
    size_t used = 0;
    char  *buf  = malloc(cap);
    if (!buf) return NULL;

    int c;
    while ((c = fgetc(stdin)) != EOF) {
        if (used + 1 >= cap) {
            cap *= 2;
            char *tmp = realloc(buf, cap);
            if (!tmp) { free(buf); return NULL; }
            buf = tmp;
        }
        buf[used++] = (char)c;
    }
    buf[used] = '\0';
    *out_len = used;
    return buf;
}

static char *next_json_object(const char *src, size_t src_len, size_t *pos) {
#ifdef PARSE_WHOLE_SRC_JSON
    return src_len - *pos > 0 ? *pos = src_len, strdup(src) : NULL;
#endif

    while (*pos < src_len && src[*pos] != '{') (*pos)++;
    if (*pos >= src_len) return NULL;

    size_t start = *pos;
    int    depth = 0;
    int    in_str = 0;

    for (size_t i = start; i < src_len; i++) {
        char ch = src[i];

        if (in_str) {
            if (ch == '\\') { i++; continue; }
            if (ch == '"')  in_str = 0;
            continue;
        }

        if (ch == '"') { in_str = 1; continue; }
        if (ch == '{') depth++;
        else if (ch == '}') {
            depth--;
            if (depth == 0) {
                size_t len = i - start + 1;
                char  *obj = malloc(len + 1);
                if (!obj) return NULL;
                memcpy(obj, src + start, len);
                obj[len] = '\0';
                *pos = i + 1;
                return obj;
            }
        }
    }
    return NULL;
}

typedef struct {
    char   location[64];
    double latitude;
    double longitude;
    double elevation;
    char   timezone[8];
    char   unit[32];
    char   time[32];
    char   group_type[32];
    char   measurement_type[32];
    double measurement;
} backend_measurement;

#define STRFIELD(dst, obj, key)                                      \
    do {                                                             \
        cJSON *_n = cJSON_GetObjectItem((obj), (key));               \
        if (_n && cJSON_IsString(_n)) {                              \
            strncpy((dst), _n->valuestring, sizeof(dst) - 1);        \
            (dst)[sizeof(dst) - 1] = '\0';                           \
        }                                                            \
    } while (0)

#define NUMFIELD(dst, obj, key)                                      \
    do {                                                             \
        cJSON *_n = cJSON_GetObjectItem((obj), (key));               \
        if (_n && cJSON_IsNumber(_n)) {                              \
            (dst) = _n->valuedouble;                                 \
        }                                                            \
    } while (0)


static int parse_payload_array(
    const char *json_str,
    backend_measurement **out_measurements,
    size_t *out_count
) {
    cJSON *root = cJSON_Parse(json_str);
    if (!root) {
        return 0;
    }

    if (!cJSON_IsArray(root)) {
        cJSON_Delete(root);
        return 0;
    }

    int count = cJSON_GetArraySize(root);

    backend_measurement *arr =
        calloc(count, sizeof(backend_measurement));

    if (!arr) {
        cJSON_Delete(root);
        return 0;
    }

    for (int i = 0; i < count; i++) {

        cJSON *item = cJSON_GetArrayItem(root, i);

        if (!cJSON_IsObject(item)) {
            continue;
        }

        backend_measurement *m = &arr[i];

        cJSON *meta = cJSON_GetObjectItem(item, "meta");

        if (meta && cJSON_IsObject(meta)) {
            STRFIELD(m->location,   meta, "location");
            NUMFIELD(m->latitude,   meta, "latitude");
            NUMFIELD(m->longitude,  meta, "longitude");
            NUMFIELD(m->elevation,  meta, "elevation");
            STRFIELD(m->timezone,   meta, "timezone");
        }

        STRFIELD(m->unit,             item, "unit");
        STRFIELD(m->time,             item, "time");
        STRFIELD(m->group_type,       item, "group_type");
        STRFIELD(m->measurement_type, item, "measurement_type");

        NUMFIELD(m->measurement, item, "measurement");
    }

    cJSON_Delete(root);

    *out_measurements = arr;
    *out_count = (size_t)count;

    return 1;
}

static int parse_payload(const char *json_str, backend_measurement *out) {
    cJSON *root = cJSON_Parse(json_str);
    if (!root) return 0;

    cJSON *meta = cJSON_GetObjectItem(root, "meta");
    if (!meta) { cJSON_Delete(root); return 0; }

    STRFIELD(out->location,         meta, "location");
    NUMFIELD(out->latitude,         meta, "latitude");
    NUMFIELD(out->longitude,        meta, "longitude");
    NUMFIELD(out->elevation,        meta, "elevation");
    STRFIELD(out->timezone,         meta, "timezone");
    STRFIELD(out->unit,             root, "unit");
    STRFIELD(out->time,             root, "time");
    STRFIELD(out->group_type,       root, "group_type");
    STRFIELD(out->measurement_type, root, "measurement_type");
    NUMFIELD(out->measurement,      root, "measurement");

    cJSON_Delete(root);
    return 1;
}

typedef struct {
    double parse_us;
    size_t malloc_calls;
    size_t realloc_calls;
    size_t calloc_calls;
    size_t free_calls;
    size_t bytes_requested;
    int    ok;
} payload_stats;

void execute_test_iteration(char *src, size_t src_len, FILE *f) {
    size_t         pos           = 0;
    size_t         total_parsed  = 0;
    size_t         total_failed  = 0;
    double         total_time_us = 0.0;
    size_t         total_mallocs = 0;
    size_t         total_reallocs= 0;
    size_t         total_callocs = 0;
    size_t         total_frees   = 0;
    size_t         total_bytes   = 0;

    size_t          stats_cap  = 256;
    size_t          stats_used = 0;
    payload_stats  *stats = malloc(stats_cap * sizeof(*stats));
    if (!stats) { return; }

    char *obj_str;
    while ((obj_str = next_json_object(src, src_len, &pos)) != NULL) {

        if (stats_used >= stats_cap) {
            stats_cap *= 2;
            payload_stats *tmp = realloc(stats, stats_cap * sizeof(*stats));
            if (!tmp) { free(obj_str); break; }
            stats = tmp;
        }

        backend_measurement m;
        memset(&m, 0, sizeof(m));

        reset_counters();

        g_tracking = 1;
        timer_start();

#ifdef PARSE_WHOLE_SRC_JSON
	backend_measurement *ms;
	size_t ms_count;
        int ok = parse_payload_array(obj_str, &ms, &ms_count);
#else
        int ok = parse_payload(obj_str, &m);
#endif

        double elapsed = timer_stop_us();
        g_tracking = 0;

        payload_stats *ps = &stats[stats_used++];
        ps->parse_us       = elapsed;
        ps->malloc_calls   = g_malloc_calls;
        ps->realloc_calls  = g_realloc_calls;
        ps->calloc_calls   = g_calloc_calls;
        ps->free_calls     = g_free_calls;
        ps->bytes_requested= g_bytes_requested;
        ps->ok             = ok;

        if (ok) {
            total_parsed++;
            total_time_us  += elapsed;
            total_mallocs  += g_malloc_calls;
            total_reallocs += g_realloc_calls;
            total_callocs  += g_calloc_calls;
            total_frees    += g_free_calls;
            total_bytes    += g_bytes_requested;
        } else {
            total_failed++;
            fprintf(stderr, "[backend] WARNING: failed to parse payload %zu\n",
                    stats_used);
        }

        free(obj_str);
    }

    if (total_parsed == 0) {
        fprintf(stderr, "[backend] No payloads successfully parsed.\n");
        free(stats);
        return;
    }

    double min_us = stats[0].parse_us, max_us = stats[0].parse_us;
    for (size_t i = 1; i < stats_used; i++) {
        if (!stats[i].ok) continue;
        if (stats[i].parse_us < min_us) min_us = stats[i].parse_us;
        if (stats[i].parse_us > max_us) max_us = stats[i].parse_us;
    }

    printf("\n");
    printf("=================================================================\n");
    printf("  cJSON BACKEND — PARSE BENCHMARK REPORT\n");
    printf("=================================================================\n");
    printf("  Payloads received : %zu\n",   stats_used);
    printf("  Payloads parsed OK: %zu\n",   total_parsed);
    printf("  Payloads failed   : %zu\n",   total_failed);
    printf("-----------------------------------------------------------------\n");
    printf("  TIMING (parse only, µs)\n");
    printf("    Total            : %.2f µs  (%.4f ms)\n",
           total_time_us, total_time_us / 1000.0);
    printf("    Mean per payload : %.4f µs\n", total_time_us / total_parsed);
    printf("    Min              : %.4f µs\n", min_us);
    printf("    Max              : %.4f µs\n", max_us);
    printf("-----------------------------------------------------------------\n");
    printf("  MEMORY OPERATIONS (inside cJSON_Parse only)\n");
    printf("    malloc  calls total    : %zu\n",  total_mallocs);
    printf("    realloc calls total    : %zu\n",  total_reallocs);
    printf("    calloc  calls total    : %zu\n",  total_callocs);
    printf("    free    calls total    : %zu\n",  total_frees);
    printf("    Heap bytes requested   : %zu\n",  total_bytes);
    printf("    malloc  calls / payload: %.2f\n",
           (double)total_mallocs  / total_parsed);
    printf("    free    calls / payload: %.2f\n",
           (double)total_frees    / total_parsed);
    printf("    bytes / payload        : %.2f\n",
           (double)total_bytes    / total_parsed);
    printf("-----------------------------------------------------------------\n");
    printf("=================================================================\n");

    if (f) {
        fprintf(f, "%.2f,%.4f,%.4f,%.4f\n", total_time_us, total_time_us / total_parsed, min_us, max_us);
    }

    free(stats);
}

int main(int argc, char *argv[]) {
    size_t iterations = 20;
    if (argc > 1) {
        sscanf(argv[1], "%zu", &iterations);
    }

    fprintf(stderr, "[backend] Reading from stdin...\n");

    size_t src_len = 0;
    char  *src = read_stdin(&src_len);
    if (!src || src_len == 0) {
        fprintf(stderr, "[backend] No input received.\n");
        free(src);
        return 1;
    }
    fprintf(stderr, "[backend] Read %zu bytes from gateway.\n", src_len);

    FILE *f = fopen("timings.csv", "w");
    fprintf(f, "TOTAL_US,MEAN_US,MIN_US,MAX_US\n");
    for (size_t i = 0; i < iterations; i++) {
        execute_test_iteration(src, src_len, f);
    }
    fclose(f);

    free(src);

    return 0;
}
