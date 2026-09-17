
//`#start header` -- edit after this line, do not edit this line
// ========================================
//
// Copyright YOUR COMPANY, THE YEAR
// All Rights Reserved
// UNPUBLISHED, LICENSED SOFTWARE.
//
// CONFIDENTIAL AND PROPRIETARY INFORMATION
// WHICH IS THE PROPERTY OF your company.
//
// ========================================
`include "cypress.v"
//`#end` -- edit above this line, do not edit this line
// Generated on 07/02/2026 at 14:23
// Component: superMaquina
module superMaquina (
	output  drq_adc_ram,
	output  drq_adc_to_filter,
	output  drq_filter_ram,
	output [7:0] error,
	output  irq,
	output [7:0] state,
	output [7:0] status,
	output [7:0] timer_event,
	input   button_pos,
	input  [7:0] cfg,
	input   clk,
	input  [7:0] ctrl,
	input   dma_req_filter,
	input   eoc_adc,
	input   nrq_dma_adc,
	input   nrq_dma_filter,
	input   reset,
	input   sync_det,
	input   tc_led,
	input   tc_ping,
	input   tc_uart,
	input   tc_watcdog
);

//`#start body` -- edit after this line, do not edit this line

parameter ST_IDLE     = 3'd0;
parameter ST_ARMED    = 3'd1;
parameter ST_SAMPLING = 3'd2;
parameter ST_DONE     = 3'd3;
parameter ST_ERROR    = 3'd4;

parameter SRC_RAW      = 2'b00;
parameter SRC_FILTER   = 2'b01;
parameter SRC_COMBINED = 2'b10;
parameter SRC_DEBUG    = 2'b11;

wire ctrl_arm          = ctrl[0];
wire ctrl_start_now    = ctrl[1];
wire ctrl_stop         = ctrl[2];
wire ctrl_clear_flags  = ctrl[3];
wire ctrl_use_sync     = ctrl[4];
wire ctrl_load_batch_lo = ctrl[5];
wire ctrl_engine_en    = ctrl[6];
wire ctrl_load_batch_hi = ctrl[7];

wire [1:0] sample_source = cfg[1:0];
wire cfg_irq_batch_en    = cfg[2];
wire cfg_irq_sync_en     = cfg[3];
wire cfg_irq_error_en    = cfg[4];
/* cfg[5..7] libres. El descarte del retardo de grupo FIR lo hace el ARM:
 * mantenerlo fuera de PLD evita que el camino filtrado por sync quede retenido
 * si el contador UDB de descarte no recibe TC. */

wire source_raw      = (sample_source == SRC_RAW) || (sample_source == SRC_DEBUG);
wire source_filter   = (sample_source == SRC_FILTER);
wire source_combined = (sample_source == SRC_COMBINED);

wire capture_raw_dma      = source_raw || source_combined;
wire capture_filter_path  = source_filter || source_combined;

wire bypass_raw_dma       = (!ctrl_engine_en) && (source_raw || source_combined);
wire bypass_filter_path   = (!ctrl_engine_en) && (source_filter || source_combined);
reg [2:0] state_reg;
reg error_stop_reg;
reg error_state_reg;
reg ctrl_arm_d;
reg ctrl_start_now_d;
reg ctrl_stop_d;
reg ctrl_clear_flags_d;
reg ctrl_load_batch_lo_d;
reg ctrl_load_batch_hi_d;
reg button_pos_d;
reg nrq_dma_adc_d;
reg nrq_dma_filter_d;
reg irq_reg;
reg sync_seen_reg;
reg button_seen_reg;
reg batch_seen_reg;
reg [4:0] sample_count_reg;
reg [8:0] batch_count_reg;
reg [8:0] batch_limit_reg;

wire arm_pulse         = ctrl_arm & ~ctrl_arm_d;
wire start_now_pulse   = ctrl_start_now & ~ctrl_start_now_d;
wire stop_pulse        = ctrl_stop & ~ctrl_stop_d;
wire clear_flags_pulse = ctrl_clear_flags & ~ctrl_clear_flags_d;
wire load_batch_lo_pulse = ctrl_load_batch_lo & ~ctrl_load_batch_lo_d;
wire load_batch_hi_pulse = ctrl_load_batch_hi & ~ctrl_load_batch_hi_d;
wire button_pos_rise = button_pos & ~button_pos_d;

wire sampling_active = ctrl_engine_en && (state_reg == ST_SAMPLING);
wire nrq_dma_adc_rise = nrq_dma_adc & ~nrq_dma_adc_d;
wire nrq_dma_filter_rise = nrq_dma_filter & ~nrq_dma_filter_d;
wire sample_done_rise = (source_filter || source_combined) ? nrq_dma_filter_rise : nrq_dma_adc_rise;
wire sample_irq_event = sample_done_rise &&
                        (sampling_active || bypass_raw_dma || bypass_filter_path);
/* Politica determinismo-primero: fuera de IDLE, todo lo secundario espera o
 * se descarta; solo muestras y sync interrumpen al ARM durante una captura.
 * - Muestras DMA: unica fuente de irq en ST_SAMPLING (y bypass).
 * - sync_det: solo se mira en ST_ARMED (es quien inicia el muestreo).
 * - button_pos: DESCARTE fuera de IDLE (solo irq/flag en ST_IDLE).
 * - TC de timers: HOLD. Nunca generan irq; cada TC queda retenido en el
 *   STATUS_TC de su propio timer fixed y el firmware lo consume por polling
 *   (el watchdog de captura tambien por polling, atendido incluso durante
 *   SAMPLING). timer_event expone la vista live solo como diagnostico. */
wire button_irq_event = button_pos_rise && (state_reg == ST_IDLE);
wire tc_capture_watchdog = tc_watcdog; /* typo externo mantenido por TopDesign */
wire [3:0] timer_event_live = {tc_capture_watchdog, tc_led, tc_ping, tc_uart};
wire sample_will_close_batch = (sample_count_reg == 5'd0);
wire batch_will_finish_capture = (batch_count_reg == batch_limit_reg);

/* En ST_ARMED el FIR sigue alimentado (historia caliente): asi las primeras
 * salidas filtradas tras el sync son validas y el ARM solo descarta el
 * retardo de grupo (63) para alinear con la ruta raw. Sin IRQ ni conteo en
 * ARMED: eso solo ocurre en ST_SAMPLING. */
wire filter_feed_active = ctrl_engine_en && capture_filter_path &&
                          ((state_reg == ST_SAMPLING) || (state_reg == ST_ARMED));

assign drq_adc_ram       = ctrl_engine_en ? (sampling_active && capture_raw_dma && eoc_adc) :
                                            (bypass_raw_dma && eoc_adc);
assign drq_adc_to_filter = ctrl_engine_en ? (filter_feed_active && eoc_adc) :
                                            (bypass_filter_path && eoc_adc);
assign drq_filter_ram    = ctrl_engine_en ? (filter_feed_active && dma_req_filter) :
                                            (bypass_filter_path && dma_req_filter);

assign irq               = irq_reg;
assign error             = {3'b000, error_state_reg, error_stop_reg, 3'b000};
assign timer_event       = {4'b0000, timer_event_live};
/* Nota: exponer batch_count_reg en state[7:3] NO entra en los 24 UDB de este
 * diseno (el fitter falla con E2071); si se necesita progreso de lotes en el
 * ARM hay que liberar PLD antes. */
assign state             = {5'b00000, state_reg};
assign status            = {
    button_seen_reg,
    batch_seen_reg,
    sync_seen_reg,
    (state_reg == ST_ERROR),
    (state_reg == ST_DONE),
    (state_reg == ST_SAMPLING),
    (state_reg == ST_ARMED),
    (state_reg == ST_IDLE)
};

always @(posedge clk or posedge reset)
begin
    if (reset)
    begin
        state_reg             <= ST_IDLE;
        error_stop_reg        <= 1'b0;
        error_state_reg       <= 1'b0;
        ctrl_arm_d            <= 1'b0;
        ctrl_start_now_d      <= 1'b0;
        ctrl_stop_d           <= 1'b0;
        ctrl_clear_flags_d    <= 1'b0;
        ctrl_load_batch_lo_d  <= 1'b0;
        ctrl_load_batch_hi_d  <= 1'b0;
        button_pos_d          <= 1'b0;
        nrq_dma_adc_d         <= 1'b0;
        nrq_dma_filter_d      <= 1'b0;
        irq_reg               <= 1'b0;
        sync_seen_reg         <= 1'b0;
        button_seen_reg       <= 1'b0;
        batch_seen_reg        <= 1'b0;
        sample_count_reg      <= 5'd29;
        batch_count_reg       <= 9'd0;
        batch_limit_reg       <= 9'd0;
    end
    else
    begin
        ctrl_arm_d           <= ctrl_arm;
        ctrl_start_now_d     <= ctrl_start_now;
        ctrl_stop_d          <= ctrl_stop;
        ctrl_clear_flags_d   <= ctrl_clear_flags;
        ctrl_load_batch_lo_d <= ctrl_load_batch_lo;
        ctrl_load_batch_hi_d <= ctrl_load_batch_hi;
        button_pos_d         <= button_pos;
        nrq_dma_adc_d        <= nrq_dma_adc;
        nrq_dma_filter_d     <= nrq_dma_filter;

        irq_reg <= sample_irq_event | button_irq_event;

        if (button_irq_event)
        begin
            button_seen_reg <= 1'b1;
        end

        if (load_batch_lo_pulse)
        begin
            batch_limit_reg[7:0] <= cfg;
        end

        if (load_batch_hi_pulse)
        begin
            batch_limit_reg[8] <= cfg[0];
        end

        if (!ctrl_engine_en)
        begin
            state_reg        <= ST_IDLE;
            sample_count_reg <= 5'd29;
            batch_count_reg  <= 9'd0;
            if (clear_flags_pulse)
            begin
                error_stop_reg  <= 1'b0;
                error_state_reg <= 1'b0;
                sync_seen_reg   <= 1'b0;
                button_seen_reg <= 1'b0;
                batch_seen_reg  <= 1'b0;
            end
        end
        else if (clear_flags_pulse)
        begin
            error_stop_reg        <= 1'b0;
            error_state_reg       <= 1'b0;
            sync_seen_reg         <= 1'b0;
            button_seen_reg       <= 1'b0;
            batch_seen_reg        <= 1'b0;
            sample_count_reg      <= 5'd29;
            batch_count_reg       <= 9'd0;
            if ((state_reg == ST_DONE) || (state_reg == ST_ERROR))
            begin
                state_reg <= ST_IDLE;
            end
        end
        else if (stop_pulse)
        begin
            if (state_reg != ST_IDLE)
            begin
                state_reg <= ST_ERROR;
                error_stop_reg <= 1'b1;
                irq_reg   <= cfg_irq_error_en;
            end
            sample_count_reg <= 5'd29;
            batch_count_reg  <= 9'd0;
        end
        else
        begin
            case (state_reg)
                ST_IDLE:
                begin
                    sample_count_reg <= 5'd29;
                    batch_count_reg  <= 9'd0;
                    if (arm_pulse)
                    begin
                        state_reg <= ST_ARMED;
                    end
                    else if (start_now_pulse)
                    begin
                        state_reg <= ST_SAMPLING;
                    end
                end

                ST_ARMED:
                begin
                    sample_count_reg <= 5'd29;
                    batch_count_reg  <= 9'd0;
                    if (start_now_pulse || (ctrl_use_sync && sync_det))
                    begin
                        state_reg <= ST_SAMPLING;
                        if (sync_det)
                        begin
                            sync_seen_reg <= 1'b1;
                            irq_reg <= cfg_irq_sync_en;
                        end
                    end
                end

                ST_SAMPLING:
                begin
                    if (sample_done_rise)
                    begin
                        if (sample_will_close_batch)
                        begin
                            sample_count_reg <= 5'd29;
                            batch_seen_reg   <= 1'b1;

                            if (batch_will_finish_capture)
                            begin
                                state_reg <= ST_DONE;
                                irq_reg   <= 1'b1;
                            end
                            else if (cfg_irq_batch_en)
                            begin
                                batch_count_reg <= batch_count_reg + 9'd1;
                                irq_reg <= 1'b1;
                            end
                            else
                            begin
                                batch_count_reg <= batch_count_reg + 9'd1;
                            end
                        end
                        else
                        begin
                            sample_count_reg <= sample_count_reg - 5'd1;
                        end
                    end
                end

                ST_DONE:
                begin
                    if (arm_pulse)
                    begin
                        state_reg <= ST_ARMED;
                        sample_count_reg <= 5'd29;
                        batch_count_reg  <= 9'd0;
                    end
                    else if (start_now_pulse)
                    begin
                        state_reg <= ST_SAMPLING;
                        sample_count_reg <= 5'd29;
                        batch_count_reg  <= 9'd0;
                    end
                end

                ST_ERROR:
                begin
                    sample_count_reg <= 5'd29;
                    batch_count_reg  <= 9'd0;
                end

                default:
                begin
                    state_reg <= ST_ERROR;
                    error_state_reg <= 1'b1;
                    irq_reg   <= cfg_irq_error_en;
                end
            endcase
        end
    end
end

//`#end` -- edit above this line, do not edit this line
endmodule
//`#start footer` -- edit after this line, do not edit this line
//`#end` -- edit above this line, do not edit this line
