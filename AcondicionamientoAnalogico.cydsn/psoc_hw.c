#include "psoc_hw.h"

void psoc_hw_set_pga(uint8 code)
{
#if PSOC_HW_CLASS == PSOC_HW_GEO
    PGAgain_SetGain(code);
#else
    PGA_SetGain(code);
#endif
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
    LPF_ref_Start();
    LPF_ADC_Start();
    Opa_ref_IN_Start();
    Opa_ref_PGA_Start();
    PGA_Start();
    psoc_hw_set_pga(pga_code);
    Opa_LP_Start();
    (void)pgavdac_code;
#endif
}
