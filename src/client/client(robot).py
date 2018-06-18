import sys
# coding: utf-8

# In[11]:


import serial
import time
import socket
import RPi.GPIO as GPIO 


# In[12]:


motionDict = {
    1 : 16,
    2: 32,
    3: 8,
    4: 4,
    5: 256,
    6: 512,
    8: 1,
    9: 128,
    10: 2
}
protocol ={
    'INT' : 0,
    'CHAR' : 1,
    'STRING' : 2,
    'WAV' : 3,
    'SENDCTRL' : 0x80000000,
    'SENDSERV' : 0x40000000,
    'SENDROBO' : 0x20000000,
    'RECVCTRL' : 0x08000000,
    'RECVSERV' : 0x04000000,
    'RECVROBO' : 0x02000000,
    'INIT' : 0x00000001,
    'QUIT' : 0x00000002,
    'LIST' : 0x00000003,
    'SEND' : 0x00000004,
    'LINK' : 0x00000005
}


# In[13]:


class Robotis:
    def __init__(self): 
        self.HEADER0 = 0xff
        self.HEADER1 = 0x55
        self.DATA_L = 0x00
        self.DATA_H = 0x00
    def motion(self, data):
        instruction = [self.HEADER0, self.HEADER1]
        self.DATA_L = data & 0xff
        self.DATA_H = (data>>8) & 0xff
        instruction.append(self.DATA_L)
        instruction.append((~self.DATA_L)&0xff)
        instruction.append(self.DATA_H)
        instruction.append((~self.DATA_H)&0xff)
        for packet in instruction:
            print("0x%X" % (packet), end="|")
        print()
        byteData = bytes(bytearray(instruction))
        print(byteData)
        ser = serial.Serial('/dev/ttyAMA0', 57600)
        ser.write(byteData)
        time.sleep(3)
        ser.write(b'\xffU\x00\xff\x00\xff')
        return byteData
    def connect(self, host, port):
        self.s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.s.connect((host,port))
        self.s.send((protocol['SENDROBO']|protocol['RECVSERV']|protocol['INIT']).to_bytes(4,'little'))
        self.s.send((1).to_bytes(4,'little'))
        self.s.send((2).to_bytes(4,'little'))
        self.s.send((2).to_bytes(4,'little'))
        self.s.send((7).to_bytes(4,'little'))
        self.s.send('robotis'.encode('utf-8'))
        self.s.send((15).to_bytes(4,'little'))
        self.s.send('ROBOTIS_PREMIUM'.encode('utf-8'))
    def readData(self):
        for i in range(0, 5):
            int.from_bytes(self.s.recv(4), byteorder='little')
        body = int.from_bytes(self.s.recv(4), byteorder='little')
        print(body)
        return body


# In[14]:


class RCcar:
    def __init__(self):
        GPIO.setmode(GPIO.BOARD)
        for i in range(35,39):
            GPIO.setup(i, GPIO.OUT)
            GPIO.output(i, 0)
    def connect(self,host,port):
        self.s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.s.connect((host,port))
        self.s.send((protocol['SENDROBO']|protocol['RECVSERV']|protocol['INIT']).to_bytes(4,'little'))
        self.s.send((1).to_bytes(4,'little'))
        self.s.send((2).to_bytes(4,'little'))
        self.s.send((2).to_bytes(4,'little'))
        self.s.send((7).to_bytes(4,'little'))
        self.s.send('Char'.encode('utf-8'))
        self.s.send((5).to_bytes(4,'little'))
        self.s.send('RCcar'.encode('utf-8'))
    def readData(self):
        for i in range(0, 5):
            int.from_bytes(self.s.recv(4), byteorder='little')
            body = int.from_bytes(self.s.recv(4), byteorder='little')
            print(body)
            return body
    def parseData(self, rawData):
        cmd=[]
        cmd.append((rawData>>3)&0x01)
        cmd.append((rawData>>2)&0x01)
        cmd.append((rawData>>1)&0x01)
        cmd.append(rawData&0x01)
        return cmd
    def moveInit(self):
        for i in range(35,39):
            GPIO.setup(i, GPIO.OUT)
            GPIO.output(i, 0)
    def moveFoward(self):
        GPIO.output(35,1)
    def moveBack(self):
        GPIO.output(36,1)
    def moveLeft(self):
        GPIO.output(37,1)
    def moveRight(self):
        GPIO.output(38,1)


# In[15]:

if __name__ == "__main__":
    if len(sys.argv)!=2:
        print(USAGE : [filename] [Robotname])
    if argv[2] == 'Robotis':
        R = Robotis()
    elif argv[2] == 'RCcar':
        R = RCcar()
    else:
        print("This program can't control that robot")
        sys.exit()
    R.connect('220.149.85.240',9002)
    while 1:
        motionNo = R.readData()
        R.motion(motionDict[motionNo])

