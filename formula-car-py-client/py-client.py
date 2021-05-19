"""
python version: 3.7.9
"""

import time
import re
from datetime import datetime
import threading
import asyncio
import serial
from functools import partial

from ping_command import ping_command

import gevent
import eel
eel.init('web')

hostname = 'esp32-js.local'
port_url = f'socket://{hostname}:23'

# ============
# data
# ============
"""
data structure:
    connection
        status: offline | online | connected
        ping:   int string in ms | '<1'
    device
        ...
    out[]
        ind:    int index
        time:   datetime.now().strftime('%H:%M:%S.%f')[:-3]
        type:   '>' | '$' | '#'  (output/input/py msg)
        line:   str
    in[str]
"""

client = {
    'connection': {
        'status': 'offline',  # offline/online/connected
        'ping': 0,  # in ms
    },
    'device': {

    },
    'out': [],
    'in': [],
}

o_item = None  # current out item index, new line -> set to None
o_CR = False  # CRLF, true if last is CR


def on_receive_text(s):
    if len(s) == 0:
        return
    global o_item, o_CR
    if o_CR and s[0] == '\n':
        s = s[1:]
    o_CR = len(s) > 0 and s[-1] == '\r'
    if len(s) == 0:
        return
    items = re.split(r'\r\n|\r|\n', s)

    def handle(line):
        if o_item is not None:
            return _concat(o_item, line)
        else:
            return _append('>', line)
    for line in items[:-1]:  # handle item until [len - 1]
        handle(line)
        o_item = None
    if len(items[-1]) > 0:
        o_item = handle(items[-1])
    else:
        o_item = None


def _concat(o_item, line):
    o_item['line'] += line
    _notify_append(len(client['out']) - o_item['ind'])
    return o_item


def _append(type, line):
    o_item = {
        'ind': len(client['out']),
        'time': datetime.now().strftime('%H:%M:%S.%f')[:-3],
        'type': type,
        'line': line
    }
    client['out'].append(o_item)
    _notify_append(0)
    return o_item


def _notify_append(last_n):
    if last_n == 0:
        hub_run(eel.append, client['out'][-1], 0)
    else:
        hub_run(eel.append, client['out'][-last_n], last_n)


def py_msg(s):
    print(s)
    _append('#', s)


def on_input_line(s):
    _append('$', s)
    write_to_socket(s + '\r\n')

# all function in this section should run in async thread


# ============
# eel expose
# ============
"""
js side:
    # eel.set_client(client)
    eel.set_device(device)
    eel.set_connection(connection)
    eel.append(out_item, last_n)  // apppend last line
        last_n = 0 -> insert new out_item
        last_n >= 1 -> replace [len - last_n] to out_item
"""


@eel.expose
def get_client():
    return client


@eel.expose
def send_line(s):
    if len(s) > 0 and (len(client['in']) == 0 or client['in'][-1] != s):
        client['in'].append(s)
    loop.call_soon_threadsafe(partial(on_input_line, s))


def notify_connection():
    hub_run(eel.set_connection, client['connection'])


def notify_device():
    hub_run(eel.set_device, client['device'])

# ============
# socket
# ============


def _write_to_socket_async_thread(s: str):
    try:
        if ser is not None and not ser.closed:
            ser.write(s.encode('utf-8'))
        else:
            py_msg('Warning: Socket is not ready!')
    except Exception as e:
        py_msg('Error occurs while writing to socket:')
        py_msg(repr(e))


def write_to_socket(s):
    loop.call_soon_threadsafe(
        partial(_write_to_socket_async_thread, s))

# ============
# main
# ============


hub = gevent.get_hub()
loop = None
ser = None  # type: serial.Serial


def hub_run(func, *args, **kwargs):
    hub.loop.run_callback_threadsafe(partial(func, *args, **kwargs))


def my_other_thread():
    global loop, ser
    tskPing = None
    tskSocket = None

    async def main():
        nonlocal tskPing
        tskPing = loop.create_task(ping_task())

        await asyncio.wait([tskPing])

    async def ping_task():
        nonlocal tskSocket
        first_ping = True
        while True:
            target_up, ping = await loop.run_in_executor(
                None, partial(ping_command, hostname))
            old_status = client['connection']['status']
            client['connection']['ping'] = ping
            if old_status == 'offline' and target_up:
                if ser is None or ser.closed:
                    client['connection']['status'] = 'online'
                else:
                    client['connection']['status'] = 'connected'
                py_msg(f'Found device {hostname}')
                if tskSocket == None or tskSocket.done():
                    tskSocket = loop.create_task(socket_task())
            elif old_status != 'offline' and not target_up:
                client['connection']['status'] = 'offline'
                py_msg(f'Device {hostname} is down')
            elif first_ping:
                py_msg('{hostname} not found')
            first_ping = False
            notify_connection()
            await asyncio.sleep(1.0)

    async def socket_task():
        global ser
        ser = None
        while client['connection']['status'] != 'offline':
            ser = None
            try:
                py_msg('Connecting socket...')
                ser = await loop.run_in_executor(
                    None, partial(serial.serial_for_url, port_url, timeout=0))
                py_msg('Socket connected')
                client['connection']['status'] = 'connected'
                notify_connection()
                while not ser.closed:
                    while ser.inWaiting():
                        b = ser.read(2048)
                        on_receive_text(b.decode('utf-8'))
                    await asyncio.sleep(1 / 1000.0)
                py_msg('Socket closed')
                ser = None
            except Exception as e:
                py_msg(repr(e))
                py_msg('Socket disconnected')
                if ser is not None:
                    ser.close()
                    ser = None
            await asyncio.sleep(1.0)

    # time.sleep(0.1)  # wait a little bit
    print('async ready')
    loop = asyncio.new_event_loop()
    asyncio.set_event_loop(loop)
    loop.run_until_complete(main())
    loop.close()


eel.start('client.html', size=(1440, 900), block=False)

print('eel ready')

thread = threading.Thread(target=my_other_thread, daemon=True)
thread.start()
# NOTICE: threading.Thread may cause failed with KeyError

# eel.spawn(my_other_thread)

while True:
    eel.sleep(1.0)
