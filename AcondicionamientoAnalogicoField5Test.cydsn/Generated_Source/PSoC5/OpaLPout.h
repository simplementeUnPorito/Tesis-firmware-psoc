/*******************************************************************************
* File Name: OpaLPout.h  
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

#if !defined(CY_PINS_OpaLPout_H) /* Pins OpaLPout_H */
#define CY_PINS_OpaLPout_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "OpaLPout_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 OpaLPout__PORT == 15 && ((OpaLPout__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    OpaLPout_Write(uint8 value);
void    OpaLPout_SetDriveMode(uint8 mode);
uint8   OpaLPout_ReadDataReg(void);
uint8   OpaLPout_Read(void);
void    OpaLPout_SetInterruptMode(uint16 position, uint16 mode);
uint8   OpaLPout_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the OpaLPout_SetDriveMode() function.
     *  @{
     */
        #define OpaLPout_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define OpaLPout_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define OpaLPout_DM_RES_UP          PIN_DM_RES_UP
        #define OpaLPout_DM_RES_DWN         PIN_DM_RES_DWN
        #define OpaLPout_DM_OD_LO           PIN_DM_OD_LO
        #define OpaLPout_DM_OD_HI           PIN_DM_OD_HI
        #define OpaLPout_DM_STRONG          PIN_DM_STRONG
        #define OpaLPout_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define OpaLPout_MASK               OpaLPout__MASK
#define OpaLPout_SHIFT              OpaLPout__SHIFT
#define OpaLPout_WIDTH              1u

/* Interrupt constants */
#if defined(OpaLPout__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in OpaLPout_SetInterruptMode() function.
     *  @{
     */
        #define OpaLPout_INTR_NONE      (uint16)(0x0000u)
        #define OpaLPout_INTR_RISING    (uint16)(0x0001u)
        #define OpaLPout_INTR_FALLING   (uint16)(0x0002u)
        #define OpaLPout_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define OpaLPout_INTR_MASK      (0x01u) 
#endif /* (OpaLPout__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define OpaLPout_PS                     (* (reg8 *) OpaLPout__PS)
/* Data Register */
#define OpaLPout_DR                     (* (reg8 *) OpaLPout__DR)
/* Port Number */
#define OpaLPout_PRT_NUM                (* (reg8 *) OpaLPout__PRT) 
/* Connect to Analog Globals */                                                  
#define OpaLPout_AG                     (* (reg8 *) OpaLPout__AG)                       
/* Analog MUX bux enable */
#define OpaLPout_AMUX                   (* (reg8 *) OpaLPout__AMUX) 
/* Bidirectional Enable */                                                        
#define OpaLPout_BIE                    (* (reg8 *) OpaLPout__BIE)
/* Bit-mask for Aliased Register Access */
#define OpaLPout_BIT_MASK               (* (reg8 *) OpaLPout__BIT_MASK)
/* Bypass Enable */
#define OpaLPout_BYP                    (* (reg8 *) OpaLPout__BYP)
/* Port wide control signals */                                                   
#define OpaLPout_CTL                    (* (reg8 *) OpaLPout__CTL)
/* Drive Modes */
#define OpaLPout_DM0                    (* (reg8 *) OpaLPout__DM0) 
#define OpaLPout_DM1                    (* (reg8 *) OpaLPout__DM1)
#define OpaLPout_DM2                    (* (reg8 *) OpaLPout__DM2) 
/* Input Buffer Disable Override */
#define OpaLPout_INP_DIS                (* (reg8 *) OpaLPout__INP_DIS)
/* LCD Common or Segment Drive */
#define OpaLPout_LCD_COM_SEG            (* (reg8 *) OpaLPout__LCD_COM_SEG)
/* Enable Segment LCD */
#define OpaLPout_LCD_EN                 (* (reg8 *) OpaLPout__LCD_EN)
/* Slew Rate Control */
#define OpaLPout_SLW                    (* (reg8 *) OpaLPout__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define OpaLPout_PRTDSI__CAPS_SEL       (* (reg8 *) OpaLPout__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define OpaLPout_PRTDSI__DBL_SYNC_IN    (* (reg8 *) OpaLPout__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define OpaLPout_PRTDSI__OE_SEL0        (* (reg8 *) OpaLPout__PRTDSI__OE_SEL0) 
#define OpaLPout_PRTDSI__OE_SEL1        (* (reg8 *) OpaLPout__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define OpaLPout_PRTDSI__OUT_SEL0       (* (reg8 *) OpaLPout__PRTDSI__OUT_SEL0) 
#define OpaLPout_PRTDSI__OUT_SEL1       (* (reg8 *) OpaLPout__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define OpaLPout_PRTDSI__SYNC_OUT       (* (reg8 *) OpaLPout__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(OpaLPout__SIO_CFG)
    #define OpaLPout_SIO_HYST_EN        (* (reg8 *) OpaLPout__SIO_HYST_EN)
    #define OpaLPout_SIO_REG_HIFREQ     (* (reg8 *) OpaLPout__SIO_REG_HIFREQ)
    #define OpaLPout_SIO_CFG            (* (reg8 *) OpaLPout__SIO_CFG)
    #define OpaLPout_SIO_DIFF           (* (reg8 *) OpaLPout__SIO_DIFF)
#endif /* (OpaLPout__SIO_CFG) */

/* Interrupt Registers */
#if defined(OpaLPout__INTSTAT)
    #define OpaLPout_INTSTAT            (* (reg8 *) OpaLPout__INTSTAT)
    #define OpaLPout_SNAP               (* (reg8 *) OpaLPout__SNAP)
    
	#define OpaLPout_0_INTTYPE_REG 		(* (reg8 *) OpaLPout__0__INTTYPE)
#endif /* (OpaLPout__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_OpaLPout_H */


/* [] END OF FILE */
