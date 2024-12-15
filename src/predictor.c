#include "predictor.h"
#include <string.h>

void add_predictor_entry(Predictor *p, int pc, int op, int target) {
    // If predictorEntry already exists for this pc, then only need to update it...
    for (int i = 0; i < p->len; i++) {
        PredictorEntry entry_i = p->table[i];

        if (entry_i.pc == pc) {
            p->table[i].target_address = target;
            return;
        }
    }

    // If does not exist then add a new entry
    if (p->len == PREDICTOR_TABLE_SIZE) {
        // Remove first item
        memmove(p->table, p->table + 1, sizeof(PredictorEntry) * (PREDICTOR_TABLE_SIZE - 1));
        p->len -= 1;
    }

    PredictorEntryType type = 0;
    switch (op) {
        case OP_BZ:
        case OP_BNZ:
        case OP_BP:
        case OP_BN:
        case OP_BNP: {
            type = TYPE_BRANCH;
            break;
        }

        case OP_JALP: {
            type = TYPE_JALP;
            break;
        }

        case OP_RET: {
            type = TYPE_RET;
            break;
        }

        default: {
            DBG("ERROR", "Invalid instruction send to predictor. %s.", get_op_name(op));
            exit(1);
        }
    }

    p->table[p->len] = (PredictorEntry) {
        .pc = pc,
        .type = type,
        .target_address = target,
    };

    p->len += 1;
}

void update_predictor_entry(Predictor *p, int pc, int target) {
    for (int i = 0; i < p->len; i++) {
        PredictorEntry entry_i = p->table[i];

        if (entry_i.pc == pc) {
            p->table[i].target_address = target;
            return true;
        }
    }
}

bool get_prediction(Predictor *p, int pc, PredictorEntry *entry) {
    for (int i = 0; i < p->len; i++) {
        PredictorEntry entry_i = p->table[i];

        if (entry_i.pc == pc) {
            *entry = entry_i;
            return true;
        }
    }

    // TODO: Default prediction

    return false;
}