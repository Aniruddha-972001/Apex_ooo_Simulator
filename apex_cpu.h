#pragma once

typedef struct UCRF{
    int value;  //value store in the condition register
    bool valid; // indicates if value is valid
    bool free;  //indicates if register is free
    bool pos_flag;
    bool neg_flag;
    bool zero_flag;
}UCRF;

typedef struct UPRF{
    int value; //value stored in physical register
    bool valid; //indicates if value is valid (free to use)
    bool free; //indicated if reg is free and available to use
    int logical_reg; //mapping to the logical register (-1 if unallocated)
}UPRF;

//TODO FRONTEND RENAME TABLE AND BACKEND RENAME TABLE
typedef struct Rename_table{
    int physical_reg[32];
    bool valid[32];
}Rename_table;

typedef struct IRS{
    int pc;
    int opcode;
    bool valid;     //validity of instruction in queue 
    bool executed;  // whether instruction has executed 
}IRS;

typedef struct MRS{
    int pc;
    int opcode;
    bool valid;     //validity of instruction in queue 
    bool executed;  // whether instruction has executed 
}MRS;

typedef struct LSQ{
    int pc;
    int opcode;
    bool valid;     //validity of instruction in queue 
    bool executed;  // whether instruction has executed 
}LSQ;

typedef struct ROB{
    int pc;
    int opcode;
    bool valid;     //validity of instruction in queue 
    bool executed;  // whether instruction has executed 
}ROB;

typedef struct CPU_Stage
{
    int pc;
    int opcode;
    int rs1;
    int rs2;
    int rs3;
    int rs1_value;
    int rs2_value;
    int rs3_value;
    int imm;
    int rd;
    int rd_value;
    int has_inst;
    int result_buffer;
    int mem_address;
} CPU_Stage;

typedef struct APEX_CPU{
    UCRF ucrf[10];
    UPRF uprf[60];
    Rename_table frontend_RT;
    Rename_table backend_RT;
    CPU_Stage fetch;
    CPU_Stage D1;
    CPU_Stage D2;
    CPU_Stage execute;
    CPU_Stage memory;
    CPU_Stage commit;
    IRS irs[8];
    MRS mrs[2];
    LSQ lsq[6];
    ROB rob[80];
}APEX_CPU;
