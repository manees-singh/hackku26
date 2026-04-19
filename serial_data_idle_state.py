#!/usr/bin/env python
import argparse
import os
import uuid
import serial
import serial.tools.list_ports

DEFAULT_BAUD = 115200
DEFAULT_LABEL = "_unknown"
TARGET_SAMPLES = 30  # stops automatically after this many

def write_csv(data, dir, label, count):
    exists = True
    while exists:
        uid = str(uuid.uuid4())[-12:]
        filename = label + "." + uid + ".csv"
        out_path = os.path.join(dir, filename)
        if not os.path.exists(out_path):
            exists = False
            try:
                with open(out_path, 'w') as file:
                    file.write(data)
                print(f"[{count}/{TARGET_SAMPLES}] Saved: {out_path}")
            except IOError as e:
                print("ERROR", e)
                return

print()
print("Available serial ports:")
available_ports = serial.tools.list_ports.comports()
for port, desc, hwid in sorted(available_ports):
    print("  {} : {} [{}]".format(port, desc, hwid))

parser = argparse.ArgumentParser(description="Serial Data Collection CSV")
parser.add_argument('-p', '--port', dest='port', type=str, required=True)
parser.add_argument('-b', '--baud', dest='baud', type=int, default=DEFAULT_BAUD)
parser.add_argument('-d', '--directory', dest='directory', type=str, default=".")
parser.add_argument('-l', '--label', dest='label', type=str, default=DEFAULT_LABEL)
parser.add_argument('-n', '--num-samples', dest='num_samples', type=int, default=TARGET_SAMPLES)

args = parser.parse_args()
port = args.port
baud = args.baud
out_dir = args.directory
label = args.label
target = args.num_samples

ser = serial.Serial()
ser.port = port
ser.baudrate = baud

try:
    ser.open()
except Exception as e:
    print("ERROR:", e)
    exit()

print(f"\nConnected to {port} at {baud} baud")
print(f"Collecting {target} samples for label: '{label}'")
print("Press Ctrl+C to stop early\n")

rx_buf = b''
sample_count = 0

try:
    os.makedirs(out_dir)
except FileExistsError:
    pass

try:
    while sample_count < target:
        if ser.in_waiting > 0:
            while ser.in_waiting:
                rx_buf += ser.read()

                if rx_buf[-4:] == b'\r\n\r\n':
                    buf_str = rx_buf.decode('utf-8').strip()
                    buf_str = buf_str.replace('\r', '')

                    if buf_str and 'timestamp' in buf_str:
                        sample_count += 1
                        write_csv(buf_str, out_dir, label, sample_count)

                        if sample_count >= target:
                            print(f"\nDone! Collected {target} samples.")
                            break

                    rx_buf = b''

except KeyboardInterrupt:
    print(f"\nStopped early. Collected {sample_count} samples.")

print("Closing serial port")
ser.close()