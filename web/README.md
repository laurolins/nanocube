# Javascript Web Frontend for Nanocubes

## Configuration

The configuration of the web viewer can be modified through a JSON. The JSON file is named `config.json` or specified by a parameter

`http://myhost.com/?config=./myconfig.json`.

The `myconfig.json` file should reside with the directory that contains `index.html`.

`nanocube_webconfig` reads from the nanocube server and generates
a JSON con figuration file with all the tweakable parameters for the
widgets.

You may obtain the default config file from the chicago crime example by

```
nanocube_webconfig -s http://`hostname -f` --ncport 51234  > mycrimes.json
```

Here are some handy attributes.

* Colormap
* Number format
* Markers on the map
* View parameters
* Limit the bar chart to display top items
* Sort bar chart items by alphabetical order
* Log scale axes and limits

There is a `css` attribute for each widget, you may change the style
of the widget by modifying or inserting standard css elements.

You may test your configuration file by

```
nanocube_webconfig -s http://`hostname -f` --ncport 51234 --config  mycrimes.json --port 8000
```

## Deployment

Please copy the content of the `dist` directory along with your customized configuration to your  web hosting directory.


## SSL Web Server
The nanocubes backend server and the test server do not support SSL, we suggest you use a standard webserver such as `nginx` as your SSL entry point and redirect the traffic appropriately.


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
