#pragma once

#include <stdbool.h>

#include "cpu_settings.h"
#include "instruction.h"

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

void add_predictor_entry(Predictor *p, int pc, int op, int target);
void update_predictor_entry(Predictor *p, int pc, int target);
bool get_prediction(Predictor *p, int pc, PredictorEntry *entry);

void push_return_address(Predictor *p, int target);
int pop_return_address(Predictor *p);