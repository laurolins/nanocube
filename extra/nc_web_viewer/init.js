function initPage(config){
    //set the title
    $(document).attr('title', config.title);

    var contents = [];
    for (var d in config.div){
	//insert the divs
	$("#maincontent").prepend("<div id="+ d +"></div>");

	var div = $("#"+d);
	//set CSS
	div.css(config.div[d]);

	if(div.height() <  1){
	    contents.push(div);
	}
    }

    $(window).on("resize load orientationchange", function(){
	contents.forEach(function(div){
	    //this will not work for multi maps
	    div.height($("#nc-container").height());
	    div.width($("#nc-container").width());
	});
    });

    $(window).resize(); //force resize on the first call
}


function initNanocube(config){
    var nc = new Nanocube({
	url:config.url,
	ready: function(nc){
	    //set options (colormap etc)
	    var options = {
		nanocube:nc,
		config: config,
		tilesurl:config.tilesurl,
		heatmapmaxlevel:config.heatmapmaxlevel
	    };

	    //create the model
	    var model = new Model(options);

	    //set the initial view
	    for(var sp in config.latlonbox.min){
		model.spatial_vars[sp]
		    .map.fitBounds([config.latlonbox.min[sp],
				    config.latlonbox.max[sp]]);
	    }
	}
    });
};
