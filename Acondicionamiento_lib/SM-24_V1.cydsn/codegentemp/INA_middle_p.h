/*******************************************************************************
* File Name: INA_middle_p.h  
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

#if !defined(CY_PINS_INA_middle_p_H) /* Pins INA_middle_p_H */
#define CY_PINS_INA_middle_p_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "INA_middle_p_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 INA_middle_p__PORT == 15 && ((INA_middle_p__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    INA_middle_p_Write(uint8 value);
void    INA_middle_p_SetDriveMode(uint8 mode);
uint8   INA_middle_p_ReadDataReg(void);
uint8   INA_middle_p_Read(void);
void    INA_middle_p_SetInterruptMode(uint16 position, uint16 mode);
uint8   INA_middle_p_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the INA_middle_p_SetDriveMode() function.
     *  @{
     */
        #define INA_middle_p_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define INA_middle_p_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define INA_middle_p_DM_RES_UP          PIN_DM_RES_UP
        #define INA_middle_p_DM_RES_DWN         PIN_DM_RES_DWN
        #define INA_middle_p_DM_OD_LO           PIN_DM_OD_LO
        #define INA_middle_p_DM_OD_HI           PIN_DM_OD_HI
        #define INA_middle_p_DM_STRONG          PIN_DM_STRONG
        #define INA_middle_p_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define INA_middle_p_MASK               INA_middle_p__MASK
#define INA_middle_p_SHIFT              INA_middle_p__SHIFT
#define INA_middle_p_WIDTH              1u

/* Interrupt constants */
#if defined(INA_middle_p__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in INA_middle_p_SetInterruptMode() function.
     *  @{
     */
        #define INA_middle_p_INTR_NONE      (uint16)(0x0000u)
        #define INA_middle_p_INTR_RISING    (uint16)(0x0001u)
        #define INA_middle_p_INTR_FALLING   (uint16)(0x0002u)
        #define INA_middle_p_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define INA_middle_p_INTR_MASK      (0x01u) 
#endif /* (INA_middle_p__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define INA_middle_p_PS                     (* (reg8 *) INA_middle_p__PS)
/* Data Register */
#define INA_middle_p_DR                     (* (reg8 *) INA_middle_p__DR)
/* Port Number */
#define INA_middle_p_PRT_NUM                (* (reg8 *) INA_middle_p__PRT) 
/* Connect to Analog Globals */                                                  
#define INA_middle_p_AG                     (* (reg8 *) INA_middle_p__AG)                       
/* Analog MUX bux enable */
#define INA_middle_p_AMUX                   (* (reg8 *) INA_middle_p__AMUX) 
/* Bidirectional Enable */                                                        
#define INA_middle_p_BIE                    (* (reg8 *) INA_middle_p__BIE)
/* Bit-mask for Aliased Register Access */
#define INA_middle_p_BIT_MASK               (* (reg8 *) INA_middle_p__BIT_MASK)
/* Bypass Enable */
#define INA_middle_p_BYP                    (* (reg8 *) INA_middle_p__BYP)
/* Port wide control signals */                                                   
#define INA_middle_p_CTL                    (* (reg8 *) INA_middle_p__CTL)
/* Drive Modes */
#define INA_middle_p_DM0                    (* (reg8 *) INA_middle_p__DM0) 
#define INA_middle_p_DM1                    (* (reg8 *) INA_middle_p__DM1)
#define INA_middle_p_DM2                    (* (reg8 *) INA_middle_p__DM2) 
/* Input Buffer Disable Override */
#define INA_middle_p_INP_DIS                (* (reg8 *) INA_middle_p__INP_DIS)
/* LCD Common or Segment Drive */
#define INA_middle_p_LCD_COM_SEG            (* (reg8 *) INA_middle_p__LCD_COM_SEG)
/* Enable Segment LCD */
#define INA_middle_p_LCD_EN                 (* (reg8 *) INA_middle_p__LCD_EN)
/* Slew Rate Control */
#define INA_middle_p_SLW                    (* (reg8 *) INA_middle_p__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define INA_middle_p_PRTDSI__CAPS_SEL       (* (reg8 *) INA_middle_p__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define INA_middle_p_PRTDSI__DBL_SYNC_IN    (* (reg8 *) INA_middle_p__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define INA_middle_p_PRTDSI__OE_SEL0        (* (reg8 *) INA_middle_p__PRTDSI__OE_SEL0) 
#define INA_middle_p_PRTDSI__OE_SEL1        (* (reg8 *) INA_middle_p__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define INA_middle_p_PRTDSI__OUT_SEL0       (* (reg8 *) INA_middle_p__PRTDSI__OUT_SEL0) 
#define INA_middle_p_PRTDSI__OUT_SEL1       (* (reg8 *) INA_middle_p__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define INA_middle_p_PRTDSI__SYNC_OUT       (* (reg8 *) INA_middle_p__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(INA_middle_p__SIO_CFG)
    #define INA_middle_p_SIO_HYST_EN        (* (reg8 *) INA_middle_p__SIO_HYST_EN)
    #define INA_middle_p_SIO_REG_HIFREQ     (* (reg8 *) INA_middle_p__SIO_REG_HIFREQ)
    #define INA_middle_p_SIO_CFG            (* (reg8 *) INA_middle_p__SIO_CFG)
    #define INA_middle_p_SIO_DIFF           (* (reg8 *) INA_middle_p__SIO_DIFF)
#endif /* (INA_middle_p__SIO_CFG) */

/* Interrupt Registers */
#if defined(INA_middle_p__INTSTAT)
    #define INA_middle_p_INTSTAT            (* (reg8 *) INA_middle_p__INTSTAT)
    #define INA_middle_p_SNAP               (* (reg8 *) INA_middle_p__SNAP)
    
	#define INA_middle_p_0_INTTYPE_REG 		(* (reg8 *) INA_middle_p__0__INTTYPE)
#endif /* (INA_middle_p__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_INA_middle_p_H */


/* [] END OF FILE */
