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
4. Fuera de captura, IDAC2/IDAC3 mantienen LPo con PI entero. Cada ~1,5 s se
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
de 3000 ms, tau de 9000 ms, Kp 1/4, barrido cada 1000 ms, reporte cada 5000 ms
y captura por LPo (canal 4). La lista completa y sus limites vive en
`control_config.h/.c`; no hay parametros operativos escondidos en `#define`.

### Ley del lazo rapido (v4, 2026-09-16)

Todas las tensiones estan en el dominio del ADC de control (`ctl_dc`), el unico
que el lazo observa; no son tensiones fisicas del tap. Medido en geo-01:

| magnitud | valor |
|---|---|
| ventana valida de lectura | -95..+98 mV |
| ruido de LPo / SUMo | +-3 / +-6 mV |
| IDAC3 sobre LPo | ~+0,25 mV/codigo (todo su recorrido: +-64 mV) |
| IDAC2 sobre LPo | ~-80 mV inmediato, ~-150 mV asentado |

Prioridades, en orden:

1. **Guarda de SUMo**: solo si SUMo esta en riel o pasa de +-60 mV, IDAC2 da un
   codigo y espera 45 s. La banda vieja de +-5 mV estaba dentro del ruido y
   hacia oscilar IDAC2 para siempre, tirando LPo de lado a lado cada 45 s.
2. **Rescate de LPo**: en riel, IDAC3 camina por signo de a 100 codigos cada
   10 s; si esta en tope, IDAC2 toma un codigo.
3. **Histeresis**: con |LPo| <= 6 mV el lazo se congela y no vuelve a actuar
   hasta que LPo sale de +-20 mV. La banda muerta vieja (100 mV) era mas ancha
   que la ventana valida, asi que el PI no corria nunca.
4. **Descarga a IDAC2**: solo con IDAC3 >= 240 codigos, el error empujandolo
   mas afuera y un error predicho menor. Un codigo de IDAC2 es mayor que todo
   IDAC3: sin esa prediccion, LPo en el hueco entre dos codigos de IDAC2 hacia
   bailar a IDAC2.
5. **PI** hacia cero con acumulador fraccional.

**Modo estable.** Tras 60 s congelado, el lazo solo despierta con 3 lecturas
seguidas (9 s) fuera de +-35 mV; un riel lo despierta siempre. Ademas el barrido
de diagnostico pasa de ~2 s a 20 s y el reporte de 5 s a 30 s: en el
osciloscopio (2026-09-16 16:34) los ticks remanentes eran el barrido del AMux
(espiga de 4 ms cada 2 s en SUMo/LPo) y la rafaga I2C del reporte (LPo +1,5 V
40 ms y cola de 0,3 s cada 5 s), no movimientos del PI. La bandera "en banda"
es ahora el veredicto del lazo (congelado), asi que solo cambia al congelarse
o despertar.

Medicion: mediana de 5 promedios por canal (la cadena muestra pozos de 40 ms y
-30 mV cada ~2 s) y 800 ms de lecturas descartadas despues de cada envio de
telemetria, porque cada rafaga I2C hacia el ESP golpea LPo +47/-54 mV durante
~0,5 s (traza del PSoC). `ctl get 255` vuelca las ultimas 127 lecturas crudas
de LPo con su tiempo para diagnosticar sin osciloscopio.

Retocar parametros del lazo rapido ya no invalida el perfil aprendido: el
perfil son IDAC0/IDAC1 y solo depende de ganancias, capacitor, pendientes
lentas y consigna del sumador. Las imagenes EEPROM v3 se migran al arrancar
conservando el perfil y tomando el lazo rapido de los valores de fabrica.

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
