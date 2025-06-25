#include <float.h>
#include <stdio.h>
#include <math.h>

#include "plugin.h"
#include "time.h"


typedef struct operator_s {
    uint8_t type;
    uint64_t size;
} operator;

enum operator_type {
    sum,
    max,
    min,
    avg
};

const char* input_key_name = "input";
const char* config_type_key = "aggregate_type";
const char* config_length_key = "data_length";

int print_element(void* const item, struct hashmap_element_s* const element) {
    if (!element) {
        printf("null}");
        return 1;
    }

    const char* key = element->key;
    printf("key(%s): ", key);
    type_value_t* value = element->data;

    switch (value->type) {
        case FLOAT:
            printf("float(%f)},", value->value.float_value);
            return 0;
        case INTEGER:
            printf("uint(%ld)},", value->value.integer_value);
            return 0;
        case STRING:
            printf("string(%s)},", value->value.str_value);
            return 0;
        case BOOLEAN:
            if (value->value.boolean_value > 0) {
                printf("bool(true)},");
            }
            else {
                printf("bool(false)},");
            }

            return 0;
    }

    return 1;
}

void print_hashmap(struct hashmap_s* map) {
    hashmap_iterate_pairs(map, print_element, NULL);
}

void print_dataset(dataset_t* data) {
    if (!data) {
        return;
    }

    printf("datapoints size: %d {", data->datapoint_size);
    for (size_t i = 0; i < data->datapoint_size; ++i) {
        if (data->datapoints == NULL || data->datapoints[i].map == NULL) {
            continue;
        }
        printf("{ts: %lu, ", data->datapoints[i].ts);
        print_hashmap(data->datapoints[i].map);
    }
    printf("}\n");
}

int free_map_element(void* const item, void* const data) {
    if (!data) {
        return 0;
    }

    hashmap_element_t* element = (hashmap_element_t*)data;

    if (element->key == NULL) {
        return 0;
    }

    if (element->key_len <= 0) {
        return 1;
    }

    type_value_t* value = element->data;
    if (value == NULL) {
        return 0;
    }

    if (value->type == STRING) {
        if (value->value.str_value != NULL) {
            free((void*)value->value.str_value);
            value->value.str_value = NULL;
        }
        free(value);
        value = NULL;

        return 0;
    }

    return 0;
}

void free_dataset_t(dataset_t* ds) {
    if (ds == NULL) {
        return;
    }

    if (ds->datapoints != NULL) {
        for (size_t i = 0; i < ds->datapoint_size; ++i) {
            if (ds->datapoints[i].map != NULL) {
                if (ds->datapoints[i].map->data != NULL) {
                    hashmap_iterate(ds->datapoints[i].map, free_map_element, NULL);
                    hashmap_destroy(ds->datapoints[i].map);
                    if (ds->datapoints[i].map != NULL) {
                        free(ds->datapoints[i].map);
                        ds->datapoints[i].map = NULL;
                    }
                }
            }
        }

        free(ds->datapoints);
        ds->datapoints = NULL;
    }

    free(ds);
    ds = NULL;
}

dataset_t* create_dataset_with_inputs(const float* input_data, size_t num_datapoints) {
    if (!input_data && num_datapoints > 0) {
        fprintf(stderr, "Error: input_data is NULL but num_datapoints is greater than 0.\n");
        return NULL;
    }
    if (num_datapoints == 0) {
        dataset_t* empty_dataset = malloc(sizeof(dataset_t));
        if (!empty_dataset) {
            fprintf(stderr, "Error: Failed to allocate memory for empty dataset_t");
            return NULL;
        }
        memset(empty_dataset, 0, sizeof(dataset_t));
        empty_dataset->datapoint_size = 0;
        empty_dataset->datapoints = NULL;
        return empty_dataset;
    }

    dataset_t* new_dataset = malloc(sizeof(dataset_t));
    if (!new_dataset) {
        fprintf(stderr, "Error: Failed to allocate memory for dataset_t");
        return NULL;
    }
    memset(new_dataset, 0, sizeof(dataset_t));

    new_dataset->datapoint_size = 0;
    new_dataset->datapoints = NULL;

    new_dataset->datapoints = malloc(num_datapoints * sizeof(datapoint_t));
    if (!new_dataset->datapoints) {
        fprintf(stderr, "Error: Failed to allocate memory for datapoints array");
        free(new_dataset);
        new_dataset = NULL;

        return NULL;
    }
    memset(new_dataset->datapoints, 0, num_datapoints * sizeof(dataset_t));
    new_dataset->datapoint_size = num_datapoints;

    for (size_t i = 0; i < num_datapoints; ++i) {
        new_dataset->datapoints[i].map = NULL;
    }

    size_t input_key_len = strlen(input_key_name);

    for (size_t i = 0; i < num_datapoints; i++) {
        struct hashmap_s* current_map = malloc(sizeof(struct hashmap_s));
        if (!current_map) {
            fprintf(stderr, "Error: Failed to allocate memory for hashmap_s");
            free_dataset_t(new_dataset);
            return NULL;
        }
        memset(current_map, 0, sizeof(struct hashmap_s));
        new_dataset->datapoints[i].map = current_map;

        // Assuming hashmap_create returns 0 on success
        if (hashmap_create(1, current_map) != 0) {
            fprintf(stderr, "Error: hashmap_create failed for datapoint %zu\n", i);
            // current_map is already assigned, dataset_destroy will free it
            free_dataset_t(new_dataset);
            return NULL;
        }

        type_value_t* current_val = malloc(sizeof(type_value_t));
        if (!current_val) {
            fprintf(stderr, "Error: Failed to allocate memory for type_value_t");
            free_dataset_t(new_dataset);
            return NULL;
        }
        current_val->type = FLOAT;
        current_val->value.float_value = input_data[i];

        if (hashmap_put(current_map, input_key_name, input_key_len, current_val) != 0) {
            fprintf(stderr, "Error: hashmap_put failed for datapoint %zu\n", i);
            free(current_val);
            current_val = NULL;
            free_dataset_t(new_dataset);
            return NULL;
        }

        new_dataset->datapoints[i].ts = (long long)i;
    }

    return new_dataset;
}

int init() {
    return 0;
}

void* on_open(hashmap_t* map) {
    operator* opt = malloc(sizeof(operator));
    const char* opt_type = hashmap_get(map, config_type_key, strlen(config_type_key));

    int* size = hashmap_get(map, config_length_key, strlen(config_length_key));
    if (size == NULL) {
        fprintf(stderr, "Error: hashmap_get failed for operator %zu\n", strlen(config_length_key));
        return NULL;
    }

    opt->size = (uint64_t)(*size);
    if (strcmp("sum", opt_type) == 0) {
        opt->type = sum;
    }
    else if (strcmp("max", opt_type) == 0) {
        opt->type = max;
    }
    else if (strcmp("min", opt_type) == 0) {
        opt->type = min;
    }
    else if (strcmp("avg", opt_type) == 0) {
        opt->type = avg;
    }
    else {
        fprintf(stderr, "Error: unknown option %s\n", opt_type);
        return NULL;
    }

    return opt;
}

float* dataset_to_input_data(const dataset_t* ds, const char* input_name) {
    if (ds == NULL) {
        fprintf(stderr, "Input dataset empty\n");
        return NULL;
    }

    size_t count = 0;
    for (size_t i = 0; i < ds->datapoint_size; ++i) {
        count += hashmap_num_entries(ds->datapoints->map);
    }
    if (count == 0) {
        fprintf(stderr, "Failed to get dataset data for input %s\n", input_name);
        return NULL;
    }

    float* input = (float*)malloc(ds->datapoint_size * sizeof(float));
    if (input == NULL) {
        return NULL;
    }
    memset(input, 0, ds->datapoint_size * sizeof(float));
    for (size_t i = 0; i < ds->datapoint_size; ++i) {
        type_value_t* val = hashmap_get(ds->datapoints[i].map, input_name, strlen(input_name));
        if (val == NULL) {
            printf("not found %s\n", input_name);
            continue;
        }
        input[i] = (float)val->value.float_value;
    }

    return input;
}

float sum_input(float* input_data, uint64_t size) {
    if (input_data == NULL) {
        return 0;
    }

    float sum = 0;
    for (size_t i = 0; i < size; ++i) {
        if (input_data[i]) {
            sum += input_data[i];
        }
    }

    return sum;
}

float avg_input(float* input_data, uint64_t size) {
    if (input_data == NULL) {
        return 0;
    }

    float sum = sum_input(input_data, size);
    float avg = sum / (float)size;

    return avg;
}

float max_input(float* input_data, uint64_t size) {
    if (input_data == NULL) {
        return 0;
    }
    float max = FLT_MIN;

    for (int i = 0; i < size; ++i) {
        if (max < input_data[i]) {
            max = input_data[i];
        }
    }

    return max;
}

float min_input(float* input_data, uint64_t size) {
    if (input_data == NULL) {
        return 0;
    }
    float min = FLT_MAX;

    for (int i = 0; i < size; ++i) {
        if (min > input_data[i]) {
            min = input_data[i];
        }
    }

    return min;
}

void on_data(void* handle, const dataset_t* ds, dataset_t* output) {
    if (handle == NULL) {
        fprintf(stderr, "Error: handle is NULL\n");
        return;
    }

    operator* opt = (operator*)handle;
    if (opt == NULL) {
        fprintf(stderr, "Error: opt is NULL\n");
        return;
    }

    float ret = 0;
    float* input_data = dataset_to_input_data(ds, input_key_name);
    switch (opt->type) {
        case sum:
            ret = sum_input(input_data, opt->size);
            break;
        case avg:
            ret = avg_input(input_data, opt->size);
            break;
        case min:
            ret = min_input(input_data, opt->size);
            break;
        case max:
            ret = max_input(input_data, opt->size);
            break;
        default:
            fprintf(stderr, "Error: opt type not found\n");
            return;
    }

    if (output == NULL) {
        output = malloc(sizeof(dataset_t));
    }
    output->datapoint_size = 1;
    output->datapoints = malloc(output->datapoint_size * sizeof(datapoint_t));

    output->datapoints->ts = get_millis_timestamp();
    output->datapoints->map = malloc(sizeof(hashmap_t));
    hashmap_create(10, output->datapoints->map);
    type_value_t* result = malloc(sizeof(type_value_t));
    result->type = FLOAT;
    result->value.float_value = ret;
    hashmap_put(output->datapoints->map, "result", 6, result);
}

void on_close(void* handle) {
    if (handle == NULL) {
        return;
    }

    operator* opt = (operator*)handle;
    if (opt == NULL) {
        return;
    }

    free(opt);
    opt = NULL;
}

void deinit() {
}

int main(void) {
    if (init() != 0) {
        fprintf(stderr, "Failed to initialize the application. Exiting.\n");
        return 1;
    }
    printf("init successful\n");

    int input_size = 140;
    float input_data[140] = {
            -0.39, 0.0, 0.01, -0.17, 0.42, -0.43, -0.34, -0.80, -0.73, -0.80, -1.42, -0.37, -0.17, 1.73, -0.36, 0.0,
            -0.00,
            -0.18, 0.49, -0.43, -0.37, -0.61, -0.58, -0.61, -1.64, 0.44, 0.10, 1.72, -0.32, 0.0, 0.20, -0.18, 0.58,
            -0.41,
            -0.37, -0.46, -0.43, -0.46, -1.43, 0.33, 0.09, 1.72, -0.30, 0.0, 0.14, -0.18, 0.24, -0.41, -0.38, -0.37,
            -0.38,
            -0.37, -1.68, 0.24, 0.04, 1.72, -0.34, 0.0, 0.06, -0.17, 0.32, -0.41, -0.36, -0.54, -0.51, -0.54, -1.75,
            0.35,
            0.06, 1.72, -0.38, 0.0, 0.19, -0.17, 0.52, -0.44, -0.37, -0.79, -0.72, -0.79, -1.55, -0.25, 0.22, 1.72,
            -0.36,
            0.0, -0.34, -0.18, 0.31, -0.40, -0.38, -0.29, -0.33, -0.29, -1.49, -0.67, 0.15, 1.72, -0.37, 0.0, 0.16,
            -0.17,
            0.29, -0.42, -0.35, -0.60, -0.57, -0.60, -1.60, 0.74, 0.15, 1.72, -0.28, 0.0, -0.32, -0.17, 0.83, -0.39,
            -0.34,
            -0.44, -0.43, -0.44, -1.62, 0.10, -0.05, 1.71, -0.30, 0.0, 0.11, -0.17, 0.24, -0.37, -0.35, -0.24, -0.27,
            -0.24,
            -1.71, 0.05, 0.05, 1.71
        };

    const char* types[] = {"sum", "avg", "max", "min"};
    for (size_t i = 0; i < 4; ++i) {
        dataset_t* input = create_dataset_with_inputs(input_data, input_size);
        if (!input) {
            fprintf(stderr, "Main: Failed to create input dataset for iteration %zu. Skipping.\n", i);
            continue;
        }

        hashmap_t* config_map = malloc(sizeof(hashmap_t));
        hashmap_create(10, config_map);
        hashmap_put(config_map, config_type_key, strlen(config_type_key), types[i]);
        hashmap_put(config_map, config_length_key, strlen(config_length_key), &input_size);
        void* instance = on_open(config_map);
        if (!instance) {
            fprintf(stderr, "Main: on_open failed for iteration %zu. Skipping.\n", i);
            free_dataset_t(input);
            continue;
        }

        for (size_t j = 0; j < 3; ++j) {
            dataset_t* output = malloc(sizeof(dataset_t));
            if (!output) {
                fprintf(stderr, "Main: Failed to allocate memory for output dataset for iteration %zu. Skipping.\n", i);
                free_dataset_t(input);
                continue;
            }
            output->datapoint_size = 0;
            output->datapoints = NULL;

            printf("call on_data %ld/%ld\n", i, j);
            on_data(instance, input, output);

            printf("call on_data %ld/%ld result\n", i, j);
            print_dataset(output);
            printf("call on_data %ld/%ld done\n", i, j);
        }

        on_close(instance);

        free_dataset_t(input);

        fflush(stdout);
    }

    deinit();
}
