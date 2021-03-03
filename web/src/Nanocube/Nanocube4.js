/*global  fetch*/

import jquery from 'jquery';
let $ = window.$ = jquery;

import {sprintf} from 'sprintf-js';

let cache = {};
//Nanocube
var Nanocube = function(opts) {
    this.schema = null ;
    this.dimensions = null ;
};

Nanocube.prototype = {
    init:function(url){
        let nc = this;
        return new Promise((resolve,reject)=>{
            nc.setUrl(url).done((res)=>resolve(res));
        });
    },
    setUrl: function(url){
	var dfd  = new $.Deferred();
	this.url = url;
	var schema_q = this.url + '/schema()';

        let nc = this;
        fetch(schema_q).then(response=>{
            response.json().then((json)=>{
                nc.setSchema(json);
                nc.setTimeInfo().done(()=> dfd.resolve(nc));
            });
        }).catch(error=>console.log('Failed to get Schema from ', url)); 
	return dfd.promise();
    },
    
    query: function() {
	return new Query(this);
    },

    setSchema:function(json) {
	var dimensions = {};
	var nc = json[0];
	nc.index_dimensions.forEach(d => {
	    //type
	    var hint = d.hint.split('|');
	    var t = hint[0];

	    var s = d.num_levels;
	    if(t != 'spatial'){
		//compute the size of the tree in bytes
		s = Math.pow(2, (d.bits_per_level * d.num_levels));
	    }

	    //spatial
	    var info =  {
		vartype: t,
		varsize: s,
                num_levels:  d.num_levels,
                bits_per_level: d.bits_per_level
	    };

	    //categorical
	    if(t == 'categorical'){
		info.valnames = {} ;
		Object.keys(d.aliases).forEach((k)=>{
		    var newk = k.split(':').join(',');
		    info.valnames[d.aliases[k]] = newk;
		});
	    }

	    //temporal
	    if(t =='temporal'){
		var timestr = hint[1];
		var timeinfo = timestr.split('_');
		info.date_offset = new Date(timeinfo[0]);
		info.bin_sec = parseInt(timeinfo[1]);
	    }

	    dimensions[d.name] = info;
	});
	this.name = nc.name;
	this.dimensions = dimensions;
    },

    setTimeInfo: function() {
	var dfd = new $.Deferred();
        
	var dim = this.dimensions;

	var tvar = Object.keys(dim).filter(function(k){
	    return dim[k].vartype === 'temporal';
	});

        if(tvar.length < 1){ // no time variable
            dfd.resolve(); // dummy promise
            return dfd.promise();
        }

        var tvarname = tvar[0];
	tvar = dim[tvar[0]];

	var twidth = tvar.varsize;
	var maxtime = twidth-1;


        this.timeinfo={};
        var timeinfo = this.timeinfo;
	this.getTimeBounds(tvarname,0,maxtime).done(function(t){
	    tvar.start = t.mintime;
	    tvar.end = t.maxtime;
	    tvar.nbins = (t.maxtime-t.mintime+1);

            //FIX!!!!! support single time dim only for now
            timeinfo.start=tvar.start;
            timeinfo.end=tvar.end;
            timeinfo.bin_sec=tvar.bin_sec;
            timeinfo.date_offset=tvar.date_offset;

            dfd.resolve();
	    return;
	});
	return dfd.promise();
    },

    getTimeBounds: function(tvarname,mintime,maxtime){
	var dfd = new $.Deferred();
	var minp = this.getMinTime(tvarname,mintime,maxtime);
	var maxp = this.getMaxTime(tvarname,mintime,maxtime);
	$.when(minp,maxp).done(function(mintime,maxtime){
	    dfd.resolve({mintime:mintime,maxtime:maxtime+1});
	});
	return dfd.promise();
    },

    getMinTime: function(tvarname,mintime,maxtime){
	var q = this.query();

	var dfd = new $.Deferred();
	//base case
	if((maxtime - mintime) < 2){
	    return dfd.resolve(Math.max(0,mintime-1));
	}

	var nc = this;
	var interval = Math.ceil((maxtime-mintime)/100000);
	q.queryTime(tvarname,mintime,interval,100000).done(function(res){
	    var timearray = res.timearray;
	    var timeconst = res.timeconst;
	    var minp = timearray.reduce(function(p,c){
		if (p.time < c.time){
		    return p;
		}
		else{
		    return c;
		}
	    });

	    var mint = minp.time *timeconst.bucketsize;
	    var end = (minp.time+1)*timeconst.bucketsize-1;
	    mint += timeconst.start;
	    end += timeconst.start;
	    nc.getMinTime(tvarname,mint,end).done(function(m){
		return dfd.resolve(m);
	    });
	});
	return dfd.promise();
    },

    getMaxTime: function(tvarname,mintime,maxtime){
	var q = this.query();

	var dfd = new $.Deferred();
	//base case
	if((maxtime - mintime) < 2){
	    return dfd.resolve(maxtime+1);
	}

	var nc = this;
	var interval = Math.ceil((maxtime-mintime)/100000);
	q.queryTime(tvarname,mintime,interval,100000).done(function(res){
	    var timearray = res.timearray;
	    var timeconst = res.timeconst;
	    var maxp = timearray.reduce(function(p,c){
		if (p.time > c.time){
		    return p;
		}
		else{
		    return c;
		}
	    });

	    var maxt = maxp.time * timeconst.bucketsize;
	    var end = (maxp.time +1) * timeconst.bucketsize-1;
	    maxt += timeconst.start;
	    end += timeconst.start;
	    nc.getMaxTime(tvarname,maxt,end).done(function(m){
		return dfd.resolve(m);
	    });
	});
	return dfd.promise();
    },

    getTbinInfo: function() {
	if (this.timeinfo){
	    return this.timeinfo;
	}

	var tbininfo = this.schema.metadata.filter(function(f) {
            return ( f.key === 'tbin') ;
        });

        var s = tbininfo[0].value.split('_');
        var offset = new Date(s[0]+'T'+s[1]+'Z');

        var res;
        var sec = 0;
        res = s[2].match(/([0-9]+)m/);
        if (res) {
            sec += +res[1]*60;
        }
        res = s[2].match(/([0-9]+)s/);
        if (res) {
            sec = +res[1];
        }

        res = s[2].match(/([0-9]+)h/);
        if (res) {
            sec = +res[1]*60*60;
        }

        res = s[2].match(/([0-9]+)[D,d]/);
        if (res) {
            sec = +res[1]*60*60*24;
        }
        return {
            date_offset: offset,
            bin_sec: sec
        };
    },

    timeToBin: function(t){
	//translate time to bin
	var timeinfo = this.timeinfo;
	var sec = (t - timeinfo.date_offset) / 1000.0;
	var bin = sec / timeinfo.bin_sec;
	bin = Math.max(bin,timeinfo.start-1);
	bin = Math.min(bin,timeinfo.end+1);
	return bin;

    },

    bucketToTime: function(t, start, bucketsize){
	start = start || 0;
	bucketsize = bucketsize || 1;
	var timeinfo = this.timeinfo;

	//Translate timebins to real dates
	var base= new Date(timeinfo.date_offset.getTime());

	//add time offset from query
	base.setSeconds(start * timeinfo.bin_sec);

	//make date and count for each record
	var offset = timeinfo.bin_sec * bucketsize * t;
	var time= new Date(base.getTime());
	time.setSeconds(offset);
	return time;
    }
};



//Query
var Query = function(nc){
    this.nanocube = nc;
    this.dimension = null ;
    this.drilldown_flag = false;
    this.query_elements = {};

    //constrains
    this.catconst = {};
    this.idconst = {};
    this.spatialconst = {};
    this.temporalconst = {};
};

Query.prototype = {
    //Functions for setting Constraints
    setConstraint: function(varname,c){
	if(!(varname in this.nanocube.dimensions)){
	    return this;
	}

	switch(this.nanocube.dimensions[varname].vartype){
	case 'spatial':
	    return this.setSpatialConst(varname, c);
	case 'categorical':
	    return this.setCatConst(varname, c);
	case 'temporal':
	    return this.setTimeConst(varname, c);
	case 'id':
	    return this.setIdConst(varname, c);
	default:
	    return this;
	}
    },

    setSpatialConst: function(varname, sel) {
	var tiles = sel.coord.map(function(c){

	});

	var coordstr = sel.coord.map(function(c){
	    c[0] = Math.max(-85,c[0]);
	    c[0] = Math.min(85,c[0]);
	    c[1] = Math.max(-180,c[1]);
	    c[1] = Math.min(180,c[1]);
	    return c[0].toFixed(4) +","+ c[1].toFixed(4);
	});
	coordstr = coordstr.join(',');
        coordstr = sprintf('poly(\'%s\')',coordstr);

	var zoom = Math.max(10,sel.zoom); //zoom level needs to be > 10
        var constraint = sprintf('b(\'%s\',region(%d,%s))',
                                 varname,zoom,coordstr);        
	this.query_elements[varname] = constraint;

	//record constraint
	var constlist = this.spatialconst[varname]  || [];
	constlist.push(tiles);
	this.spatialconst[varname]=constlist;
	return this;
    },

    setTimeConst: function(varname, timeconst) {
	var start = this.nanocube.timeToBin(timeconst.start);
	var end = this.nanocube.timeToBin(timeconst.end);


	start = Math.floor(start);
	end = Math.ceil(end);
	if(end < 0){
	    end=1;
	    start=2;
	}

	start = Math.max(start,0);
	//var constraint = 'r(\"' + varname + '\",interval(' +
	//	start + ',' + end + '))';
        var constraint =sprintf('b(\'%s\',interval(%d,%d))',
                                varname,start,end);


        this.query_elements[varname] = constraint;

	//record timeconst
	this.temporalconst[varname]={start:start, end:end, binsize: 1};
	return this;
    },

    setCatConst: function(varname, catvalues) {
	var q = this;
	var valnames = q.nanocube.dimensions[varname].valnames;

	var values = catvalues.map(function(d){
	    return {cat: d.cat, id: valnames[d.cat] };
	});

	if (values.length > 0){
            var constraint = sprintf('b(\'%s\',pathagg(%s))',
                                     varname,
                                     values.map(d=>{return 'p('+d.id+')';})
                                     .join(','));
	    this.query_elements[varname] = constraint;
	}

	//record catconst
	this.catconst[varname]= catvalues;
	return this;
    },


    setIdConst: function(varname, idvalues) {
	//console.log(idvalues);
	var values = idvalues.map(function(d){ return d.id; });

	if (values.length > 0){
	    var constraint = 'ids('+values.join(',') +')';
	    this.query_elements[varname] = constraint;
	}

	//record catconst
	this.idconst[varname]= idvalues;

	return this;
    },

    _reshape: function(res){
        var index_values = res.index_columns[0].values.slice(0);
        var idx = [];
        while(index_values.length > 0){
            idx.push(index_values
                     .splice(0,res.index_columns[0].values_per_row));
        }
        return {index:idx,values:res.measure_columns[0].values};
    },

    queryTime: function(varname, base, bucketsize, count) {
	var constraint = sprintf('b(\'%s\',intseq(%d,%d,%d))',
                                 varname,base,bucketsize,count);	
	//record timeconst
	this.timeconst={
            start:base,
            end:base+bucketsize*count-1,
            bucketsize:bucketsize
        };

	this.timebucketsize = bucketsize;
	this.query_elements[varname] = constraint;

	var dfd = new $.Deferred();

	if((base+count) < 0){
	    dfd.resolve({timeconst: this.timeconst,
			 timearray: []});
	    return dfd.promise();
	}
	base = Math.max(0,base);


        var q = this;
	this._run_query(this).done(function(data){
            var res = data[0];
            if (res.numrows==0){
                return dfd.resolve({timeconst: q.timeconst,
			            timearray: []});
            }
            var r = q._reshape(res);
            
            var tarray =r.index.map((d,i)=>{
                return {time:+d[0], val:r.values[i]};
            });
            
            return dfd.resolve({timeconst: q.timeconst,
			        timearray: tarray});
	});
	return dfd.promise();
    },

    queryTile:function(varname,t,drill) {
	var z = t.z;
	var h =  1 << z;
	var th =  1 << drill;
	var x = Math.min(Math.max(0,t.x),h);
        var y = Math.min(Math.max(0,t.y),h);
	// var y = Math.min(Math.max(0,h-1-t.y),h);  //Flip Y

        //var tile2d = "tile2d(" + x + "," + y + "," + z + ")";
	//var constraint = "a(\"" + varname + "\",dive(" + tile2d +
	//	"," + drill + "),\"img\")";

        var constraint = sprintf('b(\'%s\',dive(img2d(%d,%d,%d),%d),\'img8\')',
                                 varname,z,x,y,drill);
        

	this.query_elements[varname] = constraint;
	this.tile = {x:x,y:y,z:z};
	this.drill = drill;

	var dfd = new $.Deferred();

        var q = this;
	this._run_query(this).done(function(data){
            var res = data[0];
            if (res.numrows==0){
                return dfd.resolve([]);
            }
            var r = q._reshape(res);

	    var z = this.tile.z+this.drill;
	    var query = this;
	    var offset = {x:this.tile.x*256,y:(this.tile.y)*256};

	    data = r.index.map(function(d,i){
		var x = d[0] + offset.x;
		var y = d[1] + offset.y; //still need to flip y
		return {x:x,y:y,z:z,val:r.values[i]};
	    });

	    data = data.filter (function(d){
		return d.val !== 0;
	    });
	    return dfd.resolve(data);
	});
	return dfd.promise();
    },
    _pathToXY: function(path){
	var xypath = path.map(function(d,i){
	    var nthbit = path.length-1-i;
	    var x = ((d >> 0) & 1) << nthbit; //x 0th bit
	    var y = ((d >> 1) & 1) << nthbit; //y 1st bit
	    return {x:x, y:y};
	});
	return xypath.reduce(function(p,c){
	    return {x: p.x|c.x, y:p.y|c.y};
	});
    },
    toString: function() {
	var qelem = this.query_elements;
	var dims = Object.keys(qelem);
	var vals = dims.map(function(d) {
	    return qelem[d];
	});

	var query_string = vals.join('.');
	return sprintf('%s/q(%s.%s)',this.nanocube.url, this.nanocube.name,
                       query_string);


        //this.nanocube.url + '/' + type + '.' + query_string;
    },

    _run_query: function(ctx,query_cmd){
	//query_cmd = query_cmd || 'count';

	var query_string = this.toString();

	var dfd = $.Deferred();
	if (cache[query_string]){
	    var res = $.extend(true, {}, cache[query_string]);
	    dfd.resolveWith(ctx, [res]);
	    return dfd.promise();
	}
	else{
	    console.log(query_string);
            let ctx = this;
            fetch(query_string).then(response=>{
                response.json().then((json)=>{
		    if(Object.keys(cache).length > 10){
		        var idx = Math.floor(Math.random() * (10+1)) ;
		        var k = Object.keys(cache)[idx];
		        delete cache[k];
		    }
		    cache[query_string] = $.extend(true, {}, json);
		    dfd.resolveWith(ctx, [json]);
                });
            }).catch(error=>console.log('Failed: ', query_string)); 


	    return dfd.promise();
	}
    },


    categoricalQuery: function(varname){
        var dim = this.nanocube.dimensions;
        
	var constraint = sprintf('b(\'%s\',dive(p(),%d))', varname,
                                 dim[varname].num_levels);

	this.query_elements[varname] = constraint;

	var dfd = new $.Deferred();

        var q = this;
	this.valnames = this.nanocube.dimensions[varname].valnames;
	this._run_query(this).done(function(data){
            var res = data[0];
            if (res.numrows==0){
                return dfd.resolve({type:'cat',
			            data: []});
            }
            var r = q._reshape(res);
                        
	    //set up a val to name map
	    var valToName = {};
	    for (var name in q.valnames){
		valToName[q.valnames[name]] = name;
	    }

            var catarray =r.index.map((d,i)=>{
                return {id:d.join(','),cat:valToName[d],val:r.values[i]};
            });

	    return dfd.resolve({type:'cat', data:catarray});
	});
	return dfd.promise();
    },

    //Top K query
    topKQuery: function(varname, n){
	var constraint = "k("+n+")";
	this.query_elements[varname] = constraint;

	var dfd = new $.Deferred();

	this.valnames = this.nanocube.dimensions[varname].valnames;
	this._run_query(this,'topk').done(function(data){
	    if (!data.root.val.volume_keys){
		return dfd.resolve({type:'id', data: []});
	    }

	    data = data.root.val.volume_keys;
	    var q = this;
	    var idarray = data.map(function(d){
		return {id:d.key,cat:d.word,val:d.count};
	    });

	    return dfd.resolve({type:'id', data: idarray});
	});
	return dfd.promise();
    },

    //temporal queries, return an array of {date, val}
    temporalQuery: function(varname,start,end,interval_sec){
	// console.log(start, end);
	var q = this;
	var timeinfo = q.nanocube.getTbinInfo();

	var startbin = q.nanocube.timeToBin(start);

	var bucketsize = interval_sec / timeinfo.bin_sec;
	bucketsize = Math.max(1,Math.floor(bucketsize+0.5));

	var endbin = q.nanocube.timeToBin(end);
	startbin = Math.floor(startbin);
	endbin = Math.floor(endbin);

	var count = (endbin - startbin) /bucketsize + 1 ;
	count = Math.floor(count);

	var dfd = new $.Deferred();

	if(endbin==startbin){
	    dfd.resolve(null);
	    return dfd.promise();
	}
	startbin = Math.max(startbin,0);

	q.queryTime(varname,startbin,bucketsize,count).done(function(res){
	    //make date and count for each record
	    var nbins = res.timeconst.end - res.timeconst.start;
	    nbins = nbins/res.timeconst.bucketsize+1;
	    nbins = Math.floor(nbins);
	    var datecount = new Array(nbins);
	    for(var i=0; i < nbins; i++){
		var t = q.nanocube.bucketToTime(i,res.timeconst.start,
						res.timeconst.bucketsize);
		datecount[i]= {time:t,  val:0};
	    }

	    res.timearray.forEach(function(d,i){
		datecount[d.time].val = d.val;
	    });

	    //kill zeros
	    datecount = datecount.filter(function(d){return d.val !== 0;});
	    ///////
	    dfd.resolve({type:'temporal', data:datecount,
			 timeconst:res.timeconst });
	});
	return dfd.promise();
    },

    spatialQuery: function(varname,bb,z, maptilesize){
	maptilesize = maptilesize || 256;

	var q = this;

	var tilesize_offset = Math.log(maptilesize)/Math.log(2);
	var pb = { min:{ x: long2tile(bb.min[1],z+tilesize_offset),
			 y: lat2tile(bb.min[0],z+tilesize_offset) },
		   max:{ x: long2tile(bb.max[1],z+tilesize_offset),
			 y: lat2tile(bb.max[0],z+tilesize_offset) }
		 };


	var queries = [];
	var maxlevel = this.nanocube.dimensions[varname].varsize;
	var drill = Math.max(0,Math.min(z+8,8));

	var tilesize = 1 << drill;
	var tbbox = {min:{x: Math.floor(pb.min.x / tilesize),
			  y: Math.floor(pb.min.y / tilesize)},
		     max:{x: Math.floor(pb.max.x / tilesize),
			  y: Math.floor(pb.max.y / tilesize)}};

	z = Math.max(0,Math.min(z,maxlevel-8) );

	var h = 1 << z;

	for (var i=Math.floor(tbbox.min.x);i<=Math.floor(tbbox.max.x);i++){
	    for (var j=Math.floor(tbbox.min.y);j<=Math.floor(tbbox.max.y);j++){
		if (i < 0 || j < 0 || i >=h || j>=h){
		    continue;
		}

		var clone_q = $.extend({},q);
		queries.push(clone_q.queryTile(varname,{x:i,y:j,z:z},drill));
	    }
	}

	var dfd = new $.Deferred();
	$.when.apply($, queries).done(function(){
	    var results = arguments;
	    var merged = [];
	    merged = merged.concat.apply(merged, results);
	    dfd.resolve({type: 'spatial', opts:{pb:pb}, data:merged});
	});
	return dfd.promise();
    }
};


//Utilities
//Lat Long to Tile functions from
//https://wiki.openstreetmap.org/wiki/Slippy_map_tilenames#Lon..2Flat._to_tile_numbers

function long2tile(lon,zoom) {
    return (Math.floor((lon+180)/360*Math.pow(2,zoom)));
}

function lat2tile(lat,zoom)  {
    return (Math.floor((1-Math.log(Math.tan(lat*Math.PI/180) +
				   1/Math.cos(lat*Math.PI/180))/Math.PI)/2*
		       Math.pow(2,zoom)));
}

function tile2long(x,z) {
    return (x/Math.pow(2,z)*360-180);
}

function tile2lat(y,z) {
    var n=Math.PI-2*Math.PI*y/Math.pow(2,z);
    return (180/Math.PI*Math.atan(0.5*(Math.exp(n)-Math.exp(-n))));
}

function latlong2tile(latlong,zoom) {
    return { x: long2tile(latlong[1],zoom),
	     y: lat2tile(latlong[0],zoom),
	     z: zoom};
}


export default Nanocube;
