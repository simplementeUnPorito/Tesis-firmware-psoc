/*******************************************************************************
* File Name: Adderout.h  
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

#if !defined(CY_PINS_Adderout_H) /* Pins Adderout_H */
#define CY_PINS_Adderout_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "Adderout_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 Adderout__PORT == 15 && ((Adderout__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    Adderout_Write(uint8 value);
void    Adderout_SetDriveMode(uint8 mode);
uint8   Adderout_ReadDataReg(void);
uint8   Adderout_Read(void);
void    Adderout_SetInterruptMode(uint16 position, uint16 mode);
uint8   Adderout_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the Adderout_SetDriveMode() function.
     *  @{
     */
        #define Adderout_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define Adderout_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define Adderout_DM_RES_UP          PIN_DM_RES_UP
        #define Adderout_DM_RES_DWN         PIN_DM_RES_DWN
        #define Adderout_DM_OD_LO           PIN_DM_OD_LO
        #define Adderout_DM_OD_HI           PIN_DM_OD_HI
        #define Adderout_DM_STRONG          PIN_DM_STRONG
        #define Adderout_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define Adderout_MASK               Adderout__MASK
#define Adderout_SHIFT              Adderout__SHIFT
#define Adderout_WIDTH              1u

/* Interrupt constants */
#if defined(Adderout__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in Adderout_SetInterruptMode() function.
     *  @{
     */
        #define Adderout_INTR_NONE      (uint16)(0x0000u)
        #define Adderout_INTR_RISING    (uint16)(0x0001u)
        #define Adderout_INTR_FALLING   (uint16)(0x0002u)
        #define Adderout_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define Adderout_INTR_MASK      (0x01u) 
#endif /* (Adderout__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define Adderout_PS                     (* (reg8 *) Adderout__PS)
/* Data Register */
#define Adderout_DR                     (* (reg8 *) Adderout__DR)
/* Port Number */
#define Adderout_PRT_NUM                (* (reg8 *) Adderout__PRT) 
/* Connect to Analog Globals */                                                  
#define Adderout_AG                     (* (reg8 *) Adderout__AG)                       
/* Analog MUX bux enable */
#define Adderout_AMUX                   (* (reg8 *) Adderout__AMUX) 
/* Bidirectional Enable */                                                        
#define Adderout_BIE                    (* (reg8 *) Adderout__BIE)
/* Bit-mask for Aliased Register Access */
#define Adderout_BIT_MASK               (* (reg8 *) Adderout__BIT_MASK)
/* Bypass Enable */
#define Adderout_BYP                    (* (reg8 *) Adderout__BYP)
/* Port wide control signals */                                                   
#define Adderout_CTL                    (* (reg8 *) Adderout__CTL)
/* Drive Modes */
#define Adderout_DM0                    (* (reg8 *) Adderout__DM0) 
#define Adderout_DM1                    (* (reg8 *) Adderout__DM1)
#define Adderout_DM2                    (* (reg8 *) Adderout__DM2) 
/* Input Buffer Disable Override */
#define Adderout_INP_DIS                (* (reg8 *) Adderout__INP_DIS)
/* LCD Common or Segment Drive */
#define Adderout_LCD_COM_SEG            (* (reg8 *) Adderout__LCD_COM_SEG)
/* Enable Segment LCD */
#define Adderout_LCD_EN                 (* (reg8 *) Adderout__LCD_EN)
/* Slew Rate Control */
#define Adderout_SLW                    (* (reg8 *) Adderout__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define Adderout_PRTDSI__CAPS_SEL       (* (reg8 *) Adderout__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define Adderout_PRTDSI__DBL_SYNC_IN    (* (reg8 *) Adderout__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define Adderout_PRTDSI__OE_SEL0        (* (reg8 *) Adderout__PRTDSI__OE_SEL0) 
#define Adderout_PRTDSI__OE_SEL1        (* (reg8 *) Adderout__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define Adderout_PRTDSI__OUT_SEL0       (* (reg8 *) Adderout__PRTDSI__OUT_SEL0) 
#define Adderout_PRTDSI__OUT_SEL1       (* (reg8 *) Adderout__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define Adderout_PRTDSI__SYNC_OUT       (* (reg8 *) Adderout__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(Adderout__SIO_CFG)
    #define Adderout_SIO_HYST_EN        (* (reg8 *) Adderout__SIO_HYST_EN)
    #define Adderout_SIO_REG_HIFREQ     (* (reg8 *) Adderout__SIO_REG_HIFREQ)
    #define Adderout_SIO_CFG            (* (reg8 *) Adderout__SIO_CFG)
    #define Adderout_SIO_DIFF           (* (reg8 *) Adderout__SIO_DIFF)
#endif /* (Adderout__SIO_CFG) */

/* Interrupt Registers */
#if defined(Adderout__INTSTAT)
    #define Adderout_INTSTAT            (* (reg8 *) Adderout__INTSTAT)
    #define Adderout_SNAP               (* (reg8 *) Adderout__SNAP)
    
	#define Adderout_0_INTTYPE_REG 		(* (reg8 *) Adderout__0__INTTYPE)
#endif /* (Adderout__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_Adderout_H */


/* [] END OF FILE */
