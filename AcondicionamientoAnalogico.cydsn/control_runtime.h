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
/* Vista de diagnostico: fuerza el canal del AMux que se digitaliza, sin
 * tocar el lazo ni la configuracion guardada. PSOC_VIEW_CHANNEL_NONE (0xFF)
 * vuelve al canal de captura de la config. Devuelve 0 si el canal no existe
 * en esta placa. */
uint8 control_set_view_channel(uint8 channel);
/* Fuerza al lazo a corregir sin congelarse durante `seconds`. 0 cancela.
 * Devuelve 0 si el lazo no esta corriendo (pausado por captura o aprendiendo). */
uint8 control_force_pi(uint16 seconds);
/* Avisa al lazo que cambio una ganancia: el punto de trabajo se multiplico,
 * asi que hay que rehacerlo sin esperar a la histeresis ni al enfriamiento
 * del actuador grueso. */
/* `pga_changed` distingue las dos etapas: PGAout esta AGUAS ABAJO del sumador
 * y el par rapido lo alcanza; el PGA esta aguas arriba y su offset
 * multiplicado deja a SUMo fuera de su ventana de medicion, asi que hay que
 * volver a aprender el par lento. */
void control_on_gain_change(uint8 pga_code, uint8 pgaout_code, uint8 pga_changed);
void control_emit_snapshot(uint8 capture);
uint32 control_millis(void);
void control_tick(void);
void control_copy_capture_metadata(uint8 *out);
#define CONTROL_CAPTURE_METADATA_BYTES 320u
#endif
