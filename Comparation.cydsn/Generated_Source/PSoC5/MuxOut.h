/*******************************************************************************
* File Name: MuxOut.h
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

#if !defined(CY_AMUX_MuxOut_H)
#define CY_AMUX_MuxOut_H

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

void MuxOut_Start(void) ;
#define MuxOut_Init() MuxOut_Start()
void MuxOut_FastSelect(uint8 channel) ;
/* The Stop, Select, Connect, Disconnect and DisconnectAll functions are declared elsewhere */
/* void MuxOut_Stop(void); */
/* void MuxOut_Select(uint8 channel); */
/* void MuxOut_Connect(uint8 channel); */
/* void MuxOut_Disconnect(uint8 channel); */
/* void MuxOut_DisconnectAll(void) */


/***************************************
*         Parameter Constants
***************************************/

#define MuxOut_CHANNELS  2u
#define MuxOut_MUXTYPE   2
#define MuxOut_ATMOSTONE 0

/***************************************
*             API Constants
***************************************/

#define MuxOut_NULL_CHANNEL 0xFFu
#define MuxOut_MUX_SINGLE   1
#define MuxOut_MUX_DIFF     2


/***************************************
*        Conditional Functions
***************************************/

#if MuxOut_MUXTYPE == MuxOut_MUX_SINGLE
# if !MuxOut_ATMOSTONE
#  define MuxOut_Connect(channel) MuxOut_Set(channel)
# endif
# define MuxOut_Disconnect(channel) MuxOut_Unset(channel)
#else
# if !MuxOut_ATMOSTONE
void MuxOut_Connect(uint8 channel) ;
# endif
void MuxOut_Disconnect(uint8 channel) ;
#endif

#if MuxOut_ATMOSTONE
# define MuxOut_Stop() MuxOut_DisconnectAll()
# define MuxOut_Select(channel) MuxOut_FastSelect(channel)
void MuxOut_DisconnectAll(void) ;
#else
# define MuxOut_Stop() MuxOut_Start()
void MuxOut_Select(uint8 channel) ;
# define MuxOut_DisconnectAll() MuxOut_Start()
#endif

#endif /* CY_AMUX_MuxOut_H */


/* [] END OF FILE */
