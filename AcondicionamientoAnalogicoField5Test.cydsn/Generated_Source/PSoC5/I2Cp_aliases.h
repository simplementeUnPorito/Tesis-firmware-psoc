/*******************************************************************************
* File Name: I2Cp.h  
* Version 2.20
*
* Description:
*  This file contains the Alias definitions for Per-Pin APIs in cypins.h. 
*  Information on using these APIs can be found in the System Reference Guide.
*
* Note:
*
********************************************************************************
* Copyright 2008-2015, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions, 
* disclaimers, and limitations in the end user license agreement accompanying 
* the software package with which this file was provided.
*******************************************************************************/

#if !defined(CY_PINS_I2Cp_ALIASES_H) /* Pins I2Cp_ALIASES_H */
#define CY_PINS_I2Cp_ALIASES_H

#include "cytypes.h"
#include "cyfitter.h"


/***************************************
*              Constants        
***************************************/
#define I2Cp_0			(I2Cp__0__PC)
#define I2Cp_0_INTR	((uint16)((uint16)0x0001u << I2Cp__0__SHIFT))

#define I2Cp_1			(I2Cp__1__PC)
#define I2Cp_1_INTR	((uint16)((uint16)0x0001u << I2Cp__1__SHIFT))

#define I2Cp_INTR_ALL	 ((uint16)(I2Cp_0_INTR| I2Cp_1_INTR))
#define I2Cp_SDA			(I2Cp__SDA__PC)
#define I2Cp_SDA_INTR	((uint16)((uint16)0x0001u << I2Cp__0__SHIFT))

#define I2Cp_SCL			(I2Cp__SCL__PC)
#define I2Cp_SCL_INTR	((uint16)((uint16)0x0001u << I2Cp__1__SHIFT))

#endif /* End Pins I2Cp_ALIASES_H */


/* [] END OF FILE */
