#include "psoc_hw.h"

static uint8 g_psoc_hw_pga_code = PSOC_PGA_DEFAULT_CODE;

static uint16 psoc_hw_pga_code_to_gain_x1000(uint8 code)
{
    switch (code) {
        case 0u: return 1000u;
        case 1u: return 2000u;
        case 2u: return 4000u;
        case 3u: return 8000u;
        case 4u: return 16000u;
        case 5u: return 24000u;
        case 6u: return 32000u;
        case 7u: return 48000u;
        case 8u: return 50000u;
        default: return 1000u;
    }
}

void psoc_hw_set_pga(uint8 code)
{
    g_psoc_hw_pga_code = code;
#if PSOC_HW_CLASS == PSOC_HW_GEO
    PGAgain_SetGain(code);
#else
    PGA_SetGain(code);
#endif
}

uint8 psoc_hw_get_pga_code(void)
{
    return g_psoc_hw_pga_code;
}

uint16 psoc_hw_pga_gain_x1000(void)
{
    return psoc_hw_pga_code_to_gain_x1000(g_psoc_hw_pga_code);
}

void psoc_hw_set_pgavdac(uint8 code)
{
#if PSOC_HW_CLASS == PSOC_HW_GEO
    #if defined(PGAvdac_DEFAULT_GAIN)
    PGAvdac_SetGain(code);
    #else
    (void)code;
    #endif
#else
    #if defined(PGA_ref_DEFAULT_GAIN)
    PGA_ref_SetGain(code);
    #else
    (void)code;
    #endif
#endif
}

void psoc_hw_start_analog(uint8 pga_code, uint8 pgavdac_code)
{
#if PSOC_HW_CLASS == PSOC_HW_GEO
    OPAref_Start();
    PGAshield_Start();
    PGAp_Start();
    PGAn_Start();
    PGAp_SetGain(PGAp_GAIN_02);
    PGAn_SetGain(PGAn_GAIN_02);
    LPF_1_Start();
    LPF_2_Start();
    PGAgain_Start();
    psoc_hw_set_pga(pga_code);
    OPAbp_Start();
    OPAadder_Start();
    #if defined(PGAvdac_DEFAULT_GAIN)
    PGAvdac_Start();
    psoc_hw_set_pgavdac(pgavdac_code);
    #else
    (void)pgavdac_code;
    #endif
    OPAlp_Start();
#else
    Opa_ref_1V_Start();
    LPF_ADC_Start();
    Opa_ref_PGA_Start();
    PGA_Start();
    psoc_hw_set_pga(pga_code);
    Opa_LP_Start();
    (void)pgavdac_code;
#endif
}
