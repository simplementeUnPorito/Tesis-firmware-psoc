/*******************************************************************************
* File Name: PGAin_Amux.h  
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

#if !defined(CY_PINS_PGAin_Amux_H) /* Pins PGAin_Amux_H */
#define CY_PINS_PGAin_Amux_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "PGAin_Amux_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 PGAin_Amux__PORT == 15 && ((PGAin_Amux__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    PGAin_Amux_Write(uint8 value);
void    PGAin_Amux_SetDriveMode(uint8 mode);
uint8   PGAin_Amux_ReadDataReg(void);
uint8   PGAin_Amux_Read(void);
void    PGAin_Amux_SetInterruptMode(uint16 position, uint16 mode);
uint8   PGAin_Amux_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the PGAin_Amux_SetDriveMode() function.
     *  @{
     */
        #define PGAin_Amux_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define PGAin_Amux_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define PGAin_Amux_DM_RES_UP          PIN_DM_RES_UP
        #define PGAin_Amux_DM_RES_DWN         PIN_DM_RES_DWN
        #define PGAin_Amux_DM_OD_LO           PIN_DM_OD_LO
        #define PGAin_Amux_DM_OD_HI           PIN_DM_OD_HI
        #define PGAin_Amux_DM_STRONG          PIN_DM_STRONG
        #define PGAin_Amux_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define PGAin_Amux_MASK               PGAin_Amux__MASK
#define PGAin_Amux_SHIFT              PGAin_Amux__SHIFT
#define PGAin_Amux_WIDTH              1u

/* Interrupt constants */
#if defined(PGAin_Amux__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in PGAin_Amux_SetInterruptMode() function.
     *  @{
     */
        #define PGAin_Amux_INTR_NONE      (uint16)(0x0000u)
        #define PGAin_Amux_INTR_RISING    (uint16)(0x0001u)
        #define PGAin_Amux_INTR_FALLING   (uint16)(0x0002u)
        #define PGAin_Amux_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define PGAin_Amux_INTR_MASK      (0x01u) 
#endif /* (PGAin_Amux__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define PGAin_Amux_PS                     (* (reg8 *) PGAin_Amux__PS)
/* Data Register */
#define PGAin_Amux_DR                     (* (reg8 *) PGAin_Amux__DR)
/* Port Number */
#define PGAin_Amux_PRT_NUM                (* (reg8 *) PGAin_Amux__PRT) 
/* Connect to Analog Globals */                                                  
#define PGAin_Amux_AG                     (* (reg8 *) PGAin_Amux__AG)                       
/* Analog MUX bux enable */
#define PGAin_Amux_AMUX                   (* (reg8 *) PGAin_Amux__AMUX) 
/* Bidirectional Enable */                                                        
#define PGAin_Amux_BIE                    (* (reg8 *) PGAin_Amux__BIE)
/* Bit-mask for Aliased Register Access */
#define PGAin_Amux_BIT_MASK               (* (reg8 *) PGAin_Amux__BIT_MASK)
/* Bypass Enable */
#define PGAin_Amux_BYP                    (* (reg8 *) PGAin_Amux__BYP)
/* Port wide control signals */                                                   
#define PGAin_Amux_CTL                    (* (reg8 *) PGAin_Amux__CTL)
/* Drive Modes */
#define PGAin_Amux_DM0                    (* (reg8 *) PGAin_Amux__DM0) 
#define PGAin_Amux_DM1                    (* (reg8 *) PGAin_Amux__DM1)
#define PGAin_Amux_DM2                    (* (reg8 *) PGAin_Amux__DM2) 
/* Input Buffer Disable Override */
#define PGAin_Amux_INP_DIS                (* (reg8 *) PGAin_Amux__INP_DIS)
/* LCD Common or Segment Drive */
#define PGAin_Amux_LCD_COM_SEG            (* (reg8 *) PGAin_Amux__LCD_COM_SEG)
/* Enable Segment LCD */
#define PGAin_Amux_LCD_EN                 (* (reg8 *) PGAin_Amux__LCD_EN)
/* Slew Rate Control */
#define PGAin_Amux_SLW                    (* (reg8 *) PGAin_Amux__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define PGAin_Amux_PRTDSI__CAPS_SEL       (* (reg8 *) PGAin_Amux__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define PGAin_Amux_PRTDSI__DBL_SYNC_IN    (* (reg8 *) PGAin_Amux__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define PGAin_Amux_PRTDSI__OE_SEL0        (* (reg8 *) PGAin_Amux__PRTDSI__OE_SEL0) 
#define PGAin_Amux_PRTDSI__OE_SEL1        (* (reg8 *) PGAin_Amux__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define PGAin_Amux_PRTDSI__OUT_SEL0       (* (reg8 *) PGAin_Amux__PRTDSI__OUT_SEL0) 
#define PGAin_Amux_PRTDSI__OUT_SEL1       (* (reg8 *) PGAin_Amux__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define PGAin_Amux_PRTDSI__SYNC_OUT       (* (reg8 *) PGAin_Amux__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(PGAin_Amux__SIO_CFG)
    #define PGAin_Amux_SIO_HYST_EN        (* (reg8 *) PGAin_Amux__SIO_HYST_EN)
    #define PGAin_Amux_SIO_REG_HIFREQ     (* (reg8 *) PGAin_Amux__SIO_REG_HIFREQ)
    #define PGAin_Amux_SIO_CFG            (* (reg8 *) PGAin_Amux__SIO_CFG)
    #define PGAin_Amux_SIO_DIFF           (* (reg8 *) PGAin_Amux__SIO_DIFF)
#endif /* (PGAin_Amux__SIO_CFG) */

/* Interrupt Registers */
#if defined(PGAin_Amux__INTSTAT)
    #define PGAin_Amux_INTSTAT            (* (reg8 *) PGAin_Amux__INTSTAT)
    #define PGAin_Amux_SNAP               (* (reg8 *) PGAin_Amux__SNAP)
    
	#define PGAin_Amux_0_INTTYPE_REG 		(* (reg8 *) PGAin_Amux__0__INTTYPE)
#endif /* (PGAin_Amux__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_PGAin_Amux_H */


/* [] END OF FILE */
