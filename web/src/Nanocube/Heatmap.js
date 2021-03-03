//Leaflet
import 'leaflet';
import 'leaflet/dist/leaflet.css';
let L = window.L;
//Leaflet Icon Fix
import markericon from 'url:~/node_modules/leaflet/dist/images/marker-icon-2x.png';
import markericon2x from 'url:~/node_modules/leaflet/dist/images/marker-icon-2x.png';
import markershadow from 'url:~/node_modules/leaflet/dist/images/marker-shadow.png';
delete L.Icon.Default.prototype._getIconUrl;
L.Icon.Default.mergeOptions({
    iconRetinaUrl: markericon,
    iconUrl: markericon2x,
    shadowUrl: markershadow
});


//Leaflet Draw
import 'leaflet-draw';
import 'leaflet-draw/dist/leaflet.draw.css';

//Canvas Layer
import './L.CanvasLayer';

//JQuery
import jquery from 'jquery';
let $ = window.$ = jquery;

//d3
import * as d3 from 'd3';

//colorbrewer
import colorbrewer from 'colorbrewer';

let fetch = window.fetch;
var Heatmap=function(opts,getDataCallback,updateCallback){
    this.getDataCallback = getDataCallback;
    this.updateCallback = updateCallback;

    this._datasrc = opts.datasrc;
    this._coarse_offset = opts.coarse_offset || 0;
    this._name = opts.name || 'defaultmap';
    this._tilesurl = opts.tilesurl ||
        'https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png';

    this._maxlevels = opts.levels || 25;
    this._logheatmap = true;
    opts.map_opacity=opts.map_opacity || 0.4;
    opts.heatmap_opacity=opts.heatmap_opacity || 0.7;
    this._opts = opts;
    
    this._heatmaps = this._genLayers(this._datasrc);
    
    var map = opts.map || this._initMap(opts);
    
    this._map = map;
    
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

        this._range = opts.range || undefined;
    }

    
    //add Legend
    if (opts.legend){
        this._addLegend(map);
    }

    if('layers' in opts){
        if ('markers' in opts.layers){
            opts.layers.markers.forEach(function(d){
                var m = L.marker(d.coordinate);
                m.bindPopup(d.popup);
                m.addTo(map);
            });
        }

        if ('geojson' in opts.layers){
            let widget = this; 
            opts.layers.geojson.forEach(gj=>{
                fetch(gj).then(r=>r.json())
                    .then(j=>{
                        widget.geojson = {
                            data: L.geoJSON(j,{onEachFeature:(f,l)=>
                                               l.bindPopup(JSON.stringify(f.properties)),
                                               style:{fillOpacity:0}
                                              }),
                            fg: L.featureGroup()
                        };
                        widget.geojson.fg.addTo(map);
                        widget.update();
                    });
            });
        }
    }
};

//Setup static variables and functions
Heatmap.brushcolors = colorbrewer.Paired[12].slice(0);
Heatmap.nextcolor = function(){
    var c = Heatmap.brushcolors.shift();
    Heatmap.brushcolors.push(c);
    return c;
};

Heatmap.prototype = {
    _genLayers: function(data){
        var widget = this;
        var layers = {};
        function drawfunc(info){
            widget._canvasDraw(info.layer,info);
        }

        function colorfunc(d,i,array){
            var m = d.match(/rgba\((.+),(.+),(.+),(.+)\)/);
            if(m){
                d={r:+m[1],g:+m[2],b:+m[3],a:+m[4]*255};
                return d;
            }
            else{
                d = d3.rgb(d);
                return {r:d.r, g:d.g, b:d.b, a:i/array.length*255};
            }
        }
        
        for (var d in data){
            var layer = new L.CanvasLayer();
            layer.options.opacity=widget._opts.heatmap_opacity;
            layer.onDrawLayer=drawfunc;
            
            //set datasrc
            layer._datasrc = d;

            //set color
            var midx = Math.floor(widget._datasrc[d].colormap.length /2);
            layer._color = widget._datasrc[d].colormap[midx];
            
            //set colormap
            layer._colormap = widget._datasrc[d].colormap.map(colorfunc);

            //html label
            var htmllabel='<i style="background:'+
                layer._color+'"> </i>' + d;
            
            layers[htmllabel] = layer;
        }
        return layers;
    },

    _initMap: function(viewbbox){
        var widget = this;
        
        //Leaflet stuffs
        var map = L.map(this._name,{preferCanvas:true});

        map.attributionControl.addAttribution('<a href="http://www.nanocubes.net">Nanocubes&trade;</a>');
        map.attributionControl.addAttribution('<a href="http://www.osm.org">OpenStreetMap</a>');

        
        //make the background black
        $('.leaflet-container').css('background',
                                    this._opts.background || '#000');

        //add an OpenStreetMap tile layer
        var mapt = L.tileLayer(this._tilesurl,{
            noWrap:true,
            opacity: widget._opts.map_opacity,
            maxZoom: Math.min(this._maxlevels-9, 18)
        });


        //add base layer
        map.addLayer(mapt);

        //add nanocube layers
        for (var l in this._heatmaps){
            map.addLayer(this._heatmaps[l]);
        }

        //layer control
        if (Object.keys(this._heatmaps).length > 1){
            L.control.layers(null,this._heatmaps,{
                collapsed: false,
                position: 'bottomright'
            }).addTo(map);
        }

        map.on('overlayadd', function (e) {
            widget._datasrc[e.layer._datasrc].disabled=false;
            widget.updateCallback(widget._encodeArgs(),[],
                                  widget._datasrc);
        });

        map.on('overlayremove', function (e) {
            widget._datasrc[e.layer._datasrc].disabled=true;
            widget.updateCallback(widget._encodeArgs(),[],
                                  widget._datasrc);
            
        });

        //Refresh after move
        map.on('moveend', function(){ //update other views
            widget.updateCallback(widget._encodeArgs(),[]);
        });

        //add keyboard hooks with JQuery
        $(map._container).keydown(function(e){
            widget._keyboardShortcuts(e);
        });

        this._maptiles = mapt;
        this._initDrawingControls(map);
        this._renormalize=true;

        //add info
        //$('#'+this._name).append('<p class="info">info test</p>');
        //style
        /*var infodiv = $('#'+this._name+" .info");
          infodiv.css({
          position: 'absolute',
          'z-index':1,
          color: 'white',
          'right': '20ch',
          'top': '0.5em',
          'padding':'0px',
          'margin':'0px'
          });*/

        //add title
        d3.select('#'+this._name)
            .append('div')
            .attr('class','maptitle')
            .text(this._name);
        
        return map;
    },

    _encodeArgs: function(){
        var map = this._map;
        var args= {};
        args.global = {c:map.getCenter(),z:map.getZoom()};
        args.range = this._range;
        
        return JSON.stringify(args);
    },

    _decodeArgs: function(s){
        var map = this._map;
        var args = JSON.parse(s);
        var v = args.global;

        if (args.range){
            this._range = args.range;
        }
        
        map.setView(v.c,v.z);
    },

    _keyboardShortcuts: function(e){
        console.log(e);
        switch(e.keyCode){
        case 190: //.
            this.changeHeatmapRes(1);
            break;
        case 188: //,
            this.changeHeatmapRes(-1);
            break;
        case 66: //b
            this.changeMapOpacity(0.1);
            break;
        case 68: //d
            this.changeMapOpacity(-0.1);
            break;
        case 76: //l
            this._logheatmap = !this._logheatmap;
            //this._renormalize = true;
            this.update();
            break;
        //case 78: //n
            //this._renormalize = true;
            //this.update();
            //break;
        default:
            return;
        }
    },

    drawGeoJSONLayers: function(){
        let map = this._map;
        let widget = this;
        let bb=map.getBounds();
        if(widget.geojson){
            let fg=widget.geojson.fg;
            let layers = widget.geojson.data.getLayers();
                let active_layers = layers.filter(g=>bb.intersects(g.getBounds())); 
            
            fg.clearLayers();
            console.log('a',active_layers.length);
            if(active_layers.length < 500){
                active_layers.forEach(layer=>{
                    fg.addLayer(layer);                    
                });
            }
            else{
                active_layers.forEach(layer=>{
                    fg.addLayer(L.circleMarker(layer.getBounds().getCenter(), {radius:3,
                                                                               fillOpacity:0.5,
                                                                               stroke:0}));            
                });                
            }
        }
    },

    _initDrawingControls: function(map){
        var drawnItems = new L.FeatureGroup();
        map.addLayer(drawnItems);
        var widget = this;
        
        var drawingoptions = function(){
            return { shapeOptions:{ color: Heatmap.nextcolor() } };
        };

        map.drawControl = new L.Control.Draw({
            draw: {
                rectangle: drawingoptions(),
                polygon: drawingoptions(),
                polyline:false,
                circle:false,
                marker:false,
                circlemarker:false
            },
            edit: {
                featureGroup: drawnItems,
                edit:{
                    selectedPathOptions: {maintainColor: true}
                }
            }
        });

        map.addControl(map.drawControl);

        map.on(L.Draw.Event.CREATED, function (e) {
            drawnItems.addLayer(e.layer);
            
            //add constraints to the other maps
            widget.updateCallback(widget._encodeArgs(),
                                  [{
                                      type:"SPATIAL",
                                      key:e.layer.options.color
                                  }]);
            
            //Set color for the next shape
            var options = {};
            options[e.layerType] = drawingoptions();
            map.drawControl.setDrawingOptions(options);
        });

        function updateWidget(e){
            widget.updateCallback(widget._encodeArgs()) ;          
        }
            
        map.on(L.Draw.Event.EDITED, updateWidget);

        map.on(L.Draw.Event.EDITMOVE, updateWidget);

        map.on(L.Draw.Event.EDITRESIZE, updateWidget);

        map.on(L.Draw.Event.DELETED, updateWidget);

        this._drawnItems = drawnItems;
    },

    addConstraint:function(constraint){
        if(constraint.type != "SPATIAL"){
            return;
        }

        var map = this._map;

        var key = constraint.key;
        var event = constraint.event || null;

        var shape;
        if(!event){  // create a rect
            var s = map.getSize();
            var nw = map.containerPointToLatLng([s.x*0.25,s.y*0.25]); 
            var se = map.containerPointToLatLng([s.x*0.75,s.y*0.75]); 
            shape = L.rectangle([nw,se],{color:key});
        }

        this._drawnItems.addLayer(shape);
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

        var bb = map.getBounds();
        var sw = bb.getSouthWest();
        var ne = bb.getNorthEast();


        res.global = {};
        res.global.coord = [[sw.lat,sw.lng],
                            [sw.lat,ne.lng],
                            [ne.lat,ne.lng],
                            [ne.lat,sw.lng]];

        res.global.zoom = map.getZoom() + 8;

        //add polygonal constraints  
        this._drawnItems.getLayers().forEach(function(d){
            res[d.options.color]={};
            res[d.options.color] = {
                coord: d.getLatLngs()[0].map(function(d){
                    return [d.lat,d.lng];
                }),
                zoom: map.getZoom() + 8
            };                                     
        });
        return res;
    },

    update: function(){
        let map = this._map;

        //force redraw
        map.invalidateSize();  
        
        for(var l in this._heatmaps){
            var layer = this._heatmaps[l];
            if (!this._datasrc[layer._datasrc].disabled){
                layer.needRedraw();
            }
        }
    },

    drawCanvasLayer: function(res,canvas,cmap,opacity){
        var arr = this.dataToArray(res.opts.pb,res.data);
        this.render(arr,res.opts.pb,cmap,canvas,opacity);
        this.drawGeoJSONLayers();
    },

    dataToArray: function(pb,data){
        var origin = pb.min;
        var width = pb.max.x-pb.min.x+1;
        var height = pb.max.y-pb.min.y+1;

        var arr = [];
        //Explicit Loop for better performance
        var idx = Object.keys(data);
        for (var i = 0, len = idx.length; i < len; i++) {
            var ii= idx[i];
            var d = data[ii];
            var _i = d.x - origin.x;
            var _j = d.y - origin.y;
            if(_i <0 || _j <0 || _i >=width || _j>=height){
                continue;
            }
            var _idx =  _j*width+_i;
            arr[_idx] = d.val;
        }
        return arr;
    },

    genColorMap: function(data,colors,log,ext){
        if (ext == undefined){
            ext = d3.extent(data,function(d){
                return d.val;
            });
        }

        var minv = ext[0];
        if (log){ //log
            ext = ext.map(function(d){return Math.log(d-minv+2);});
        }

        //compute domain
        var interval = (ext[1]-ext[0])/(colors.length-1);
        var domain=Array.apply(null,Array(colors.length))
            .map(function(d,i){
                return i*interval+ext[0];
            });

        if (log){ //anti log
            domain = domain.map(function(d){return Math.exp(d)+minv-2;});
        }

        return d3.scaleLinear().domain(domain).range(colors);
    },

    render: function(arr,pb,colormap,canvas,opacity){
        var realctx = canvas.getContext("2d");        
        var width = pb.max.x-pb.min.x+1;
        var height = pb.max.y-pb.min.y+1;

        //create a proxy canvas
        var c = document.createElement('canvas');
        c.width = width;
        c.height = height;
        
        var proxyctx = c.getContext('2d');
        var imgData = proxyctx.createImageData(width,height);
        var buf = new ArrayBuffer(imgData.data.length);
        var buf8 = new Uint8ClampedArray(buf);
        var pixels = new Uint32Array(buf);

        //Explicit Loop for better performance
        var idx = Object.keys(arr);
        var dom = d3.extent(colormap.domain());

        for (var i = 0, len=idx.length; i < len; i++) {
            var ii= idx[i];
            var v = arr[ii];
            v = Math.max(v,dom[0]);
            v = Math.min(v,dom[1]);            
            var color = colormap(v);
            color.a *= opacity;
            pixels[ii] =
                (color.a << 24) |         // alpha
                (color.b << 16) |         // blue
                (color.g <<  8) |         // green
                color.r;                  // red
        }

        imgData.data.set(buf8);
        proxyctx.putImageData(imgData, 0, 0);

        //copy onto the real canvas ...
        realctx.mozImageSmoothingEnabled = false;
        realctx.webkitImageSmoothingEnabled = false;
        realctx.msImageSmoothingEnabled = false;
        realctx.imageSmoothingEnabled = false;
        realctx.globalCompositeOperation = 'copy';
        realctx.drawImage(c,0,0,canvas.width,canvas.height);
    },

    _canvasDraw: function(layer,info){
        var canvas = info.canvas;
        var ctx = canvas.getContext('2d');
        var map = this._map;

        var z = map.getZoom();
        z = Math.min(z, this._maxlevels-8);
        z -= this._coarse_offset;

        var startdata = window.performance.now();
        var widget = this;

        var bb = map.getBounds();
        var nw = bb.getNorthWest();
        var se = bb.getSouthEast();

        var bbox = { min:[nw.lat,nw.lng], max:[se.lat,se.lng] } ;

        try{
            var promises = widget.getDataCallback(layer._datasrc,bbox,z);
            var promarray = Object.keys(promises).map(function(k){
                return promises[k];
            });
            var promkeys = Object.keys(promises);
            $.when.apply($,promarray).done(function(){
                var results = arguments;
                promkeys.forEach(function(d,i){
                    console.log('tiletime:',window.performance.now()-startdata);
                    
                    var res = results[i];
                    widget._renormalize= true;
                    
                    if(widget._renormalize){
                        var cmap = widget.genColorMap(res.data,
                                                      layer._colormap,
                                                      widget._logheatmap,
                                                      widget._range);
                        layer._cmap = cmap;
                        widget._renormalize = false;


                        if(widget._opts.legend){
                            //update the legend
                            var ext = widget._range;

                            if(ext == undefined){
                                ext = d3.extent(res.data,function(d){
                                    return d.val;
                                });
                            }
                            
                            if (widget._logheatmap){ //log
                                ext = ext.map(function(d){
                                    return Math.log(d);
                                });
                            }
                            var valcolor = Array.apply(null, Array(5))
                                .map(function (_, i) {
                                    return ext[0]+i * (ext[1]-ext[0])/5;
                                });
                            
                            if (widget._logheatmap){ //anti log
                                valcolor = valcolor.map(function(d){
                                    return Math.floor(Math.exp(d)+0.5); });
                            }
                            
                            valcolor = valcolor.map(function(d) {
                                return {
                                    val:d,
                                    color: JSON.parse(JSON.stringify(cmap(d)))
                                };
                            });

                            widget.updateLegend(widget._map,valcolor);
                        }
                    }
                    
                    var startrender = window.performance.now();
                    widget.drawCanvasLayer(res,canvas,layer._cmap,
                                           layer.options.opacity);
                    console.log('rendertime:',
                                window.performance.now()-startrender);
                    
                });
            });
        }
        catch(err){
            console.log(err);
        }
    },
    changeHeatmapRes: function(levels){
        var offset = this._coarse_offset+levels;
        offset = Math.max(0,offset);
        offset = Math.min(8,offset);
        this._coarse_offset = offset;
        this.update(); //force redraw
    },
    changeMapOpacity: function(o){
        var op = this._maptiles.options.opacity+o;
        op = Math.max(0.0,op);
        op = Math.min(1.0,op);
        this._maptiles.setOpacity(op);
    },

    updateInfo: function(html_str){
        $('#'+this._name+" .info").html(html_str);
    },

    _addLegend: function(map){
        var legend = L.control({position: 'bottomright'});
        
        legend.onAdd = function (map) {
            var div = L.DomUtil.create('div', 'legendinfo legend');
            return div;
        };          

        legend.addTo(map);
    },
    updateLegend: function(map,valcolor){
        var legend = d3.select(map._container).select('.legend');
        var htmlstr= valcolor.map(function(d,i) {
            var colorstr = 'rgb('+parseInt(d.color.r) + ',' + 
                parseInt(d.color.g)+','+parseInt(d.color.b)+')';
            
            
            var prefix='';
            if (i == 0){
                prefix = '&le; ';
            }

            if(i == valcolor.length-1){
                prefix = '&ge; ';
            }
            
            return '<i style="background:'+colorstr+'"></i>' +
                prefix + Math.floor(d.val*100)/100.0 ;
        });
        legend.html(htmlstr.join('<br />'));
    }
};

export default Heatmap;
