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

bool send_to_irs(Cpu *cpu, IQE iqe)
{
    if (cpu->irs.len >= IRS_CAPACITY)
        return false;

    cpu->irs.queue[cpu->irs.len] = iqe;
    cpu->irs.len += 1;

    return true;
}

bool send_to_mrs(Cpu *cpu, IQE iqe)
{
    if (cpu->mrs.len >= MRS_CAPACITY)
        return false;

    cpu->mrs.queue[cpu->mrs.len] = iqe;
    cpu->mrs.len += 1;

    return true;
}

bool send_to_lsq(Cpu *cpu, IQE iqe)
{
    if (cpu->lsq.len >= LSQ_CAPACITY)
        return false;

    cpu->lsq.queue[cpu->lsq.len] = iqe;
    cpu->lsq.len += 1;

    return true;
}

bool send_to_reservation_station(void *cpu, Instruction inst)
{
    Cpu *_cpu = (Cpu *)cpu;

    IQE iqe = make_iqe(_cpu, inst);

    switch (iqe.op)
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
    {
        DBG("INFO", "Sent instruction 0x%x to IRS", iqe.op);
        return send_to_irs(_cpu, iqe);
    }

    case OP_HALT:
    case OP_NOP:
    {
        // TODO: Where do these go?
        DBG("WARN", "Where to send `HALT` and `NOP` in `send_to_reservation_station`", iqe.op);

        break;
    }

    case OP_LOAD:
    case OP_STORE:
    case OP_LDR:
    case OP_STR:
    {
        DBG("INFO", "Sent instruction 0x%x to LSQ", iqe.op);
        return send_to_lsq(_cpu, iqe);
    }

    case OP_DIV:
    case OP_MUL:
    {
        DBG("INFO", "Sent instruction 0x%x to MRS", iqe.op);
        return send_to_mrs(_cpu, iqe);
    }

    default:
        DBG("ERROR", "Unknown Opcode `0x%x` encountered in `send_to_reservation_station`", iqe.op);
    }

    return false;
}