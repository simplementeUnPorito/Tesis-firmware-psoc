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
