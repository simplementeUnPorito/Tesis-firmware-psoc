/*******************************************************************************
* File Name: SPIp.h  
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

#if !defined(CY_PINS_SPIp_ALIASES_H) /* Pins SPIp_ALIASES_H */
#define CY_PINS_SPIp_ALIASES_H

#include "cytypes.h"
#include "cyfitter.h"


/***************************************
*              Constants        
***************************************/
#define SPIp_0			(SPIp__0__PC)
#define SPIp_0_INTR	((uint16)((uint16)0x0001u << SPIp__0__SHIFT))

#define SPIp_1			(SPIp__1__PC)
#define SPIp_1_INTR	((uint16)((uint16)0x0001u << SPIp__1__SHIFT))

#define SPIp_2			(SPIp__2__PC)
#define SPIp_2_INTR	((uint16)((uint16)0x0001u << SPIp__2__SHIFT))

#define SPIp_3			(SPIp__3__PC)
#define SPIp_3_INTR	((uint16)((uint16)0x0001u << SPIp__3__SHIFT))

#define SPIp_INTR_ALL	 ((uint16)(SPIp_0_INTR| SPIp_1_INTR| SPIp_2_INTR| SPIp_3_INTR))
#define SPIp_CS			(SPIp__CS__PC)
#define SPIp_CS_INTR	((uint16)((uint16)0x0001u << SPIp__0__SHIFT))

#define SPIp_SCK			(SPIp__SCK__PC)
#define SPIp_SCK_INTR	((uint16)((uint16)0x0001u << SPIp__1__SHIFT))

#define SPIp_MOSI			(SPIp__MOSI__PC)
#define SPIp_MOSI_INTR	((uint16)((uint16)0x0001u << SPIp__2__SHIFT))

#define SPIp_MISO			(SPIp__MISO__PC)
#define SPIp_MISO_INTR	((uint16)((uint16)0x0001u << SPIp__3__SHIFT))

#endif /* End Pins SPIp_ALIASES_H */


/* [] END OF FILE */
