#include "rename.h"
#include "macros.h"

void fl_push(FreeList *fl, int item) {
    if (fl->len == FREE_LIST_CAPACITY) {
        DBG("ERROR", "Tried to push item into a full FreeList. %c", ' ');
        return;
    }

    fl->len += 1;
    fl->data[fl->tail] = item;
    fl->tail = (fl->tail + 1) % FREE_LIST_CAPACITY;
}

int fl_pop(FreeList *fl) {
    if (fl->len == 0) {
        DBG("ERROR", "Tried to pop item from an empty free list. %c", ' ');
        return -1;
    }

    fl->len -= 1;
    int item = fl->data[fl->head];
    fl->head = (fl->head + 1) % FREE_LIST_CAPACITY;

    return item;
}

RenameTable initialize_rename_table() {
    RenameTable rt = {0};

    for (int i = 0; i < PHYS_REGS_COUNT; i++) {
        if (i < ARCH_REGS_COUNT) {
            rt.table[i] = i;
        } else {
            fl_push(&rt.fl, i);
        }
    }

    return rt;
}

int map_source_register(RenameTable *rt, int arch) {
    if (arch >= ARCH_REGS_COUNT) {
        DBG("ERROR", "Invalid architectural register %d", arch);
        return -1;
    }
    
    return rt->table[arch];
}

int map_dest_register(RenameTable *rt, int arch) {
    if (arch >= ARCH_REGS_COUNT) {
        DBG("ERROR", "Invalid architectural register %d", arch);
        return -1;
    }

    int old_mapping = rt->table[arch];
    rt->table[arch] = fl_pop(&rt->fl);
    fl_push(&rt->fl, old_mapping);

    return rt->table[arch];
}