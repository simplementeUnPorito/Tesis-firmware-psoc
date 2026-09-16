# Control de calibracion unificado

Este es el unico proyecto PSoC activo. El mismo binario Release contiene la
captura, el aprendizaje lento y el PI permanente; Debug activa las pruebas con
`PSOC_TEST=1`. Los antiguos proyectos `AcondicionamientoAnalogicoTest` y
`AcondicionamientoAnalogicoField5Test.cydsn` fueron absorbidos y retirados.

## Secuencia de operacion

1. Al encender se validan dos registros EEPROM alternados (version y CRC).
2. Si no existe perfil compatible, el estado queda `FALTA_APRENDER`; no se
   aprende automaticamente.
3. `ctl learn` autoriza una medicion lenta de IDAC0/IDAC1 en las ganancias
   nominales. Solo se guarda un resultado estable y valido.
4. Fuera de captura, IDAC2/IDAC3 mantienen LPo con PI entero. Cada 5 s se
   recorren SEo, BPo, OPA_SUMo, SUMo y LPo para diagnostico.
5. Al armar captura se congelan los cuatro IDAC, se detienen PI, aprendizaje,
   barrido y telemetria, se desconecta el capacitor y se carga
   `FIR_adquisition`. Despues del volcado se restaura `FIR_calibration` y el
   control continua.

El sexto canal del AMux no es una senal: conecta el capacitor de 100 nF durante
calibracion cuando el parametro 3 vale uno.

## Configuracion inicial

Las unidades del protocolo son enteras: microvoltios, milisegundos y codigos
IDAC. Los valores principales de fabrica son PGA x4 (codigo 2), PGAout x24
(codigo 5), promedio de 54 muestras filtradas, capacitor habilitado, periodo PI
de 5000 ms, tau de 30000 ms, Kp 1/4, banda muerta de +/-100000 uV, rescate de
80 codigos con espera de 40000 ms, barrido cada 5000 ms, reporte cada 1000 ms y
captura por LPo (canal 4). La lista completa y sus limites vive en
`control_config.h/.c`; no hay parametros operativos escondidos en `#define`.

`control_config_valid()` rechaza rangos, canales, pendientes, tiempos y margenes
inconsistentes. Una modificacion incompatible invalida el perfil aprendido. El
guardado es explicito; el PI nunca escribe EEPROM durante sus iteraciones.

## Comandos desde el USB del esclavo

Los comandos son transportados por el ESP32 hacia el PSoC:

- `ctl report`: estado, configuracion, perfil, IDAC y DC de los cinco taps.
- `ctl get N` / `ctl set N VALOR`: leer o preparar un parametro.
- `ctl apply`, `ctl save`, `ctl defaults`: aplicar, persistir o restaurar.
- `ctl learn`: autorizar aprendizaje lento.
- `ctl pause` / `ctl resume`: congelar o reanudar control.
- `ctl channel N`: elegir SEo=0, BPo=1, OPA_SUMo=2, SUMo=3 o LPo=4.

Los cambios se aplican completos entre ciclos y se rechazan durante armado o
adquisicion. La captura no se bloquea por perfil ausente o banda incorrecta:
esas condiciones viajan como advertencia y metadatos para que decida el maestro.

## Verificacion

- Host: `gcc tests/control_test.c AcondicionamientoAnalogico.cydsn/control_config.c`.
- PSoC normal: `program_psoc.ps1 -Configuration Release`.
- PSoC pruebas: `program_psoc.ps1 -Configuration Debug -SelfTest`.
- ESP: `pio run -e slave2 -e slaveTest` desde el proyecto `slave`.

La captura espera el evento real `ARMED` antes de producir SYNC; nunca debe
inferir que el PSoC esta listo a partir de una demora fija.
