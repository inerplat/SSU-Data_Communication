import requests
import json
def getOAuthTocken(apiKey):
    tockenURL = 'https://api.cognitive.microsoft.com/sts/v1.0/issueToken'
    fetchTockenHeader = {
            'Content-type' : 'application/x-www-form-urlencoded',
            'Content-Length' : '0',
            'Ocp-Apim-Subscription-Key' : apiKey
    }
    tockenResponse = requests.post(tockenURL, headers=fetchTockenHeader)
    OAuthTocken=tockenResponse.content.decode('utf-8')
    return OAuthTocken

def STT(voiceDataPath, OAuthTocken, outputFormat):
    recogMode = 'interactive'
    language = 'ko-KR'
    apiURL = 'https://speech.platform.bing.com/speech/recognition/'+ recogMode + '/cognitiveservices/v1?language=' + language + '&format=' + outputFormat
    
    requestHeader = {
            #'Ocp-Apim-Subscription-Key' : apiKey,
            'Authorization' : 'Bearer '+ OAuthTocken,
            'Content-type' : 'audio/wav; codec=audio/pcm; samplerate=16000'
    }
    requestBody = open(voiceDataPath, 'rb').read()
    response = requests.post(apiURL, data=requestBody, headers=requestHeader)
    decodedResponse=response.content.decode('utf-8')
    return decodedResponse

def preProc(jsonData):
    if jsonData['RecognitionStatus']=='InitialSilenceTimeout':
        return ''
    text = ''
    for nowData in jsonData['NBest']:
        if(nowData['Confidence']>0.5):
            text+=nowData['MaskedITN']+' '
    return text

if __name__ == "__main__" :
    oauth=getOAuthTocken(open('apiKey.key','rb').read())
    #print(STT('output.wav',oauth, 'detailed'))
    print(STT('output.wav',oauth,'simple'))