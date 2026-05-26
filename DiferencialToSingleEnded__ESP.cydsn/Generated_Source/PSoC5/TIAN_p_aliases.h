/*******************************************************************************
* File Name: TIAN_p.h  
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

#if !defined(CY_PINS_TIAN_p_ALIASES_H) /* Pins TIAN_p_ALIASES_H */
#define CY_PINS_TIAN_p_ALIASES_H

#include "cytypes.h"
#include "cyfitter.h"


/***************************************
*              Constants        
***************************************/
#define TIAN_p_0			(TIAN_p__0__PC)
#define TIAN_p_0_INTR	((uint16)((uint16)0x0001u << TIAN_p__0__SHIFT))

#define TIAN_p_INTR_ALL	 ((uint16)(TIAN_p_0_INTR))

#endif /* End Pins TIAN_p_ALIASES_H */


/* [] END OF FILE */
