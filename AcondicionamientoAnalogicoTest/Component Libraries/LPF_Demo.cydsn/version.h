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
*/
/*--------------------------------------------------
•	1.0.1 - Initial code.   
------------------------------------*/
#if !defined(VERSION_H)
// This is a sexy yet slightly confusing macro to turn any literal including numbers into a string literal
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
	
#define VERSION_H
#define SW_TITLE	CY_PROJECT_NAME

#define SW_VER_REV_REL	1.0.1
#endif		// VERSION_H
/* [] END OF FILE */
