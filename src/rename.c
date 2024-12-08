#include "rename.h"
#include "macros.h"

void fl_push(FreeList *uprf_fl, int item) {
    if (uprf_fl->len == FREE_LIST_CAPACITY) {
        DBG("ERROR", "Tried to push item into a full FreeList. %c", ' ');
        return;
    }

    uprf_fl->len += 1;
    uprf_fl->data[uprf_fl->tail] = item;
    uprf_fl->tail = (uprf_fl->tail + 1) % FREE_LIST_CAPACITY;
}

int fl_pop(FreeList *uprf_fl) {
    if (uprf_fl->len == 0) {
        DBG("ERROR", "Tried to pop item from an empty free list. %c", ' ');
        return -1;
    }

    uprf_fl->len -= 1;
    int item = uprf_fl->data[uprf_fl->head];
    uprf_fl->head = (uprf_fl->head + 1) % FREE_LIST_CAPACITY;

    return item;
}

RenameTable initialize_rename_table() {
    RenameTable rt = {0};

    for (int i = 0; i < PHYS_REGS_COUNT; i++) {
        if (i < ARCH_REGS_COUNT) {
            rt.table[i] = i;
        } else {
            fl_push(&rt.uprf_fl, i);
        }
    }

    rt.cc = 0;
    for (int i = 1; i < CC_REGS_COUNT; i++) {
        fl_push(&rt.ucrf_fl, i);
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
    rt->table[arch] = fl_pop(&rt->uprf_fl);
    fl_push(&rt->uprf_fl, old_mapping);

    return rt->table[arch];
}

int map_cc_register(RenameTable *rt) {
    int old_mapping = rt->cc;
    rt->cc = fl_pop(&rt->ucrf_fl);
    fl_push(&rt->ucrf_fl, old_mapping);

    return rt->cc;
}

int get_cc_register(RenameTable *rt) {
    return rt->cc;
}