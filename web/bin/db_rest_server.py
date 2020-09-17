from fastapi import FastAPI, Form
from fastapi.responses import Response
from fastapi.middleware.cors import CORSMiddleware

import pandas as pd
from sqlalchemy import create_engine
engine = create_engine('sqlite:///./crimes50k.db')

app = FastAPI()
app.add_middleware(CORSMiddleware,allow_origins=['*'])

@app.get('/')
def hello_world():
    return {'msg':'Hello, World!'}

@app.post('/data')
def data(q:str=Form(...),format:str=Form(...)):    
    print(q)
        
    data=pd.read_sql(q,engine).replace(to_replace=float('nan'),value=None)

    if format=='json':
        return data.to_dict(orient='records')
    
    if format=='csv':
        return Response(content=data.to_csv(index=False),media_type='text/csv')
