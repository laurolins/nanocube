/*global $,console,module */

//-----------------------------------------------------------------------
// Tile
//-----------------------------------------------------------------------
function Tile (x, y, level) {
    this.x = x;
    this.y = y;
    this.level = level;
    return this;
}

Tile.prototype.raw = function() {
    return "qaddr(" + this.x + "," + this.y + "," + this.level + ")";
};

Tile.prototype.tile2d = function() {
    return "tile2d(" + this.x + "," + this.y + "," + this.level + ")";
};

//-----------------------------------------------------------------------
// Dimension
//-----------------------------------------------------------------------

function Dimension (obj) {
    this.name   = obj.name;
    this.type   = obj.type;
    return this;
}

//-----------------------------------------------------------------------
// Query
//-----------------------------------------------------------------------
function Query (nanocube) {
    this.nanocube       = nanocube;
    this.dimension      = null;
    this.drilldown_flag = false;
    this.query_elements = {};
};

Query.prototype.reset = function() {
    this.query_elements = {};
    return this;
};

Query.prototype.dim = function(dim_name) {
    this.dimension = this.nanocube.getDimension(dim_name);
    return this;
};

Query.prototype.drilldown = function() {
    this.drilldown_flag = true;
    return this;
};

Query.prototype.rollup = function() {
    this.drilldown_flag = false;
    return this;
};

//Categorial Query
Query.prototype.findAndDive = function(selection) {
    var varname = this.dimension.name;
    var constraint = "";
    if(selection == null && this.drilldown_flag){
	constraint = 'a(\"'+varname+'\",dive([],1))';
    }
    else{
	constraint = 'r(\"'+varname+'\",set('+selection.join(',') +'))';
    }

    this.query_elements[this.dimension.name] = constraint;
    return this;
};

//Rectangle Query
Query.prototype.rectQuery = function(topLeft,bottomRight) {
    var varname = this.dimension.name;
    var constraint = "r(\""+varname+"\",range2d("
	+topLeft.tile2d()+","+bottomRight.tile2d()
	+"))";

    this.query_elements[varname] = constraint;
    return this;
};

//Polygon Query
Query.prototype.polygonQuery = function(pts) {
    var varname = this.dimension.name;
    var w = Math.pow(2,pts[0].level) ;
    var constraint = "r(\""+varname+"\",mercator_mask(";

    var coordstr = '\"'+ pts.map(function(d){
	return (d.x/w*2-1) +','+ (d.y/w*2-1);
    }).join(',')+'\"';

    constraint =  constraint+coordstr+","+ pts[0].level +"))";

    this.query_elements[varname] = constraint;
    return this;
};

//Tile Query
Query.prototype.findTile = function(tile,drill) {
    var varname = this.dimension.name;
    var constraint = "a(\""+varname+"\",dive("+tile.tile2d()
	+","+drill+"),\"img\")";
    this.query_elements[this.dimension.name] = constraint;
    return this;
};

//Time Series Query
Query.prototype.tseries = function(base, bucket, count) {
    var varname = this.dimension.name;
    var constraint = "";
    if (count > 1){
	constraint = 'r(\"'+varname+'\",mt_interval_sequence('
	    +base+','+bucket+','+count+'))';
	this.timebucketsize = bucket;
    }
    else{
	constraint = 'r(\"'+varname+'\",interval('
	    +base+','+(base+bucket)+'))';
    }

    this.query_elements[this.dimension.name] = constraint;
    return this;
};

Query.prototype.toString = function(type) {
    var qelem = this.query_elements;
    var dims = Object.keys(qelem);
    var vals = dims.map(function(d){return qelem[d];});

    var query_string = vals.join('.');
    return this.nanocube.url+'/'+type+'.'+ query_string;
};

Query.prototype.run_query = function(callback) {
    var query_string = this.toString('count');
    console.log(query_string);
    return $.getJSON(query_string);
};


//-----------------------------------------------------------------------
// Nanocube
//-----------------------------------------------------------------------

function Nanocube(opts) {
    this.url        = opts.url;
    this.schema     = null;
    this.dimensions = null;

    var that = this;

    $.getJSON(that.getSchemaQuery())
	.done(function(json){
	    that.setSchema(json);
	    that.setTimeInfo().done(function(){
		opts.ready(that);
	    });
	})
	.fail(function(){
	    console.log('Failed to get Schema from ', that.url);
	});
}

Nanocube.prototype.setSchema = function(json){
    this.schema = json;
    this.dimensions = this.schema.fields
	.filter(function (f) {return f.type.indexOf("nc_dim") == 0;})
	.map(function (f){return new Dimension({name:f.name, type:f.type});});
};

Nanocube.prototype.getDimension = function(dim_name) {
    var lst = this.dimensions.filter(function (f) {
	return f.name==dim_name; } );
    return lst[0];
};

Nanocube.prototype.getSchemaQuery = function() {
    return this.url + "/schema";
};

Nanocube.prototype.getTQuery = function() {
    return this.url + "/tquery";
};

Nanocube.prototype.query = function() {
    return new Query(this);
};

Nanocube.prototype.getFieldNames = function() {
    return this.schema.fields
	.filter(function (f) { return f.type.indexOf("nc_dim") == 0;})
	.map(function (f) { return f.name; });
};

Nanocube.prototype.getTbinInfo = function() {
    var tbininfo=this.schema.metadata.filter(function (f) {
	return (f.key == 'tbin');
    });

    var s = tbininfo[0].value.split('_');
    var offset=new Date(s[0]+'T'+s[1]+'Z'); //construct UTC iso time string
    
    var day=0, hour=0, min=0, sec=0, res=null;
    res = s[2].match(/([0-9]+)m/);
    if (res){
	min = +res[1];
    }
    res = s[2].match(/([0-9]+)s/);
    if (res){
	sec = +res[1];
    }

    res = s[2].match(/([0-9]+)h/);
    if (res){
	hour = +res[1];
    }

    res = s[2].match(/([0-9]+)d/);
    if (res){
	day = +res[1];
    }

    return {date_offset:offset, 
	    bin_to_hour: day*24+hour+min/60.0+sec/3600.0};
};

Nanocube.prototype.getTimeBound = function(timevar,start,end,func,lastdfd){
    var dfd = lastdfd || $.Deferred();
    
    var q = this.query();
    var len = (end-start);
    var elem = 8192;
    var binsize = Math.floor(len/elem+1);

    q = q.dim(timevar);
    q = q.tseries(start,binsize,elem);
    var that = this;
    q.run_query().done(function(json){
	if (!json.root.children){
	    throw new Error("Invalid Range");
	}
	
	var bound = json.root.children.reduce(function(prev,curr){
	    return func(curr.path[0], prev);
	},json.root.children[0].path[0]);
	
	if (binsize > 1){
	    var newstart = start + bound * binsize;
	    var newend = newstart + binsize;
	    return that.getTimeBound(timevar,newstart,newend,func,dfd);
	}
	else{
	    return dfd.resolve(start+bound);
	}
    });
    return dfd.promise();
};

Nanocube.prototype.setTimeInfo = function() {
    var dfd = new $.Deferred();
    var that = this;
    var tvar  = this.schema.fields.filter(function (f) {
	return f.type.indexOf("nc_dim_time") == 0;
    });

    var minb = this.getTimeBound(tvar[0].name,0,Math.pow(2,32),Math.min);
    var maxb = this.getTimeBound(tvar[0].name,0,Math.pow(2,32),Math.max);
    $.when(minb,maxb).done(function(minbound,maxbound){
	that.timeinfo = that.getTbinInfo();

	that.timeinfo.start = minbound;
	that.timeinfo.end = maxbound;	
	that.timeinfo.nbins=(that.timeinfo.end-that.timeinfo.start+1);
	console.log(that.timeinfo);
	dfd.resolve();
    });    
    return dfd.promise();
};

//module.exports = {
//    Nanocube: Nanocube,
//    Tile: Tile
//};
