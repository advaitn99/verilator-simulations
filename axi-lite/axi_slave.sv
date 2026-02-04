

module axi_slave(
  
  input wire aclk,
  input wire areset,
  
  input wire [31:0] s_awaddr, // Write address
  input wire s_awvalid, // Write address valid
  output reg s_awready, // Slave ready for write address
  
  input wire [31:0] s_wdata, // Write data
  input wire s_wvalid, // Write data valid
  output reg s_wready, // Slave ready for write data
  
  output reg [1:0] s_bresp, // Write response from slave
  output reg s_bvalid, // Write response valid
  input wire s_bready, // Master ready for write respose
  
  input wire [31:0] s_araddr, // Read address
  input wire s_arvalid, // Read address valid
  output reg s_arready, // Slave ready for read address

  output reg [31:0] s_rdata, // Read data
  output reg s_rvalid, // Read data is valid
  input wire s_rready, // Master ready for read data
  output reg [1:0] s_rresp // Read response from slave
);
  
  localparam idle = 0,
  get_write_addr = 1,
  get_read_addr = 2,
  get_write_data = 3,
  get_read_data = 4,
  send_write_resp = 5,
  fetch_data = 6,
  write_mem = 7;
  
  reg [2:0] state;
  reg [31:0] waddr, raddr, wdata, rdata;
  reg [1:0] slv_status = 0;

  reg [31:0] mem[128];
  
  always @(posedge aclk or negedge areset) begin
    if(areset == 1'b0) begin
      state <= idle;
      s_awready <= 1'b0;
      s_wready <= 1'b0;
      s_bresp <= 2'b00;

      for(int i=0; i<128; i++) begin
        mem[i] <= 32'h00000000;
      end
    end
    else begin
      case(state)

      idle: begin
        if(s_awvalid == 1'b1) begin
            state <= get_write_addr;
            s_awready <= 1'b1;
        end
        else if(s_arvalid == 1'b1) begin
            state <= get_read_addr;
            s_arready <= 1'b1;
        end
        else begin
            state <= idle;
            s_awready <= 1'b0;
            s_wready <= 1'b0;
            s_bresp <= 2'b00;  
        end
      end

      get_write_addr: begin
        if(s_awvalid == 1'b1) begin
            waddr <= s_awaddr;
            s_awready <= 1'b0;
            s_wready <= 1'b1;
            state <= get_write_data;
        end
        else begin
            state <= idle;
        end
      end

      get_write_data: begin
        if(s_wvalid == 1'b1) begin
            wdata <= s_wdata;
            s_wready <= 1'b0;
            state <= write_mem;
        end
        else begin
            state <= get_write_data;
        end
      end

      write_mem: begin
        if(waddr <= 32'h7F) begin
            slv_status = 2'b00;
            mem[waddr] <= wdata;
            state <= send_write_resp;
        end
        else begin
            slv_status = 2'b11;
            state <= send_write_resp;
        end
      end

      get_read_addr: begin
        if(s_wvalid == 1'b1) begin
            raddr <= s_araddr;
            s_arready <= 1'b0;
            state <= fetch_data;
        end
        else begin
            state <= idle;
        end
      end

      fetch_data: begin
        if(raddr <= 32'h7F) begin
            rdata <= mem[raddr];
            state <= get_read_data;
            slv_status <= 2'b00;
        end
        else begin
            slv_status <= 2'b11;
            state <= get_read_data;
        end
      end

      get_read_data: begin
        if(s_rready == 1'b1) begin
            s_rdata <= rdata;
            s_rvalid <= 1'b1;
            s_rresp <= slv_status;
            state <= idle;
        end
        else begin
            state <= get_read_data;
        end
      end

      send_write_resp: begin

        
        if(s_bready == 1'b1) begin
            s_bresp <= slv_status;
            s_bvalid <= 1'b1;
            state <= idle;
        end
        else begin
            state <= send_write_resp;
        end
      end
      endcase
    end
  end
  
endmodule