#pragma once

#include "cpu_settings.h"
#include "rs.h"

typedef struct {
	IQE queue[ROB_CAPACITY];
	int len;
} Rob;

// Function to remove first item if it is completed
bool rob_get_completed(Rob *rob, IQE *iqe);
