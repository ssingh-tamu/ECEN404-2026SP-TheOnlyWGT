`timescale 1ns/1ps

module tb_counter;
    reg clk_sys;
    reg rstn;
    reg step_en;
    reg set_reset_pulse;
    reg [31:0] spi_out;
    reg mem_write_pulse;
    wire [15:0] value;

    binary_counter uut (
        .clk_sys(clk_sys),
        .rstn(rstn),
        .step_en(step_en),
        .set_reset_pulse(set_reset_pulse),
        .spi_out(spi_out),
        .mem_write_pulse(mem_write_pulse),
        .value(value)
    );

    // Clock generation
    initial clk_sys = 0;
    always #5 clk_sys = ~clk_sys; // 100 MHz system cock

    initial begin
        // Initialize signals
        rstn = 0; step_en = 0; set_reset_pulse = 0; mem_write_pulse = 0; spi_out = 31'b0;
        #20;
        rstn = 1;

        #10; step_en = 1;
        #30; step_en = 0;
        #10; spi_out = 31'h0003AAAA; set_reset_pulse = 1;
        #50; spi_out = 31'h1234567; set_reset_pulse = 0;
        #10; step_en = 1;
        #60; step_en = 0;
        #20; mem_write_pulse = 1; spi_out = 31'hABCDEF0;
        #50; mem_write_pulse = 0; step_en = 1;
        #20; spi_out = 31'h0005AAAA; set_reset_pulse = 1;
        #30; set_reset_pulse = 0; 
        #80;

        #50;
        $finish;
    end

    // Dump waveforms
    initial begin
        $dumpfile("waveform.vcd");
        $dumpvars(0, tb_counter);
    end

endmodule