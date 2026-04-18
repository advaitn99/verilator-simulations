

module cpu_top(
    input wire clk,
    input wire reset
);

    /*  Pipeline registers */

    // IF/ID pipeline register
    reg [31:0] IF_ID_instruction;
    reg [31:0] IF_ID_pcPlus4;

    // ID/EX pipeline register
    reg [31:0] ID_EX_readData1, ID_EX_readData2, ID_EX_signExtImm;
    reg [4:0] ID_EX_rd, ID_EX_rs, ID_EX_rt;
    reg [3:0] ID_EX_aluOp;
    reg ID_EX_regWrite, ID_EX_memRead, ID_EX_memWrite;
    reg [31:0] ID_EX_pcPlus4;
    reg ID_EX_isBranch, ID_EX_aluSrc, ID_EX_memtoReg, ID_EX_isImmd;

    // EX/MEM pipeline register
    reg [31:0] EX_MEM_aluResult, EX_MEM_writeData;
    reg [4:0] EX_MEM_rd, EX_MEM_rs, EX_MEM_rt;
    reg EX_MEM_regWrite, EX_MEM_memRead, EX_MEM_memWrite, EX_MEM_memtoReg, EX_MEM_isImmd;

    // MEM/WB pipeline register
    reg [31:0] MEM_WB_readData, MEM_WB_aluResult;
    reg [4:0] MEM_WB_rd, MEM_WB_rt;

    /* verilator lint_off UNUSED */
    reg [4:0] MEM_WB_rs;
    /* verilator lint_on UNUSED */

    reg MEM_WB_regWrite;
    reg MEM_WB_memtoReg;
    reg MEM_WB_isImmd;

    // PC register
    reg [31:0] pc;

    /* Intermediate wires */
    wire [31:0] if_instruction, if_pcPlus4;
    wire [31:0] id_readData1, id_readData2;
    wire        ctrl_regWrite, ctrl_aluSrc, ctrl_memWrite, ctrl_memRead, ctrl_memtoReg, ctrl_isBranch, ctrl_isImmd;
    wire [3:0]  ctrl_aluOp;
    wire [31:0] ex_aluInputB;
    wire [31:0] ex_aluResult;
    wire        ex_aluZero;
    wire [31:0] mem_readData;
    wire [31:0] wb_writeData;
    wire [4:0] wb_writeReg;
    wire [31:0] branchTarget;
    wire        isBranchTaken;

    /* verilator lint_off UNUSED */
    wire [1:0] forwardA, forwardB;
    /* verilator lint_on UNUSED */


    /* Write-back mux */
    assign wb_writeData = MEM_WB_memtoReg ? MEM_WB_readData : MEM_WB_aluResult;

    /* Write back register mux */
    assign wb_writeReg = (MEM_WB_isImmd) ? MEM_WB_rt : MEM_WB_rd;  // For I-type, rt is the dest reg

    /* ALU source mux */
    assign ex_aluInputB = ID_EX_aluSrc ? ID_EX_signExtImm : ID_EX_readData2;


    /* Module instances */
    progMem prog_memory (
        .pc(pc),
        .instruction(if_instruction),
        .nextPC(if_pcPlus4)
    );

    regFile register_file (
        .readReg1(IF_ID_instruction[25:21]),  // Read in ID stage
        .readReg2(IF_ID_instruction[20:16]),  // Read in ID stage
        .writeReg(wb_writeReg),              // Write in WB stage
        .writeData(wb_writeData),
        .regWrite(MEM_WB_regWrite),
        .clk(clk),
        .reset(reset),
        .readData1(id_readData1),
        .readData2(id_readData2)
    );

    controlUnit control_unit (
        .opcode(IF_ID_instruction[31:26]),
        .funct(IF_ID_instruction[5:0]),
        .regWrite(ctrl_regWrite),
        .aluSrc(ctrl_aluSrc),
        .aluOp(ctrl_aluOp),
        .memWrite(ctrl_memWrite),
        .memRead(ctrl_memRead),
        .memtoReg(ctrl_memtoReg),
        .isBranch(ctrl_isBranch),
        .isImmd(ctrl_isImmd)
    );

    alu arithmetic_logic_unit (
        .a(ID_EX_readData1),
        .b(ex_aluInputB),
        .aluOp(ID_EX_aluOp),
        .result(ex_aluResult),
        .isZero(ex_aluZero)
    );

    memoryUnit data_memory (
        .address(EX_MEM_aluResult),
        .writeData(EX_MEM_writeData),
        .memWrite(EX_MEM_memWrite),
        .memRead(EX_MEM_memRead),
        .clk(clk),
        .reset(reset),
        .readData(mem_readData)
    );

    branchUnit branch_unit (
        .pc(ID_EX_pcPlus4),
        .imm(ID_EX_signExtImm[15:0]),
        .isBranch(ID_EX_isBranch),
        .isZero(ex_aluZero),
        .branchTarget(branchTarget),
        .isBranchTaken(isBranchTaken)
    );

    hazard_det_unit hazard_detection_unit (
        .id_ex_rs(ID_EX_rs),
        .id_ex_rt(ID_EX_rt),
        .ex_mem_regWrite(EX_MEM_regWrite),
        .ex_mem_rd(EX_MEM_rd),
        .mem_wb_regWrite(MEM_WB_regWrite),
        .mem_wb_rd(MEM_WB_rd),
        .forwardA(forwardA),
        .forwardB(forwardB)
    );


    /* Pipeline stage latching */
    always @(posedge clk or posedge reset) begin
        if (reset) begin
            pc               <= 32'b0;
            IF_ID_instruction <= 32'b0;
            IF_ID_pcPlus4    <= 32'b0;

            ID_EX_readData1  <= 32'b0;
            ID_EX_readData2  <= 32'b0;
            ID_EX_signExtImm <= 32'b0;
            ID_EX_rd         <= 5'b0;
            ID_EX_rs         <= 5'b0;
            ID_EX_rt         <= 5'b0;
            ID_EX_aluOp      <= 4'b0;
            ID_EX_regWrite   <= 0;
            ID_EX_memRead    <= 0;
            ID_EX_memWrite   <= 0;
            ID_EX_pcPlus4    <= 32'b0;
            ID_EX_isBranch   <= 0;
            ID_EX_aluSrc     <= 0;
            ID_EX_memtoReg   <= 0;
            ID_EX_isImmd     <= 0;
            EX_MEM_aluResult <= 32'b0;
            EX_MEM_writeData <= 32'b0;
            EX_MEM_rd        <= 5'b0;
            EX_MEM_rs        <= 5'b0;
            EX_MEM_rt        <= 5'b0;
            EX_MEM_regWrite  <= 0;
            EX_MEM_memRead   <= 0;
            EX_MEM_memWrite  <= 0;
            EX_MEM_memtoReg  <= 0;
            EX_MEM_isImmd    <= 0;
            MEM_WB_readData  <= 32'b0;
            MEM_WB_aluResult <= 32'b0;
            MEM_WB_rd        <= 5'b0;
            MEM_WB_rs        <= 5'b0;
            MEM_WB_rt        <= 5'b0;
            MEM_WB_isImmd    <= 0;
            MEM_WB_regWrite  <= 0;
            MEM_WB_memtoReg  <= 0;
        end else begin
            // --- IF stage: advance PC ---
            pc               <= isBranchTaken ? branchTarget : if_pcPlus4;
            IF_ID_instruction <= if_instruction;
            IF_ID_pcPlus4    <= if_pcPlus4;

            // --- ID stage: decode & register read ---
            ID_EX_readData1  <= id_readData1;
            ID_EX_readData2  <= id_readData2;
            ID_EX_signExtImm <= {{16{IF_ID_instruction[15]}}, IF_ID_instruction[15:0]};
            ID_EX_rd         <= IF_ID_instruction[15:11];
            ID_EX_rs         <= IF_ID_instruction[25:21];
            ID_EX_rt         <= IF_ID_instruction[20:16];
            ID_EX_isImmd     <= ctrl_isImmd;
            ID_EX_aluOp      <= ctrl_aluOp;
            ID_EX_regWrite   <= ctrl_regWrite;
            ID_EX_memRead    <= ctrl_memRead;
            ID_EX_memWrite   <= ctrl_memWrite;
            ID_EX_pcPlus4    <= IF_ID_pcPlus4;
            ID_EX_isBranch   <= ctrl_isBranch;
            ID_EX_aluSrc     <= ctrl_aluSrc;
            ID_EX_memtoReg   <= ctrl_memtoReg;

            // --- EX stage: ALU result & forward control ---
            EX_MEM_aluResult <= ex_aluResult;
            EX_MEM_writeData <= ID_EX_readData2;
            EX_MEM_rd        <= ID_EX_rd;
            EX_MEM_rs        <= ID_EX_rs;
            EX_MEM_rt        <= ID_EX_rt;
            EX_MEM_isImmd    <= ID_EX_isImmd;
            EX_MEM_regWrite  <= ID_EX_regWrite;
            EX_MEM_memRead   <= ID_EX_memRead;
            EX_MEM_memWrite  <= ID_EX_memWrite;
            EX_MEM_memtoReg  <= ID_EX_memtoReg;

            // --- MEM stage: memory access & forward control ---
            MEM_WB_readData  <= mem_readData;
            MEM_WB_aluResult <= EX_MEM_aluResult;
            MEM_WB_rd        <= EX_MEM_rd;
            MEM_WB_rs        <= EX_MEM_rs;
            MEM_WB_rt        <= EX_MEM_rt;
            MEM_WB_isImmd    <= EX_MEM_isImmd;
            MEM_WB_regWrite  <= EX_MEM_regWrite;
            MEM_WB_memtoReg  <= EX_MEM_memtoReg;
        end
    end

endmodule
