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

#include "`$INSTANCE_NAME`.h"
#include "`$INSTANCE_NAME`_bypass.h"

_Bool `$INSTANCE_NAME`_initVar = 0u;
#define STATUS_BAD_PARAM 0x1
#define STATUS_GOOD 0x0

#define BYPASS 0 
#define LPF 1 

/* Module static var */
static uint8 rc_val = 0;
static `$INSTANCE_NAME`_cf_e fc_last = `$INSTANCE_NAME`_CORNER_FREQ_DFLT;	/* Set the the default value to start */
static uint8_t bypass_last = BYPASS;
/*--------------------------------------------------------------------
* uint8 `$INSTANCE_NAME`_Init(`$INSTANCE_NAME`_cf_e fc)
* 
* This is the initialization function.
* It is used for Start() and SetFc().
* 
* 	Parameters:
* 		fc = corner frequency enum value.
* 
* 	Returns:
* 		STATUS_GOOD = No error
* 		STATUS_BAD_PARAM = Bad fc parameter
*/
uint8 `$INSTANCE_NAME`_Init(`$INSTANCE_NAME`_cf_e fc)
{
uint8 result = STATUS_GOOD;

	switch(fc)
	{
	case `$INSTANCE_NAME`_CORNER_FREQ_15K:
		rc_val = (3<<`$INSTANCE_NAME`_RC_SH);
		break;
	case `$INSTANCE_NAME`_CORNER_FREQ_31K:
		rc_val = (1<<`$INSTANCE_NAME`_RC_SH);
		break;
	case `$INSTANCE_NAME`_CORNER_FREQ_77K:
		rc_val = (2<<`$INSTANCE_NAME`_RC_SH);
		break;
	case `$INSTANCE_NAME`_CORNER_FREQ_153K:
		rc_val = (0<<`$INSTANCE_NAME`_RC_SH);
		break;
	default:	/* invalid parameter */
		result = STATUS_BAD_PARAM;
		break;
	}
	if(result == STATUS_GOOD)
	{
		`$INSTANCE_NAME`_CR_REG = (`$INSTANCE_NAME`_CR_REG & ~(`$INSTANCE_NAME`_RC_MASK)) | rc_val;
		`$INSTANCE_NAME`_bypass_Start();
		fc_last = fc;
	}
	return (result);
}

/*--------------------------------------------------------------------
* void `$INSTANCE_NAME`_Start(void)
* 
* This is the Start function that calls the initialization with the last user  Fc enum value
* and Enables it.
* 
* 	Parameters:
* 		none
* 
* 	Returns:
* 		none
*/
void `$INSTANCE_NAME`_Start(void)
{
	`$INSTANCE_NAME`_Init(fc_last);	/* Initialize with the last fc enum value */
	`$INSTANCE_NAME`_initVar = 1u;
	`$INSTANCE_NAME`_Enable();
}

/*--------------------------------------------------------------------
* void `$INSTANCE_NAME`_Stop(void)
* 
* This issues `$INSTANCE_NAME`_Disable()
* 
* 	Parameters:
* 		none
* 
* 	Returns:
* 		none
*/
void `$INSTANCE_NAME`_Stop(void)
{
	`$INSTANCE_NAME`_Disable();
}

/*--------------------------------------------------------------------
* void `$INSTANCE_NAME`_Enable(void)
* 
* This is the Enable function sets the LPF amux to point to the LPF
* 
* 	Parameters:
* 		none
* 
* 	Returns:
* 		none
*/
void `$INSTANCE_NAME`_Enable(void)
{
	`$INSTANCE_NAME`_bypass_FastSelect(LPF);	// set Amux to LPF.ao
	bypass_last = LPF;
}


/*--------------------------------------------------------------------
* void `$INSTANCE_NAME`_Disable(void)
* 
* This is the Disable function sets the LPF amux to bypass around the LPF.
* 
* 	Parameters:
* 		none
* 
* 	Returns:
* 		none
*/
void `$INSTANCE_NAME`_Disable(void)
{
	`$INSTANCE_NAME`_bypass_FastSelect(BYPASS);	// set Amux to bypass
	bypass_last = BYPASS;
}

static char *fc_str[] =
{
	"15 KHz",
	"31 KHz",
	"77 KHz",
	"153 KHz",
	"Bypass",
};

/*--------------------------------------------------------------------
* void `$INSTANCE_NAME`_GetFc_Str(void)
* 
* This is the Disable function sets the LPF amux to bypass around the LPF.
* 
* 	Parameters:
* 		none
* 
* 	Returns:
* 		pointer to char of selected corner frequency string
*/
char *`$INSTANCE_NAME`_GetFc_Str(void)
{
	if(bypass_last == BYPASS)
	{	/* LPF bypassed */
		return(fc_str[4]);
	}
	else
	{	/* Corner frequency string */
		return(fc_str[fc_last]);		
	}
}

/* [] END OF FILE */
