/*global $ L colorbrewer d3 window */

var Map=function(opts,getDataCallback,updateCallback){
    this.getDataCallback = getDataCallback;
    this.updateCallback = updateCallback;

    this._datasrc = opts.datasrc;
    this._coarse_offset = opts.coarse_offset || 0;
    this._name = opts.name || 'defaultmap';
    this._tilesurl = opts.tilesurl ||
        'https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png';

    this._layers = this._genLayers(this._datasrc);
    this._maxlevels = opts.levels || 25;
    this._logheatmap = true;
    this._opts = opts;
    this.compare = false;
    this.colorsUsed = [];
    this.colorNumber = {};
    this.newLayerColors = {};

    this.retbrush = {
        color:'',
        x:'',
        y:''
    };
    
    
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
Map.brushcolors = colorbrewer.Paired[12].slice(0);
Map.nextcolor = function(){
    var c = Map.brushcolors.shift();
    Map.brushcolors.push(c);
    return c;
};
Map.shp = shp;

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
        
        for (var d in data){
            var layer = L.canvasOverlay(drawfunc,{opacity:0.7});

            //set datasrc
            layer._datasrc = d;

            //set color
            var midx = Math.floor(widget._datasrc[d].colormap.length /2);
            layer._color = widget._datasrc[d].colormap[midx];
            
            //set colormap
            layer._colormap = widget._datasrc[d].colormap.map(colorfunc);
            layer._colormap2 = widget._datasrc[d].colormap2.map(colorfunc);

            //htmllabel
            var htmllabel='<i style="background:'+
                    layer._color+'"> </i>' + d;
            
            layers[htmllabel] = layer;
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
            maxZoom: Math.min(this._maxlevels-8, 18)
        });

        //add base layer
        map.addLayer(mapt);

        //add nanocube layers
        for (var l in this._layers){
            map.addLayer(this._layers[l]);
        }

        //Layer
        if (Object.keys(this._layers).length > 1){
            L.control.layers(null,this._layers,
                             {
                                 collapsed: false,
                                 position: 'bottomright'
                             })
                .addTo(map);
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
        widget.compareShapes = [];

        map.on('draw:created', function (e) {
            drawnItems.addLayer(e.layer);

            if(widget.compare){
                widget.compareShapes.push(e.layer._leaflet_id);
                e.layer.setStyle({color: '#ffffff'});
                widget.updateCallback(widget._encodeArgs(),
                                  [{
                                      type:"SPATIAL",
                                      key:"#ffffff"
                                  }]);
                return;
            }

            if(firstShape){
                console.log(e.layer);
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

            if(widget.compare){
                return;
            }

            if(firstEdit){
                var popup = L.popup()
                    .setLatLng(latlng)
                    .setContent('<p>Double click inside a polygon to change its color!</p>')
                    .openOn(map);
                firstEdit = false;
            }

            drawnItems.on('dblclick', function(e){
                console.log(drawnItems);
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
            //console.log(files);
            var r = new FileReader();
            r.onload = function(e) {
                var gjw;
                var gj;
                console.log(typeof e.target.result);
                if((typeof e.target.result) == 'object'){
                    var geojson = shp.parseZip(e.target.result);
                    gjw = L.geoJson(geojson, {
                        style: {
                            "color": "#ffffff",
                            "opacity": 0.7
                        }
                    });
                    gj = L.geoJson(geojson, {
                        style: {
                            "color": initColor,
                            "opacity": 0.7
                        }
                    });
                    console.log(gj);
                }
                console.log(gj);
                try{
                    if(widget.compare){
                        if(gjw === undefined){
                            gjw = L.geoJson(JSON.parse(e.target.result), {
                                style: {
                                    "color": '#ffffff',
                                    "opacity": 0.7
                                }
                            });
                        }
                        console.log(gjw);
                        Object.keys(gjw._layers).map(function(k){
                            if(gjw._layers[k]._layers){
                                var lids = gjw._layers[k].getLayers().map(function(k){
                                    return k._leaflet_id;
                                });
                                Object.keys(gjw._layers[k]._layers).map(function(l){
                                    drawnItems.addLayer(gjw._layers[k]._layers[l]);
                                    widget.compareShapes.push(gjw._layers[k]._layers[l]._leaflet_id);
                                });
                            }
                            else{
                                drawnItems.addLayer(gjw._layers[k]);
                                widget.compareShapes.push(gjw._layers[k]._leaflet_id);
                            }
                        });
                        widget.updateCallback(widget._encodeArgs(),
                            [{
                                type:"SPATIAL",
                                key:'#ffffff'
                            }]);
                        return;

                    }
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

        if(this.compare){
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
        //force redraw
        this._map.fire('resize');        
        
        for(var l in this._layers){
            var layer = this._layers[l];
            if (!this._datasrc[layer._datasrc].disabled){
                layer._reset();
            }
        }
    },

    drawCanvasLayer: function(res,canvas,cmap,opacity,res2,cmap2){
        var arr;
        if(res2){
            arr = this.dataToArray(res.opts.pb,res.data,res2.data);
            this.render(arr,res.opts.pb,cmap,canvas,opacity,cmap2);
        }
        else{
            arr = this.dataToArray(res.opts.pb,res.data);
            this.render(arr,res.opts.pb,cmap,canvas,opacity);
        }
    },

    dataToArray: function(pb,data,data2){
        var origin = pb.min;
        var width = pb.max.x-pb.min.x+1;
        var height = pb.max.y-pb.min.y+1;

        //var arr = new Array(width*height).map(function () { return 0;});
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

        if(data2){
            var idx2 = Object.keys(data2);
            for (var j = 0, len2 = idx2.length; j < len2; j++){
                var jj = idx2[j];
                var d2 = data2[jj];
                var _i2 = d2.x - origin.x;
                var _j2 = d2.y - origin.y;
                if(_i2 <0 || _j2 <0 || _i2 >=width || _j2>=height){
                    continue;
                }
                var _idx2 = _j2 * width + _i2;
                if(arr[_idx2])
                    arr[_idx2] = arr[_idx2] - d2.val;
                else
                    arr[_idx2] = -d2.val;
            }
        }
        return arr;
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

    render: function(arr,pb,colormap,canvas,opacity,colormap2){
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
        var dom = d3.extent(colormap.domain());
        var dom2;
        if(colormap2)
            dom2 = d3.extent(colormap2.domain());

        for (var i = 0, len = idx.length; i < len; i++) {
            var ii= idx[i];
            var v = arr[ii];
            if(colormap2){
                if(v >= 0){
                    v = Math.max(v,dom[0]);
                    v = Math.min(v,dom[1]);
                }
                else{
                    v = -v;
                    v = Math.max(v,dom2[0]);
                    v = Math.min(v,dom2[1]);
                    v = -v;
                }
            }
            else{
                v = Math.max(v,dom[0]);
                v = Math.min(v,dom[1]);
            }
            var color;
            if(colormap2){
                if(v >= 0)
                    color = colormap(v);
                else{
                    v = -v;
                    color = colormap2(v);
                }
            }
            else{
                color = colormap(v);
            }
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
        var canvas = options.canvas;
        var ctx = canvas.getContext('2d');
        ctx.clearRect(0,0,canvas.width,canvas.height);

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
            console.log(promises);
            var promarray = Object.keys(promises).map(function(k){
                return promises[k];
            });
            var promkeys = Object.keys(promises);
            $.when.apply($,promarray).done(function(){
                var results = arguments;
                promkeys.forEach(function(d,i){
                    console.log('tiletime:',window.performance.now()-startdata);
                    if(widget.compare){
                        var label = d.split('&-&');
                        if(label[0] == "second")
                            return;
                    }
                    var res = results[i];
                    var res2;
                    if(widget.compare) res2 = results[i + 1];

                    widget._renormalize= true;

                    if(widget._renormalize){
                        var cmap = widget.normalizeColorMap(res.data,
                                                            layer._colormap,
                                                            widget._logheatmap);
                        var cmap2;
                        if(widget.compare){
                            cmap2 = widget.normalizeColorMap(res2.data,
                                                             layer._colormap2,
                                                             widget._logheatmap);
                        }
                        layer._cmap = cmap;
                        if(widget.compare) layer._cmap2 = cmap2;

                        widget._renormalize = false;


                        if(widget._opts.legend){
                            //update the legend
                            var ext = d3.extent(res.data,function(d){
                                return d.val;
                            });
                            
                            if (widget._logheatmap){ //log
                                ext = ext.map(function(d){ return Math.log(d); });
                            }
                            var valcolor = Array.apply(null, Array(5)).map(function (_, i) {return ext[0]+i * (ext[1]-ext[0])/5;});
                            
                            if (widget._logheatmap){ //anti log
                                valcolor = valcolor.map(function(d){ return Math.floor(Math.exp(d)+0.5); });
                            }
                            
                            valcolor = valcolor.map(function(d) {return {val:d, color: JSON.parse(JSON.stringify(cmap(d)))};});
                            widget.updateLegend(widget._map,valcolor);
                            console.log(widget._map); 
                        }
                    }
                    
                    var startrender = window.performance.now();
                    if(widget.compare){
                        widget.drawCanvasLayer(res,canvas,layer._cmap,
                                               layer.options.opacity,
                                               res2, layer._cmap2);
                    }
                    else{
                        widget.drawCanvasLayer(res,canvas,layer._cmap,
                                               layer.options.opacity);
                    }

                    console.log('rendertime:',
                                window.performance.now()-startrender);

                    //res.total_count =  res.data.reduce(function(p,c){
                    //    return p+c.val;
                    //},0);
                    //widget.updateInfo('Total: '+
                    //                  d3.format(',')(res.total_count));
                    
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
    adjustToCompare: function(){
        var map = this._map;
        var widget = this;
        if(this.compare){
            widget._drawnItems.eachLayer(function (layer) {
                layer.setStyle({color: '#ffffff'});
            });
            widget.updateCallback(widget._encodeArgs());
        }
        else{
            widget.compareShapes.map(function(k){
                widget._drawnItems.removeLayer(widget._drawnItems._layers[k]);
                // delete widget._drawnItems._layers[k];
                // map.removeLayer(widget._drawnItems._layers[k]);
            });
            console.log(widget._drawnItems);
            widget._drawnItems.eachLayer(function (layer) {
                console.log(widget.newLayerColors);
                var c = widget.newLayerColors[layer._leaflet_id];
                if(c !== undefined){
                    layer.setStyle({color: c});
                }
            });
            widget.updateCallback(widget._encodeArgs());
        }
    }
};
