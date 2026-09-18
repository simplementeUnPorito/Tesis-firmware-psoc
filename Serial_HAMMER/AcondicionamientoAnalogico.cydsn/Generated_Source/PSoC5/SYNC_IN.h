/*******************************************************************************
* File Name: SYNC_IN.h  
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

#if !defined(CY_PINS_SYNC_IN_H) /* Pins SYNC_IN_H */
#define CY_PINS_SYNC_IN_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "SYNC_IN_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 SYNC_IN__PORT == 15 && ((SYNC_IN__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    SYNC_IN_Write(uint8 value);
void    SYNC_IN_SetDriveMode(uint8 mode);
uint8   SYNC_IN_ReadDataReg(void);
uint8   SYNC_IN_Read(void);
void    SYNC_IN_SetInterruptMode(uint16 position, uint16 mode);
uint8   SYNC_IN_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the SYNC_IN_SetDriveMode() function.
     *  @{
     */
        #define SYNC_IN_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define SYNC_IN_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define SYNC_IN_DM_RES_UP          PIN_DM_RES_UP
        #define SYNC_IN_DM_RES_DWN         PIN_DM_RES_DWN
        #define SYNC_IN_DM_OD_LO           PIN_DM_OD_LO
        #define SYNC_IN_DM_OD_HI           PIN_DM_OD_HI
        #define SYNC_IN_DM_STRONG          PIN_DM_STRONG
        #define SYNC_IN_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define SYNC_IN_MASK               SYNC_IN__MASK
#define SYNC_IN_SHIFT              SYNC_IN__SHIFT
#define SYNC_IN_WIDTH              1u

/* Interrupt constants */
#if defined(SYNC_IN__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in SYNC_IN_SetInterruptMode() function.
     *  @{
     */
        #define SYNC_IN_INTR_NONE      (uint16)(0x0000u)
        #define SYNC_IN_INTR_RISING    (uint16)(0x0001u)
        #define SYNC_IN_INTR_FALLING   (uint16)(0x0002u)
        #define SYNC_IN_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define SYNC_IN_INTR_MASK      (0x01u) 
#endif /* (SYNC_IN__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define SYNC_IN_PS                     (* (reg8 *) SYNC_IN__PS)
/* Data Register */
#define SYNC_IN_DR                     (* (reg8 *) SYNC_IN__DR)
/* Port Number */
#define SYNC_IN_PRT_NUM                (* (reg8 *) SYNC_IN__PRT) 
/* Connect to Analog Globals */                                                  
#define SYNC_IN_AG                     (* (reg8 *) SYNC_IN__AG)                       
/* Analog MUX bux enable */
#define SYNC_IN_AMUX                   (* (reg8 *) SYNC_IN__AMUX) 
/* Bidirectional Enable */                                                        
#define SYNC_IN_BIE                    (* (reg8 *) SYNC_IN__BIE)
/* Bit-mask for Aliased Register Access */
#define SYNC_IN_BIT_MASK               (* (reg8 *) SYNC_IN__BIT_MASK)
/* Bypass Enable */
#define SYNC_IN_BYP                    (* (reg8 *) SYNC_IN__BYP)
/* Port wide control signals */                                                   
#define SYNC_IN_CTL                    (* (reg8 *) SYNC_IN__CTL)
/* Drive Modes */
#define SYNC_IN_DM0                    (* (reg8 *) SYNC_IN__DM0) 
#define SYNC_IN_DM1                    (* (reg8 *) SYNC_IN__DM1)
#define SYNC_IN_DM2                    (* (reg8 *) SYNC_IN__DM2) 
/* Input Buffer Disable Override */
#define SYNC_IN_INP_DIS                (* (reg8 *) SYNC_IN__INP_DIS)
/* LCD Common or Segment Drive */
#define SYNC_IN_LCD_COM_SEG            (* (reg8 *) SYNC_IN__LCD_COM_SEG)
/* Enable Segment LCD */
#define SYNC_IN_LCD_EN                 (* (reg8 *) SYNC_IN__LCD_EN)
/* Slew Rate Control */
#define SYNC_IN_SLW                    (* (reg8 *) SYNC_IN__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define SYNC_IN_PRTDSI__CAPS_SEL       (* (reg8 *) SYNC_IN__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define SYNC_IN_PRTDSI__DBL_SYNC_IN    (* (reg8 *) SYNC_IN__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define SYNC_IN_PRTDSI__OE_SEL0        (* (reg8 *) SYNC_IN__PRTDSI__OE_SEL0) 
#define SYNC_IN_PRTDSI__OE_SEL1        (* (reg8 *) SYNC_IN__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define SYNC_IN_PRTDSI__OUT_SEL0       (* (reg8 *) SYNC_IN__PRTDSI__OUT_SEL0) 
#define SYNC_IN_PRTDSI__OUT_SEL1       (* (reg8 *) SYNC_IN__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define SYNC_IN_PRTDSI__SYNC_OUT       (* (reg8 *) SYNC_IN__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(SYNC_IN__SIO_CFG)
    #define SYNC_IN_SIO_HYST_EN        (* (reg8 *) SYNC_IN__SIO_HYST_EN)
    #define SYNC_IN_SIO_REG_HIFREQ     (* (reg8 *) SYNC_IN__SIO_REG_HIFREQ)
    #define SYNC_IN_SIO_CFG            (* (reg8 *) SYNC_IN__SIO_CFG)
    #define SYNC_IN_SIO_DIFF           (* (reg8 *) SYNC_IN__SIO_DIFF)
#endif /* (SYNC_IN__SIO_CFG) */

/* Interrupt Registers */
#if defined(SYNC_IN__INTSTAT)
    #define SYNC_IN_INTSTAT            (* (reg8 *) SYNC_IN__INTSTAT)
    #define SYNC_IN_SNAP               (* (reg8 *) SYNC_IN__SNAP)
    
	#define SYNC_IN_0_INTTYPE_REG 		(* (reg8 *) SYNC_IN__0__INTTYPE)
#endif /* (SYNC_IN__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_SYNC_IN_H */


/* [] END OF FILE */
