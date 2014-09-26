# Development Version to update the Nanocube's API

Here is a still messy document towards the API documentation

[API](https://github.com/laurolins/nanocube/blob/api3/API.md)

# Nanocubes: an in-memory data structure for spatiotemporal data cubes

Nanocubes are a fast datastructure for in-memory data cubes developed
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
| 2.1 | Javascript front-end, CSV Loading, Bug Fixes  |
| 2.0 | New feature-rich querying API                  |
| 1.0 | Original release with a simple querying API   |

## Compiling latest release

**Prerequisites** The nanocubes server is written in C++ 11 (gcc >=4.7.2). You'll need a
recent version of [boost](http://www.boost.org) (>=1.48) and the
[GNU build system](http://www.gnu.org/software/autoconf/).

Run the following commands to compile nanocubes on your linux/mac system.

    $ wget https://github.com/laurolins/nanocube/archive/2.1.zip
    $ unzip 2.1.zip
    $ cd nanocube-2.1
    $ ./bootstrap
    $ ./configure
    $ make

If a recent version of gcc is not the default, you can run `configure`
with the specfic recent version of gcc in your system. For example

    $ CXX=g++-4.8 ./configure

If your system has tcmalloc installed, we suggest linking nanocubes with it.
For example

    $ CXX=g++-4.8 ./configure --with-tcmalloc

## Loading a CSV file into a nanocube

This procedure assumes your machine is not running anything on ports
8000 and 29512

1. Installing Pandas in a separate python env (Optional)

        $ wget http://pypi.python.org/packages/source/v/virtualenv/virtualenv-1.11.4.tar.gz
        $ tar xfz virtualenv-1.11.4.tar.gz
        $ python virtualenv-1.11.4/virtualenv.py  myPy
        
        # activate the virtualenv, type "deactivate" to disable the env
        $ source myPy/bin/activate
        $ pip install argparse numpy pandas

2. Start a web server in the "web" directory and send it to background

        $ cd web
        $ python -m SimpleHTTPServer &

3. Run the script and pipe it to the nanocubes server using the
   included example dataset
   ([Chicago Crime](https://data.cityofchicago.org/Public-Safety/Crimes-2001-to-present/ijzp-q8t2))

        $ cd ../scripts
        $ python csv2Nanocube.py --port=29510 --sep=',' --catcol='crime' --timecol='time' crime50k.csv | NANOCUBE_BIN=../src  ../src/nanocube-leaf -b 1 -q 29510

This should be it. Point your browser to http://localhost:8000 for the
viewer

        Parameters for csv2Nanocube.py
        --catcol='Column names of categorical variable'
        --latcol='Column names of latitude'
        --loncol='Column names of longitude'
        --countcol='Column names of the count'
        --timecol='Column names of the time variable'
        --timebinsize='time bin size in seconds(s) minutes(m) hours(h) days(D)
        --sep='Delimiter of Columns'
        e.g. 1D/30m/60s'


## Further Details

For a better understanding on how to ingest data into nanocubes and
how to query nanocubes follow this
[link](https://github.com/laurolins/nanocube/wiki). For larger
datasets or if you want more flexibility on ingesting/querying data
using nanocubes the CSV loading method illustrated above might not be
the most efficient way to go.

## Asking for help

IRC: #nanocubes channel on irc.freenode.net

mailing list: http://mailman.nanocubes.net/mailman/listinfo/nanocubes-discuss_mailman.nanocubes.net
