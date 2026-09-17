/*******************************************************************************
* File Name: LPF_bypass.h
* Version 1.60
*
*  Description:
*    This file contains the constants and function prototypes for the Analog
*    Multiplexer User Module AMux.
*
*   Note:
*
*******************************************************************************
* Copyright 2012-2013, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions, 
* disclaimers, and limitations in the end user license agreement accompanying 
* the software package with which this file was provided.
********************************************************************************/

#if !defined(CY_AMUX_LPF_bypass_H)
#define CY_AMUX_LPF_bypass_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cyfitter_cfg.h"

/* Check to see if required defines such as CY_PSOC5LP are available */
/* They are defined starting with cy_boot v3.0 */
#if !defined (CY_PSOC5LP)
    #error Component PM_AMux_v1_60 requires cy_boot v3.0 or later
#endif /* (CY_ PSOC5LP) */


/***************************************
*        Function Prototypes
***************************************/

void LPF_bypass_Start(void) ;
void LPF_bypass_Init(void) ;
void LPF_bypass_Stop(void) ;
void LPF_bypass_Select(uint8 channel) ;
void LPF_bypass_FastSelect(uint8 channel) ;
void LPF_bypass_DisconnectAll(void) ;


/***************************************
*     Initial Parameter Constants
***************************************/

#define LPF_bypass_CHANNELS  2u
#define LPF_bypass_MUXTYPE   1u


/***************************************
*             API Constants
***************************************/

#define LPF_bypass_NULL_CHANNEL  0xFFu
#define LPF_bypass_MUX_SINGLE   1u
#define LPF_bypass_MUX_DIFF     2u


/***************************************
*        Conditional Functions
***************************************/

#if (LPF_bypass_MUXTYPE == LPF_bypass_MUX_SINGLE)
    # define LPF_bypass_Connect(channel) LPF_bypass_Set(channel)
    # define LPF_bypass_Disconnect(channel) LPF_bypass_Unset(channel)
#else
    void LPF_bypass_Connect(uint8 channel) ;
    void LPF_bypass_Disconnect(uint8 channel) ;
#endif /* LPF_bypass_MUXTYPE == LPF_bypass_MUX_SINGLE */

#endif /* CY_AMUX_LPF_bypass_H */


/* [] END OF FILE */
