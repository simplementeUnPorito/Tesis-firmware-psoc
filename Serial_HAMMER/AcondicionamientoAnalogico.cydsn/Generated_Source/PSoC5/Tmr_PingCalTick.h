/*******************************************************************************
* File Name: Tmr_PingCalTick.h
* Version 2.80
*
*  Description:
*     Contains the function prototypes and constants available to the timer
*     user module.
*
*   Note:
*     None
*
********************************************************************************
* Copyright 2008-2017, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
********************************************************************************/

#if !defined(CY_TIMER_Tmr_PingCalTick_H)
#define CY_TIMER_Tmr_PingCalTick_H

#include "cytypes.h"
#include "cyfitter.h"
#include "CyLib.h" /* For CyEnterCriticalSection() and CyExitCriticalSection() functions */

extern uint8 Tmr_PingCalTick_initVar;

/* Check to see if required defines such as CY_PSOC5LP are available */
/* They are defined starting with cy_boot v3.0 */
#if !defined (CY_PSOC5LP)
    #error Component Timer_v2_80 requires cy_boot v3.0 or later
#endif /* (CY_ PSOC5LP) */


/**************************************
*           Parameter Defaults
**************************************/

#define Tmr_PingCalTick_Resolution                 16u
#define Tmr_PingCalTick_UsingFixedFunction         1u
#define Tmr_PingCalTick_UsingHWCaptureCounter      0u
#define Tmr_PingCalTick_SoftwareCaptureMode        0u
#define Tmr_PingCalTick_SoftwareTriggerMode        0u
#define Tmr_PingCalTick_UsingHWEnable              0u
#define Tmr_PingCalTick_EnableTriggerMode          0u
#define Tmr_PingCalTick_InterruptOnCaptureCount    0u
#define Tmr_PingCalTick_RunModeUsed                1u
#define Tmr_PingCalTick_ControlRegRemoved          0u

#if defined(Tmr_PingCalTick_TimerUDB_sCTRLReg_SyncCtl_ctrlreg__CONTROL_REG)
    #define Tmr_PingCalTick_UDB_CONTROL_REG_REMOVED            (0u)
#elif  (Tmr_PingCalTick_UsingFixedFunction)
    #define Tmr_PingCalTick_UDB_CONTROL_REG_REMOVED            (0u)
#else 
    #define Tmr_PingCalTick_UDB_CONTROL_REG_REMOVED            (1u)
#endif /* End Tmr_PingCalTick_TimerUDB_sCTRLReg_SyncCtl_ctrlreg__CONTROL_REG */


/***************************************
*       Type defines
***************************************/


/**************************************************************************
 * Sleep Wakeup Backup structure for Timer Component
 *************************************************************************/
typedef struct
{
    uint8 TimerEnableState;
    #if(!Tmr_PingCalTick_UsingFixedFunction)

        uint16 TimerUdb;
        uint8 InterruptMaskValue;
        #if (Tmr_PingCalTick_UsingHWCaptureCounter)
            uint8 TimerCaptureCounter;
        #endif /* variable declarations for backing up non retention registers in CY_UDB_V1 */

        #if (!Tmr_PingCalTick_UDB_CONTROL_REG_REMOVED)
            uint8 TimerControlRegister;
        #endif /* variable declaration for backing up enable state of the Timer */
    #endif /* define backup variables only for UDB implementation. Fixed function registers are all retention */

}Tmr_PingCalTick_backupStruct;


/***************************************
*       Function Prototypes
***************************************/

void    Tmr_PingCalTick_Start(void) ;
void    Tmr_PingCalTick_Stop(void) ;

void    Tmr_PingCalTick_SetInterruptMode(uint8 interruptMode) ;
uint8   Tmr_PingCalTick_ReadStatusRegister(void) ;
/* Deprecated function. Do not use this in future. Retained for backward compatibility */
#define Tmr_PingCalTick_GetInterruptSource() Tmr_PingCalTick_ReadStatusRegister()

#if(!Tmr_PingCalTick_UDB_CONTROL_REG_REMOVED)
    uint8   Tmr_PingCalTick_ReadControlRegister(void) ;
    void    Tmr_PingCalTick_WriteControlRegister(uint8 control) ;
#endif /* (!Tmr_PingCalTick_UDB_CONTROL_REG_REMOVED) */

uint16  Tmr_PingCalTick_ReadPeriod(void) ;
void    Tmr_PingCalTick_WritePeriod(uint16 period) ;
uint16  Tmr_PingCalTick_ReadCounter(void) ;
void    Tmr_PingCalTick_WriteCounter(uint16 counter) ;
uint16  Tmr_PingCalTick_ReadCapture(void) ;
void    Tmr_PingCalTick_SoftwareCapture(void) ;

#if(!Tmr_PingCalTick_UsingFixedFunction) /* UDB Prototypes */
    #if (Tmr_PingCalTick_SoftwareCaptureMode)
        void    Tmr_PingCalTick_SetCaptureMode(uint8 captureMode) ;
    #endif /* (!Tmr_PingCalTick_UsingFixedFunction) */

    #if (Tmr_PingCalTick_SoftwareTriggerMode)
        void    Tmr_PingCalTick_SetTriggerMode(uint8 triggerMode) ;
    #endif /* (Tmr_PingCalTick_SoftwareTriggerMode) */

    #if (Tmr_PingCalTick_EnableTriggerMode)
        void    Tmr_PingCalTick_EnableTrigger(void) ;
        void    Tmr_PingCalTick_DisableTrigger(void) ;
    #endif /* (Tmr_PingCalTick_EnableTriggerMode) */


    #if(Tmr_PingCalTick_InterruptOnCaptureCount)
        void    Tmr_PingCalTick_SetInterruptCount(uint8 interruptCount) ;
    #endif /* (Tmr_PingCalTick_InterruptOnCaptureCount) */

    #if (Tmr_PingCalTick_UsingHWCaptureCounter)
        void    Tmr_PingCalTick_SetCaptureCount(uint8 captureCount) ;
        uint8   Tmr_PingCalTick_ReadCaptureCount(void) ;
    #endif /* (Tmr_PingCalTick_UsingHWCaptureCounter) */

    void Tmr_PingCalTick_ClearFIFO(void) ;
#endif /* UDB Prototypes */

/* Sleep Retention APIs */
void Tmr_PingCalTick_Init(void)          ;
void Tmr_PingCalTick_Enable(void)        ;
void Tmr_PingCalTick_SaveConfig(void)    ;
void Tmr_PingCalTick_RestoreConfig(void) ;
void Tmr_PingCalTick_Sleep(void)         ;
void Tmr_PingCalTick_Wakeup(void)        ;


/***************************************
*   Enumerated Types and Parameters
***************************************/

/* Enumerated Type B_Timer__CaptureModes, Used in Capture Mode */
#define Tmr_PingCalTick__B_TIMER__CM_NONE 0
#define Tmr_PingCalTick__B_TIMER__CM_RISINGEDGE 1
#define Tmr_PingCalTick__B_TIMER__CM_FALLINGEDGE 2
#define Tmr_PingCalTick__B_TIMER__CM_EITHEREDGE 3
#define Tmr_PingCalTick__B_TIMER__CM_SOFTWARE 4



/* Enumerated Type B_Timer__TriggerModes, Used in Trigger Mode */
#define Tmr_PingCalTick__B_TIMER__TM_NONE 0x00u
#define Tmr_PingCalTick__B_TIMER__TM_RISINGEDGE 0x04u
#define Tmr_PingCalTick__B_TIMER__TM_FALLINGEDGE 0x08u
#define Tmr_PingCalTick__B_TIMER__TM_EITHEREDGE 0x0Cu
#define Tmr_PingCalTick__B_TIMER__TM_SOFTWARE 0x10u


/***************************************
*    Initialial Parameter Constants
***************************************/

#define Tmr_PingCalTick_INIT_PERIOD             255u
#define Tmr_PingCalTick_INIT_CAPTURE_MODE       ((uint8)((uint8)1u << Tmr_PingCalTick_CTRL_CAP_MODE_SHIFT))
#define Tmr_PingCalTick_INIT_TRIGGER_MODE       ((uint8)((uint8)0u << Tmr_PingCalTick_CTRL_TRIG_MODE_SHIFT))
#if (Tmr_PingCalTick_UsingFixedFunction)
    #define Tmr_PingCalTick_INIT_INTERRUPT_MODE (((uint8)((uint8)0u << Tmr_PingCalTick_STATUS_TC_INT_MASK_SHIFT)) | \
                                                  ((uint8)((uint8)0 << Tmr_PingCalTick_STATUS_CAPTURE_INT_MASK_SHIFT)))
#else
    #define Tmr_PingCalTick_INIT_INTERRUPT_MODE (((uint8)((uint8)0u << Tmr_PingCalTick_STATUS_TC_INT_MASK_SHIFT)) | \
                                                 ((uint8)((uint8)0 << Tmr_PingCalTick_STATUS_CAPTURE_INT_MASK_SHIFT)) | \
                                                 ((uint8)((uint8)0 << Tmr_PingCalTick_STATUS_FIFOFULL_INT_MASK_SHIFT)))
#endif /* (Tmr_PingCalTick_UsingFixedFunction) */
#define Tmr_PingCalTick_INIT_CAPTURE_COUNT      (2u)
#define Tmr_PingCalTick_INIT_INT_CAPTURE_COUNT  ((uint8)((uint8)(1u - 1u) << Tmr_PingCalTick_CTRL_INTCNT_SHIFT))


/***************************************
*           Registers
***************************************/

#if (Tmr_PingCalTick_UsingFixedFunction) /* Implementation Specific Registers and Register Constants */


    /***************************************
    *    Fixed Function Registers
    ***************************************/

    #define Tmr_PingCalTick_STATUS         (*(reg8 *) Tmr_PingCalTick_TimerHW__SR0 )
    /* In Fixed Function Block Status and Mask are the same register */
    #define Tmr_PingCalTick_STATUS_MASK    (*(reg8 *) Tmr_PingCalTick_TimerHW__SR0 )
    #define Tmr_PingCalTick_CONTROL        (*(reg8 *) Tmr_PingCalTick_TimerHW__CFG0)
    #define Tmr_PingCalTick_CONTROL2       (*(reg8 *) Tmr_PingCalTick_TimerHW__CFG1)
    #define Tmr_PingCalTick_CONTROL2_PTR   ( (reg8 *) Tmr_PingCalTick_TimerHW__CFG1)
    #define Tmr_PingCalTick_RT1            (*(reg8 *) Tmr_PingCalTick_TimerHW__RT1)
    #define Tmr_PingCalTick_RT1_PTR        ( (reg8 *) Tmr_PingCalTick_TimerHW__RT1)

    #if (CY_PSOC3 || CY_PSOC5LP)
        #define Tmr_PingCalTick_CONTROL3       (*(reg8 *) Tmr_PingCalTick_TimerHW__CFG2)
        #define Tmr_PingCalTick_CONTROL3_PTR   ( (reg8 *) Tmr_PingCalTick_TimerHW__CFG2)
    #endif /* (CY_PSOC3 || CY_PSOC5LP) */
    #define Tmr_PingCalTick_GLOBAL_ENABLE  (*(reg8 *) Tmr_PingCalTick_TimerHW__PM_ACT_CFG)
    #define Tmr_PingCalTick_GLOBAL_STBY_ENABLE  (*(reg8 *) Tmr_PingCalTick_TimerHW__PM_STBY_CFG)

    #define Tmr_PingCalTick_CAPTURE_LSB         (* (reg16 *) Tmr_PingCalTick_TimerHW__CAP0 )
    #define Tmr_PingCalTick_CAPTURE_LSB_PTR       ((reg16 *) Tmr_PingCalTick_TimerHW__CAP0 )
    #define Tmr_PingCalTick_PERIOD_LSB          (* (reg16 *) Tmr_PingCalTick_TimerHW__PER0 )
    #define Tmr_PingCalTick_PERIOD_LSB_PTR        ((reg16 *) Tmr_PingCalTick_TimerHW__PER0 )
    #define Tmr_PingCalTick_COUNTER_LSB         (* (reg16 *) Tmr_PingCalTick_TimerHW__CNT_CMP0 )
    #define Tmr_PingCalTick_COUNTER_LSB_PTR       ((reg16 *) Tmr_PingCalTick_TimerHW__CNT_CMP0 )


    /***************************************
    *    Register Constants
    ***************************************/

    /* Fixed Function Block Chosen */
    #define Tmr_PingCalTick_BLOCK_EN_MASK                     Tmr_PingCalTick_TimerHW__PM_ACT_MSK
    #define Tmr_PingCalTick_BLOCK_STBY_EN_MASK                Tmr_PingCalTick_TimerHW__PM_STBY_MSK

    /* Control Register Bit Locations */
    /* Interrupt Count - Not valid for Fixed Function Block */
    #define Tmr_PingCalTick_CTRL_INTCNT_SHIFT                  0x00u
    /* Trigger Polarity - Not valid for Fixed Function Block */
    #define Tmr_PingCalTick_CTRL_TRIG_MODE_SHIFT               0x00u
    /* Trigger Enable - Not valid for Fixed Function Block */
    #define Tmr_PingCalTick_CTRL_TRIG_EN_SHIFT                 0x00u
    /* Capture Polarity - Not valid for Fixed Function Block */
    #define Tmr_PingCalTick_CTRL_CAP_MODE_SHIFT                0x00u
    /* Timer Enable - As defined in Register Map, part of TMRX_CFG0 register */
    #define Tmr_PingCalTick_CTRL_ENABLE_SHIFT                  0x00u

    /* Control Register Bit Masks */
    #define Tmr_PingCalTick_CTRL_ENABLE                        ((uint8)((uint8)0x01u << Tmr_PingCalTick_CTRL_ENABLE_SHIFT))

    /* Control2 Register Bit Masks */
    /* As defined in Register Map, Part of the TMRX_CFG1 register */
    #define Tmr_PingCalTick_CTRL2_IRQ_SEL_SHIFT                 0x00u
    #define Tmr_PingCalTick_CTRL2_IRQ_SEL                      ((uint8)((uint8)0x01u << Tmr_PingCalTick_CTRL2_IRQ_SEL_SHIFT))

    #if (CY_PSOC5A)
        /* Use CFG1 Mode bits to set run mode */
        /* As defined by Verilog Implementation */
        #define Tmr_PingCalTick_CTRL_MODE_SHIFT                 0x01u
        #define Tmr_PingCalTick_CTRL_MODE_MASK                 ((uint8)((uint8)0x07u << Tmr_PingCalTick_CTRL_MODE_SHIFT))
    #endif /* (CY_PSOC5A) */
    #if (CY_PSOC3 || CY_PSOC5LP)
        /* Control3 Register Bit Locations */
        #define Tmr_PingCalTick_CTRL_RCOD_SHIFT        0x02u
        #define Tmr_PingCalTick_CTRL_ENBL_SHIFT        0x00u
        #define Tmr_PingCalTick_CTRL_MODE_SHIFT        0x00u

        /* Control3 Register Bit Masks */
        #define Tmr_PingCalTick_CTRL_RCOD_MASK  ((uint8)((uint8)0x03u << Tmr_PingCalTick_CTRL_RCOD_SHIFT)) /* ROD and COD bit masks */
        #define Tmr_PingCalTick_CTRL_ENBL_MASK  ((uint8)((uint8)0x80u << Tmr_PingCalTick_CTRL_ENBL_SHIFT)) /* HW_EN bit mask */
        #define Tmr_PingCalTick_CTRL_MODE_MASK  ((uint8)((uint8)0x03u << Tmr_PingCalTick_CTRL_MODE_SHIFT)) /* Run mode bit mask */

        #define Tmr_PingCalTick_CTRL_RCOD       ((uint8)((uint8)0x03u << Tmr_PingCalTick_CTRL_RCOD_SHIFT))
        #define Tmr_PingCalTick_CTRL_ENBL       ((uint8)((uint8)0x80u << Tmr_PingCalTick_CTRL_ENBL_SHIFT))
    #endif /* (CY_PSOC3 || CY_PSOC5LP) */

    /*RT1 Synch Constants: Applicable for PSoC3 and PSoC5LP */
    #define Tmr_PingCalTick_RT1_SHIFT                       0x04u
    /* Sync TC and CMP bit masks */
    #define Tmr_PingCalTick_RT1_MASK                        ((uint8)((uint8)0x03u << Tmr_PingCalTick_RT1_SHIFT))
    #define Tmr_PingCalTick_SYNC                            ((uint8)((uint8)0x03u << Tmr_PingCalTick_RT1_SHIFT))
    #define Tmr_PingCalTick_SYNCDSI_SHIFT                   0x00u
    /* Sync all DSI inputs with Mask  */
    #define Tmr_PingCalTick_SYNCDSI_MASK                    ((uint8)((uint8)0x0Fu << Tmr_PingCalTick_SYNCDSI_SHIFT))
    /* Sync all DSI inputs */
    #define Tmr_PingCalTick_SYNCDSI_EN                      ((uint8)((uint8)0x0Fu << Tmr_PingCalTick_SYNCDSI_SHIFT))

    #define Tmr_PingCalTick_CTRL_MODE_PULSEWIDTH            ((uint8)((uint8)0x01u << Tmr_PingCalTick_CTRL_MODE_SHIFT))
    #define Tmr_PingCalTick_CTRL_MODE_PERIOD                ((uint8)((uint8)0x02u << Tmr_PingCalTick_CTRL_MODE_SHIFT))
    #define Tmr_PingCalTick_CTRL_MODE_CONTINUOUS            ((uint8)((uint8)0x00u << Tmr_PingCalTick_CTRL_MODE_SHIFT))

    /* Status Register Bit Locations */
    /* As defined in Register Map, part of TMRX_SR0 register */
    #define Tmr_PingCalTick_STATUS_TC_SHIFT                 0x07u
    /* As defined in Register Map, part of TMRX_SR0 register, Shared with Compare Status */
    #define Tmr_PingCalTick_STATUS_CAPTURE_SHIFT            0x06u
    /* As defined in Register Map, part of TMRX_SR0 register */
    #define Tmr_PingCalTick_STATUS_TC_INT_MASK_SHIFT        (Tmr_PingCalTick_STATUS_TC_SHIFT - 0x04u)
    /* As defined in Register Map, part of TMRX_SR0 register, Shared with Compare Status */
    #define Tmr_PingCalTick_STATUS_CAPTURE_INT_MASK_SHIFT   (Tmr_PingCalTick_STATUS_CAPTURE_SHIFT - 0x04u)

    /* Status Register Bit Masks */
    #define Tmr_PingCalTick_STATUS_TC                       ((uint8)((uint8)0x01u << Tmr_PingCalTick_STATUS_TC_SHIFT))
    #define Tmr_PingCalTick_STATUS_CAPTURE                  ((uint8)((uint8)0x01u << Tmr_PingCalTick_STATUS_CAPTURE_SHIFT))
    /* Interrupt Enable Bit-Mask for interrupt on TC */
    #define Tmr_PingCalTick_STATUS_TC_INT_MASK              ((uint8)((uint8)0x01u << Tmr_PingCalTick_STATUS_TC_INT_MASK_SHIFT))
    /* Interrupt Enable Bit-Mask for interrupt on Capture */
    #define Tmr_PingCalTick_STATUS_CAPTURE_INT_MASK         ((uint8)((uint8)0x01u << Tmr_PingCalTick_STATUS_CAPTURE_INT_MASK_SHIFT))

#else   /* UDB Registers and Register Constants */


    /***************************************
    *           UDB Registers
    ***************************************/

    #define Tmr_PingCalTick_STATUS              (* (reg8 *) Tmr_PingCalTick_TimerUDB_rstSts_stsreg__STATUS_REG )
    #define Tmr_PingCalTick_STATUS_MASK         (* (reg8 *) Tmr_PingCalTick_TimerUDB_rstSts_stsreg__MASK_REG)
    #define Tmr_PingCalTick_STATUS_AUX_CTRL     (* (reg8 *) Tmr_PingCalTick_TimerUDB_rstSts_stsreg__STATUS_AUX_CTL_REG)
    #define Tmr_PingCalTick_CONTROL             (* (reg8 *) Tmr_PingCalTick_TimerUDB_sCTRLReg_SyncCtl_ctrlreg__CONTROL_REG )
    
    #if(Tmr_PingCalTick_Resolution <= 8u) /* 8-bit Timer */
        #define Tmr_PingCalTick_CAPTURE_LSB         (* (reg8 *) Tmr_PingCalTick_TimerUDB_sT16_timerdp_u0__F0_REG )
        #define Tmr_PingCalTick_CAPTURE_LSB_PTR       ((reg8 *) Tmr_PingCalTick_TimerUDB_sT16_timerdp_u0__F0_REG )
        #define Tmr_PingCalTick_PERIOD_LSB          (* (reg8 *) Tmr_PingCalTick_TimerUDB_sT16_timerdp_u0__D0_REG )
        #define Tmr_PingCalTick_PERIOD_LSB_PTR        ((reg8 *) Tmr_PingCalTick_TimerUDB_sT16_timerdp_u0__D0_REG )
        #define Tmr_PingCalTick_COUNTER_LSB         (* (reg8 *) Tmr_PingCalTick_TimerUDB_sT16_timerdp_u0__A0_REG )
        #define Tmr_PingCalTick_COUNTER_LSB_PTR       ((reg8 *) Tmr_PingCalTick_TimerUDB_sT16_timerdp_u0__A0_REG )
    #elif(Tmr_PingCalTick_Resolution <= 16u) /* 8-bit Timer */
        #if(CY_PSOC3) /* 8-bit addres space */
            #define Tmr_PingCalTick_CAPTURE_LSB         (* (reg16 *) Tmr_PingCalTick_TimerUDB_sT16_timerdp_u0__F0_REG )
            #define Tmr_PingCalTick_CAPTURE_LSB_PTR       ((reg16 *) Tmr_PingCalTick_TimerUDB_sT16_timerdp_u0__F0_REG )
            #define Tmr_PingCalTick_PERIOD_LSB          (* (reg16 *) Tmr_PingCalTick_TimerUDB_sT16_timerdp_u0__D0_REG )
            #define Tmr_PingCalTick_PERIOD_LSB_PTR        ((reg16 *) Tmr_PingCalTick_TimerUDB_sT16_timerdp_u0__D0_REG )
            #define Tmr_PingCalTick_COUNTER_LSB         (* (reg16 *) Tmr_PingCalTick_TimerUDB_sT16_timerdp_u0__A0_REG )
            #define Tmr_PingCalTick_COUNTER_LSB_PTR       ((reg16 *) Tmr_PingCalTick_TimerUDB_sT16_timerdp_u0__A0_REG )
        #else /* 16-bit address space */
            #define Tmr_PingCalTick_CAPTURE_LSB         (* (reg16 *) Tmr_PingCalTick_TimerUDB_sT16_timerdp_u0__16BIT_F0_REG )
            #define Tmr_PingCalTick_CAPTURE_LSB_PTR       ((reg16 *) Tmr_PingCalTick_TimerUDB_sT16_timerdp_u0__16BIT_F0_REG )
            #define Tmr_PingCalTick_PERIOD_LSB          (* (reg16 *) Tmr_PingCalTick_TimerUDB_sT16_timerdp_u0__16BIT_D0_REG )
            #define Tmr_PingCalTick_PERIOD_LSB_PTR        ((reg16 *) Tmr_PingCalTick_TimerUDB_sT16_timerdp_u0__16BIT_D0_REG )
            #define Tmr_PingCalTick_COUNTER_LSB         (* (reg16 *) Tmr_PingCalTick_TimerUDB_sT16_timerdp_u0__16BIT_A0_REG )
            #define Tmr_PingCalTick_COUNTER_LSB_PTR       ((reg16 *) Tmr_PingCalTick_TimerUDB_sT16_timerdp_u0__16BIT_A0_REG )
        #endif /* CY_PSOC3 */
    #elif(Tmr_PingCalTick_Resolution <= 24u)/* 24-bit Timer */
        #define Tmr_PingCalTick_CAPTURE_LSB         (* (reg32 *) Tmr_PingCalTick_TimerUDB_sT16_timerdp_u0__F0_REG )
        #define Tmr_PingCalTick_CAPTURE_LSB_PTR       ((reg32 *) Tmr_PingCalTick_TimerUDB_sT16_timerdp_u0__F0_REG )
        #define Tmr_PingCalTick_PERIOD_LSB          (* (reg32 *) Tmr_PingCalTick_TimerUDB_sT16_timerdp_u0__D0_REG )
        #define Tmr_PingCalTick_PERIOD_LSB_PTR        ((reg32 *) Tmr_PingCalTick_TimerUDB_sT16_timerdp_u0__D0_REG )
        #define Tmr_PingCalTick_COUNTER_LSB         (* (reg32 *) Tmr_PingCalTick_TimerUDB_sT16_timerdp_u0__A0_REG )
        #define Tmr_PingCalTick_COUNTER_LSB_PTR       ((reg32 *) Tmr_PingCalTick_TimerUDB_sT16_timerdp_u0__A0_REG )
    #else /* 32-bit Timer */
        #if(CY_PSOC3 || CY_PSOC5) /* 8-bit address space */
            #define Tmr_PingCalTick_CAPTURE_LSB         (* (reg32 *) Tmr_PingCalTick_TimerUDB_sT16_timerdp_u0__F0_REG )
            #define Tmr_PingCalTick_CAPTURE_LSB_PTR       ((reg32 *) Tmr_PingCalTick_TimerUDB_sT16_timerdp_u0__F0_REG )
            #define Tmr_PingCalTick_PERIOD_LSB          (* (reg32 *) Tmr_PingCalTick_TimerUDB_sT16_timerdp_u0__D0_REG )
            #define Tmr_PingCalTick_PERIOD_LSB_PTR        ((reg32 *) Tmr_PingCalTick_TimerUDB_sT16_timerdp_u0__D0_REG )
            #define Tmr_PingCalTick_COUNTER_LSB         (* (reg32 *) Tmr_PingCalTick_TimerUDB_sT16_timerdp_u0__A0_REG )
            #define Tmr_PingCalTick_COUNTER_LSB_PTR       ((reg32 *) Tmr_PingCalTick_TimerUDB_sT16_timerdp_u0__A0_REG )
        #else /* 32-bit address space */
            #define Tmr_PingCalTick_CAPTURE_LSB         (* (reg32 *) Tmr_PingCalTick_TimerUDB_sT16_timerdp_u0__32BIT_F0_REG )
            #define Tmr_PingCalTick_CAPTURE_LSB_PTR       ((reg32 *) Tmr_PingCalTick_TimerUDB_sT16_timerdp_u0__32BIT_F0_REG )
            #define Tmr_PingCalTick_PERIOD_LSB          (* (reg32 *) Tmr_PingCalTick_TimerUDB_sT16_timerdp_u0__32BIT_D0_REG )
            #define Tmr_PingCalTick_PERIOD_LSB_PTR        ((reg32 *) Tmr_PingCalTick_TimerUDB_sT16_timerdp_u0__32BIT_D0_REG )
            #define Tmr_PingCalTick_COUNTER_LSB         (* (reg32 *) Tmr_PingCalTick_TimerUDB_sT16_timerdp_u0__32BIT_A0_REG )
            #define Tmr_PingCalTick_COUNTER_LSB_PTR       ((reg32 *) Tmr_PingCalTick_TimerUDB_sT16_timerdp_u0__32BIT_A0_REG )
        #endif /* CY_PSOC3 || CY_PSOC5 */ 
    #endif

    #define Tmr_PingCalTick_COUNTER_LSB_PTR_8BIT       ((reg8 *) Tmr_PingCalTick_TimerUDB_sT16_timerdp_u0__A0_REG )
    
    #if (Tmr_PingCalTick_UsingHWCaptureCounter)
        #define Tmr_PingCalTick_CAP_COUNT              (*(reg8 *) Tmr_PingCalTick_TimerUDB_sCapCount_counter__PERIOD_REG )
        #define Tmr_PingCalTick_CAP_COUNT_PTR          ( (reg8 *) Tmr_PingCalTick_TimerUDB_sCapCount_counter__PERIOD_REG )
        #define Tmr_PingCalTick_CAPTURE_COUNT_CTRL     (*(reg8 *) Tmr_PingCalTick_TimerUDB_sCapCount_counter__CONTROL_AUX_CTL_REG )
        #define Tmr_PingCalTick_CAPTURE_COUNT_CTRL_PTR ( (reg8 *) Tmr_PingCalTick_TimerUDB_sCapCount_counter__CONTROL_AUX_CTL_REG )
    #endif /* (Tmr_PingCalTick_UsingHWCaptureCounter) */


    /***************************************
    *       Register Constants
    ***************************************/

    /* Control Register Bit Locations */
    #define Tmr_PingCalTick_CTRL_INTCNT_SHIFT              0x00u       /* As defined by Verilog Implementation */
    #define Tmr_PingCalTick_CTRL_TRIG_MODE_SHIFT           0x02u       /* As defined by Verilog Implementation */
    #define Tmr_PingCalTick_CTRL_TRIG_EN_SHIFT             0x04u       /* As defined by Verilog Implementation */
    #define Tmr_PingCalTick_CTRL_CAP_MODE_SHIFT            0x05u       /* As defined by Verilog Implementation */
    #define Tmr_PingCalTick_CTRL_ENABLE_SHIFT              0x07u       /* As defined by Verilog Implementation */

    /* Control Register Bit Masks */
    #define Tmr_PingCalTick_CTRL_INTCNT_MASK               ((uint8)((uint8)0x03u << Tmr_PingCalTick_CTRL_INTCNT_SHIFT))
    #define Tmr_PingCalTick_CTRL_TRIG_MODE_MASK            ((uint8)((uint8)0x03u << Tmr_PingCalTick_CTRL_TRIG_MODE_SHIFT))
    #define Tmr_PingCalTick_CTRL_TRIG_EN                   ((uint8)((uint8)0x01u << Tmr_PingCalTick_CTRL_TRIG_EN_SHIFT))
    #define Tmr_PingCalTick_CTRL_CAP_MODE_MASK             ((uint8)((uint8)0x03u << Tmr_PingCalTick_CTRL_CAP_MODE_SHIFT))
    #define Tmr_PingCalTick_CTRL_ENABLE                    ((uint8)((uint8)0x01u << Tmr_PingCalTick_CTRL_ENABLE_SHIFT))

    /* Bit Counter (7-bit) Control Register Bit Definitions */
    /* As defined by the Register map for the AUX Control Register */
    #define Tmr_PingCalTick_CNTR_ENABLE                    0x20u

    /* Status Register Bit Locations */
    #define Tmr_PingCalTick_STATUS_TC_SHIFT                0x00u  /* As defined by Verilog Implementation */
    #define Tmr_PingCalTick_STATUS_CAPTURE_SHIFT           0x01u  /* As defined by Verilog Implementation */
    #define Tmr_PingCalTick_STATUS_TC_INT_MASK_SHIFT       Tmr_PingCalTick_STATUS_TC_SHIFT
    #define Tmr_PingCalTick_STATUS_CAPTURE_INT_MASK_SHIFT  Tmr_PingCalTick_STATUS_CAPTURE_SHIFT
    #define Tmr_PingCalTick_STATUS_FIFOFULL_SHIFT          0x02u  /* As defined by Verilog Implementation */
    #define Tmr_PingCalTick_STATUS_FIFONEMP_SHIFT          0x03u  /* As defined by Verilog Implementation */
    #define Tmr_PingCalTick_STATUS_FIFOFULL_INT_MASK_SHIFT Tmr_PingCalTick_STATUS_FIFOFULL_SHIFT

    /* Status Register Bit Masks */
    /* Sticky TC Event Bit-Mask */
    #define Tmr_PingCalTick_STATUS_TC                      ((uint8)((uint8)0x01u << Tmr_PingCalTick_STATUS_TC_SHIFT))
    /* Sticky Capture Event Bit-Mask */
    #define Tmr_PingCalTick_STATUS_CAPTURE                 ((uint8)((uint8)0x01u << Tmr_PingCalTick_STATUS_CAPTURE_SHIFT))
    /* Interrupt Enable Bit-Mask */
    #define Tmr_PingCalTick_STATUS_TC_INT_MASK             ((uint8)((uint8)0x01u << Tmr_PingCalTick_STATUS_TC_SHIFT))
    /* Interrupt Enable Bit-Mask */
    #define Tmr_PingCalTick_STATUS_CAPTURE_INT_MASK        ((uint8)((uint8)0x01u << Tmr_PingCalTick_STATUS_CAPTURE_SHIFT))
    /* NOT-Sticky FIFO Full Bit-Mask */
    #define Tmr_PingCalTick_STATUS_FIFOFULL                ((uint8)((uint8)0x01u << Tmr_PingCalTick_STATUS_FIFOFULL_SHIFT))
    /* NOT-Sticky FIFO Not Empty Bit-Mask */
    #define Tmr_PingCalTick_STATUS_FIFONEMP                ((uint8)((uint8)0x01u << Tmr_PingCalTick_STATUS_FIFONEMP_SHIFT))
    /* Interrupt Enable Bit-Mask */
    #define Tmr_PingCalTick_STATUS_FIFOFULL_INT_MASK       ((uint8)((uint8)0x01u << Tmr_PingCalTick_STATUS_FIFOFULL_SHIFT))

    #define Tmr_PingCalTick_STATUS_ACTL_INT_EN             0x10u   /* As defined for the ACTL Register */

    /* Datapath Auxillary Control Register definitions */
    #define Tmr_PingCalTick_AUX_CTRL_FIFO0_CLR             0x01u   /* As defined by Register map */
    #define Tmr_PingCalTick_AUX_CTRL_FIFO1_CLR             0x02u   /* As defined by Register map */
    #define Tmr_PingCalTick_AUX_CTRL_FIFO0_LVL             0x04u   /* As defined by Register map */
    #define Tmr_PingCalTick_AUX_CTRL_FIFO1_LVL             0x08u   /* As defined by Register map */
    #define Tmr_PingCalTick_STATUS_ACTL_INT_EN_MASK        0x10u   /* As defined for the ACTL Register */

#endif /* Implementation Specific Registers and Register Constants */

#endif  /* CY_TIMER_Tmr_PingCalTick_H */


/* [] END OF FILE */
