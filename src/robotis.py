class RC_100A:
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
        #for packet in instruction:
        #    print("0x%X" % (packet), end="|")
        #print()
        byteData = bytes(bytearray(instruction))
        return byteData
if __name__ == '__main__':
    R1 = RC_100A()
    print(R1.motion(0x1234))


