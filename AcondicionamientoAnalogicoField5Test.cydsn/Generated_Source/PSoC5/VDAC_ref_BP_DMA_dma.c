/***************************************************************************
* File Name: VDAC_ref_BP_DMA_dma.c  
* Version 1.70
*
*  Description:
*   Provides an API for the DMAC component. The API includes functions
*   for the DMA controller, DMA channels and Transfer Descriptors.
*
*
*   Note:
*     This module requires the developer to finish or fill in the auto
*     generated funcions and setup the dma channel and TD's.
*
********************************************************************************
* Copyright 2008-2010, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions, 
* disclaimers, and limitations in the end user license agreement accompanying 
* the software package with which this file was provided.
********************************************************************************/
#include <CYLIB.H>
#include <CYDMAC.H>
#include <VDAC_ref_BP_DMA_dma.H>



/****************************************************************************
* 
* The following defines are available in Cyfitter.h
* 
* 
* 
* VDAC_ref_BP_DMA__DRQ_CTL_REG
* 
* 
* VDAC_ref_BP_DMA__DRQ_NUMBER
* 
* Number of TD's used by this channel.
* VDAC_ref_BP_DMA__NUMBEROF_TDS
* 
* Priority of this channel.
* VDAC_ref_BP_DMA__PRIORITY
* 
* True if VDAC_ref_BP_DMA_TERMIN_SEL is used.
* VDAC_ref_BP_DMA__TERMIN_EN
* 
* TERMIN interrupt line to signal terminate.
* VDAC_ref_BP_DMA__TERMIN_SEL
* 
* 
* True if VDAC_ref_BP_DMA_TERMOUT0_SEL is used.
* VDAC_ref_BP_DMA__TERMOUT0_EN
* 
* 
* TERMOUT0 interrupt line to signal completion.
* VDAC_ref_BP_DMA__TERMOUT0_SEL
* 
* 
* True if VDAC_ref_BP_DMA_TERMOUT1_SEL is used.
* VDAC_ref_BP_DMA__TERMOUT1_EN
* 
* 
* TERMOUT1 interrupt line to signal completion.
* VDAC_ref_BP_DMA__TERMOUT1_SEL
* 
****************************************************************************/


/* Zero based index of VDAC_ref_BP_DMA dma channel */
uint8 VDAC_ref_BP_DMA_DmaHandle = DMA_INVALID_CHANNEL;

/*********************************************************************
* Function Name: uint8 VDAC_ref_BP_DMA_DmaInitalize
**********************************************************************
* Summary:
*   Allocates and initialises a channel of the DMAC to be used by the
*   caller.
*
* Parameters:
*   BurstCount.
*       
*       
*   ReqestPerBurst.
*       
*       
*   UpperSrcAddress.
*       
*       
*   UpperDestAddress.
*       
*
* Return:
*   The channel that can be used by the caller for DMA activity.
*   DMA_INVALID_CHANNEL (0xFF) if there are no channels left. 
*
*
*******************************************************************/
uint8 VDAC_ref_BP_DMA_DmaInitialize(uint8 BurstCount, uint8 ReqestPerBurst, uint16 UpperSrcAddress, uint16 UpperDestAddress) 
{

    /* Allocate a DMA channel. */
    VDAC_ref_BP_DMA_DmaHandle = (uint8)VDAC_ref_BP_DMA__DRQ_NUMBER;

    /* Configure the channel. */
    (void)CyDmaChSetConfiguration(VDAC_ref_BP_DMA_DmaHandle,
                                  BurstCount,
                                  ReqestPerBurst,
                                  (uint8)VDAC_ref_BP_DMA__TERMOUT0_SEL,
                                  (uint8)VDAC_ref_BP_DMA__TERMOUT1_SEL,
                                  (uint8)VDAC_ref_BP_DMA__TERMIN_SEL);

    /* Set the extended address for the transfers */
    (void)CyDmaChSetExtendedAddress(VDAC_ref_BP_DMA_DmaHandle, UpperSrcAddress, UpperDestAddress);

    /* Set the priority for this channel */
    (void)CyDmaChPriority(VDAC_ref_BP_DMA_DmaHandle, (uint8)VDAC_ref_BP_DMA__PRIORITY);
    
    return VDAC_ref_BP_DMA_DmaHandle;
}

/*********************************************************************
* Function Name: void VDAC_ref_BP_DMA_DmaRelease
**********************************************************************
* Summary:
*   Frees the channel associated with VDAC_ref_BP_DMA.
*
*
* Parameters:
*   void.
*
*
*
* Return:
*   void.
*
*******************************************************************/
void VDAC_ref_BP_DMA_DmaRelease(void) 
{
    /* Disable the channel */
    (void)CyDmaChDisable(VDAC_ref_BP_DMA_DmaHandle);
}

