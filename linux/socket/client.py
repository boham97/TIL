import socket
import threading

def recv_data(client_socket):
    print("thread start:")
    while 1:
        data = client_socket.recv(30)
        if len(data) == 0:                                          #소켓 연결 종료시 종료
            break
        print("recive : ", data.decode('utf-8').split('\x00')[0])
    print("thread end:", data)


if __name__ == "__main__":
    host = socket.gethostbyname(socket.gethostname())
    port = 1234
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    client_socket.connect((host, port))
    thread = threading.Thread(target=recv_data, args=(client_socket,))
    thread.start()
    print('>> Connect Server')

    while True:
        message = input() + '\0'
        client_socket.send(message.encode())
        if message == 'exit\0':
            close_data = message
            break
    thread.join()
    print("thread end:", close_data)   
    client_socket.close()
