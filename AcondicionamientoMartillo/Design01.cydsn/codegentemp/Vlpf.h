/*******************************************************************************
* File Name: Vlpf.h  
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

#if !defined(CY_PINS_Vlpf_H) /* Pins Vlpf_H */
#define CY_PINS_Vlpf_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "Vlpf_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 Vlpf__PORT == 15 && ((Vlpf__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    Vlpf_Write(uint8 value);
void    Vlpf_SetDriveMode(uint8 mode);
uint8   Vlpf_ReadDataReg(void);
uint8   Vlpf_Read(void);
void    Vlpf_SetInterruptMode(uint16 position, uint16 mode);
uint8   Vlpf_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the Vlpf_SetDriveMode() function.
     *  @{
     */
        #define Vlpf_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define Vlpf_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define Vlpf_DM_RES_UP          PIN_DM_RES_UP
        #define Vlpf_DM_RES_DWN         PIN_DM_RES_DWN
        #define Vlpf_DM_OD_LO           PIN_DM_OD_LO
        #define Vlpf_DM_OD_HI           PIN_DM_OD_HI
        #define Vlpf_DM_STRONG          PIN_DM_STRONG
        #define Vlpf_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define Vlpf_MASK               Vlpf__MASK
#define Vlpf_SHIFT              Vlpf__SHIFT
#define Vlpf_WIDTH              1u

/* Interrupt constants */
#if defined(Vlpf__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in Vlpf_SetInterruptMode() function.
     *  @{
     */
        #define Vlpf_INTR_NONE      (uint16)(0x0000u)
        #define Vlpf_INTR_RISING    (uint16)(0x0001u)
        #define Vlpf_INTR_FALLING   (uint16)(0x0002u)
        #define Vlpf_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define Vlpf_INTR_MASK      (0x01u) 
#endif /* (Vlpf__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define Vlpf_PS                     (* (reg8 *) Vlpf__PS)
/* Data Register */
#define Vlpf_DR                     (* (reg8 *) Vlpf__DR)
/* Port Number */
#define Vlpf_PRT_NUM                (* (reg8 *) Vlpf__PRT) 
/* Connect to Analog Globals */                                                  
#define Vlpf_AG                     (* (reg8 *) Vlpf__AG)                       
/* Analog MUX bux enable */
#define Vlpf_AMUX                   (* (reg8 *) Vlpf__AMUX) 
/* Bidirectional Enable */                                                        
#define Vlpf_BIE                    (* (reg8 *) Vlpf__BIE)
/* Bit-mask for Aliased Register Access */
#define Vlpf_BIT_MASK               (* (reg8 *) Vlpf__BIT_MASK)
/* Bypass Enable */
#define Vlpf_BYP                    (* (reg8 *) Vlpf__BYP)
/* Port wide control signals */                                                   
#define Vlpf_CTL                    (* (reg8 *) Vlpf__CTL)
/* Drive Modes */
#define Vlpf_DM0                    (* (reg8 *) Vlpf__DM0) 
#define Vlpf_DM1                    (* (reg8 *) Vlpf__DM1)
#define Vlpf_DM2                    (* (reg8 *) Vlpf__DM2) 
/* Input Buffer Disable Override */
#define Vlpf_INP_DIS                (* (reg8 *) Vlpf__INP_DIS)
/* LCD Common or Segment Drive */
#define Vlpf_LCD_COM_SEG            (* (reg8 *) Vlpf__LCD_COM_SEG)
/* Enable Segment LCD */
#define Vlpf_LCD_EN                 (* (reg8 *) Vlpf__LCD_EN)
/* Slew Rate Control */
#define Vlpf_SLW                    (* (reg8 *) Vlpf__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define Vlpf_PRTDSI__CAPS_SEL       (* (reg8 *) Vlpf__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define Vlpf_PRTDSI__DBL_SYNC_IN    (* (reg8 *) Vlpf__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define Vlpf_PRTDSI__OE_SEL0        (* (reg8 *) Vlpf__PRTDSI__OE_SEL0) 
#define Vlpf_PRTDSI__OE_SEL1        (* (reg8 *) Vlpf__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define Vlpf_PRTDSI__OUT_SEL0       (* (reg8 *) Vlpf__PRTDSI__OUT_SEL0) 
#define Vlpf_PRTDSI__OUT_SEL1       (* (reg8 *) Vlpf__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define Vlpf_PRTDSI__SYNC_OUT       (* (reg8 *) Vlpf__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(Vlpf__SIO_CFG)
    #define Vlpf_SIO_HYST_EN        (* (reg8 *) Vlpf__SIO_HYST_EN)
    #define Vlpf_SIO_REG_HIFREQ     (* (reg8 *) Vlpf__SIO_REG_HIFREQ)
    #define Vlpf_SIO_CFG            (* (reg8 *) Vlpf__SIO_CFG)
    #define Vlpf_SIO_DIFF           (* (reg8 *) Vlpf__SIO_DIFF)
#endif /* (Vlpf__SIO_CFG) */

/* Interrupt Registers */
#if defined(Vlpf__INTSTAT)
    #define Vlpf_INTSTAT            (* (reg8 *) Vlpf__INTSTAT)
    #define Vlpf_SNAP               (* (reg8 *) Vlpf__SNAP)
    
	#define Vlpf_0_INTTYPE_REG 		(* (reg8 *) Vlpf__0__INTTYPE)
#endif /* (Vlpf__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_Vlpf_H */


/* [] END OF FILE */
