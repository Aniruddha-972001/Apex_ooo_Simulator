#pragma once

#include "cpu_settings.h"
#include "rs.h"

typedef struct RobNodeStruct {
	IQE iqe;
	struct RobNodeStruct *next;
} RobNode;

typedef struct {
	RobNode *head;
	int len;
} Rob;

// Function to remove first item if it is completed
bool rob_get_completed(Rob *rob, IQE *iqe);

// Function to add an IQE to ROB
IQE *rob_push_iqe(Rob *rob, IQE iqe);
