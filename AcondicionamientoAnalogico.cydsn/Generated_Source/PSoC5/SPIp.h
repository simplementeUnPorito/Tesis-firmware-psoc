/*******************************************************************************
* File Name: SPIp.h  
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

#if !defined(CY_PINS_SPIp_H) /* Pins SPIp_H */
#define CY_PINS_SPIp_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "SPIp_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 SPIp__PORT == 15 && ((SPIp__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    SPIp_Write(uint8 value);
void    SPIp_SetDriveMode(uint8 mode);
uint8   SPIp_ReadDataReg(void);
uint8   SPIp_Read(void);
void    SPIp_SetInterruptMode(uint16 position, uint16 mode);
uint8   SPIp_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the SPIp_SetDriveMode() function.
     *  @{
     */
        #define SPIp_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define SPIp_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define SPIp_DM_RES_UP          PIN_DM_RES_UP
        #define SPIp_DM_RES_DWN         PIN_DM_RES_DWN
        #define SPIp_DM_OD_LO           PIN_DM_OD_LO
        #define SPIp_DM_OD_HI           PIN_DM_OD_HI
        #define SPIp_DM_STRONG          PIN_DM_STRONG
        #define SPIp_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define SPIp_MASK               SPIp__MASK
#define SPIp_SHIFT              SPIp__SHIFT
#define SPIp_WIDTH              4u

/* Interrupt constants */
#if defined(SPIp__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in SPIp_SetInterruptMode() function.
     *  @{
     */
        #define SPIp_INTR_NONE      (uint16)(0x0000u)
        #define SPIp_INTR_RISING    (uint16)(0x0001u)
        #define SPIp_INTR_FALLING   (uint16)(0x0002u)
        #define SPIp_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define SPIp_INTR_MASK      (0x01u) 
#endif /* (SPIp__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define SPIp_PS                     (* (reg8 *) SPIp__PS)
/* Data Register */
#define SPIp_DR                     (* (reg8 *) SPIp__DR)
/* Port Number */
#define SPIp_PRT_NUM                (* (reg8 *) SPIp__PRT) 
/* Connect to Analog Globals */                                                  
#define SPIp_AG                     (* (reg8 *) SPIp__AG)                       
/* Analog MUX bux enable */
#define SPIp_AMUX                   (* (reg8 *) SPIp__AMUX) 
/* Bidirectional Enable */                                                        
#define SPIp_BIE                    (* (reg8 *) SPIp__BIE)
/* Bit-mask for Aliased Register Access */
#define SPIp_BIT_MASK               (* (reg8 *) SPIp__BIT_MASK)
/* Bypass Enable */
#define SPIp_BYP                    (* (reg8 *) SPIp__BYP)
/* Port wide control signals */                                                   
#define SPIp_CTL                    (* (reg8 *) SPIp__CTL)
/* Drive Modes */
#define SPIp_DM0                    (* (reg8 *) SPIp__DM0) 
#define SPIp_DM1                    (* (reg8 *) SPIp__DM1)
#define SPIp_DM2                    (* (reg8 *) SPIp__DM2) 
/* Input Buffer Disable Override */
#define SPIp_INP_DIS                (* (reg8 *) SPIp__INP_DIS)
/* LCD Common or Segment Drive */
#define SPIp_LCD_COM_SEG            (* (reg8 *) SPIp__LCD_COM_SEG)
/* Enable Segment LCD */
#define SPIp_LCD_EN                 (* (reg8 *) SPIp__LCD_EN)
/* Slew Rate Control */
#define SPIp_SLW                    (* (reg8 *) SPIp__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define SPIp_PRTDSI__CAPS_SEL       (* (reg8 *) SPIp__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define SPIp_PRTDSI__DBL_SYNC_IN    (* (reg8 *) SPIp__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define SPIp_PRTDSI__OE_SEL0        (* (reg8 *) SPIp__PRTDSI__OE_SEL0) 
#define SPIp_PRTDSI__OE_SEL1        (* (reg8 *) SPIp__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define SPIp_PRTDSI__OUT_SEL0       (* (reg8 *) SPIp__PRTDSI__OUT_SEL0) 
#define SPIp_PRTDSI__OUT_SEL1       (* (reg8 *) SPIp__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define SPIp_PRTDSI__SYNC_OUT       (* (reg8 *) SPIp__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(SPIp__SIO_CFG)
    #define SPIp_SIO_HYST_EN        (* (reg8 *) SPIp__SIO_HYST_EN)
    #define SPIp_SIO_REG_HIFREQ     (* (reg8 *) SPIp__SIO_REG_HIFREQ)
    #define SPIp_SIO_CFG            (* (reg8 *) SPIp__SIO_CFG)
    #define SPIp_SIO_DIFF           (* (reg8 *) SPIp__SIO_DIFF)
#endif /* (SPIp__SIO_CFG) */

/* Interrupt Registers */
#if defined(SPIp__INTSTAT)
    #define SPIp_INTSTAT            (* (reg8 *) SPIp__INTSTAT)
    #define SPIp_SNAP               (* (reg8 *) SPIp__SNAP)
    
	#define SPIp_0_INTTYPE_REG 		(* (reg8 *) SPIp__0__INTTYPE)
	#define SPIp_1_INTTYPE_REG 		(* (reg8 *) SPIp__1__INTTYPE)
	#define SPIp_2_INTTYPE_REG 		(* (reg8 *) SPIp__2__INTTYPE)
	#define SPIp_3_INTTYPE_REG 		(* (reg8 *) SPIp__3__INTTYPE)
#endif /* (SPIp__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_SPIp_H */


/* [] END OF FILE */
