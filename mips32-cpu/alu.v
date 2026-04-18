
module alu(
    input wire [31:0] a,
    input wire [31:0] b,
    input wire [3:0] aluOp,
    output reg [31:0] result,
    output wire isZero
);

    assign isZero = (result == 32'b0);

    always @(*) begin
        case (aluOp)
            4'b0000: result = a + b;                                      // ADD
            4'b0001: result = a - b;                                      // SUB
            4'b0010: result = a & b;                                      // AND
            4'b0011: result = a | b;                                      // OR
            4'b0100: result = a ^ b;                                      // XOR
            4'b0101: result = ($signed(a) < $signed(b)) ? 32'd1 : 32'd0; // SLT
            default: result = 32'b0;
        endcase
    end
endmodule
