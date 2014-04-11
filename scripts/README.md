# Scripts for starting nanocubes from a csv file

This procedure assumes your machine is not running anything on ports
8000 and 29512

Requires gcc >=4.7, Boost >= 1.48 and Python pandas (pip install pandas)
(apt-get install g++-4.8 libboost-dev)

0. Installing Pandas in a separate python env (Optional)

        wget http://pypi.python.org/packages/source/v/virtualenv/virtualenv-1.11.4.tar.gz
        tar xfz virtualenv-1.11.4.tar.gz
        python virtualenv-1.11.4/virtualenv.py  myPy
        
        #activate the virtualenv, type "deactivate" to disable the env
        source myPy/bin/activate
        pip install argparse numpy pandas

1. Get Nanocubes from github

        git clone https://github.com/laurolins/nanocube.git
        cd nanocube

2. Switch to "dev" branch

        git fetch
        git checkout dev

3. Build nanocubes

        ./bootstrap
        CXX=g++-4.8 ./configure
        make

4. Start a web server in the "web" directory and send it to background

        cd web
        python -m SimpleHTTPServer &

5. run the script and pipe it to the nanocubes server (I have included a
part of the chicago crime dataset)

        cd ../scripts
        python csv2Nanocube.py --catcol='Primary Type' crime50k.csv | NANOCUBE_BIN=../src  ../src/ncserve --rf=100000 --threads=100

This should be it. Point your browser to http://localhost:8000 for the
viewer

        Parameters for csv2Nanocube.py
        --catcol='Column names of categorical variable'
        --latcol='Column names of latitude'
        --loncol='Column names of longitude'
        --countcol='Column names of the count'
        --timecol='Column names of the time variable'
        --timebinsize='time bin size in seconds(s) minutes(m) hours(h) days(D)
        e.g. 1D/30m/60s'
