/*******************************************************************************
* File Name: BufferIn_p.h  
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

#if !defined(CY_PINS_BufferIn_p_H) /* Pins BufferIn_p_H */
#define CY_PINS_BufferIn_p_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "BufferIn_p_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 BufferIn_p__PORT == 15 && ((BufferIn_p__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    BufferIn_p_Write(uint8 value);
void    BufferIn_p_SetDriveMode(uint8 mode);
uint8   BufferIn_p_ReadDataReg(void);
uint8   BufferIn_p_Read(void);
void    BufferIn_p_SetInterruptMode(uint16 position, uint16 mode);
uint8   BufferIn_p_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the BufferIn_p_SetDriveMode() function.
     *  @{
     */
        #define BufferIn_p_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define BufferIn_p_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define BufferIn_p_DM_RES_UP          PIN_DM_RES_UP
        #define BufferIn_p_DM_RES_DWN         PIN_DM_RES_DWN
        #define BufferIn_p_DM_OD_LO           PIN_DM_OD_LO
        #define BufferIn_p_DM_OD_HI           PIN_DM_OD_HI
        #define BufferIn_p_DM_STRONG          PIN_DM_STRONG
        #define BufferIn_p_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define BufferIn_p_MASK               BufferIn_p__MASK
#define BufferIn_p_SHIFT              BufferIn_p__SHIFT
#define BufferIn_p_WIDTH              1u

/* Interrupt constants */
#if defined(BufferIn_p__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in BufferIn_p_SetInterruptMode() function.
     *  @{
     */
        #define BufferIn_p_INTR_NONE      (uint16)(0x0000u)
        #define BufferIn_p_INTR_RISING    (uint16)(0x0001u)
        #define BufferIn_p_INTR_FALLING   (uint16)(0x0002u)
        #define BufferIn_p_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define BufferIn_p_INTR_MASK      (0x01u) 
#endif /* (BufferIn_p__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define BufferIn_p_PS                     (* (reg8 *) BufferIn_p__PS)
/* Data Register */
#define BufferIn_p_DR                     (* (reg8 *) BufferIn_p__DR)
/* Port Number */
#define BufferIn_p_PRT_NUM                (* (reg8 *) BufferIn_p__PRT) 
/* Connect to Analog Globals */                                                  
#define BufferIn_p_AG                     (* (reg8 *) BufferIn_p__AG)                       
/* Analog MUX bux enable */
#define BufferIn_p_AMUX                   (* (reg8 *) BufferIn_p__AMUX) 
/* Bidirectional Enable */                                                        
#define BufferIn_p_BIE                    (* (reg8 *) BufferIn_p__BIE)
/* Bit-mask for Aliased Register Access */
#define BufferIn_p_BIT_MASK               (* (reg8 *) BufferIn_p__BIT_MASK)
/* Bypass Enable */
#define BufferIn_p_BYP                    (* (reg8 *) BufferIn_p__BYP)
/* Port wide control signals */                                                   
#define BufferIn_p_CTL                    (* (reg8 *) BufferIn_p__CTL)
/* Drive Modes */
#define BufferIn_p_DM0                    (* (reg8 *) BufferIn_p__DM0) 
#define BufferIn_p_DM1                    (* (reg8 *) BufferIn_p__DM1)
#define BufferIn_p_DM2                    (* (reg8 *) BufferIn_p__DM2) 
/* Input Buffer Disable Override */
#define BufferIn_p_INP_DIS                (* (reg8 *) BufferIn_p__INP_DIS)
/* LCD Common or Segment Drive */
#define BufferIn_p_LCD_COM_SEG            (* (reg8 *) BufferIn_p__LCD_COM_SEG)
/* Enable Segment LCD */
#define BufferIn_p_LCD_EN                 (* (reg8 *) BufferIn_p__LCD_EN)
/* Slew Rate Control */
#define BufferIn_p_SLW                    (* (reg8 *) BufferIn_p__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define BufferIn_p_PRTDSI__CAPS_SEL       (* (reg8 *) BufferIn_p__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define BufferIn_p_PRTDSI__DBL_SYNC_IN    (* (reg8 *) BufferIn_p__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define BufferIn_p_PRTDSI__OE_SEL0        (* (reg8 *) BufferIn_p__PRTDSI__OE_SEL0) 
#define BufferIn_p_PRTDSI__OE_SEL1        (* (reg8 *) BufferIn_p__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define BufferIn_p_PRTDSI__OUT_SEL0       (* (reg8 *) BufferIn_p__PRTDSI__OUT_SEL0) 
#define BufferIn_p_PRTDSI__OUT_SEL1       (* (reg8 *) BufferIn_p__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define BufferIn_p_PRTDSI__SYNC_OUT       (* (reg8 *) BufferIn_p__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(BufferIn_p__SIO_CFG)
    #define BufferIn_p_SIO_HYST_EN        (* (reg8 *) BufferIn_p__SIO_HYST_EN)
    #define BufferIn_p_SIO_REG_HIFREQ     (* (reg8 *) BufferIn_p__SIO_REG_HIFREQ)
    #define BufferIn_p_SIO_CFG            (* (reg8 *) BufferIn_p__SIO_CFG)
    #define BufferIn_p_SIO_DIFF           (* (reg8 *) BufferIn_p__SIO_DIFF)
#endif /* (BufferIn_p__SIO_CFG) */

/* Interrupt Registers */
#if defined(BufferIn_p__INTSTAT)
    #define BufferIn_p_INTSTAT            (* (reg8 *) BufferIn_p__INTSTAT)
    #define BufferIn_p_SNAP               (* (reg8 *) BufferIn_p__SNAP)
    
	#define BufferIn_p_0_INTTYPE_REG 		(* (reg8 *) BufferIn_p__0__INTTYPE)
#endif /* (BufferIn_p__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_BufferIn_p_H */


/* [] END OF FILE */
