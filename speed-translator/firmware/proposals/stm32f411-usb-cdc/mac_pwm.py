import argparse

import serial
import serial.tools.list_ports

parser = argparse.ArgumentParser(description="Send PWM commands to the STM32 over USB serial.")
parser.add_argument("--port", help="Serial device, e.g. COM3 or /dev/ttyACM0")
args = parser.parse_args()

print("STM32 PWM Controller")

port = args.port
if port is None:
    ports = [
        p.device for p in serial.tools.list_ports.comports()
        if "usbmodem" in p.device.lower()
    ]
    if not ports:
        parser.error(
            "No macOS USB modem port detected. List available ports with "
            "python -m serial.tools.list_ports and pass --port PORT."
        )
    port = ports[0]

print("Using port:", port)

with serial.Serial(port, 115200, timeout=2) as ser:
    print("Enter frequency and duty cycle.")
    print("Example: 20000 50")
    print("Type q to quit.")

    while True:
        command = input("PWM > ").strip()

        if command.lower() == "q":
            break

        parts = command.split()

        if len(parts) != 2:
            print("Example: 20000 50")
            continue

        try:
            frequency, duty = map(int, parts)
        except ValueError:
            print("Please enter two integers.")
            continue

        if not (10 <= frequency <= 100000):
            print("Frequency must be 10-100000 Hz.")
            continue

        if not (0 <= duty <= 100):
            print("Duty cycle must be 0-100%.")
            continue

        message = f"PWM {frequency} {duty}\n"
        ser.write(message.encode("ascii"))
        ser.flush()

        print(f"Sent: {frequency} Hz, {duty}%")
