/*******************************************************************************
* File Name: LPF_bypass.c
* Version 1.60
*
*  Description:
*    This file contains all functions required for the analog multiplexer
*    AMux User Module.
*
*   Note:
*
*******************************************************************************
* Copyright 2012-2013, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions, 
* disclaimers, and limitations in the end user license agreement accompanying 
* the software package with which this file was provided.
********************************************************************************/

#include "LPF_bypass.h"

uint8 LPF_bypass_initVar = 0u;
uint8 LPF_bypass_lastChannel = LPF_bypass_NULL_CHANNEL;


/*******************************************************************************
* Function Name: LPF_bypass_Start
********************************************************************************
* Summary:
*  Disconnect all channels.
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
void LPF_bypass_Start(void) 
{
    LPF_bypass_DisconnectAll();
    LPF_bypass_initVar = 1u;
}


/*******************************************************************************
* Function Name: LPF_bypass_Init
********************************************************************************
* Summary:
*  Disconnect all channels.
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
void LPF_bypass_Init(void) 
{
    LPF_bypass_DisconnectAll();
}


/*******************************************************************************
* Function Name: LPF_bypass_Stop
********************************************************************************
* Summary:
*  Disconnect all channels.
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
void LPF_bypass_Stop(void) 
{
    LPF_bypass_DisconnectAll();
}


/*******************************************************************************
* Function Name: LPF_bypass_Select
********************************************************************************
* Summary:
*  This functions first disconnects all channels then connects the given
*  channel.
*
* Parameters:
*  channel:  The channel to connect to the common terminal.
*
* Return:
*  void
*
*******************************************************************************/
void LPF_bypass_Select(uint8 channel) 
{
    LPF_bypass_DisconnectAll();        /* Disconnect all previous connections */
    LPF_bypass_Connect(channel);       /* Make the given selection */
    LPF_bypass_lastChannel = channel;  /* Update last channel */
}


/*******************************************************************************
* Function Name: LPF_bypass_FastSelect
********************************************************************************
* Summary:
*  This function first disconnects the last connection made with FastSelect or
*  Select, then connects the given channel. The FastSelect function is similar
*  to the Select function, except it is faster since it only disconnects the
*  last channel selected rather than all channels.
*
* Parameters:
*  channel:  The channel to connect to the common terminal.
*
* Return:
*  void
*
*******************************************************************************/
void LPF_bypass_FastSelect(uint8 channel) 
{
    /* Disconnect the last valid channel */
    if( LPF_bypass_lastChannel != LPF_bypass_NULL_CHANNEL)   /* Update last channel */
    {
        LPF_bypass_Disconnect(LPF_bypass_lastChannel);
    }

    /* Make the new channel connection */
    LPF_bypass_Connect(channel);
    LPF_bypass_lastChannel = channel;   /* Update last channel */
}


#if(LPF_bypass_MUXTYPE == LPF_bypass_MUX_DIFF)
/*******************************************************************************
* Function Name: LPF_bypass_Connect
********************************************************************************
* Summary:
*  This function connects the given channel without affecting other connections.
*
* Parameters:
*  channel:  The channel to connect to the common terminal.
*
* Return:
*  void
*
*******************************************************************************/
void LPF_bypass_Connect(uint8 channel) 
{
    LPF_bypass_CYAMUXSIDE_A_Set(channel);
    LPF_bypass_CYAMUXSIDE_B_Set(channel);
}


/*******************************************************************************
* Function Name: LPF_bypass_Disconnect
********************************************************************************
* Summary:
*  This function disconnects the given channel from the common or output
*  terminal without affecting other connections.
*
* Parameters:
*  channel:  The channel to disconnect from the common terminal.
*
* Return:
*  void
*
*******************************************************************************/
void LPF_bypass_Disconnect(uint8 channel) 
{
    LPF_bypass_CYAMUXSIDE_A_Unset(channel);
    LPF_bypass_CYAMUXSIDE_B_Unset(channel);
}
#endif /* (LPF_bypass_MUXTYPE == LPF_bypass_MUX_DIFF) */


/*******************************************************************************
* Function Name: LPF_bypass_DisconnectAll
********************************************************************************
* Summary:
*  This function disconnects all channels.
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
void LPF_bypass_DisconnectAll(void) 
{
    uint8 chan;

#if(LPF_bypass_MUXTYPE == LPF_bypass_MUX_SINGLE)
    for(chan = 0; chan < LPF_bypass_CHANNELS ; chan++)
    {
        LPF_bypass_Unset(chan);
    }
#else
    for(chan = 0; chan < LPF_bypass_CHANNELS ; chan++)
    {
        LPF_bypass_CYAMUXSIDE_A_Unset(chan);
        LPF_bypass_CYAMUXSIDE_B_Unset(chan);
    }
#endif /* (LPF_bypass_MUXTYPE == LPF_bypass_MUX_SINGLE) */
}


/* [] END OF FILE */
