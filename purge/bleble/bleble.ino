#include <stdio.h>
#include <string.h>
#include <SoftwareSerial.h> //시리얼통신 라이브러리 호출

#define Header0   0xff
#define Header1 0xff
#define Packet   0x09
#define pID   0xfd
#define CMD   0x16
#define DATA1   0x00
 
int blueTx=2;   //Tx (보내는핀 설정)at
int blueRx=3;   //Rx (받는핀 설정)
int robotTx=8;
int robotRx=9;
unsigned char bluetoothInput;
SoftwareSerial mySerial(blueTx, blueRx);  //시리얼 통신을 위한 객체선언
SoftwareSerial robotSerial(robotTx,robotRx);

void robotMotion(int Data)
{
  unsigned char Data_L = Data&0xff, Data_H = Data&0xff00;
  unsigned char buf[6];
  buf[0] = 0xff;
  buf[1] = 0x55;
  buf[2] = Data_L;
  buf[3] = ~Data_L;
  buf[4] = Data_H;
  buf[5] = ~Data_H;
  for(int i=0;i<6;i++) robotSerial.write(buf[i]);
}

void setup() 
{
  Serial.begin(9600);   //시리얼모니터
  mySerial.begin(9600); //블루투스 시리얼
  robotSerial.begin(57600);
}

void loop()
{
  mySerial.listen();
  if(mySerial.available())
  {
    char c=mySerial.read();
    Serial.print(c);
    if(c=='!')
    {
      delay(3);
      char go=mySerial.read()-'0';
      robotSerial.listen();
      switch(go)
      {
        case 1:
          robotMotion(4);
          break;
        case 2:
          robotMotion(8);
          break;
        case 3:
          robotMotion(2);
          break;
        case 4:
          robotMotion(1);
          break;
        case 7:
          robotMotion(64);
          break;
        case 6: 
          robotMotion(64);
          break;
      }
      delay(3000);
      robotMotion(0);
    }
  }
}
