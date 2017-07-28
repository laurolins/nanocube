# Javascript web frontend for nanocubes

Setup

```
# Assuimg we started a server on localhost:12345

# clone repo
git clone git@gitlab.research.att.com:andrew7jin/nanocube.git

# checkout webgui-dev branch
cd nanocube
git checkout webgui-dev

# install packages
cd extra/webgui-dev

# download/install dependencies
npm install

# check if files do all fit together (compiling scripts into a single script)
./node_modules/.bin/grunt

# create config script
python ./scripts/genconfig.py -s http://localhost:12345 > chicago.json

# start a front-end server
# python -m SimpleHTTPServer 8080
./node_modules/.bin/http-server

# go to browser and url
http://localhost:8080/?config=chicago.json
```
