#pragma once

#include <stdio.h>
#include <stdlib.h>
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "hashmap.h"

typedef enum data_type
{
    FLOAT,
    INTEGER,
    STRING,
    BOOLEAN
} data_type;

typedef union data_value {
    double float_value;
    uint64_t integer_value;
    const char * str_value;
    uint8_t boolean_value;
} data_value;

static inline double get_float_data_value(data_value dv)
{
    return dv.float_value;
}

static inline uint64_t get_int_data_value(data_value dv)
{
    return dv.integer_value;
}

static inline const char * get_string_data_value(data_value dv)
{
    return dv.str_value;
}

static inline uint8_t get_bool_data_value(data_value dv)
{
    return dv.boolean_value;
}

static inline int set_float_data_value(data_value * dv, double val)
{
    if (dv == NULL) {
        return -1;
    }

    dv->float_value = val;
    return 0;
}

static inline int set_int_data_value(data_value * dv, uint64_t val)
{
    if (dv == NULL) {
        return -1;
    }

    dv->integer_value = val;
    return 0;
}

static inline int set_string_data_value(data_value * dv, const char * val)
{
    if (dv == NULL) {
        return -1;
    }

    dv->str_value = val;
    return 0;
}

static inline int set_bool_data_value(data_value * dv, uint8_t val)
{
    if (dv == NULL) {
        return -1;
    }

    dv->boolean_value = val;
    return 0;
}

typedef struct type_value_s
{
    data_value value;
    data_type type;
} type_value_t;

typedef struct datapoint_s
{
    uint64_t ts;
    // key is string, value is value_type *
    struct hashmap_s * map;
} datapoint_t;

static inline void get_float_from_dp(datapoint_t dp, const char * tag, float * value)
{
    if (dp.map == NULL) {
        printf("hashmap is empty");
        return;
    }

    type_value_t * tv = hashmap_get(dp.map, tag, strlen(tag));
    /* if (tv == NULL) {
        printf("tag value not found\n");
        return;
    } */

    *value = 0.1;
}

typedef struct dataset_s
{
    int datapoint_size;
    datapoint_t * datapoints;
} dataset_t;

static inline float * get_float(const dataset_t * ds, const char * tag, int * size)
{
    float * float_data = malloc(sizeof(float) * ds->datapoint_size);
    *size = ds->datapoint_size;

    for (int i = 0; i < ds->datapoint_size; i++) {
        float_data[i] = 0.1;
    }

    return float_data;
}

static inline void append_string_to_dp(datapoint_t * dp, const char * tag_name, char * value)
{
    // ds->datapoint_size = 1;
    // datapoint_t * datapoints = NULL;
    //  ds->datapoints = realloc(void * ptr, size_t size);
}

static inline void append_string(dataset_t * ds, const char * tag_name)
{
    ds->datapoint_size = 1;
    datapoint_t * datapoints = NULL;
    // ds->datapoints = realloc(void * ptr, size_t size);
}

#define VERSION_1 1

typedef struct plugin_s
{
    int version;
    const char * name;
    const char * ui_params;
    int (*init)();
    void (*deinit)();
    void * (*on_open)(struct hashmap_s * cfg);
    void (*on_data)(void * instance, dataset_t * input, dataset_t * output);
} plugin_t;

int init();
void deinit();
void * on_open(hashmap_t * map);
void on_close(void * handle);
void on_data(void * handle, const dataset_t * ds, dataset_t * output);
const char * get_ui_params();
const char* get_mcp_config();
const char * get_name();
int get_version();

#ifdef __cplusplus
}
#endif