`timescale 1ns/1ps

module tb_spi;
    reg clk_sys;
    reg rstn;
    reg sclk;
    reg mosi;
    reg cs_n;
    
    wire        write_mem0_pulse;
    wire        write_mem1_pulse;
    wire [15:0] write_addr_latched;
    wire [15:0] write_data_latched;
    wire        set_reset0_pulse;
    wire        set_reset1_pulse;
    wire [15:0] reset_value_latched;
    wire        set_nco0_phase_pulse;
    wire        set_nco1_phase_pulse;
    wire [31:0] nco_phase_latched;

    spi_peripheral uut (
        .clk_sys(clk_sys),
        .rstn(rstn),
        .sclk(sclk),
        .mosi(mosi),
        .cs_n(cs_n),
        .write_mem0_pulse(write_mem0_pulse),
        .write_mem1_pulse(write_mem1_pulse),
        .write_addr_latched(write_addr_latched),
        .write_data_latched(write_data_latched),
        .set_reset0_pulse(set_reset0_pulse),
        .set_reset1_pulse(set_reset1_pulse),
        .reset_value_latched(reset_value_latched),
        .set_nco0_phase_pulse(set_nco0_phase_pulse),
        .set_nco1_phase_pulse(set_nco1_phase_pulse),
        .nco_phase_latched(nco_phase_latched)
    );

    // Clock generation
    initial clk_sys = 0;
    always #5 clk_sys = ~clk_sys; // 100 MHz system cock

    // Task to send one SPI bit (MOSI) with clock toggle
    task send_spi_bit(input bit_value);
    begin
        mosi = bit_value;
        sclk = 1; #10;  // SCLK high
        sclk = 0; #10;  // SCLK falling edge samples MOSI
    end
    endtask

    // Task to send a complete SPI frame (MSB first)
    task send_spi_frame(input [34:0] frame); // 35 bits
    integer i;
    begin
        cs_n = 0;      // activate CS
        for (i = 34; i >= 0; i=i-1) begin
            send_spi_bit(frame[i]);
        end
        cs_n = 1;      // deactivate CS to trigger decoding
        #20;           // wait a little after CS goes high
    end
    endtask

    initial begin
        // Initialize signals
        rstn = 0; sclk = 0; mosi = 0; cs_n = 1;
        #20;
        rstn = 1;

        // {opcode, addr, data}
        send_spi_frame({3'b010, 16'h1234, 16'hABCD});
        send_spi_frame({3'b000, 16'h5678, 16'hEFAB});
        send_spi_frame({3'b100, 16'h0123, 16'h4567});

        #10;
        if (write_mem0_pulse !== 1'b1) begin
            $display("ERROR: write_mem0_pulse did not go high!");
        end else begin
            $display("PASS: write_mem0_pulse went high as expected.");
            $display("Latched addr: %h, data: %h", write_addr_latched, write_data_latched);
        end

        #50;
        $finish;
    end

    // Dump waveforms
    initial begin
        $dumpfile("waveform.vcd");
        $dumpvars(0, tb_spi);
    end

endmodule