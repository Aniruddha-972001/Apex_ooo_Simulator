#pragma once

#include <stdbool.h>

#include "cpu_settings.h"
#include "instruction.h"
#include "cpu_structs.h"

void add_predictor_entry(Predictor *p, int pc, int op, int target);
void update_predictor_entry(Predictor *p, int pc, int target);
bool get_prediction(Predictor *p, int pc, PredictorEntry *entry);

void push_return_address(Predictor *p, int target);
int pop_return_address(Predictor *p);

void print_predictor(Predictor p);