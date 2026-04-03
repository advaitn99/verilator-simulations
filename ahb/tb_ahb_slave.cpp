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
#include <string>


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


class Logger {
private:
    std::string filename;
    std::ofstream stream;

public:
    Logger(const std::string &fname) : filename(fname) {
        stream.open(filename, std::ios_base::out | std::ios_base::app);
        if (!stream.is_open()) {
            std::cerr << "Error: Unable to open log file: " << filename << std::endl;
        }
    }

    ~Logger() {
        if (stream.is_open()) stream.close();
    }

    void log(const std::string &tag, const std::string &msg) {
        if (stream.is_open()) {
            stream << "[" << tag << "]: " << msg << std::endl;
        }
    }
};


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
        Logger *logger;

    public:
        generator(Logger *logger) : logger(logger) {}

        transaction* generate_sequence()
        {
            tx->op     = rng.generate_int(0,1);
            tx->addr   = rng.generate_int(20, 50) * 2;
            tx->wdata  = rng.generate_int(DATA_MIN,DATA_MAX);
            tx->hsize  = 1;
            tx->hburst = rng.generate_int(0,7);
            tx->burst_count =rng.generate_int(2,10);

            logger->log("GEN", "OP: " + std::to_string(tx->op)
                + " ADDR: " + std::to_string(tx->addr)
                + " WDATA: " + std::to_string(tx->wdata)
                + " HSIZE: " + std::to_string(tx->hsize)
                + " HBURST: " + std::to_string(tx->hburst)
                + " NUM BURSTS: " + std::to_string(tx->burst_count));

            return tx;
        }
};


class monitor
{
    private:
        Vahb_slave *dut;
        std::queue<transaction*> &mon_to_scb;
        Logger *logger;

    public:

    monitor(Vahb_slave *dut, std::queue<transaction*> &mon_to_scb, Logger *logger): dut(dut), mon_to_scb(mon_to_scb), logger(logger)
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

        logger->log("MON", "OP: " + std::to_string(t->op)
            + " ADDR: " + std::to_string(t->addr)
            + " WDATA: " + std::to_string(t->wdata)
            + " RDATA: " + std::to_string(t->rdata)
            + " HRESP: " + std::to_string(t->hresp));

        mon_to_scb.push(t);
    }
};



class driver
{
    private:
        Vahb_slave *dut;
        RandomNumberGenerator rng_drv;
        monitor *m;
        Logger *logger;

    public:

        driver(Vahb_slave *dut, monitor *m, Logger *logger): dut(dut), m(m), logger(logger)
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

                logger->log("DRV", "OP: " + std::to_string(trans->op)
                    + " ADDR: " + std::to_string(trans->addr)
                    + " HTRANS: 2"
                    + " WDATA: " + std::to_string(trans->wdata));

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

            logger->log("DRV", "OP: " + std::to_string(trans->op)
                + " ADDR: " + std::to_string(trans->addr)
                + " HTRANS: 2");

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

                logger->log("DRV", "OP: " + std::to_string(trans->op)
                    + " ADDR: " + std::to_string(addr)
                    + " HTRANS: " + std::to_string(htrans)
                    + " WDATA: " + std::to_string(wdata));

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

                logger->log("DRV", "OP: " + std::to_string(trans->op)
                    + " ADDR: " + std::to_string(addr)
                    + " HTRANS: " + std::to_string(htrans));

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

                logger->log("DRV", "OP: " + std::to_string(trans->op)
                    + " ADDR: " + std::to_string(addr)
                    + " HTRANS: " + std::to_string(htrans)
                    + " WDATA: " + std::to_string(wdata));

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

                logger->log("DRV", "OP: " + std::to_string(trans->op)
                    + " ADDR: " + std::to_string(addr)
                    + " HTRANS: " + std::to_string(htrans));
                
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
        Logger *logger;

    public:
        scoreboard(std::queue<transaction*> &mon_to_scb, Logger *logger): mon_to_scb(mon_to_scb), logger(logger)
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
                        logger->log("SCO", "READ DATA MISMATCH"
                            " ADDR: " + std::to_string(t->addr)
                            + " RDATA received: " + std::to_string(t->rdata)
                            + " RDATA expected: " + std::to_string(mem[t->addr])
                            + " HRESP: " + std::to_string(t->hresp));
                        fail++;
                    }
                }
            }

            logger->log("SCO", "Scoreboard report"
                " Operation: " + std::to_string(op)
                + " TOTAL BURSTS RECEIVED: " + std::to_string(burst_count)
                + " PASS: " + std::to_string(pass)
                + " FAIL: " + std::to_string(fail));
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

    Logger logger("sim_logs.log");

    transaction *t;
    generator *g = new generator(&logger);
    monitor *m = new monitor(dut, mon_to_scb, &logger);
    driver *d = new driver(dut, m, &logger);
    scoreboard *s = new scoreboard(mon_to_scb, &logger);

    dut->hclk = 0;

    logger.log("SIM", "###### Reset DUT #######");
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
    
    logger.log("SIM", "###### SIMULATION ENDED #######");

    m_trace->close();
    delete dut;
    delete t;
    delete g;
    delete d;
    delete m;
    delete s;
    exit(EXIT_SUCCESS);
}
