#ifndef CONTROL_RUNTIME_H
#define CONTROL_RUNTIME_H
#include "control_config.h"
#include "project.h"
enum { CONTROL_NEEDS_LEARNING, CONTROL_LEARNING, CONTROL_RUNNING,
       CONTROL_PAUSED, CONTROL_FAILED };
/* Extended commands retain the existing AB cmd p1 p2 XOR framing.
 * D0 selects parameter; D1..D4 stage its four LE bytes; D5 commits that
 * parameter into staging; D6 action; D7 queries an indexed metadata word.
 * No command applies half a parameter/configuration. */
#define CONTROL_CMD_FIRST 0xD0u
#define CONTROL_CMD_LAST  0xD7u
enum { CONTROL_APPLY=1, CONTROL_SAVE, CONTROL_DEFAULTS, CONTROL_LEARN,
       CONTROL_PAUSE, CONTROL_RESUME, CONTROL_REPORT };
void control_init(void);
void control_service(void);
void control_on_filtered_sample(int32 counts);
void control_prepare_capture(void);
void control_select_capture_path(void);
void control_resume(void);
uint8 control_command(uint8 cmd,uint8 p1,uint8 p2);
uint8 control_capture_channel(void);
void control_emit_snapshot(uint8 capture);
uint32 control_millis(void);
void control_tick(void);
void control_copy_capture_metadata(uint8 *out);
#define CONTROL_CAPTURE_METADATA_BYTES 320u
#endif
