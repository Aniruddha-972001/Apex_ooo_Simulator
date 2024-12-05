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
} IQE;

// Integer Reservation Station
typedef struct {
    IQE queue[IRS_CAPACITY];
    int len;
} IRS;

// Multiply Reservation Station
typedef struct {
    IQE queue[MRS_CAPACITY];
    int len;
} MRS;

// Load Store Queue
typedef struct {
    IQE queue[LSQ_CAPACITY];
    int len;
} LSQ;

// Function to tell which RS an instruction should go into
bool send_to_reservation_station(void *cpu, Instruction inst);

// Function to create an IQE from an Instruction
IQE make_iqe(void *cpu, Instruction inst);

// TODO: Functions to retrieve first ready instruction

// TODO: Functions to send forwarded data to each RS