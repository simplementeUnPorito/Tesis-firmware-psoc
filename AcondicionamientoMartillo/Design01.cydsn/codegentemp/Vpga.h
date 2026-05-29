/*******************************************************************************
* File Name: Vpga.h  
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

#if !defined(CY_PINS_Vpga_H) /* Pins Vpga_H */
#define CY_PINS_Vpga_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "Vpga_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 Vpga__PORT == 15 && ((Vpga__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    Vpga_Write(uint8 value);
void    Vpga_SetDriveMode(uint8 mode);
uint8   Vpga_ReadDataReg(void);
uint8   Vpga_Read(void);
void    Vpga_SetInterruptMode(uint16 position, uint16 mode);
uint8   Vpga_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the Vpga_SetDriveMode() function.
     *  @{
     */
        #define Vpga_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define Vpga_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define Vpga_DM_RES_UP          PIN_DM_RES_UP
        #define Vpga_DM_RES_DWN         PIN_DM_RES_DWN
        #define Vpga_DM_OD_LO           PIN_DM_OD_LO
        #define Vpga_DM_OD_HI           PIN_DM_OD_HI
        #define Vpga_DM_STRONG          PIN_DM_STRONG
        #define Vpga_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define Vpga_MASK               Vpga__MASK
#define Vpga_SHIFT              Vpga__SHIFT
#define Vpga_WIDTH              1u

/* Interrupt constants */
#if defined(Vpga__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in Vpga_SetInterruptMode() function.
     *  @{
     */
        #define Vpga_INTR_NONE      (uint16)(0x0000u)
        #define Vpga_INTR_RISING    (uint16)(0x0001u)
        #define Vpga_INTR_FALLING   (uint16)(0x0002u)
        #define Vpga_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define Vpga_INTR_MASK      (0x01u) 
#endif /* (Vpga__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define Vpga_PS                     (* (reg8 *) Vpga__PS)
/* Data Register */
#define Vpga_DR                     (* (reg8 *) Vpga__DR)
/* Port Number */
#define Vpga_PRT_NUM                (* (reg8 *) Vpga__PRT) 
/* Connect to Analog Globals */                                                  
#define Vpga_AG                     (* (reg8 *) Vpga__AG)                       
/* Analog MUX bux enable */
#define Vpga_AMUX                   (* (reg8 *) Vpga__AMUX) 
/* Bidirectional Enable */                                                        
#define Vpga_BIE                    (* (reg8 *) Vpga__BIE)
/* Bit-mask for Aliased Register Access */
#define Vpga_BIT_MASK               (* (reg8 *) Vpga__BIT_MASK)
/* Bypass Enable */
#define Vpga_BYP                    (* (reg8 *) Vpga__BYP)
/* Port wide control signals */                                                   
#define Vpga_CTL                    (* (reg8 *) Vpga__CTL)
/* Drive Modes */
#define Vpga_DM0                    (* (reg8 *) Vpga__DM0) 
#define Vpga_DM1                    (* (reg8 *) Vpga__DM1)
#define Vpga_DM2                    (* (reg8 *) Vpga__DM2) 
/* Input Buffer Disable Override */
#define Vpga_INP_DIS                (* (reg8 *) Vpga__INP_DIS)
/* LCD Common or Segment Drive */
#define Vpga_LCD_COM_SEG            (* (reg8 *) Vpga__LCD_COM_SEG)
/* Enable Segment LCD */
#define Vpga_LCD_EN                 (* (reg8 *) Vpga__LCD_EN)
/* Slew Rate Control */
#define Vpga_SLW                    (* (reg8 *) Vpga__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define Vpga_PRTDSI__CAPS_SEL       (* (reg8 *) Vpga__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define Vpga_PRTDSI__DBL_SYNC_IN    (* (reg8 *) Vpga__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define Vpga_PRTDSI__OE_SEL0        (* (reg8 *) Vpga__PRTDSI__OE_SEL0) 
#define Vpga_PRTDSI__OE_SEL1        (* (reg8 *) Vpga__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define Vpga_PRTDSI__OUT_SEL0       (* (reg8 *) Vpga__PRTDSI__OUT_SEL0) 
#define Vpga_PRTDSI__OUT_SEL1       (* (reg8 *) Vpga__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define Vpga_PRTDSI__SYNC_OUT       (* (reg8 *) Vpga__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(Vpga__SIO_CFG)
    #define Vpga_SIO_HYST_EN        (* (reg8 *) Vpga__SIO_HYST_EN)
    #define Vpga_SIO_REG_HIFREQ     (* (reg8 *) Vpga__SIO_REG_HIFREQ)
    #define Vpga_SIO_CFG            (* (reg8 *) Vpga__SIO_CFG)
    #define Vpga_SIO_DIFF           (* (reg8 *) Vpga__SIO_DIFF)
#endif /* (Vpga__SIO_CFG) */

/* Interrupt Registers */
#if defined(Vpga__INTSTAT)
    #define Vpga_INTSTAT            (* (reg8 *) Vpga__INTSTAT)
    #define Vpga_SNAP               (* (reg8 *) Vpga__SNAP)
    
	#define Vpga_0_INTTYPE_REG 		(* (reg8 *) Vpga__0__INTTYPE)
#endif /* (Vpga__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_Vpga_H */


/* [] END OF FILE */
