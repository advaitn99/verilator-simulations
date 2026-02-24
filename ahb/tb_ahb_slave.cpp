#include <stdlib.h>
#include <iostream>
#include <verilated.h>
#include <verilated_vcd_c.h>
#include "Vahb_slave.h"
#include "Vahb_slave_ahb_slave.h"
#include <random>
#include <chrono>
#include <thread>
#include <fstream>
#include <queue>


#define MAX_SIM_TIME 100
#define NUM_TESTS    20
#define DATA_MIN    1
#define DATA_MAX    100

vluint64_t sim_time = 0;
int num_test = 0;
VerilatedVcdC *m_trace;
Vahb_slave *dut;


void tick(void)
{
    dut->hclk ^= 1;
    dut->eval();
    m_trace->dump(sim_time);
    sim_time++;
}


class transaction
{
    public:
        uint8_t op;      /* Operation select */
        uint32_t addr;   /* Write address */
        uint32_t wdata;  /* Write data */
        uint32_t rdata;  /* Read data */
        uint8_t hresp;   /* Response */
        uint8_t hsize;   /* Transfer size */
        uint8_t hburst;  /* Burst size */
        uint8_t burst_count;
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
            tx->addr   = rng.generate_int(20, 50) * 2;
            tx->wdata  = rng.generate_int(DATA_MIN,DATA_MAX);
            tx->hsize  = 1;
            tx->hburst = 4;
            tx->burst_count = 5;

            std::ofstream logfile("sim_logs.log", std::ios_base::out | std::ios_base::app);

            if (logfile.is_open()) 
            {
                logfile << "[GEN]: OP: " << (int)tx->op
                        << " ADDR: " << (int)tx->addr
                        << " WDATA: " << (int)tx->wdata
                        << " HSIZE: " << (int)tx->hsize
                        << " HBURST: " << (int)tx->hburst
                        << " NUM BURSTS: " << (int)tx->burst_count
                <<std::endl;
                logfile.close();
            } else 
            {
                std::cerr << "Error: Unable to open log file." << std::endl;
            }

            return tx;
        }
};


class monitor
{
    private:
        Vahb_slave *dut;
        std::queue<transaction*> &mon_to_scb;
        

    public:

    monitor(Vahb_slave *dut, std::queue<transaction*> mon_to_scb): dut(dut), mon_to_scb(mon_to_scb)
    {

    }

    void monitor_dut(transaction *trans)
    {
        transaction* t = new transaction();
        t->op = trans->op;
        t->addr = trans->addr;
        t->hresp = dut->s_ahb_hresp;
        t->wdata = trans->wdata;
        t->rdata = dut->s_ahb_rdata;

        std::ofstream logfile("sim_logs.log", std::ios_base::out | std::ios_base::app);
        if (logfile.is_open())
        {
            logfile << "[MON]: OP: " << (int)(t->op)
                    << " ADDR: " << (int)t->addr
                    << " WDATA: " << (int)t->wdata
                    << " RDATA: " << (int)t->rdata
                    << " HRESP: " << (int)t->hresp
            <<std::endl;
            logfile.close();
        } else
        {
            std::cerr << "Error: Unable to open log file." << std::endl;
        }

        mon_to_scb.push(t);
    }
};



class driver
{
    private:
        Vahb_slave *dut;
        RandomNumberGenerator rng_drv;
        monitor *m;

    public:

        driver(Vahb_slave *dut, monitor *m): dut(dut), m(m)
        {
        }

        void dut_reset()
        {
            int half_clk_counts = 0;

            /* Assert reset */
            dut->hresetn = 0;
            
            /* Keep reset for 5 clk cycles */
            while(half_clk_counts < 10)
            {
                tick();
                half_clk_counts++;
            }

            /* Deassert reset */
            dut->hresetn = 1;
        }

        void dut_single_write(transaction *trans)
        {
            if(trans != NULL)
            {
                dut->s_ahb_haddr  = trans->addr;
                dut->s_ahb_hwrite = 1;
                dut->s_ahb_hsel   = 1;
                dut->s_ahb_hsize  = trans->hsize;
                dut->s_ahb_hburst = 0;
                dut->s_ahb_htrans = 2;
                dut->s_ahb_hwdata = trans->wdata;

                /* Wait for HREADY signal */
                while(dut->s_ahb_hready != 1)
                {
                    tick();
                }

                std::ofstream logfile("sim_logs.log", std::ios_base::out | std::ios_base::app);
                if (logfile.is_open())
                {
                    logfile << "[DRV]: OP: " << (int)(trans->op)
                            << " ADDR: " << (int)trans->addr
                            <<" HTRANS: " << 2
                            <<" WDATA: " << trans->wdata
                    <<std::endl;
                    logfile.close();
                } else
                {
                    std::cerr << "Error: Unable to open log file." << std::endl;
                }

                m->monitor_dut(trans);

                tick();
                tick();
                
            }
        }

        void dut_single_read(transaction* trans)
        {
            int rdata;

            dut->s_ahb_haddr  = trans->addr;
            dut->s_ahb_hwrite = 0;
            dut->s_ahb_hsel   = 1;
            dut->s_ahb_hsize  = trans->hsize;
            dut->s_ahb_hburst = 0;
            dut->s_ahb_htrans = 2;

            /* Wait for HREADY signal */
            while(dut->s_ahb_hready != 1)
            {
                tick();
            }

            rdata = dut->s_ahb_rdata;

            std::ofstream logfile("sim_logs.log", std::ios_base::out | std::ios_base::app);
            if (logfile.is_open())
            {
                logfile << "[DRV]: OP: " << (int)(trans->op)
                        << " ADDR: " << (int)trans->addr
                        <<" HTRANS: " << 2
                <<std::endl;
                logfile.close();
            } else
            {
                std::cerr << "Error: Unable to open log file." << std::endl;
            }

            m->monitor_dut(trans);

            tick();
            tick();
        }

        void dut_incr_write(transaction* trans)
        {
            int first = 1;
            int htrans, wdata;
            int num_bursts;
            int addr;

            switch(trans->hburst)
            {
                case 1: num_bursts = trans->burst_count; break;
                case 3: num_bursts = 4; break;
                case 5: num_bursts = 8; break;
                case 7: num_bursts = 16; break;
                default: num_bursts = 4;
            }

            for(int i = 0; i < num_bursts; i++)
            {
                transaction *t = new transaction();
                if(first)
                {
                    dut->s_ahb_htrans = 2; /* NONSEQ */
                    htrans = 2;
                    first = 0;
                }
                else
                {
                    dut->s_ahb_htrans = 3; /* SEQ */
                    htrans = 3;
                }
                
                dut->s_ahb_haddr  = trans->addr;
                dut->s_ahb_hwrite = 1;
                dut->s_ahb_hsel   = 1;
                dut->s_ahb_hsize  = trans->hsize;
                dut->s_ahb_hburst = trans->hburst;
                wdata = rng_drv.generate_int(1,100);
                dut->s_ahb_hwdata = wdata;

                /* Apply inputs at POSEDGE */
                tick();

                /* Wait for HREADY signal */
                while(dut->s_ahb_hready != 1)
                {
                    tick();
                }

                addr = dut->ahb_slave->next_addr;
                trans->addr = addr; 
                trans->wdata = wdata;               

                std::ofstream logfile("sim_logs.log", std::ios_base::out | std::ios_base::app);
                if (logfile.is_open())
                {
                    logfile << "[DRV]: OP: " << (int)(trans->op)
                            << " ADDR: " << (int)addr
                            <<" HTRANS: " << htrans
                            <<" WDATA: " << wdata
                    <<std::endl;
                    logfile.close();
                } else
                {
                    std::cerr << "Error: Unable to open log file." << std::endl;
                }

                t->addr = trans->addr;
                t->op = trans->op;
                t->wdata = trans->wdata;

                m->monitor_dut(t);

                //delete t;

                /* Keep inputs stable for NEGEDGE */
                tick();
            }
        }

        void dut_incr_read(transaction* trans)
        {
            int first = 1;
            int htrans, wdata;
            int num_bursts;
            int addr, rdata;

            switch(trans->hburst)
            {
                case 1: num_bursts = trans->burst_count; break;
                case 3: num_bursts = 4; break;
                case 5: num_bursts = 8; break;
                case 7: num_bursts = 16; break;
                default: num_bursts = 4;
            }

            for(int i = 0; i < num_bursts; i++)
            {
                transaction *t = new transaction();
                if(first)
                {
                    dut->s_ahb_htrans = 2; /* NONSEQ */
                    htrans = 2;
                    first = 0;
                }
                else
                {
                    dut->s_ahb_htrans = 3; /* SEQ */
                    htrans = 3;
                }
                
                dut->s_ahb_haddr  = trans->addr;
                dut->s_ahb_hwrite = 0;
                dut->s_ahb_hsel   = 1;
                dut->s_ahb_hsize  = trans->hsize;
                dut->s_ahb_hburst = trans->hburst;

                /* Apply inputs at POSEDGE */
                tick();

                /* Wait for HREADY signal */
                while(dut->s_ahb_hready != 1)
                {
                    tick();
                }

                addr = dut->ahb_slave->next_addr;
                rdata = dut->s_ahb_rdata;
                trans->addr = addr;

                std::ofstream logfile("sim_logs.log", std::ios_base::out | std::ios_base::app);
                if (logfile.is_open()) 
                {
                    logfile << "[DRV]: OP: " << (int)(trans->op)
                            << " ADDR: " << (int)addr
                            <<" HTRANS: " << htrans
                    <<std::endl;
                    logfile.close();
                } else 
                {
                    std::cerr << "Error: Unable to open log file." << std::endl;
                }

                t->addr = trans->addr;
                t->op   = trans->op;

                m->monitor_dut(t);

                // delete t;
                
                /* Keep inputs stable for NEGEDGE */
                tick();
            }
        }

        void dut_wrap_write(transaction* trans)
        {
            int first = 1;
            int htrans, wdata;
            int num_bursts;
            int addr;

            switch(trans->hburst)
            {
                case 2: num_bursts = 4; break;
                case 4: num_bursts = 8; break;
                case 6: num_bursts = 16; break;
                default: num_bursts = 4;
            }

            for(int i = 0; i < num_bursts; i++)
            {
                transaction *t = new transaction();
                if(first)
                {
                    dut->s_ahb_htrans = 2; /* NONSEQ */
                    htrans = 2;
                    first = 0;
                }
                else
                {
                    dut->s_ahb_htrans = 3; /* SEQ */
                    htrans = 3;
                }
                
                dut->s_ahb_haddr  = trans->addr;
                dut->s_ahb_hwrite = 1;
                dut->s_ahb_hsel   = 1;
                dut->s_ahb_hsize  = trans->hsize;
                dut->s_ahb_hburst = trans->hburst;
                wdata = rng_drv.generate_int(1,100);
                dut->s_ahb_hwdata = wdata;

                /* Apply inputs at POSEDGE */
                tick();

                /* Wait for HREADY signal */
                while(dut->s_ahb_hready != 1)
                {
                    tick();
                }

                addr = dut->ahb_slave->next_addr;
                trans->addr = addr; 
                trans->wdata = wdata;               

                std::ofstream logfile("sim_logs.log", std::ios_base::out | std::ios_base::app);
                if (logfile.is_open())
                {
                    logfile << "[DRV]: OP: " << (int)(trans->op)
                            << " ADDR: " << (int)addr
                            <<" HTRANS: " << htrans
                            <<" WDATA: " << wdata
                    <<std::endl;
                    logfile.close();
                } else
                {
                    std::cerr << "Error: Unable to open log file." << std::endl;
                }

                t->addr = trans->addr;
                t->op   = trans->op;
                t->wdata = trans->wdata;

                m->monitor_dut(t);             

                /* Keep inputs stable for NEGEDGE */
                tick();
            }
        }

        void dut_wrap_read(transaction* trans)
        {
            int first = 1;
            int htrans;
            int num_bursts;
            int addr, rdata;

            switch(trans->hburst)
            {
                case 2: num_bursts = 4; break;
                case 4: num_bursts = 8; break;
                case 6: num_bursts = 16; break;
                default: num_bursts = 4;
            }

            for(int i = 0; i < num_bursts; i++)
            {
                transaction *t = new transaction();
                if(first)
                {
                    dut->s_ahb_htrans = 2; /* NONSEQ */
                    htrans = 2;
                    first = 0;
                }
                else
                {
                    dut->s_ahb_htrans = 3; /* SEQ */
                    htrans = 3;
                }

                dut->s_ahb_haddr  = trans->addr;
                dut->s_ahb_hwrite = 0;
                dut->s_ahb_hsel   = 1;
                dut->s_ahb_hsize  = trans->hsize;
                dut->s_ahb_hburst = trans->hburst;

                /* Apply inputs at POSEDGE */
                tick();

                /* Wait for HREADY signal */
                while(dut->s_ahb_hready != 1)
                {
                    tick();
                }

                addr = dut->ahb_slave->next_addr;
                rdata = dut->s_ahb_rdata;
                trans->addr = addr;

                std::ofstream logfile("sim_logs.log", std::ios_base::out | std::ios_base::app);
                if (logfile.is_open()) 
                {
                    logfile << "[DRV]: OP: " << (int)(trans->op)
                            << " ADDR: " << (int)addr
                            <<" HTRANS: " << htrans
                    <<std::endl;
                    logfile.close();
                } else 
                {
                    std::cerr << "Error: Unable to open log file." << std::endl;
                }
                
                t->addr = trans->addr;
                t->op   = trans->op;

                m->monitor_dut(t);

                /* Keep inputs stable for NEGEDGE */
                tick();
            }
        }

        void drive(transaction *trans)
        {
            if(trans->op == 1)
            {
                if(trans->hburst == 0)
                {
                    dut_single_write(trans);
                }

                else if(trans->hburst % 2 != 0)
                {
                    dut_incr_write(trans);
                }

                else if(trans->hburst % 2 == 0)
                {
                    dut_wrap_write(trans);
                }
            }
            else
            {
                if(trans->hburst == 0)
                {
                    dut_single_read(trans);
                }

                else if(trans->hburst % 2 != 0)
                {
                    dut_incr_read(trans);
                }

                else if(trans->hburst % 2 == 0)
                {
                    dut_wrap_read(trans);
                }
            }
        }
};


class scoreboard
{
    private:
        uint8_t mem[256] = {0};
        std::queue<transaction*> &mon_to_scb;
        

    public:
        scoreboard(std::queue<transaction*> mon_to_scb): mon_to_scb(mon_to_scb)
        {
        }

        void sco_check()
        {
            int pass = 0, fail = 0;
            int burst_count = 0;
            int op = 0;
            while(!mon_to_scb.empty())
            {
                transaction *t = new transaction();
                t = mon_to_scb.front();
                mon_to_scb.pop();
                burst_count++;

                if((int)t->op == 1)
                {
                    mem[(int)t->addr] = (uint8_t)t->wdata;
                    pass++;
                    op = 1;
                }
                else
                {
                    op = 0;
                    if(mem[(int)t->addr] == (uint8_t)t->rdata)
                    {
                        pass++;
                    }
                    else
                    {
                        std::ofstream logfile("sim_logs.log", std::ios_base::out | std::ios_base::app);
                        if (logfile.is_open())
                        {
                            logfile << "[SCO]: READ DATA MISMATCH "
                                    << " ADDR: " << (int)t->addr
                                    << " RDATA received: " << (int)t->rdata
                                    << " RDATA expected: " << (int)mem[t->addr]
                                    << " HRESP: " << (int)t->hresp
                            <<std::endl;
                            logfile.close();
                        } else
                        {
                            std::cerr << "Error: Unable to open log file." << std::endl;
                        }
                        fail++;
                    }
                }
            }

            std::ofstream logfile("sim_logs.log", std::ios_base::out | std::ios_base::app);
            if (logfile.is_open())
            {
                logfile << "[SCO]: Scoreboard report "
                        << " Operation: " << op
                        << " TOTAL BURSTS RECEIVED: " << (int)burst_count
                        << " PASS: " << (int)pass
                        << " FAIL: " << (int)fail
                <<std::endl;
                logfile.close();
            } else
            {
                std::cerr << "Error: Unable to open log file." << std::endl;
            }
        }
};


int main(int argc, char** argv, char** env) {
    dut = new Vahb_slave;
    Verilated::traceEverOn(true);
    m_trace = new VerilatedVcdC;
    dut->trace(m_trace, 5);
    m_trace->open("waveform.vcd");

    std::queue<transaction*> mon_to_scb;
    //std::queue<transaction*> drv_to_mon;

    transaction *t;
    generator *g = new generator();
    monitor *m = new monitor(dut, mon_to_scb);
    driver *d = new driver(dut, m);
    scoreboard *s = new scoreboard(mon_to_scb);

    dut->hclk = 0;

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
        while(dut->hclk != 0)
        {
            tick();
        }
        //tick();

        t = g->generate_sequence();

        d->drive(t);

        s->sco_check();

        tick();
        tick();

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
    delete t;
    delete g;
    delete d;
    delete m;
    delete s;
    exit(EXIT_SUCCESS);
}
