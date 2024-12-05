# Design Document - Out-Of-Order Apex Processor

## Structures and Functions

### struct CPU
Fields:
- UCRF (struct UCRF): User control registers for CC flags
- UPRF (struct UPRF): User program registers
- Rename Table (struct RenameTable): Maps architectural registers to physical registers
- ROB (struct ROB): Reorder buffer for in-order commitment
- IRS (struct IRS): Integer reservation station (8 entries)
- LSQ (struct LSQ): Load store queue (6 entries)
- MRS (struct MRS): Multiplication reservation station (2 entries)
- Control Flow Predictor (struct Predictor): Predicts outcomes of control flow instructions
- Return Address Stack (struct ReturnStack): 4-deep stack for JALP/RET instructions
- Forwarding Bus: Separate buses for each functional unit
- Function Units:
  - IntFU: For integer, logical, and control flow operations
  - MUL: 4-stage pipeline for multiplication
  - MEM: 3-stage pipeline for memory operations

Associated Functions:
- Lookup: Checks predictor table for control flow instructions
  - Arguments: Instruction address
  - Returns: Predicted outcome and target address

- establish_predictor_entry: Creates new entry in predictor table
  - Arguments: Instruction type, address
  - Returns: void

- update_address_into_predictor_field: Manages return address stack
  - Arguments: Instruction type, address
  - Returns: void

- flush_speculative_inst: Clears speculative instructions on misprediction
  - Arguments: void
  - Returns: void

### struct Predictor
Fields:
- Instruction type: Type of control flow instruction
- Target Address: Predicted jump address
- Prediction status: Recent prediction outcome
- FIFO table: 8 entries for recent control flow instructions

Associated Functions:
- Lookup: Checks for existing prediction
- establish_predictor_entry: Creates new prediction entry
- update_address_into_predictor_field: Updates prediction information
- flush_speculative_inst: Removes incorrect predictions

### Pipeline Stages and Functions

#### Fetch Stage
Functions:
- Fetch_Stage: Retrieves instructions from memory
- Arguments: CPU
- Returns: void

Associated Operations:
- Uses Control Flow predictor for prediction
- Fetches based on predicted path or sequential execution

#### Decode Stage
Functions:
- D1: First decode stage for instruction identification
- D2: Second decode stage for register renaming
- Arguments: CPU
- Returns: void

#### Dispatch Stage
Functions:
- Dispatch: Routes instructions to appropriate reservation stations
- Issue_Reservation_Station: Issues ready instructions
- Arguments: CPU
- Returns: void

#### Execute Stage
Functions:
- Execute_IntFU: For integer and control flow operations
- Execute_MUL: For multiplication operations
- MEM: For memory operations
- Data_Forwarding: Handles result forwarding
- Arguments: CPU
- Returns: void

#### Commit Stage
Functions:
- Commit_Instruction: Finalizes instruction execution
- Deallocate_Resources: Frees up used resources
- Checkpoint_State: Creates recovery points
- Restore_Checkpoint: Restores state after misprediction
- Arguments: CPU
- Returns: void

### struct IRS_Entry
Fields:
- rs1, rs2, rd: Architectural and physical registers
- value and ready bit
- counter
- rob entry id
- free bit

### struct MRS_Entry
Fields:
- rs1, rs2, rd: Architectural and physical registers
- value and ready bit
- counter
- rob entry id
- free bit

### struct LSQ_Entry
Fields:
- rs1, rs2, rd: Architectural and physical registers
- value and ready bit
- counter
- rob entry id
- free bit
- mem address
- mem add valid

### struct ROB_Entry
Fields:
- rs1, rs2: Architectural and physical registers
- free bit
- status

### Forwarding Bus
Fields:
- register
- register value
- zero flag

Associated Functions:
- get_rs1_value: Checks operand readiness for rs1
- get_rs2_value: Checks operand readiness for rs2
- Arguments: Register number
- Returns: Register value if ready

### Control Flow Handling
Functions:
1. Lookup:
   - Receives instruction address
   - Searches predictor table
   - Returns prediction or default behavior

2. establish_predictor_entry:
   - Creates new predictor entry
   - Initializes prediction based on offset
   - Manages table space using FIFO

3. update_address_into_predictor_field:
   - Manages return address stack
   - Handles JALP/RET instructions
   - Updates prediction information

4. flush_speculative_inst:
   - Removes speculative instructions
   - Restores processor state
   - Ensures execution correctness

## Flow of Execution
1. Fetch Stage retrieves instruction
2. Decode Stages (D1, D2) process instruction
3. Dispatch routes to appropriate reservation station
4. Execute Stage processes in functional units
5. Results forwarded to dependent instructions
6. Commit Stage ensures in-order completion
