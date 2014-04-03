function initPage(config){
    //set the title
    $(document).attr('title', config.title);
    
    for (var d in config.div){
        //insert the divs
        $("body").append("<div id="+ d +"></div>");
        //set CSS
        $("#"+d).css(config.div[d]);
    }
}

function initNanocube(config){
    var nc = new Nanocube({ 
        url:config.url, 
        ready: function(nc){
            //for colormap use
            var cdomain = new Array(11);
            for(var i =0; i < cdomain.length; i++){
                cdomain[i] = i*1.0/(cdomain.length-1);
            }
            
            //set options (colormap etc)
            var options = {
                nanocube:nc,
                colormaps: [{colors:colorbrewer.YlOrRd[9].reverse(),
                             domain:cdomain}],
                maptileurl:config.tileurl
            };

            //create the model
            var model = new Model(options);

            //set the initial view
            for(var sp in config.latlonbox.min){
                model.spatial_vars[sp].map.fitBounds([config.latlonbox.min[sp],
                                                      config.latlonbox.max[sp]
                                                     ]);
            }
        }
    });
};
