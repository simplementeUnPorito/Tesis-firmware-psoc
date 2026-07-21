# Firmware PSoC de Tesis

Este repositorio contiene el firmware y los diseños PSoC 5LP del sistema de adquisición MASW. Es una unidad independiente del superproyecto [`Tesis`](https://github.com/simplementeUnPorito/Tesis) y conserva el historial que antes vivía en `src/psoc`.

## Contenido

- `AcondicionamientoAnalogico.cydsn/`: firmware activo para nodos GEO y HAMMER.
- `Analog_LPF_v1_0.cylib/` y `Component Libraries/`: componentes reutilizables.
- `shared/`: código compartido.
- `BUILD_PROGRAM_PSOC.md`: compilación y programación con PSoC Creator 4.4.
- `program_psoc.ps1`: automatización de programación.

Los directorios de compilación (`CortexM3/` y `codegentemp/`) se regeneran con PSoC Creator y no se versionan.

## Uso

Desde el superproyecto, este repositorio se encuentra en `firmware/psoc`. También puede clonarse y trabajarse de forma independiente:

```powershell
git clone https://github.com/simplementeUnPorito/Tesis-firmware-psoc.git
cd Tesis-firmware-psoc
```

Abrir `AcondicionamientoAnalogico.cydsn/AcondicionamientoAnalogico.cywrk` con PSoC Creator 4.4. La integración física y de protocolo con los ESP32 está documentada en el repositorio `Tesis-firmware-esp32`.
