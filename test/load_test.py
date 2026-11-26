"""Hardware test: 30 MQTT 3.1.1 clients. Python standard library only.

Run only against the new test broker; existing clients count toward its limit.
Credentials default to include/credentials.h and never appear in output.
"""
import argparse
import json
import re
import socket
import struct
import time
import uuid
import urllib.request
from pathlib import Path


def field(value):
    value = value.encode() if isinstance(value, str) else value
    return struct.pack('!H', len(value)) + value


def packet(kind, payload=b''):
    n, encoded = len(payload), bytearray()
    while True:
        digit, n = n % 128, n // 128
        encoded.append(digit | (128 if n else 0))
        if not n:
            return bytes([kind]) + encoded + payload


def exact(sock, count):
    result = b''
    while len(result) < count:
        part = sock.recv(count - len(result))
        if not part:
            raise ConnectionError('Broker closed a connection')
        result += part
    return result


def receive(sock):
    kind = exact(sock, 1)[0]
    size, multiplier = 0, 1
    for _ in range(4):
        digit = exact(sock, 1)[0]
        size += (digit & 127) * multiplier
        if not digit & 128:
            return kind, exact(sock, size)
        multiplier *= 128
    raise AssertionError('Invalid MQTT remaining length')


def expect(sock, kind, body):
    actual = receive(sock)
    assert actual == (kind, body), f'Unexpected packet type/body: {actual[0]:02x}'


def connect(args, client_id):
    sock = socket.create_connection((args.host, args.port), timeout=10)
    try:
        body = field('MQTT') + bytes([4, 0xC2]) + struct.pack('!H', 30)
        body += field(client_id) + field(args.user) + field(args.password)
        sock.sendall(packet(0x10, body))
        expect(sock, 0x20, b'\x00\x00')
        return sock
    except BaseException:
        sock.close()
        raise


def subscribe(sock, topic):
    sock.sendall(packet(0x82, b'\x00\x01' + field(topic) + b'\x00'))
    expect(sock, 0x90, b'\x00\x01\x00')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('host', help='IP of MqttBroker2')
    parser.add_argument('--port', type=int, default=1883)
    parser.add_argument('--clients', type=int, default=30)
    parser.add_argument('--seconds', type=int, default=120)
    parser.add_argument('--user')
    parser.add_argument('--password')
    args = parser.parse_args()
    if args.clients < 3 or args.clients > 30:
        parser.error('--clients must be between 3 and 30')
    if args.seconds < 1:
        parser.error('--seconds must be positive')
    credentials = Path(__file__).resolve().parents[1] / 'include' / 'credentials.h'
    if args.user is None or args.password is None:
        text = credentials.read_text()
        def setting(name):
            match = re.search(r'\b' + name + r'\[\]\s*=\s*("(?:\\.|[^"\\])*")', text)
            if not match:
                raise ValueError('Missing credential: ' + name)
            return json.loads(match[1])
        args.user = args.user if args.user is not None else setting('MQTT_USER')
        args.password = args.password if args.password is not None else setting('MQTT_PASSWORD')
    prefix = 'loadtest-' + uuid.uuid4().hex[:10]
    topic = 'mqttbroker2/test/' + prefix
    sockets = []
    try:
        for index in range(args.clients):
            sock = connect(args, f'{prefix}-{index}')
            sockets.append(sock)
            subscribe(sock, topic)
            time.sleep(0.05)
        with urllib.request.urlopen(f'http://{args.host}/status', timeout=10) as response:
            status = json.load(response)
        assert status['socket_limit'] == 40, 'Firmware does not report 40 sockets'
        assert status['clients'] == args.clients, 'Unexpected active client count'
        if args.clients == 30:
            try:
                extra = connect(args, prefix + '-over-capacity')
            except ConnectionError:
                pass
            else:
                extra.close()
                raise AssertionError('The 31st client was unexpectedly accepted')
        print(f'{len(sockets)} clients connected; web status and socket limit verified.')
        started, rounds = time.monotonic(), 0
        while time.monotonic() - started < args.seconds:
            payload = str(rounds).encode()
            sockets[rounds % len(sockets)].sendall(packet(0x30, field(topic) + payload))
            for sock in sockets:
                expect(sock, 0x30, field(topic) + payload)
                sock.sendall(packet(0xC0))
                expect(sock, 0xD0, b'')
            rounds += 1
            time.sleep(0.2)
        # Orderly and abrupt disconnects must both release capacity.
        for index in range(3):
            if index == 0:
                sockets[index].sendall(packet(0xE0))
            sockets[index].close()
        time.sleep(2)
        for index in range(3):
            sockets[index] = connect(args, f'{prefix}-replacement-{index}')
            subscribe(sockets[index], topic)
        sockets[-1].sendall(packet(0x30, field(topic) + b'reconnected'))
        for sock in sockets:
            expect(sock, 0x30, field(topic) + b'reconnected')
        print(f'PASS: {rounds} broadcasts to {args.clients} clients; ping and reconnect successful.')
    finally:
        for sock in sockets:
            try:
                sock.sendall(packet(0xE0))
            except OSError:
                pass
            sock.close()


if __name__ == '__main__':
    main()
