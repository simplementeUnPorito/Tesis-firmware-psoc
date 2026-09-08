/* Driver SD en modo SPI sobre el componente SPI Master del TopDesign (SPIp:
 * CS=P2.3, SCK=P2.4, MOSI=P2.5, MISO=P2.6). Expone bloques de 512 bytes a
 * FatFs; las capturas nunca escriben LBAs fijos ni pisan la tabla FAT.
 *
 * El CS del TopDesign quedó cableado al `ss` de hardware del SPIM, que sube
 * entre bytes — inservible para el protocolo SD. sd_spi_init() lo desengancha
 * del DSI vía el registro de bypass del puerto (mismo truco ya validado en
 * tx1_gpio_detach_dsi de main.c) y lo maneja por software.
 *
 * Reglas de integración con la política de eventos determinismo-primero:
 *  - Ninguna función toca timers ni UART; esperas por lazos acotados/CyDelayUs.
 *  - Nada de esto corre en ISR. Solo main loop.
 */
#ifndef SD_SPI_H
#define SD_SPI_H

#include "project.h"

#define SD_BLOCK_BYTES 512u

/* Tipos de tarjeta detectados */
#define SD_TYPE_NONE  0u
#define SD_TYPE_SD1   1u   /* SD v1.x, direccionamiento por byte */
#define SD_TYPE_SD2   2u   /* SD v2 estándar (byte) */
#define SD_TYPE_SDHC  3u   /* SD v2 alta capacidad (bloque) */

/* Intenta la secuencia completa de init en modo SPI (CMD0/CMD8/ACMD41/CMD58).
 * Devuelve 1 si quedó lista. Idempotente: se puede reintentar. */
uint8 sd_spi_init(void);

uint8 sd_spi_present(void);
uint8 sd_spi_card_type(void);
uint32 sd_spi_sector_count(void);

/* Byte de estado compacto para telemetría (PSOC_EVT_SD_STATUS / ack 0xBC):
 * bit0 = presente, bits1-2 = tipo (SD_TYPE_*), bit3 = self-test OK previo. */
uint8 sd_spi_status_byte(void);

/* Diagnostico de la ultima inicializacion. Permite separar por software una
 * SD muda de un SPI que ni siquiera conmuta sus pads.
 *
 * stage: 1=relojes iniciales, 2=CMD0, 3=CMD8, 4=ACMD41/CMD1,
 *        5=CMD58, 6=CMD16, 7=CSD, 8=capacidad, 9=lista.
 *        bit7 indica timeout interno del RX FIFO.
 * last_r1: ultima respuesta R1 observada.
 * pin_flags: niveles fisicos vistos mientras el SPI estaba trabajando. */
#define SD_DIAG_MISO_LOW   0x01u
#define SD_DIAG_MISO_HIGH  0x02u
#define SD_DIAG_SCK_LOW    0x04u
#define SD_DIAG_SCK_HIGH   0x08u
#define SD_DIAG_MOSI_LOW   0x10u
#define SD_DIAG_MOSI_HIGH  0x20u
#define SD_DIAG_CS_LOW     0x40u
#define SD_DIAG_CS_HIGH    0x80u

uint8 sd_spi_diag_stage(void);
uint8 sd_spi_diag_last_r1(void);
uint8 sd_spi_diag_pin_flags(void);

/* Lectura/escritura de un bloque de 512 bytes. lba es índice de bloque
 * (independiente del tipo de tarjeta; la conversión a byte-address para SDSC
 * se hace adentro). Devuelven 1 en éxito. */
uint8 sd_spi_read_block(uint32 lba, uint8 *dst);
uint8 sd_spi_write_block(uint32 lba, const uint8 *src);

/* Escribe un patrón en un bloque de scratch, lo relee y compara.
 * No pisa el área de datos de capturas. Devuelve 1 si verificó. */
uint8 sd_spi_self_test(void);
void sd_spi_set_self_test_result(uint8 ok);

#endif

/* [] END OF FILE */
