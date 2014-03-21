L.NanocubeLayer = L.TileLayer.Canvas.extend({
    initialize: function(options){
        L.TileLayer.Canvas.prototype.initialize.call(this, options);
        this.model = options.model;
        this.variable = options.variable;
        
        this.colormap = d3.scale.linear()
            .domain(options.colormap.domain)
            .range(options.colormap.colors);

        this.coarselevels = 0;
        this.smooth = true;
    }
});

L.NanocubeLayer.prototype.redraw = function(){
    //this.reallydraw = true;
    L.TileLayer.Canvas.prototype.redraw.call(this);
};

L.NanocubeLayer.prototype.drawTile = function(canvas, tilePoint, zoom){    
    var drill = Math.min(this.variable.maxlevel-zoom,8) - this.coarselevels ;
    drill = Math.max(0,drill);
    drill = Math.min(8,drill);

    var size = Math.pow(2,drill);
    var ntiles = Math.pow(2,zoom);

    //fix negative tiles
    while(tilePoint.x < 0){
        tilePoint.x += ntiles;
    }

    while(tilePoint.y < 0){
        tilePoint.y += ntiles;
    }
    
    //fix overflow tiles
    while(tilePoint.x > ntiles){
        tilePoint.x -= ntiles;
    }

    while(tilePoint.y > ntiles){
        tilePoint.y -= ntiles;
    }

    //flip y for nanocubes
    var ty = (ntiles-1)-tilePoint.y;

    //query
    var tile = new Tile(tilePoint.x,ty,zoom);
    var that = this;
    this.model.tileQuery(this.variable, tile, drill, function(data){
        var result = that.processData(data);
        if(result == null){
            return;
        }

        that.max = Math.max(that.max, result.max);
        that.min = Math.min(that.min, result.min);
        
        that.renderTile(canvas,size,result.data);
    });
    this.tileDrawn(canvas);
};

L.NanocubeLayer.prototype.renderTile = function(canvas, size, data){
    var ctx = canvas.getContext('2d');
    var imgData=ctx.createImageData(size,size);
    var pixels = imgData.data; 
    var length = pixels.length;
    
    //set color
    var that = this;

    if (! this.smooth){ //blocky rendering
        ctx.imageSmoothingEnabled = false;
        ctx.webkitImageSmoothingEnabled = false;
        ctx.mozImageSmoothingEnabled = false;
    }

    data.forEach(function(d){ 
        if(d.v < 1e-6){ 
            return;
        }

        var color = d3.rgb(that.colormap(d.v/that.max));
        //var color = {r:255,g:0,b:0};
        var idx = (imgData.height-1-d.y)*imgData.width + d.x;
        pixels[idx*4]=color.r;
        pixels[idx*4+1]=color.g ;
        pixels[idx*4+2]=color.b;
        pixels[idx*4+3]= d.v/that.max*255*2;
    });
    
    //set image
    var sc = canvas.width*1.0/size;
    if (sc !=1){
        //create a proxy canvas
        var newCanvas = $('<canvas>')
                .attr("width", imgData.width)
                .attr("height", imgData.height)[0];
        newCanvas.getContext("2d").putImageData(imgData, 0, 0);
        
        ctx.drawImage(newCanvas,0,0,canvas.width,canvas.height);
    }
    else{
        ctx.putImageData(imgData,0,0);
    }
};


L.NanocubeLayer.prototype.processData = function(bindata){
    if(bindata == null){
        return null;
    }

    var record_size = 10;
    var view = new DataView(bindata);
    var n_records = bindata.byteLength / record_size;
    if (n_records < 1) {
        return null;
    }
    
    var x_array = new Uint8Array(n_records);
    var y_array = new Uint8Array(n_records);
    var count_array = new Float64Array(n_records);
    
    var logdata = new Array(n_records);
    var maxv = -Infinity;
    var minv = Infinity;
    
    for (var i=0; i<n_records; ++i) {
        var rx = view.getUint8( record_size*i+1 );
        var ry = view.getUint8( record_size*i   );
        var rv = view.getFloat64( record_size*i+2, true );
        if (rv < 1e-6){ //skip zeros
            continue;
        }

        rv = Math.log(rv);
        logdata[i] = {x:rx, y:ry, v: rv};
        maxv = Math.max(maxv,rv);
        minv = Math.min(minv,rv);
    }
    
    return {min:minv,max:maxv,data:logdata};
};


L.NanocubeLayer.prototype._addTilesFromCenterOut = function (bounds){
    this.max = -Infinity;
    this.min = Infinity;
    L.TileLayer.Canvas.prototype._addTilesFromCenterOut.call(this, bounds);  
};
