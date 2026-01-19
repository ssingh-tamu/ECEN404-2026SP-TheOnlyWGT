`timescale 1ns/1ps

module tb_memory;
    reg clk_sys;
    reg rstn;
    reg write_pulse;
    reg [31:0] spi_out;
    reg [15:0] counter_addr;
    wire [15:0] sram_data;
    wire [15:0] sram_addr;
    wire sram_we_n;

    sram_controller uut (
        .clk_sys(clk_sys),
        .rstn(rstn),
        .write_pulse(write_pulse),
        .spi_out(spi_out),
        .counter_addr(counter_addr),
        .sram_data(sram_data),
        .sram_addr(sram_addr),
        .sram_we_n(sram_we_n)
    );

    // Clock generation
    initial clk_sys = 0;
    always #5 clk_sys = ~clk_sys; // 100 MHz system cock

    initial begin
        // Initialize signals
        rstn = 0; write_pulse = 0; spi_out = 31'b0; counter_addr = 31'b0; 
        #20;
        rstn = 1;

        #10; counter_addr = 31'd1;
        #10; counter_addr = 31'd2;
        #10; counter_addr = 31'd3;
        #10; counter_addr = 31'd4;
        #10; write_pulse = 1; spi_out = 31'h1234ABCD;
        #10; counter_addr = 31'd5;
        #10; counter_addr = 31'd6;
        #10; write_pulse = 0; 
        #10; counter_addr = 31'd7;
        #10; counter_addr = 31'd8;

        #50;
        $finish;
    end

    // Dump waveforms
    initial begin
        $dumpfile("waveform.vcd");
        $dumpvars(0, tb_memory);
    end

endmodule