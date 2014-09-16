## Load data from comma-separated value (csv) files
Suppose we want to create a nanocube for the following [table in csv](wiki/example.csv "example.csv") format:

<pre>
latitude,longitude,time,device,make
44.00124456,-73.74023438,2013-02-21T12:49,android,Samsung
42.0090331,-74.79492188,2013-04-11T13:58,iphone,Apple
45.68366959,-94.04296875,2013-02-28T17:33,iphone,Apple
37.97076194,-85.69335938,2013-04-17T05:04,android,LG
...
</pre>

Imagine for instance that this table stores check-in reports of a
social website like foursquare or the defunct brightkite. In addition
to location and time we have a categorical variable indicating which
device was used to report the check-in. The goal we have in mind is to
create a nanocube back-end for this dataset that will respond (at
interactive rates) the queries needed on a front-end that has pannable
and zoomable map to visualize a heatmap of counts of reports in
the different regions and different times. We also want the user to
filter these count reports by device.

To load this csv file into a nanocube you can use the `csv2Nanocube.py` script.

       $python csv2Nanocube.py --latcol=latitude --loncol=longitude --timecol=time --catcol=device,make example.csv | ncserve ...

The `csv2Nanocube.py` script attempts to parse the csv file by reading the first 10000 lines of the csv file.  The script summarizes categorical variables and infer date formats, then convert the data into a binary format that is readable by the nanocube server.

For streaming data, rather than reading from files, the script takes the `-` parameter to read from `stdin`.  Therefore you can pipe the csv style output of a program to nanocubes.  For example:

      $StreamingDataProgram | python csv2Nanocube.py --latcol=latitude --loncol=longitude --timecol=time --catcol=device,make - | ncserve ...


##  Web client configuration

The `csv2Nanocube.py` script will attempt to generate a configuration file `config.json` for the javascript web client.  If you move the Nanocubes server away from the localhost and port 29512, please modify `url` attribute of the configuration to reflect the changes.  The web client will load `config.json` by default, however the configuration file can also be passed as a &#35 label.  For example `http://localhost:29512/#dataset` will load the configuration from `http://localhost:29512/dataset.json`
