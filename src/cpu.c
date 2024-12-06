#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "cpu.h"
#include "instruction.h"
#include "macros.h"
#include "rename.h"
#include "rob.h"
#include "rs.h"

Cpu initialize_cpu(char *asm_file) {
  InstructionList inst_list =
      parse(asm_file); // We will need to free this later

  Cpu cpu = {0};
  cpu.code = inst_list;
  cpu.pc = 4000;
  cpu.rt = initialize_rename_table();
  cpu.rob.head = NULL;

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
int pc_to_index(int pc) { return (pc - 4000) / 4; }

// Fetch stage
void fetch(Cpu *cpu) {
  // Don't fetch if an instruction already exists in Fetch
  if (cpu->fetch.has_inst)
    return;

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
  if (!cpu->decode_1.has_inst)
    return;

  // Currently Decode 1 does nothing
}

void decode_2(Cpu *cpu) {
  if (!cpu->decode_2.has_inst)
    return;

  // Renaming registers
  if (cpu->decode_2.inst.rd != -1) {
    int temp = cpu->decode_2.inst.rd;
    cpu->decode_2.inst.rd = map_dest_register(&cpu->rt, cpu->decode_2.inst.rd);

    cpu->uprf_valid[cpu->decode_2.inst.rd] = false;
    DBG("INFO", "Renamed Register R%d to P%d", temp, cpu->decode_2.inst.rd);
  }

  if (cpu->decode_2.inst.rs1 != -1) {
    int temp = cpu->decode_2.inst.rs1;
    cpu->decode_2.inst.rs1 =
        map_source_register(&cpu->rt, cpu->decode_2.inst.rs1);
    DBG("INFO", "Renamed Register R%d to P%d", temp, cpu->decode_2.inst.rs1);
  }
  if (cpu->decode_2.inst.rs2 != -1) {
    int temp = cpu->decode_2.inst.rs2;
    cpu->decode_2.inst.rs2 =
        map_source_register(&cpu->rt, cpu->decode_2.inst.rs2);
    DBG("INFO", "Renamed Register R%d to P%d", temp, cpu->decode_2.inst.rs2);
  }
  if (cpu->decode_2.inst.rs3 != -1) {
    int temp = cpu->decode_2.inst.rs3;
    cpu->decode_2.inst.rs3 =
        map_source_register(&cpu->rt, cpu->decode_2.inst.rs3);
    DBG("INFO", "Renamed Register R%d to P%d", temp, cpu->decode_2.inst.rs3);
  }
}

void int_fu(Cpu *cpu) {
  if (!cpu->intFU.has_inst)
    return;

  if (cpu->intFU.cycles > 0) {
    cpu->intFU.cycles -= 1;
  }

  if (cpu->intFU.cycles == 0) {
    IQE *iqe = cpu->intFU.iqe;

    switch (iqe->op) {
    case OP_ADD: {
      iqe->result_buffer = iqe->rs1_value + iqe->rs2_value;
      break;
    }
    case OP_SUB: {
      iqe->result_buffer = iqe->rs1_valid - iqe->rs2_value;
      break;
    }
    case OP_AND: {
      iqe->result_buffer = iqe->rs1_value & iqe->rs2_value;
      break;
    }
    case OP_OR: {
      iqe->result_buffer = iqe->rs1_value | iqe->rs2_value;
      break;
    }
    case OP_XOR: {
      iqe->result_buffer = iqe->rs1_value ^ iqe->rs2_value;
      break;
    }
    case OP_MOVC: {
      iqe->result_buffer = iqe->imm;
      break;
    }
    case OP_BZ: {
      // TODO
      break;
    }
    case OP_BNZ: {
      // TODO
      break;
    }
    case OP_ADDL: {
      iqe->result_buffer = iqe->rs1_value + iqe->imm;
      break;
    }
    case OP_SUBL: {
      iqe->result_buffer = iqe->rs1_value - iqe->imm;
      break;
    }
    case OP_CMP: {
      // TODO
      break;
    }
    case OP_CML: {
      // TODO
      break;
    }
    case OP_BP: {
      // TODO
      break;
    }
    case OP_BN: {
      // TODO
      break;
    }
    case OP_BNP: {
      // TODO
      break;
    }
    case OP_JUMP: {
      // TODO
      break;
    }
    case OP_JALP: {
      // TODO
      break;
    }
    case OP_RET: {
      // TODO
      break;
    }
    case OP_NOP:
    case OP_HALT: {
      // Nothing
      break;
    }
    default: {
      DBG("WARN", "Invalid opcode `0x%x` found in IntFU.", cpu->intFU.iqe->op);
    }
    }
  }
}

void mul_fu(Cpu *cpu) {
  if (!cpu->mulFU.has_inst)
    return;

  if (cpu->mulFU.cycles > 0) {
    cpu->mulFU.cycles -= 1;
  }

  if (cpu->mulFU.cycles == 0) {
    // TODO: Perform function
  }
}

void mem_fu(Cpu *cpu) {
  if (!cpu->memFU.has_inst)
    return;

  if (cpu->memFU.cycles > 0) {
    cpu->memFU.cycles -= 1;
  }

  if (cpu->memFU.cycles == 0) {
    // TODO: Perform function
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
    // Also update uprf_valid
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

  // IntFU
  if (cpu->intFU.has_inst && cpu->intFU.cycles == 0) {
    cpu->intFU.has_inst = false;
    cpu->intFU.iqe->completed = true;

    if (cpu->intFU.iqe->rd != -1) {
      DBG("INFO", "Forwarding P%d -> %d", cpu->intFU.iqe->rd, cpu->intFU.iqe->result_buffer);

      irs_send_forwarded_register(&cpu->irs, cpu->intFU.iqe->rd,
                                  cpu->intFU.iqe->result_buffer);
      mrs_send_forwarded_register(&cpu->mrs, cpu->intFU.iqe->rd,
                                  cpu->intFU.iqe->result_buffer);
      lsq_send_forwarded_register(&cpu->lsq, cpu->intFU.iqe->rd,
                                  cpu->intFU.iqe->result_buffer);
    }
  }

  // MulFU
  if (cpu->mulFU.has_inst && cpu->mulFU.cycles == 0) {
    cpu->mulFU.has_inst = false;
    cpu->mulFU.iqe->completed = true;

    if (cpu->mulFU.iqe->rd != -1) {
	    DBG("INFO", "Forwarding P%d -> %d", cpu->intFU.iqe->rd, cpu->mulFU.iqe->result_buffer);

      irs_send_forwarded_register(&cpu->irs, cpu->mulFU.iqe->rd,
                                  cpu->mulFU.iqe->result_buffer);
      mrs_send_forwarded_register(&cpu->mrs, cpu->mulFU.iqe->rd,
                                  cpu->mulFU.iqe->result_buffer);
      lsq_send_forwarded_register(&cpu->lsq, cpu->mulFU.iqe->rd,
                                  cpu->mulFU.iqe->result_buffer);
    }
  }

  // MemFU
  if (cpu->memFU.has_inst && cpu->memFU.cycles == 0) {
    cpu->memFU.has_inst = false;
    cpu->memFU.iqe->completed = true;

    if (cpu->memFU.iqe->rd != -1) {
    DBG("INFO", "Forwarding P%d -> %d", cpu->intFU.iqe->rd, cpu->memFU.iqe->result_buffer);

      irs_send_forwarded_register(&cpu->irs, cpu->memFU.iqe->rd,
                                  cpu->memFU.iqe->result_buffer);
      mrs_send_forwarded_register(&cpu->mrs, cpu->memFU.iqe->rd,
                                  cpu->memFU.iqe->result_buffer);
      lsq_send_forwarded_register(&cpu->lsq, cpu->memFU.iqe->rd,
                                  cpu->memFU.iqe->result_buffer);
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

void print_stages(Cpu *cpu) {
  if (!DEBUG)
    return;

  // Fetch
  printf("Fetch: ");
  if (cpu->fetch.has_inst) {
    print_instruction(cpu->fetch.inst);
  } else {
    printf("No instruction.\n");
  }

  // Decode 1
  printf("Decode 1: ");
  if (cpu->decode_1.has_inst) {
    print_instruction(cpu->decode_1.inst);
  } else {
    printf("No instruction.\n");
  }

  // Decode 2
  printf("Decode 2: ");
  if (cpu->decode_2.has_inst) {
    print_instruction(cpu->decode_2.inst);
  } else {
    printf("No instruction.\n");
  }

  // IRS
  printf("IRS: [ ");
  for (int i = 0; i < cpu->irs.len; i++) {
    if (i == 0)
      printf("\n");
    printf("       ");
    print_iqe(cpu->irs.queue[i]);
  }
  printf(" ]\n");

  // MRS
  printf("MRS: [ ");
  for (int i = 0; i < cpu->mrs.len; i++) {
    if (i == 0)
      printf("\n");
    printf("       ");
    print_iqe(cpu->mrs.queue[i]);
  }
  printf(" ]\n");

  // LSQ
  printf("LSQ: [ ");
  for (int i = 0; i < cpu->lsq.len; i++) {
    if (i == 0)
      printf("\n");
    printf("       ");
    print_iqe(cpu->lsq.queue[i]);
  }
  printf(" ]\n");

  // IntFU
  printf("IntFU: ");
  if (cpu->intFU.has_inst) {
    print_iqe(cpu->intFU.iqe);
  } else {
    printf("No instruction.\n");
  }

  // MulFU
  printf("MulFU: ");
  if (cpu->mulFU.has_inst) {
    print_iqe(cpu->mulFU.iqe);
  } else {
    printf("No instruction.\n");
  }

  // MemFU
  printf("MemFU: ");
  if (cpu->memFU.has_inst) {
    print_iqe(cpu->memFU.iqe);
  } else {
    printf("No instruction.\n");
  }

  // ROB
  printf("ROB: [ ");
  RobNode *node = cpu->rob.head;
  bool first_item_flag = true;
  while (node != NULL) {
  	if (first_item_flag) {
   		first_item_flag = false;
     	printf("\n");
   }
    printf("       ");
    print_iqe(&node->iqe);
    node = node->next;
  }
  printf(" ]\n");
}

bool simulate_cycle(Cpu *cpu) {
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

  // Forward data to next stage
  forward_pipeline(cpu);

  return sim_completed;
}
