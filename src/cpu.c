#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "cpu.h"
#include "instruction.h"
#include "macros.h"
#include "rename.h"
#include "rob.h"
#include "rs.h"
#include "util.h"
#include "commands.h"

Cpu initialize_cpu(char *asm_file)
{
    InstructionList inst_list =
        parse(asm_file); // We will need to free this later

    Cpu cpu = {0};
    cpu.code = inst_list;
    cpu.pc = 4000;
    cpu.rt = initialize_rename_table();
    cpu.rob.head = NULL;

    memset(&cpu.uprf_valid, 1, sizeof(int) * PHYS_REGS_COUNT);
    memset(&cpu.ucrf_valid, 1, sizeof(int) * CC_REGS_COUNT);

    return cpu;
}

int get_urpf_value(Cpu cpu, int phy_reg, int *dest)
{
    if (phy_reg >= PHYS_REGS_COUNT)
    {
        DBG("ERROR", "Tried to read value of P%d.", phy_reg);
        return false;
    }

    if (cpu.uprf_valid[phy_reg])
    {
        *dest = cpu.uprf[phy_reg];

        return true;
    }

    return false;
}

int get_ucrf_value(Cpu cpu, int cc, Cc *dest)
{
    if (cc >= CC_REGS_COUNT)
    {
        DBG("ERROR", "Tried to read value of C%d.", cc);
        return false;
    }

    if (cpu.ucrf_valid[cc])
    {
        *dest = cpu.ucrf[cc];

        return true;
    }

    return false;
}

void forward_register(Cpu *cpu, int rd, int value)
{
    irs_send_forwarded_register(&cpu->irs, rd, value);
    mrs_send_forwarded_register(&cpu->mrs, rd, value);
    lsq_send_forwarded_register(&cpu->lsq, rd, value);

    cpu->fw_uprf[rd] = value;
    cpu->fw_uprf_valid[rd] = true;
}

void forward_cc_register(Cpu *cpu, int cc, Cc value)
{
    cpu->fw_ucrf_valid[cc] = true;
    cpu->fw_ucrf[cc] = value;
}

void set_cc_flags(IQE *iqe)
{
    iqe->cc_value = (Cc){false, false, false};

    if (iqe->result_buffer == 0)
    {
        iqe->cc_value.z = true;
    }
    else if (iqe->result_buffer > 0)
    {
        iqe->cc_value.p = true;
    }
    else
    {
        iqe->cc_value.n = true;
    }
}

void flush_cpu_after(Cpu *cpu, int timestamp)
{
    cpu->fetch.has_inst = false;
    cpu->decode_1.has_inst = false;
    cpu->decode_2.has_inst = false;

    if (cpu->intFU.has_inst && cpu->intFU.iqe->timestamp > timestamp)
    {
        cpu->intFU.has_inst = false;
    }
    if (cpu->mulFU.has_inst && cpu->mulFU.iqe->timestamp > timestamp)
    {
        cpu->mulFU.has_inst = false;
    }
    if (cpu->memFU.has_inst && cpu->memFU.iqe->timestamp > timestamp)
    {
        cpu->memFU.has_inst = false;
    }

    // Flush IRS, LSQ, MRS
    irs_flush_after(&cpu->irs, timestamp);
    mrs_flush_after(&cpu->mrs, timestamp);
    lsq_flush_after(&cpu->lsq, timestamp);

    // Flush ROB
    rob_flush_after(&cpu->rob, timestamp);
}

void reset_cpu_from_bis(Cpu *cpu, BisEntry bis_entry)
{
    cpu->rt = bis_entry.rt;
    cpu->predictor.rs = bis_entry.p.rs;

    memcpy(cpu->fw_ucrf, bis_entry.fw_ucrf, sizeof(Cc) * CC_REGS_COUNT);
    memcpy(cpu->fw_ucrf_valid, bis_entry.fw_ucrf_valid, sizeof(int) * CC_REGS_COUNT);
    memcpy(cpu->fw_uprf, bis_entry.fw_uprf, sizeof(int) * PHYS_REGS_COUNT);
    memcpy(cpu->fw_uprf_valid, bis_entry.fw_uprf_valid, sizeof(int) * PHYS_REGS_COUNT);
}

// Convert pc from address space to index in instruction list
int pc_to_index(int pc)
{
    assert(pc % 4 == 0 && "Program counter was not valid.");
    assert(pc >= 4000 && "Program counter was less than 4000.");
    return (pc - 4000) / 4;
}

// Fetch stage
void fetch(Cpu *cpu)
{
    // Don't fetch if an instruction already exists in Fetch
    if (cpu->fetch.has_inst)
        return;

    int index = pc_to_index(cpu->pc);

    if (index >= 0 && index < cpu->code.len)
    {
        Instruction inst = cpu->code.data[index];
        inst.pc = cpu->pc;

        cpu->fetch.has_inst = true;
        cpu->fetch.inst = inst;

        cpu->pc += 4; // Go to next instruction
        // Get prediction
        switch (inst.op)
        {
        case OP_BZ:
        case OP_BNZ:
        case OP_BP:
        case OP_BN:
        case OP_BNP:
        {
            PredictorEntry entry = {0};
            bool predicted = get_prediction(&cpu->predictor, inst.pc, &entry);

            if (predicted)
            {
                cpu->pc = entry.target_address;

                // Default prediction
                if (inst.imm < 0)
                {
                    // Negative offset is always taken
                    cpu->pc = inst.pc + inst.imm;
                }
            }

            break;
        }

        case OP_JALP:
        {
            PredictorEntry entry = {0};
            bool predicted = get_prediction(&cpu->predictor, inst.pc, &entry);

            if (predicted)
            {
                cpu->pc = entry.target_address;
                
                push_return_address(&cpu->predictor, inst.pc + 4);
            }

            break;
        }

        case OP_RET:
        {
            // Only if a predictor entry for this RET exists in the predictor table we
            // change CPU pc.
            PredictorEntry entry = {0};
            if (get_prediction(&cpu->predictor, inst.pc, &entry)) {
                int return_address = pop_return_address(&cpu->predictor);
                cpu->pc = return_address;
            }
            break;
        }

        default:
        {
            break;
        }
        }

        cpu->fetch.inst.next_pc = cpu->pc;
    }
    else
    {
        cpu->fetch.has_inst = false;

        DBG("WARN", "Invalid program counter: %d (index = %d)", cpu->pc, index);
    }
}

void decode_1(Cpu *cpu)
{
    if (!cpu->decode_1.has_inst)
        return;

    // Currently Decode 1 does nothing
}

void decode_2(Cpu *cpu)
{
    if (!cpu->decode_2.has_inst)
        return;

    if (cpu->decode_2.inst.rs1 != -1)
    {
        int temp = cpu->decode_2.inst.rs1;
        cpu->decode_2.inst.rs1 =
            map_source_register(&cpu->rt, cpu->decode_2.inst.rs1);
        DBG("INFO", "Renamed Register R%d to P%d", temp, cpu->decode_2.inst.rs1);
    }

    if (cpu->decode_2.inst.rs2 != -1)
    {
        int temp = cpu->decode_2.inst.rs2;
        cpu->decode_2.inst.rs2 =
            map_source_register(&cpu->rt, cpu->decode_2.inst.rs2);
        DBG("INFO", "Renamed Register R%d to P%d", temp, cpu->decode_2.inst.rs2);
    }

    if (cpu->decode_2.inst.rs3 != -1)
    {
        int temp = cpu->decode_2.inst.rs3;
        cpu->decode_2.inst.rs3 =
            map_source_register(&cpu->rt, cpu->decode_2.inst.rs3);
        DBG("INFO", "Renamed Register R%d to P%d", temp, cpu->decode_2.inst.rs3);
    }

    // Renaming registers
    if (cpu->decode_2.inst.rd != -1)
    {
        int temp = cpu->decode_2.inst.rd;
        cpu->decode_2.inst.rd = map_dest_register(&cpu->rt, cpu->decode_2.inst.rd);

        cpu->uprf_valid[cpu->decode_2.inst.rd] = false;
        cpu->fw_uprf_valid[cpu->decode_2.inst.rd] = false;
        DBG("INFO", "Renamed Register R%d to P%d", temp, cpu->decode_2.inst.rd);
    }

    cpu->decode_2.inst.cc = get_cc_register(&cpu->rt);
    switch (cpu->decode_2.inst.op)
    {
    case OP_ADD:
    case OP_SUB:
    case OP_MUL:
    case OP_DIV:
    case OP_AND:
    case OP_OR:
    case OP_XOR:
    case OP_ADDL:
    case OP_SUBL:
    {
        cpu->decode_2.inst.cc = map_cc_register(&cpu->rt);
    }
    }

    cpu->decode_2.inst.bis_entry = (BisEntry){
        .fw_ucrf_valid = {0},
        .fw_ucrf = {0},

        .fw_uprf_valid = {0},
        .fw_uprf = {0},

        .rt = cpu->rt,
        .p = cpu->predictor,
    };

    memcpy(cpu->decode_2.inst.bis_entry.fw_ucrf, cpu->fw_ucrf, sizeof(Cc) * CC_REGS_COUNT);
    memcpy(cpu->decode_2.inst.bis_entry.fw_ucrf_valid, cpu->fw_ucrf_valid, sizeof(int) * CC_REGS_COUNT);
    memcpy(cpu->decode_2.inst.bis_entry.fw_uprf, cpu->fw_uprf, sizeof(int) * PHYS_REGS_COUNT);
    memcpy(cpu->decode_2.inst.bis_entry.fw_uprf_valid, cpu->fw_uprf_valid, sizeof(int) * PHYS_REGS_COUNT);
}

void int_fu(Cpu *cpu)
{
    if (!cpu->intFU.has_inst)
        return;

    if (cpu->intFU.cycles > 0)
    {
        cpu->intFU.cycles -= 1;
    }

    if (cpu->intFU.cycles == 0)
    {
        IQE *iqe = cpu->intFU.iqe;

        switch (iqe->op)
        {
        case OP_ADD:
        {
            iqe->result_buffer = iqe->rs1_value + iqe->rs2_value;
            set_cc_flags(iqe);
            break;
        }
        case OP_SUB:
        {
            iqe->result_buffer = iqe->rs1_value - iqe->rs2_value;
            set_cc_flags(iqe);
            break;
        }
        case OP_AND:
        {
            iqe->result_buffer = iqe->rs1_value & iqe->rs2_value;
            set_cc_flags(iqe);
            break;
        }
        case OP_OR:
        {
            iqe->result_buffer = iqe->rs1_value | iqe->rs2_value;
            set_cc_flags(iqe);
            break;
        }
        case OP_XOR:
        {
            iqe->result_buffer = iqe->rs1_value ^ iqe->rs2_value;
            set_cc_flags(iqe);
            break;
        }
        case OP_MOVC:
        {
            iqe->result_buffer = iqe->imm;
            break;
        }
        case OP_BZ:
        {
            iqe->result_buffer = iqe->pc + iqe->imm;

            if (iqe->cc_value.z)
            {
                add_predictor_entry(&cpu->predictor, iqe->pc, iqe->op, iqe->result_buffer);

                if (iqe->result_buffer != iqe->next_pc)
                {
                    DBG("INFO", "Should flush BZ %c", ' ');

                    flush_cpu_after(cpu, iqe->timestamp);
                    reset_cpu_from_bis(cpu, iqe->bis_entry);
                    cpu->pc = iqe->result_buffer;
                }
            }
            else
            {
                add_predictor_entry(&cpu->predictor, iqe->pc, iqe->op, iqe->pc + 4);

                if (iqe->next_pc != iqe->pc + 4)
                {
                    DBG("INFO", "Mispredicted, need to flush. %c", ' ');

                    flush_cpu_after(cpu, iqe->timestamp);
                    reset_cpu_from_bis(cpu, iqe->bis_entry);
                    cpu->pc = iqe->pc + 4; // Reset to next instruction after branch
                }
            }

            break;
        }
        case OP_BNZ:
        {
            if (!iqe->cc_value.z)
            {
                iqe->result_buffer = iqe->pc + iqe->imm;

                add_predictor_entry(&cpu->predictor, iqe->pc, iqe->op, iqe->result_buffer);

                if (iqe->result_buffer != iqe->next_pc)
                {
                    DBG("INFO", "Should flush BNZ %c", ' ');

                    flush_cpu_after(cpu, iqe->timestamp);
                    reset_cpu_from_bis(cpu, iqe->bis_entry);
                    cpu->pc = iqe->result_buffer;
                }
            }
            else
            {
                add_predictor_entry(&cpu->predictor, iqe->pc, iqe->op, iqe->pc + 4);

                if (iqe->next_pc != iqe->pc + 4)
                {
                    DBG("INFO", "Mispredicted, need to flush. %c", ' ');

                    flush_cpu_after(cpu, iqe->timestamp);
                    reset_cpu_from_bis(cpu, iqe->bis_entry);
                    cpu->pc = iqe->pc + 4; // Reset to next instruction after branch
                }
            }
            break;
        }
        case OP_ADDL:
        {
            iqe->result_buffer = iqe->rs1_value + iqe->imm;
            set_cc_flags(iqe);
            break;
        }
        case OP_SUBL:
        {
            iqe->result_buffer = iqe->rs1_value - iqe->imm;
            set_cc_flags(iqe);
            break;
        }
        case OP_CMP:
        {
            if (iqe->rs1_value == iqe->rs2_value)
                iqe->result_buffer = 0;
            else if (iqe->rs1_value < iqe->rs2_value)
                iqe->result_buffer = -1;
            else
                iqe->result_buffer = 1;

            set_cc_flags(iqe);
            break;
        }
        case OP_CML:
        {
            if (iqe->rs1_value == iqe->imm)
                iqe->result_buffer = 0;
            else if (iqe->rs1_value < iqe->imm)
                iqe->result_buffer = -1;
            else
                iqe->result_buffer = 1;

            set_cc_flags(iqe);
            break;
        }
        case OP_BP:
        {
            if (iqe->cc_value.p)
            {
                iqe->result_buffer = iqe->pc + iqe->imm;

                add_predictor_entry(&cpu->predictor, iqe->pc, iqe->op, iqe->result_buffer);

                if (iqe->result_buffer > iqe->pc)
                {
                    DBG("INFO", "Should flush BP %c", ' ');
                    flush_cpu_after(cpu, iqe->timestamp);
                    reset_cpu_from_bis(cpu, iqe->bis_entry);
                    cpu->pc = iqe->result_buffer;
                }
            }
            else
            {
                add_predictor_entry(&cpu->predictor, iqe->pc, iqe->op, iqe->pc + 4);

                if (iqe->next_pc != iqe->pc + 4)
                {
                    DBG("INFO", "Mispredicted, need to flush. %c", ' ');

                    flush_cpu_after(cpu, iqe->timestamp);
                    reset_cpu_from_bis(cpu, iqe->bis_entry);
                    cpu->pc = iqe->pc + 4; // Reset to next instruction after branch
                }
            }
            break;
        }
        case OP_BN:
        {
            if (iqe->cc_value.n)
            {
                iqe->result_buffer = iqe->pc + iqe->imm;

                add_predictor_entry(&cpu->predictor, iqe->pc, iqe->op, iqe->result_buffer);

                if (iqe->result_buffer > iqe->pc)
                {
                    DBG("INFO", "Should branch BN %c", ' ');
                    flush_cpu_after(cpu, iqe->timestamp);
                    reset_cpu_from_bis(cpu, iqe->bis_entry);
                    cpu->pc = iqe->result_buffer;
                }
            }
            else
            {
                add_predictor_entry(&cpu->predictor, iqe->pc, iqe->op, iqe->pc + 4);

                if (iqe->next_pc != iqe->pc + 4)
                {
                    DBG("INFO", "Mispredicted, need to flush. %c", ' ');

                    flush_cpu_after(cpu, iqe->timestamp);
                    reset_cpu_from_bis(cpu, iqe->bis_entry);
                    cpu->pc = iqe->pc + 4; // Reset to next instruction after branch
                }
            }
            break;
        }
        case OP_BNP:
        {
            if (!iqe->cc_value.p)
            {
                iqe->result_buffer = iqe->pc + iqe->imm;

                add_predictor_entry(&cpu->predictor, iqe->pc, iqe->op, iqe->result_buffer);

                if (iqe->result_buffer > iqe->pc)
                {
                    DBG("INFO", "Should branch BNP %c", ' ');
                    flush_cpu_after(cpu, iqe->timestamp);
                    reset_cpu_from_bis(cpu, iqe->bis_entry);
                    cpu->pc = iqe->result_buffer;
                }
            }
            else
            {
                add_predictor_entry(&cpu->predictor, iqe->pc, iqe->op, iqe->pc + 4);

                if (iqe->next_pc != iqe->pc + 4)
                {
                    DBG("INFO", "Mispredicted, need to flush. %c", ' ');

                    flush_cpu_after(cpu, iqe->timestamp);
                    reset_cpu_from_bis(cpu, iqe->bis_entry);
                    cpu->pc = iqe->pc + 4; // Reset to next instruction after branch
                }
            }
            break;
        }
        case OP_JUMP:
        {
            iqe->result_buffer = iqe->rs1_value + iqe->imm;

            DBG("INFO", "Should jump JUMP to %d", iqe->result_buffer);
            flush_cpu_after(cpu, iqe->timestamp);
            reset_cpu_from_bis(cpu, iqe->bis_entry);
            cpu->pc = iqe->result_buffer;

            break;
        }
        case OP_JALP:
        {
            int jump_addr = iqe->imm + iqe->pc;
            iqe->result_buffer = iqe->pc + 4;

            if (iqe->next_pc != jump_addr) {
                DBG("INFO", "Should jump JALP to %d with return address %d", jump_addr, iqe->result_buffer);
                flush_cpu_after(cpu, iqe->timestamp);
                reset_cpu_from_bis(cpu, iqe->bis_entry);
                cpu->pc = jump_addr;
            }

            PredictorEntry e = {0};
            if (!get_prediction(&cpu->predictor, iqe->pc, &e)) {
                // Only push the return address in IntFu if this JALP appeared for the first time
                push_return_address(&cpu->predictor, iqe->result_buffer); // Push return address into return stack
            }
            add_predictor_entry(&cpu->predictor, iqe->pc, iqe->op, jump_addr);

            break;
        }
        case OP_RET:
        {
            iqe->result_buffer = iqe->rs1_value;
            
            if (iqe->next_pc != iqe->result_buffer)
            {
                DBG("INFO", "Should flush RET %c", ' ');
                flush_cpu_after(cpu, iqe->timestamp);
                reset_cpu_from_bis(cpu, iqe->bis_entry);
                cpu->pc = iqe->result_buffer;
            }

            // If A prediction of the RET was not there in the predictor table
            PredictorEntry e = {0};
            if (!get_prediction(&cpu->predictor, iqe->pc, &e)) {
                // Need to pop an entry from return stack
                pop_return_address(&cpu->predictor);
            }

            add_predictor_entry(&cpu->predictor, iqe->pc, iqe->op, -1); // No target address for RET

            break;
        }
        case OP_NOP:
        case OP_HALT:
        {
            // Nothing
            break;
        }
        default:
        {
            DBG("WARN", "Invalid opcode `0x%x` found in IntFU.", cpu->intFU.iqe->op);
        }
        }
    }
}

void mul_fu(Cpu *cpu)
{
    if (!cpu->mulFU.has_inst)
        return;

    if (cpu->mulFU.cycles > 0)
    {
        cpu->mulFU.cycles -= 1;
    }

    if (cpu->mulFU.cycles == 0)
    {
        IQE *iqe = cpu->mulFU.iqe;
        switch (iqe->op)
        {
        case OP_DIV:
        {
            iqe->result_buffer = iqe->rs1_value / iqe->rs2_value;
            set_cc_flags(iqe);
            break;
        }
        case OP_MUL:
        {
            iqe->result_buffer = iqe->rs1_value * iqe->rs2_value;
            set_cc_flags(iqe);
            break;
        }
        default:
        {
            DBG("WARN", "Invalid opcode `0x%x` found in IntFU.", cpu->intFU.iqe->op);
        }
        }
    }
}

void mem_fu(Cpu *cpu)
{
    if (!cpu->memFU.has_inst)
        return;

    if (cpu->memFU.cycles > 0)
    {
        cpu->memFU.cycles -= 1;
    }

    if (cpu->memFU.cycles == 0)
    {
        IQE *iqe = cpu->memFU.iqe;

        switch (iqe->op)
        {
        case OP_LOAD:
        {
            iqe->result_buffer = iqe->rs1_value + iqe->imm;
            break;
        }
        case OP_STORE:
        {
            iqe->result_buffer = iqe->rs2_value + iqe->imm;
            break;
        }
        case OP_LDR:
        {
            iqe->result_buffer = iqe->rs1_value + iqe->rs2_value;
            break;
        }
        case OP_STR:
        {
            iqe->result_buffer = iqe->rs2_value + iqe->rs3_value;
            break;
        }
        default:
        {
            DBG("WARN", "Invalid instruction found in MemFU: %s", get_op_name(iqe->op));
            break;
        }
        }
    }
}

bool commit(Cpu *cpu)
{
    IQE iqe = {0};
    bool halt = false;

    if (rob_get_completed(&cpu->rob, &iqe))
    {
        if (iqe.op == OP_HALT)
        {
            reset_cpu_from_bis(cpu, iqe.bis_entry);
            halt = true;
        }

        switch (iqe.op)
        {
        case OP_LDR:
        case OP_LOAD:
        {
            iqe.result_buffer = cpu->memory[iqe.result_buffer];

            // Forward the value loaded
            forward_register(cpu, iqe.rd, iqe.result_buffer);

            break;
        }
        case OP_STR:
        case OP_STORE:
        {
            cpu->memory[iqe.result_buffer] = iqe.rs1_value;

            break;
        }
        default:
        {
            // Do nothing special
            break;
        }
        }

        if (iqe.rd != -1)
        {
            cpu->uprf_valid[iqe.rd] = true;
            cpu->uprf[iqe.rd] = iqe.result_buffer;
        }

        cpu->ucrf_valid[iqe.cc] = true;
        cpu->ucrf[iqe.cc] = iqe.cc_value;
    }

    return halt;
}

// Forwards data from each stage in the pipeline to the next stage
void forward_pipeline(Cpu *cpu)
{
    // IntFU
    if (cpu->intFU.has_inst && cpu->intFU.cycles == 0)
    {
        cpu->intFU.has_inst = false;
        cpu->intFU.iqe->completed = true;

        if (cpu->intFU.iqe->rd != -1)
        {
            DBG("INFO", "Forwarding P%d -> %d", cpu->intFU.iqe->rd, cpu->intFU.iqe->result_buffer);

            forward_register(cpu, cpu->intFU.iqe->rd, cpu->intFU.iqe->result_buffer);
            forward_cc_register(cpu, cpu->intFU.iqe->cc, cpu->intFU.iqe->cc_value);
        }
    }

    // MulFU
    if (cpu->mulFU.has_inst && cpu->mulFU.cycles == 0)
    {
        cpu->mulFU.has_inst = false;
        cpu->mulFU.iqe->completed = true;

        if (cpu->mulFU.iqe->rd != -1)
        {
            DBG("INFO", "Forwarding P%d -> %d", cpu->mulFU.iqe->rd, cpu->mulFU.iqe->result_buffer);

            forward_register(cpu, cpu->mulFU.iqe->rd, cpu->mulFU.iqe->result_buffer);
            forward_cc_register(cpu, cpu->mulFU.iqe->cc, cpu->mulFU.iqe->cc_value);
        }
    }

    // MemFU
    if (cpu->memFU.has_inst && cpu->memFU.cycles == 0)
    {
        cpu->memFU.has_inst = false;
        cpu->memFU.iqe->completed = true;
    }

    // IRS -> IntFU
    if (!cpu->intFU.has_inst)
    {
        IQE *iqe = {0};

        if (irs_get_first_ready_iqe((void *)cpu, &iqe))
        {
            cpu->intFU.has_inst = true;
            cpu->intFU.iqe = iqe;
            cpu->intFU.cycles = INT_FU_STAGES;
        }
    }

    // MRS -> MulFU
    if (!cpu->mulFU.has_inst)
    {
        IQE *iqe = {0};

        if (mrs_get_first_ready_iqe((void *)cpu, &iqe))
        {
            cpu->mulFU.has_inst = true;
            cpu->mulFU.iqe = iqe;
            cpu->mulFU.cycles = MUL_FU_STAGES;
        }
    }

    // LSQ -> MemFU
    if (!cpu->memFU.has_inst)
    {
        IQE *iqe = {0};

        if (lsq_get_first_ready_iqe((void *)cpu, &iqe))
        {
            cpu->memFU.has_inst = true;
            cpu->memFU.iqe = iqe;
            cpu->memFU.cycles = MEM_FU_STAGES;
        }
    }

    // Decode 2 -> Reservation Station & ROB
    if (cpu->decode_2.has_inst)
    {
        IQE iqe = make_iqe((void *)cpu, cpu->decode_2.inst);
        IQE *rob_loc = rob_push_iqe(&cpu->rob, iqe);

        if (rob_loc == 0)
        {
            DBG("ERROR", "ROB was full. %c", ' ');
            exit(1);
        }
        DBG("INFO", "ROB len: %d", cpu->rob.len);

        if (send_to_reservation_station((void *)cpu, rob_loc))
        {
            cpu->decode_2.has_inst = false;
        }
        else
        {
            // The reservation station was full, so we could not forward
            // So we stall all previous stages
            return;
        }
    }

    // Decode 1 -> Decode 2
    if (cpu->decode_1.has_inst)
    {
        cpu->decode_1.has_inst = false;

        cpu->decode_2.has_inst = true;
        cpu->decode_2.inst = cpu->decode_1.inst;
    }

    // Fetch -> Decode 1
    if (cpu->fetch.has_inst)
    {
        cpu->fetch.has_inst = false;

        cpu->decode_1.has_inst = true;
        cpu->decode_1.inst = cpu->fetch.inst;
    }
}

void print_stages(const Cpu *cpu)
{
    if (!DEBUG)
        return;

    // Fetch
    printf("Fetch: ");
    if (cpu->fetch.has_inst)
    {
        print_instruction(cpu->fetch.inst);
    }
    else
    {
        printf("No instruction.\n");
    }

    // Decode 1
    printf("Decode 1: ");
    if (cpu->decode_1.has_inst)
    {
        print_instruction(cpu->decode_1.inst);
    }
    else
    {
        printf("No instruction.\n");
    }

    // Decode 2
    printf("Decode 2: ");
    if (cpu->decode_2.has_inst)
    {
        print_instruction(cpu->decode_2.inst);
    }
    else
    {
        printf("No instruction.\n");
    }

    // IRS
    printf("IRS: [ ");
    for (int i = 0; i < cpu->irs.len; i++)
    {
        if (i == 0)
            printf("\n");
        printf("       ");
        print_iqe(cpu->irs.queue[i]);
    }
    printf(" ]\n");

    // MRS
    printf("MRS: [ ");
    for (int i = 0; i < cpu->mrs.len; i++)
    {
        if (i == 0)
            printf("\n");
        printf("       ");
        print_iqe(cpu->mrs.queue[i]);
    }
    printf(" ]\n");

    // LSQ
    printf("LSQ: [ ");
    for (int i = 0; i < cpu->lsq.len; i++)
    {
        if (i == 0)
            printf("\n");
        printf("       ");
        print_iqe(cpu->lsq.queue[i]);
    }
    printf(" ]\n");

    // IntFU
    printf("IntFU: ");
    if (cpu->intFU.has_inst)
    {
        print_iqe(cpu->intFU.iqe);
    }
    else
    {
        printf("No instruction.\n");
    }

    // MulFU
    printf("MulFU: ");
    if (cpu->mulFU.has_inst)
    {
        print_iqe(cpu->mulFU.iqe);
    }
    else
    {
        printf("No instruction.\n");
    }

    // MemFU
    printf("MemFU: ");
    if (cpu->memFU.has_inst)
    {
        print_iqe(cpu->memFU.iqe);
    }
    else
    {
        printf("No instruction.\n");
    }

    // ROB
    printf("ROB: [ ");
    RobNode *node = cpu->rob.head;
    bool first_item_flag = true;
    while (node != NULL)
    {
        if (first_item_flag)
        {
            first_item_flag = false;
            printf("\n");
        }
        printf("       ");
        print_iqe(&node->iqe);
        node = node->next;
    }
    printf(" ]\n");
}

void print_registers(const Cpu *cpu)
{
    printf("Registers:\n");
    for (int i = 0; i < 4; i++)
    {
        printf("    ");
        for (int j = 0; j < 8; j++)
        {
            int arch_r = i * 8 + j;
            int phy_r = cpu->rt.table[arch_r]; // Get current mapping for architectural register
            int v = cpu->uprf[phy_r];
            printf("R%d\t[%d]\t", arch_r, v);
        }
        printf("\n");
    }
}

void print_data_memory(const Cpu *cpu)
{
    printf("Data Memory:\n");
    // Print first 10 memory locations
    for (int i = 0; i < 20; i++)
    {
        printf("    [%d] = %d\n", i, cpu->memory[i]);
    }
}

bool simulate_cycle(Cpu *cpu)
{
    cpu->cycles += 1;
    DBG("\nINFO",
        "==================== Cycle %d ====================", cpu->cycles);

    // Fetch
    fetch(cpu);

    // Decode 1
    decode_1(cpu);

    // Decode 2
    decode_2(cpu);

    // IntFU
    int_fu(cpu);

    // MulFU
    mul_fu(cpu);

    // MemFU
    mem_fu(cpu);

    // Commit
    bool sim_completed = commit(cpu);

    // Print stages
    print_stages(cpu);
    print_data_memory(cpu);
    print_registers(cpu);
    // print_rename_table(cpu->rt);
    print_predictor(cpu->predictor);

    // Forward data to next stage
    forward_pipeline(cpu);

    return sim_completed;
}

void display(Cpu *cpu){
    if (cpu == NULL) {
        printf("Cpu was not initialized. Please run the 'Initialize' command.\n");
        return;
    }

    printf("----------\n%s\n----------\n", "Stages:");

    // All stages
    print_stages(cpu);

    // All regs
    print_registers(cpu);

    // Flags
    //print_flags(cpu);

    // First 10 mem locations
    print_data_memory(cpu);
}

void show_mem(Cpu *cpu, int address){
    if (cpu == NULL) {
        printf("Cpu was not initialized. Please run the 'Initialize' command.\n");
        return;
    }

    if (address < 0 || address >= DATA_MEMORY_SIZE) {
        printf("Invalid address provided. Please enter an address between 0 and %d.\n", DATA_MEMORY_SIZE-1);
        return;
    }

    printf("----------\n%s\n----------\n", "Data Memory:");
    printf("[0x%X] | %d\n", address, cpu->memory[address]);
}

void set_memory(Cpu *cpu, char *filename){
        if (cpu == NULL) {
        printf("Cpu was not initialized. Please run the 'Initialize' command.\n");
        return;
    }
    
    FILE *fp;
    size_t nread;
    size_t len = 0;
    char *line = NULL;
    int data_memory_idx = 0;

    if (!filename)
    {
        printf("No file name provided.\n");
        return;
    }

    fp = fopen(filename, "r");
    if (!fp)
    {
        printf("Failed to open file %s.\n", filename);
        return;
    }

    nread = getline(&line, &len, fp);
    if (nread == -1) {
        printf("Memory file was empty.\n");
        return;
    }

    // Keep getting numbers separated by ','
    trim(line);

    char *token = strtok(line, ",");

    while (token != NULL) {
        trim(token);
        int value = atoi(token);

        if (data_memory_idx >= DATA_MEMORY_SIZE) {
            printf("Too many values provided in memory file. Terminating early.\n");
            return;
        }
        cpu->memory[data_memory_idx] = value; // Update data memory
        data_memory_idx += 1;

        token = strtok(NULL, ",");
    }

    free(line);
    fclose(fp);
}
