

module branchUnit(
    input wire [31:0] pc,
    input wire [15:0] imm,
    input wire isBranch,
    input wire isZero,
    output wire [31:0] branchTarget,
    output wire isBranchTaken
);

    assign isBranchTaken = isBranch && isZero;
    assign branchTarget  = pc + {{14{imm[15]}}, imm, 2'b00}; // Sign-extend and shift left by 2
endmodule
