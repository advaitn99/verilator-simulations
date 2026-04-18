
module hazard_det_unit(
    input wire [4:0] id_ex_rs,       // Source reg 1 of instr in EX stage
    input wire [4:0] id_ex_rt,       // Source reg 2 of instr in EX stage
    input wire ex_mem_regWrite,      // RegWrite from EX/MEM pipeline reg
    input wire [4:0] ex_mem_rd,      // Dest reg from EX/MEM pipeline reg
    input wire mem_wb_regWrite,      // RegWrite from MEM/WB pipeline reg
    input wire [4:0] mem_wb_rd,      // Dest reg from MEM/WB pipeline reg
    output reg [1:0] forwardA,
    output reg [1:0] forwardB
);

    // ForwardA mux control (ALU operand A / rs)
    always @(*) begin
        if (ex_mem_regWrite && (ex_mem_rd != 0) && (ex_mem_rd == id_ex_rs))
            forwardA = 2'b10; // Forward from EX/MEM
        else if (mem_wb_regWrite && (mem_wb_rd != 0) && (mem_wb_rd == id_ex_rs))
            forwardA = 2'b01; // Forward from MEM/WB
        else
            forwardA = 2'b00; // No forwarding
    end

    // ForwardB mux control (ALU operand B / rt)
    always @(*) begin
        if (ex_mem_regWrite && (ex_mem_rd != 0) && (ex_mem_rd == id_ex_rt))
            forwardB = 2'b10; // Forward from EX/MEM
        else if (mem_wb_regWrite && (mem_wb_rd != 0) && (mem_wb_rd == id_ex_rt))
            forwardB = 2'b01; // Forward from MEM/WB
        else
            forwardB = 2'b00; // No forwarding
    end

endmodule
