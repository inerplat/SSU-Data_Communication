
# coding: utf-8

# In[38]:


from sklearn.model_selection import train_test_split
import re
import sys
from sklearn.externals import  joblib
from sklearn.model_selection import GridSearchCV
from sklearn.svm import  LinearSVC
from sklearn.linear_model import LogisticRegression
from sklearn.feature_extraction.text import  TfidfVectorizer,CountVectorizer
import os
import numpy as np
import string
import json
import nltk
import pyaudio
from konlpy.tag import *
import serial
import pyaudio
import wave
import time
import urllib3
import json
import base64
kkma=Kkma()
tw=Twitter()
po = pyaudio.PyAudio()
for index in range(po.get_device_count()): 
    desc = po.get_device_info_by_index(index)
    #if desc["name"] == "record":
    #print ("DEVICE: %s  INDEX:  %s  RATE:  %s " %  (desc["name"], index,  int(desc["defaultSampleRate"])))

    
ser = serial.Serial(
    port='COM4',\
    baudrate=9600,\
    parity=serial.PARITY_NONE,\
    stopbits=serial.STOPBITS_ONE,\
    bytesize=serial.EIGHTBITS,\
        timeout=0)
#print(ser.portstr) #연결된 포트 확인.

CHUNK = 1024
FORMAT = pyaudio.paInt16
CHANNELS = 1
RATE = 16000
RECORD_SECONDS = 3
WAVE_OUTPUT_FILENAME = "output.wav"

p = pyaudio.PyAudio()


stream = p.open(format=FORMAT,
                channels=CHANNELS,
                rate=RATE,
                input=True,
                frames_per_buffer=CHUNK)

print("Start to record the audio.")
frames = []

for i in range(0, int(RATE / CHUNK * RECORD_SECONDS)):
    data = stream.read(CHUNK)
    frames.append(data)

print("Recording is finished.")

stream.stop_stream()
stream.close()
p.terminate()

wf = wave.open(WAVE_OUTPUT_FILENAME, 'wb')
wf.setnchannels(CHANNELS)
wf.setsampwidth(p.get_sample_size(FORMAT))
wf.setframerate(RATE)
wf.writeframes(b''.join(frames))
wf.close()
openApiURL = "http://aiopen.etri.re.kr:8000/WiseASR/Recognition"
accessKey = ""
audioFilePath = "output.wav"
languageCode = "korean"

file = open(audioFilePath, "rb")
audioContents = base64.b64encode(file.read()).decode("utf8")
file.close()

requestJson = {
    "access_key": accessKey,
    "argument": {
        "language_code": languageCode,
        "audio": audioContents
    }
}

http = urllib3.PoolManager()
response = http.request(
    "POST",
    openApiURL,
    headers={"Content-Type": "application/json; charset=UTF-8"},
    body=json.dumps(requestJson)
)

#print("[responseCode] " + str(response.status))
#print("[responBody]")
jsonData=json.loads(response.data.decode('utf-8'))
resultT=jsonData['return_object']['recognized']
print(resultT)
x_text_list=[]
f = open("x_text_list.out", 'r')
lines=f.readlines()
for line in lines:
    x_text_list.append(line.strip('\n'))
gsvc=joblib.load('model.pkl')
vectorizer = CountVectorizer()
vectorizer.fit_transform(x_text_list)
arr=tw.pos(resultT)
arr2=kkma.pos(resultT)
tmp=''
for arr in arr:
    if arr[1]=='Noun' or arr[1]=='Verb':
        tmp=tmp+arr[0]+' '
for arr2 in arr2:
    if arr2[1][0]=='N' or arr2[1][0]=='V':
        tmp=tmp+arr2[0]+' '
predict=[]
predict.append(tmp)
Result = gsvc.predict(vectorizer.transform(predict))
print(Result[0])
send='!'+str(Result[0])
ser.write(bytes(send, encoding='ascii')) #출력방식1
time.sleep(3)
ser.close()

