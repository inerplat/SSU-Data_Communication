import azureAPI
import speech2file
import nlp
import svm
import json
import winSerial
noMotion=0


apiKey = open('apiKey.key','rb').read()
OAuth = azureAPI.getOAuthTocken(apiKey)
speech2file.makeFile('output.wav',3)
rawData = azureAPI.STT('output.wav',OAuth, 'detailed')
jsonData = json.loads(rawData)
print(jsonData)

text = azureAPI.preProc(jsonData)


if text=='':
    result = noMotion
else:
    predict = [nlp.NounVerb(text)]
    result = svm.predict("model.pkl", "x_text_list.out", predict)

print(result)

winSerial.send(result, 'COM4', 9600)

