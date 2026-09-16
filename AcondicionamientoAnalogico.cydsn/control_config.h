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
    CP_FINE_LIMIT, CP_COARSE_LIMIT, CP_COUNT
};
#define CONTROL_CONFIG_VERSION 1u
#define CONTROL_CONFIG_BYTES (CP_COUNT * 4u)
typedef struct { int32_t value[CP_COUNT]; } ControlConfig;
void control_config_defaults(ControlConfig *c);
int control_config_valid(const ControlConfig *c, unsigned channels, unsigned has_cap);
int control_config_profile_compatible(const ControlConfig *a, const ControlConfig *b);
void control_config_encode(const ControlConfig *c, uint8_t *out);
void control_config_decode(ControlConfig *c, const uint8_t *in);
uint16_t control_crc(const uint8_t *data, unsigned n);

typedef struct {
    int16_t fine, coarse;
    int32_t previous_error;
    int64_t fraction;
    uint32_t previous_ms, rescue_ms;
    uint8_t has_previous, has_rescue;
} ControlPI;
/* One call per fresh measurement. Outputs are absolute signed IDAC codes. */
void control_pi_step(ControlPI *p, const ControlConfig *c, uint32_t now,
                     int32_t lp_uv, int lp_valid, int32_t sum_uv, int sum_valid);
#endif
