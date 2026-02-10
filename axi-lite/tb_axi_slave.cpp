#include <stdlib.h>
#include <iostream>
#include <verilated.h>
#include <verilated_vcd_c.h>
#include "Vaxi_slave.h"
#include <random>
#include <chrono>


#define MAX_SIM_TIME 2000
#define NUM_TESTS    5

vluint64_t sim_time = 0;
int num_test = 0;
VerilatedVcdC *m_trace;
Vaxi_slave *dut;


// 1. Seed the random number engine
// Use a time-based seed for different results each time the program runs
unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
std::mt19937 engine(seed); // Mersenne Twister engine is a common choice

// 2. Define the distribution
// Creates a uniform distribution for integers between 1 and 100 (inclusive)
std::uniform_int_distribution<int> dist_addr(1, 100);

std::uniform_int_distribution<int> dist_data(1, 20);

std::uniform_int_distribution<int> dist_op(0, 1);


void tick(void)
{
    dut->aclk ^= 1;
    dut->eval();
    m_trace->dump(sim_time);
    sim_time++;
}


class transaction
{
    public:
        uint8_t op;      /* Operation select */
        uint32_t awaddr; /* Write address */
        uint32_t wdata;  /* Write data */
        uint32_t araddr; /* Read address */
        uint32_t rdata;  /* Read data */
        uint8_t bresp;   /* Write response */
        uint8_t rresp;   /* Read response */
};


class generator
{
    private:
        transaction *tx = new transaction();

    public:
        transaction* generate_sequence()
        {
            tx->op     = rand() % 2;
            tx->awaddr = rand() % 100;
            tx->wdata  = rand() % 20;
            tx->araddr = rand() % 100;

            return tx;
        }
};


class driver
{
    private:
        Vaxi_slave *dut;

    public:

        driver(Vaxi_slave *dut)
        {
            this->dut = dut;
        }

        void dut_reset()
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

        void dut_write(transaction *trans)
        {
            if(trans != NULL)
            {
                /* Disable read signals */
                dut->s_arvalid = 0;
                dut->s_araddr = 0;
                dut->s_rready = 0;

                /* Send write addr to DUT */
                dut->s_awvalid = 1;
                dut->s_awaddr = trans->awaddr;

                /* Wait for data write ready from slave */
                while(dut->s_wready != 1)
                {
                    tick();
                    tick();
                }

                /* Send write data to DUT and be ready for response */
                dut->s_awvalid = 0;
                dut->s_awaddr = 0;
                dut->s_wvalid = 1;
                dut->s_wdata = trans->wdata;
                dut->s_bready = 1;

                while(dut->s_bvalid != 1)
                {
                    tick();
                    tick();
                }

                dut->s_wvalid = 0;
                dut->s_wdata = 0;
                dut->s_bready = 0;

                while(dut->s_bvalid != 0)
                {
                    tick();
                    tick();
                }
            }
        }

        void dut_read(transaction* trans)
        {
            /* Disable write signals */
            dut->s_awaddr = 0;
            dut->s_awvalid = 0;
            dut->s_wdata = 0;
            dut->s_wvalid = 0;
            dut->s_bready = 0;

            /* Send read address to DUT and be ready for response */
            dut->s_araddr = trans->araddr;
            dut->s_arvalid = 1;
            dut->s_rready = 1;

            while(dut->s_arready != 1)
            {
                tick();
                tick();
            }

            while(dut->s_rvalid != 1)
            {
                tick();
                tick();
            }

            dut->s_arvalid = 0;
            dut->s_araddr = 0;
            dut->s_rready = 0;

            while(dut->s_rvalid != 0)
            {
                tick();
                tick();
            }
        }

        void drive(transaction *trans)
        {
            if(trans->op == 1)
            {
                dut_write(trans);
            }
            else
            {
                dut_read(trans);
            }
        }
};


int main(int argc, char** argv, char** env) {
    dut = new Vaxi_slave;
    Verilated::traceEverOn(true);
    m_trace = new VerilatedVcdC;
    dut->trace(m_trace, 5);
    m_trace->open("waveform.vcd");

    transaction *t;
    generator *g = new generator();
    driver *d = new driver(dut);

    dut->aclk = 0;

    //std::cout<<"#### RESET DUT ####"<<std::endl;
    d->dut_reset();

    while(num_test < NUM_TESTS)
    {

        /* Next posedge */
        while(dut->aclk != 0)
        {
            tick();
        }
        tick();

        //std::cout<<"#### GENERATE SEQUENCE  ####"<<std::endl;
        t = g->generate_sequence();
        //std::cout<<"op: "<<t->op<<" awaddr: "<<t->awaddr<<" wdata: "<<t->wdata<<" araddr: "<<t->araddr<<std::endl;

        d->drive(t);

        num_test++;
    }

    std::cout<<"NUM_TEST: "<<num_test<<std::endl;

    m_trace->close();
    delete dut;
    exit(EXIT_SUCCESS);
}
