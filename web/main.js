require.config({
    paths: {
	'jquery': 'lib/jquery/jquery-1.11.1',
	'jquery.mobile': 'lib/jquery.mobile/jquery.mobile-1.4.4.min',
	'd3': 'lib/d3/d3',
	'colorbrewer': 'lib/colorbrewer'
    },
    shim:{
	'jquery.mobile':{ deps:['jquery'] },
	'nanocube3':{ deps:['jquery'] },
	'app':{ deps:['nanocube3','jquery','jquery.mobile','colorbrewer','d3'] }
    }
});

//run...
requirejs(['app'])
