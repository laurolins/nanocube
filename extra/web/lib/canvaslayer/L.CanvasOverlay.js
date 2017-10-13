/*
 Generic  Canvas Overlay for leaflet, 
 Stanislav Sumbera, April , 2014

 - added userDrawFunc that is called when Canvas need to be redrawn
 - added few useful params fro userDrawFunc callback
  - fixed resize map bug
  inspired & portions taken from  :   https://github.com/Leaflet/Leaflet.heat
  

*/


L.CanvasOverlay = L.Class.extend({

    initialize: function (userDrawFunc, options) {
        this._userDrawFunc = userDrawFunc;
        L.setOptions(this, options);
    },

    drawing: function (userDrawFunc) {
        this._userDrawFunc = userDrawFunc;
        return this;
    },

    params:function(options){
        L.setOptions(this, options);
        return this;
    },
    
    canvas: function () {
        return this._canvas;
    },

    redraw: function () {
        if (!this._frame) {
            this._frame = L.Util.requestAnimFrame(this._redraw, this);
        }
        return this;
    },

    
  
    onAdd: function (map) {
        this._map = map;
        this._canvas = L.DomUtil.create('Canvas', 'leaflet-heatmap-layer');

        var size = this._map.getSize();
        this._canvas.width = size.x;
        this._canvas.height = size.y;

        var animated = this._map.options.zoomAnimation && L.Browser.any3d;
        L.DomUtil.addClass(this._canvas, 'leaflet-zoom-' + (animated ? 'animated' : 'hide'));


        map._panes.overlayPane.appendChild(this._canvas);

        map.on('moveend', this._reset, this);
        //map.on('move', this._move, this);
        map.on('resize',  this._resize, this);

        if (map.options.zoomAnimation && L.Browser.any3d) {
            map.on('zoomanim', this._animateZoom, this);
        }

        this._reset();
    },

    onRemove: function (map) {
        map.getPanes().overlayPane.removeChild(this._canvas);
 
        map.off('moveend', this._reset, this);
        map.off('resize', this._resize, this);

        if (map.options.zoomAnimation) {
            map.off('zoomanim', this._animateZoom, this);
        }
        this_canvas = null;

    },

    addTo: function (map) {
        map.addLayer(this);
        return this;
    },

    _resize: function (resizeEvent) {
        if(resizeEvent.newSize){
            this._canvas.width  = resizeEvent.newSize.x;
            this._canvas.height = resizeEvent.newSize.y;
        }
    },
    _reset: function () {
        this._redraw();
        var topLeft = this._map.containerPointToLayerPoint([0, 0]);
        L.DomUtil.setPosition(this._canvas, topLeft);
        //console.log("canvas pos:",L.DomUtil.getPosition(this._canvas));
    },
    
    _move: function(){
        //var topLeft = this._map.containerPointToLayerPoint([0, 0]);
        //L.DomUtil.setPosition(this._canvas, topLeft);
    },
    
    _redraw: function () {
        var size     = this._map.getSize();
        var bounds   = this._map.getBounds();
        var zoomScale = (size.x * 180) / (20037508.34  * (bounds.getEast() - bounds.getWest())); // resolution = 1/zoomScale
        var zoom = this._map.getZoom();
     
        // console.time('process');
        //var p = this._map.latLngToContainerPoint(new L.LatLng(40, -74));
        //var c = this._canvas;
        //var ctx = c.getContext("2d");
        //ctx.clearRect(0,0,c.width,c.height);
        //ctx.fillStyle = "#FF0000";
        //ctx.fillRect(p.x,p.y,150,75);

        
        if (this._userDrawFunc) {
            this._userDrawFunc(this,{
                canvas   :this._canvas,
                bounds   : bounds,
                size     : size,
                zoomScale: zoomScale,
                zoom : zoom,
                options: this.options
            });
        }
        
        // console.timeEnd('process');
        
        this._frame = null;
    },

    _animateZoom: function (e) {
        var scale = this._map.getZoomScale(e.zoom),
            offset = this._map._getCenterOffset(e.center)._multiplyBy(-scale).subtract(this._map._getMapPanePos());

        this._canvas.style[L.DomUtil.TRANSFORM] = L.DomUtil.getTranslateString(offset) + ' scale(' + scale + ')';

    }
});

L.canvasOverlay = function (userDrawFunc, options) {
    return new L.CanvasOverlay(userDrawFunc, options);
};
