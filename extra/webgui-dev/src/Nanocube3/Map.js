/*global $ L colorbrewer d3 window */

var Map=function(opts,getDataCallback,updateCallback, getXYCallback){
    this.getDataCallback = getDataCallback;
    this.updateCallback = updateCallback;
    this.getXYCallback = getXYCallback;

    this._datasrc = opts.datasrc;
    this._coarse_offset = opts.coarse_offset || 0;
    this._name = opts.name || 'defaultmap';

    this._tilesurl = opts.tilesurl ||
        'https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png';

    this.retx = ['default'];
    this.rety = ['default'];
    this.retbrush = {
        color:'',
        x:'',
        y:''
    };
    this._layers = this._genLayers(this._datasrc);
    this._maxlevels = opts.levels || 25;
    this._logheatmap = true;
    this._opts = opts;
    this.compare = false;
    this.colorsUsed = [];
    this.colorNumber = {};
    this.newLayerColors = {};

    
    
    
    var map = this._initMap();

    this._map = map;

    //add Legend
    if (opts.legend){
        this._addLegend(map);
    }

    
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

    if('layers' in opts){
        if ('markers' in opts.layers){
            opts.layers.markers.forEach(function(d){
                var m = L.marker(d.coordinate);
                m.bindPopup(d.popup);
                m.addTo(map);
            });
        }                        
    }

    

};

//Setup static variables and functions
Map.brushcolors = colorbrewer.Accent[8].slice(0);
// console.log(Map.brushcolors);
Map.nextcolor = function(){
    var c = Map.brushcolors.shift();
    Map.brushcolors.push(c);
    return c;
};
Map.shp = shp;
Map.heatcolormaps = {
    "#e41a1c": colorbrewer.Reds[9].slice(0).reverse(),
    "#377eb8": colorbrewer.Blues[9].slice(0).reverse(),
    "#4daf4a": colorbrewer.Greens[9].slice(0).reverse(),
    "#984ea3": colorbrewer.Purples[9].slice(0).reverse(),
    "#ff7f00": colorbrewer.Oranges[9].slice(0).reverse(),

    "#7fc97f": colorbrewer.Greens[9].slice(0).reverse(),
    "#beaed4": colorbrewer.Purples[9].slice(0).reverse(),
    "#fdc086": colorbrewer.Oranges[9].slice(0).reverse(),
    "#ffff99": colorbrewer.YlOrRd[9].slice(0).reverse(),
    "#386cb0": colorbrewer.Blues[9].slice(0).reverse(),
    "#f0027f": colorbrewer.Reds[9].slice(0).reverse(),
    "#bf5b17": colorbrewer.YlOrBr[9].slice(0),
    "#666666": colorbrewer.Greys[9].slice(0).reverse()

};

function arraysEqual(arr1, arr2) {
    if(arr1.length !== arr2.length)
        return false;
    for(var i = arr1.length; i--;) {
        if(arr1[i] !== arr2[i])
            return false;
    }

    return true;
}

function hexToColor(color){
    var colors = {"#e41a1c":"Red", "#377eb8":"Blue","#4daf4a":"Green",
                  "#984ea3":"Purple","#ff7f00":"Orange", "#7fc97f":"Green", 
                  "#beaed4":"Purple", "#fdc086":"Orange", "#ffff99":"Yellow", 
                  "#386cb0":"Blue", "#f0027f":"Red", "#bf5b17":"Brown", 
                  "#666666":"Gray"};
    if(typeof colors[color] != 'undefined')
        return colors[color];
    return color;
}


Map.prototype = {
    _genLayers: function(data){
        var widget = this;
        var layers = {};
        function drawfunc(layer,options){
                widget._canvasDraw(layer,options);
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

        var hcmaps = {};
        Object.keys(Map.heatcolormaps).map(function(h){
            hcmaps[h] = Map.heatcolormaps[h].map(colorfunc);
        });
        // console.log(widget.retx,widget.rety);
        for (var d in data){
            for (var i in widget.retx){
                for(var j in widget.rety){
                    var layer = L.canvasOverlay(drawfunc,{opacity:0.7});

                    layer.zIndex = -100;
                    //set datasrc
                    layer._datasrc = d;

                    //set color
                    var midx = Math.floor(widget._datasrc[d].colormap.length /2);
                    layer._color = widget._datasrc[d].colormap[midx];
                    
                    //set colormap
                    layer._colormap = widget._datasrc[d].colormap.map(colorfunc);

                    layer._hcmaps = hcmaps;

                    layer._xy = [widget.retx[i],widget.rety[j]];

                    var label='X: '+hexToColor(widget.retx[i])+
                              ' Y: '+hexToColor(widget.rety[j]);
                    
                    layers[label] = layer;
                }
            }
        }
        return layers;
    },
    
    _initMap: function(viewbbox){
        var widget = this;
        
        //Leaflet stuffs
        var map = L.map(this._name,{detectRetina:true,
                                    attribution: '<a href="https://www.mapbox.com/about/maps/">Terms and Feedback</a>'});

        //make the background black
        $('.leaflet-container').css('background','#000');

        //add an OpenStreetMap tile layer
        var mapt = L.tileLayer(this._tilesurl,{
            noWrap:true,
            opacity:0.4,
            maxZoom: Math.min(this._maxlevels-8, 18),
            zIndex: -1000
        });

        //add base layer
        map.addLayer(mapt);

        //add nanocube layers
        for (var l in this._layers){
            map.addLayer(this._layers[l]);
        }

        //Layer
        map.layercontrol = L.control.layers(this._layers, null,
                                             {
                                                 collapsed: false,
                                                 position: 'bottomright'
                                             });
        map.layercontrol.addTo(map);

        map.on('baselayerchange', function(e){
            console.log("what");
            e.layer._reset();
            // e.layer.bringToBack();
            // mapt.bringToBack();
            widget.updateCallback(widget._encodeArgs(),[],
                                  widget._datasrc);
        });


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

        return JSON.stringify(args);
    },

    _decodeArgs: function(s){
        var map = this._map;
        var args = JSON.parse(s);
        var v = args.global;
        
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
            this._renormalize = true;
            this.update();
            break;
        case 78: //n
            this._renormalize = true;
            this.update();
            break;
        default:
            return;
        }
    },

    _initDrawingControls: function(map){
        var drawnItems = new L.FeatureGroup();
        map.addLayer(drawnItems);
        var widget = this;
        
        var initColor = Map.nextcolor();

        var drawingoptions = function(){
            return { shapeOptions:{ color: initColor } };
        };

        map.drawControl = new L.Control.Draw({
            draw: {
                rectangle: drawingoptions(),
                polygon: drawingoptions(),
                polyline:false,
                circle:false,
                marker:false
            },
            edit: {
                featureGroup: drawnItems,
                edit:{
                    selectedPathOptions: {maintainColor: true}
                }
            }
        });

        map.addControl(map.drawControl);

        var firstShape = true;
        var firstEdit = true;
        var latlng;
        // widget.compareShapes = [];

        map.on('draw:created', function (e) {
            drawnItems.addLayer(e.layer);

            // if(widget.compare){
            //     widget.compareShapes.push(e.layer._leaflet_id);
            //     e.layer.setStyle({color: '#ffffff'});
            //     widget.updateCallback(widget._encodeArgs(),
            //                       [{
            //                           type:"SPATIAL",
            //                           key:"#ffffff"
            //                       }]);
            //     return;
            // }

            if(firstShape){
                // console.log(e.layer);
                var p1 = e.layer._latlngs[0];
                var p2 = e.layer._latlngs[1];
                var p3 = e.layer._latlngs[2];
                latlng = [2 * Math.abs(p1.lat - p2.lat) / 3 + Math.min(p1.lat, p2.lat), 
                          (p2.lng + p3.lng) / 2];
                firstShape = false;
                widget.colorNumber[initColor] = 0;
                widget.colorsUsed.push(initColor);
            }

            else{
                var nextColor = Map.nextcolor();
                widget.colorNumber[nextColor] = widget.colorsUsed.length;
                widget.colorsUsed.push(nextColor);
            }

            //keep track of color
            widget.newLayerColors[e.layer._leaflet_id] = e.layer.options.color;

            //add constraints to the other maps
            widget.updateCallback(widget._encodeArgs(),
                                  [{
                                      type:"SPATIAL",
                                      key:e.layer.options.color
                                  }]);
            
            //Set color for the next shape
            // var options = {};
            // options[e.layerType] = drawingoptions();
            // map.drawControl.setDrawingOptions(options);
        });

        

        map.on('draw:editstart', function(e){

            var overlay = $('.leaflet-overlay-pane');
            // console.log(overlay.first(), overlay.last());
            // map.addLayer(drawnItems);

            // if(widget.compare){
            //     return;
            // }

            if(firstEdit){
                var popup = L.popup()
                    .setLatLng(latlng)
                    .setContent('<p>Double click inside a polygon to change its color!</p>')
                    .openOn(map);
                firstEdit = false;
            }

            drawnItems.on('dblclick', function(e){
                // console.log(drawnItems);
                // console.log(e.layer);
                var cn = widget.colorNumber[e.layer.options.color] + 1;
                if(cn >= widget.colorsUsed.length)
                    cn = 0;
                if(e.layer.lids){
                    e.layer.lids.map(function(k){
                        drawnItems._layers[k].setStyle({color: widget.colorsUsed[cn]});
                        widget.newLayerColors[k] = e.layer.options.color;
                    });

                }
                else{
                    e.layer.setStyle({color: widget.colorsUsed[cn]});
                    widget.newLayerColors[e.layer._leaflet_id] = e.layer.options.color;
                }
                
                widget.updateCallback(widget._encodeArgs(),
                                  [{
                                      type:"SPATIAL",
                                      key:e.layer.options.color
                                  }]);
            });

        });

        map.on('draw:edited', function (e) {
            widget.updateCallback(widget._encodeArgs());
        });

        map.on('draw:editstop', function (e) {
            map.addLayer(drawnItems);
            drawnItems.eachLayer(function (layer) {
                var c = widget.newLayerColors[layer._leaflet_id];
                if(c !== undefined){
                    layer.setStyle({color: c});
                }
            });
            drawnItems.off('dblclick');
            widget.updateCallback(widget._encodeArgs());
        });

                    

        map.on('draw:editing', function (e) {
            widget.updateCallback(widget._encodeArgs()) ;          
        });

        map.on('draw:deleted', function(e) {
            //add constraints to the other maps
            widget.updateCallback(widget._encodeArgs());
        });

        map.on('draw:deletestart', function(e){
            drawnItems.on('click', function(e){
                if(e.layer.lids){
                    e.layer.lids.map(function(k){
                        widget._drawnItems.removeLayer(widget._drawnItems._layers[k]);
                    });

                }
            });
        });

        map.on('draw:deletestop', function(e){
            drawnItems.off('click');
        });

        $(document).on('dragenter', function (e) 
        {
            e.stopPropagation();
            e.preventDefault();
        });
        $(document).on('dragover', function (e) 
        {
          e.stopPropagation();
          e.preventDefault();
        });
        $(document).on('drop', function (e) 
        {
            e.stopPropagation();
            e.preventDefault();
        }); 

        var obj = $('#'+this._name);
        obj.on('dragenter', function (e) {
            e.stopPropagation();
            e.preventDefault();
        });
        obj.on('dragover', function (e) {
             e.stopPropagation();
             e.preventDefault();
        });
        obj.on('drop', function (e) {
            e.preventDefault();
            var files = e.originalEvent.dataTransfer.files;
            var r = new FileReader();
            r.onload = function(e) {
                var gj;
                if((typeof e.target.result) == 'object'){
                    var geojson = shp.parseZip(e.target.result);

                    gj = L.geoJson(geojson, {
                        style: {
                            "color": initColor,
                            "opacity": 0.7
                        }
                    });
                }
                try{

                    if(gj === undefined){
                        gj = L.geoJson(JSON.parse(e.target.result), {
                            style: {
                                "color": initColor,
                                "opacity": 0.7
                            }
                        });
                    }
                    if(firstShape){
                        var center = gj.getBounds().getCenter();
                        latlng = [center.lat, center.lng];
                        firstShape = false;
                        widget.colorNumber[initColor] = 0;
                        widget.colorsUsed.push(initColor);
                    }
                    else{
                        var nextColor = Map.nextcolor();
                        widget.colorNumber[nextColor] = widget.colorsUsed.length;
                        widget.colorsUsed.push(nextColor);
                    }
                    var col;
                    Object.keys(gj._layers).map(function(k){
                        if(gj._layers[k]._layers){
                            var lids = gj._layers[k].getLayers().map(function(k){
                                return k._leaflet_id;
                            });
                            Object.keys(gj._layers[k]._layers).map(function(l){
                                gj._layers[k]._layers[l].lids = lids;
                                drawnItems.addLayer(gj._layers[k]._layers[l]);
                                col = gj._layers[k]._layers[l].options.color;
                                widget.newLayerColors[gj._layers[k]._layers[l]._leaflet_id] = col;
                            });
                        }
                        else{
                            drawnItems.addLayer(gj._layers[k]);
                            col = gj._layers[k].options.color;
                            widget.newLayerColors[gj._layers[k]._leaflet_id] = col;
                        }
                    });

                    widget.updateCallback(widget._encodeArgs(),
                        [{
                            type:"SPATIAL",
                            key:col
                        }]);
                }
                catch(err){
                    console.log(err);
                }
            };
            for (var i = 0; i < files.length; i++){
                if(files[i].name.endsWith('.zip'))
                    try{
                        r.readAsArrayBuffer(files[i]);
                    }
                    catch(err){
                        console.log(err);
                    }

                else{
                    try{
                        r.readAsText(files[i]);
                    }
                    catch(err){
                        console.log(err);
                    }
                }
            }
        });

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
        res.global.coord = [[[sw.lat,sw.lng],
                            [sw.lat,ne.lng],
                            [ne.lat,ne.lng],
                            [ne.lat,sw.lng]]];

        res.global.zoom = map.getZoom() + 8;

        if(!this.checkRet()){
            if(this._drawnItems.getLayers().length === 0){
                return res;
            }
            else{
                res.brush = {};
                res.brush.coord = [];
                this._drawnItems.getLayers().forEach(function(d){
                    res.brush.coord.push(d._latlngs.map(function(d){
                        return [d.lat,d.lng];
                    }));
                });
                res.brush.zoom = map.getZoom() + 8;
                res.global = undefined;
                return res;
            }
        }

        //add polygonal constraints  
        this._drawnItems.getLayers().forEach(function(d){
            if(res[d.options.color] && res[d.options.color].coord){
                res[d.options.color].coord
                    .push(d._latlngs.map(function(d){
                        return [d.lat,d.lng];
                    }));
            }
            else{
                res[d.options.color] = {};
                res[d.options.color] = {
                    coord: [d._latlngs.map(function(d){
                        return [d.lat,d.lng];
                    })],
                    zoom: map.getZoom() + 8
                };
            }                               
        });
        return res;
    },

    update: function(){
        var map = this._map;
        var widget = this;
        var xydata = this.getXYCallback();
         if(!arraysEqual(this.retx,xydata[0]) || !arraysEqual(this.rety,xydata[1])){
            console.log("Rebuilding..");
            this.retx = xydata[0];
            this.rety = xydata[1];
            for (var l1 in this._layers){
                map.removeLayer(this._layers[l1]);
                map.layercontrol.removeLayer(this._layers[l1]);

            }

            this._layers = this._genLayers(this._datasrc);
            for (var l2 in this._layers){

                map.layercontrol.addBaseLayer(this._layers[l2],l2);
            }
        }
        //force redraw
        this._map.fire('resize');
        this._map.fire('moveend');



        // for(var l in this._layers){
        //     var layer = this._layers[l];
        //     if (!this._datasrc[layer._datasrc].disabled){
        //         layer._reset();
        //     }
        // }


    },

    drawCanvasLayer: function(res,canvas,cmaps,opacity){
        var keys = Object.keys(res);
        var pb = res[keys[0]].opts.pb;
        var data = {};
        Object.keys(res).map(function(k){
            data[k] = res[k].data;
        });
        var arr = this.dataToArray(pb,data);
        // console.log(data, arr);
        this.render(arr[0],arr[1],pb,cmaps,canvas,opacity);

    },

    dataToArray: function(pb,data){
        var origin = pb.min;
        var width = pb.max.x-pb.min.x+1;
        var height = pb.max.y-pb.min.y+1;

        //var arr = new Array(width*height).map(function () { return 0;});
        var tarr = [];
        var max = [];

        //Explicit Loop for better performance
        Object.keys(data).map(function(k){
            var idx = Object.keys(data[k]);
        
            for (var i = 0, len = idx.length; i < len; i++) {
                var ii= idx[i];
                var d = data[k][ii];
                var _i = d.x - origin.x;
                var _j = d.y - origin.y;
                if(_i <0 || _j <0 || _i >=width || _j>=height){
                    continue;
                }
                var _idx =  _j*width+_i;
                if(tarr[_idx]){
                    if(Math.max.apply(null,tarr[_idx]) < d.val)
                        max[_idx] = k;
                    tarr[_idx].push(d.val);
                }
                else{
                    tarr[_idx] = [d.val];
                    max[_idx] = k;
                }
            }
        });

        // console.log(tarr);

        var arr = [];

        function add(a,b){
            return a + b;
        }

        var idx2 = Object.keys(tarr);
        for (var j = 0, len = idx2.length; j < len; j++) {
            var jj= idx2[j];
            var values = tarr[jj];
            var m = Math.max.apply(null, values);
            if(values.length == 1)
                arr[jj] = m;
            else{
                var rest = values.reduce(add, 0);
                rest -= m;
                rest /= (values.length - 1);
                arr[jj] = m - rest;
            }
        }
        // console.log(arr);

        return [arr, max];
    },

    normalizeColorMap: function(data,colors,log){
        var ext = d3.extent(data,function(d){
            return d.val;
        });

        var minv = ext[0];
        if (log){ //log
            ext = ext.map(function(d){return Math.log(d-minv+2);});
        }

        //compute domain
        var interval = (ext[1]-ext[0])/(colors.length-1);
        var domain=Array.apply(null,Array(colors.length)).map(function(d,i){
            return i*interval+ext[0];
        });

        if (log){ //anti log
            domain = domain.map(function(d){return Math.exp(d)+minv-2;});
        }

        return d3.scaleLinear().domain(domain).range(colors);
    },

    render: function(arr,max,pb,colormaps,canvas,opacity){
        var realctx = canvas.getContext("2d");        
        var width = pb.max.x-pb.min.x+1;
        var height = pb.max.y-pb.min.y+1;

        //create a proxy canvas
        var c = $('<canvas>').attr("width", width).attr("height", height)[0];
        var proxyctx = c.getContext("2d");
        var imgData = proxyctx.createImageData(width,height);

        var buf = new ArrayBuffer(imgData.data.length);
        var buf8 = new Uint8ClampedArray(buf);
        var pixels = new Uint32Array(buf);

        //Explicit Loop for better performance
        var idx = Object.keys(arr);
        var dom = {};

        Object.keys(colormaps).map(function(k){
            dom[k] = d3.extent(colormaps[k].domain());
        });

        for (var i = 0, len = idx.length; i < len; i++) {
            var ii= idx[i];
            var v = arr[ii];
            var k = max[ii];
            v = Math.max(v, dom[k][0]);
            v = Math.min(v, dom[k][1]);
            var color;
            color = colormaps[k](v);

            // color.a *= opacity;
            pixels[ii] =
                (color.a << 24) |         // alpha
                (color.b << 16) |         // blue
                (color.g <<  8) |         // green
                color.r;                  // red
        }

        imgData.data.set(buf8);
        proxyctx.putImageData(imgData, 0, 0);

        //Clear
        realctx.imageSmoothingEnabled = false;
        realctx.mozImageSmoothingEnabled = false;
        realctx.clearRect(0,0,canvas.width,canvas.height);

        //draw onto the real canvas ...
        realctx.drawImage(c,0,0,canvas.width,canvas.height);
    },

    _canvasDraw: function(layer,options){
        // console.log(layer);
        var canvas = options.canvas;

        // canvas.attr("transform", "translate3d(0px, 0px, -5px)");
        var ctx = canvas.getContext('2d');
        // console.log(ctx);
        

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
            // console.log(promises);
            var promarray = Object.keys(promises).map(function(k){
                return promises[k];
            });
            var promkeys = Object.keys(promises);
            ctx.clearRect(0,0,canvas.width,canvas.height);
            $.when.apply($,promarray).done(function(){
                var results = arguments;
                var res = {};
                promkeys.forEach(function(d,i){
                    console.log('tiletime:',window.performance.now()-startdata);
                    var label = d.split('&-&');
                    var xyc = label[0].split('&');
                    var ret = {};
                    xyc.map(function(k){
                        ret[k.charAt(0)] = k.substring(1);
                    });

                    //check ret.x, ret.y
                    if(ret.x != layer._xy[0] && layer._xy[0] != 'default')
                        return;
                    if(ret.y != layer._xy[1] && layer._xy[1] != 'default')
                        return;

                    if(ret.c){
                        res[ret.c] = results[i];
                    }
                    else{
                        res.global = results[i];
                    }
                });
                widget._renormalize = true;
                if(widget._renormalize){
                    var cmaps = {};
                    Object.keys(res).map(function(c){
                        if(c == 'global'){
                            cmaps.global = widget.normalizeColorMap(res.global.data,
                                                                    layer._colormap,
                                                                    widget._logheatmap);
                        }
                        else{
                            cmaps[c] = widget.normalizeColorMap(res[c].data,
                                                                layer._hcmaps[c],
                                                                widget._logheatmap);
                        }
                    });
                    layer._cmaps = cmaps;
                    widget._renormalize = false;

                    if(widget._opts.legend){
                        //idk
                    }
                }
                var startrender = window.performance.now();
                widget.drawCanvasLayer(res,canvas,layer._cmaps,layer.options.opacity);
                console.log('rendertime:', window.performance.now()-startrender);
            });
        }
        catch(err){
            ctx.clearRect(0,0,canvas.width,canvas.height);
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
        var legend = L.control({position: 'bottomleft'});
        
        legend.onAdd = function (map) {
            var div = L.DomUtil.create('div', 'legendinfo legend');
            return div;
        };          

        legend.addTo(map);
    },
    updateLegend: function(map,valcolor){
        var legend = d3.select(map._container).select('.legend');
        var htmlstr= valcolor.map(function(d) {
            var colorstr = 'rgb('+parseInt(d.color.r) +','+parseInt(d.color.g)+','+parseInt(d.color.b)+')';
            return '<i style="background:'+colorstr+'"></i>' + d.val;
        });
        legend.html(htmlstr.join('<br />'));
    },
    checkRet: function(){
        return this.retbrush.color == this._name || this.retbrush.x == this._name ||
            this.retbrush.y == this._name;
    },
    adjustToCompare: function(){
        // var map = this._map;
        // var widget = this;
        // if(this.compare){
        //     widget._drawnItems.eachLayer(function (layer) {
        //         layer.setStyle({color: '#ffffff'});
        //     });
        //     widget.updateCallback(widget._encodeArgs());
        // }
        // else{
        //     widget.compareShapes.map(function(k){
        //         widget._drawnItems.removeLayer(widget._drawnItems._layers[k]);
        //         // delete widget._drawnItems._layers[k];
        //         // map.removeLayer(widget._drawnItems._layers[k]);
        //     });
        //     // console.log(widget._drawnItems);
        //     widget._drawnItems.eachLayer(function (layer) {
        //         // console.log(widget.newLayerColors);
        //         var c = widget.newLayerColors[layer._leaflet_id];
        //         if(c !== undefined){
        //             layer.setStyle({color: c});
        //         }
        //     });
        //     widget.updateCallback(widget._encodeArgs());
        // }
    }
};
