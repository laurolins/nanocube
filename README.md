# Nanocubes: an in-memory data structure for spatiotemporal data cubes

Nanocubes are a fast datastructure for in-memory data cubes developed
at the
[Information Visualization department](http://www.research.att.com/~infovis)
at [AT&T Labs – Research](http://www.research.att.com). Visualizations
powered by nanocubes can be used to explore datasets with billions of
elements at interactive rates in a web browser, and in some cases nanocubes
uses sufficiently little memory that you can run a nanocube in a
modern-day laptop.

## News

<table>
<tr>
<td>
2013-10-22  
</td>
<td>
<a href="https://github.com/laurolins/nanocube/wiki">Overview on how to use nanocubes</a>
</td>
</tr>
</table>

## Prerequisites

The nanocubes server is written in C++ 11. You'll need
[boost](http://www.boost.org) and the
[GNU build system](http://www.gnu.org/software/autoconf/). Nanocubes
use [mongoose](https://github.com/valenok/mongoose) internally as its
web server, but we include a copy of the mongoose source in the tree.


## Building

    $ git clone https://github.com/laurolins/nanocube.git
    $ ./bootstrap
	$ CXX='g++-4.7' ./configure
	$ make


## Running

The way nanocube servers currently work is they read a dataset from
`stdin` and then start an HTTP server which answers queries. There's
no authentication mechanism! You should solve this at a different
level of your stack.

You'll also need a client application that can issue nanocube queries
and build visualizations with it. We offer an example Javascript
client API
[here](https://github.com/laurolins/nanocube/tree/1.0/api/js), and are
currently working on a complete, self-contained example. We'll
be releasing the C++ client as open source in the near future as well.

You'll quickly learn that the nanocube HTTP server is not as, ahem,
battle-hardened as you and we would like it to be. We welcome feedback
on the form of [email](mailto:cscheid@research.att.com),
[issues](http://github.com/laurolins/nanocube/issues), and,
especially,
[pull requests](http://github.com/laurolins/nanocube/pulls).


## Branches

You should probably start with the
[1.0 branch](https://github.com/laurolins/nanocube/tree/1.0).  The
master branch is currently much more unstable than the 1.0 branch,
which is the one that hosts the nanocube server code that's currently
running on [nanocubes.net](http://nanocubes.net).
