
# coding: utf-8

# In[1]:


import os
import socket
import speech2file
def cls():
    os.system('cls' if os.name=='nt' else 'clear')


# In[2]:


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
    'LINK' : 0x00000004,
    'VOICE' : 0x00000005,
    'CMDN' : 0x00000006,
    'FIN' : 0x00000007,
    'CMDT' : 0x00000008,
    'SEND': 0x00000009,
}


# In[3]:


class control:
    def __init__(self):
        self.status = 0
        self.s = 0
        self.robotID=0
        self.rbList = []
    def parse(self):
        usage = self.s.recv(4)
        typeCnt = int.from_bytes(self.s.recv(4), byteorder='litte') << 3
        bodyList = []
        for i in range(int(bodySize/4)):
            bodyList.append(self.s.recv(4))
    def connect(self, host, port):
        if self.status == 1:
            print('You are already connected to the server.')
            return
        self.s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.s.connect((host,port))
        self.s.send((protocol['SENDCTRL']|protocol['RECVSERV']|protocol['INIT']).to_bytes(4,'little'))
        self.s.send((0).to_bytes(4,'little'))
        self.status = 1
        cls()
        print('Server connect successful!')
    def requestList(self):
        self.s.send((protocol['SENDCTRL']|protocol['RECVSERV']|protocol['LIST']).to_bytes(4,'little'))
        self.s.send((0).to_bytes(4,'little'))
        header1 = self.s.recv(4)
        header2 = self.s.recv(4)
        bodySize = int.from_bytes(header2, byteorder='little') << 3
        bodyList=[]
        for i in range(int(bodySize/4)):
            bodyList.append(self.s.recv(4))
        data = ''
        robotDict ={}
        dataSize = []
        for now in range(int.from_bytes(bodyList[1], byteorder='little')):
            byte = int.from_bytes(self.s.recv(4), byteorder='little')
            dataSize.append(byte)
            for i in range(byte):
                data+=self.s.recv(1).decode()
        realData = data.replace('\0','')
        readByte = 0
        dataList = []
        #print(dataSize)
        for size in dataSize:
            dataList.append(realData[readByte:readByte+size-1])
            readByte=readByte+size-1
        for i in range(0,len(dataList),3):
            if dataList[i+2]=='0':
                robotDict[dataList[i]] = dataList[i+1]
        print('You can use the robot under the list\n--------------------\n',robotDict)
        print('Enter the number of robots : ')
        robotCnt = input()
        for robot in range(int(robotCnt)):
            print('Enter the ID of the robot : ')
            self.robotID = int(input())
            self.s.send((protocol['SENDCTRL']|protocol['RECVSERV']|protocol['LINK']).to_bytes(4,'little'))
            self.s.send((1).to_bytes(4,'little'))
            self.s.send((0).to_bytes(4,'little'))
            self.s.send((1).to_bytes(4,'little'))
            self.s.send((4).to_bytes(4,'little'))
            self.s.send((self.robotID).to_bytes(4,'little'))
            self.rbList.append(self.robotID)
        cls()
        print('Robot connect successful!')
    def record(self, fileName):
        speech2file.makeFile(fileName, 4)
    def sendVoice(self, fileName):
        f=open(fileName,'rb').read()
        size = os.stat(fileName).st_size
        self.s.send((protocol['SENDCTRL']|protocol['RECVSERV']|protocol['VOICE']).to_bytes(4,'little'))
        self.s.send((2).to_bytes(4,'little'))
        self.s.send((0).to_bytes(4,'little'))
        self.s.send((1).to_bytes(4,'little'))
        self.s.send((3).to_bytes(4,'little'))
        self.s.send((1).to_bytes(4,'little'))
        self.s.send((4).to_bytes(4,'little'))
        self.s.send((self.robotID).to_bytes(4,'little'))
        self.s.send((size).to_bytes(4,'little'))
        self.s.send(f)
        cls()
        print('File send sucessful!')
    def sendCmd(self):
        print('Control Robot using command (exit : x)')
        while 1:
            print('Input command(W, A, S ,D ,L, R) : ')
            cmd = input()
            if cmd == 'x' or cmd=='X':
                break
            for robot in self.rbList : 
                self.s.send((protocol['SENDCTRL']|protocol['RECVSERV']|protocol['CMDN']).to_bytes(4,'little'))
                self.s.send((1).to_bytes(4,'little'))
                self.s.send((0).to_bytes(4,'little'))
                self.s.send((2).to_bytes(4,'little'))
                self.s.send((4).to_bytes(4,'little'))
                self.s.send((robot).to_bytes(4,'little'))
                self.s.send((4).to_bytes(4,'little'))
                
                if cmd == 'w' or cmd=='W':
                    self.s.send((1).to_bytes(4,'little'))
                if cmd == 'a' or  cmd=='A':
                    self.s.send((2).to_bytes(4,'little'))
                if cmd == 'd' or cmd=='D':
                    self.s.send((3).to_bytes(4,'little'))
                if cmd == 'l' or cmd=='L':
                    self.s.send((4).to_bytes(4,'little'))
                if cmd == 'r' or cmd=='R':
                    self.s.send((5).to_bytes(4,'little'))
                
    def sendText(self):
        text = input()
        for robot in self.rbList : 
                self.s.send((protocol['SENDCTRL']|protocol['RECVSERV']|protocol['CMDN']).to_bytes(4,'little'))
                self.s.send((1).to_bytes(4,'little'))
                self.s.send((0).to_bytes(4,'little'))
                self.s.send((2).to_bytes(4,'little'))
                self.s.send((4).to_bytes(4,'little'))
                self.s.send((robot).to_bytes(4,'little'))
                self.s.send((len(text)).to_bytes(4,'little'))
                self.s.send(text)
    def close(self):
        s.close()
    def chkConnect(self):
        if self.status==0:
            print('Not connected with server.\n')
        return self.status


# In[4]:




# In[5]:

if __name__ == "__main__":
    client = control()
    print('#################Robot control client#################')
    while 1:
        print('Enter the command do you want')
        print('[1] Connect with server\n[2] Connect with robot\n[3] Record Voice\n[4] Send voice file to server\n[5] Send command\n[6] Send test\n[7] exit')
        cmd = input()
        if cmd=='1':
            client.connect('220.149.85.240',9002)
        elif cmd=='2' and client.chkConnect():
            client.requestList()
        elif cmd=='3' and client.chkConnect():
            client.record('output.wav')
        elif cmd=='4' and client.chkConnect():
            client.sendVoice('output.wav')
        elif cmd=='5' and client.chkConnect():
            client.sendCmd()
        elif cmd=='6' and client.chkConnect():
            ctlText=input()
            client.sendText(ctlText)
        elif cmd=='7':
            client.close()
            break

