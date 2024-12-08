#include "macros.h"
#include "cpu.h"
#include "rs.h"

IQE make_iqe(void *cpu, Instruction inst)
{
    Cpu *_cpu = (Cpu *)cpu;

    IQE iqe = (IQE){
        .op = inst.op,

        .rd = inst.rd,
        .rs1 = inst.rs1,
        .rs2 = inst.rs2,
        .rs3 = inst.rs3,
        .imm = inst.imm,

        .result_buffer = 0,
        .rs1_value = 0,
        .rs2_value = 0,
        .rs3_value = 0,

        .rs1_valid = false,
        .rs2_valid = false,
        .rs3_valid = false,

        .timestamp = _cpu->cycles,

        .completed = false,
    };

    if (iqe.rs1 != -1)
    {
        iqe.rs1_valid = get_urpf_value(*_cpu, iqe.rs1, &iqe.rs1_value);
    }
    if (iqe.rs2 != -1)
    {
        iqe.rs2_valid = get_urpf_value(*_cpu, iqe.rs2, &iqe.rs2_value);
    }
    if (iqe.rs3 != -1)
    {
        iqe.rs3_valid = get_urpf_value(*_cpu, iqe.rs3, &iqe.rs3_value);
    }

    return iqe;
}

void print_iqe(IQE *iqe) {
	printf("IQE { %s ", get_op_name(iqe->op));

    if (iqe->rd != -1) {
        printf("RD: P%d ", iqe->rd);
    }

	if (iqe->rs1 != -1) {
		if (iqe->rs1_valid) {
			printf("RS1: %d ", iqe->rs1_value);
		} else {
			printf("RS1: P%d ", iqe->rs1);
		}
	}

	if (iqe->rs2 != -1) {
		if (iqe->rs2_valid) {
			printf("RS2: %d ", iqe->rs2_value);
		} else {
			printf("RS2: P%d ", iqe->rs2);
		}
	}

	if (iqe->rs3 != -1) {
		if (iqe->rs3_valid) {
			printf("RS3: %d ", iqe->rs3_value);
		} else {
			printf("RS3: P%d ", iqe->rs3);
		}
	}

    if (iqe->imm != -1) {
        printf("IMM: %d ", iqe->imm);
    }

    printf("}\n");
}

bool iqe_is_ready(IQE iqe) {
	bool result = true;

	if (iqe.rs1 != -1) {
		result &= iqe.rs1_valid;
	}

	if (iqe.rs2 != -1) {
		result &= iqe.rs2_valid;
	}

	if (iqe.rs3 != -1) {
		result &= iqe.rs3_valid;
	}

	return result;
}

bool send_to_irs(Cpu *cpu, IQE *iqe)
{
    if (cpu->irs.len >= IRS_CAPACITY)
        return false;

    cpu->irs.queue[cpu->irs.len] = iqe;
    cpu->irs.len += 1;

    return true;
}

bool send_to_mrs(Cpu *cpu, IQE *iqe)
{
    if (cpu->mrs.len >= MRS_CAPACITY)
        return false;

    cpu->mrs.queue[cpu->mrs.len] = iqe;
    cpu->mrs.len += 1;

    return true;
}

bool send_to_lsq(Cpu *cpu, IQE *iqe)
{
    if (cpu->lsq.len >= LSQ_CAPACITY)
        return false;

    cpu->lsq.queue[cpu->lsq.len] = iqe;
    cpu->lsq.len += 1;

    return true;
}

bool send_to_reservation_station(void *cpu, IQE *iqe)
{
    Cpu *_cpu = (Cpu *)cpu;

    // IQE iqe = make_iqe(_cpu, inst);

    switch (iqe->op)
    {

    case OP_ADD:
    case OP_SUB:
    case OP_AND:
    case OP_OR:
    case OP_XOR:
    case OP_MOVC:
    case OP_BZ:
    case OP_BNZ:
    case OP_ADDL:
    case OP_SUBL:
    case OP_CMP:
    case OP_CML:
    case OP_BP:
    case OP_BN:
    case OP_BNP:
    case OP_JUMP:
    case OP_JALP:
    case OP_RET:
    case OP_HALT:
    case OP_NOP:
    {
        DBG("INFO", "Sent instruction 0x%x to IRS", iqe->op);
        return send_to_irs(_cpu, iqe);
    }

    case OP_LOAD:
    case OP_STORE:
    case OP_LDR:
    case OP_STR:
    {
        DBG("INFO", "Sent instruction 0x%x to LSQ", iqe->op);
        return send_to_lsq(_cpu, iqe);
    }

    case OP_DIV:
    case OP_MUL:
    {
        DBG("INFO", "Sent instruction 0x%x to MRS", iqe->op);
        return send_to_mrs(_cpu, iqe);
    }

    default:
        DBG("ERROR", "Unknown Opcode `0x%x` encountered in `send_to_reservation_station`", iqe->op);
    }

    return false;
}

void queue_remove_entry(IQE **queue, int *len, int index) {
    if (index >= *len) {
        DBG("WARN", "`queue_remove_entry` : Trying to remove item beyond queue length. Len: %d, Index: %d", *len, index);
        return;
    }

    for (int i = index + 1; i < *len; i++) {
        queue[i - 1] = queue[i];
    }

    *len -= 1;
}

bool irs_get_first_ready_iqe(void *cpu, IQE **dest) {
    Cpu *_cpu = (Cpu *)cpu;

    if (_cpu->irs.len == 0) return false;

    for (int i = 0; i < _cpu->irs.len; i++) {
        IQE *iqe = _cpu->irs.queue[i];

        // TODO: Do we need to check uprf as well?
        // Try to get values from fw_uprf
        if (iqe->rs1 != -1) {
            if (_cpu->fw_uprf_valid[iqe->rs1]) {
                iqe->rs1_value = _cpu->fw_uprf[iqe->rs1];
                iqe->rs1_valid = true;
            }
        }
        
        if (iqe->rs2 != -1) {
            if (_cpu->fw_uprf_valid[iqe->rs2]) {
                iqe->rs2_value = _cpu->fw_uprf[iqe->rs2];
                iqe->rs2_valid = true;
            }
        }

        if (iqe->rs3 != -1) {
            if (_cpu->fw_uprf_valid[iqe->rs3]) {
                iqe->rs3_value = _cpu->fw_uprf[iqe->rs3];
                iqe->rs3_valid = true;
            }
        }

        if (iqe_is_ready(*iqe)) {
            *dest = iqe;

            queue_remove_entry(_cpu->irs.queue, &_cpu->irs.len, i);

            return true;
        }
    }

    return false;
}

bool mrs_get_first_ready_iqe(void *cpu, IQE **dest) {
    Cpu *_cpu = (Cpu *)cpu;

    if (_cpu->mrs.len == 0) return false;

    for (int i = 0; i < _cpu->mrs.len; i++) {
        IQE *iqe = _cpu->mrs.queue[i];
    
        // TODO: Do we need to check uprf as well?
        // Try to get values from fw_uprf
        if (iqe->rs1 != -1) {
            if (_cpu->fw_uprf_valid[iqe->rs1]) {
                iqe->rs1_value = _cpu->fw_uprf[iqe->rs1];
                iqe->rs1_valid = true;
            }
        }
        
        if (iqe->rs2 != -1) {
            if (_cpu->fw_uprf_valid[iqe->rs2]) {
                iqe->rs2_value = _cpu->fw_uprf[iqe->rs2];
                iqe->rs2_valid = true;
            }
        }

        if (iqe->rs3 != -1) {
            if (_cpu->fw_uprf_valid[iqe->rs3]) {
                iqe->rs3_value = _cpu->fw_uprf[iqe->rs3];
                iqe->rs3_valid = true;
            }
        }

        // If IQE is ready move it out
        if (iqe_is_ready(*iqe)) {
            *dest = iqe;

            queue_remove_entry(_cpu->mrs.queue, &_cpu->mrs.len, i);

            return true;
        }
    }

    return false;
}

bool lsq_get_first_ready_iqe(void *cpu, IQE **dest) {
    Cpu *_cpu = (Cpu *)cpu;

    if (_cpu->lsq.len == 0) return false;

    for (int i = 0; i < _cpu->lsq.len; i++) {
        IQE *iqe = _cpu->lsq.queue[i];

        // TODO: Do we need to check uprf as well?
        // Try to get values from fw_uprf
        if (iqe->rs1 != -1) {
            if (_cpu->fw_uprf_valid[iqe->rs1]) {
                iqe->rs1_value = _cpu->fw_uprf[iqe->rs1];
                iqe->rs1_valid = true;
            }
        }
        
        if (iqe->rs2 != -1) {
            if (_cpu->fw_uprf_valid[iqe->rs2]) {
                iqe->rs2_value = _cpu->fw_uprf[iqe->rs2];
                iqe->rs2_valid = true;
            }
        }

        if (iqe->rs3 != -1) {
            if (_cpu->fw_uprf_valid[iqe->rs3]) {
                iqe->rs3_value = _cpu->fw_uprf[iqe->rs3];
                iqe->rs3_valid = true;
            }
        }

        if (iqe_is_ready(*iqe)) {
            *dest = iqe;

            queue_remove_entry(_cpu->lsq.queue, &_cpu->lsq.len, i);

            return true;
        }
    }

    return false;
}

void update_iqe_with_forwarded(IQE *iqe, int phy_reg, int reg_value) {
	if (iqe->rs1 == phy_reg) {
		iqe->rs1_value = reg_value;
		iqe->rs1_valid = true;
	}
	if (iqe->rs2 == phy_reg) {
		iqe->rs2_value = reg_value;
		iqe->rs2_valid = true;
	}
	if (iqe->rs3 == phy_reg) {
		iqe->rs3_value = reg_value;
		iqe->rs3_valid = true;
	}
}

void irs_send_forwarded_register(IRS *irs, int phy_reg, int reg_value) {
	for (int i = 0; i < irs->len; i++) {
		IQE *iqe = irs->queue[i];

		update_iqe_with_forwarded(iqe, phy_reg, reg_value);
	}
}

void mrs_send_forwarded_register(MRS *mrs, int phy_reg, int reg_value) {
	for (int i = 0; i < mrs->len; i++) {
		IQE *iqe = mrs->queue[i];

		update_iqe_with_forwarded(iqe, phy_reg, reg_value);
	}
}

void lsq_send_forwarded_register(LSQ *lsq, int phy_reg, int reg_value) {
	for (int i = 0; i < lsq->len; i++) {
		IQE *iqe = lsq->queue[i];

		update_iqe_with_forwarded(iqe, phy_reg, reg_value);
	}
}
