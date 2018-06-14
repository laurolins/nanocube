# Javascript Web Frontend for Nanocubes

## Configuration


The configuration of the web viewer can be modified through a JSON. The JSON file is named `config.json` or specified by a parameter

`http://myhost.com/?config=./myconfig.json`.

The `myconfig.json` file should reside with the directory that contains `index.html`.

The `ncwebviewer-config` reads from the nanocube server and generates
a JSON con figuration file with all the tweakable parameters for the
widgets.  Here are some handy examples.

* Colormap
* Number format
* Markers on the map
* View parameters
* Limit the bar chart to display top items
* Sort bar chart items by alphabetical order
* Log scale axes and limits

There is a `css` attribute for each widget, you may change the style
of the widget by modifying or inserting standard css elements.


## Development

If you are interested in developing this GUI, please install the dependencies for development by using `npm` and start the development server.

```
npm install
npm start
```
The development server will be started on the `dist` directory.
Please move your `json` configuration files into the `dist` directory after starting the server

For production builds, please run:

```
npm run build
```
