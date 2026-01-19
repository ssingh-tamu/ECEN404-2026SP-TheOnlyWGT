`timescale 1ns/1ps

module tb_nco;
    reg clk_sys;
    reg rstn;
    reg change_phase_pulse;
    reg [31:0] spi_out;
    wire step_pulse;
    wire square;

    nco_square uut (
        .clk_sys(clk_sys),
        .rstn(rstn),
        .change_phase_pulse(change_phase_pulse),
        .spi_out(spi_out),
        .step_pulse(step_pulse),
        .square(square)
    );

    // Clock generation
    initial clk_sys = 0;
    always #5 clk_sys = ~clk_sys; // 100 MHz system cock

    initial begin
        // Initialize signals
        rstn = 0; change_phase_pulse = 0; spi_out = 32'b0; 
        #20;
        rstn = 1;

        #30; change_phase_pulse = 1; spi_out = 31'h0FFFFFFF;
        #30; change_phase_pulse = 0;
        #500; 
        #30; change_phase_pulse = 1; spi_out = 31'h02FFFFFF;
        #30; change_phase_pulse = 0;
        #5000;

        #50;
        $finish;
    end

    // Dump waveforms
    initial begin
        $dumpfile("waveform.vcd");
        $dumpvars(0, tb_nco);
    end

endmodule