/* Transafers types (HTRANS) */
`define IDLE        2'b00
`define BUSY        2'b01
`define NONSEQ      2'b10
`define SEQ         2'b11

/* Subordinate response */
`define OKAY        2'b00
`define ERROR       2'b11

/* Bus widths */
`define ADDR_WIDTH  32
`define DATA_WIDTH  32
`define BURST_WIDTH 3

/* Types of Bursts (HBURSTS) */
`define SINGLE      3'b000
`define INCR        3'b001
`define WRAP4       3'b010
`define INCR4       3'b011
`define WRAP8       3'b100
`define INCR8       3'b101
`define WRAP16      3'b110
`define INCR16      3'b111

/* Supported transfer size (HSIZE) */
`define BYTE        3'b000
`define HALFWORD    3'b001
`define WORD        3'b010


/* Byte addressable memory */
reg [7:0] mem[256];
