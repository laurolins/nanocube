L.NanocubeLayer = L.TileLayer.Canvas.extend({
    initialize: function(options){
        L.TileLayer.Canvas.prototype.initialize.call(this, options);
        this.model = options.model;
        this.variable = options.variable;
        this.colormap = d3.scale.linear()
            .domain(options.colormap.domain)
            .range(options.colormap.colors);

        this.coarselevels = 1;
        this.smooth = false;
        
        this.show_count = false;
        this.log = true;
    }
});

L.NanocubeLayer.prototype.toggleLog = function(){
    this.log = !this.log;
    this.redraw();
};

L.NanocubeLayer.prototype.toggleShowCount = function(){
    this.show_count = !this.show_count;
    this.redraw();
};


L.NanocubeLayer.prototype.redraw = function(){
    if (this._map) {
	this._reset({hard: false});  //no hard resetting 
	this._update();
    }
    for (var i in this._tiles) {
	this._redrawTile(this._tiles[i]);
    }
    return this;
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

    var tile = new Tile(tilePoint.x,ty,zoom);
    tile.canvas = canvas;
    tile.drill = drill;
    tile.tilePoint = tilePoint;
    this.tilelist.push(tile);
};

L.NanocubeLayer.prototype.renderTile = function(canvas, size,tilePoint,zoom,
                                                minv,maxv,data){
    var ctx = canvas.getContext('2d');
    
    if (data == null){
        var imgBlankData = ctx.createImageData(canvas.width,canvas.height);
        ctx.putImageData(imgBlankData,0,0);

        if (this.show_count){//draw grid box
            this.drawGridCount(ctx,tilePoint,zoom,data);
        }
        return;
    }


    var imgData=ctx.createImageData(size,size);
    var pixels = imgData.data; 
    var length = pixels.length;
    
    if (! this.smooth){ //blocky rendering
        ctx.imageSmoothingEnabled = false;
        ctx.webkitImageSmoothingEnabled = false;
        ctx.mozImageSmoothingEnabled = false;
    }

    //set color
    var that = this;
    if (this.log){
        minv = Math.log(minv+1);
        maxv = Math.log(maxv);
    }

    data.forEach(function(d){ 
        if(d.v < 1e-6){ 
            return;
        }

        var v = d.v;
        if (that.log){
            v = Math.log(v+1);
        }

        v = (v-minv)/maxv;
        var color = d3.rgb(that.colormap(v));
        var idx = (imgData.height-1-d.y)*imgData.width + d.x;
        pixels[idx*4]=color.r;
        pixels[idx*4+1]=color.g ;
        pixels[idx*4+2]=color.b;
        pixels[idx*4+3]= v*255*2;
    });
    
    //set image
    var sc = canvas.width*1.0/size;

    //clear the canvas
    imgBlankData = ctx.createImageData(canvas.width,canvas.height);
    ctx.putImageData(imgBlankData,0,0);

    //scale
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

    if (this.show_count){//draw grid box
        this.drawGridCount(ctx,tilePoint,zoom,data);
    }
};

L.NanocubeLayer.prototype.drawGridCount = function(ctx,tilePoint,zoom,data){
    ctx.lineWidth="0.5";
    ctx.strokeStyle="white";
    ctx.rect(0,0,ctx.canvas.width,ctx.canvas.height);
    ctx.stroke();

    var totalstr =  "("+tilePoint.x + "," + tilePoint.y +","+zoom+")   ";
    if(data != null){
        //Total count
        var total = data.reduce(function(prev,curr){return prev+curr.v;},0);
        totalstr +=  total.toString().replace(/\B(?=(\d{3})+(?!\d))/g, ",");
    }
    ctx.font="10pt sans-serif";
    ctx.fillStyle="white";
    ctx.fillText(totalstr,10,20);
    
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
    
    var data = [];
    for (var i=0; i<n_records; ++i) {
        var rx = view.getUint8( record_size*i+1 );
        var ry = view.getUint8( record_size*i   );
        var rv = view.getFloat64( record_size*i+2, true );
        if (rv < 1e-6){ //skip zeros
            continue;
        }
        data.push({x:rx, y:ry, v: rv});
    }

    return data;
};


L.NanocubeLayer.prototype._addTilesFromCenterOut = function(bounds){
    this.tilelist = []; //empty the list
    L.TileLayer.Canvas.prototype._addTilesFromCenterOut.call(this, bounds);  

    if (this.tilelist.length < 1){
        return;
    }
    var tilelist=this.tilelist.slice(0); //copy the tilelit

    var that = this;    
    var promises = tilelist.map(function(t){ 
        return that.queryTile(t);
    });

    //Process the tile
    $.when.apply($,promises).then(function(){
        //After querying
        var value_arr = tilelist.map(function(t){
            if (t.data==null){
                return [];
            }
            else{
                return t.data.map(function(d){ return d.v;});
            }
        });

        //flatten the arrays
        var min_arr = value_arr.reduce(function(a, b){
            return a.concat(Math.min.apply(null,b));
        });
        var max_arr = value_arr.reduce(function(a, b){
            return a.concat(Math.max.apply(null,b));
        });

        //find max and min
        var minv = Math.min.apply(null,min_arr);
        var maxv = Math.max.apply(null,max_arr);
        
        //render the tiles
        tilelist.map(function(t){
            var size = Math.pow(2,t.drill);
            that.renderTile(t.canvas, size, t.tilePoint,
                            t.level, minv,maxv,t.data);
        });
    });
};

L.NanocubeLayer.prototype.queryTile = function(tile){
    var that = this;
    var dfd = $.Deferred();
    this.model.tileQuery(that.variable,tile,tile.drill, function(data){
        tile.data = that.processData(data);
        dfd.resolve();
    });
    return dfd.promise();
};
