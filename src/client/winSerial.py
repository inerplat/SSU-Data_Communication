import serial

def send(data, port, baudrate):
    sendData = '!'+data
    ser = serial.Serial(port=port,baudrate=baudrate, parity=serial.PARITY_NONE, stopbits=serial.STOPBITS_ONE, bytesize=serial.EIGHTBITS, timeout=0)
    ser.write(bytes(send, encoding='ascii'))
    time.sleep(2)
    ser.close()