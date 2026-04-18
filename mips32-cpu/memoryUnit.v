
module memoryUnit(
    input wire [31:0] address,
    input wire [31:0] writeData,
    input wire memWrite,
    input wire memRead,
    input wire clk,
    input wire reset,
    output reg [31:0] readData
);

    // Sink unused address bits to suppress warnings
    wire _unused = &{1'b0, address[31:12], address[1:0]};

    reg [31:0] memory [0:1023]; // 1024 words of 32 bits each

    integer i;

    // Initialize memory on reset
    always @(posedge clk or posedge reset) begin
        if (reset) begin
            for (i = 0; i < 1024; i = i + 1) begin
                memory[i] <= 32'b0;
            end
        end else if (memWrite) begin
            memory[address[11:2]] <= writeData;
        end else if (memRead) begin
            readData <= memory[address[11:2]];
        end else begin
            readData <= 32'b0;
        end
    end
endmodule
