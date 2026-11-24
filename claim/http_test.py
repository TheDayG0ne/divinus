import http.client
import pathlib
import subprocess
import time
import shutil
import atexit
import base64
from http.client import RemoteDisconnected
from urllib.parse import urlencode

root = pathlib.Path('/tmp/divinus-http-test')
shutil.rmtree(root, ignore_errors=True)
root.mkdir(mode=0o700)
atexit.register(shutil.rmtree, root, ignore_errors=True)
(root / 'eula').mkdir()
shadow = root / 'shadow'
original = 'root::1:2:3:4:5:6:7\n'
shadow.write_text(original)
(root / 'eula/eula.en.txt').write_text('Majestic EULA\nVersion 1.0, 1 September 2026\n')
(root / 'www').mkdir()
(root / 'www/index.html').write_text('external webui')
(root / 'www/app.js').write_text('console.log(1)')
(root / 'outside.txt').write_text('outside')
try:
    (root / 'www/outside-link').symlink_to(root / 'outside.txt')
except (OSError, NotImplementedError):
    pass
server = subprocess.Popen(['/tmp/http-test'])

def request(method, path, body=None, headers=None):
    headers = dict(headers or {})
    headers.setdefault('Authorization', 'Basic ' + base64.b64encode(b'root:abcdefgh').decode())
    conn = http.client.HTTPConnection('127.0.0.1', 18081, timeout=3)
    conn.request(method, path, body, headers)
    response = conn.getresponse()
    data = response.read()
    conn.close()
    return response.status, data

try:
    time.sleep(1)
    assert request('GET', '/setup.html')[0] == 200
    assert request('GET', '/eula.en.txt')[1].startswith(b'Majestic EULA')
    headers = {'Content-Type': 'application/x-www-form-urlencoded'}
    for password_text, expected in (
        ('abcdefg', 400),
        ('abcdefgh', 200),
        ('паролька', 200),
    ):
        shadow.write_text(original)
        (root / 'eula-accepted').unlink(missing_ok=True)
        form = urlencode({'password': password_text, 'confirm': password_text,
                          'eula': 'accepted', 'eula_lang': 'en'})
        assert request('POST', '/setup', form, headers)[0] == expected
        assert (shadow.read_text() == original) if expected == 400 else (shadow.read_text() != original)
        if expected == 400:
            assert not (root / 'eula-accepted').exists()
    shadow.write_text(original)
    (root / 'eula-accepted').unlink(missing_ok=True)
    malformed = 'password=%C3%28&confirm=%C3%28&eula=accepted&eula_lang=en'
    assert request('POST', '/setup', malformed, headers)[0] == 400
    assert shadow.read_text() == original
    assert not (root / 'eula-accepted').exists()
    password = 'password=abcdefgh&confirm=abcdefgh'
    for suffix in ('', '&eula=no&eula_lang=en', '&eula=accepted&eula=no&eula_lang=en', '&eula=accepted&eula_lang=ru'):
        assert request('POST', '/setup', password + suffix, headers)[0] == 400
        assert shadow.read_text() == original
        assert not (root / 'eula-accepted').exists()
    assert request('POST', '/setup', password + '&eula=accepted&eula_lang=en', headers)[0] == 200
    assert shadow.read_text() != original
    assert (root / 'eula-accepted').read_text().startswith('1.0 en web ')
    assert request('GET', '/', headers={'Authorization': ''})[0] == 401
    assert request('GET', '/', headers={'Authorization': 'Basic cm9vdDpiYWQ='})[0] == 401
    assert request('GET', '/api/status', headers={'Authorization': ''})[0] == 401
    saved_shadow = shadow.read_text()
    shadow.write_text('root:!:1:2:3:4:5:6:7\n')
    assert request('GET', '/')[0] == 401
    shadow.write_text(saved_shadow)
    assert request('GET', '/')[1] == b'external webui'
    assert request('GET', '/app.js')[1] == b'console.log(1)'
    assert request('GET', '/missing.js')[0] == 400
    try:
        traversal_status = request('GET', '/../outside.txt')[0]
    except RemoteDisconnected:
        traversal_status = 400
    assert traversal_status in (400, 404)
    try:
        symlink_status = request('GET', '/outside-link')[0]
    except RemoteDisconnected:
        symlink_status = 400
    assert symlink_status == 400
    assert request('POST', '/setup', password + '&eula=accepted&eula_lang=en', headers)[0] == 403
    print('HTTP EULA tests passed')
finally:
    server.terminate()
    server.wait(timeout=5)
