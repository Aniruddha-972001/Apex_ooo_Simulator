#pragma once

#include <stdbool.h>
#include "cpu_settings.h"

typedef struct {
    bool z, n, p;
} Cc;

typedef struct {
    int return_addresses[RETURN_STACK_SIZE];
    int len;
} ReturnStack;

typedef enum {
    TYPE_BRANCH,
    TYPE_JALP,
    TYPE_RET,
} PredictorEntryType;

typedef struct {
    PredictorEntryType type;
    int target_address;
    int pc;
} PredictorEntry;

typedef struct {
    PredictorEntry table[PREDICTOR_TABLE_SIZE];
    int len;

    ReturnStack rs;
} Predictor;