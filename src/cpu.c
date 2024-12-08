#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "cpu.h"
#include "instruction.h"
#include "macros.h"
#include "rename.h"
#include "rob.h"
#include "rs.h"

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
  memset(&cpu.ucrf_valid, 1, sizeof(int) * PHYS_REGS_COUNT);

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

int get_ucrf_value(Cpu cpu, int cc, Cc *dest) {
  if (cc >= CC_REGS_COUNT) {
    DBG("ERROR", "Tried to read value of C%d.", cc);
    return false;
  }

  if (cpu.ucrf_valid[cc]) {
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

void forward_cc_register(Cpu *cpu, int cc, Cc value) {
  cpu->fw_ucrf_valid[cc] = true;
  cpu->fw_ucrf[cc] = value;
}

void set_cc_flags(IQE *iqe) {
  iqe->cc_value = (Cc) { false, false, false };
  
  if (iqe->result_buffer == 0) {
    iqe->cc_value.z = true;
  } else if (iqe->result_buffer > 0) {
    iqe->cc_value.p = true;
  } else {
    iqe->cc_value.n = true;
  }
}

// Convert pc from address space to index in instruction list
int pc_to_index(int pc) { return (pc - 4000) / 4; }

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

    cpu->fetch.has_inst = true;
    cpu->fetch.inst = inst;

    cpu->pc += 4; // Go to next instruction
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
    DBG("INFO", "Renamed Register R%d to P%d", temp, cpu->decode_2.inst.rd);
  }

  cpu->decode_2.inst.cc = get_cc_register(&cpu->rt);
  switch (cpu->decode_2.inst.op) {
    case OP_ADD:
    case OP_SUB:
    case OP_MUL:
    case OP_DIV:
    case OP_AND:
    case OP_OR:
    case OP_XOR:
    case OP_ADDL:
    case OP_SUBL: {
      cpu->decode_2.inst.cc = map_cc_register(&cpu->rt);
    }
  }
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
      if (iqe->cc_value.z) {
        DBG("INFO", "Should branch BZ %c", ' ');
      }
      break;
    }
    case OP_BNZ:
    {
      if (!iqe->cc_value.z) {
        DBG("INFO", "Should branch BNZ %c", ' ');
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
      if (iqe->rs1_value == iqe->rs2_value) iqe->result_buffer = 0;
      else if (iqe->rs1_value < iqe->rs2_value) iqe->result_buffer = -1;
      else iqe->result_buffer = 1;

      set_cc_flags(iqe);
      break;
    }
    case OP_CML:
    {
      if (iqe->rs1_value == iqe->imm) iqe->result_buffer = 0;
      else if (iqe->rs1_value < iqe->imm) iqe->result_buffer = -1;
      else iqe->result_buffer = 1;

      set_cc_flags(iqe);
      break;
    }
    case OP_BP:
    {
      if (iqe->cc_value.p) {
        DBG("INFO", "Should branch BP %c", ' ');
      }
      break;
    }
    case OP_BN:
    {
      if (iqe->cc_value.n) {
        DBG("INFO", "Should branch BN %c", ' ');
      }
      break;
    }
    case OP_BNP:
    {
      if (!iqe->cc_value.p) {
        DBG("INFO", "Should branch BNP %c", ' ');
      }
      break;
    }
    case OP_JUMP:
    {
      // TODO
      break;
    }
    case OP_JALP:
    {
      // TODO
      break;
    }
    case OP_RET:
    {
      // TODO
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
    switch (iqe->op) {
      case OP_DIV: {
        iqe->result_buffer = iqe->rs1_value / iqe->rs2_value;
        set_cc_flags(iqe);
        break;
      }
      case OP_MUL: {
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
    // TODO: Perform function
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
      halt = true;
    }
    // TODO: Do something with this IQE
    // Also update uprf_valid

    if (iqe.rd != -1) {
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

    if (cpu->memFU.iqe->rd != -1)
    {
      DBG("INFO", "Forwarding P%d -> %d", cpu->memFU.iqe->rd, cpu->memFU.iqe->result_buffer);

      forward_register(cpu, cpu->memFU.iqe->rd, cpu->memFU.iqe->result_buffer);
    }
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

void print_stages(Cpu *cpu)
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

void print_registers(Cpu *cpu) {
  printf("Registers:\n");
  for (int i = 0; i < 4; i++) {
    printf("    ");
    for (int j = 0; j < 8; j++) {
      int r = map_source_register(&cpu->rt, i * 8 + j);
      printf("r = %d\n", r);
      int v = cpu->uprf[r];
      printf("v = %d", v);
      printf("R%d\t[%d]\t", r, v);
    }
    printf("\n");
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

  // Forward data to next stage
  forward_pipeline(cpu);

  // Temp
  if (sim_completed) {
    print_registers(cpu);
  }

  return sim_completed;
}
