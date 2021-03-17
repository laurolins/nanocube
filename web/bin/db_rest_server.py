from fastapi import FastAPI, Form
from fastapi.responses import Response
from fastapi.middleware.cors import CORSMiddleware

import pandas as pd
from sqlalchemy import create_engine

#default arguments
file = './crime50k.db'


#arguments from env
import os
if 'DBFILE' in os.environ:
    file = os.environ['DBFILE']
    
engine = create_engine('sqlite:///'+file)

app = FastAPI()
app.add_middleware(CORSMiddleware,allow_origins=['*'])

@app.post('/data')
def data(q:str=Form(...),format:str=Form(...)):    
    print(q)
    
    data=pd.read_sql(q,engine).replace(to_replace=float('nan'),value=None)

    if format=='json':
        return data.to_dict(orient='records')
    
    if format=='csv':
        return Response(content=data.to_csv(index=False),media_type='text/csv')


import uvicorn
if __name__ == "__main__":
    uvicorn.run("db_rest_server:app")
