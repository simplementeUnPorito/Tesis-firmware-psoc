/*******************************************************************************
* File Name: Vbuffer.h  
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

#if !defined(CY_PINS_Vbuffer_H) /* Pins Vbuffer_H */
#define CY_PINS_Vbuffer_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "Vbuffer_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 Vbuffer__PORT == 15 && ((Vbuffer__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    Vbuffer_Write(uint8 value);
void    Vbuffer_SetDriveMode(uint8 mode);
uint8   Vbuffer_ReadDataReg(void);
uint8   Vbuffer_Read(void);
void    Vbuffer_SetInterruptMode(uint16 position, uint16 mode);
uint8   Vbuffer_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the Vbuffer_SetDriveMode() function.
     *  @{
     */
        #define Vbuffer_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define Vbuffer_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define Vbuffer_DM_RES_UP          PIN_DM_RES_UP
        #define Vbuffer_DM_RES_DWN         PIN_DM_RES_DWN
        #define Vbuffer_DM_OD_LO           PIN_DM_OD_LO
        #define Vbuffer_DM_OD_HI           PIN_DM_OD_HI
        #define Vbuffer_DM_STRONG          PIN_DM_STRONG
        #define Vbuffer_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define Vbuffer_MASK               Vbuffer__MASK
#define Vbuffer_SHIFT              Vbuffer__SHIFT
#define Vbuffer_WIDTH              1u

/* Interrupt constants */
#if defined(Vbuffer__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in Vbuffer_SetInterruptMode() function.
     *  @{
     */
        #define Vbuffer_INTR_NONE      (uint16)(0x0000u)
        #define Vbuffer_INTR_RISING    (uint16)(0x0001u)
        #define Vbuffer_INTR_FALLING   (uint16)(0x0002u)
        #define Vbuffer_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define Vbuffer_INTR_MASK      (0x01u) 
#endif /* (Vbuffer__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define Vbuffer_PS                     (* (reg8 *) Vbuffer__PS)
/* Data Register */
#define Vbuffer_DR                     (* (reg8 *) Vbuffer__DR)
/* Port Number */
#define Vbuffer_PRT_NUM                (* (reg8 *) Vbuffer__PRT) 
/* Connect to Analog Globals */                                                  
#define Vbuffer_AG                     (* (reg8 *) Vbuffer__AG)                       
/* Analog MUX bux enable */
#define Vbuffer_AMUX                   (* (reg8 *) Vbuffer__AMUX) 
/* Bidirectional Enable */                                                        
#define Vbuffer_BIE                    (* (reg8 *) Vbuffer__BIE)
/* Bit-mask for Aliased Register Access */
#define Vbuffer_BIT_MASK               (* (reg8 *) Vbuffer__BIT_MASK)
/* Bypass Enable */
#define Vbuffer_BYP                    (* (reg8 *) Vbuffer__BYP)
/* Port wide control signals */                                                   
#define Vbuffer_CTL                    (* (reg8 *) Vbuffer__CTL)
/* Drive Modes */
#define Vbuffer_DM0                    (* (reg8 *) Vbuffer__DM0) 
#define Vbuffer_DM1                    (* (reg8 *) Vbuffer__DM1)
#define Vbuffer_DM2                    (* (reg8 *) Vbuffer__DM2) 
/* Input Buffer Disable Override */
#define Vbuffer_INP_DIS                (* (reg8 *) Vbuffer__INP_DIS)
/* LCD Common or Segment Drive */
#define Vbuffer_LCD_COM_SEG            (* (reg8 *) Vbuffer__LCD_COM_SEG)
/* Enable Segment LCD */
#define Vbuffer_LCD_EN                 (* (reg8 *) Vbuffer__LCD_EN)
/* Slew Rate Control */
#define Vbuffer_SLW                    (* (reg8 *) Vbuffer__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define Vbuffer_PRTDSI__CAPS_SEL       (* (reg8 *) Vbuffer__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define Vbuffer_PRTDSI__DBL_SYNC_IN    (* (reg8 *) Vbuffer__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define Vbuffer_PRTDSI__OE_SEL0        (* (reg8 *) Vbuffer__PRTDSI__OE_SEL0) 
#define Vbuffer_PRTDSI__OE_SEL1        (* (reg8 *) Vbuffer__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define Vbuffer_PRTDSI__OUT_SEL0       (* (reg8 *) Vbuffer__PRTDSI__OUT_SEL0) 
#define Vbuffer_PRTDSI__OUT_SEL1       (* (reg8 *) Vbuffer__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define Vbuffer_PRTDSI__SYNC_OUT       (* (reg8 *) Vbuffer__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(Vbuffer__SIO_CFG)
    #define Vbuffer_SIO_HYST_EN        (* (reg8 *) Vbuffer__SIO_HYST_EN)
    #define Vbuffer_SIO_REG_HIFREQ     (* (reg8 *) Vbuffer__SIO_REG_HIFREQ)
    #define Vbuffer_SIO_CFG            (* (reg8 *) Vbuffer__SIO_CFG)
    #define Vbuffer_SIO_DIFF           (* (reg8 *) Vbuffer__SIO_DIFF)
#endif /* (Vbuffer__SIO_CFG) */

/* Interrupt Registers */
#if defined(Vbuffer__INTSTAT)
    #define Vbuffer_INTSTAT            (* (reg8 *) Vbuffer__INTSTAT)
    #define Vbuffer_SNAP               (* (reg8 *) Vbuffer__SNAP)
    
	#define Vbuffer_0_INTTYPE_REG 		(* (reg8 *) Vbuffer__0__INTTYPE)
#endif /* (Vbuffer__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_Vbuffer_H */


/* [] END OF FILE */
