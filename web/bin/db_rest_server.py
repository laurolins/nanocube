from flask import Flask,request,make_response
from flask_cors import cross_origin
app = Flask(__name__)

import pandas as pd
from sqlalchemy import create_engine
engine = create_engine('sqlite:////crime.db')


@app.route('/')
def hello_world():
    return 'Hello, World!'

import json

@app.route('/data',methods=['POST'])
@cross_origin()
def data():    
    if request.method == 'POST':
        q = request.form['q']
        format = request.form['format']
        resp = make_response()
        print(q)
        
        data=pd.read_sql(q,engine).replace(to_replace=float('nan'),value=None)
        mime=''

        if format=='json':
            data = json.dumps(data.to_dict(orient='records'))
            mime = 'text/json'
        
        if format=='csv':
            data = data.to_csv(index=False)
            mime = 'text/csv'

        resp.data=data
        resp.mimetype=mime

        return resp
    else:
        return ''
