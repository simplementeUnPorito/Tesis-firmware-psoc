/* ========================================
 *
 * Copyright CONSULTRON, 2020
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONSULTRON allows PUBLIC use of the file.
 * CONSULTRON provides NO WARRANTY expressed or implied.
 *
 * ========================================
Module: `$INSTANCE_NAME`.c
Module Build: `$CY_MAJOR_VERSION`.`$CY_MINOR_VERSION`.1
Module Comment: `$CY_COMMENT`
*/

#if !defined(`$INSTANCE_NAME`_H)	// Only allow if this header is not already defined.
#define `$INSTANCE_NAME`_H				// define that this files has been called already in the precompile phase

#include "cyfitter.h"
#include "cytypes.h"
#include "CyLib.h"

/* Register defs */
#define `$INSTANCE_NAME`_CR_REG 	(* (reg8 *) `$INSTANCE_NAME`_lpf__CR0)
#define `$INSTANCE_NAME`_CR_PTR 	(  (reg8 *) `$INSTANCE_NAME`_lpf__CR0)

/* RC (Resistor and Cap) bit defs */
#define `$INSTANCE_NAME`_RC_SH	4
#define `$INSTANCE_NAME`_RC_MASK	(3<<`$INSTANCE_NAME`_RC_SH)

/* SWIN (Switch input) bit defs */
#define `$INSTANCE_NAME`_SWIN_NC	0
#define `$INSTANCE_NAME`_SWIN_AG	1
#define `$INSTANCE_NAME`_SWIN_AMX	2
#define `$INSTANCE_NAME`_SWIN_AG_AMX	3
#define `$INSTANCE_NAME`_SWIN_SH	0
#define `$INSTANCE_NAME`_SWIN_MASK	(3<<`$INSTANCE_NAME`_SWIN_SH)

/* SWOUT (Switch output) bit defs */
#define `$INSTANCE_NAME`_SWOUT_NC	0
#define `$INSTANCE_NAME`_SWOUT_ABUS	1
#define `$INSTANCE_NAME`_SWOUT_SH	2
#define `$INSTANCE_NAME`_SWOUT_MASK	(1<<`$INSTANCE_NAME`_SWOUT_SH)

#define `$INSTANCE_NAME`_CORNER_FREQ_DFLT_STRG	"`$cf_s`"	/* User defined default value string */
#define `$INSTANCE_NAME`_CORNER_FREQ_DFLT	`$Fc`	/* User defined default enum value */
/* Corner frequency enum values */
typedef enum
{
	`$INSTANCE_NAME`_CORNER_FREQ_15K=0,
	`$INSTANCE_NAME`_CORNER_FREQ_31K=1,
	`$INSTANCE_NAME`_CORNER_FREQ_77K=2,
	`$INSTANCE_NAME`_CORNER_FREQ_153K=3,
} `$INSTANCE_NAME`_cf_e;

/* Function declarations */
uint8 `$INSTANCE_NAME`_Init(`$INSTANCE_NAME`_cf_e fc);
void `$INSTANCE_NAME`_Start(void);
void `$INSTANCE_NAME`_Stop(void);
void `$INSTANCE_NAME`_Enable(void);
void `$INSTANCE_NAME`_Disable(void);
char *`$INSTANCE_NAME`_GetFc_Str(void);

/* macros */
#define `$INSTANCE_NAME`_SetFc(fc) `$INSTANCE_NAME`_Init(fc)	/* Alias for Init() */

#endif 		// #if !defined(`$INSTANCE_NAME`_H)
/* [] END OF FILE */
