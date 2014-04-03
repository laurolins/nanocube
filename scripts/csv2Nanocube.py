import sys,dateutil,datetime,argparse,re,subprocess,os,json,socket

import cStringIO as StringIO
import pandas as pd
import numpy as np

class NanocubeInput:
    def __init__(self, args):
        self.name=args.InputFile[0]
        self.timebinsize=self.parseTimeBin(args.timebinsize)

        self.spname=args.spname
        self.catcol=args.catcol
        self.timecol=args.timecol
        self.latcol=args.latcol
        self.loncol=args.loncol
        self.countcol = args.countcol

        self.datefmt=args.datefmt
        self.levels = args.levels
        
        self.field=[]
        self.valname={}
        self.offset=None
        
        self.minlatlon={s:[float("inf"),float("inf")] for s in self.spname}
        self.maxlatlon={s:[float("-inf"),float("-inf")] for s in self.spname}


        #start nanocube
        #cmd = os.environ['NANOCUBE_BIN']+'/ncserve --rf=1000 --threads=100'
        #self.ncproc = subprocess.Popen([cmd],stdin=subprocess.PIPE,
        #                               shell=True)

        self.readcsv(args.InputFile)
        self.writeConfigFile()
        
    def writeConfigFile(self):
        config = {}
        config['div'] = {}
        #map
        for div in self.spname:
            div = div.replace(" ", "_");
            config['div'][div] = {'height':'100%',
                                  'width':'%d%%'%(100/len(self.spname)),
                                  'padding':0, 'margin': 0, 
                                  'float' :'left',
                                  'z-index':0}

        #time
        for i,div in enumerate(self.timecol):
            div = div.replace(" ", "_");
            config['div'][div] = {'position': 'absolute',
                                  'font': '10pt sans-serif',
                                  'height':'100px',
                                  'width': '960px',
                                  'bottom': '%dpx'%(i*(100+10)+10),
                                  'left': '10px',
                                  'background-color':'#555', 
                                  'opacity': 0.8,
                                  'z-index':1}
                                  
        #cat
        top = 20
        for i,div in enumerate(self.catcol):
            nval = len(self.valname[div])
            height = 20*nval+50 #50 is the default margin size
            lmargin = max(30,max([len(k) for k in self.valname[div]])*7)
            div = div.replace(" ", "_");
            config['div'][div] = {'position': 'absolute',
                                  'font': '10pt sans-serif',
                                  'margin-left':'%dpx'%(lmargin+20),
                                  'height':'%dpx'%(height) ,
                                  'width': '%dpx'%(200+lmargin),
                                  'top': '%dpx'%(top),
                                  'right': '10px',
                                  'background-color':'#555', 
                                  'opacity': 0.8,
                                  'z-index':1}
            top +=height+10

        #other info
        config['div']['info'] = {'position': 'absolute',
                                 'font': '10pt sans-serif',
                                 'color': 'white',
                                 'top': '5px',
                                 'right': '5px',
                                 'opacity': 0.9,
                                 'z-index':1}

        config['latlonbox'] = { 'min':self.minlatlon,
                                'max':self.maxlatlon }
        
        config['url'] = 'http://%s:29512'%(socket.getfqdn())
        config['title'] = self.name
        config['tilesurl'] = 'http://{s}.tile.osm.org/{z}/{x}/{y}.png'

        json.dump(config, open(os.environ['NANOCUBE_WEB']
                               + '/config.json','wb'),indent=4);

    def readcsv(self,files):
        coi = self.timecol+self.catcol+self.latcol+self.loncol
        if self.countcol is not None:
            coi += [self.countcol]
        start = True
        for f in files:
            comp = None
            if f.split('.')[-1] == 'gz':
                comp = 'gzip'
            reader = pd.read_csv(f,usecols=coi,
                                 parse_dates=self.timecol,
                                 infer_datetime_format=True,
                                 chunksize=100000,
                                 compression=comp)

            for i,data in enumerate(reader):
                data = data[coi].copy()

                data = self.processData(data)
                if start:
                    sys.stdout.write(self.header(data))
                    start = False
                
                self.writeRawData(data)
                        
    def processData(self, data):
        if self.countcol is None:
            self.countcol = 'count'

        if self.countcol not in data.columns: #add count
            data[self.countcol]=1

        #get NaN's for errors
        data[self.countcol] = data[self.countcol].astype(np.float)

        #drop bad data
        data = data.dropna().copy()
                    
        #process data
        data = self.processLatLon(data)
        data = self.processCat(data)
        data = self.processDate(data)
                                
        #sort by date
        #data = data.sort(self.timecol[0])
        return data
    
    def writeRawData(self,data):
        columns = []
        for i,spname in enumerate(self.spname):
            columns += [self.loncol[i],self.latcol[i]]
            data[self.loncol[i]] = data[self.loncol[i]].astype('<u4'); 
            data[self.latcol[i]] = data[self.latcol[i]].astype('<u4'); 
            
        for i,c in enumerate(self.catcol):                
            columns += [c]
            data[c] = data[c].astype('<u1'); 

        for i,d in enumerate(self.timecol):                
            columns += [d]
            data[d] = data[d].astype('<u2'); 

        columns += [self.countcol]
        data[self.countcol] = data[self.countcol].astype('<u4')

        data = data[columns] #permute

        rec = data.to_records(index=False)
        rec.tofile(sys.stdout)
            
    def processLatLon(self,data):
        for i,spname in enumerate(self.spname):
            lat = self.latcol[i]
            lon = self.loncol[i]
            lvl = self.levels
            data[lon] = data[lon].astype(np.float)
            data[lat] = data[lat].astype(np.float)
            data = data.dropna()

            data = data[data[lon] > -180]
            data = data[data[lon] < 180]
            data = data[data[lat] > -85.0511]
            data = data[data[lat] < 85.0511]

            #update min max latlon
            self.minlatlon[spname][0] = min(data[lat].min(),
                                            self.minlatlon[spname][0])
            self.minlatlon[spname][1] = min(data[lon].min(),
                                            self.minlatlon[spname][1])
            self.maxlatlon[spname][0] = max(data[lat].max(),
                                            self.maxlatlon[spname][0])
            self.maxlatlon[spname][1] = max(data[lon].max(),
                                            self.maxlatlon[spname][1])

            data[lon] = self.lonToTileX(data[lon],lvl)
            data[lat] = self.latToTileY(data[lat],lvl)
        return data
            
    def processDate(self,inputdata):
        data = inputdata.copy()
        for i,d in enumerate(self.timecol): #convert strings to datetime
            data[d] = pd.to_datetime(data[d].apply(str),
                                     infer_datetime_format=True,
                                     format=self.datefmt,coerce=True)
        data = data.dropna()
        
        if self.offset is None: #compute offset
            year = data[self.timecol].min().min().year
            self.offset = datetime.datetime(year=year,month=1,day=1)

        for i,d in enumerate(self.timecol):
            data[d] -= self.offset
            data[d] = data[d] / self.timebinsize
        return data
        
    def processCat(self,data):
        for i,c in enumerate(self.catcol):            
            #fix the spaces
            data[c] = data[c].apply(lambda x : str(x).replace(' ','_'))

            if c not in self.valname:
                self.valname[c] = {}
            labels = np.unique(data[c])

            updateValname = False
            for l in labels:
                if l not in self.valname[c]:
                    updateValname = True                    
                    newid = len(self.valname[c])
                    self.valname[c][l] = newid
                    
            data[c] = data[c].apply(lambda x : self.valname[c][x])
        return data
            
    def latToTileY(self,lat_deg,zoom):
        lat_deg = np.maximum(-85.0511,lat_deg)
        lat_deg = np.minimum(85.0511,lat_deg)
        lat_rad = lat_deg / 180 * np.pi 
        n = 2 ** zoom
        ytile = n*(1-(np.log(np.tan(lat_rad)+1.0/np.cos(lat_rad))/np.pi))/2.0
        return (n-1-ytile) #flip

    def lonToTileX(self,lon_deg,zoom):
        lon_deg = np.maximum(-180,lon_deg)
        lon_deg = np.minimum(180,lon_deg)
        n = 2 ** zoom
        xtile = n*((lon_deg + 180) / 360)
        return xtile

    def parseTimeBin(self,timebinsize):
        match = re.match(r'^(\d+)([smhDWWY])$',timebinsize)
        num, unit = match.groups()
        td = np.timedelta64(num,unit)
        return np.timedelta64(td,'s')

    def header(self,data):
        h = ''
        h += 'name: %s\n'%(self.name.replace(' ',"_"))
        h += 'encoding: binary\n'
        for sp in self.spname:
            h += 'metadata: %s__origin degrees_mercator_quadtree%d\n'%(
                sp,self.levels)
            h += 'field: %s nc_dim_quadtree_%d\n'%(sp.replace(' ',"_"), 
                                                   self.levels)
            
        for c in self.catcol:
            h += 'field: %s nc_dim_cat_1\n'%(c.replace(' ',"_"))
            for k in self.valname[c]:
                h+='valname: %s %d %s\n'%(c.replace(' ',"_"),
                                          self.valname[c][k],k)
                        
        for d in self.timecol:
            h += "metadata: tbin %s_%s_%ds\n"%(self.offset.date(),
                                               self.offset.time(),
                                               self.timebinsize.astype(np.int))
            h += 'field: %s nc_dim_time_2\n'%(d.replace(' ',"_"))

        h += 'field: %s nc_var_uint_4\n\n' %(self.countcol.replace(' ',"_"))
        return h

def main(argv):
    #parse arguments
    parser = argparse.ArgumentParser()
    parser.add_argument('InputFile',type=str, nargs='+')
    parser.add_argument('--timebinsize',type=str, default='1h')
    parser.add_argument('--timecol', type=str,nargs='+',default=['Date'])
    parser.add_argument('--datefmt', type=str, default=None)
    parser.add_argument('--spname', type=str,nargs='+',default=['src'])
    parser.add_argument('--levels', type=int, default=25)
    parser.add_argument('--latcol', type=str,nargs='+',default=['Latitude'])
    parser.add_argument('--loncol', type=str,nargs='+',default=['Longitude'])
    parser.add_argument('--catcol', type=str,nargs='+',default=[])
    parser.add_argument('--countcol', type=str, default=None)
    args = parser.parse_args()

    if 'NANOCUBE_WEB' not in os.environ:
        os.environ['NANOCUBE_WEB'] = '../web'

    if 'NANOCUBE_BIN' not in os.environ:
        os.environ['NANOCUBE_BIN'] = '../src'

    ncinput = NanocubeInput(args)    

    
if __name__ == '__main__':
    main(sys.argv)
