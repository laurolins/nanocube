/*global fetch */

//Leaflet
import 'leaflet';
import 'leaflet/dist/leaflet.css';
let L = window.L;
//Leaflet Icon Fix
import markericon from 'leaflet/dist/images/marker-icon-2x.png';
import markericon2x from 'leaflet/dist/images/marker-icon-2x.png';
import markershadow from 'leaflet/dist/images/marker-shadow.png';
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
var PolygonMap=function(opts,getDataCallback,updateCallback){
    this.getDataCallback = getDataCallback;
    this.updateCallback = updateCallback;

    this._opts = opts;
    this._datasrc = opts.datasrc;
    this._coarse_offset = opts.coarse_offset || 0;

    this._name = opts.name || 'defaultmap';
    this._tilesurl = opts.tilesurl ||
        'https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png';
    
    this._map = opts.map || this._initMap();
    this._selection={};
    
    
    var widget = this;

    var mapdiv = d3.select('#'+this._name);

    //Add dropbox
    this.norm_sel = mapdiv
        .append('select').attr('class','dropdownlist');
    this.norm_sel.selectAll('option')
        .data(['none', 'total']).enter()
        .append('option')
        .attr('value',function(d){ return d;})
         .html(function(d){ return d;});    

    this.norm_sel.on('change', function(){
        widget.norm_const = widget.norm_sel.property('value');
        widget.update();
        widget.updateCallback(widget._encodeArgs(),[]);
    });

    if(opts.norm_const){
        this.norm_sel.property('value',opts.norm_const);
        this.norm_sel.dispatch('change');
    }
   
    try{
        widget.transfunc = {
            trans: eval(opts.heatmapfunc['trans']),    //jshint ignore:line
            inverse: eval(opts.heatmapfunc['inverse']) //jshint ignore:line
        };
    }
    catch(err){
        widget.transfunc = {
            trans:function(x){return x;},
            inverse: function(x){return x;}
        };
    }

    if(opts.logheatmap){
        widget.transfunc={
            'trans':function(x){ return Math.log(x);},
            'inverse':function(x){ return Math.exp(x);}            
        };
    }    
    
    //load the initial polygons 
    fetch(opts.geojson).then(function(data){
        return data.json();
    }).then(function(j){
        widget._layers = widget._genLayers(j);
        widget.update();
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
    _genLayers:function(features){
        var datasrc = this._datasrc;
        var layers = {};
        var widget = this;
        var datacallback =this.getDataCallback;

        Object.keys(datasrc).forEach(function(d){
            var layer = widget._initFeatures(d,features);
            layer.colormap = datasrc[d].colormap;
            layers[d]=layer;

            //propagate total for normalization
            var p = datacallback(layer._datasrc,true);
            $.when.apply($,Object.keys(p).map(d=>p[d])).done(function(){
                let results =arguments[0];
                layer.eachLayer(polygon=>{                        
                    let e = results.data
                        .find(d=>(+d.cat==polygon.feature.properties.KEY))||
                        {val:0}; 
                    polygon.feature.properties.total = e.val;
                });
            });
        });
        return layers;
    },
    _initFeatures:function(d,json){
        var map = this._map;
        var features = L.geoJSON(json);
        features._datasrc= d;            
        var unselected_style ={color:'white',weight:1};
        var laststyle=null;
        var selectedStyle = function(){
            var c = nextColor();

            if(c){
                laststyle = {color:c, weight:3};
                return laststyle;
            }
            else{
                return null;
            }
        };

        features.setStyle(unselected_style);                
        var widget = this;
        features.eachLayer(function(polygon){ 
            polygon.on('click',function(e){
                let selection=widget._selection;
                
                if(!e.originalEvent.shiftKey && 'brush' in selection){
                    return;
                }
                
                if(e.originalEvent.shiftKey){
                    if(Object.keys(selection).length ==0 ){
                        selection.brush=widget.allcats.slice(0);
                    }
                    if(selection.brush.indexOf(polygon)!=-1){ //new
                        selection.brush=selection.brush.filter(function(d){
                            return d!=polygon; 
                        });
                    }
                    else{
                        selection.brush.push(polygon);
                    }

                    if(selection.brush.length == widget.allcats.length){
                        delete selection.brush;
                    }
                    
                    widget._selection = selection;
                    widget.update();
                    widget.updateCallback(widget._encodeArgs(),[]);
                    return;
                }
                
                
                if(e.originalEvent.metaKey){
                    if(polygon.options.color in selection){
                        selection = selection[polygon.options.color].filter(function(d){
                            return d!=polygon;
                        });
                        polygon.setStyle(unselected_style); // deselect
                        polygon.bringToBack();
                    }
                    else{
                        if(! laststyle) { //no prev sel
                            return;
                        }
                        polygon.setStyle(laststyle);
                        polygon.bringToFront();
                        
                        //add to selection
                        selection[polygon.options.color].push(polygon);
                        polygon.openPopup();
                    }
                    widget._selection = selection;
                    widget.updateCallback(widget._encodeArgs(),[]);             
                    return;                    
                }

                    
                //Deselect old layer
                if(polygon.options.color in selection){
                    delete selection[polygon.options.color];

                    putbackColor(polygon.options.color);
                    polygon.setStyle(unselected_style); // deselect
                    polygon.bringToBack();
                }
                else{ //Select new layer
                    var selstyle= selectedStyle();
                    if(! selstyle) { // run out of colors
                            return;
                    }
                    polygon.setStyle(selstyle);
                    polygon.bringToFront();

                    //add to selection
                    selection[polygon.options.color] = [polygon];
                    polygon.openPopup();
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
        var args= this.getSelection();
        args.global = {c:map.getCenter(),z:map.getZoom()};
        args.norm_const = this.norm_sel.property('value') || 'none';
        
        return JSON.stringify(args);
    },
    
    _decodeArgs: function(s){
        var map = this._map;
        var args = JSON.parse(s);
        var v = args.global;
        
        map.setView(v.c,v.z);

        if(args.norm_const){
            this.norm_sel.property('value',args.norm_const);
            this.norm_sel.dispatch('change');
        }
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
        var selection = this._selection;
        var res = {};
        
        Object.keys(selection).forEach(function(k){
            res[k] = selection[k].map(function(p){
                return {cat:p.feature.properties.KEY,
                        id:p.feature.properties.id};
            });
        });
        return res;
    },

    update: function(){
        var layers = this._layers;
        if(!layers){
            return;
        }
        
        var datacallback =this.getDataCallback;
        var map = this._map;
        var widget = this;
        
        Object.keys(layers).forEach(function(k){
            var layer = layers[k];
            var promises={};
            
            var p  = datacallback(layer._datasrc);                
            Object.keys(p).forEach(function(k){
                promises[k] = p[k];
            });

            var promarray = Object.keys(promises).map(function(k){
                return promises[k];
            });
            
            var promkeys = Object.keys(promises);
            $.when.apply($,promarray).done(function(){
                var results = arguments;
                var res = {};
                promkeys.forEach(function(d,i){
                    res[d] = results[i];
                });
                widget.redrawLayer(res,layer);
            });
            
        });
        map.invalidateSize();
    },

    normalizeColorMap: function(data,colors){
        var ext = d3.extent(data,function(d){
            return d.val;
        });

        var minv = ext[0];
        var widget = this;
        ext = ext.map(function(d){return widget.transfunc.trans(d-minv+2);});
        
        
        if((ext[1]-ext[0])/ext[0]< 1e-4){
            ext[0]= ext[1] - 1e-4*Math.abs(ext[0]);
        }

        //compute domain
        var interval = (ext[1]-ext[0])/(colors.length-1);
        var domain=Array.apply(null, Array(colors.length))
            .map(function(d,i){
                return i*interval+ext[0];
            });

        domain = domain.map(function(d){
            return widget.transfunc.inverse(d)+minv-2;
        });

        return  d3.scaleLinear().domain(domain).range(colors);
    },

    redrawLayer: function(res,layer){
        var widget = this;
        Object.keys(res).forEach(function(k){
            var data = res[k].data;

            if(widget._selection.brush){
                var ind = widget._selection.brush.map(function(d){
                    return d.feature.properties.KEY;
                });
                data = data.filter(function(d){
                    return ind.indexOf(+d.cat) != -1;
                });
            }

            
            var norm_const={};
            layer.eachLayer(function(polygon){
                if(widget.norm_const in polygon.feature.properties){
                    norm_const[polygon.feature.properties.KEY]=
                        polygon.feature.properties[widget.norm_const];
                }
            });

            data = data.map(function(d){
                if(widget._opts.thres){
                    if (d.val < widget._opts.thres){
                        d.val = 0;
                    }
                }
                d.val /= norm_const[d.cat] || 1 ;                
                return d;
            });
            data = data.filter(d=> (d.val != 0));
            
            
            //normalize the data // hack !!!
            var cmap = widget.normalizeColorMap(data, layer.colormap,
                                                widget.transfunc);

            var pcolor = {};
            data.forEach(function(d){
                pcolor[d.cat] = { id:d.id, color:cmap(d.val), val:d.val };
            });

            widget.allcats = [];
            layer.eachLayer(function(polygon){
                widget.allcats.push(polygon);

                var key=polygon.feature.properties.KEY;
                var d = {color:cmap(0), val: 0}; 
                if(key in pcolor){
                    d = pcolor[key];
                }

                /*
                  var v = d3.format(',.2f')(pcolor[key].val);
                  v += ' hours';

                  if (widget.norm_const == 'total_hours'){
                  v = d3.format(',.2%')(pcolor[key].val);
                  }                    
                  
                  if (widget.norm_const == 'subscription_acct'){
                  v = d3.format(',.2f')(pcolor[key].val);
                  v +=' hours';
                  
                  if(pcolor[key].val < 1){
                  v = d3.format(',.2f')(pcolor[key].val * 60);
                  v +=' minutes';
                  }
                  }                    
                  
                  if (pcolor[key].val > 1000){
                  v = d3.format('.2s')(pcolor[key].val);
                  v +=' hours';
                  }
                */

                let v = d3.format('.2s')(d.val);
                    
                if(widget.norm_const in polygon.feature.properties){
                    v = d3.format('.2%')(d.val);
                }

                let name = polygon.feature.properties.NAME
                    || polygon.feature.properties.KEY;
                polygon.bindTooltip(name+'<br />'+ v);
                polygon.feature.properties.id =
                    polygon.feature.properties.id || d.id;
                polygon.setStyle({
                    fillColor: d.color,
                    fillOpacity:0.6
                });
                
                if(widget._selection.brush){
                    if(widget._selection.brush.indexOf(polygon) == -1){
                        polygon.setStyle({
                            fillColor: 'black',
                            fillOpacity:0.6
                        });
                    }
                }
            });
        });
    }
};

export default PolygonMap;
