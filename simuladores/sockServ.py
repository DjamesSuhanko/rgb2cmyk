import socket
HOST = '192.168.1.200'
PORT = 1234            
tcp = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
orig = (HOST, PORT)
tcp.bind(orig)
tcp.listen(1)
while True:
    con, client = tcp.accept()
    print( 'Client: ', client)
    while True:
        msg = con.recv(6)
        if not msg: break
        #print( client, msg)
        for i in range(len(msg)):
            print(msg[i])
    print( 'Closing connection to: ', client)
    con.close()
