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
| 2.* | Soon: [New API](https://github.com/laurolins/nanocube/blob/api3/API.md) which requires modifications to clients |
| 2.1.2 | Minor fixes, better documentation, shutdown service |
| 2.1.1 | Fixed csv2Nanocube.py to work with pandas 0.14.0 |
| 2.1 | Javascript front-end, CSV Loading, Bug Fixes  |
| 2.0 | New feature-rich querying API                  |
| 1.0 | Original release with a simple querying API   |

## Compiling the latest release

**Prerequisites**

1. The nanocubes server is 64-bit only.  There is NO support on 32-bit operating systems.
2. The nanocubes server is written in C++ 11.  You must use a recent version of gcc (>= 4.7.2).
3. The nanocubes server uses [Boost](http://www.boost.org).  You must use version 1.48 or later.
4. To build the nanocubes server, you must have the [GNU build system](http://www.gnu.org/software/autoconf/) installed.

**Linux (Ubuntu)**

On a newly installed 64-bit Ubuntu 14.04 system, gcc/g++ is already 4.8.2, but you may have to install the following packages:

    $ sudo apt-get install automake
    $ sudo apt-get install libtool
    $ sudo apt-get install zlib1g-dev
    $ sudo apt-get install libboost-dev
    $ sudo apt-get install libboost-test-dev
    $ sudo apt-get install libboost-system-dev
    $ sudo apt-get install libboost-thread-dev

**Mac OS X (10.9)**

Example installation on Mac OS 10.9 Maverick with a local homebrew:

	$git clone https://github.com/mxcl/homebrew.git

Set your path to use this local homebrew				

	$export PATH=${PWD}/homebrew/bin:${PATH}

Install the packages (This assumes your g++ has been installed by [XCode](https://developer.apple.com/xcode/))

	brew install boost libtool autoconf automake

Set path to the boost directory

	export BOOST_ROOT=${PWD}/homebrew

**General Instructions**

Run the following commands to compile nanocubes on your linux/mac system. Replace `X.X.X`
with valid release numbers (e.g. 2.1.1, 2.1, 2.0).

    $ wget https://github.com/laurolins/nanocube/archive/X.X.X.zip
    $ unzip X.X.X.zip
    $ cd nanocube-X.X.X
    $ ./bootstrap
    $ ./configure
    $ make

If a recent version of gcc is not the default, you can run `configure`
with the specfic recent version of gcc in your system. For example

    $ CXX=g++-4.8 ./configure

**Tcmalloc**

We strongly suggest linking nanocubes with [Thread-Caching Malloc](http://goog-perftools.sourceforge.net/doc/tcmalloc.html), or tcmalloc for short.
It is faster than the default system malloc, and in some cases, we found that the amount of memory used by nanocubes was reduced
by over 50% when using libtcmalloc.  To install on a Ubuntu 14.04 machine, install the following package and all of its dependencies.

    $ sudo apt-get install libgoogle-perftools-dev

You must then re-run the configure script, indicating support for tcmalloc.

    $ ./configure --with-tcmalloc
    $ make clean
    $ make


## Loading a CSV file into a nanocube

1. For compiling our python helper code, you will need the following packages:

        $ sudo apt-get install python-dev
        $ sudo apt-get install gfortran

2. Install the python data analysis library (pandas) in a separate python environment (Recommended)

        $ wget http://pypi.python.org/packages/source/v/virtualenv/virtualenv-1.11.4.tar.gz
        $ tar xfz virtualenv-1.11.4.tar.gz
        $ python virtualenv-1.11.4/virtualenv.py  myPy
        
        # activate the virtualenv, type "deactivate" to disable the env when done
        $ source myPy/bin/activate
        $ pip install argparse numpy pandas

3. Start a web server in the "web" directory and send it to background.  If port 8000 is already being used
on your system, please choose another port.

        $ cd web
        $ python -m SimpleHTTPServer 8000 &

4. Run the script and pipe it to the nanocubes server using the
   included example dataset
   ([Chicago Crime](https://data.cityofchicago.org/Public-Safety/Crimes-2001-to-present/ijzp-q8t2)).  If port 29512 is already
   being used on your system, please choose another port.  Note that the port is specified for both the python script and for
   the nanocubes server (nanocube-leaf). If these are not the same, you'll run into problems.

        $ cd ../scripts
        $ python csv2Nanocube.py --sep=',' --timecol='time' --latcol='Latitude' --loncol='Longitude' --catcol='crime' --port=29512 crime50k.csv | NANOCUBE_BIN=../src  ../src/nanocube-leaf --batch-size 1 --query-port 29512 --report-frequency 10000 --threads 100


   Please note: We modified the original dataset slightly in this example by changing the names of two columns in the header.  In previous versions,
there were columns called 'Date' and 'Primary Type'.  We have renamed these 'time' and 'crime' to avoid any confusion and make the subsequent
examples easier to read.

   The first few lines of the example dataset are shown below. The first line is a header, which describes each of the columns in this table of data.
   You should notice that there are columns called: time, crime, Latitude, Longitude.  These are the columns used for this visualization.

        ID,Case Number,time,Block,IUCR,crime,Description,Location Description,Arrest,Domestic,Beat,District,Ward,Community Area,FBI Code,X Coordinate,Y Coordinate,Year,Updated On,Latitude,Longitude,Location
        9435145,HW579013,12/21/2013 04:05:00 AM,013XX S KILDARE AVE,0420,BATTERY,AGGRAVATED:KNIFE/CUTTING INSTR,RESIDENTIAL YARD (FRONT/BACK),false,false,1011,010,24,29,04B,1147977,1893242,2013,12/23/2013 12:39:51 AM,41.863004448921934,-87.7322698761511,"(41.863004448921934, -87.7322698761511)"
        9435117,HW578998,12/21/2013 04:15:00 AM,005XX N LAWLER AVE,0486,BATTERY,DOMESTIC BATTERY SIMPLE,APARTMENT,false,true,1532,015,28,25,08B,1142638,1903220,2013,01/05/2014 12:39:48 AM,41.89048623626327,-87.75162080720938,"(41.89048623626327, -87.75162080720938)"
        9457369,HW579005,12/21/2013 04:15:00 AM,077XX S ADA ST,0486,BATTERY,DOMESTIC BATTERY SIMPLE,RESIDENCE,false,true,0612,006,17,71,08B,1168828,1853330,2013,01/23/2014 12:40:19 AM,41.75305549754325,-87.65688114331137,"(41.75305549754325, -87.65688114331137)"
        9435159,HW579015,12/21/2013 04:30:00 AM,049XX S KEDZIE AVE,1305,CRIMINAL DAMAGE,CRIMINAL DEFACEMENT,CTA PLATFORM,true,false,0821,008,14,63,14,1155836,1871729,2013,12/23/2013 12:39:51 AM,41.80381553747461,-87.7039986610427,"(41.80381553747461, -87.7039986610427)"


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
   You can see that 49,186 points were inserted into the nanocube, which is using 18MB of RAM.

        VERSION: 2014.10.10_15:35
        parent process is waiting...
        started redirecting stdin content to write-channel of parent-child pipe
        query-port:  29512
        insert-port: 0
        (stdin     ) count:      10000 mem. res:          6MB. time(s):          0
        (stdin     ) count:      20000 mem. res:          9MB. time(s):          1
        (stdin     ) count:      30000 mem. res:         12MB. time(s):          2
        (stdin     ) count:      40000 mem. res:         15MB. time(s):          2
        (stdin:done) count:      49186 mem. res:         18MB. time(s):          3


5. That's it.  Point your browser (Firefox, Chrome, Safari) to http://localhost:8000 for the viewer. If you needed to change the port number in Step 3 above, make sure that you specify the same number here.

6. If you believe there may be a problem, try running 'nctest.sh' in the scripts subdirectory.  It will make some queries of the nanocube (change the script if you are not using port 29512) and compare the results to known results that we gathered ourselves.  If the results match, it will report 'SUCCESS'.

7. When finished, terminate the nanocube (e.g. Control-C) and then type 'deactivate' on the command-line to shut the virtual python environment down.

For this example we assume you are running everything on your
localhost. Modify `config.json` accordingly in the `web` folder for
different setups.

### Subsequent Runs

Running this example again later, you do not need to reinstall the linux or python packages.

        $ cd nanocube-X.X.X
        $ source myPy/bin/activate
        $ cd web
        $ python -m SimpleHTTPServer 8000 &
        $ cd ../scripts
        $ python csv2Nanocube.py --sep=',' --timecol='time' --latcol='Latitude' --loncol='Longitude' --catcol='crime' --port=29512 crime50k.csv | NANOCUBE_BIN=../src  ../src/nanocube-leaf --batch-size 1 --query-port 29512 --report-frequency 10000 --threads 100


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
