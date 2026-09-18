#ifndef CONTROL_CONFIG_H
#define CONTROL_CONFIG_H
#include <stdint.h>

/* Wire/NVM order is explicit, never sizeof(struct). Values are signed LE32.
 * Physical slopes use microvolts/code; times use milliseconds. */
enum ControlParameter {
    CP_PGA, CP_PGAOUT, CP_AVERAGE, CP_CAPACITOR, CP_PERIOD_MS,
    CP_TAU_MS, CP_KP_NUM, CP_KP_DEN, CP_DEADBAND_UV, CP_ENTER_MS,
    CP_FINE_STEP, CP_RESCUE_STEP, CP_RESCUE_MS, CP_SUM_BAND_UV,
    CP_COARSE_STEP, CP_FINE_SLOPE_UV, CP_COARSE_SLOPE_UV,
    CP_SLOW0_SLOPE_UV, CP_SLOW1_SLOPE_UV, CP_SLOW_TAU_MS,
    CP_OPA_TARGET_UV, CP_LEARN_BAND_UV, CP_LEARN_TIMEOUT_MS,
    CP_SCAN_MS, CP_REPORT_MS, CP_CAPTURE_CHANNEL, CP_SETTLE_SAMPLES,
    CP_VALID_LOW_UV, CP_VALID_HIGH_UV, CP_MARGIN_UV,
    CP_INITIAL2, CP_INITIAL3, CP_STABLE_UV, CP_STABLE_MS,
    CP_FINE_LIMIT, CP_COARSE_LIMIT,
    /* v4: hysteresis hold band, fine mid-range, coarse cooldown and the
     * blind window after a telemetry burst. */
    CP_HOLD_UV, CP_FINE_MID, CP_COARSE_MS, CP_QUIET_MS,
    /* Settled mode: after SETTLED_MS frozen, a wider wake band that must be
     * exceeded WAKE_COUNT readings in a row, and slower scan and reports. */
    CP_SETTLED_MS, CP_SETTLED_BAND_UV, CP_WAKE_COUNT,
    CP_SCAN_SETTLED_MS, CP_REPORT_SETTLED_MS,
    /* v5: recentrado lento.  Congelado dentro de la banda, el lazo no vuelve
     * al centro nunca: la deriva lo deja pegado a un borde y la siguiente
     * correccion sale de golpe.  Si lleva RECENTER_MS con |LPo| pasado
     * RECENTER_UV, mueve UN codigo hacia el centro y se vuelve a congelar. */
    CP_RECENTER_UV, CP_RECENTER_MS, CP_COUNT
};
/* Telemetry frame version stays 3 (ESP accepts 1..3; keys are generic).
 * The persistent image carries its own layout version. */
#define CONTROL_CONFIG_VERSION 3u
#define CONTROL_NV_VERSION 5u
#define CONTROL_CONFIG_COUNT_V3 36u
#define CONTROL_CONFIG_COUNT_V4 45u
#define CONTROL_CONFIG_BYTES (CP_COUNT * 4u)
typedef struct { int32_t value[CP_COUNT]; } ControlConfig;
void control_config_defaults(ControlConfig *c);
int control_config_valid(const ControlConfig *c, unsigned channels, unsigned has_cap);
int control_config_profile_compatible(const ControlConfig *a, const ControlConfig *b);
void control_config_encode(const ControlConfig *c, uint8_t *out);
void control_config_decode(ControlConfig *c, const uint8_t *in);
/* Decodes the first n parameters over defaults (older NV layouts). */
void control_config_decode_n(ControlConfig *c, const uint8_t *in, unsigned n);
uint16_t control_crc(const uint8_t *data, unsigned n);
/* The ADC can return a stable, plausible number while its analog input buffer
 * is outside common mode.  A sample is usable only inside the measured
 * bank-domain window, excluding the configured guard margin. */
int control_measurement_valid(const ControlConfig *c, int32_t dc_uv);

typedef struct {
    int16_t fine, coarse;
    int32_t previous_error;
    int64_t fraction;
    uint32_t previous_ms, rescue_ms;
    uint32_t coarse_ms, hold_ms, recenter_ms;
    uint8_t has_previous, has_rescue, has_coarse, holding, wake_count;
    /* Biseccion del rescate: paso vigente y signo del ultimo movimiento. */
    int16_t rescue_step;
    int8_t rescue_dir;
} ControlPI;
/* True once the loop has stayed frozen for SETTLED_MS. */
int control_pi_settled(const ControlPI *p, const ControlConfig *c, uint32_t now);
/* One call per fresh measurement. Outputs are absolute signed IDAC codes. */
void control_pi_step(ControlPI *p, const ControlConfig *c, uint32_t now,
                     int32_t lp_uv, int lp_valid, int32_t sum_uv, int sum_valid);
#endif
