#!/usr/bin/env python3
"""Linux integration test. See README.md. Never run against a production binary."""
import base64
import ctypes
import ctypes.util
import http.client
import json
import os
from pathlib import Path
import signal
import socket
import subprocess
import sys
import time

root = Path('/tmp/divinus-central-only-test')
binary = root / 'divinus'
shadow = root / 'shadow'
config = root / 'divinus.yaml'
static = root / 'www'
manifest = root / 'cameras.manifest'
port, rtsp = 18081, 18554
if os.geteuid() != 0 or len(sys.argv) != 2 or sys.argv[1] != '--disposable-test-root':
    raise SystemExit('Requires root in an isolated disposable Linux VM; read README.md')
if not binary.is_file() or binary.is_symlink():
    raise SystemExit('Install the specially compiled test binary at ' + str(binary))
static.mkdir(exist_ok=True)
(static / 'probe.txt').write_text('static-ok')
reserved_files = ['audio.mp3', 'audio.pcm', 'video.264', 'video.265', 'video.mp4',
                  'mjpeg', 'image.jpg', 'exit', 'setup', 'setup.html']
for name in reserved_files:
    (static / name).write_text('must-not-serve')
(static / 'assets').mkdir(exist_ok=True)
(static / 'assets' / 'probe.txt').write_text('nested-static-ok')
manifest.write_text('slot=1 rtsp_port=18555\n')
manifest.chmod(0o600)
config.write_text('system:\n  web_port: 18081\n  web_enable_static: true\n'
                  '  isp_thread_stack_size: 16384\n  venc_stream_thread_stack_size: 16384\n'
                  '  web_server_thread_stack_size: 65536\n  watchdog: 30\n'
                  'isp:\n  mirror: false\n  flip: false\n'
                  'jpeg:\n  enable: false\nmjpeg:\n  enable: false\n'
                  'rtsp:\n  enable: true\n  port: 18554\n'
                  'onvif:\n  enable: true\nstream:\n  enable: true\n')
original = config.read_bytes()
crypt = ctypes.CDLL(ctypes.util.find_library('crypt'))
crypt.crypt.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
crypt.crypt.restype = ctypes.c_char_p
password_hash = crypt.crypt(b'testpassword', b'$6$testsalt$').decode()
auth = 'Basic ' + base64.b64encode(b'root:testpassword').decode()
def request(path, authenticated=True, method='GET'):
    conn = http.client.HTTPConnection('127.0.0.1', port, timeout=3)
    conn.request(method, path, headers={'Authorization': auth} if authenticated else {})
    r = conn.getresponse()
    body = r.read()
    code = r.status
    conn.close()
    return code, body

def ready(proc):
    for _ in range(100):
        if proc.poll() is not None:
            raise AssertionError('process exited: ' + str(proc.returncode))
        try:
            return request('/setup', False)
        except (OSError, http.client.HTTPException):
            time.sleep(.1)
    raise AssertionError('HTTP listener did not start')

def no_listener(port):
    with socket.socket() as s:
        s.settimeout(.5)
        assert s.connect_ex(('127.0.0.1', port)) != 0, port

shadow.write_text('root::1:2:3:4:5:6:7\n')
# An occupied HTTP port must cause a synchronous nonzero startup failure.
with socket.socket() as blocker:
    blocker.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    blocker.bind(('0.0.0.0', port))
    blocker.listen()
    failed = subprocess.run([str(binary), '--central-only'], cwd=root, timeout=10)
    assert failed.returncode != 0, 'startup succeeded despite occupied HTTP port'
log_path = root / 'server.log'
log = log_path.open('w')
proc = subprocess.Popen([str(binary), '--central-only'], cwd=root, stderr=log)
try:
    assert ready(proc)[0] == 200
    assert request('/api/cameras', False)[0] == 403
    shadow.write_text('root:' + password_hash + ':1:2:3:4:5:6:7\n')
    for path in ['/api/status', '/api/cameras', '/api/isp', '/onvif/device_service', '/probe.txt']:
        assert request(path, False)[0] == 401, path
        assert request(path, False, 'PUT')[0] == 401, path
    code, body = request('/api/status')
    assert code == 200 and json.loads(body)['mode'] == 'central-only'
    code, body = request('/api/cameras')
    assert code == 200 and json.loads(body)['cameras'][0]['id'] == 'camera-1'
    assert request('/probe.txt') == (200, b'static-ok')
    assert request('/assets/probe.txt') == (200, b'nested-static-ok')
    for name in reserved_files:
        for path in ['/' + name, '/' + name + '/', '//' + name, '/' + name + '//']:
            assert request(path)[0] == 404, path
    assert request('/assets//probe.txt')[0] == 404
    for path in ['/api/isp', '/api/openwall', '/api/time', '/api/jpeg', '/api/mjpeg', '/api/mp4', '/audio.mp3', '/audio.pcm', '/video.264', '/video.265', '/video.mp4', '/mjpeg', '/image.jpg', '/onvif/device_service', '/exit', '/api/unknown']:
        assert request(path)[0] == 404, path
    assert request('/api/status?x=1')[0] == 404
    assert request('/api/cameras', method='POST')[0] == 404
    # Occupy every request slot with incomplete headers and bodies. Periodic
    # bytes must not extend the absolute deadline indefinitely.
    slow = []
    try:
        for i in range(50):
            s = socket.create_connection(('127.0.0.1', port), timeout=3)
            s.sendall(b'GET /api/status HTTP/1.1\r\nHost: localhost\r\n' if i % 2 == 0 else
                      b'POST /setup HTTP/1.1\r\nHost: localhost\r\nContent-Length: 100\r\n\r\nx')
            slow.append(s)
        time.sleep(5)
        for s in slow:
            s.sendall(b'x')
        time.sleep(7)
        for s in slow:
            s.settimeout(3)
            try:
                assert s.recv(1) == b'', 'slow request was not closed'
            except ConnectionResetError:
                pass
        assert request('/api/status')[0] == 200
    finally:
        for s in slow:
            s.close()
    no_listener(rtsp)
    os.kill(proc.pid, signal.SIGHUP)
    for _ in range(100):
        if log_path.read_text().count('central-only Linux x86') >= 2:
            break
        if proc.poll() is not None:
            raise AssertionError('restart exited')
        time.sleep(.1)
    else:
        raise AssertionError('SIGHUP did not exec a new central-only process')
    for _ in range(100):
        try:
            if request('/api/status')[0] == 200:
                break
        except (OSError, http.client.HTTPException):
            pass
        time.sleep(.1)
    else:
        raise AssertionError('HTTP did not recover after exec')
    assert proc.poll() is None
    no_listener(rtsp)
finally:
    proc.terminate()
    proc.wait(timeout=15)
    log.close()
    assert config.read_bytes() == original, 'central-only changed configuration'
