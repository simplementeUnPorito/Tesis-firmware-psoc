/*******************************************************************************
* File Name: MuxIn_n.h  
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

#if !defined(CY_PINS_MuxIn_n_H) /* Pins MuxIn_n_H */
#define CY_PINS_MuxIn_n_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "MuxIn_n_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 MuxIn_n__PORT == 15 && ((MuxIn_n__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    MuxIn_n_Write(uint8 value);
void    MuxIn_n_SetDriveMode(uint8 mode);
uint8   MuxIn_n_ReadDataReg(void);
uint8   MuxIn_n_Read(void);
void    MuxIn_n_SetInterruptMode(uint16 position, uint16 mode);
uint8   MuxIn_n_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the MuxIn_n_SetDriveMode() function.
     *  @{
     */
        #define MuxIn_n_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define MuxIn_n_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define MuxIn_n_DM_RES_UP          PIN_DM_RES_UP
        #define MuxIn_n_DM_RES_DWN         PIN_DM_RES_DWN
        #define MuxIn_n_DM_OD_LO           PIN_DM_OD_LO
        #define MuxIn_n_DM_OD_HI           PIN_DM_OD_HI
        #define MuxIn_n_DM_STRONG          PIN_DM_STRONG
        #define MuxIn_n_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define MuxIn_n_MASK               MuxIn_n__MASK
#define MuxIn_n_SHIFT              MuxIn_n__SHIFT
#define MuxIn_n_WIDTH              1u

/* Interrupt constants */
#if defined(MuxIn_n__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in MuxIn_n_SetInterruptMode() function.
     *  @{
     */
        #define MuxIn_n_INTR_NONE      (uint16)(0x0000u)
        #define MuxIn_n_INTR_RISING    (uint16)(0x0001u)
        #define MuxIn_n_INTR_FALLING   (uint16)(0x0002u)
        #define MuxIn_n_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define MuxIn_n_INTR_MASK      (0x01u) 
#endif /* (MuxIn_n__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define MuxIn_n_PS                     (* (reg8 *) MuxIn_n__PS)
/* Data Register */
#define MuxIn_n_DR                     (* (reg8 *) MuxIn_n__DR)
/* Port Number */
#define MuxIn_n_PRT_NUM                (* (reg8 *) MuxIn_n__PRT) 
/* Connect to Analog Globals */                                                  
#define MuxIn_n_AG                     (* (reg8 *) MuxIn_n__AG)                       
/* Analog MUX bux enable */
#define MuxIn_n_AMUX                   (* (reg8 *) MuxIn_n__AMUX) 
/* Bidirectional Enable */                                                        
#define MuxIn_n_BIE                    (* (reg8 *) MuxIn_n__BIE)
/* Bit-mask for Aliased Register Access */
#define MuxIn_n_BIT_MASK               (* (reg8 *) MuxIn_n__BIT_MASK)
/* Bypass Enable */
#define MuxIn_n_BYP                    (* (reg8 *) MuxIn_n__BYP)
/* Port wide control signals */                                                   
#define MuxIn_n_CTL                    (* (reg8 *) MuxIn_n__CTL)
/* Drive Modes */
#define MuxIn_n_DM0                    (* (reg8 *) MuxIn_n__DM0) 
#define MuxIn_n_DM1                    (* (reg8 *) MuxIn_n__DM1)
#define MuxIn_n_DM2                    (* (reg8 *) MuxIn_n__DM2) 
/* Input Buffer Disable Override */
#define MuxIn_n_INP_DIS                (* (reg8 *) MuxIn_n__INP_DIS)
/* LCD Common or Segment Drive */
#define MuxIn_n_LCD_COM_SEG            (* (reg8 *) MuxIn_n__LCD_COM_SEG)
/* Enable Segment LCD */
#define MuxIn_n_LCD_EN                 (* (reg8 *) MuxIn_n__LCD_EN)
/* Slew Rate Control */
#define MuxIn_n_SLW                    (* (reg8 *) MuxIn_n__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define MuxIn_n_PRTDSI__CAPS_SEL       (* (reg8 *) MuxIn_n__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define MuxIn_n_PRTDSI__DBL_SYNC_IN    (* (reg8 *) MuxIn_n__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define MuxIn_n_PRTDSI__OE_SEL0        (* (reg8 *) MuxIn_n__PRTDSI__OE_SEL0) 
#define MuxIn_n_PRTDSI__OE_SEL1        (* (reg8 *) MuxIn_n__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define MuxIn_n_PRTDSI__OUT_SEL0       (* (reg8 *) MuxIn_n__PRTDSI__OUT_SEL0) 
#define MuxIn_n_PRTDSI__OUT_SEL1       (* (reg8 *) MuxIn_n__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define MuxIn_n_PRTDSI__SYNC_OUT       (* (reg8 *) MuxIn_n__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(MuxIn_n__SIO_CFG)
    #define MuxIn_n_SIO_HYST_EN        (* (reg8 *) MuxIn_n__SIO_HYST_EN)
    #define MuxIn_n_SIO_REG_HIFREQ     (* (reg8 *) MuxIn_n__SIO_REG_HIFREQ)
    #define MuxIn_n_SIO_CFG            (* (reg8 *) MuxIn_n__SIO_CFG)
    #define MuxIn_n_SIO_DIFF           (* (reg8 *) MuxIn_n__SIO_DIFF)
#endif /* (MuxIn_n__SIO_CFG) */

/* Interrupt Registers */
#if defined(MuxIn_n__INTSTAT)
    #define MuxIn_n_INTSTAT            (* (reg8 *) MuxIn_n__INTSTAT)
    #define MuxIn_n_SNAP               (* (reg8 *) MuxIn_n__SNAP)
    
	#define MuxIn_n_0_INTTYPE_REG 		(* (reg8 *) MuxIn_n__0__INTTYPE)
#endif /* (MuxIn_n__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_MuxIn_n_H */


/* [] END OF FILE */
