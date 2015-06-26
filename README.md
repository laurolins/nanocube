# Nanocubes: an in-memory data structure for spatiotemporal data cubes

Nanocubes are a fast data structure for in-memory data cubes developed at the [Information Visualization department](http://www.research.att.com/infovis) at [AT&T Labs Research](http://www.research.att.com). Visualizations powered by nanocubes can be used to explore datasets with billions of elements at interactive rates in a web browser, and in some cases nanocubes uses sufficiently little memory that you can run a nanocube in a modern-day laptop.

## Releases

| Number | Description |
|:------:|-------------|
| 3.2.1 | Improved web client, documentation, testing |
| 3.2 | Sliding window; removed legacy assertions that would crash the server |
| 3.1 | Added new tools, standarized tool names, restructured code repository, bug fixes |
| 3.0.1 | Fixed a bug that was causing memory inefficiencies |
| 3.0 | [New API](./API.md) to support more general nanocubes |
| 2.1.3 | Minor fixes, improved csv2Nanocube.py script  |
| 2.1.2 | Minor fixes, better documentation, shutdown service |
| 2.1.1 | Fixed csv2Nanocube.py to work with pandas 0.14.0 |
| 2.1 | Javascript front-end, CSV Loading, Bug Fixes  |
| 2.0 | New feature-rich querying API                  |
| 1.0 | Original release with a simple querying API   |

## What is new in release 3.2.1

### Web Client

Improved overall layout and look of the web client front-end, including size
and position of charts, font ratios, colormaps, and log scales.  Also enabled
switching between alphabetical and numerical sorting of the bar charts.


### Documentation

Improved README file to be more accurate and have clearer instructions. Added
more details on how to configure the web client.

### Testing

Simplified the nctest.sh script to detect which OS you are using and make the
appropriate comparisons.  Included expected data for MacOS and Ubuntu.

## Installing prerequisites

The following are prerequisites for all systems:

1. The nanocubes server is 64-bit only.  There is NO support on 32-bit operating systems.
2. The nanocubes server is written in C++ 11.  You must use a recent version of gcc (>= 4.8).
3. The nanocubes server uses [Boost](http://www.boost.org).  You must use version 1.48 or later.
4. To build the nanocubes server, you must have the [GNU build system](http://www.gnu.org/software/autoconf/) installed.

#### Linux (Ubuntu)

On a newly installed 64-bit Ubuntu 14.04 system, gcc/g++ is already 4.8.2, but you should install the following packages:

```
sudo apt-get install build-essential
sudo apt-get install automake
sudo apt-get install libtool
sudo apt-get install zlib1g-dev
sudo apt-get install libboost-all-dev
```

#### Mac OS X (10.9 and 10.10)

Example installation on Mac OS 10.9 Mavericks and 10.10 Yosemite with a local homebrew:

```
git clone https://github.com/mxcl/homebrew.git
```

Set your path to use this local homebrew				

```
export PATH=${PWD}/homebrew/bin:${PATH}
```

Install the packages (This assumes your g++ has been installed by [XCode](https://developer.apple.com/xcode/))

```
brew install boost libtool autoconf automake
```

Set path to the boost directory

```
export BOOST_ROOT=${PWD}/homebrew
```


## Compiling the latest release

To compile the nanocubes toolkit, run the following commands on your linux/mac system.  You can replace `3.2` with other valid release numbers, e.g. 3.0.1, 3.0, 2.1.3, 2.1, 2.0, but in those cases please follow the instructions in the README.md file for those releases as they will differ slightly.

```
wget https://github.com/laurolins/nanocube/archive/3.2.zip
unzip 3.2.zip
cd nanocube-3.2
export NANOCUBE_SRC=`pwd`
./bootstrap
mkdir build
cd build
../configure --prefix=$NANOCUBE_SRC CXXFLAGS="-O3"
make
make install
cd ..
```

After these commands you should have directory `nanocube-3.2/bin` with the nanocubes toolkit inside. To make these tools more easily accessible in your account, add the `nanocube-3.2/bin` directory to your PATH environment variable.

```
export NANOCUBE_BIN=$NANOCUBE_SRC/bin
export PATH=$NANOCUBE_BIN:$PATH
```

**Please note:** If the default version of g++ on your system is too old,
you can run `configure` and specify a more recent version of g++: `CXX=g++-4.8 ../configure --prefix=$NANOCUBE_SRC CXXFLAGS="-O3"`.

**Please note:** For better performance you might configure nanocubes with the tcmalloc
option (see details below): `../configure --prefix=$NANOCUBE_SRC --with-tcmalloc CXXFLAGS="-O3"`.

## Running a nanocube

With the nanocube toolkit installed, we are ready to build a
nanocube with the [Chicago Crimes](https://data.cityofchicago.org/Public-Safety/Crimes-2001-to-present/ijzp-q8t2)
example dataset file included in the distribution.  Here is the command:

    cat $NANOCUBE_SRC/data/crime50k.dmp | nanocube-leaf -q 29512 -f 10000

The command above simply says to start a nanocube back-end process
from the `crime50k.dmp` data file, answer queries on port 29512, and report
statistics every 10,000 insertions.  Sample output from this call is shown below.
After inserting all 50,000 records, the nanocube is using 26MB of memory (approximately 20MB if
you are using tcmalloc).

##### Output
```
VERSION: 3.2
query-port: 29512
(stdin     ) count:      10000 mem. res:          7MB. time(s):          0
(stdin     ) count:      20000 mem. res:         12MB. time(s):          0
(stdin     ) count:      30000 mem. res:         17MB. time(s):          0
(stdin     ) count:      40000 mem. res:         22MB. time(s):          0
(stdin     ) count:      50000 mem. res:         26MB. time(s):          0
(stdin:done) count:      50000 mem. res:         26MB. time(s):          0
```

**Please note:** If port 29512 is already in use, select another port and use it consistently throughout the examples below.

**Please note:** For lower level details on how to generate valid data for nanocubes go [here](https://github.com/laurolins/nanocube/wiki/nanocube-ready-dmp)

## Simple queries

With a nanocube process running, we are able to query this nanocube
using the HTTP-based [API](./API.md).
Using your favorite browser (assuming your favorite is Chrome, Firefox, or Safari), enter the following simple
queries and verify the [JSON](http://www.json.org/) objects returned are correct.
Please note that differences in computers (e.g. compilers, libraries) may result in some of the data being returned
in a different order (e.g. query 2 and query 3).

#### Query 1: Total count of all records

```
http://localhost:29512/count
```

##### Output

```
{ "layers":[  ], "root":{ "val":50000 } }
```

##### Interpretation

Starting at the root of the nanocube, we have 50,000 records in total.


#### Query 2: Schema of the nanocube

```
http://localhost:29512/schema
```

##### Ouput

```json
{
  "fields": [
    {"name": "location","type": "nc_dim_quadtree_25","valnames": {}},
    {
      "name": "crime",
      "type": "nc_dim_cat_1",
      "valnames": {
        "OTHER_OFFENSE": 22,
        "NON-CRIMINAL_(SUBJECT_SPECIFIED)": 18,
        "NARCOTICS": 16,
        "GAMBLING": 9,
        "MOTOR_VEHICLE_THEFT": 15,
        "OTHER_NARCOTIC_VIOLATION": 21,
        "OBSCENITY": 19,
        "HOMICIDE": 10,
        "THEFT": 29,
        "DECEPTIVE_PRACTICE": 8,
        "CRIMINAL_DAMAGE": 5,
        "STALKING": 28,
        "BATTERY": 2,
        "PUBLIC_PEACE_VIOLATION": 25,
        "PUBLIC_INDECENCY": 24,
        "ASSAULT": 1,
        "BURGLARY": 3,
        "ROBBERY": 26,
        "LIQUOR_LAW_VIOLATION": 14,
        "INTERFERENCE_WITH_PUBLIC_OFFICER": 11,
        "NON-CRIMINAL": 17,
        "PROSTITUTION": 23,
        "ARSON": 0,
        "INTIMIDATION": 12,
        "SEX_OFFENSE": 27,
        "CONCEALED_CARRY_LICENSE_VIOLATION": 4,
        "OFFENSE_INVOLVING_CHILDREN": 20,
        "KIDNAPPING": 13,
        "CRIM_SEXUAL_ASSAULT": 7,
        "WEAPONS_VIOLATION": 30,
        "CRIMINAL_TRESPASS": 6
      }
    },
    {"name": "time","type": "nc_dim_time_2","valnames": {}},
    {"name": "count","type": "nc_var_uint_4","valnames": {}}
  ],
  "metadata": [
    {"key": "location__origin","value": "degrees_mercator_quadtree25"},
    {"key": "tbin","value": "2013-12-01_00:00:00_3600s"},
    {"key": "name","value": "crime50k.csv"}
  ]
}
```

##### Interpretation

The schema reports all of the dimensions (i.e. fields) of the nanocube and their types. In this case, there are four fields with names: `location`, `crime`, `time`, and `count`.  The types of these fields are (roughly) described as: quadree with 25 levels, categorical with 1 byte, time with 2 bytes, and unsigned integer with 4 bytes. Also specified for each field are the valid values of these fields.  In this example, we only really need to specify them for the categorical dimension `crime` which lists the names and values of the criminal offenses. There is also some additional metadata reported to indicate when the time dimension should begin, how large the time bins are, how to project the quadtree onto a map, and the original data file name.

#### Query 3: Breakdown the counts per `crime` type

```
http://localhost:29512/count.a("crime",dive([],1))
```

##### Output

```json
{
  "layers": ["anchor:crime"],
  "root": {
    "children": [
      {"path": [0],"val": 63},
      {"path": [1],"val": 2629},
      {"path": [2],"val": 8990},
      {"path": [3],"val": 2933},
      {"path": [4],"val": 1},
      {"path": [5],"val": 4660},
      {"path": [6],"val": 1429},
      {"path": [7],"val": 181},
      {"path": [8],"val": 2190},
      {"path": [9],"val": 2},
      {"path": [10],"val": 69},
      {"path": [11],"val": 229},
      {"path": [12],"val": 21},
      {"path": [13],"val": 46},
      {"path": [14],"val": 69},
      {"path": [15],"val": 2226},
      {"path": [16],"val": 5742},
      {"path": [17],"val": 1},
      {"path": [18],"val": 1},
      {"path": [19],"val": 1},
      {"path": [20],"val": 456},
      {"path": [21],"val": 2},
      {"path": [22],"val": 3278},
      {"path": [23],"val": 211},
      {"path": [24],"val": 2},
      {"path": [25],"val": 441},
      {"path": [26],"val": 2132},
      {"path": [27],"val": 119},
      {"path": [28],"val": 20},
      {"path": [29],"val": 11367},
      {"path": [30],"val": 489}
    ]
  }
}
```

##### Interpretation

We would like to "anchor" (thus the ".a" in the query) on the `crime` dimension and report counts for each of the possible values.  Notice that we do not specify the crimes by name but rather by value.  `"path":[0], "val":63` indicates that there were 63 counts of ARSON, because ARSON has a value of 0 in the schema returned by Query 2.  Similarly, there were 2629 counts of ASSAULT.

## Simple test script

If you believe there may be a problem with the crime nanocube, try
running 'nctest.sh' in the `test` subdirectory.  It will make some
queries of the nanocube (change the script if you are not using port
29512) and compare the results to known results that we gathered
ourselves (for both Ubuntu 14.04 and MacOS 10.10).  If the results match, it will
report 'SUCCESS'.  In the case of FAILURE, it may still be a simple discrepancy
so you should look at the output to see if the results may simply be sorted differently.

<!--
 and produces `out.txt`.
Please compare your results with the expected results generated on MacOSX 10.10 or Ubuntu 14.04.
-->

```
cd $NANOCUBE_SRC/test
./nctest.sh
```

## Simple web client

**Please note:** This viewer should work with any nanocube that has
one spatial dimension, zero or more categorical dimensions and one temporal dimension.

To visualize the Chicago Crimes nanocube, you can use the simple
viewer that we have included with the nanocube distribution.  The
nanocube viewer, written in JavaScript, [D3](http://d3js.org), and HTML5, can be found here:
`$NANOCUBE_SRC/extra/nc_web_viewer`.  Before starting the viewer however, we need to specify where
the nanocube process is being hosted on our machine.  We do this by creating a `nc_web_viewer`
specific `.json` configuration file and putting it in the same directory as the viewer.  In this case,
we can generate a valid configuration file for the Chicago crime data by running the following command
(a python script found in `$NANOCUBE_SRC/bin`) and specifying the machine and port of the nanocube.

```
ncwebviewer-config -s http://localhost:29512 -o $NANOCUBE_SRC/extra/nc_web_viewer/config_crime.json
```

You can now start the viewer by running the following:

```
cd $NANOCUBE_SRC/extra/nc_web_viewer
python -m SimpleHTTPServer 8000
```

By pointing a web browser to the following URL we can get to the
`nc_web_viewer` and get our first visualization of the Chicago Crime
data.  Note that the name of the configuration file (without file extension) is
specified in the URL.

```
http://localhost:8000/#config_crime
```

The intial view should look like the image below, which shows a map of
Chicago, together with a bar chart of the number of crimes (sorted
alphabetically) and an hourly time-series of all crimes.  The left
mouse button will pan the image.  You can zoom further into Chicago by
using the navigation buttons in the top-left of the viewer, or using
the mouse wheel.  You can select specific crimes by clicking on the
corresponding bars (or names of crimes).  To sort the bar chart by
number of occurrences, click the title "crime" above the bars.  All
widgets in the viewer should have tool-tips to help you navigate
properly.

![image](./data/ChicagoCrimeInitial.png?raw=true)


Please see [here](https://github.com/laurolins/nanocube/tree/master/extra/nc_web_viewer) for more configuration options.

## Auxiliary python tools

The nanocubes distribution comes with several auxiliary tools that help simplify creating, testing,
and monitoring nanocubes.  They can be found in the `$NANOCUBE_SRC/bin` subdirectory after compiling nanocubes (and running `make install`).

| Tool Name | Description |
|:------:|-------------|
| nanocube-binning-csv | Convert a `.csv` file into a `.dmp` file readable by the nanocube-leaf program |
| nanocube-benchmark | From a text file with one query per row, generate a report of latency and sizes by running those queries on a given nanocube server |
| nanocube-view-dmp | Show records of a `.dmp` file on the command line |
| nanocube-monitor | Feedback of latency and size distribution for nanocube profiling |

These tools are all Python scripts, which require some additional packages
to run.  Please follow the (Ubuntu linux) instructions below for compiling and running the extra tools.
Once the packages have been installed, you need not re-install in the future.  You will only have to
activate the virtual python environment (shown in step C).

A. Install the python development package

```
sudo apt-get install python-dev
```

B. Install the python data analysis library (pandas) in a separate python environment

```
cd $NANOCUBE_SRC
wget http://pypi.python.org/packages/source/v/virtualenv/virtualenv-1.11.6.tar.gz
tar xfz virtualenv-1.11.6.tar.gz
python virtualenv-1.11.6/virtualenv.py  myPy
```

C. Activate the virtual python environment.  Install the additional python libraries.  Thankfully this
needs to be done only once since it takes several minutes for it to complete.
Once you are done with the tools, you should type `deactivate` to disable.

```
# Make sure PYTHONHOME and PYTHONPATH are unset
unset PYTHONHOME
unset PYTHONPATH

source myPy/bin/activate
pip install pandas numpy argparse
```


## Loading data into nanocubes

Nanocubes are only able to ingest data in a specific format, but the data can be contained in
a static file or be streamed into the nanocube server process in real-time.  From a high
level, the data format is really just an efficient form of a data table, which consists of
a header describing the columns of the table followed by the individual records.  We refer
to files in this format as `dmp` files.  To complicate matters slightly,
not all DMP files are nanocube-ready.  They may need to be further processed, but we will discuss
this issue later.  For now, we will demonstrate how you can turn a simple comma-separate value data
file (i.e. CSV file) into a nanocube-ready DMP file.

#### Loading CSV files

We have provided a python script `nanocube-binning-csv` for converting CSV files into nanocube-ready DMP files.
In the `data` subdirectory, we have included an example CSV file from the Chicago Crime dataset.  The first few
lines of the example dataset are shown below. The first line is a header, which describes each of
the columns in this table of data.  You should notice that there are columns called:
time, crime, Latitude, Longitude.  These are the columns that we are interested in at this time.

```
ID,Case Number,time,Block,IUCR,crime,Description,Location Description,Arrest,Domestic,Beat,District,Ward,Community Area,FBI Code,X Coordinate,Y Coordinate,Year,Updated On,Latitude,Longitude,Location
9418031,HW561348,12/06/2013 06:25:00 PM,040XX W WILCOX ST,2024,NARCOTICS,POSS: HEROIN(WHITE),SIDEWALK,true,false,1115,011,28,26,18,1149444,1899069,2013,12/11/2013 12:40:36 AM,41.8789661034259,-87.72673345412568,"(41.8789661034259, -87.72673345412568)"
9418090,HW561429,12/06/2013 06:26:00 PM,026XX W LITHUANIAN PLAZA CT,1310,CRIMINAL DAMAGE,TO PROPERTY,GROCERY FOOD STORE,false,false,0831,008,15,66,14,1160196,1858843,2013,12/10/2013 12:39:15 AM,41.76836587673295,-87.68836274472295,"(41.76836587673295, -87.68836274472295)"
9418107,HW561399,12/06/2013 06:29:00 PM,045XX S DAMEN AVE,0860,THEFT,RETAIL THEFT,DEPARTMENT STORE,true,false,0924,009,12,61,06,1163636,1874247,2013,12/10/2013 12:39:15 AM,41.810564946613454,-87.6753212816967,"(41.810564946613454, -87.6753212816967)"
9418222,HW561455,12/06/2013 06:30:00 PM,008XX W ALDINE AVE,0820,THEFT,$500 AND UNDER,RESIDENCE,true,false,1924,019,44,6,06,1169898,1922154,2013,12/14/2013 12:39:48 AM,41.94189139041086,-87.65095594008946,"(41.94189139041086, -87.65095594008946)"
```

Follow these simple instructions to convert that file into the DMP file that was used earlier when we built our first nanocube.
This assumes that you have already installed the auxiliary python packages discussed above, and activated the virtual
python environment.  The new DMP file should be identical to the one used before.

```
cd $NANOCUBE_SRC/data
nanocube-binning-csv    \
--sep=','               \
--timecol='time'        \
--latcol='Latitude'     \
--loncol='Longitude'    \
--catcol='crime'        \
crime50k.csv > crime50k_from_csv.dmp
```

Note that in the example above, every row is going to count as one,
and that is the measure that will be pre-aggregating: the number of
rows.  If we have a column in the .csv file that has weights which we
would like to use as our measure instead, we can add the the directive
`--countcol=<colname>`. Furthermore, if we don't have a time
dimension, we should omit the `--timecol=<colname>`
directive and a dummy time column (which is currently required) is
generated with all the rows having the same time value.

#### Further details

For a better understanding on how to ingest data into nanocubes and
how to query nanocubes follow this
[link](https://github.com/laurolins/nanocube/wiki). For larger
datasets or if you want more flexibility on ingesting/querying data
using nanocubes, the CSV loading method illustrated above might not be
the most efficient way to go.

**Please note:** The documentation at that link is largely out-of-date, but we are working to fix it presently.


## Thread-Caching Malloc (tcmalloc)

We strongly suggest linking nanocubes with
[Thread-Caching Malloc](http://goog-perftools.sourceforge.net/doc/tcmalloc.html),
or tcmalloc for short.  It is faster than the default system malloc,
and in some cases, we found that the amount of memory used by
nanocubes was reduced by over 50% when using libtcmalloc.  To install
on a Ubuntu 14.04 machine, install the following package and all of
its dependencies.

    sudo apt-get install libgoogle-perftools-dev

For Mac OS (Static libraries only)

    brew install gperftools

You must then re-run the configure script, indicating support for tcmalloc.

    ../configure --prefix=$NANOCUBES_SRC LIBS=${PWD}/homebrew/lib/libtcmalloc_minimal.a
    make clean
    make -j
    make install    

## Asking for help

Our mailing list is the best and fastest way to ask questions and make suggestions related to nanocubes.
If you are having a problem, please search the archives before creating new topics to see if
your question has already been answered.  If you have other ideas for how we can improve
nanocubes, please let us know.

A nice front-end for our mailing list is now being served through [Nabble](http://nanocubes-discuss.64146.x6.nabble.com).
You should be able to post messages, search the archives, and even register as a new user from here.

The actual mailing list can be found [here](http://mailman.nanocubes.net/mailman/listinfo/nanocubes-discuss_mailman.nanocubes.net).
