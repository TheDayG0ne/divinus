#!/usr/bin/env python3
"""Integration test for a future x86 test server, not a production camera.

Start a server on localhost with CLAIM_SHADOW compiled to a disposable shadow
file and RTSP auth disabled in its configuration. Pass PORT and that file's
absolute path. This script overwrites and removes the file; never pass a real
shadow file. The server must already be running. No build is performed here.
"""
import base64
import ctypes
import ctypes.util
import pathlib
import socket
import sys

port = int(sys.argv[1])
shadow = pathlib.Path(sys.argv[2])
if not shadow.is_absolute() or shadow.name != 'divinus-rtsp-test-shadow':
    raise SystemExit('Use an absolute disposable path named divinus-rtsp-test-shadow')
if shadow.exists() or shadow.is_symlink():
    raise SystemExit('Test shadow path must not already exist')

crypt = ctypes.CDLL(ctypes.util.find_library('crypt'))
crypt.crypt.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
crypt.crypt.restype = ctypes.c_char_p
hash1 = crypt.crypt(b'secret', b'$6$test$').decode()
assert not hash1.startswith('*')
def set_shadow(value):
    shadow.write_text('root:' + value + ':1:2:3:4:5:6:7\n')

def authorization(password):
    return 'Basic ' + base64.b64encode(('root:' + password).encode()).decode()

def request(sock, seq, method='OPTIONS', headers=()):
    msg = f'{method} rtsp://127.0.0.1/stream RTSP/1.0\r\nCSeq: {seq}\r\n'
    msg += ''.join(f'{key}: {value}\r\n' for key, value in headers) + '\r\n'
    sock.sendall(msg.encode())
    data = b''
    while b'\r\n\r\n' not in data:
        part = sock.recv(4096)
        if not part:
            raise AssertionError('connection closed before response')
        data += part
    head = data.split(b'\r\n\r\n', 1)[0].decode('ascii')
    assert f'CSeq: {seq}' in head, head
    return head

def denied(response):
    assert response.startswith('RTSP/1.0 401 '), response
    assert 'WWW-Authenticate: Basic realm="Access the camera streams"' in response, response

try:
    set_shadow(hash1)
    with socket.create_connection(('127.0.0.1', port), timeout=3) as sock:
        good = [('Authorization', authorization('secret'))]
        denied(request(sock, 1))
        denied(request(sock, 2, headers=[('Authorization', authorization('wrong'))]))
        denied(request(sock, 3, headers=[('Authorization', authorization('secret') + 'junk')]))
        denied(request(sock, 4, headers=good + good))
        assert request(sock, 5, headers=good).startswith('RTSP/1.0 200 ')
        denied(request(sock, 6, 'GET_PARAMETER'))
        assert request(sock, 7, 'GET_PARAMETER', good).startswith('RTSP/1.0 200 ')
        set_shadow('!')
        denied(request(sock, 8, headers=good))
        set_shadow('')
        denied(request(sock, 9, headers=good))
        shadow.unlink()
        denied(request(sock, 10, headers=good))
        set_shadow(hash1)
        assert request(sock, 11, headers=good).startswith('RTSP/1.0 200 ')
    # Claim gate closes newly accepted connections before RTSP authentication.
    set_shadow('')
    with socket.create_connection(('127.0.0.1', port), timeout=3) as sock:
        sock.sendall(b'OPTIONS rtsp://127.0.0.1/stream RTSP/1.0\r\nCSeq: 1\r\n\r\n')
        try:
            assert sock.recv(4096) == b''
        except ConnectionResetError:
            pass
finally:
    shadow.unlink(missing_ok=True)
