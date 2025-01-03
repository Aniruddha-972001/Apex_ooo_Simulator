# Design Document - Apex CPU Simulator (Draft 1)

## Data Structures

### CPU

- **UCRF**: Array of 10 integer entries.
- **UPRF**: Array of 60 integer entries.
- **Rename Table**: Array of integers, used for register renaming.
- **Free List (for Rename Table)**: Array of integers representing the free list of registers available for renaming.
- **Forwarded Registers**: Copy of `UCRF` and `UPRF` to hold the values of forwarded registers. Same size as `UCRF` and `UPRF`.
- **Forwarded Register Validity**: Array of boolean values (0 or 1) indicating whether each forwarded register has a valid value.
- **Data Memory**: Array of integers representing the data memory.
- **Program Counter (PC)**: Integer representing the program counter.
- **Clock Cycles**: Integer tracking the number of clock cycles in the simulation.
- **Code Memory**: Array of `Instruction` structures representing the instruction memory.
- **Stalled?**: Boolean flag indicating if the CPU is stalled.
- **UPRF Locks**: Array of semaphores indicating the lock status of each entry in the `UPRF`.
- **UCRF Locks**: Array of semaphores indicating the lock status of each entry in the `UCRF`.
- **IRS (Instruction Reservation Station)**: Queue of `IQE` entries representing the reservation station for integer operations.
- **LSQ (Load/Store Queue)**: Queue of `IQE` entries for managing load and store operations.
- **MRS (Multiply Reservation Station)**: Queue of `IQE` entries for managing multiply operations.
- **ROB (Reorder Buffer)**: Queue of `IQE` entries for managing instruction completion and commit.
- **Fetch**: Current state of the fetch stage (type: `CPU_STAGE`).
- **D1**: Current state of the first decode stage (type: `CPU_STAGE`).
- **D2**: Current state of the second decode stage (type: `CPU_STAGE`).
- **Int FU**: Current state of the integer functional unit (type: `CPU_STAGE`).
- **MUL FU**: Current state of the multiplication functional unit (type: `CPU_STAGE`).
- **MEM FU**: Current state of the memory functional unit (type: `CPU_STAGE`).
- **Commit**: Current state of the commit stage (type: `CPU_STAGE`).
- **Predictor Queue**: Current state of the predictor queue.
- **Return Stack**: Current state of the Return Stack.


---

### Instruction

- **Opcode (str)**: A string representation of the instruction's opcode.
- **Opcode (int)**: The integer value representing the opcode of the instruction.
- **Rd**: The destination register for the instruction.
- **Rs1**: The first source register for the instruction.
- **Rs2**: The second source register for the instruction.
- **Rs3**: The third source register for the instruction.
- **Imm**: The immediate value (if applicable) for the instruction.

---

### IQE (Instruction Queue Entry)

Each entry in the instruction queue contains the following information:

- **Opcode**: Integer representing the opcode.
- **Rd**: The destination register.
- **Rs1**: The first source register.
- **Rs2**: The second source register.
- **Rs3**: The third source register.
- **Imm**: The immediate value.
- **Rd_value**: The value of the destination register.
- **Rs1_value**: The value of the first source register.
- **Rs2_value**: The value of the second source register.
- **Rs3_value**: The value of the third source register.
- **Imm_value**: The value of the immediate.
- **Completed**: Boolean flag indicating whether the instruction has completed execution.
- **Rename Table**: Current state of rename table 
- **Free List**: Current state of the free list
- **Forwarded rergister file**: Current state of the forwarded register file
- **Forwarded rergister locks**: Current state of the forwarded register locks
- **Predictor Queue**: Current state of the predictor queue.
- **Return Stack**: Current state of the Return Stack.



---

### PQE (Prediction Queue Entry)
- **PC**: Address of the control flow instruction.
- **Type**: Type of the control flow instruction (Branch ,JALP, RET).
- **Next_PC**: Address of the predicted next instruction.

---

### RSE (Return Stack Entry)
- **Return Address**: Calculated address to return to.

---

### CPU_STAGE

Represents a stage in the CPU pipeline.

- **Has instruction**: Boolean flag indicating whether the stage has an instruction.
- **Entry**: An `IQE` representing the instruction currently in the stage.

---

## Important Functions

### Parse File

- **Description**: Parses the assembly code from the given file and returns an array of parsed instructions.
- **Return Value**: Array of `Instruction[]`.
- **Arguments**: `char* filename` — The name of the file containing the assembly code.

**TODO**: Implement error reporting for invalid syntax.

---

### Simulate

- **Description**: Simulates the execution of instructions on the CPU. It manages the progression through all stages of the CPU pipeline.
- **Return Value**: `void` (no return value).
- **Arguments**: `cpu*` — The current state of the CPU.

---

### Fetch

- **Description**: Reads instructions from memory and fills the `IQE` entries for further processing.
- **Return Value**: `TODO` (to be determined).
- **Arguments**: `cpu*` — The current state of the CPU.

**TODO**: Implement instruction prediction (branch prediction).

---

### Decode (D1)

- **Description**: Responsible for register renaming and completing the entries in the `IQE`. If the Instruction was a conditional jump or RET or JALP instruction then we also save the current state of the CPU. This information is stored in the IQE.
- **Return Value**: `TODO` (to be determined).
- **Arguments**: `cpu*` — The current state of the CPU.

---

### Decode (D2)

- **Description**: Forwards instructions to their respective issue queues. Also, forwards instructions to the Reorder Buffer (ROB).
- **Return Value**: `TODO` (to be determined).
- **Arguments**: `cpu*` — The current state of the CPU.

---

### IRS (Integer Reservation Station)

- **Description**: Checks if the source registers are locked, verifies if the values have been forwarded, and fills in the register values. The instruction is then forwarded to the Integer Functional Unit (Int FU).
- **Return Value**: `TODO` (to be determined).
- **Arguments**: `cpu*` — The current state of the CPU.

---

### LSQ (Load/Store Queue)

- **Description**: Similar to IRS, this checks for locked registers, verifies forwarded values, and fills in the register values. The instruction is then forwarded to the Memory Functional Unit (MEM FU).
- **Return Value**: `TODO` (to be determined).
- **Arguments**: `cpu*` — The current state of the CPU.

---

### MRS (Multiply Reservation Station)

- **Description**: Checks for locked source registers, verifies forwarded values, and fills in the register values. The instruction is then forwarded to the Multiply Functional Unit (MUL FU).
- **Return Value**: `TODO` (to be determined).
- **Arguments**: `cpu*` — The current state of the CPU.

---

### Int FU (Integer Functional Unit)

- **Description**: Simulates the execution of instructions in the integer functional unit.
- **Return Value**: `TODO` (to be determined).
- **Arguments**: `cpu*` — The current state of the CPU.

---

### MUL FU (Multiplication Functional Unit)

- **Description**: Simulates the execution of instructions in the multiplication functional unit.
- **Return Value**: `TODO` (to be determined).
- **Arguments**: `cpu*` — The current state of the CPU.

---

### MEM FU (Memory Functional Unit)

- **Description**: Simulates the execution of instructions in the memory functional unit.
- **Return Value**: `TODO` (to be determined).
- **Arguments**: `cpu*` — The current state of the CPU.

---

### ROB (Reorder Buffer)

- **Description**: Monitors the completion of instructions and forwards completed instructions to the commit stage.
- **Return Value**: `TODO` (to be determined).
- **Arguments**: `cpu*` — The current state of the CPU.

---

### Commit

- **Description**: Commits instructions by updating registers and memory.
- **Return Value**: `bool` — Indicates whether the simulation has terminated.
- **Arguments**: `cpu*` — The current state of the CPU.

---

## Flushing of Instructions 

- **Description**: Wheather or not we have to flush an instruction is decided in intFU. When we have to flush we first inform the ROB , the instruction after which we have to flush. ROB will then remove all the instruction after the given one from the queue and also remove them from LSQ,IRS,MRS. Then we update the PC to the correct address and also reset important structures such as rename table , free list , forwarded registers and forwarded register locks.  


---

##  Predictor 

-**Description**: The decode stage will add an entry to the predictor queue if the current instruction is a branch or JALP or RET. of, this entry already exists in the predictor queue we do not make any changes. When a branch or JALP or RET instructions next address is calculated in the intFU we update the `next_PC` field  with the calculate next address(if instuction was JALP the calculated address is stored in the return stack) and finally next time the instruction is fetched we check if an entry in the predictor queue exisist for the current fetched instruction, if it exists then insted of updating the pc value to `pc+4` we simply change it to `next_PC` from the predictor queue.

----
## Notes

- All Arrays/Queues mentioned in this design document are custom data structures that support dynamic resizing when needed.
