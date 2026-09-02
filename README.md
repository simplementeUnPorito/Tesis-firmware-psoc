# Firmware PSoC de Tesis

Este repositorio contiene el firmware y los diseños PSoC 5LP del sistema de adquisición MASW. Es una unidad independiente del superproyecto [`Tesis`](https://github.com/simplementeUnPorito/Tesis) y conserva el historial que antes vivía en `src/psoc`.

## Contenido

- `AcondicionamientoAnalogico.cydsn/`: firmware activo para nodos GEO y HAMMER.
- `AcondicionamientoAnalogicoTest/AcondicionamientoAnalogico.cydsn/`: variante
  de banco para el autotest de placa; no es el firmware de campo.
- `Analog_LPF_v1_0.cylib/` y `Component Libraries/`: componentes reutilizables.
- `shared/`: código compartido.
- `BUILD_PROGRAM_PSOC.md`: compilación y programación con PSoC Creator 4.4.
- `program_psoc.ps1`: automatización de programación.
- `reset_psoc.ps1`: reset por KitProg después de regrabar el ESP32.
- `AUTOTEST_NODO_ESCLAVO.md`: pruebas digitales/analógicas, comandos y
  criterios del firmware de banco.
- `TOPDESIGN_TO_KICAD.md`: pinout, valores recuperados y criterios para
  transferir el TopDesign a una placa KiCad con CY8CKIT-059 y ESP32 DevKitC.
- `extract_topdesign_strings.ps1`: extractor de referencias y valores del
  archivo binario `TopDesign.cysch`, sin depender de la interfaz gráfica.
- `verify_carrier_pinout.ps1`: comprueba que el pinout físico usado por la
  carrier coincide con `cyfitter.h`, está bloqueado en el `.cydwr` y proviene
  de una compilación exitosa.

Los directorios de compilación (`CortexM3/` y `codegentemp/`) se regeneran con PSoC Creator y no se versionan.

## Uso

Desde el superproyecto, este repositorio se encuentra en
`src/firmware/psoc`. También puede clonarse y trabajarse de forma independiente:

```powershell
git clone https://github.com/simplementeUnPorito/Tesis-firmware-psoc.git
cd Tesis-firmware-psoc
```

Abrir `AcondicionamientoAnalogico.cydsn/AcondicionamientoAnalogico.cywrk` con PSoC Creator 4.4. La integración física y de protocolo con los ESP32 está documentada en el repositorio `Tesis-firmware-esp32`.

Flujo automatizado en Windows:

```powershell
.\program_psoc.ps1             # firmware normal
.\program_psoc.ps1 -SelfTest   # firmware de autotest
.\reset_psoc.ps1               # sólo reset
```

La programación usa las cuatro matrices de 256 filas con ECC. No recuperar
recetas históricas que grababan sólo hasta `lastRow`: omiten configuración
digital del PSoC 5LP.
