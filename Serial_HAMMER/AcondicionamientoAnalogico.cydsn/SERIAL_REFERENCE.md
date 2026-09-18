# Referencia serial HAMMER

Esta carpeta es una copia de trabajo del proyecto PSoC recuperada de Git.

- Commit: `806a58094af489d6a936f1b19f17a5b8c124d204`
- Fecha: 2026-07-21 00:17:24 -0300
- Pareja ESP32: `adf41af1e002821cd970bc6a2468cafe4dcd58a8`
- Enlace PSoC-ESP: UART serial

## Cableado confirmado

- PSoC P1[5] (`Rx`) <- ESP GPIO16 (TX)
- PSoC P1[2] (`Tx`) -> ESP GPIO17 (RX)
- PSoC P12[2] (`SYNC_IN`) <- ESP GPIO23 (SYNC)

La asignación también queda registrada en
`Generated_Source/PSoC5/cyfitter.h`. El proyecto puede servir como base para
GEO si se habilita y enruta su TopDesign correspondiente; eso todavía no se
modificó en este snapshot.

## Cambios sobre el snapshot (2026-09-17)

La lógica VDAC y el calibrador de una fase no se tocaron. Solo se portó el
watchdog de ARMED de `443a4ca` (1-sep): si el flanco de SYNC no llega en 60 s,
el PSoC se desarma solo, vuelve a IDLE y emite `PSOC_EVT_ARMED_TIMEOUT`
(0x4B). Sin esto, un SYNC perdido lo dejaba armado y sordo hasta un reset. El
ESP de `esp32/Serial` ya reacciona a ese evento: vuelve a STOPPED y le manda un
NACK al maestro.

`Generated_Source` quedó fuera de la carpeta `.cydsn` al extraer el snapshot, y
así no compilaba: `main.c` incluye `isr_Timer_1.h`, que el TopDesign ya no
genera pero sigue presente entre los archivos viejos. Se movió adentro, como
está en el commit original. `cyprjmgr -rebuild -c Release -m adc_2604.params`
compila (Rebuild Succeeded). Todavía no se grabó.
