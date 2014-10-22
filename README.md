# Nanocubes: an in-memory data structure for spatiotemporal data cubes

Nanocubes are a fast data structure for in-memory data cubes developed
at the
[Information Visualization department](http://www.research.att.com/infovis)
at [AT&T Labs – Research](http://www.research.att.com). Visualizations
powered by nanocubes can be used to explore datasets with billions of
elements at interactive rates in a web browser, and in some cases nanocubes
uses sufficiently little memory that you can run a nanocube in a
modern-day laptop.

## Releases

| Number | Description |
|:------:|-------------|
| 3.0.1 | Fixed a bug that was causing memory inneficiencies |
| 3.0 | [New API](https://github.com/laurolins/nanocube/blob/api3/API.md) to support more general nanocubes |
| 2.1.3 | Minor fixes, improved csv2Nanocube.py script  |
| 2.1.2 | Minor fixes, better documentation, shutdown service |
| 2.1.1 | Fixed csv2Nanocube.py to work with pandas 0.14.0 |
| 2.1 | Javascript front-end, CSV Loading, Bug Fixes  |
| 2.0 | New feature-rich querying API                  |
| 1.0 | Original release with a simple querying API   |

## Compiling the latest release

**Prerequisites**

1. The nanocubes server is 64-bit only.  There is NO support on 32-bit operating systems.
2. The nanocubes server is written in C++ 11.  You must use a recent version of gcc (>= 4.8).
3. The nanocubes server uses [Boost](http://www.boost.org).  You must use version 1.48 or later.
4. To build the nanocubes server, you must have the [GNU build system](http://www.gnu.org/software/autoconf/) installed.

**Linux (Ubuntu)**

On a newly installed 64-bit Ubuntu 14.04 system, gcc/g++ is already 4.8.2, but you may have to install the following packages:

    sudo apt-get install build-essential
    sudo apt-get install automake
    sudo apt-get install libtool
    sudo apt-get install zlib1g-dev
    sudo apt-get install libboost-all-dev


**Mac OS X (10.9)**

Example installation on Mac OS 10.9 Maverick with a local homebrew:

	git clone https://github.com/mxcl/homebrew.git

Set your path to use this local homebrew				

	export PATH=${PWD}/homebrew/bin:${PATH}

Install the packages (This assumes your g++ has been installed by [XCode](https://developer.apple.com/xcode/))

	brew install boost libtool autoconf automake

Set path to the boost directory

	export BOOST_ROOT=${PWD}/homebrew

**General Instructions**

Run the following commands to compile nanocubes on your linux/mac system. Replace `3.0.1`
with other valid release numbers (e.g. 3.0, 2.1.3, 2.1, 2.0).

    wget https://github.com/laurolins/nanocube/archive/3.0.1.zip
    unzip 3.0.1.zip
    cd nanocube-3.0.1
    ./bootstrap
    ./configure
    make

If a recent version of gcc is not the default, you can run `configure`
with the specfic recent version of gcc in your system. For example

    CXX=g++-4.8 ./configure

**Tcmalloc**

We strongly suggest linking nanocubes with [Thread-Caching Malloc](http://goog-perftools.sourceforge.net/doc/tcmalloc.html), or tcmalloc for short.
It is faster than the default system malloc, and in some cases, we found that the amount of memory used by nanocubes was reduced
by over 50% when using libtcmalloc.  To install on a Ubuntu 14.04 machine, install the following package and all of its dependencies.

    sudo apt-get install libgoogle-perftools-dev

You must then re-run the configure script, indicating support for tcmalloc.

    ./configure --with-tcmalloc
    make clean
    make


## Loading a CSV file into a nanocube

1. For compiling our python helper code, you will need the following packages:

        sudo apt-get install python-dev

2. Install the python data analysis library (pandas) in a separate python environment (Recommended)

        wget http://pypi.python.org/packages/source/v/virtualenv/virtualenv-1.11.6.tar.gz
        tar xfz virtualenv-1.11.6.tar.gz
        python virtualenv-1.11.6/virtualenv.py  myPy
        
        # Make sure PYTHONHOME and PYTHONPATH are unset
        unset PYTHONHOME
        unset PYTHONPATH

        # activate the virtualenv, type "deactivate" to disable the env when done
        source myPy/bin/activate
        pip install pandas numpy argparse

3. Start a web server in the "web" directory and send it to background.  If port 8000 is already being used
on your system, please choose another port.

        cd web
        python -m SimpleHTTPServer 8000 &

4. Run the script and pipe it to the nanocubes server using the
   included example dataset
   ([Chicago Crime](https://data.cityofchicago.org/Public-Safety/Crimes-2001-to-present/ijzp-q8t2)).  If port 29512 is already
   being used on your system, please choose another port.  Note that the port is specified for both the python script and for
   the nanocubes server (nanocube-leaf). If these are not the same, you'll run into problems.

        cd ../scripts
        python csv2Nanocube.py --sep=',' --timecol='time' --latcol='Latitude' --loncol='Longitude' --catcol='crime' --port=29512 crime50k.csv | NANOCUBE_BIN=../src  ../src/nanocube-leaf --batch-size 1 --query-port 29512 --report-frequency 10000 --threads 100


   Please note: We modified the original dataset slightly in this example by changing the names of two columns in the header.  In previous versions,
there were columns called 'Date' and 'Primary Type'.  We have renamed these 'time' and 'crime' to avoid any confusion and make the subsequent
examples easier to read.

   The first few lines of the example dataset are shown below. The first line is a header, which describes each of the columns in this table of data.
   You should notice that there are columns called: time, crime, Latitude, Longitude.  These are the columns used for this visualization.

        ID,Case Number,time,Block,IUCR,crime,Description,Location Description,Arrest,Domestic,Beat,District,Ward,Community Area,FBI Code,X Coordinate,Y Coordinate,Year,Updated On,Latitude,Longitude,Location
        9418031,HW561348,12/06/2013 06:25:00 PM,040XX W WILCOX ST,2024,NARCOTICS,POSS: HEROIN(WHITE),SIDEWALK,true,false,1115,011,28,26,18,1149444,1899069,2013,12/11/2013 12:40:36 AM,41.8789661034259,-87.72673345412568,"(41.8789661034259, -87.72673345412568)"
        9418090,HW561429,12/06/2013 06:26:00 PM,026XX W LITHUANIAN PLAZA CT,1310,CRIMINAL DAMAGE,TO PROPERTY,GROCERY FOOD STORE,false,false,0831,008,15,66,14,1160196,1858843,2013,12/10/2013 12:39:15 AM,41.76836587673295,-87.68836274472295,"(41.76836587673295, -87.68836277 4472295)"
        9418107,HW561399,12/06/2013 06:29:00 PM,045XX S DAMEN AVE,0860,THEFT,RETAIL THEFT,DEPARTMENT STORE,true,false,0924,009,12,61,06,1163636,1874247,2013,12/10/2013 12:39:15 AM,41.810564946613454,-87.6753212816967,"(41.810564946613454, -87.6753212816967)"
        9418222,HW561455,12/06/2013 06:30:00 PM,008XX W ALDINE AVE,0820,THEFT,$500 AND UNDER,RESIDENCE,true,false,1924,019,44,6,06,1169898,1922154,2013,12/14/2013 12:39:48 AM,41.94189139041086,-87.65095594008946,"(41.94189139041086, -87.65095594008946)"

   The parameters for csv2Nanocube.py are listed below.  Note that when we called the script above, we specified the categorical dimension (crime) and the time dimension (time), as well as Latitude and Longitude.  However, the script is smart enough to identify the Latitude and Longitude columns automatically if they have these names.  If they were named differently (e.g. lat, long), we would have to use the other parameters for the script (--latcol, --loncol) to identify them for the script.  If your data is also separated by a character other than a comma, you can indicate this when you run the script using the '--sep' parameter.

        --catcol='Column names of categorical variable'
        --latcol='Column names of latitude'
        --loncol='Column names of longitude'
        --countcol='Column names of the count'
        --timecol='Column names of the time variable'
        --timebinsize='time bin size in seconds(s) minutes(m) hours(h) days(D)'
        --port='Port of the nanocubes server'
        --sep='Delimiter of Columns'
        e.g. 1D/30m/60s'

   The output generated by running the csv2Nanocube.py script should look like the following.
   You can see that 50,000 points were inserted into the nanocube, which is using 18MB of RAM.

        VERSION: 2014.10.11_16:40
        parent process is waiting...
        started redirecting stdin content to write-channel of parent-child pipe
        query-port:  29512
        insert-port: 0
        (stdin     ) count:      10000 mem. res:          6MB. time(s):          0
        (stdin     ) count:      20000 mem. res:          9MB. time(s):          1
        (stdin     ) count:      30000 mem. res:         12MB. time(s):          2
        (stdin     ) count:      40000 mem. res:         15MB. time(s):          2
        (stdin     ) count:      50000 mem. res:         18MB. time(s):          3
        (stdin:done) count:      50000 mem. res:         18MB. time(s):          3

5. That's it.  Point your browser (Firefox, Chrome, Safari) to http://localhost:8000 for the viewer. If you needed to change the port number in Step 3 above, make sure that you specify the same number here.

6. If you believe there may be a problem, try running 'nctest.sh' in the scripts subdirectory.  It will make some queries of the nanocube (change the script if you are not using port 29512) and compare the results to known results that we gathered ourselves.  If the results match, it will report 'SUCCESS'.

7. When finished, terminate the nanocube (e.g. Control-C) and then type 'deactivate' on the command-line to shut the virtual python environment down.

For this example we assume you are running everything on your
localhost. Modify `config.json` accordingly in the `web` folder for
different setups.

### Subsequent Runs

Running this example again later, you do not need to reinstall the linux or python packages.

        cd nanocube-3.0.1
        source myPy/bin/activate
        cd web
        python -m SimpleHTTPServer 8000 &
        cd ../scripts
        python csv2Nanocube.py --sep=',' --timecol='time' --latcol='Latitude' --loncol='Longitude' --catcol='crime' --port=29512 crime50k.csv | NANOCUBE_BIN=../src  ../src/nanocube-leaf --batch-size 1 --query-port 29512 --report-frequency 10000 --threads 100


## Further Details

For a better understanding on how to ingest data into nanocubes and
how to query nanocubes follow this
[link](https://github.com/laurolins/nanocube/wiki). For larger
datasets or if you want more flexibility on ingesting/querying data
using nanocubes the CSV loading method illustrated above might not be
the most efficient way to go.

## Asking for help

Our mailing list is the best and fastest way to ask questions and make suggestions related to nanocubes.
If you are having a problem, please search the archives before creating new topics to see if
your question has already been answered.  If you have other ideas for how we can improve
nanocubes, please let us know.

A nice front-end for our mailing list is now being served through [Nabble](http://nanocubes-discuss.64146.x6.nabble.com).
You should be able to post messages, search the archives, and even register as a new user from here.

The actual mailing list can be found [here](http://mailman.nanocubes.net/mailman/listinfo/nanocubes-discuss_mailman.nanocubes.net).
