/*******************************************************************************
* File Name: SUM_Amux.h  
* Version 2.20
*
* Description:
*  This file contains Pin function prototypes and register defines
*
* Note:
*
********************************************************************************
* Copyright 2008-2015, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions, 
* disclaimers, and limitations in the end user license agreement accompanying 
* the software package with which this file was provided.
*******************************************************************************/

#if !defined(CY_PINS_SUM_Amux_H) /* Pins SUM_Amux_H */
#define CY_PINS_SUM_Amux_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "SUM_Amux_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 SUM_Amux__PORT == 15 && ((SUM_Amux__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    SUM_Amux_Write(uint8 value);
void    SUM_Amux_SetDriveMode(uint8 mode);
uint8   SUM_Amux_ReadDataReg(void);
uint8   SUM_Amux_Read(void);
void    SUM_Amux_SetInterruptMode(uint16 position, uint16 mode);
uint8   SUM_Amux_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the SUM_Amux_SetDriveMode() function.
     *  @{
     */
        #define SUM_Amux_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define SUM_Amux_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define SUM_Amux_DM_RES_UP          PIN_DM_RES_UP
        #define SUM_Amux_DM_RES_DWN         PIN_DM_RES_DWN
        #define SUM_Amux_DM_OD_LO           PIN_DM_OD_LO
        #define SUM_Amux_DM_OD_HI           PIN_DM_OD_HI
        #define SUM_Amux_DM_STRONG          PIN_DM_STRONG
        #define SUM_Amux_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define SUM_Amux_MASK               SUM_Amux__MASK
#define SUM_Amux_SHIFT              SUM_Amux__SHIFT
#define SUM_Amux_WIDTH              1u

/* Interrupt constants */
#if defined(SUM_Amux__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in SUM_Amux_SetInterruptMode() function.
     *  @{
     */
        #define SUM_Amux_INTR_NONE      (uint16)(0x0000u)
        #define SUM_Amux_INTR_RISING    (uint16)(0x0001u)
        #define SUM_Amux_INTR_FALLING   (uint16)(0x0002u)
        #define SUM_Amux_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define SUM_Amux_INTR_MASK      (0x01u) 
#endif /* (SUM_Amux__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define SUM_Amux_PS                     (* (reg8 *) SUM_Amux__PS)
/* Data Register */
#define SUM_Amux_DR                     (* (reg8 *) SUM_Amux__DR)
/* Port Number */
#define SUM_Amux_PRT_NUM                (* (reg8 *) SUM_Amux__PRT) 
/* Connect to Analog Globals */                                                  
#define SUM_Amux_AG                     (* (reg8 *) SUM_Amux__AG)                       
/* Analog MUX bux enable */
#define SUM_Amux_AMUX                   (* (reg8 *) SUM_Amux__AMUX) 
/* Bidirectional Enable */                                                        
#define SUM_Amux_BIE                    (* (reg8 *) SUM_Amux__BIE)
/* Bit-mask for Aliased Register Access */
#define SUM_Amux_BIT_MASK               (* (reg8 *) SUM_Amux__BIT_MASK)
/* Bypass Enable */
#define SUM_Amux_BYP                    (* (reg8 *) SUM_Amux__BYP)
/* Port wide control signals */                                                   
#define SUM_Amux_CTL                    (* (reg8 *) SUM_Amux__CTL)
/* Drive Modes */
#define SUM_Amux_DM0                    (* (reg8 *) SUM_Amux__DM0) 
#define SUM_Amux_DM1                    (* (reg8 *) SUM_Amux__DM1)
#define SUM_Amux_DM2                    (* (reg8 *) SUM_Amux__DM2) 
/* Input Buffer Disable Override */
#define SUM_Amux_INP_DIS                (* (reg8 *) SUM_Amux__INP_DIS)
/* LCD Common or Segment Drive */
#define SUM_Amux_LCD_COM_SEG            (* (reg8 *) SUM_Amux__LCD_COM_SEG)
/* Enable Segment LCD */
#define SUM_Amux_LCD_EN                 (* (reg8 *) SUM_Amux__LCD_EN)
/* Slew Rate Control */
#define SUM_Amux_SLW                    (* (reg8 *) SUM_Amux__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define SUM_Amux_PRTDSI__CAPS_SEL       (* (reg8 *) SUM_Amux__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define SUM_Amux_PRTDSI__DBL_SYNC_IN    (* (reg8 *) SUM_Amux__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define SUM_Amux_PRTDSI__OE_SEL0        (* (reg8 *) SUM_Amux__PRTDSI__OE_SEL0) 
#define SUM_Amux_PRTDSI__OE_SEL1        (* (reg8 *) SUM_Amux__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define SUM_Amux_PRTDSI__OUT_SEL0       (* (reg8 *) SUM_Amux__PRTDSI__OUT_SEL0) 
#define SUM_Amux_PRTDSI__OUT_SEL1       (* (reg8 *) SUM_Amux__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define SUM_Amux_PRTDSI__SYNC_OUT       (* (reg8 *) SUM_Amux__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(SUM_Amux__SIO_CFG)
    #define SUM_Amux_SIO_HYST_EN        (* (reg8 *) SUM_Amux__SIO_HYST_EN)
    #define SUM_Amux_SIO_REG_HIFREQ     (* (reg8 *) SUM_Amux__SIO_REG_HIFREQ)
    #define SUM_Amux_SIO_CFG            (* (reg8 *) SUM_Amux__SIO_CFG)
    #define SUM_Amux_SIO_DIFF           (* (reg8 *) SUM_Amux__SIO_DIFF)
#endif /* (SUM_Amux__SIO_CFG) */

/* Interrupt Registers */
#if defined(SUM_Amux__INTSTAT)
    #define SUM_Amux_INTSTAT            (* (reg8 *) SUM_Amux__INTSTAT)
    #define SUM_Amux_SNAP               (* (reg8 *) SUM_Amux__SNAP)
    
	#define SUM_Amux_0_INTTYPE_REG 		(* (reg8 *) SUM_Amux__0__INTTYPE)
#endif /* (SUM_Amux__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_SUM_Amux_H */


/* [] END OF FILE */
