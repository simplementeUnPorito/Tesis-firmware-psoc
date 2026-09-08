/*******************************************************************************
* File Name: WaveDAC8_p.h  
* Version 2.10
*
* Description:
*  This file contains the function prototypes and constants used in
*  the 8-bit Waveform DAC (WaveDAC8) Component.
*
********************************************************************************
* Copyright 2013, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions, 
* disclaimers, and limitations in the end user license agreement accompanying 
* the software package with which this file was provided.
*******************************************************************************/

#if !defined(CY_WaveDAC8_WaveDAC8_p_H) 
#define CY_WaveDAC8_WaveDAC8_p_H

#include "cytypes.h"
#include "cyfitter.h"
#include <WaveDAC8_p_Wave1_DMA_dma.h>
#include <WaveDAC8_p_Wave2_DMA_dma.h>
#include <WaveDAC8_p_VDAC8.h>


/***************************************
*  Initial Parameter Constants
***************************************/

#define WaveDAC8_p_WAVE1_TYPE     (0u)     /* Waveform for wave1 */
#define WaveDAC8_p_WAVE2_TYPE     (2u)     /* Waveform for wave2 */
#define WaveDAC8_p_SINE_WAVE      (0u)
#define WaveDAC8_p_SQUARE_WAVE    (1u)
#define WaveDAC8_p_TRIANGLE_WAVE  (2u)
#define WaveDAC8_p_SAWTOOTH_WAVE  (3u)
#define WaveDAC8_p_ARB_DRAW_WAVE  (10u) /* Arbitrary (draw) */
#define WaveDAC8_p_ARB_FILE_WAVE  (11u) /* Arbitrary (from file) */

#define WaveDAC8_p_WAVE1_LENGTH   (100u)   /* Length for wave1 */
#define WaveDAC8_p_WAVE2_LENGTH   (100u)   /* Length for wave2 */
	
#define WaveDAC8_p_DEFAULT_RANGE    (1u) /* Default DAC range */
#define WaveDAC8_p_DAC_RANGE_1V     (0u)
#define WaveDAC8_p_DAC_RANGE_1V_BUF (16u)
#define WaveDAC8_p_DAC_RANGE_4V     (1u)
#define WaveDAC8_p_DAC_RANGE_4V_BUF (17u)
#define WaveDAC8_p_VOLT_MODE        (0u)
#define WaveDAC8_p_CURRENT_MODE     (1u)
#define WaveDAC8_p_DAC_MODE         (((WaveDAC8_p_DEFAULT_RANGE == WaveDAC8_p_DAC_RANGE_1V) || \
									  (WaveDAC8_p_DEFAULT_RANGE == WaveDAC8_p_DAC_RANGE_4V) || \
							  		  (WaveDAC8_p_DEFAULT_RANGE == WaveDAC8_p_DAC_RANGE_1V_BUF) || \
									  (WaveDAC8_p_DEFAULT_RANGE == WaveDAC8_p_DAC_RANGE_4V_BUF)) ? \
									   WaveDAC8_p_VOLT_MODE : WaveDAC8_p_CURRENT_MODE)

#define WaveDAC8_p_DACMODE WaveDAC8_p_DAC_MODE /* legacy definition for backward compatibility */

#define WaveDAC8_p_DIRECT_MODE (0u)
#define WaveDAC8_p_BUFFER_MODE (1u)
#define WaveDAC8_p_OUT_MODE    (((WaveDAC8_p_DEFAULT_RANGE == WaveDAC8_p_DAC_RANGE_1V_BUF) || \
								 (WaveDAC8_p_DEFAULT_RANGE == WaveDAC8_p_DAC_RANGE_4V_BUF)) ? \
								  WaveDAC8_p_BUFFER_MODE : WaveDAC8_p_DIRECT_MODE)

#if(WaveDAC8_p_OUT_MODE == WaveDAC8_p_BUFFER_MODE)
    #include <WaveDAC8_p_BuffAmp.h>
#endif /* WaveDAC8_p_OUT_MODE == WaveDAC8_p_BUFFER_MODE */

#define WaveDAC8_p_CLOCK_INT      (1u)
#define WaveDAC8_p_CLOCK_EXT      (0u)
#define WaveDAC8_p_CLOCK_SRC      (0u)

#if(WaveDAC8_p_CLOCK_SRC == WaveDAC8_p_CLOCK_INT)  
	#include <WaveDAC8_p_DacClk.h>
	#if defined(WaveDAC8_p_DacClk_PHASE)
		#define WaveDAC8_p_CLK_PHASE_0nS (1u)
	#endif /* defined(WaveDAC8_p_DacClk_PHASE) */
#endif /* WaveDAC8_p_CLOCK_SRC == WaveDAC8_p_CLOCK_INT */

#if (CY_PSOC3)
	#define WaveDAC8_p_HI16FLASHPTR   (0xFFu)
#endif /* CY_PSOC3 */

#define WaveDAC8_p_Wave1_DMA_BYTES_PER_BURST      (1u)
#define WaveDAC8_p_Wave1_DMA_REQUEST_PER_BURST    (1u)
#define WaveDAC8_p_Wave2_DMA_BYTES_PER_BURST      (1u)
#define WaveDAC8_p_Wave2_DMA_REQUEST_PER_BURST    (1u)


/***************************************
*   Data Struct Definition
***************************************/

/* Low power Mode API Support */
typedef struct
{
	uint8   enableState;
}WaveDAC8_p_BACKUP_STRUCT;


/***************************************
*        Function Prototypes 
***************************************/

void WaveDAC8_p_Start(void)             ;
void WaveDAC8_p_StartEx(const uint8 * wavePtr1, uint16 sampleSize1, const uint8 * wavePtr2, uint16 sampleSize2)
                                        ;
void WaveDAC8_p_Init(void)              ;
void WaveDAC8_p_Enable(void)            ;
void WaveDAC8_p_Stop(void)              ;

void WaveDAC8_p_Wave1Setup(const uint8 * wavePtr, uint16 sampleSize)
                                        ;
void WaveDAC8_p_Wave2Setup(const uint8 * wavePtr, uint16 sampleSize)
                                        ;

void WaveDAC8_p_Sleep(void)             ;
void WaveDAC8_p_Wakeup(void)            ;

#define WaveDAC8_p_SetSpeed       WaveDAC8_p_VDAC8_SetSpeed
#define WaveDAC8_p_SetRange       WaveDAC8_p_VDAC8_SetRange
#define WaveDAC8_p_SetValue       WaveDAC8_p_VDAC8_SetValue
#define WaveDAC8_p_DacTrim        WaveDAC8_p_VDAC8_DacTrim
#define WaveDAC8_p_SaveConfig     WaveDAC8_p_VDAC8_SaveConfig
#define WaveDAC8_p_RestoreConfig  WaveDAC8_p_VDAC8_RestoreConfig


/***************************************
*    Variable with external linkage 
***************************************/

extern uint8 WaveDAC8_p_initVar;

extern const uint8 CYCODE WaveDAC8_p_wave1[WaveDAC8_p_WAVE1_LENGTH];
extern const uint8 CYCODE WaveDAC8_p_wave2[WaveDAC8_p_WAVE2_LENGTH];


/***************************************
*            API Constants
***************************************/

/* SetRange constants */
#if(WaveDAC8_p_DAC_MODE == WaveDAC8_p_VOLT_MODE)
    #define WaveDAC8_p_RANGE_1V       (0x00u)
    #define WaveDAC8_p_RANGE_4V       (0x04u)
#else /* current mode */
    #define WaveDAC8_p_RANGE_32uA     (0x00u)
    #define WaveDAC8_p_RANGE_255uA    (0x04u)
    #define WaveDAC8_p_RANGE_2mA      (0x08u)
    #define WaveDAC8_p_RANGE_2048uA   WaveDAC8_p_RANGE_2mA
#endif /* WaveDAC8_p_DAC_MODE == WaveDAC8_p_VOLT_MODE */

/* Power setting for SetSpeed API */
#define WaveDAC8_p_LOWSPEED       (0x00u)
#define WaveDAC8_p_HIGHSPEED      (0x02u)


/***************************************
*              Registers        
***************************************/

#define WaveDAC8_p_DAC8__D WaveDAC8_p_VDAC8_viDAC8__D


/***************************************
*         Register Constants       
***************************************/

/* CR0 vDac Control Register 0 definitions */

/* Bit Field  DAC_HS_MODE */
#define WaveDAC8_p_HS_MASK        (0x02u)
#define WaveDAC8_p_HS_LOWPOWER    (0x00u)
#define WaveDAC8_p_HS_HIGHSPEED   (0x02u)

/* Bit Field  DAC_MODE */
#define WaveDAC8_p_MODE_MASK      (0x10u)
#define WaveDAC8_p_MODE_V         (0x00u)
#define WaveDAC8_p_MODE_I         (0x10u)

/* Bit Field  DAC_RANGE */
#define WaveDAC8_p_RANGE_MASK     (0x0Cu)
#define WaveDAC8_p_RANGE_0        (0x00u)
#define WaveDAC8_p_RANGE_1        (0x04u)
#define WaveDAC8_p_RANGE_2        (0x08u)
#define WaveDAC8_p_RANGE_3        (0x0Cu)
#define WaveDAC8_p_IDIR_MASK      (0x04u)

#define WaveDAC8_p_DAC_RANGE      ((uint8)(1u << 2u) & WaveDAC8_p_RANGE_MASK)
#define WaveDAC8_p_DAC_POL        ((uint8)(1u >> 1u) & WaveDAC8_p_IDIR_MASK)


#endif /* CY_WaveDAC8_WaveDAC8_p_H  */

/* [] END OF FILE */
