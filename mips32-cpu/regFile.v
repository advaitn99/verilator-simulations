
module regFile(
    input wire [4:0] readReg1,
    input wire [4:0] readReg2,
    input wire [4:0] writeReg,
    input wire [31:0] writeData,
    input wire regWrite,
    input wire clk,
    input wire reset,
    output wire [31:0] readData1,
    output wire [31:0] readData2
);

    reg [31:0] registers [31:0]; // 32 registers of 32 bits each

    integer i;

    // Combinational reads
    assign readData1 = registers[readReg1];
    assign readData2 = registers[readReg2];

    // Clocked writes
    always @(posedge clk or posedge reset) begin
        if (reset) begin
            for (i = 0; i < 32; i = i + 1) begin
                registers[i] <= 32'b0;
            end
        end else if (regWrite && writeReg != 5'b0) begin
            registers[writeReg] <= writeData;
        end
    end
endmodule
