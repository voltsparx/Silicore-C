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
} PluginSpec;

typedef struct {
    const char* json_context;
    int json_length;
} PluginContext;

typedef struct {
    char* json_output;
    int json_length;
    int severity;
} PluginOutput;

const PluginSpec* silicore_plugin_spec(void);
PluginOutput silicore_plugin_run(const PluginContext* ctx);
void silicore_plugin_free_output(PluginOutput* out);

#ifdef __cplusplus
}
#endif
