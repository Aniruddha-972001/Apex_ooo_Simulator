#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "macros.h"
#include "cpu.h"
#include "instruction.h"
#include "rename.h"
#include "rob.h"
#include "rs.h"

Cpu initialize_cpu(char *asm_file) {
    InstructionList inst_list = parse(asm_file); // We will need to free this later

    Cpu cpu = {0};
    cpu.code = inst_list;
    cpu.pc = 4000;
    cpu.rt = initialize_rename_table();

    memset(&cpu.uprf_valid, 1, sizeof(int) * PHYS_REGS_COUNT);

    return cpu;
}

int get_urpf_value(Cpu cpu, int phy_reg, int *dest) {
    if (phy_reg >= PHYS_REGS_COUNT) {
        DBG("ERROR", "Tried to read value of P%d.", phy_reg);
        return false;
    }

    if (cpu.uprf_valid[phy_reg]) {
        *dest = cpu.uprf[phy_reg];

        return true;
    }

    return false;
}

// Convert pc from address space to index in instruction list
int pc_to_index(int pc) {
    return (pc - 4000) / 4;
}

// Fetch stage
void fetch(Cpu *cpu) {
    // Don't fetch if an instruction already exists in Fetch
    if (cpu->fetch.has_inst) return;

    int index = pc_to_index(cpu->pc);

    if (index >= 0 && index < cpu->code.len) {
        Instruction inst = cpu->code.data[index];

        cpu->fetch.has_inst = true;
        cpu->fetch.inst = inst;

        cpu->pc += 4; // Go to next instruction
    } else {
        cpu->fetch.has_inst = false;

        DBG("WARN", "Invalid program counter: %d (index = %d)", cpu->pc, index);
    }
}

void decode_1(Cpu *cpu) {
    if (!cpu->decode_1.has_inst) return;

    // Currently Decode 1 does nothing
}

void decode_2(Cpu *cpu) {
    if (!cpu->decode_2.has_inst) return;

    // Renaming registers
    if (cpu->decode_2.inst.rd != -1) {
        int temp = cpu->decode_2.inst.rd;
        cpu->decode_2.inst.rd = map_dest_register(&cpu->rt, cpu->decode_2.inst.rd);
        DBG("INFO", "Renamed Register R%d to P%d", temp, cpu->decode_2.inst.rd);
    }

    if (cpu->decode_2.inst.rs1 != -1) {
        int temp = cpu->decode_2.inst.rs1;
        cpu->decode_2.inst.rs1 = map_source_register(&cpu->rt, cpu->decode_2.inst.rs1);
        DBG("INFO", "Renamed Register R%d to P%d", temp, cpu->decode_2.inst.rs1);
    }
    if (cpu->decode_2.inst.rs2 != -1) {
        int temp = cpu->decode_2.inst.rs2;
        cpu->decode_2.inst.rs2 = map_source_register(&cpu->rt, cpu->decode_2.inst.rs2);
        DBG("INFO", "Renamed Register R%d to P%d", temp, cpu->decode_2.inst.rs2);
    }
    if (cpu->decode_2.inst.rs3 != -1) {
        int temp = cpu->decode_2.inst.rs3;
        cpu->decode_2.inst.rs3 = map_source_register(&cpu->rt, cpu->decode_2.inst.rs3);
        DBG("INFO", "Renamed Register R%d to P%d", temp, cpu->decode_2.inst.rs3);
    }
}

void int_fu(Cpu *cpu) {
    if (!cpu->intFU.has_inst) return;

    if (cpu->intFU.cycles > 0) {
        cpu->intFU.cycles -= 1;
    }

    if (cpu->intFU.cycles == 0) {
        // TODO: Perform function

        // TODO: Forward data

        cpu->intFU.has_inst = false;
    }
}

void mul_fu(Cpu *cpu) {
    if (!cpu->mulFU.has_inst) return;

    if (cpu->mulFU.cycles > 0) {
        cpu->mulFU.cycles -= 1;
    }

    if (cpu->mulFU.cycles == 0) {
        // TODO: Perform function

        // TODO: Forward data

        cpu->mulFU.has_inst = false;
    }
}

void mem_fu(Cpu *cpu) {
    if (!cpu->memFU.has_inst) return;

    if (cpu->memFU.cycles > 0) {
        cpu->memFU.cycles -= 1;
    }

    if (cpu->memFU.cycles == 0) {
        // TODO: Perform function

        // TODO: Forward data

        cpu->memFU.has_inst = false;
    }
}

bool commit(Cpu *cpu) {
	IQE iqe = {0};
	bool halt = false;

	if (rob_get_completed(&cpu->rob, &iqe)) {
		if (iqe.op == OP_HALT) {
			halt = true;
		}
		// TODO: Do something with this IQE
	}

	return halt;
}

// Forwards data from each stage in the pipeline to the next stage
void forward_pipeline(Cpu *cpu) {
    // IRS -> IntFU
    if (!cpu->intFU.has_inst) {
        IQE *iqe = {0};

        if (irs_get_first_ready_iqe((void *)cpu, &iqe)) {
            cpu->intFU.has_inst = true;
            cpu->intFU.iqe = iqe;
            cpu->intFU.cycles = INT_FU_STAGES;
        }
    }

    // MRS -> MulFU
    if (!cpu->mulFU.has_inst) {
        IQE *iqe = {0};

        if (mrs_get_first_ready_iqe((void *)cpu, &iqe)) {
            cpu->mulFU.has_inst = true;
            cpu->mulFU.iqe = iqe;
            cpu->mulFU.cycles = MUL_FU_STAGES;
        }
    }

    // LSQ -> MemFU
    if (!cpu->memFU.has_inst) {
        IQE *iqe = {0};

        if (lsq_get_first_ready_iqe((void *)cpu, &iqe)) {
            cpu->memFU.has_inst = true;
            cpu->memFU.iqe = iqe;
            cpu->memFU.cycles = MEM_FU_STAGES;
        }
    }

    // Decode 2 -> Reservation Station & ROB
    if (cpu->decode_2.has_inst) {
    	IQE iqe = make_iqe((void *)cpu, cpu->decode_2.inst);
        IQE *rob_loc = rob_push_iqe(&cpu->rob, iqe);

        if (rob_loc == 0) {
        	DBG("ERROR", "ROB was full. %c", ' ');
         	exit(1);
        }
        DBG("INFO", "ROB len: %d", cpu->rob.len);

        if (send_to_reservation_station((void *)cpu, rob_loc)) {
            cpu->decode_2.has_inst = false;
        } else {
            // The reservation station was full, so we could not forward
            // So we stall all previous stages
            return;
        }
    }

    // Decode 1 -> Decode 2
    if (cpu->decode_1.has_inst) {
        cpu->decode_1.has_inst = false;

        cpu->decode_2.has_inst = true;
        cpu->decode_2.inst = cpu->decode_1.inst;
    }

    // Fetch -> Decode 1
    if (cpu->fetch.has_inst) {
        cpu->fetch.has_inst = false;

        cpu->decode_1.has_inst = true;
        cpu->decode_1.inst = cpu->fetch.inst;
    }
}

bool simulate_cycle(Cpu *cpu) {
    cpu->cycles += 1;
    DBG("\nINFO", "==================== Cycle %d ====================", cpu->cycles);

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

    // Forward data to next stage
    forward_pipeline(cpu);

    return sim_completed;
}
