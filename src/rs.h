#pragma once

#include <stdbool.h>

#include "cpu_settings.h"
#include "instruction.h"

// Instruction Queue Entry
typedef struct {
    int op; // Opcode

    int rd;     // Register Index
    int rs1;    // Register Index
    int rs2;    // Register Index
    int rs3;    // Register Index
    int imm;    // Register Index

    int result_buffer;  // Actual values
    int rs1_value;      // Actual values
    int rs2_value;      // Actual values
    int rs3_value;      // Actual values

    bool rs1_valid;
    bool rs2_valid;
    bool rs3_valid;

    size_t timestamp;   // Cycle number

    bool completed;     // Execution completed
} IQE;

// Integer Reservation Station
typedef struct {
    IQE *queue[IRS_CAPACITY];
    int len;
} IRS;

// Multiply Reservation Station
typedef struct {
    IQE *queue[MRS_CAPACITY];
    int len;
} MRS;

// Load Store Queue
typedef struct {
    IQE *queue[LSQ_CAPACITY];
    int len;
} LSQ;

// Function to tell which RS an instruction should go into
bool send_to_reservation_station(void *cpu, IQE *iqe);

// Function to create an IQE from an Instruction
IQE make_iqe(void *cpu, Instruction inst);

// Functions to retrieve first ready instruction
bool irs_get_first_ready_iqe(void *cpu, IQE **dest);
bool mrs_get_first_ready_iqe(void *cpu, IQE **dest);
bool lsq_get_first_ready_iqe(void *cpu, IQE **dest);

// TODO: Functions to send forwarded data to each RS
