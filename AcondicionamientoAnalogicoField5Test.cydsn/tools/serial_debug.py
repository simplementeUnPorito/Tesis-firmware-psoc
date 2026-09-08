#!/usr/bin/env python3
"""
Monitor serie multi-puerto para depurar en vivo el lazo ESP (slave, COM8 tipico,
CP210x) y/o el UART de debug directo del PSoC ("PC", componente nuevo en
TopDesign/Digital -- ver psoc_debug.c/.h -- tipicamente en COM6 via KitProg).

Por que esto y no solo el monitor de PlatformIO: PlatformIO se queda pegado
a UN puerto (el del ESP) y no deja mandar comandos mientras mira el log del
PSoC al mismo tiempo. Este script abre los puertos que le pidas en paralelo,
mezcla la salida con [PUERTO] y timestamp relativo, y deja mandar comandos
de texto a cualquiera de ellos sin cerrar nada.

Requiere: pip install pyserial

Uso:
    python tools/serial_debug.py --port COM8 --port COM6
    python tools/serial_debug.py --port COM8:115200 --port COM6:115200 --log cal_debug.log

Mientras corre, escribir en la consola y Enter para mandar:
    cal                 -> manda "cal\n" al PRIMER puerto listado (default)
    COM8:cal             -> manda "cal\n" especificamente a COM8
    COM6:cualquier_texto  -> manda texto a COM6 (ej. comandos de debug propios)
    all:texto             -> manda a TODOS los puertos abiertos
    quit / exit           -> cierra el script

Ctrl+C tambien cierra limpio.

NOTA IMPORTANTE: un puerto serie solo lo puede tener abierto un proceso a la
vez. Si el monitor de PlatformIO (`platformio device monitor`) sigue corriendo
sobre el mismo puerto, este script va a fallar al abrirlo -- cerrar ese
monitor primero (Ctrl+C en su terminal) antes de correr esto.
"""

import argparse
import sys
import threading
import time

try:
    import serial
except ImportError:
    print("Falta pyserial. Instalar con: pip install pyserial", file=sys.stderr)
    sys.exit(1)


class PortWorker:
    def __init__(self, name, port, baud, log_file, start_time):
        self.name = name
        self.port_name = port
        self.baud = baud
        self.log_file = log_file
        self.start_time = start_time
        self.ser = None
        self.thread = None
        self.stop_flag = False

    def open(self):
        try:
            self.ser = serial.Serial(self.port_name, self.baud, timeout=0.2)
            return True
        except serial.SerialException as exc:
            print("[{}] No se pudo abrir {}: {}".format(self.name, self.port_name, exc))
            print("    (¿esta abierto en otro programa? ej. el monitor de PlatformIO)")
            return False

    def _emit(self, line):
        elapsed = time.monotonic() - self.start_time
        out = "[{}][{:9.3f}s] {}".format(self.name, elapsed, line)
        print(out)
        if self.log_file:
            self.log_file.write(out + "\n")
            self.log_file.flush()

    def reader_loop(self):
        buf = b""
        while not self.stop_flag:
            try:
                chunk = self.ser.read(256)
            except serial.SerialException as exc:
                self._emit("** error de lectura: {} -- cerrando este puerto **".format(exc))
                return
            if not chunk:
                continue
            buf += chunk
            while b"\n" in buf:
                line, buf = buf.split(b"\n", 1)
                text = line.decode("utf-8", errors="replace").rstrip("\r")
                if text != "":
                    self._emit(text)

    def start(self):
        self.thread = threading.Thread(target=self.reader_loop, daemon=True)
        self.thread.start()

    def send(self, text):
        if self.ser is None:
            return
        payload = (text + "\n").encode("utf-8")
        self.ser.write(payload)
        self._emit(">> (enviado) {}".format(text))

    def close(self):
        self.stop_flag = True
        if self.ser is not None:
            try:
                self.ser.close()
            except serial.SerialException:
                pass


def parse_port_spec(spec, default_baud):
    if ":" in spec:
        port, baud_s = spec.rsplit(":", 1)
        try:
            return port, int(baud_s)
        except ValueError:
            return spec, default_baud
    return spec, default_baud


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                      formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--port", action="append", required=True,
                         help="Puerto a abrir, opcionalmente PUERTO:BAUD (ej. COM8:115200). Repetible.")
    parser.add_argument("--baud", type=int, default=115200,
                         help="Baud rate default para puertos sin baud explicito (default 115200).")
    parser.add_argument("--log", default=None, help="Archivo donde tambien guardar todo lo recibido/enviado.")
    args = parser.parse_args()

    log_file = open(args.log, "a", encoding="utf-8") if args.log else None
    start_time = time.monotonic()

    workers = []
    for spec in args.port:
        port_name, baud = parse_port_spec(spec, args.baud)
        label = port_name.upper()
        w = PortWorker(label, port_name, baud, log_file, start_time)
        if w.open():
            print("[{}] abierto @ {} baud".format(label, baud))
            w.start()
            workers.append(w)

    if not workers:
        print("Ningun puerto se pudo abrir. Saliendo.")
        if log_file:
            log_file.close()
        sys.exit(1)

    default_worker = workers[0]
    by_name = {w.name: w for w in workers}

    print("--- Listo. Default de envio: {} (ver --help para targets explicitos) ---".format(default_worker.name))

    try:
        while True:
            try:
                line = input()
            except EOFError:
                break
            line = line.strip()
            if line == "":
                continue
            if line in ("quit", "exit"):
                break

            target_name = None
            payload = line
            if ":" in line:
                maybe_target, rest = line.split(":", 1)
                key = maybe_target.upper()
                if key == "ALL":
                    for w in workers:
                        w.send(rest)
                    continue
                if key in by_name:
                    target_name = key
                    payload = rest

            worker = by_name.get(target_name, default_worker)
            worker.send(payload)
    except KeyboardInterrupt:
        print("\nCtrl+C -- cerrando puertos...")
    finally:
        for w in workers:
            w.close()
        if log_file:
            log_file.close()


if __name__ == "__main__":
    main()
