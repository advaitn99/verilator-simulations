`include "ahb_operations.sv"


module ahb_slave(
    input wire hclk,
    input wire hresetn,
    input wire [`ADDR_WIDTH-1 : 0] s_ahb_haddr,
    input wire [`DATA_WIDTH-1 : 0] s_ahb_hwdata,
    input wire [`BURST_WIDTH-1 : 0] s_ahb_hburst,
    input wire [2:0] s_ahb_hsize,
    input wire [1:0] s_ahb_htrans,
    input wire s_ahb_hwrite,
    input wire s_ahb_hsel,
    output reg [`DATA_WIDTH-1 : 0] s_ahb_rdata,
    output reg [1:0] s_ahb_hresp,
    output reg s_ahb_hready
);

    reg [2:0] state, next_state;
    localparam idle                 = 0,
               control_phase        = 1,
               addr_phase           = 2,
               data_write_phase     = 3,
               data_read_read       = 4;

    integer burst_count = 0;
    reg first = 0;
    reg [31:0] next_addr = 0;
    reg [31:0] ret_addr = 0;
    reg [7:0] boundary = 0;



    always_ff @(posedge hclk or negedge hresetn) begin
        if(!hresetn) begin
            state <= idle;
        end
        else begin
            state <= next_state;
        end
    end

    always_comb begin

        case(state)

        idle:
        begin
            s_ahb_hready = 1'b0;
            burst_count = 0;
            first = 0;
            s_ahb_hresp = `OKAY;
            next_state = control_phase;
        end

        control_phase:
        begin
            s_ahb_hready = 1'b0;

            if(hresetn && s_ahb_hsel && s_ahb_hwrite) begin
                if(s_ahb_haddr < 256) begin
                    next_state = addr_phase;
                end
                else begin
                    next_state = idle;
                    s_ahb_hresp = `ERROR;
                end
            end
            else if(hresetn && s_ahb_hsel && !s_ahb_hwrite) begin
                if(s_ahb_haddr < 256) begin
                    next_state = addr_phase;
                end
                else begin
                    next_state = idle;
                    s_ahb_hresp = `ERROR;
                end
            end
            else begin
                next_state = idle;
            end
            

        end

        addr_phase: 
        begin
            if(s_ahb_htrans == `NONSEQ) begin
                next_addr = s_ahb_haddr;
                if(s_ahb_hwrite)
                    next_state = data_write_phase;
                else
                    next_state = data_read_read;
            end

            else if(s_ahb_htrans == `SEQ) begin
                next_addr = ret_addr;
                if(s_ahb_hwrite)
                    next_state = data_write_phase;
                else
                    next_state = data_read_read;
            end
        end

        data_write_phase:
        begin
            case(s_ahb_hburst)

            `SINGLE: begin
            end

            `INCR: begin
            end

            `WRAP4: begin
            end

            `INCR4: begin
            end

            `WRAP8: begin
            end

            `INCR8: begin
            end

            `WRAP16: begin
            end

            `INCR16: begin
            end
            endcase
        end

        data_read_read:
        begin
        end
        endcase
    end

endmodule
