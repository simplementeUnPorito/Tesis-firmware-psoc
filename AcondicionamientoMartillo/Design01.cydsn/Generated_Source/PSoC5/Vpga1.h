/*******************************************************************************
* File Name: Vpga1.h  
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

#if !defined(CY_PINS_Vpga1_H) /* Pins Vpga1_H */
#define CY_PINS_Vpga1_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "Vpga1_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 Vpga1__PORT == 15 && ((Vpga1__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    Vpga1_Write(uint8 value);
void    Vpga1_SetDriveMode(uint8 mode);
uint8   Vpga1_ReadDataReg(void);
uint8   Vpga1_Read(void);
void    Vpga1_SetInterruptMode(uint16 position, uint16 mode);
uint8   Vpga1_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the Vpga1_SetDriveMode() function.
     *  @{
     */
        #define Vpga1_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define Vpga1_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define Vpga1_DM_RES_UP          PIN_DM_RES_UP
        #define Vpga1_DM_RES_DWN         PIN_DM_RES_DWN
        #define Vpga1_DM_OD_LO           PIN_DM_OD_LO
        #define Vpga1_DM_OD_HI           PIN_DM_OD_HI
        #define Vpga1_DM_STRONG          PIN_DM_STRONG
        #define Vpga1_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define Vpga1_MASK               Vpga1__MASK
#define Vpga1_SHIFT              Vpga1__SHIFT
#define Vpga1_WIDTH              1u

/* Interrupt constants */
#if defined(Vpga1__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in Vpga1_SetInterruptMode() function.
     *  @{
     */
        #define Vpga1_INTR_NONE      (uint16)(0x0000u)
        #define Vpga1_INTR_RISING    (uint16)(0x0001u)
        #define Vpga1_INTR_FALLING   (uint16)(0x0002u)
        #define Vpga1_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define Vpga1_INTR_MASK      (0x01u) 
#endif /* (Vpga1__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define Vpga1_PS                     (* (reg8 *) Vpga1__PS)
/* Data Register */
#define Vpga1_DR                     (* (reg8 *) Vpga1__DR)
/* Port Number */
#define Vpga1_PRT_NUM                (* (reg8 *) Vpga1__PRT) 
/* Connect to Analog Globals */                                                  
#define Vpga1_AG                     (* (reg8 *) Vpga1__AG)                       
/* Analog MUX bux enable */
#define Vpga1_AMUX                   (* (reg8 *) Vpga1__AMUX) 
/* Bidirectional Enable */                                                        
#define Vpga1_BIE                    (* (reg8 *) Vpga1__BIE)
/* Bit-mask for Aliased Register Access */
#define Vpga1_BIT_MASK               (* (reg8 *) Vpga1__BIT_MASK)
/* Bypass Enable */
#define Vpga1_BYP                    (* (reg8 *) Vpga1__BYP)
/* Port wide control signals */                                                   
#define Vpga1_CTL                    (* (reg8 *) Vpga1__CTL)
/* Drive Modes */
#define Vpga1_DM0                    (* (reg8 *) Vpga1__DM0) 
#define Vpga1_DM1                    (* (reg8 *) Vpga1__DM1)
#define Vpga1_DM2                    (* (reg8 *) Vpga1__DM2) 
/* Input Buffer Disable Override */
#define Vpga1_INP_DIS                (* (reg8 *) Vpga1__INP_DIS)
/* LCD Common or Segment Drive */
#define Vpga1_LCD_COM_SEG            (* (reg8 *) Vpga1__LCD_COM_SEG)
/* Enable Segment LCD */
#define Vpga1_LCD_EN                 (* (reg8 *) Vpga1__LCD_EN)
/* Slew Rate Control */
#define Vpga1_SLW                    (* (reg8 *) Vpga1__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define Vpga1_PRTDSI__CAPS_SEL       (* (reg8 *) Vpga1__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define Vpga1_PRTDSI__DBL_SYNC_IN    (* (reg8 *) Vpga1__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define Vpga1_PRTDSI__OE_SEL0        (* (reg8 *) Vpga1__PRTDSI__OE_SEL0) 
#define Vpga1_PRTDSI__OE_SEL1        (* (reg8 *) Vpga1__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define Vpga1_PRTDSI__OUT_SEL0       (* (reg8 *) Vpga1__PRTDSI__OUT_SEL0) 
#define Vpga1_PRTDSI__OUT_SEL1       (* (reg8 *) Vpga1__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define Vpga1_PRTDSI__SYNC_OUT       (* (reg8 *) Vpga1__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(Vpga1__SIO_CFG)
    #define Vpga1_SIO_HYST_EN        (* (reg8 *) Vpga1__SIO_HYST_EN)
    #define Vpga1_SIO_REG_HIFREQ     (* (reg8 *) Vpga1__SIO_REG_HIFREQ)
    #define Vpga1_SIO_CFG            (* (reg8 *) Vpga1__SIO_CFG)
    #define Vpga1_SIO_DIFF           (* (reg8 *) Vpga1__SIO_DIFF)
#endif /* (Vpga1__SIO_CFG) */

/* Interrupt Registers */
#if defined(Vpga1__INTSTAT)
    #define Vpga1_INTSTAT            (* (reg8 *) Vpga1__INTSTAT)
    #define Vpga1_SNAP               (* (reg8 *) Vpga1__SNAP)
    
	#define Vpga1_0_INTTYPE_REG 		(* (reg8 *) Vpga1__0__INTTYPE)
#endif /* (Vpga1__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_Vpga1_H */


/* [] END OF FILE */
