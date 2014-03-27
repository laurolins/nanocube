import sys,dateutil,datetime,argparse,re,subprocess
import pandas as pd
import numpy as np

class NanocubeInput:
    def __init__(self, args):
        self.name=args.InputFile[0].name
        self.timebinsize=self.parseTimeBin(args.timebinsize)

        self.spname=[c.replace(' ','_') for c in args.spname]
        self.catcol=[c.replace(' ','_') for c in args.catcol]
        self.datecol=[c.replace(' ','_') for c in args.datecol]
        self.latcol=[c.replace(' ','_') for c in args.latcol]
        self.loncol=[c.replace(' ','_') for c in args.loncol]
        self.countcol=args.countcol.replace(' ','_')
        self.levels = args.levels
        
        self.field=[]
        self.valname={}

        coi = self.datecol+self.catcol+self.latcol+self.loncol

        #cmd = 'ncserve --rf=100000 --threads=100'
        #self.ncproc = subprocess.Popen([cmd], stdin=subprocess.PIPE,
        #                               shell=True)

        self.readcsv(args.InputFile,coi)

    def readcsv(self,files,coi):
        start = True
        for f in files:
            reader = pd.read_csv(f,chunksize=5000)
            for i,data in enumerate(reader):
                if start:
                    #compute the time offset
                    for i,d in enumerate(self.datecol):
                        data[d] = (pd.to_datetime(data[d]))
                    year = data[self.datecol].min().min().year
                    self.offset = datetime.datetime(year=year,month=1,day=1)

                data = self.processData(data,coi)

                if start:
                    sys.stdout.write(self.header(data))
                    start = False

                self.writeRawData(data)

    def processData(self, data, coi):
        data.columns = [c.replace(' ', '_') for c in data.columns]
        data = data[coi].dropna()
            
            
        data = self.processLatLon(data)
        data = self.processCat(data)
        data = self.processDate(data,self.offset)
            
        if self.countcol not in data.columns:
            data[self.countcol]=1
            data[self.countcol] = data[self.countcol].astype('<u4')

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

        for i,d in enumerate(self.datecol):                
            columns += [d]
            data[d] = data[d].astype('<u2'); 

        columns += ['count']
        data['count'] = data['count'].astype('<u4'); 

        data = data[columns] #permute

        rec = data.to_records(index=False)
        #print rec
        rec.tofile(sys.stdout)
            
    def processLatLon(self,data):
        for i,spname in enumerate(self.spname):
            lat = self.latcol[i]
            lon = self.loncol[i]
            lvl = self.levels
            
            data = data[data[lon] > -180]
            data = data[data[lon] < 180]
            data = data[data[lat] > -85.0511]
            data = data[data[lat] < 85.0511]

            data[lon] = self.lonToTileX(data[lon],lvl)
            data[lat] = self.latToTileY(data[lat],lvl)
        return data
            
    def processDate(self,inputdata,offset):
        data = inputdata.copy()
        for i,d in enumerate(self.datecol):
            data[d] = pd.to_datetime(data[d],coerce=True)
            data = data.dropna()

            data[d] -= offset
            data[d] = data[d] / self.timebinsize
        return data
            
    def processCat(self,data):
        for i,c in enumerate(self.catcol):            
            #fix the spaces
            data[c] = data[c].apply(lambda x : x.replace(' ','_'))

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
        h += 'name: %s\n'%(self.name)
        h += 'encoding: binary\n'
        for sp in self.spname:
            h += 'metadata: %s__origin degrees_mercator_quadtree%d\n'%(
                sp,self.levels)
            h += 'field: %s nc_dim_quadtree_%d\n'%(sp, self.levels)
            
        for c in self.catcol:
            h += 'field: %s nc_dim_cat_1\n'%(c)
            for k in self.valname[c]:
                h+='valname: %s %d %s\n'%(c,self.valname[c][k],k)
                        
        for d in self.datecol:
            h += "metadata: tbin %s_%s_%ds\n"%(self.offset.date(),
                                               self.offset.time(),
                                               self.timebinsize.astype(np.int))
            h += 'field: %s nc_dim_time_2\n'%(d)

        h += 'field: count nc_var_uint_4\n\n'
        return h

def main(argv):
    #parse arguments
    parser = argparse.ArgumentParser()
    parser.add_argument('InputFile',type=argparse.FileType('r'), nargs='+')
    parser.add_argument('--timebinsize',type=str, default='1h')
    parser.add_argument('--datecol', type=str,nargs='+',default=['Date'])
    parser.add_argument('--spname', type=str,nargs='+',default=['src'])
    parser.add_argument('--levels', type=int, default=25)
    parser.add_argument('--latcol', type=str,nargs='+',default=['Latitude'])
    parser.add_argument('--loncol', type=str,nargs='+',default=['Longitude'])
    parser.add_argument('--catcol', type=str,nargs='+',default=[])
    parser.add_argument('--countcol', type=str, default='count')
    args = parser.parse_args()

    ncinput = NanocubeInput(args)    

    
if __name__ == '__main__':
    main(sys.argv)
