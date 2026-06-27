#ifndef CALIBRATION_LEGACY_TYPES_H
#define CALIBRATION_LEGACY_TYPES_H

/* Tipos historicos de la calibracion por busqueda binaria/verify/realcheck.
 * No incluir desde calibration.h ni desde calibration_tables.h: el firmware
 * activo usa PI-only. Este archivo existe solo para conservar la forma vieja
 * sin contaminar las estructuras activas. */

typedef struct {
    uint8  avg_n;
    uint8  window_count;
    uint16 max_samples;
    int32  settle_tol_counts;
    uint8  stable_streak;
} PsocCalLegacyAvgCfg;

typedef struct {
    uint8  enable;
    int32  tol_counts;
    uint8  nudge_step;
    uint8  max_nudges;
    uint16 discard_samples;
    uint16 nudge_discard_samples;
    PsocCalLegacyAvgCfg avg;
} PsocCalLegacyRealcheckCfg;

#endif
