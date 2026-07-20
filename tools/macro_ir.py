import socket, struct, time, sys

IP = "192.168.1.X"  # IP de ta 3DS
PORT = 4950

BUTTON_A = 0x001
BUTTON_B = 0x002
BUTTON_X = 0x400
BUTTON_R = 0x100

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

def send(buttons):
    pkt = struct.pack('<IIIII', 0xFFF & ~buttons, 0x2000000, 0x7FF7FF, 0x80800081, 0)
    sock.sendto(pkt, (IP, PORT))

def hold(buttons, sec):
    end = time.monotonic() + sec
    while time.monotonic() < end:
        send(buttons)
        time.sleep(0.03)  # 30ms entre chaque envoi

def tap(buttons):
    hold(buttons, 0.1)

seq = [
    (BUTTON_A, 5),
    (BUTTON_A, 0.5),
    (BUTTON_X, 0.5),
    (BUTTON_A, 1),
    (BUTTON_R, 0.5),
    (BUTTON_A, 3),
    (BUTTON_B, 1),
    (BUTTON_B, 1),
    (BUTTON_B, 1),
]

print("InputRedirection + mode continu (pas de perte)")
print("Ctrl+C pour arrêter.")
time.sleep(2)

try:
    while True:
        for btn, delay in seq:
            tap(btn)
            time.sleep(delay)
except KeyboardInterrupt:
    send(0)
    sock.close()
