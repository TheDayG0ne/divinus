import http.client
import pathlib
import subprocess
import time
import shutil
import atexit

root = pathlib.Path('/tmp/divinus-http-test')
shutil.rmtree(root, ignore_errors=True)
root.mkdir(mode=0o700)
atexit.register(shutil.rmtree, root, ignore_errors=True)
(root / 'eula').mkdir()
shadow = root / 'shadow'
original = 'root::1:2:3:4:5:6:7\n'
shadow.write_text(original)
(root / 'eula/eula.en.txt').write_text('Majestic EULA\nVersion 1.0, 1 September 2026\n')
server = subprocess.Popen(['/tmp/http-test'])

def request(method, path, body=None, headers={}):
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
    short_password = 'password=abcdefg&confirm=abcdefg'
    assert request('POST', '/setup', short_password + '&eula=accepted&eula_lang=en', headers)[0] == 400
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
    assert request('POST', '/setup', password + '&eula=accepted&eula_lang=en', headers)[0] == 403
    print('HTTP EULA tests passed')
finally:
    server.terminate()
    server.wait(timeout=5)
