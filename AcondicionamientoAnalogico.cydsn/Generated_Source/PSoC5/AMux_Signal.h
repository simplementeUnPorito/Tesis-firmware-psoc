/*******************************************************************************
* File Name: AMux_Signal.h
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

#if !defined(CY_AMUX_AMux_Signal_H)
#define CY_AMUX_AMux_Signal_H

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

void AMux_Signal_Start(void) ;
#define AMux_Signal_Init() AMux_Signal_Start()
void AMux_Signal_FastSelect(uint8 channel) ;
/* The Stop, Select, Connect, Disconnect and DisconnectAll functions are declared elsewhere */
/* void AMux_Signal_Stop(void); */
/* void AMux_Signal_Select(uint8 channel); */
/* void AMux_Signal_Connect(uint8 channel); */
/* void AMux_Signal_Disconnect(uint8 channel); */
/* void AMux_Signal_DisconnectAll(void) */


/***************************************
*         Parameter Constants
***************************************/

#define AMux_Signal_CHANNELS  2u
#define AMux_Signal_MUXTYPE   1
#define AMux_Signal_ATMOSTONE 0

/***************************************
*             API Constants
***************************************/

#define AMux_Signal_NULL_CHANNEL 0xFFu
#define AMux_Signal_MUX_SINGLE   1
#define AMux_Signal_MUX_DIFF     2


/***************************************
*        Conditional Functions
***************************************/

#if AMux_Signal_MUXTYPE == AMux_Signal_MUX_SINGLE
# if !AMux_Signal_ATMOSTONE
#  define AMux_Signal_Connect(channel) AMux_Signal_Set(channel)
# endif
# define AMux_Signal_Disconnect(channel) AMux_Signal_Unset(channel)
#else
# if !AMux_Signal_ATMOSTONE
void AMux_Signal_Connect(uint8 channel) ;
# endif
void AMux_Signal_Disconnect(uint8 channel) ;
#endif

#if AMux_Signal_ATMOSTONE
# define AMux_Signal_Stop() AMux_Signal_DisconnectAll()
# define AMux_Signal_Select(channel) AMux_Signal_FastSelect(channel)
void AMux_Signal_DisconnectAll(void) ;
#else
# define AMux_Signal_Stop() AMux_Signal_Start()
void AMux_Signal_Select(uint8 channel) ;
# define AMux_Signal_DisconnectAll() AMux_Signal_Start()
#endif

#endif /* CY_AMUX_AMux_Signal_H */


/* [] END OF FILE */
