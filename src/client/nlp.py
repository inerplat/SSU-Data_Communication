from konlpy.tag import *

def ngram(command):
    temp=command+' '
    for i in range(1,4):
        for j in range(len(command)-i+1):
            temp=temp+command[j:j+i]+' '
    return temp

def NounVerb(rawText):
    kkma=Kkma().pos(rawText)
    tw=Twitter().pos(rawText)
    returnText = ''
    for Kpos,Tpos in zip(kkma,tw):
        if Tpos[1] == 'Noun' or Tpos[1] == 'Verb':
            returnText+=ngram(Tpos[0]) + ' '
        if Kpos[1][0] =='N' or Kpos[1][0] == 'V':
            returnText+=Kpos[0] + ' '
    return returnText


if __name__ == "__main__":
    print(NounVerb('뿌리 깊은 나무는 바람에 아니 흔들려서 꽃 좋고 열매 많으니'))