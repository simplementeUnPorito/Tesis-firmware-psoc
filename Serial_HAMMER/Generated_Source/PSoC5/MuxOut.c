/*******************************************************************************
* File Name: MuxOut.c
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

#include "MuxOut.h"

static uint8 MuxOut_lastChannel = MuxOut_NULL_CHANNEL;


/*******************************************************************************
* Function Name: MuxOut_Start
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
void MuxOut_Start(void) 
{
    uint8 chan;

    for(chan = 0u; chan < MuxOut_CHANNELS ; chan++)
    {
#if (MuxOut_MUXTYPE == MuxOut_MUX_SINGLE)
        MuxOut_Unset(chan);
#else
        MuxOut_CYAMUXSIDE_A_Unset(chan);
        MuxOut_CYAMUXSIDE_B_Unset(chan);
#endif
    }

    MuxOut_lastChannel = MuxOut_NULL_CHANNEL;
}


#if (!MuxOut_ATMOSTONE)
/*******************************************************************************
* Function Name: MuxOut_Select
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
void MuxOut_Select(uint8 channel) 
{
    MuxOut_DisconnectAll();        /* Disconnect all previous connections */
    MuxOut_Connect(channel);       /* Make the given selection */
    MuxOut_lastChannel = channel;  /* Update last channel */
}
#endif


/*******************************************************************************
* Function Name: MuxOut_FastSelect
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
void MuxOut_FastSelect(uint8 channel) 
{
    /* Disconnect the last valid channel */
    if( MuxOut_lastChannel != MuxOut_NULL_CHANNEL)
    {
        MuxOut_Disconnect(MuxOut_lastChannel);
    }

    /* Make the new channel connection */
#if (MuxOut_MUXTYPE == MuxOut_MUX_SINGLE)
    MuxOut_Set(channel);
#else
    MuxOut_CYAMUXSIDE_A_Set(channel);
    MuxOut_CYAMUXSIDE_B_Set(channel);
#endif


    MuxOut_lastChannel = channel;   /* Update last channel */
}


#if (MuxOut_MUXTYPE == MuxOut_MUX_DIFF)
#if (!MuxOut_ATMOSTONE)
/*******************************************************************************
* Function Name: MuxOut_Connect
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
void MuxOut_Connect(uint8 channel) 
{
    MuxOut_CYAMUXSIDE_A_Set(channel);
    MuxOut_CYAMUXSIDE_B_Set(channel);
}
#endif

/*******************************************************************************
* Function Name: MuxOut_Disconnect
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
void MuxOut_Disconnect(uint8 channel) 
{
    MuxOut_CYAMUXSIDE_A_Unset(channel);
    MuxOut_CYAMUXSIDE_B_Unset(channel);
}
#endif

#if (MuxOut_ATMOSTONE)
/*******************************************************************************
* Function Name: MuxOut_DisconnectAll
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
void MuxOut_DisconnectAll(void) 
{
    if(MuxOut_lastChannel != MuxOut_NULL_CHANNEL) 
    {
        MuxOut_Disconnect(MuxOut_lastChannel);
        MuxOut_lastChannel = MuxOut_NULL_CHANNEL;
    }
}
#endif

/* [] END OF FILE */
