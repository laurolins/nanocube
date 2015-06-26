## Load data from comma-separated value (csv) files
Suppose we want to create a nanocube for the following [file](https://github.com/laurolins/nanocube/blob/master/data/crime50k.csv):

<pre>
ID,Case Number,time,Block,IUCR,crime,Description,Location Description,Arrest,Domestic,Beat,District,Ward,Community Area,FBI Code,X Coordinate,Y Coordinate,Year,Updated On,Latitude,Longitude,Location
9418031,HW561348,12/06/2013 06:25:00 PM,040XX W WILCOX ST,2024,NARCOTICS,POSS: HEROIN(WHITE),SIDEWALK,true,false,1115,011,28,26,18,1149444,1899069,2013,12/11/2013 12:40:36 AM,41.8789661034259,-87.72673345412568,"(41.8789661034259, -87.72673345412568)"
9418090,HW561429,12/06/2013 06:26:00 PM,026XX W LITHUANIAN PLAZA CT,1310,CRIMINAL DAMAGE,TO PROPERTY,GROCERY FOOD STORE,false,false,0831,008,15,66,14,1160196,1858843,2013,12/10/2013 12:39:15 AM,41.76836587673295,-87.68836274472295,"(41.76836587673295, -87.68836274472295)"
9418107,HW561399,12/06/2013 06:29:00 PM,045XX S DAMEN AVE,0860,THEFT,RETAIL THEFT,DEPARTMENT STORE,true,false,0924,009,12,61,06,1163636,1874247,2013,12/10/2013 12:39:15 AM,41.810564946613454,-87.6753212816967,"(41.810564946613454, -87.6753212816967)"
...
</pre>

```
nanocube-binning-csv --latcol=Latitude --loncol=Longitude --timecol=time --catcol=crime,'Location Description' crime50k.csv | nanocube-leaf ...
```

The `nanocube-binning-csv` script attempts to parse the csv file and convert it into the nanocubes binary format.

This scripts summerizes the categorical values from the first chunk of the file.The chuck size is specified by `--chunksize` (10000 lines by default), if there are unseen values from the subsequent parts of the file, they would not be shown in the bar chart.

For streaming data, rather than reading from files, the script takes the `-` parameter to read from `stdin`.  Therefore you can pipe the csv style output of a program to nanocubes.  For example:

```
$StreamingDataProgram | nanocube-binning-csv --latcol=Latitude --loncol=Longitude --Timecol=time --catcol=crime,'Location Description' - | nanocube-leaf ...
```

#### Other useful parameters
* `--chunksize` specify the chunksize for csv parsing.
* `--name` specify the name/title of the nanocube.
* `--sep` specify the separator of the csv file. e.g. `--sep='|'`.
* `--header` specifics a header for header less csv files. e.g. `--header='lat,lon,time,crime'`
* `--datefmt` specify the [date format](https://docs.python.org/2/library/datetime.html#strftime-strptime-behavior) for csv parsing.
* `--catbytes` specify the categorical variable size in bytes (1, 2 or 4 only).
* `--countbytes` specify the counter variable size in bytes (4 or 8 only).
