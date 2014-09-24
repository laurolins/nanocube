/* global $,d3,L,console */


//-----------------------------------------------------------------------
// binary_xhr
//-----------------------------------------------------------------------

function binary_xhr(url, handler){
    var xhr = new window.XMLHttpRequest();
    var ready = false;
    xhr.onreadystatechange = function() {
	if (xhr.readyState === 4 && xhr.status === 200
	    && ready !== true) {
	    if (xhr.responseType === "arraybuffer") {
		handler(xhr.response, url);
	    } else if (xhr.mozResponseArrayBuffer !== null) {
		handler(xhr.mozResponseArrayBuffer, url);
	    } else if (xhr.responseText !== null) {
		var data = String(xhr.responseText);
		var ary = new Array(data.length);
		for (var i = 0; i <data.length; i++) {
		    ary[i] = data.charCodeAt(i) & 0xff;
		}
		var uint8ay = new Uint8Array(ary);
		handler(uint8ay.buffer, url);
	    }
	    ready = true;
	}
    };
    xhr.open("GET", url, true);
    xhr.responseType="arraybuffer";
    xhr.send();
}


//-----------------------------------------------------------------------
// Dimension
//-----------------------------------------------------------------------

function Dimension (obj) {
    this.name   = obj.name;
    this.type   = obj.type;
    return this;
}

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
	.filter(function (f) { return f.type.indexOf("nc_dim") == 0;})
	.map(function (f){return new Dimension({name:f.name, type:f.type});});
};



//         $.getJSON(that.getTQuery(), function(json){
//             var addr = json.root.children[0].addr;
//             addr = addr.toString();
//             var start = addr.substring(0,addr.length-8);
//             var end = addr.substring(addr.length-8,addr.length);
//             //console.log(start,end);

//             start = parseInt(start,16);
//             end = parseInt(end,16);

//             if (isNaN(start)) start=0;
//             if (isNaN(end)) end=0;

//             //console.log(start,end,end-start);


//             var tbinfo = that.getTbinInfo();

//             that.timeinfo = tbinfo;
//             that.timeinfo.start=start;
//             that.timeinfo.end=end;
//             that.timeinfo.nbins=(end-start+1);

//             opts.ready(that);
//         });
//     });

//     return this;
// }

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
    var datearr = s[0].split(/[^0-9]/);
    var timearr = s[1].split(/[^0-9]/);
    var offset=new Date (+datearr[0],+datearr[1]-1,+datearr[2],
			 +timearr[0],+timearr[1],+timearr[2]);

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

    return {date_offset:offset, bin_to_hour: day*24+hour+min/60.0+sec/3600.0};
};


Nanocube.prototype.setTimeInfo = function() {
    var dfd = $.Deferred();
    var that = this;
    var tvar  = this.schema.fields.filter(function (f) { 
	return f.type.indexOf("nc_dim_time") == 0;
    });
    var q = this.query();
    q = q.dim(tvar[0].name);
    q = q.tseries(0,1,65536);
    return q.run_query().done(function(json){
	if (!json.root.children){
	    return;
	}

	var tbounds = json.root.children.reduce(function(prev,curr){
	    return { min: Math.min(curr.path[0], prev.min),
		     max: Math.max(curr.path[0], prev.max)};
	}, {min:65536,max:0});
	
	var tbinfo = that.getTbinInfo();    
	that.timeinfo = tbinfo;
	that.timeinfo.start=tbounds.min;
	that.timeinfo.end=tbounds.max;
	that.timeinfo.nbins=(that.timeinfo.end 
			     -that.timeinfo.start+1);
	dfd.resolve();
    });
    return dfd.promise();
};

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

Query.prototype.findAndDive = function(addr, offset) {
    var varname = this.dimension.name;
    var constraint = 'r(\"'+varname+'\",dive([],1))';
    this.query_elements[this.dimension.name] = constraint;
    return this;
};

Query.prototype.rectQuery = function(topLeft,bottomRight) {
    var varname = this.dimension.name;    
    var constraint = "r(\""+varname+"\",range2d(tile2d("
	    +topLeft.x+","+topLeft.y+","+topLeft.z+"),tile2d("
	    +bottomRight.x+","+bottomRight.y+","+bottomRight.z+")))";
    
    this.query_elements[varname] = constraint;
    return this;
};


Query.prototype.tseries = function(base, bucket, count) {
    var varname = this.dimension.name;
    var constraint = 'r(\"'+varname+'\",mt_interval_sequence('+
	base+','+bucket+','+count+'))';
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
}

Query.prototype.run_tile = function(callback) {
    var query_string = this.toString('tile');
    //var query_string = [this.nanocube.url,"tile"]
    //        .concat(_.values(this.query_elements)).join("/");
    console.log(query_string);
    binary_xhr(query_string, callback);
    return this;
};


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
