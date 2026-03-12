#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char* id;
    const char* title;
    const char* version;
    const char* scopes;
} FilterSpec;

typedef struct {
    const char* json_context;
    int json_length;
} FilterContext;

typedef struct {
    char* json_output;
    int json_length;
    int severity;
} FilterOutput;

const FilterSpec* silicore_filter_spec(void);
FilterOutput silicore_filter_run(const FilterContext* ctx);
void silicore_filter_free_output(FilterOutput* out);

#ifdef __cplusplus
}
#endif
