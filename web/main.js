require.config({
    paths: {
	jquery: 'lib/jquery/jquery-1.11.1',
	leaflet: 'lib/leaflet/leaflet-src',
	d3: 'lib/d3/d3',
    },
    shim:{
	'nanocube3':{ deps:['jquery'] },
	'app':{ deps:['nanocube3'] }
    }
});

requirejs(['app'])
