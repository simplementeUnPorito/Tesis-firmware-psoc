/*******************************************************************************
* File Name: MuxIn.c
* Version 1.80
*
*  Description:
*    This file contains all functions required for the analog multiplexer
*    AMux User Module.
*
*   Note:
*
*******************************************************************************
* Copyright 2008-2010, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions, 
* disclaimers, and limitations in the end user license agreement accompanying 
* the software package with which this file was provided.
********************************************************************************/

#include "MuxIn.h"

static uint8 MuxIn_lastChannel = MuxIn_NULL_CHANNEL;


/*******************************************************************************
* Function Name: MuxIn_Start
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
void MuxIn_Start(void) 
{
    uint8 chan;

    for(chan = 0u; chan < MuxIn_CHANNELS ; chan++)
    {
#if (MuxIn_MUXTYPE == MuxIn_MUX_SINGLE)
        MuxIn_Unset(chan);
#else
        MuxIn_CYAMUXSIDE_A_Unset(chan);
        MuxIn_CYAMUXSIDE_B_Unset(chan);
#endif
    }

    MuxIn_lastChannel = MuxIn_NULL_CHANNEL;
}


#if (!MuxIn_ATMOSTONE)
/*******************************************************************************
* Function Name: MuxIn_Select
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
void MuxIn_Select(uint8 channel) 
{
    MuxIn_DisconnectAll();        /* Disconnect all previous connections */
    MuxIn_Connect(channel);       /* Make the given selection */
    MuxIn_lastChannel = channel;  /* Update last channel */
}
#endif


/*******************************************************************************
* Function Name: MuxIn_FastSelect
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
void MuxIn_FastSelect(uint8 channel) 
{
    /* Disconnect the last valid channel */
    if( MuxIn_lastChannel != MuxIn_NULL_CHANNEL)
    {
        MuxIn_Disconnect(MuxIn_lastChannel);
    }

    /* Make the new channel connection */
#if (MuxIn_MUXTYPE == MuxIn_MUX_SINGLE)
    MuxIn_Set(channel);
#else
    MuxIn_CYAMUXSIDE_A_Set(channel);
    MuxIn_CYAMUXSIDE_B_Set(channel);
#endif


    MuxIn_lastChannel = channel;   /* Update last channel */
}


#if (MuxIn_MUXTYPE == MuxIn_MUX_DIFF)
#if (!MuxIn_ATMOSTONE)
/*******************************************************************************
* Function Name: MuxIn_Connect
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
void MuxIn_Connect(uint8 channel) 
{
    MuxIn_CYAMUXSIDE_A_Set(channel);
    MuxIn_CYAMUXSIDE_B_Set(channel);
}
#endif

/*******************************************************************************
* Function Name: MuxIn_Disconnect
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
void MuxIn_Disconnect(uint8 channel) 
{
    MuxIn_CYAMUXSIDE_A_Unset(channel);
    MuxIn_CYAMUXSIDE_B_Unset(channel);
}
#endif

#if (MuxIn_ATMOSTONE)
/*******************************************************************************
* Function Name: MuxIn_DisconnectAll
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
void MuxIn_DisconnectAll(void) 
{
    if(MuxIn_lastChannel != MuxIn_NULL_CHANNEL) 
    {
        MuxIn_Disconnect(MuxIn_lastChannel);
        MuxIn_lastChannel = MuxIn_NULL_CHANNEL;
    }
}
#endif

/* [] END OF FILE */
