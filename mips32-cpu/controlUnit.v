
module controlUnit(
    input wire [5:0] opcode,
    input wire [5:0] funct,
    output reg regWrite,
    output reg aluSrc,
    output reg [3:0] aluOp,
    output reg memWrite,
    output reg memRead,
    output reg memtoReg,
    output reg isBranch,
    output reg isImmd
);

    // ALU control: translate R-type funct field to internal aluOp
    reg [3:0] aluCtrl;
    always @(*) begin
        case (funct)
            6'd32:   aluCtrl = 4'b0000; // add
            6'd34:   aluCtrl = 4'b0001; // sub
            6'd36:   aluCtrl = 4'b0010; // and
            6'd37:   aluCtrl = 4'b0011; // or
            6'd42:   aluCtrl = 4'b0101; // slt
            default: aluCtrl = 4'b0000;
        endcase
    end

    always @(*) begin
        case (opcode)
            6'b000000: begin // R-type
                regWrite = 1;
                aluSrc = 0;
                aluOp = aluCtrl;
                memWrite = 0;
                memRead = 0;
                memtoReg = 0;
                isBranch = 0;
                isImmd = 0;
            end
            6'b100011: begin // LW
                regWrite = 1;
                aluSrc = 1;
                aluOp = 4'b0000; // ADD for address calculation
                memWrite = 0;
                memRead = 1;
                memtoReg = 1;
                isBranch = 0;
                isImmd = 1;
            end
            6'b101011: begin // SW
                regWrite = 0;
                aluSrc = 1;
                aluOp = 4'b0000; // ADD for address calculation
                memWrite = 1;
                memRead = 0;
                memtoReg = 0;
                isBranch = 0;
                isImmd = 1;
            end
            6'b000100: begin // BEQ
                regWrite = 0;
                aluSrc = 0;
                aluOp = 4'b0001; // SUB for comparison
                memWrite = 0;
                memRead = 0;
                memtoReg = 0;
                isBranch = 1;
                isImmd = 1;
            end
            6'b001000: begin // ADDI
                regWrite = 1;
                aluSrc = 1;
                aluOp = 4'b0000; // ADD for immediate addition
                memWrite = 0;
                memRead = 0;
                memtoReg = 0;
                isBranch = 0;
                isImmd = 1;
            end
            6'b001001: begin // SUBI
                regWrite = 1;
                aluSrc = 1;
                aluOp = 4'b0001; // SUB for immediate subtraction
                memWrite = 0;
                memRead = 0;
                memtoReg = 0;
                isBranch = 0;
                isImmd = 1;
            end
            default: begin // NOP or undefined instruction
                regWrite = 0;
                aluSrc = 0;
                aluOp = 4'b0000;
                memWrite = 0;
                memRead = 0;
                memtoReg = 0;
                isBranch = 0;
                isImmd = 0;
            end
        endcase
    end
endmodule
