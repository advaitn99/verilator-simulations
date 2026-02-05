#include <stdlib.h>
#include <iostream>
#include <verilated.h>
#include <verilated_vcd_c.h>
#include "Vaxi_slave.h"


#define MAX_SIM_TIME 2000
#define CLK_FREQ_MHZ 1
#define CLK_HALF_PERIOD_US (500/CLK_FREQ_MHZ)     

vluint64_t sim_time = 0;
VerilatedVcdC *m_trace;
Vaxi_slave *dut;
static double timestamp = 0;

void tick(void)
{
    dut->aclk ^= 1;
    dut->eval();
    m_trace->dump(sim_time);
    sim_time++;
}

void single_clk_period(void)
{
    tick();
    tick();
}

void reset_dut()
{
    int half_clk_counts = 0;

    /* Assert reset */
    dut->areset = 0;
    
    /* Keep reset for 5 clk cycles */
    while(half_clk_counts < 10)
    {
        tick();
        half_clk_counts++;
    }

    /* Deassert reset */
    dut->areset = 1;
}


void send_write_address(long int addr)
{
    dut->s_awvalid = 1;
    dut->s_awaddr = addr;
}

void send_write_data(long int data)
{
    dut->s_awvalid = 0;
    if(dut->s_wready == 1)
    {
        dut->s_wvalid = 1;
        dut->s_wdata = data;
    }
}

void get_write_resp(void)
{
    dut->s_wvalid = 0;
    dut->s_bready = 1;
}

int main(int argc, char** argv, char** env) {
    dut = new Vaxi_slave;
    Verilated::traceEverOn(true);
    m_trace = new VerilatedVcdC;
    dut->trace(m_trace, 5);
    m_trace->open("waveform.vcd");

    dut->aclk = 0;

    /* Send the reset sequence to the AXI slave */
    reset_dut();

    single_clk_period();
    
    /* Send write response */
    send_write_address(130);

    /* Wait for data write ready from slave */
    while(dut->s_wready != 1)
    {
        single_clk_period();
    }

    /* Send write data to slave */
    send_write_data(20);

    single_clk_period();
    
    get_write_resp();

    while(sim_time < MAX_SIM_TIME)
    {
        single_clk_period();
        
        if(dut->s_bvalid == 1)
        {
            break;
        }
    }
    
    while(sim_time < MAX_SIM_TIME)
    {
        single_clk_period();
    }

    m_trace->close();
    delete dut;
    exit(EXIT_SUCCESS);
}
