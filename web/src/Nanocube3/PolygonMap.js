/*global $ L colorbrewer d3 window fetch turf */

var PolygonMap=function(opts,getDataCallback,updateCallback){
    this.getDataCallback = getDataCallback;
    this.updateCallback = updateCallback;

    this._datasrc = opts.datasrc;
    this._coarse_offset = opts.coarse_offset || 0;
    this._name = opts.name || 'defaultmap';
    this._tilesurl = opts.tilesurl ||
        'https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png';

    this._opts = opts;
    
    this._map = opts.map || this._initMap();

    this._selection=[];
    
    var widget = this;


    //load the initial polygons 
    fetch('sdma.geojson').then(function(data){
        return data.json();
    }).then(function(j){
        widget._features = widget._initFeatures(j);
    });

    
    //set according to url
    if(opts.args){
        this._decodeArgs(opts.args);
    }
    else{
        if ('viewbox' in opts){
            this.setSelection('global',opts.viewbox);
        }
        else if ('view' in opts){
            this.setSelection('global', opts.view);
        }
        else{
            this.setSelection('global', {c:{lat:0,lng:0},z:0});
        }
    }
};

var colormap = ['#009fdb','#ea7400'];
function nextColor(){
    var c = colormap.shift();
    return c; 
}

function putbackColor(c){
    colormap.unshift(c);
}


PolygonMap.prototype={
    _initFeatures:function(f){
        var map = this._map;
        var features = L.geoJSON(f);
        
        var unselected_style ={color:'white',weight:1};
        var selectedStyle = function(){
            var c = nextColor();
            if(c){
                return {color:c, weight:3};
            }
            else{
                return null;
            }
        };

        features.setStyle(unselected_style);
        
        var widget = this;
        features.eachLayer(function(layer){
            //put the name there
            layer.bindTooltip(layer.feature.properties.NAME);

            layer.on('click',function(e){
                var selection = widget._selection;

                //Deselect old layer
                if(selection.indexOf(layer) != -1){
                    selection = selection.filter(function(d){
                        return d !== layer;
                    });
                    putbackColor(layer.options.color);
                    layer.setStyle(unselected_style); // deselect
                }
                else{ //Select new layer
                    var selstyle= selectedStyle();
                    if(! selstyle) { // run out of colors
                        return;
                    }
                    layer.setStyle(selstyle);
                    layer.bringToFront();
                    selection.push(layer); //add to selection
                    layer.openPopup();
                }
                widget._selection = selection;
                widget.updateCallback(widget._encodeArgs(),[]);
                
            });
        });
        
        features.addTo(map);
        return features;
    },
    
    _initMap:function(){
        //Leaflet stuffs
        var map = L.map(this._name);
        
        map.attributionControl
            .addAttribution('<a href="http://www.nanocubes.net">Nanocubes&trade;</a>');
        map.attributionControl
            .addAttribution('<a href="http://www.osm.org">OpenStreetMap</a>');

        //make the background black
        $('.leaflet-container').css('background','#000');

        //add an OpenStreetMap tile layer
        var mapt = L.tileLayer(this._tilesurl,{
            noWrap:true,
            opacity:0.4,
            detectRetina:true,
            maxZoom: 18 //Math.min(this._maxlevels-8, 18)
        });

        mapt.addTo(map);

        var widget = this;
        //Refresh after move
        map.on('moveend', function(){ //update other views
            widget.updateCallback(widget._encodeArgs(),[]);
        });
        return map;
    },

    _encodeArgs: function(){
        var map = this._map;
        var args= {};
        args.global = {c:map.getCenter(),z:map.getZoom()};

        return JSON.stringify(args);
    },
    
    _decodeArgs: function(s){
        var map = this._map;
        var args = JSON.parse(s);
        var v = args.global;
        
        map.setView(v.c,v.z);
    },

    setSelection: function(key,v){
        var map =this._map;
        if (key == 'global'){
            if('c' in v && 'z' in v){
                //set the view
                map.setView(v.c,v.z);
            }
            else if (v.length==2){  //viewbox
                map.fitBounds(v);
            }
        }
    },

    getSelection: function(){
        var res = {};
        var map = this._map;

        //global
        var bb = map.getBounds();
        var sw = bb.getSouthWest();
        var ne = bb.getNorthEast();

        res.global = {
            coord: [[sw.lat,sw.lng],
                    [sw.lat,ne.lng],
                    [ne.lat,ne.lng],
                    [ne.lat,sw.lng]],
            zoom: map.getZoom() + 8
        };


        var selection = this._selection;
        selection.forEach(function(layer){
            var key = layer.options.color;
            var ll = layer.getLatLngs()[0];
            if (! ('lat' in ll[0])){
                ll = ll[0];
            }
            
            res[key] ={
                coord: ll.map(function(d){ return [d.lat,d.lng];}),
                zoom: map.getZoom() + 8
            };
        });
        
        console.log(res);
        return res;
    },

    update: function(){
        //force redraw
        this._map.invalidateSize();  
    }
};
