#!/usr/bin/env python3
"""Reintenta abrir COM8/COM6, espera el handshake del ESP ("esperando ARM" o
"DETECTADO"), manda 'cal' solo, y loguea todo lo que llegue. Pensado para
correr sin que el usuario tenga que tipear nada -- solo prender/reiniciar
las placas."""
import time
import serial

PORTS = [("COM8", 115200), ("COM6", 115200)]
TRIGGER_MARKERS = ("esperando ARM", "DETECTADO", "listo")
CAL_SENT_TO = "COM8"


def try_open(port, baud):
    try:
        return serial.Serial(port, baud, timeout=0.3)
    except serial.SerialException:
        return None


def main():
    conns = {}
    cal_sent = False
    print("esperando puertos libres...", flush=True)
    bufs = {}
    t0 = time.monotonic()

    while True:
        for port, baud in PORTS:
            if port not in conns:
                s = try_open(port, baud)
                if s:
                    conns[port] = s
                    bufs[port] = b""
                    print("[{}] conectado".format(port), flush=True)

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
                if not text:
                    continue
                print("[{}][{:9.3f}s] {}".format(port, time.monotonic() - t0, text), flush=True)
                if not cal_sent and CAL_SENT_TO in conns and any(m in text for m in TRIGGER_MARKERS):
                    time.sleep(1.0)
                    conns[CAL_SENT_TO].write(b"cal\n")
                    print("[{}] >> enviado 'cal' automaticamente".format(CAL_SENT_TO), flush=True)
                    cal_sent = True
        time.sleep(0.05)


if __name__ == "__main__":
    main()
