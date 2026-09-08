/*******************************************************************************
* File Name: DiffOut_p.h  
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

#if !defined(CY_PINS_DiffOut_p_H) /* Pins DiffOut_p_H */
#define CY_PINS_DiffOut_p_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "DiffOut_p_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 DiffOut_p__PORT == 15 && ((DiffOut_p__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    DiffOut_p_Write(uint8 value);
void    DiffOut_p_SetDriveMode(uint8 mode);
uint8   DiffOut_p_ReadDataReg(void);
uint8   DiffOut_p_Read(void);
void    DiffOut_p_SetInterruptMode(uint16 position, uint16 mode);
uint8   DiffOut_p_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the DiffOut_p_SetDriveMode() function.
     *  @{
     */
        #define DiffOut_p_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define DiffOut_p_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define DiffOut_p_DM_RES_UP          PIN_DM_RES_UP
        #define DiffOut_p_DM_RES_DWN         PIN_DM_RES_DWN
        #define DiffOut_p_DM_OD_LO           PIN_DM_OD_LO
        #define DiffOut_p_DM_OD_HI           PIN_DM_OD_HI
        #define DiffOut_p_DM_STRONG          PIN_DM_STRONG
        #define DiffOut_p_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define DiffOut_p_MASK               DiffOut_p__MASK
#define DiffOut_p_SHIFT              DiffOut_p__SHIFT
#define DiffOut_p_WIDTH              1u

/* Interrupt constants */
#if defined(DiffOut_p__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in DiffOut_p_SetInterruptMode() function.
     *  @{
     */
        #define DiffOut_p_INTR_NONE      (uint16)(0x0000u)
        #define DiffOut_p_INTR_RISING    (uint16)(0x0001u)
        #define DiffOut_p_INTR_FALLING   (uint16)(0x0002u)
        #define DiffOut_p_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define DiffOut_p_INTR_MASK      (0x01u) 
#endif /* (DiffOut_p__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define DiffOut_p_PS                     (* (reg8 *) DiffOut_p__PS)
/* Data Register */
#define DiffOut_p_DR                     (* (reg8 *) DiffOut_p__DR)
/* Port Number */
#define DiffOut_p_PRT_NUM                (* (reg8 *) DiffOut_p__PRT) 
/* Connect to Analog Globals */                                                  
#define DiffOut_p_AG                     (* (reg8 *) DiffOut_p__AG)                       
/* Analog MUX bux enable */
#define DiffOut_p_AMUX                   (* (reg8 *) DiffOut_p__AMUX) 
/* Bidirectional Enable */                                                        
#define DiffOut_p_BIE                    (* (reg8 *) DiffOut_p__BIE)
/* Bit-mask for Aliased Register Access */
#define DiffOut_p_BIT_MASK               (* (reg8 *) DiffOut_p__BIT_MASK)
/* Bypass Enable */
#define DiffOut_p_BYP                    (* (reg8 *) DiffOut_p__BYP)
/* Port wide control signals */                                                   
#define DiffOut_p_CTL                    (* (reg8 *) DiffOut_p__CTL)
/* Drive Modes */
#define DiffOut_p_DM0                    (* (reg8 *) DiffOut_p__DM0) 
#define DiffOut_p_DM1                    (* (reg8 *) DiffOut_p__DM1)
#define DiffOut_p_DM2                    (* (reg8 *) DiffOut_p__DM2) 
/* Input Buffer Disable Override */
#define DiffOut_p_INP_DIS                (* (reg8 *) DiffOut_p__INP_DIS)
/* LCD Common or Segment Drive */
#define DiffOut_p_LCD_COM_SEG            (* (reg8 *) DiffOut_p__LCD_COM_SEG)
/* Enable Segment LCD */
#define DiffOut_p_LCD_EN                 (* (reg8 *) DiffOut_p__LCD_EN)
/* Slew Rate Control */
#define DiffOut_p_SLW                    (* (reg8 *) DiffOut_p__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define DiffOut_p_PRTDSI__CAPS_SEL       (* (reg8 *) DiffOut_p__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define DiffOut_p_PRTDSI__DBL_SYNC_IN    (* (reg8 *) DiffOut_p__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define DiffOut_p_PRTDSI__OE_SEL0        (* (reg8 *) DiffOut_p__PRTDSI__OE_SEL0) 
#define DiffOut_p_PRTDSI__OE_SEL1        (* (reg8 *) DiffOut_p__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define DiffOut_p_PRTDSI__OUT_SEL0       (* (reg8 *) DiffOut_p__PRTDSI__OUT_SEL0) 
#define DiffOut_p_PRTDSI__OUT_SEL1       (* (reg8 *) DiffOut_p__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define DiffOut_p_PRTDSI__SYNC_OUT       (* (reg8 *) DiffOut_p__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(DiffOut_p__SIO_CFG)
    #define DiffOut_p_SIO_HYST_EN        (* (reg8 *) DiffOut_p__SIO_HYST_EN)
    #define DiffOut_p_SIO_REG_HIFREQ     (* (reg8 *) DiffOut_p__SIO_REG_HIFREQ)
    #define DiffOut_p_SIO_CFG            (* (reg8 *) DiffOut_p__SIO_CFG)
    #define DiffOut_p_SIO_DIFF           (* (reg8 *) DiffOut_p__SIO_DIFF)
#endif /* (DiffOut_p__SIO_CFG) */

/* Interrupt Registers */
#if defined(DiffOut_p__INTSTAT)
    #define DiffOut_p_INTSTAT            (* (reg8 *) DiffOut_p__INTSTAT)
    #define DiffOut_p_SNAP               (* (reg8 *) DiffOut_p__SNAP)
    
	#define DiffOut_p_0_INTTYPE_REG 		(* (reg8 *) DiffOut_p__0__INTTYPE)
#endif /* (DiffOut_p__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_DiffOut_p_H */


/* [] END OF FILE */
