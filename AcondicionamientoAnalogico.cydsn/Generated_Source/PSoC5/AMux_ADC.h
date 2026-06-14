/*******************************************************************************
* File Name: AMux_ADC.h
* Version 1.80
*
*  Description:
*    This file contains the constants and function prototypes for the Analog
*    Multiplexer User Module AMux.
*
*   Note:
*
********************************************************************************
* Copyright 2008-2010, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions, 
* disclaimers, and limitations in the end user license agreement accompanying 
* the software package with which this file was provided.
********************************************************************************/

#if !defined(CY_AMUX_AMux_ADC_H)
#define CY_AMUX_AMux_ADC_H

#include "cyfitter.h"
#include "cyfitter_cfg.h"

#if ((CYDEV_CHIP_FAMILY_USED == CYDEV_CHIP_FAMILY_PSOC3) || \
         (CYDEV_CHIP_FAMILY_USED == CYDEV_CHIP_FAMILY_PSOC4) || \
         (CYDEV_CHIP_FAMILY_USED == CYDEV_CHIP_FAMILY_PSOC5))    
    #include "cytypes.h"
#else
    #include "syslib/cy_syslib.h"
#endif /* ((CYDEV_CHIP_FAMILY_USED == CYDEV_CHIP_FAMILY_PSOC3) */


/***************************************
*        Function Prototypes
***************************************/

void AMux_ADC_Start(void) ;
#define AMux_ADC_Init() AMux_ADC_Start()
void AMux_ADC_FastSelect(uint8 channel) ;
/* The Stop, Select, Connect, Disconnect and DisconnectAll functions are declared elsewhere */
/* void AMux_ADC_Stop(void); */
/* void AMux_ADC_Select(uint8 channel); */
/* void AMux_ADC_Connect(uint8 channel); */
/* void AMux_ADC_Disconnect(uint8 channel); */
/* void AMux_ADC_DisconnectAll(void) */


/***************************************
*         Parameter Constants
***************************************/

#define AMux_ADC_CHANNELS  4u
#define AMux_ADC_MUXTYPE   1
#define AMux_ADC_ATMOSTONE 1

/***************************************
*             API Constants
***************************************/

#define AMux_ADC_NULL_CHANNEL 0xFFu
#define AMux_ADC_MUX_SINGLE   1
#define AMux_ADC_MUX_DIFF     2


/***************************************
*        Conditional Functions
***************************************/

#if AMux_ADC_MUXTYPE == AMux_ADC_MUX_SINGLE
# if !AMux_ADC_ATMOSTONE
#  define AMux_ADC_Connect(channel) AMux_ADC_Set(channel)
# endif
# define AMux_ADC_Disconnect(channel) AMux_ADC_Unset(channel)
#else
# if !AMux_ADC_ATMOSTONE
void AMux_ADC_Connect(uint8 channel) ;
# endif
void AMux_ADC_Disconnect(uint8 channel) ;
#endif

#if AMux_ADC_ATMOSTONE
# define AMux_ADC_Stop() AMux_ADC_DisconnectAll()
# define AMux_ADC_Select(channel) AMux_ADC_FastSelect(channel)
void AMux_ADC_DisconnectAll(void) ;
#else
# define AMux_ADC_Stop() AMux_ADC_Start()
void AMux_ADC_Select(uint8 channel) ;
# define AMux_ADC_DisconnectAll() AMux_ADC_Start()
#endif

#endif /* CY_AMUX_AMux_ADC_H */


/* [] END OF FILE */
