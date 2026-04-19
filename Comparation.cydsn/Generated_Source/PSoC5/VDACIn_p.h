/*******************************************************************************
* File Name: VDACIn_p.h  
* Version 1.90
*
*  Description:
*    This file contains the function prototypes and constants used in
*    the 8-bit Voltage DAC (vDAC8) User Module.
*
*   Note:
*     
*
********************************************************************************
* Copyright 2008-2012, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions, 
* disclaimers, and limitations in the end user license agreement accompanying 
* the software package with which this file was provided.
*******************************************************************************/

#if !defined(CY_VDAC8_VDACIn_p_H) 
#define CY_VDAC8_VDACIn_p_H

#include "cytypes.h"
#include "cyfitter.h"

/* Check to see if required defines such as CY_PSOC5LP are available */
/* They are defined starting with cy_boot v3.0 */
#if !defined (CY_PSOC5LP)
    #error Component VDAC8_v1_90 requires cy_boot v3.0 or later
#endif /* (CY_ PSOC5LP) */


/***************************************
*       Type defines
***************************************/

/* Sleep Mode API Support */
typedef struct
{
    uint8 enableState; 
    uint8 data_value;
}VDACIn_p_backupStruct;

/* component init state */
extern uint8 VDACIn_p_initVar;


/***************************************
*        Function Prototypes 
***************************************/

void VDACIn_p_Start(void)           ;
void VDACIn_p_Stop(void)            ;
void VDACIn_p_SetSpeed(uint8 speed) ;
void VDACIn_p_SetRange(uint8 range) ;
void VDACIn_p_SetValue(uint8 value) ;
void VDACIn_p_DacTrim(void)         ;
void VDACIn_p_Init(void)            ;
void VDACIn_p_Enable(void)          ;
void VDACIn_p_SaveConfig(void)      ;
void VDACIn_p_RestoreConfig(void)   ;
void VDACIn_p_Sleep(void)           ;
void VDACIn_p_Wakeup(void)          ;


/***************************************
*            API Constants
***************************************/

/* SetRange constants */

#define VDACIn_p_RANGE_1V       0x00u
#define VDACIn_p_RANGE_4V       0x04u


/* Power setting for Start API  */
#define VDACIn_p_LOWSPEED       0x00u
#define VDACIn_p_HIGHSPEED      0x02u


/***************************************
*  Initialization Parameter Constants
***************************************/

 /* Default DAC range */
#define VDACIn_p_DEFAULT_RANGE    0u
 /* Default DAC speed */
#define VDACIn_p_DEFAULT_SPEED    0u
 /* Default Control */
#define VDACIn_p_DEFAULT_CNTL      0x00u
/* Default Strobe mode */
#define VDACIn_p_DEFAULT_STRB     0u
 /* Initial DAC value */
#define VDACIn_p_DEFAULT_DATA     100u
 /* Default Data Source */
#define VDACIn_p_DEFAULT_DATA_SRC 0u


/***************************************
*              Registers        
***************************************/
#define VDACIn_p_CR0_REG            (* (reg8 *) VDACIn_p_viDAC8__CR0 )
#define VDACIn_p_CR0_PTR            (  (reg8 *) VDACIn_p_viDAC8__CR0 )
#define VDACIn_p_CR1_REG            (* (reg8 *) VDACIn_p_viDAC8__CR1 )
#define VDACIn_p_CR1_PTR            (  (reg8 *) VDACIn_p_viDAC8__CR1 )
#define VDACIn_p_Data_REG           (* (reg8 *) VDACIn_p_viDAC8__D )
#define VDACIn_p_Data_PTR           (  (reg8 *) VDACIn_p_viDAC8__D )
#define VDACIn_p_Strobe_REG         (* (reg8 *) VDACIn_p_viDAC8__STROBE )
#define VDACIn_p_Strobe_PTR         (  (reg8 *) VDACIn_p_viDAC8__STROBE )
#define VDACIn_p_SW0_REG            (* (reg8 *) VDACIn_p_viDAC8__SW0 )
#define VDACIn_p_SW0_PTR            (  (reg8 *) VDACIn_p_viDAC8__SW0 )
#define VDACIn_p_SW2_REG            (* (reg8 *) VDACIn_p_viDAC8__SW2 )
#define VDACIn_p_SW2_PTR            (  (reg8 *) VDACIn_p_viDAC8__SW2 )
#define VDACIn_p_SW3_REG            (* (reg8 *) VDACIn_p_viDAC8__SW3 )
#define VDACIn_p_SW3_PTR            (  (reg8 *) VDACIn_p_viDAC8__SW3 )
#define VDACIn_p_SW4_REG            (* (reg8 *) VDACIn_p_viDAC8__SW4 )
#define VDACIn_p_SW4_PTR            (  (reg8 *) VDACIn_p_viDAC8__SW4 )
#define VDACIn_p_TR_REG             (* (reg8 *) VDACIn_p_viDAC8__TR )
#define VDACIn_p_TR_PTR             (  (reg8 *) VDACIn_p_viDAC8__TR )
/* Power manager */
#define VDACIn_p_PWRMGR_REG         (* (reg8 *) VDACIn_p_viDAC8__PM_ACT_CFG )
#define VDACIn_p_PWRMGR_PTR         (  (reg8 *) VDACIn_p_viDAC8__PM_ACT_CFG )
  /* Standby Power manager */
#define VDACIn_p_STBY_PWRMGR_REG    (* (reg8 *) VDACIn_p_viDAC8__PM_STBY_CFG )
#define VDACIn_p_STBY_PWRMGR_PTR    (  (reg8 *) VDACIn_p_viDAC8__PM_STBY_CFG )

/***************************************
*  Registers definitions
* for backward capability        
***************************************/
#define VDACIn_p_CR0         (* (reg8 *) VDACIn_p_viDAC8__CR0 )
#define VDACIn_p_CR1         (* (reg8 *) VDACIn_p_viDAC8__CR1 )
#define VDACIn_p_Data        (* (reg8 *) VDACIn_p_viDAC8__D )
#define VDACIn_p_Data_PTR    (  (reg8 *) VDACIn_p_viDAC8__D )
#define VDACIn_p_Strobe      (* (reg8 *) VDACIn_p_viDAC8__STROBE )
#define VDACIn_p_SW0         (* (reg8 *) VDACIn_p_viDAC8__SW0 )
#define VDACIn_p_SW2         (* (reg8 *) VDACIn_p_viDAC8__SW2 )
#define VDACIn_p_SW3         (* (reg8 *) VDACIn_p_viDAC8__SW3 )
#define VDACIn_p_SW4         (* (reg8 *) VDACIn_p_viDAC8__SW4 )
#define VDACIn_p_TR          (* (reg8 *) VDACIn_p_viDAC8__TR )
/* Power manager */
#define VDACIn_p_PWRMGR      (* (reg8 *) VDACIn_p_viDAC8__PM_ACT_CFG )
  /* Standby Power manager */
#define VDACIn_p_STBY_PWRMGR (* (reg8 *) VDACIn_p_viDAC8__PM_STBY_CFG )


/***************************************
*         Register Constants       
***************************************/

/* CR0 vDac Control Register 0 definitions */

/* Bit Field  DAC_HS_MODE                  */
#define VDACIn_p_HS_MASK        0x02u
#define VDACIn_p_HS_LOWPOWER    0x00u
#define VDACIn_p_HS_HIGHSPEED   0x02u

/* Bit Field  DAC_MODE                  */
#define VDACIn_p_MODE_MASK      0x10u
#define VDACIn_p_MODE_V         0x00u
#define VDACIn_p_MODE_I         0x10u

/* Bit Field  DAC_RANGE                  */
#define VDACIn_p_RANGE_MASK     0x0Cu
#define VDACIn_p_RANGE_0        0x00u
#define VDACIn_p_RANGE_1        0x04u

/* CR1 iDac Control Register 1 definitions */

/* Bit Field  DAC_MX_DATA                  */
#define VDACIn_p_SRC_MASK       0x20u
#define VDACIn_p_SRC_REG        0x00u
#define VDACIn_p_SRC_UDB        0x20u

/* This bit enable reset from UDB array      */
#define VDACIn_p_RESET_MASK     0x10u
#define VDACIn_p_RESET_ENABLE   0x10u
#define VDACIn_p_RESET_DISABLE  0x00u

/* This bit enables data from DAC bus      */
#define VDACIn_p_DACBUS_MASK     0x20u
#define VDACIn_p_DACBUS_ENABLE   0x20u
#define VDACIn_p_DACBUS_DISABLE  0x00u

/* DAC STROBE Strobe Control Register definitions */

/* Bit Field  DAC_MX_STROBE                  */
#define VDACIn_p_STRB_MASK     0x08u
#define VDACIn_p_STRB_EN       0x08u
#define VDACIn_p_STRB_DIS      0x00u

/* PM_ACT_CFG (Active Power Mode CFG Register)     */ 
#define VDACIn_p_ACT_PWR_EN   VDACIn_p_viDAC8__PM_ACT_MSK
  /* Standby Power enable mask */
#define VDACIn_p_STBY_PWR_EN  VDACIn_p_viDAC8__PM_STBY_MSK


/*******************************************************************************
*              Trim    
* Note - VDAC trim values are stored in the "Customer Table" area in * Row 1 of
*the Hidden Flash.  There are 8 bytes of trim data for each VDAC block.
* The values are:
*       I Gain offset, min range, Sourcing
*       I Gain offset, min range, Sinking
*       I Gain offset, med range, Sourcing
*       I Gain offset, med range, Sinking
*       I Gain offset, max range, Sourcing
*       I Gain offset, max range, Sinking
*       V Gain offset, 1V range
*       V Gain offset, 4V range
*
* The data set for the 4 VDACs are arranged using a left side/right side
* approach:
*   Left 0, Left 1, Right 0, Right 1.
* When mapped to the VDAC0 thru VDAC3 as:
*   VDAC 0, VDAC 2, VDAC 1, VDAC 3
*******************************************************************************/
#define VDACIn_p_TRIM_M7_1V_RNG_OFFSET  0x06u
#define VDACIn_p_TRIM_M8_4V_RNG_OFFSET  0x07u
/*Constatnt to set DAC in current mode and turnoff output */
#define VDACIn_p_CUR_MODE_OUT_OFF       0x1Eu 
#define VDACIn_p_DAC_TRIM_BASE          (VDACIn_p_viDAC8__TRIM__M1)

#endif /* CY_VDAC8_VDACIn_p_H  */


/* [] END OF FILE */


