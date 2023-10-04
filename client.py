import socket
import errno

def clear_socket_buffer(sock):
    """Clear out the socket's receive buffer."""
    sock.setblocking(0)  # Set the socket to non-blocking mode
    try:
        while True:
            data = sock.recv(1024)
            if not data:
                break
    except socket.error as e:
        if e.args[0] not in (errno.EAGAIN, errno.EWOULDBLOCK):
            raise
    finally:
        sock.setblocking(1)  # Set the socket back to blocking mode

def main():
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_ip = '127.0.0.1'
    server_port = 8888

    # Set a timeout for the socket operations
    client_socket.settimeout(5.0)  # Set timeout to 5 seconds

    try:
        client_socket.connect((server_ip, server_port))
        print(f"Connected to server {server_ip}:{server_port}")
    except socket.timeout:
        print("Connection attempt timed out. Exiting.")
        return

    while True:
        message = input("Enter message to send to server (or 'exit' to quit): ")

        if message.lower() == 'exit':
            print("Exiting.")
            break

        clear_socket_buffer(client_socket)
        client_socket.send(message.encode())

        try:
            # Receive data with a timeout
            response = client_socket.recv(1024)
            print(f"Received from server: {response.decode()}")
        except socket.timeout:
            print("Waiting for the server response timed out. Please try again.")

    client_socket.close()

if __name__ == "__main__":
    main()
