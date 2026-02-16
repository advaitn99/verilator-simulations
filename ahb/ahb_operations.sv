`include "ahb_params.svh"


function bit [31:0] wr_single_tr(input bit [2:0] hsize, bit [31:0] wr_addr, bit [31:0] hwdata);
    case(hsize)

    `BYTE: begin
        mem[wr_addr] = hwdata[7:0];
    end

    `HALFWORD: begin
        mem[wr_addr]     = hwdata[7:0];
        mem[wr_addr + 1] = hwdata[15:8];
    end

    `WORD: begin
        mem[wr_addr]     = hwdata[7:0];
        mem[wr_addr + 1] = hwdata[15:8];
        mem[wr_addr + 2] = hwdata[23:16];
        mem[wr_addr + 3] = hwdata[31:24];
    end

    endcase

    return wr_addr;

endfunction


function bit [31:0] wr_incr_tr(input bit [2:0] hsize, bit [31:0] wr_addr, bit [31:0] hwdata);
    bit [31:0] next_addr;

    case(hsize)

    `BYTE: begin
        mem[wr_addr] = hwdata[7:0];
        next_addr = wr_addr + 1;
    end

    `HALFWORD: begin
        mem[wr_addr]     = hwdata[7:0];
        mem[wr_addr + 1] = hwdata[15:8];
        next_addr = wr_addr + 2;
    end

    `WORD: begin
        mem[wr_addr]     = hwdata[7:0];
        mem[wr_addr + 1] = hwdata[15:8];
        mem[wr_addr + 2] = hwdata[23:16];
        mem[wr_addr + 3] = hwdata[31:24];
        next_addr = wr_addr + 4;
    end

    endcase

    return next_addr;

endfunction


function bit [7:0] get_boundary(input bit [2:0] hsize, bit [2:0] hburst, bit [31:0] wr_addr);

    bit [7:0] temp;

    case(hsize)

    `BYTE: begin

        case(hburst)

        `WRAP4: temp = 1*4; 

        `WRAP8: temp = 1*8;

        `WRAP16: temp = 1*16;

        endcase
    end

    `HALFWORD: begin

        case(hburst)

        `WRAP4: temp = 2*4; 

        `WRAP8: temp = 2*8;

        `WRAP16: temp = 2*16;

        endcase
    end

    `WORD: begin

        case(hburst)

        `WRAP4: temp = 4*4; 

        `WRAP8: temp = 4*8;

        `WRAP16: temp = 4*16;

        endcase
    end

    endcase

    return temp;
endfunction


function bit [31:0] wr_wrap_tr(input bit [7:0] boundary, bit [2:0] hsize, bit[31:0] wr_addr, bit [31:0] hwdata);

    bit [31:0] addr0, addr1, addr2, addr3;

    case(hsize)

    `BYTE: begin

    mem[wr_addr] = hwdata[7:0];

    if((wr_addr + 1) % 32'(boundary) == 0)
        addr0 = (wr_addr + 1) - 32'(boundary);
    else
        addr0 = (wr_addr + 1);

        return addr0;
    end

    `HALFWORD: begin

        mem[wr_addr]     = hwdata[7:0];

        if((wr_addr + 1) % 32'(boundary) == 0)
            addr0 = (wr_addr + 1) - 32'(boundary);
        else
            addr0 = (wr_addr + 1);

        mem[addr0] = hwdata[15:8];

        if((addr0 + 1) % 32'(boundary) == 0)
            addr1 = (addr0 + 1) - 32'(boundary);
        else
            addr1 = (addr0 + 1);    

        return addr1;

    end


    `WORD: begin

        mem[wr_addr]     = hwdata[7:0];

        if((wr_addr + 1) % 32'(boundary) == 0)
            addr0 = (wr_addr + 1) - 32'(boundary);
        else
            addr0 = (wr_addr + 1);

        mem[addr0] = hwdata[15:8];

        if((addr0 + 1) % 32'(boundary) == 0)
            addr1 = (addr0 + 1) - 32'(boundary);
        else
            addr1 = (addr0 + 1);
        
        mem[addr1] = hwdata[23:16];

        if((addr1 + 1) % 32'(boundary) == 0)
            addr2 = (addr1 + 1) - 32'(boundary);
        else
            addr2 = (addr1 + 1);

        mem[addr2] = hwdata[31:24];

        if((addr2 + 1) % 32'(boundary) == 0)
            addr3 = (addr2 + 1) - 32'(boundary);
        else
            addr3 = (addr2 + 1);

        return addr3;
    end

    endcase

endfunction
