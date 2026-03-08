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

    default: /* NOP */;

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

    default: /* NOP */;

    endcase

    return next_addr;

endfunction


function bit [7:0] get_boundary(input bit [2:0] hsize, bit [2:0] hburst);

    bit [7:0] temp;

    case(hsize)

    `BYTE: begin

        case(hburst)

        `WRAP4: temp = 1*4; 

        `WRAP8: temp = 1*8;

        `WRAP16: temp = 1*16;

        default: /* NOP */;

        endcase
    end

    `HALFWORD: begin

        case(hburst)

        `WRAP4: temp = 2*4; 

        `WRAP8: temp = 2*8;

        `WRAP16: temp = 2*16;

        default: /* NOP */;

        endcase
    end

    `WORD: begin

        case(hburst)

        `WRAP4: temp = 4*4; 

        `WRAP8: temp = 4*8;

        `WRAP16: temp = 4*16;

        default: /* NOP */;

        endcase
    end

    default: /* NOP */;

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

    default: /* NOP */;

    endcase

endfunction


function bit [31:0] rd_single_tr(input bit [2:0] hsize, bit [31:0] rd_addr, output bit [31:0] rddata);

    case(hsize)

    `BYTE: begin
        rddata[7:0] = mem[rd_addr];
        return rd_addr;
    end

    `HALFWORD: begin
        rddata[7:0] = mem[rd_addr];
        rddata[15:8] = mem[rd_addr + 1];
        return rd_addr;
    end

    `WORD: begin
        rddata[7:0] = mem[rd_addr];
        rddata[15:8] = mem[rd_addr + 1];
        rddata[23:16] = mem[rd_addr + 2];
        rddata[31:24] = mem[rd_addr + 3];
        return rd_addr;
    end

    default: /* NOP */;

    endcase
endfunction


function bit [31:0] rd_incr_tr(input bit [2:0] hsize, bit [31:0] rd_addr,output bit [31:0] rddata);
    bit [31:0] next_addr;

    case(hsize)

    `BYTE: begin
        rddata[7:0] = mem[rd_addr];
        next_addr = rd_addr + 1;
    end

    `HALFWORD: begin
        rddata[7:0] = mem[rd_addr];
        rddata[15:8] = mem[rd_addr + 1];
        next_addr = rd_addr + 2;
    end

    `WORD: begin
        rddata[7:0] = mem[rd_addr];
        rddata[15:8] = mem[rd_addr + 1];
        rddata[23:16] = mem[rd_addr + 2];
        rddata[31:24] = mem[rd_addr + 3];
        next_addr = rd_addr + 4;
    end

    default: /* NOP */;

    endcase

    return next_addr;
endfunction


function bit [31:0] rd_wrap_tr(input bit [7:0] boundary, bit [2:0] hsize, bit[31:0] rd_addr,output bit [31:0] rddata);

    bit [31:0] addr0, addr1, addr2, addr3;

    case(hsize)

    `BYTE: begin

    rddata[7:0] = mem[rd_addr];

    if((rd_addr + 1) % 32'(boundary) == 0)
        addr0 = (rd_addr + 1) - 32'(boundary);
    else
        addr0 = (rd_addr + 1);

        return addr0;
    end

    `HALFWORD: begin

        rddata[7:0] = mem[rd_addr];

        if((rd_addr + 1) % 32'(boundary) == 0)
            addr0 = (rd_addr + 1) - 32'(boundary);
        else
            addr0 = (rd_addr + 1);

        rddata[15:8] = mem[addr0];

        if((addr0 + 1) % 32'(boundary) == 0)
            addr1 = (addr0 + 1) - 32'(boundary);
        else
            addr1 = (addr0 + 1);    

        return addr1;

    end


    `WORD: begin

        rddata[7:0] = mem[rd_addr];

        if((rd_addr + 1) % 32'(boundary) == 0)
            addr0 = (rd_addr + 1) - 32'(boundary);
        else
            addr0 = (rd_addr + 1);

        rddata[15:8] = mem[addr0];

        if((addr0 + 1) % 32'(boundary) == 0)
            addr1 = (addr0 + 1) - 32'(boundary);
        else
            addr1 = (addr0 + 1);    
        
        rddata[23:16] = mem[addr1];

        if((addr1 + 1) % 32'(boundary) == 0)
            addr2 = (addr1 + 1) - 32'(boundary);
        else
            addr2 = (addr1 + 1);

        rddata[31:24] = mem[addr2];

        if((addr2 + 1) % 32'(boundary) == 0)
            addr3 = (addr2 + 1) - 32'(boundary);
        else
            addr3 = (addr2 + 1);

        return addr3;
    end

    default: /* NOP */;

    endcase

endfunction
