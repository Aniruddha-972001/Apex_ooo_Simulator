#pragma once

#include "cpu_settings.h"
#include "cpu_structs.h"
#include "rename.h"
#include "predictor.h"

typedef struct {
    int fw_uprf_valid[PHYS_REGS_COUNT]; // Forwarded registers valid bits
    int fw_uprf[PHYS_REGS_COUNT];       // Forwarded registers

    int fw_ucrf_valid[CC_REGS_COUNT];   // Forwarded CC registers valid bits
    Cc fw_ucrf[CC_REGS_COUNT];          // Forwarded CC registers

    RenameTable rt;                     // RenameTable and FreeList

    Predictor p;                        // Predictor
} BisEntry;