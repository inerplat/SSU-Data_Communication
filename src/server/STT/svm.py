from sklearn.externals import  joblib
from sklearn.model_selection import GridSearchCV
from sklearn.feature_extraction.text import  TfidfVectorizer,CountVectorizer
def predict(model, vec, wordList):
    x_text_list=[]
    f = open(vec, 'r')
    lines=f.readlines()
    for line in lines:
        x_text_list.append(line.strip('\n'))
    gscv=joblib.load(model)
    vectorizer = CountVectorizer()
    vectorizer.fit_transform(x_text_list)
    return gscv.predict(vectorizer.transform(wordList))

if __name__ == "__main__":
    testList=['왼 돌아']
    print(predict("model.pkl", "x_text_list.out", testList))
