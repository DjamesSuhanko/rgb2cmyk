#!/usr/bin/env python

import socket
import time
def client_program():
    host = "192.168.1.209"  # as both code is running on same pc
    port = 1234  # socket server port number

    client_socket = socket.socket()  # instantiate
    client_socket.connect((host, port))  # connect to the server

    message = b'\x5e\x0f\x0f\x0f\x05\x24'
    client_socket.send(message)  # send message
    
    
    time.sleep(3)
    client_socket.close()  # close the connection

    
if __name__ == '__main__':
    client_program()
