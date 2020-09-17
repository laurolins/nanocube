# Nanocubes: an in-memory data structure for spatiotemporal data cubes

Nanocubes are a fast data structure for in-memory data cubes developed at the Information Visualization department at [AT&T Labs Research](http://www.research.att.com). Visualizations powered by nanocubes can be used to explore datasets with billions of elements at interactive rates in a web browser, and in some cases nanocubes uses sufficiently little memory that you can run a nanocube in a modern-day laptop.

# About this branch

The `master` branch now contains a new implementation of Nanocubes in the C programming language (version v4.0 on). The goal with this new implementation was to get a much finer control in all aspects of the data structure and specially on its memory aspects (allocation, layout). In our original C++ template-based implementation of Nanocubes (up to version 3.3), we implemented the Nanocube data structure on top of C++ STL (standard library) and while this was a reasonable solution at the time, it had some important downsides: (1) complex serialization which made it hard to save/load Nanocube into files; (2) variations in the internal memory layout of a Nanocube based on the specific STL implementation we used.

Here is a link to the new [API](/api/README.md)

# Docker Demo
```
$git clone https://github.com/laurolins/nanocube.git
$cd nanocube
#build the docker image
$docker build . -t nanocube 

#run the demo
$docker run -it --rm -p 12345:80 nanocube

# See the demo at http://localhost:12345/ zoom into Chicago
```

# Compiling on Linux or Mac

```shell
# Dependencies for Ubuntu 18.04
# sudo apt install build-essential curl unzip
#
# Dependencies for Mac OS X 10.13.4
# XCode

# get the v4 branch
curl -L -O https://github.com/laurolins/nanocube/archive/master.zip
unzip master.zip
cd nanocube-master

# modify INSTALL_DIR to point to another installation folder if needed
export INSTALL_DIR="$(pwd)/install"
./configure --with-polycover --prefix="$INSTALL_DIR"
make
make install

# Test if nanocubes is working
$INSTALL_DIR/bin/nanocube

# Add nanocube binaries to the PATH environment variable
export PATH="$INSTALL_DIR/bin":$PATH
```

# Creating and serving a nanocube index

```shell
# create a nanocube index for the Chicago Crime dataset (small example included)
# Inputs: (1) CSV data file, (2) mapping file (data/crime50k.map)
# Output: (1) nanocube index called data/crime50k.nanocube
nanocube create <(gunzip -c data/crime50k.csv.gz) data/crime50k.map data/crime50k.nanocube -header

# serve the nanocube index just created on port 51234
nanocube serve 51234 crimes=data/crime50k.nanocube &

# test querying the schema of the index
curl "localhost:51234/schema()"

# test querying the number of indexed records
curl "localhost:51234/format('text');q(crimes)"

# test querying the number of records per crime type
curl "localhost:51234/format('text');q(crimes.b('type',dive(1),'name'))"

```

For more information on `.map` files go to [mapping files](/MAPPING.md)

For more query examples go to [API](/api/README.md)

# Viewer
```shell
# Setup a web viewer on port 8000 for the crimes nanocube previously opened 
# on port 51234.
#
# Parameters:
#     -s         nanocube backend server (any http heachable machine)
#     --ncport   nanocube backend port
#     -p         port of the webviewer to be open in the localhost
#

nanocube_webconfig -s http://`hostname -f` --ncport 51234 -p 8000
```

Zoom into the Chicago region to see a heatmap of crimes.

![image](./doc/chicago_crime.png)

# Extra

For more advanced information follow this link: [extra](/EXTRA.md)

