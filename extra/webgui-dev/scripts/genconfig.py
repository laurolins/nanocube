from __future__ import print_function, division

import json,sys,argparse,requests

try:
    from urllib.request import urlopen
except ImportError:
    from urllib2 import urlopen

def initConfig(name,url):
    config = {}
    config['nanocube'] = {name: {'url':url}}
    config['datasrc'] = {name:{'expr':name, 'colormap': 'Reds'}}
    config['widget'] = {}
    return config


def main():
    #parse the simple arguments
    argparser = argparse.ArgumentParser()
    argparser.add_argument('--width',type=int,default=1920)
    argparser.add_argument('--height',type=int,default=1080)
    argparser.add_argument('-s','--server',type=str)
    args = argparser.parse_args()
    if args.server is None:
        sys.stderr.writelines(['Please specify a nanocube server with -s\n'])
        sys.exit(1)


    #download the schema
    url = args.server
    try:
        #ctx = ssl.create_default_context()
        #ctx.check_hostname = False
        #ctx.verify_mode = ssl.CERT_NONE
        #response = urlopen(url+'/schema',context=ctx)
        #schema = json.loads(response.read())
        r = requests.get(url+'/schema', allow_redirects=False,verify=False)
        schema = r.json() #json.loads(r.read())
    except:
        print ('Fail to read schema from %s'%(url),file=sys.stderr)
        sys.exit(1)

    spatialvars = [ x for x in schema['fields'] if
                    x['type'].startswith('nc_dim_quadtree')]
    catvars = [ x for x in schema['fields'] if
                x['type'].startswith('nc_dim_cat')]
    timevars = [ x for x in schema['fields'] if
                 x['type'].startswith('nc_dim_time')]


    schema['metadata'] = {s['key']:s['value'] for s in schema['metadata']}
    ncname = schema['metadata']['name']
    ncname = ncname.replace(' ','_').replace('-','_').replace('.','_')
    config = initConfig(ncname, url)

    sp = {v['name']: spatialWidget(v,w=1.0/len(spatialvars)) for
          v in spatialvars}
    ts = {v['name']: timeseriesWidget(v,w=0.7*args.width) for
          v in timevars}
    cats = {v['name']: catWidget(v) for
            v in catvars}

    for c in cats:
        config['widget'][c] = cats[c]

    for s in sp:
        config['widget'][s] = sp[s]

    for t in ts:
        config['widget'][t] = ts[t]

    print(json.dumps(config,indent=2))


def spatialWidget(v,w=1.0):
    levels = v['type'].replace('nc_dim_quadtree_','')
    levels = int(levels)
    return {
        'type':'spatial',
        'title': v['name'],
        'tilesurl':'http://{s}.tile.stamen.com/toner-lite/{z}/{x}/{y}.png',
        'coarse_offset':1,
        'viewbox':[[-85,-180],[85,180]],
        'levels':levels,
        'css':{
            'width':'%d%%'%(int(w*100)),
            'height': '100%',
            'float':'left'
        }
    }

def timeseriesWidget(v,w):
    return {
        'title': v['name'],
        'type':'time',

        'css':{
            'opacity': 0.8,
            'bottom': '30px',
            'height': '100px',
            'width': '%dpx'%(int(w)),
            'position': 'absolute',
            'background-color': '#555',
            'left': '30px'
        }
    }

def catWidget(v):
    maxkeylen = max([ len(k) for k in v['valnames'].keys() ])
    marginleft = int(maxkeylen*11)
    width = marginleft+200
    height = min(200,len(v['valnames'].keys())*20+40)

    return {
        'title':v['name'],
        'logaxis': False,
        'alpha_order': True,
        'type':'cat',        
        
        'css':{
            'opacity': 0.8,
            'height': '%dpx'%(height),
            'width': '%dpx'%(width),
        }
    }

if __name__ == '__main__':
    main()
