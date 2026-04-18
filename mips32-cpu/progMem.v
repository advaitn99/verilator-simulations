
module progMem(
    input wire [31:0] pc, // Program Counter
    output wire [31:0] instruction,
    output wire [31:0] nextPC
);

    reg [7:0] memory [0:511]; // 512 words of 8 bits each

    initial begin
       $readmemh("program.hex", memory); // Load program from hex file 
    end

    /* Combinatorial read: instruction and nextPC are available within the same cycle as pc.
       The pc register and branch mux live in cpu_top. */
    assign instruction = {memory[pc+3], memory[pc+2], memory[pc+1], memory[pc]};
    assign nextPC      = pc + 4;

endmodule
