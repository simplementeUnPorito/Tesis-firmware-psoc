/*******************************************************************************
* File Name: AMux_Vref.h
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

#if !defined(CY_AMUX_AMux_Vref_H)
#define CY_AMUX_AMux_Vref_H

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

void AMux_Vref_Start(void) ;
#define AMux_Vref_Init() AMux_Vref_Start()
void AMux_Vref_FastSelect(uint8 channel) ;
/* The Stop, Select, Connect, Disconnect and DisconnectAll functions are declared elsewhere */
/* void AMux_Vref_Stop(void); */
/* void AMux_Vref_Select(uint8 channel); */
/* void AMux_Vref_Connect(uint8 channel); */
/* void AMux_Vref_Disconnect(uint8 channel); */
/* void AMux_Vref_DisconnectAll(void) */


/***************************************
*         Parameter Constants
***************************************/

#define AMux_Vref_CHANNELS  2u
#define AMux_Vref_MUXTYPE   1
#define AMux_Vref_ATMOSTONE 0

/***************************************
*             API Constants
***************************************/

#define AMux_Vref_NULL_CHANNEL 0xFFu
#define AMux_Vref_MUX_SINGLE   1
#define AMux_Vref_MUX_DIFF     2


/***************************************
*        Conditional Functions
***************************************/

#if AMux_Vref_MUXTYPE == AMux_Vref_MUX_SINGLE
# if !AMux_Vref_ATMOSTONE
#  define AMux_Vref_Connect(channel) AMux_Vref_Set(channel)
# endif
# define AMux_Vref_Disconnect(channel) AMux_Vref_Unset(channel)
#else
# if !AMux_Vref_ATMOSTONE
void AMux_Vref_Connect(uint8 channel) ;
# endif
void AMux_Vref_Disconnect(uint8 channel) ;
#endif

#if AMux_Vref_ATMOSTONE
# define AMux_Vref_Stop() AMux_Vref_DisconnectAll()
# define AMux_Vref_Select(channel) AMux_Vref_FastSelect(channel)
void AMux_Vref_DisconnectAll(void) ;
#else
# define AMux_Vref_Stop() AMux_Vref_Start()
void AMux_Vref_Select(uint8 channel) ;
# define AMux_Vref_DisconnectAll() AMux_Vref_Start()
#endif

#endif /* CY_AMUX_AMux_Vref_H */


/* [] END OF FILE */
