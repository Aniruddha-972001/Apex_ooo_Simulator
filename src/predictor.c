#include "predictor.h"
#include "macros.h"
#include <string.h>
#include <stdlib.h>

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
            return;
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

    return false;
}


void push_return_address(Predictor *p, int target) {
    if (p->rs.len == RETURN_STACK_SIZE) {
        DBG("ERROR", "Tried to push into a full return stack. %c", ' ');
        exit(1);
    }

    p->rs.return_addresses[p->rs.len] = target;
    p->rs.len += 1;
}

int pop_return_address(Predictor *p) {
    if (p->rs.len == 0) {
        DBG("ERROR", "Tried to pop out of an empty return stack. %c", ' ');
        exit(1);
    }

    p->rs.len -= 1;
    return p->rs.return_addresses[p->rs.len];
}

void print_predictor(Predictor p) {
    printf("Predictor: [\n");
    for (int i = 0; i < p.len; i++) {
        PredictorEntry entry = p.table[i];

        printf("    PredictorEntry { pc: %d, target: %d }\n", entry.pc, entry.target_address);
    }
    printf("    ]\n");

    ReturnStack rs = p.rs;
    printf("Return Stack: [\n");
    for (int i = 0; i < rs.len; i++) {
        printf("    Return { pc: %d }\n", rs.return_addresses[i]);
    }
    printf("    ]\n");
}