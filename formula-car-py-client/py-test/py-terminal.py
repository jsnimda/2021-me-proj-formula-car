#!/usr/bin/env python3

import select
import socket
import time

HOST = 'esp32-js.local'  # The server's hostname or IP address
PORT = 23        # The port used by the server

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.connect((HOST, PORT))

    for i in range(1000):
        r, w, err = select.select([s], [], [], 0)
        for rs in r:  # iterate through readable sockets
            if rs is s:
                # read from a client
                data = rs.recv(1024)
                if not data:
                    print('\r{}:'.format(rs.getpeername()), 'disconnected')
                    rs.close()
                else:
                    print('\r{}:'.format(rs.getpeername()), data)
        s.sendall(b'Hello, world\r\n')
        time.sleep(1)

# print('Received', repr(data))
