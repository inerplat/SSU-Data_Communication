# coding: utf-8
import requests
import json

def STT(voiceDataPath):
    recogMode = 'interactive'
    language = 'ko-KR'
    outputFormat = 'simple'
    apiKey = ''
    apiURL = 'https://speech.platform.bing.com/speech/recognition/'+ recogMode + '/cognitiveservices/v1?language=' + language + '&format=' + outputFormat
    
    requestHeader = {
            'Ocp-Apim-Subscription-Key' : apiKey,
            'Content-type' : 'audio/wav; codec=audio/pcm; samplerate=16000'
    }
    
    requestBody = open(voiceDataPath, 'rb').read()
    response = requests.post(apiURL, data=requestBody, headers=requestHeader)
    responseJson = json.loads(response.content.decode('utf-8'))
    return responseJson['DisplayText']

