# Javascript Web Frontend for Nanocubes

## Configuration


The configuration of the web viewer can be modified through a JSON
file. The JSON file is named `config.json` or specified by a parameter
`http://myhost.com/?config=myconfig.json`.

The `ncwebviewer-config` reads from the nanocube server and generates
a JSON con figuration file with all the tweakable parameters for the
widgets.  Here are some handy examples.

* Colormap
* Number format
* Markers on the map
* View parameters
* Limit the bar chart to display top n items
* Sort bar chart items by alphabetical order
* Log scale axes


There is a `css` attribute for each widget, you may change the style
of the widget by modifying or inserting standard css elements.


## Development If you are interesting in developing this GUI, please
install the dependencies for development by using `npm`.

```
npm install
```

Use `Grunt` to process the Javascript files.

```
./node_modules/.bin/grunt
```