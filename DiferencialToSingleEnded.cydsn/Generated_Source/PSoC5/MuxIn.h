/*******************************************************************************
* File Name: MuxIn.h
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

#if !defined(CY_AMUX_MuxIn_H)
#define CY_AMUX_MuxIn_H

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

void MuxIn_Start(void) ;
#define MuxIn_Init() MuxIn_Start()
void MuxIn_FastSelect(uint8 channel) ;
/* The Stop, Select, Connect, Disconnect and DisconnectAll functions are declared elsewhere */
/* void MuxIn_Stop(void); */
/* void MuxIn_Select(uint8 channel); */
/* void MuxIn_Connect(uint8 channel); */
/* void MuxIn_Disconnect(uint8 channel); */
/* void MuxIn_DisconnectAll(void) */


/***************************************
*         Parameter Constants
***************************************/

#define MuxIn_CHANNELS  2u
#define MuxIn_MUXTYPE   2
#define MuxIn_ATMOSTONE 0

/***************************************
*             API Constants
***************************************/

#define MuxIn_NULL_CHANNEL 0xFFu
#define MuxIn_MUX_SINGLE   1
#define MuxIn_MUX_DIFF     2


/***************************************
*        Conditional Functions
***************************************/

#if MuxIn_MUXTYPE == MuxIn_MUX_SINGLE
# if !MuxIn_ATMOSTONE
#  define MuxIn_Connect(channel) MuxIn_Set(channel)
# endif
# define MuxIn_Disconnect(channel) MuxIn_Unset(channel)
#else
# if !MuxIn_ATMOSTONE
void MuxIn_Connect(uint8 channel) ;
# endif
void MuxIn_Disconnect(uint8 channel) ;
#endif

#if MuxIn_ATMOSTONE
# define MuxIn_Stop() MuxIn_DisconnectAll()
# define MuxIn_Select(channel) MuxIn_FastSelect(channel)
void MuxIn_DisconnectAll(void) ;
#else
# define MuxIn_Stop() MuxIn_Start()
void MuxIn_Select(uint8 channel) ;
# define MuxIn_DisconnectAll() MuxIn_Start()
#endif

#endif /* CY_AMUX_MuxIn_H */


/* [] END OF FILE */
