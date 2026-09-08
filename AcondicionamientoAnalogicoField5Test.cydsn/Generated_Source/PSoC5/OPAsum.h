/*******************************************************************************
* File Name: OPAsum.h  
* Version 1.90
*
* Description:
*  This file contains the function prototypes and constants used in
*  the OpAmp (Analog Buffer) Component.
*
* Note:
*
********************************************************************************
* Copyright 2008-2012, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions, 
* disclaimers, and limitations in the end user license agreement accompanying 
* the software package with which this file was provided.
*******************************************************************************/


#if !defined(CY_OPAMP_OPAsum_H) 
#define CY_OPAMP_OPAsum_H 

#include "cyfitter.h"
#include "cytypes.h"
#include "CyLib.h"

/* Check to see if required defines such as CY_PSOC5LP are available */
/* They are defined starting with cy_boot v3.0 */
#if !defined (CY_PSOC5LP)
    #error Component OpAmp_v1_90 requires cy_boot v3.0 or later
#endif /* (CY_ PSOC5LP) */


/***************************************
*   Data Struct Definition
***************************************/

/* Low power Mode API Support */
typedef struct
{
    uint8   enableState;
}   OPAsum_BACKUP_STRUCT;

/* Variable describes initial state of the component */
extern uint8 OPAsum_initVar;


/**************************************
*        Function Prototypes 
**************************************/

void OPAsum_Start(void)               ;
void OPAsum_Stop(void)                ;
void OPAsum_SetPower(uint8 power)     ;
void OPAsum_Sleep(void)               ;
void OPAsum_Wakeup(void)              ;
void OPAsum_SaveConfig(void)          ;
void OPAsum_RestoreConfig(void)       ;
void OPAsum_Init(void)                ;
void OPAsum_Enable(void)              ;


/**************************************
*           API Constants
**************************************/

/* Power constants for SetPower() function */
#define OPAsum_LPOCPOWER              (0x00u)
#define OPAsum_LOWPOWER               (0x01u)
#define OPAsum_MEDPOWER               (0x02u)
#define OPAsum_HIGHPOWER              (0x03u)


/**************************************
*           Parameter Defaults        
**************************************/
#define OPAsum_DEFAULT_POWER          (3u)
#define OPAsum_DEFAULT_MODE           (0u)


/**************************************
*             Registers
**************************************/

#define OPAsum_CR_REG                 (* (reg8 *) OPAsum_ABuf__CR)
#define OPAsum_CR_PTR                 (  (reg8 *) OPAsum_ABuf__CR)

#define OPAsum_MX_REG                 (* (reg8 *) OPAsum_ABuf__MX)
#define OPAsum_MX_PTR                 (  (reg8 *) OPAsum_ABuf__MX)

#define OPAsum_SW_REG                 (* (reg8 *) OPAsum_ABuf__SW)
#define OPAsum_SW_PTR                 (  (reg8 *) OPAsum_ABuf__SW)

/* Active mode power manager register */
#define OPAsum_PM_ACT_CFG_REG         (* (reg8 *) OPAsum_ABuf__PM_ACT_CFG)
#define OPAsum_PM_ACT_CFG_PTR         (  (reg8 *) OPAsum_ABuf__PM_ACT_CFG)

/* Alternative mode power manager register */
#define OPAsum_PM_STBY_CFG_REG        (* (reg8 *) OPAsum_ABuf__PM_STBY_CFG)
#define OPAsum_PM_STBY_CFG_PTR        (  (reg8 *) OPAsum_ABuf__PM_STBY_CFG)

/* ANIF.PUMP.CR1 Pump Configuration Register 1 */
#define OPAsum_PUMP_CR1_REG           (* (reg8 *) CYDEV_ANAIF_CFG_PUMP_CR1)
#define OPAsum_PUMP_CR1_PTR           (  (reg8 *) CYDEV_ANAIF_CFG_PUMP_CR1)

/* Trim register defines */
#define OPAsum_TR0_REG                (* (reg8 *) OPAsum_ABuf__TR0)
#define OPAsum_TR0_PTR                (  (reg8 *) OPAsum_ABuf__TR0)

#define OPAsum_TR1_REG                (* (reg8 *) OPAsum_ABuf__TR1)
#define OPAsum_TR1_PTR                (  (reg8 *) OPAsum_ABuf__TR1)


/**************************************
*       Register Constants
**************************************/

/* CX Analog Buffer Input Selection Register */

/* Power mode defines */

/* PM_ACT_CFG (Active Power Mode CFG Register) mask */ 
#define OPAsum_ACT_PWR_EN             OPAsum_ABuf__PM_ACT_MSK 
/* PM_STBY_CFG (Alternative Active Power Mode CFG Register) mask */ 
#define OPAsum_STBY_PWR_EN            OPAsum_ABuf__PM_STBY_MSK 
/* Power mask */
#define OPAsum_PWR_MASK               (0x03u)

/* MX Analog Buffer Input Selection Register */

/* Bit Field  MX_VN */
#define OPAsum_MX_VN_MASK             (0x30u)
#define OPAsum_MX_VN_NC               (0x00u)
#define OPAsum_MX_VN_AG4              (0x10u)
#define OPAsum_MX_VN_AG6              (0x20u)

/* Bit Field  MX_VP */
#define OPAsum_MX_VP_MASK             (0x0Fu)
#define OPAsum_MX_VP_NC               (0x00u)
#define OPAsum_MX_VP_VREF             (0x09u)
#define OPAsum_MX_VP_AG4              (0x01u)
#define OPAsum_MX_VP_AG5              (0x02u)
#define OPAsum_MX_VP_AG6              (0x03u)
#define OPAsum_MX_VP_AG7              (0x04u)
#define OPAsum_MX_VP_ABUS0            (0x05u)
#define OPAsum_MX_VP_ABUS1            (0x06u)
#define OPAsum_MX_VP_ABUS2            (0x07u)
#define OPAsum_MX_VP_ABUS3            (0x08u)

/* SW Analog Buffer Routing Switch Reg */

/* Bit Field  SW */
#define OPAsum_SW_MASK                (0x07u) 
#define OPAsum_SW_SWINP               (0x04u)  /* Enable positive input */
#define OPAsum_SW_SWINN               (0x02u)  /* Enable negative input */

/* Pump configuration register masks */
#define OPAsum_PUMP_CR1_CLKSEL        (0x40u)
#define OPAsum_PUMP_CR1_FORCE         (0x20u)
#define OPAsum_PUMP_CR1_AUTO          (0x10u)

#endif /* CY_OPAMP_OPAsum_H */


/* [] END OF FILE */
