# Web-based Nanocube Viewer

We provide a basic web-interface for exploring a nanocube.

This web interface can be configured through a `json` file.

For example, we configure the viewing parameters for the Chicago Crime data via `config_crime.json`, the index.html file in the webserver reads the config from the  `?config=` or `#`

```
http://localhost:8000/index.html?config=config_crime
http://localhost:8000/index.html#config_crime
```

We provide the `ncwebviewer-config` script for generating a basic configuration. Users may customize the configurations as needed.  The configuration file is a json file with viewing parameters and styles for the charts and map.

* `title` : sets the title of the page.
* `url` : specifies the location of the nanocube server.
* `latlonbox` : specifies the initial view.
* `tilesurl` : specifies the url for map tiles by the leaflet conventions.
* `heatmapmaxlevel` : specifies the max zoom level of the heatmap

Each of the map, the charts, or the info line is implemented as a `div` in the webpage, the elements inside the `div` attributes specifies the CSS styles and additional parameters for the `div`.

The div names should match the names of the variables in the nanocube, with the exception of the latitudes and longitudes, they are merged into `location`.  The `info` element correspond to the time and count information on the top right hand corner.  Users may modify the position, size, and fonts as if they are modifying the CSS of the `div`.

Here are some addition configurations:
* Spatial variables: `colormap`: sets the heatmap colors.

```
"colormap":["rgba(0,0,0,0)","rgba(128,128,255,1.0)","rgba(0,0,255,1.0)","rgba(255,255,255,1.0)"]
```
* Categorial variables:
  * `displaynumcat`: sets how many values should the chart display.
  * `alpha_order`: if `true`, the categorical values are sorted in alphabetical order, if `false`, the values are sorted in descending order of count.
  * `logaxis`: if `true`, show log-scale axis.
  