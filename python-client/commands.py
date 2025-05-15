# commands.py
# purpose: provide a simple Unix-compatible command-line interface to control an ESP32 device
#          over a TCP socket using keyboard inputs.
# feastures:
#   - creates a TCP connection to the ESP32 access point
#   - sends single-character commands over the socket
#   - non-blocking keyboard input handling for responsive control
# Alisa Yurevich, Tufts University, EE14 spring 2025

import socket  
import sys    
import termios  
import tty      
import time     
import select   

ESP_IP = "192.168.4.1" 
PORT = 80 
SEND_INTERVAL = 0.1 #send interval should not be faster than serial, or i2c transfer rate

def create_connection():
    """
    intializes a tcp socket and connects to the ESP32
    
    returns: 
        socket.socket: the connected socket or none if failure
    """
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        s.connect((ESP_IP, PORT))
        print("Connected to ESP32")
        return s
    except socket.error as e:
        print(f"Failed to connect: {e}")
        return None

def send_command(sock, command):
    """
    sends a single character command to the ESP32 over a TCP socket

    param: 
        sock (socket.socket): active socket connection
        command (str): command to send over
    
    returns: 
        sock (socket.socket), or none if error
    """
    try:
        sock.sendall(f"{command}\n".encode())  #send the command with newline as byte
        print(f"sent: {command}")
    except (socket.error, BrokenPipeError) as e:
        print(f"Connection error: {e}")
        sock.close()
        return None
    return sock

def is_key_pressed():
    """
    checks if a key has been pressed (non-blocking)

    returns:
        bool: true if key available
    """
    return select.select([sys.stdin], [], [], 0) == ([sys.stdin], [], [])

def get_key():
    """
    reads character from stdin and returns it as uppercase (for command sending purposes)

    returns:
        uppercase str command
    """
    return sys.stdin.read(1).upper()

def setup_terminal():
    """
    sets terminal to cbreak mode 

    returns:
        tuple: file descripter and og terminal settings
    """
    fd = sys.stdin.fileno() 
    old_settings = termios.tcgetattr(fd)
    tty.setcbreak(sys.stdin.fileno())  
    return fd, old_settings

def reset_terminal(fd, old_settings): 
    """
    restores terminal to previous settings

    param:
        fd (int): file descriptor for stdin.
        old_settings (list): original terminal settings.
    """
    termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)

#--- MAIN ---#
print("Type to control the ESP32 (Ctrl+C to exit)")
sock = create_connection() # creates connection
fd, old_settings = setup_terminal() # config real time input
try:
    current_key = None
    last_send_time = 0 #time stamp of last command

    while True:
        if is_key_pressed(): 
            key = get_key()
            if key in ("W", "A", "S", "D", "E", "F", "R", "H",): 
                now = time.time()
                if now - last_send_time >= SEND_INTERVAL: #check interval
                    if sock is None:
                        sock = create_connection() #in case of disconnect
                    if sock: 
                        sock = send_command(sock, key) # end
                        last_send_time = now
        time.sleep(0.01) #delay
except KeyboardInterrupt:
    print("\nExciting!")
finally: 
    reset_terminal(fd, old_settings)
    if sock:
        sock.close()
