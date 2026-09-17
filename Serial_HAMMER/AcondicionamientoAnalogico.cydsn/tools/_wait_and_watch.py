#!/usr/bin/env python3
"""Reintenta abrir COM8/COM6 hasta que estén libres, y despues va imprimiendo
todo lo que llegue. Pensado para correr en background mientras el usuario
cierra el monitor de PlatformIO y programa el PSoC -- no hace falta avisar,
esto detecta la reconexion sola."""
import sys
import time
import serial

PORTS = [("COM8", 115200), ("COM6", 115200)]


def try_open(port, baud):
    try:
        return serial.Serial(port, baud, timeout=0.3)
    except serial.SerialException:
        return None


def main():
    conns = {}
    print("esperando puertos libres...", flush=True)
    while len(conns) < len(PORTS):
        for port, baud in PORTS:
            if port in conns:
                continue
            s = try_open(port, baud)
            if s:
                conns[port] = s
                print("[{}] conectado".format(port), flush=True)
        if len(conns) < len(PORTS):
            time.sleep(2)

    print("--- todos los puertos abiertos, escuchando ---", flush=True)
    bufs = {p: b"" for p in conns}
    t0 = time.monotonic()
    while True:
        for port, ser in list(conns.items()):
            try:
                chunk = ser.read(256)
            except serial.SerialException as exc:
                print("[{}] error: {}".format(port, exc), flush=True)
                del conns[port]
                continue
            if not chunk:
                continue
            bufs[port] += chunk
            while b"\n" in bufs[port]:
                line, bufs[port] = bufs[port].split(b"\n", 1)
                text = line.decode("utf-8", errors="replace").rstrip("\r")
                if text:
                    print("[{}][{:9.3f}s] {}".format(port, time.monotonic() - t0, text), flush=True)
        if not conns:
            print("todos los puertos se cerraron, saliendo", flush=True)
            sys.exit(0)


if __name__ == "__main__":
    main()
