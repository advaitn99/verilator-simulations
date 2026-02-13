#include <stdlib.h>
#include <iostream>
#include <verilated.h>
#include <verilated_vcd_c.h>
#include "Vaxi_slave.h"
#include <random>
#include <chrono>
#include <thread>
#include <fstream>
#include <queue>

#define MAX_SIM_TIME 2000
#define NUM_TESTS    50

vluint64_t sim_time = 0;
int num_test = 0;
VerilatedVcdC *m_trace;
Vaxi_slave *dut;


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


class RandomNumberGenerator {
private:
    std::mt19937 engine;
public:
    RandomNumberGenerator() : engine(std::chrono::system_clock::now().time_since_epoch().count()) {}

    int generate_int(int min, int max) {
        std::uniform_int_distribution<int> dist(min, max);
        return dist(engine);
    }
};


class generator
{
    private:
        transaction *tx = new transaction();
        RandomNumberGenerator rng;

    public:
        transaction* generate_sequence()
        {
            tx->op     = rng.generate_int(0,1);
            tx->awaddr = rng.generate_int(80,100);
            tx->wdata  = rng.generate_int(1,20);
            tx->araddr = rng.generate_int(80,100);

            std::ofstream logfile("sim_logs.log", std::ios_base::out | std::ios_base::app);

            if (logfile.is_open()) 
            {
                logfile << "[GEN]: OP: " << (int)tx->op
                        << " AWADDR: " << (int)tx->awaddr
                        << " WDATA: " << (int)tx->wdata
                        << " ARADDR: "<< (int)tx->araddr
                <<std::endl;
                logfile.close();
            } else 
            {
                std::cerr << "Error: Unable to open log file." << std::endl;
            }

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
            std::ofstream logfile("sim_logs.log", std::ios_base::out | std::ios_base::app);
            if (logfile.is_open()) 
            {
                logfile << "[DRV]: OP: " << (int)trans->op
                        << " AWADDR: " << (int)trans->awaddr
                        << " WDATA: " << (int)trans->wdata
                        << " ARADDR: "<< (int)trans->araddr
                <<std::endl;
                logfile.close();
            } else 
            {
                std::cerr << "Error: Unable to open log file." << std::endl;
            }
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


class  monitor
{
    private:
        Vaxi_slave *dut;
        transaction *t1 = new transaction();
        transaction *t3;
        std::queue<transaction*> &mon_to_scb;

    public:
        monitor(Vaxi_slave *dut, std::queue<transaction*> &mon_to_scb): dut(dut), mon_to_scb(mon_to_scb)
        {
            //this->dut = dut;
        }

        void monitor_dut(transaction *t2)
        {
            if(t2->op == 1)
            {                
                t1->op = t2->op;
                /* store write address */
                t1->awaddr = t2->awaddr;
                t1->wdata = t2->wdata;


                t1->bresp = dut->s_bresp;
                mon_to_scb.push(t1);
                
                std::ofstream logfile("sim_logs.log", std::ios_base::out | std::ios_base::app);
                if (logfile.is_open()) 
                {
                    logfile << "[MON]: " << " BRESP: " << (int)t1->bresp <<std::endl;
                    logfile.close();
                } else 
                {
                    std::cerr << "Error: Unable to open log file." << std::endl;
                }

                while(dut->s_bvalid != 0)
                {
                    tick();
                    tick();
                }
            }
            else
            {
                t1->op = t2->op;
                t1->araddr = t2->araddr;

                t1->rdata  = dut->s_rdata;
                t1->rresp = dut->s_rresp;
                mon_to_scb.push(t1);

                std::ofstream logfile("sim_logs.log", std::ios_base::out | std::ios_base::app);
                if (logfile.is_open()) 
                {
                    logfile << "[MON]: " << " RDATA: " << (int)t1->rdata
                    << " RRESP: " << (int)t1->rresp <<std::endl;
                    logfile.close();
                } else 
                {
                    std::cerr << "Error: Unable to open log file." << std::endl;
                }

                while(dut->s_rvalid != 0)
                {
                    tick();
                    tick();
                }
            }
        }

};


class scoreboard
{
    private:
        transaction *trans;
        std::queue<transaction*> &mon_to_scb;
        uint32_t mem[128] = {0};

    public:

        scoreboard(std::queue<transaction*> &mon_to_scb): mon_to_scb(mon_to_scb) {}

        void scb_check()
        {
            
            if(!mon_to_scb.empty())
            {
                trans = mon_to_scb.front();
                mon_to_scb.pop();
                if(trans->op == 1)
                {
                    if(trans->bresp == 0)
                    {
                        if(trans->rdata == mem[trans->araddr])
                        {
                            std::ofstream logfile("sim_logs.log", std::ios_base::out | std::ios_base::app);
                            if (logfile.is_open()) 
                            {
                                logfile << "[SCB]: WRITE DATA SUCCESS" << std::endl;
                                logfile.close();
                                mem[trans->awaddr] = trans->wdata;
                            } else 
                            {
                                std::cerr << "Error: Unable to open log file." << std::endl;
                            }
                        }
                    }
                    else
                    {
                        std::ofstream logfile("sim_logs.log", std::ios_base::out | std::ios_base::app);
                        if (logfile.is_open()) 
                        {
                            logfile << "[SCB]: WRITE DATA ERROR: "<<(int)trans->bresp<<std::endl;
                            logfile.close();
                        } else 
                        {
                            std::cerr << "Error: Unable to open log file." << std::endl;
                        }
                    }
                }
                else
                {
                    if(trans->rresp == 0)
                    {
                        std::ofstream logfile("sim_logs.log", std::ios_base::out | std::ios_base::app);
                        if(trans->rdata == mem[trans->araddr])
                        {
                            if (logfile.is_open()) 
                            {
                                logfile << "[SCB]: READ DATA MATCHED"<<std::endl;
                                logfile.close();
                            } else 
                            {
                                std::cerr << "Error: Unable to open log file." << std::endl;
                            }
                        }
                    }
                    else
                    {
                        std::ofstream logfile("sim_logs.log", std::ios_base::out | std::ios_base::app);
                        if (logfile.is_open()) 
                        {
                            logfile << "[SCB]: READ DATA ERROR: "<<(int)trans->rresp<<std::endl;
                            logfile.close();
                        } else 
                        {
                            std::cerr << "Error: Unable to open log file." << std::endl;
                        }
                    }

                }

            }
        }
};


int main(int argc, char** argv, char** env) {
    dut = new Vaxi_slave;
    Verilated::traceEverOn(true);
    m_trace = new VerilatedVcdC;
    dut->trace(m_trace, 5);
    m_trace->open("waveform.vcd");

    std::queue<transaction*> mon_to_scb;

    transaction *t;
    generator *g = new generator();
    driver *d = new driver(dut);
    monitor *m = new monitor(dut, mon_to_scb);
    scoreboard *s = new scoreboard(mon_to_scb);

    dut->aclk = 0;

    std::ofstream logfile("sim_logs.log", std::ios_base::out | std::ios_base::app);

    if (logfile.is_open()) 
    {
        logfile << "###### Reset DUT #######" << std::endl;
        logfile.close();
    } else 
    {
        std::cerr << "Error: Unable to open log file." << std::endl;
    }
    d->dut_reset();

    while(num_test < NUM_TESTS)
    {

        /* Next posedge */
        while(dut->aclk != 0)
        {
            tick();
        }
        tick();

        t = g->generate_sequence();

        d->drive(t);
        m->monitor_dut(t);
        s->scb_check();

        num_test++;
    }
    
    logfile.open("sim_logs.log", std::ios_base::out | std::ios_base::app);
    if (logfile.is_open()) 
    {
        logfile << "###### SIMULATION ENDED #######" << std::endl;
        logfile.close();
    } else 
    {
        std::cerr << "Error: Unable to open log file." << std::endl;
    }

    m_trace->close();
    delete dut;
    exit(EXIT_SUCCESS);
}
