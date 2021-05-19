
# by default, have to use cp950 to decode subprocess.Popen(...) output
# change it to utf-8
import ctypes
if not ctypes.windll.kernel32.SetConsoleOutputCP(65001):
    raise ctypes.WinError(ctypes.get_last_error())

import subprocess
import re


def ping_command(hostname, print_if_fail=False):
    """
    return (result, ping), where result is bool, ping is int string in ms or '<1'
    """
    proc = subprocess.Popen(
        ['ping', '-w', '2000', '-n', '1', hostname],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )

    stdout, stderr = proc.communicate()

    if proc.returncode != 0:
        if print_if_fail:
            print(stdout.decode('utf-8'))
        return (False, '?')

    response = stdout.decode('utf-8')
    res = re.search(r'(<?\d+)\D*?ms', response, re.MULTILINE)
    if res is None:
        if print_if_fail:
            print(stdout.decode('utf-8'))
        return (True, '?')
    ping = res.group(1)
    return (True, ping)


def _test(hostname):
    print(f'{hostname}: { ping_command(hostname) }')


if __name__ == '__main__':
    print('timeout test:')
    _test('1.2.3.4')  # time out test
    _test('domain.local')
    _test('esp32-js.local')
    print('normal test:')
    _test('google.com')
    _test('domain.test')
    _test('8.8.8.8')
    _test('1.1.1.1')
    _test('localhost')
