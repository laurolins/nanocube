set -x

#install dependencies
apt-get update 
apt-get install --no-install-recommends --no-install-suggests -y \
        build-essential git nginx ca-certificates

#clone and build
git clone https://github.com/laurolins/nanocube.git /nanocube-src 
cd /nanocube-src 
export CXXFLAGS=-O2
export CFLAGS=-O2
./configure --with-polycover --prefix=/nanocube 
make && make install 

#make sample data directories
mkdir /data /www 
cp -a /nanocube-src/web/dist/* /www 
#sample web config
mv /app/config.json /data

#sample nanocube
cd /nanocube-src
bash -c '/nanocube/bin/nanocube create <(gunzip -c ./data/crime50k.csv.gz) ./data/crime50k.map /data/crime50k.nanocube -header' 

#sample database
apt-get install --no-install-recommends --no-install-suggests -y \
        python3-pip python3 
pip3 install --no-cache-dir fastapi uvicorn[standard] gunicorn \
     python-multipart pandas sqlalchemy

python3 -c 'import pandas as pd;from sqlalchemy import create_engine; d=pd.read_csv("data/crime50k.csv.gz");d=d.rename(columns={"Date":"time","Primary Type":"type"}); d.time=pd.to_datetime(d.time); d.to_sql("crime",create_engine("sqlite:////data/crime50k.db"),index=False)'


#cleanup
apt-get remove --purge -y build-essential git
apt-get autoremove -y && apt-get clean
rm -rf /var/lib/apt/lists/*
rm -rf /nanocube-src 
