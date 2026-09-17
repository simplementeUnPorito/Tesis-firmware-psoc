/*******************************************************************************
* File Name: AMux_IN.h
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

#if !defined(CY_AMUX_AMux_IN_H)
#define CY_AMUX_AMux_IN_H

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

void AMux_IN_Start(void) ;
#define AMux_IN_Init() AMux_IN_Start()
void AMux_IN_FastSelect(uint8 channel) ;
/* The Stop, Select, Connect, Disconnect and DisconnectAll functions are declared elsewhere */
/* void AMux_IN_Stop(void); */
/* void AMux_IN_Select(uint8 channel); */
/* void AMux_IN_Connect(uint8 channel); */
/* void AMux_IN_Disconnect(uint8 channel); */
/* void AMux_IN_DisconnectAll(void) */


/***************************************
*         Parameter Constants
***************************************/

#define AMux_IN_CHANNELS  2u
#define AMux_IN_MUXTYPE   2
#define AMux_IN_ATMOSTONE 1

/***************************************
*             API Constants
***************************************/

#define AMux_IN_NULL_CHANNEL 0xFFu
#define AMux_IN_MUX_SINGLE   1
#define AMux_IN_MUX_DIFF     2


/***************************************
*        Conditional Functions
***************************************/

#if AMux_IN_MUXTYPE == AMux_IN_MUX_SINGLE
# if !AMux_IN_ATMOSTONE
#  define AMux_IN_Connect(channel) AMux_IN_Set(channel)
# endif
# define AMux_IN_Disconnect(channel) AMux_IN_Unset(channel)
#else
# if !AMux_IN_ATMOSTONE
void AMux_IN_Connect(uint8 channel) ;
# endif
void AMux_IN_Disconnect(uint8 channel) ;
#endif

#if AMux_IN_ATMOSTONE
# define AMux_IN_Stop() AMux_IN_DisconnectAll()
# define AMux_IN_Select(channel) AMux_IN_FastSelect(channel)
void AMux_IN_DisconnectAll(void) ;
#else
# define AMux_IN_Stop() AMux_IN_Start()
void AMux_IN_Select(uint8 channel) ;
# define AMux_IN_DisconnectAll() AMux_IN_Start()
#endif

#endif /* CY_AMUX_AMux_IN_H */


/* [] END OF FILE */
