var MAXCACHE=150;
var hourbSizes = [1,12,24,7*24];
var colors = colorbrewer.Set1[9];

function Model(opt){
    this.nanocube = opt.nanocube;
    this.options = opt;

    this.query_cache = {};
    this.selcolors = {};

    //initialize the variables
    this.initVars();

    this.cache_off = false;

};

//Init Variables according to the schema
Model.prototype.initVars = function(){
    var variables = this.nanocube.schema.fields.filter(function(f){
	return f.type.match(/^nc_dim/);
    });
    this.spatial_vars = {};
    this.cat_vars = {};
    this.temporal_vars = {};

    //loop through the schema and create the variables
    var that = this;
    variables.forEach(function(v){
	var vref={};
	var t = v.type.match(/nc_dim_(.+)_(.+)/);

	switch(t[1]){
	case 'quadtree':  //Create a spatial var and map
	    if ($('#'+v.name).length < 1){
		return;
	    }

	    var cmap = that.options.config.div[v.name].colormap ||
		    colorbrewer.YlOrRd[9].reverse();

	    var cdomain=cmap.map(function(d,i){return i*1.0/(cmap.length-1);});
	    var cm={colors:cmap,domain:cdomain};

	    vref  = new SpatialVar(v.name);

	    vref.maxlevel = +t[2];
	    if (that.options.heatmapmaxlevel != undefined){
		vref.maxlevel = Math.min(that.options.heatmapmaxlevel,
					 vref.maxlevel);
	    }

	    //Create the map and heatmap
	    var ret = that.createMap(vref,cm);
	    vref.map=ret.map;
	    vref.heatmap=ret.heatmap;
	    if(that.options.smooth != undefined){
		vref.heatmap.smooth = that.options.smooth;
	    }

	    var mid = Math.floor(cm.colors.length/2.0);
	    that.selcolors[v.name] = cm.colors[mid];
	    that.spatial_vars[v.name] = vref;
	    break;

	case 'cat': //Create a categorical var and barchart
	    if ($('#'+v.name).length < 1){
		return;
	    }

	    vref  = new CatVar(v.name,v.valnames,
			       that.options.config['div'][v.name]['displaynumcat'],
			       that.options.config['div'][v.name]['alpha_order']);

	    //init the gui component (move it elsewhere?)
	    vref.widget = new GroupedBarChart(v.name,
					      that.options.config['div'][v.name]['logaxis']);

	    //set selection and click callback
	    vref.widget.setSelection(vref.constraints[0].selection);
	    vref.widget.setClickCallback(function(d){
		if (typeof d != "undefined") {
		    vref.constraints[0].toggle(d.addr);
		    d3.event.stopPropagation();
		} else {
		    vref.alpha_order = !vref.alpha_order;
		}
		that.redraw();
	    });

	    that.cat_vars[v.name] = vref;
	    break;

	case 'time': //Create a temporal var and timeseries
	    if ($('#'+v.name).length < 1){
		return;
	    }

	    //Get the time information
	    var tinfo = that.nanocube.timeinfo;

	    vref  = new TimeVar(v.name, tinfo.date_offset,
				tinfo.start,tinfo.end,
				tinfo.bin_to_hour);

	    var nbins = tinfo.end-tinfo.start+1;

	    //init gui
	    vref.widget = new Timeseries(v.name);
	    vref.widget.brush_callback = function(start,end){
		vref.constraints[0].setSelection(start,end,vref.date_offset);
		that.redraw(vref);
	    };

	    vref.widget.update_display_callback=function(start,end){
		vref.constraints[0].setRange(start,end,vref.date_offset);
		that.redraw();
	    };

	    //set the timeseries to the finest resolution
	    while (tinfo.bin_to_hour >=  hourbSizes[0]){
		hourbSizes.shift();
	    }

	    that.setTimeBinSize(tinfo.bin_to_hour,vref);
	    that.temporal_vars[v.name] = vref;
	    break;
	default:
	    break;
	}
    });
};

//Redraw
Model.prototype.redraw = function(calling_var,sp){
    var that = this;
    var spatial = true;
    if (sp != undefined){
	spatial = sp;
    }

    //spatial
    Object.keys(that.spatial_vars).forEach(function(v){
	if(calling_var != that.spatial_vars[v] && spatial){
	    that.spatial_vars[v].update();
	    that.updateInfo();
	}
    });

    //update each spatial polygon constraints
    Object.keys(that.spatial_vars).forEach(function(v){
	var spvar = that.spatial_vars[v];
    });

    //temporal
    Object.keys(that.temporal_vars).forEach(function(v){
	var thisvref = that.temporal_vars[v];
	if(calling_var != thisvref){ that.jsonQuery(thisvref); }
    });

    //categorical
    Object.keys(that.cat_vars).forEach(function(v){
	var thisvref = that.cat_vars[v];
	if(calling_var != thisvref){ that.jsonQuery(thisvref); }
    });
};

//Tile queries for Spatial Variables
Model.prototype.tileQuery = function(vref,tile,drill,callback){
    var q = this.nanocube.query();
    var that = this;
    Object.keys(that.temporal_vars).forEach(function(v){
	q = that.temporal_vars[v].constraints[0].add(q);
    });

    Object.keys(that.cat_vars).forEach(function(v){
	q = that.cat_vars[v].constraints[0].add(q);
    });

    Object.keys(that.spatial_vars).forEach(function(v){
	var spvref = that.spatial_vars[v];
	if(vref != spvref){
	    q = spvref.view_const.add(q);
	}
    });

    //q = q.drilldown().dim(vref.dim).findAndDive(tile.raw(),drill);

    q = q.drilldown().dim(vref.dim).findTile(tile,drill);

    var qstr = q.toString('count');
    var data = this.getCache(qstr);

    if (data != null){ //cached
	callback(data);
    }
    else{
	q.run_query()
	    .done(function(data){
		callback(data);
		that.setCache(qstr,data);
	    });
    }
};

//Caching Functions
Model.prototype.setCache = function(qstr,data){

    this.query_cache[qstr] = data;
    var keys = Object.keys(this.query_cache);

    if (keys.length > MAXCACHE){
	var rand = keys[Math.floor(Math.random() * keys.length)];
	delete this.query_cache[keys[rand]];
    }
};

Model.prototype.getCache = function(qstr){

    if (!this.cache_off && (qstr in this.query_cache)){
	return this.query_cache[qstr];
    }
    else{
	return null;
    }
};


//JSON Queries for Cat and Time Variables
Model.prototype.jsonQuery = function(v){
    var queries = this.queries(v);
    var that =this;
    var keys = Object.keys(queries);

    keys.forEach(function(k){
	var q = queries[k];
	var qstr = q.toString('count');
	var json = that.getCache(qstr);
	var color = that.selcolors[k];

	if (json != null){ //cached
	    v.update(json,k,color,q);
	}
	else{
	    q.run_query().done(function(json){
		v.update(json,k,color,q);
		that.setCache(qstr,json);
	    });
	}
    });
};

//Remove unused constraints from variables
Model.prototype.removeObsolete= function(k){
    var that = this;
    Object.keys(that.temporal_vars).forEach(function(v){
	that.temporal_vars[v].removeObsolete(k);
    });

    Object.keys(that.cat_vars).forEach(function(v){
	that.cat_vars[v].removeObsolete(k);
    });
};


//Setup maps
Model.prototype.createMap = function(spvar,cm){
    var map=L.map(spvar.dim,{
	maxZoom: Math.min(18,spvar.maxlevel+1)
    });

    var maptile = L.tileLayer(this.options.tilesurl,{
	noWrap:true,
	opacity:0.4 });

    var heatmap = new L.NanocubeLayer({
	opacity: 0.6,
	model: this,
	variable: spvar,
	noWrap:true,
	colormap:cm,
	log: this.options.logcolormap
    });

    var that = this;
    map.on('moveend', function(e){
	var b = map.getBounds();
	var level = map.getZoom();
	var tilelist = boundsToTileList(b,Math.min(level+8, spvar.maxlevel));

	spvar.setCurrentView(tilelist);
	that.redraw(spvar);
	that.updateInfo();
    });

    maptile.addTo(map);
    heatmap.addTo(map);

    //register panel functions
    this.panelFuncs(maptile,heatmap);


    //register keyboard shortcuts
    this.keyboardShortcuts(spvar,map,heatmap);


    //Drawing Rect and Polygons
    this.addDraw(map,spvar,false);

    return {map:map, heatmap:heatmap};
};

//Colors
Model.prototype.nextColor=function(){
    var c =colors.shift();
    colors.push(c);
    return c;
};

//Add Rectangles and polygons controls
Model.prototype.addDraw = function(map,spvar){
    //Leaflet draw interactions
    map.drawnItems = new L.FeatureGroup();
    map.drawnItems.addTo(map);

    map.editControl = new L.Control.Draw({
	draw: {
	    rectangle: true,
	    //polygon: false,
	    polyline: false,
	    circle: false,
	    marker: false,
	    polygon: { allowIntersection: false }
	},
	edit: {
	    featureGroup: map.drawnItems
	}
    });
    map.editControl.setDrawingOptions({
	rectangle:{shapeOptions:{color: this.nextColor(), weight: 2,
				 opacity:.9}},
	polygon:{shapeOptions:{color: this.nextColor(), weight: 2,
			       opacity:.9}}
    });

    map.editControl.addTo(map);

    //Leaflet created event
    var that = this;
    map.on('draw:created', function (e) {
	that.drawCreated(e,spvar);
	if (spvar.dim in spvar.constraints){
	    that.toggleGlobal(spvar); //auto disable global
	}
    });

    map.on('draw:deleted', function (e) {
	that.drawDeleted(e,spvar);
    });

    map.on('draw:editing', function (e) {
	that.drawEditing(e,spvar);
    });

    map.on('draw:edited', function (e) {
	that.drawEdited(e,spvar);
    });
};

//Functions for drawing / editing / deleting shapes
Model.prototype.drawCreated = function(e,spvar){
    //add the layer
    spvar.map.drawnItems.addLayer(e.layer);

    //update the contraints
    var coords = e.layer.toGeoJSON().geometry.coordinates[0];
    coords = coords.map(function(e){ return L.latLng(e[1],e[0]);});
    coords.pop();

    var tilelist = genTileList(coords,
			       Math.min(spvar.maxlevel,e.target._zoom+8));
    var color = e.layer.options.color;

    this.selcolors[e.layer._leaflet_id] = color;
    spvar.addSelection(e.layer._leaflet_id, tilelist);

    //events for popups
    var that = this;
    e.layer.on('mouseover', function(){
	//update polygon count before opening popup
	that.updatePolygonCount(e.layer, spvar);
	e.layer.openPopup();
    });

    e.layer.on('mouseout', function(){e.layer.closePopup();});


    //set next color
    if (e.layerType == 'rectangle'){
	spvar.map.editControl.setDrawingOptions({
	    rectangle:{shapeOptions:{color: this.nextColor(),weight: 2,
				     opacity:.9}}
	});
    }

    if (e.layerType == 'polygon'){
	spvar.map.editControl.setDrawingOptions({
	    polygon:{shapeOptions:{color: this.nextColor(),weight: 2,
				   opacity:.9}}
	});
    }
    this.redraw(spvar,false);
};

//draw count on the polygon
Model.prototype.updatePolygonCount = function(layer, spvar){
    var q = this.totalcount_query(spvar.constraints[layer._leaflet_id]);
    q.run_query().done(function(json){
	var countstr ="Count: 0";
	if (json != null){
	    var count = json.root.val;
	    countstr ="Count: ";
	    countstr += count.toString().replace(/\B(?=(\d{3})+(?!\d))/g,",");
	}

	var geojson = layer.toGeoJSON();
	var bbox = bboxGeoJSON(geojson);
	var bboxstr = "Bbox: ";
	bboxstr += "(("+bbox[0][0].toFixed(3)+","+bbox[0][1].toFixed(3)+"),";
	bboxstr += "("+bbox[1][0].toFixed(3)+","+bbox[1][1].toFixed(3)+"))";

	layer.bindPopup(countstr+"<br />" +bboxstr);
    });
};

Model.prototype.drawDeleted = function(e,spvar){
    var layers = e.layers;
    var that = this;
    layers.eachLayer(function (layer) {
	spvar.deleteSelection(layer._leaflet_id);
	delete that.selcolors[layer._leaflet_id];
	that.removeObsolete(layer._leaflet_id);
    });
    this.redraw();
};

Model.prototype.drawEdited = function(e,spvar){
    var that = this;
    var layers = e.layers;
    layers.eachLayer(function (layer) {
	var coords = layer.toGeoJSON().geometry.coordinates[0];
	coords = coords.map(function(e){ return L.latLng(e[1],e[0]); });
	coords.pop();
	var tilelist = genTileList(coords, Math.min(spvar.maxlevel,
						    e.target._zoom+8));
	spvar.updateSelection(layer._leaflet_id,tilelist);
    });

    this.redraw(spvar,false);
};

Model.prototype.drawEditing = function(e,spvar){
    var that = this;
    var obj = e.layer._shape || e.layer._poly;

    var coords = obj._latlngs; // no need to convert
    var tilelist = genTileList(coords, Math.min(spvar.maxlevel,
						e.target._zoom+8));
    spvar.updateSelection(obj._leaflet_id,tilelist);
    this.redraw(spvar,false);
};

//Panel
Model.prototype.panelFuncs = function(maptiles,heatmap){
    //panel btns
    var that = this;

    $("#heatmap-rad-btn-dec").on('click', function(){
	heatmap.coarselevels = Math.max(heatmap.coarselevels-1,0);
	return heatmap.redraw();
    });

    $("#heatmap-rad-btn-inc").on('click', function(){
	heatmap.coarselevels = Math.min(heatmap.coarselevels+1,8);
	return heatmap.redraw();
    });

    $("#heatmap-op-btn-dec").on('click', function(){
	var heatmapop = heatmap.options.opacity;
	heatmapop = Math.max(heatmapop-0.1, 1e-3);
	return heatmap.setOpacity(heatmapop);
    });

    $("#heatmap-op-btn-inc").on('click', function(){
	var heatmapop = heatmap.options.opacity;
	heatmapop = Math.min(heatmapop+0.1, 1.0);
	return heatmap.setOpacity(heatmapop);
    });

    $("#map-op-btn-dec").on('click', function(){
	var mapop = maptiles.options.opacity;
	mapop = Math.max(mapop-0.1, 1e-3);
	return maptiles.setOpacity(mapop);
    });

    $("#map-op-btn-inc").on('click', function(){
	var mapop = maptiles.options.opacity;
	mapop = Math.min(mapop+0.1, 1.0);
	return maptiles.setOpacity(mapop);
    });

    $("#flip-grid").on('change', function(){
	return heatmap.toggleShowCount(); //refresh
    });

    $("#flip-log").on('change', function(){
	return heatmap.toggleLog(); //refresh
    });

    $("#flip-refresh").on('change', function(){
	if (this.value == "on"){
	    that.animate(true,1,10);
	}
	else{
	    that.animate(false);
	}
    });

    $("#tbinsize-btn-dec").on('click', function(){
	var k = Object.keys(that.temporal_vars);
	var tvar = that.temporal_vars[k[0]];
	var hr = hourbSizes.pop();

	hourbSizes.unshift(tvar.binSizeHour());
	that.setTimeBinSize(hr, tvar); //shift in reverse
	return that.redraw(); //refresh
    });

    $("#tbinsize-btn-inc").on('click', function(){
	var k = Object.keys(that.temporal_vars);
	var tvar = that.temporal_vars[k[0]];
	var hr = hourbSizes.shift();

	hourbSizes.push(tvar.binSizeHour());
	that.setTimeBinSize(hr, tvar); //shift forward
	return that.redraw(); //refresh
    });
};


//Keyboard
Model.prototype.keyboardShortcuts = function(spvar,map){
    var maptiles, heatmap;
    //get the maptiles and heatmap
    Object.keys(map._layers).forEach(function(k){
	if (map._layers[k] instanceof L.NanocubeLayer){
	    heatmap = map._layers[k];
	}
	else{
	    maptiles = map._layers[k];
	}
    });

    //Keyboard interactions
    var that = this;
    var id = map._container.id;
    $('#'+id).keypress(function (e) {
	var code = e.keyCode || e.which;
	if ( e.which == 13 ) {
	    e.preventDefault();
	}

	var heatmapop = heatmap.options.opacity;
	var mapop = maptiles.options.opacity;

	switch(code){
	    //Coarsening
	case 44: //','
	    heatmap.coarselevels = Math.max(0,heatmap.coarselevels-1);
	    return heatmap.redraw();
	    break;
	case 46: //'.'
	    heatmap.coarselevels = Math.min(8,heatmap.coarselevels+1);
	    return heatmap.redraw();
	    break;

	    //Opacity
	case 60: //'shift + ,'
	    //decrease opacity
	    heatmapop = Math.max(heatmapop-0.1,0);
	    return heatmap.setOpacity(heatmapop);
	    break;
	case 62:  //'shift + .'
	    //increase opacity
	    heatmapop = Math.min(heatmapop+0.1,1.0);
	    return heatmap.setOpacity(heatmapop);
	    break;

	case 100: //'d'
	    //decrease opacity
	    mapop = Math.max(mapop-0.1,0);
	    return maptiles.setOpacity(mapop);
	    break;
	case 98:  //'b'
	    //increase opacity
	    mapop = Math.min(mapop+0.1,1.0);
	    return maptiles.setOpacity(mapop);
	    break;

	case 103: // 'g'
	    that.toggleGlobal(spvar);
	    return that.redraw(); //refresh
	    break;

	case 115: // 's' smooth or blocky heatmap
	    heatmap.smooth = !heatmap.smooth;
	    return heatmap.redraw(); //refresh
	    break;

	case 116: // 't' bin size change
	    var k = Object.keys(that.temporal_vars);
	    var tvar = that.temporal_vars[k[0]];
	    var hr = hourbSizes.shift();
	    hourbSizes.push(tvar.binSizeHour());
	    that.setTimeBinSize(hr,tvar);
	    return that.redraw(); //refresh
	    break;

	case 99: // 'c' toggle count
	    return heatmap.toggleShowCount(); //refresh
	    break;

	case 108: // 'l' toggle log scale
	    return heatmap.toggleLog(); //refresh
	    break;

	default:
	    break;
	}
    });
};

//Generate queries with respect to different variables
Model.prototype.queries = function(vref){
    var q = this.nanocube.query();
    var that = this;

    //add constraints of the other variables
    Object.keys(that.temporal_vars).forEach(function(v){
	var thisvref = that.temporal_vars[v];
	if (thisvref != vref){
	    q = thisvref.constraints[0].add(q);
	}
    });
    Object.keys(that.cat_vars).forEach(function(v){
	var thisvref = that.cat_vars[v];
	if (thisvref != vref){
	    q = thisvref.constraints[0].add(q);
	}
    });

    //add spatial view constraints
    Object.keys(that.spatial_vars).forEach(function(v){
	var thisvref = that.spatial_vars[v];
	q = thisvref.view_const.add(q);
    });

    //add spatial selection constraints
    var res = {};
    Object.keys(that.spatial_vars).forEach(function(v){
	var thisvref = that.spatial_vars[v];
	Object.keys(thisvref.constraints).forEach(function(c){
	    var newq = $.extend(true, {}, q); //copy the query
	    res[c]  = thisvref.constraints[c].add(newq);
	});
    });

    Object.keys(res).forEach(function(c){
	res[c] = vref.constraints[0].addSelf(res[c]);
    });

    return res;
};

//Total Count
Model.prototype.totalcount_query = function(spconst){
    var q = this.nanocube.query();
    var that = this;
    //add constraints of the other variables
    Object.keys(that.temporal_vars).forEach(function(v){
	var thisvref = that.temporal_vars[v];
	q = thisvref.constraints[0].add(q);
    });
    Object.keys(that.cat_vars).forEach(function(v){
	var thisvref = that.cat_vars[v];
	q = thisvref.constraints[0].add(q);
    });

    if (spconst == undefined){
	//add spatial view constraints
	Object.keys(that.spatial_vars).forEach(function(v){
	    var thisvref = that.spatial_vars[v];
	    q = thisvref.view_const.add(q);
	});
    }
    else{
	q = spconst.add(q);
    }

    return q;
};

//Set the total count
Model.prototype.updateInfo = function(){
    var that = this;
    var q = this.totalcount_query();

    q.run_query().done(function(json){
	if (json == null){
	    return;
	}
	//count
	var count = 0;
	if (typeof json.root.val != 'undefined'){
	    count = json.root.val;
	}
	var countstr = d3.format(",")(count);


	//Time
	var tvarname = Object.keys(that.temporal_vars)[0];
	var tvar  = that.temporal_vars[tvarname];

	if (!tvar){ //For defaulttime/ no time constraint
	    $('#info').text('Total: ' + countstr);
	    return;
	}

	var time_const = tvar.constraints[0];
	var start = time_const.selection_start;

	var startdate = new Date(tvar.date_offset);

	//Set time in milliseconds from 1970
	startdate.setTime(startdate.getTime()+
			  start*tvar.bin_to_hour*3600*1000);

	var dhours = time_const.selected_bins *tvar.bin_to_hour;

	var enddate = new Date(startdate);
	enddate.setTime(enddate.getTime()+dhours*3600*1000);

	$('#info').text(startdate + ' - '+ enddate + ' '
			+ ' Total: ' + countstr);
    });
};

//Toggle view constraint for spatial variables
Model.prototype.toggleGlobal = function(spvar){
    spvar.toggleViewConst();
    this.removeObsolete(spvar.dim);
};

//Set time aggregation
Model.prototype.setTimeBinSize = function(hr, tvar){
    var b2h = tvar.bin_to_hour;
    //update on the time series plot
    tvar.widget.setBinSizeTxt(hr);
    tvar.setBinSize(Math.ceil(hr*1.0/b2h));
};

Model.prototype.updateTimeStep = function(stepsize,window){
    var that = this;
    var nc = that.nanocube;
    $.getJSON(nc.getSchemaQuery()).done(function(json){
	nc.setSchema(json);
	nc.setTimeInfo().done(function(){
	    nc.setTimeInfo().done(function(){
		var tvarname = Object.keys(that.temporal_vars)[0];
		var tvar = that.temporal_vars[tvarname];
		var time_const = tvar.constraints[0];

		var start = nc.timeinfo.start;
		var end = nc.timeinfo.end;

		var tbinfo = nc.getTbinInfo();
		tvar.date_offset = tbinfo.date_offset;
		tvar.bin_to_hour = tbinfo.bin_to_hour;
		time_const.bin_to_hour = tbinfo.bin_to_hour;

		if (stepsize < 0){ //reset
		    time_const.start=start;
		    time_const.end=end;
		    time_const.nbins=end-start+1;
		}
		else{ //advance
		    time_const.nbins = window;
		    time_const.end=end;
		    time_const.start = time_const.end-window;
		}

		time_const.setSelection(0,0);
		tvar.widget.x.domain([0,1]);
		that.redraw();
	    });
	});
    });
};
    /*var that = this;
    $.getJSON(this.nanocube.getTQuery(), function(json){
	var addr = json.root.children[0].addr;
	addr = addr.toString();
	var start = addr.substring(0,addr.length-8);
	var end = addr.substring(addr.length-8,addr.length);

	start = parseInt(start,16);
	end = parseInt(end,16);

	if (isNaN(start)) start=0;
	if (isNaN(end)) end=0;

	var tvarname = Object.keys(that.temporal_vars)[0];
	var tvar  = that.temporal_vars[tvarname];
	var time_const = tvar.constraints[0];

	if (stepsize < 0){ //reset
	    time_const.start=start;
	    time_const.nbins=end-start+1;
	}
	else{ //advance
	    time_const.nbins=window;
	    time_const.start+=stepsize;
	    if(time_const.start >= end){
		time_const.start=start;
	    }
	}

	time_const.setSelection(0,0);
	tvar.widget.x.domain([0,1]);
	that.redraw();
    });
};*/

Model.prototype.animate = function(auto,stepsize,window){
    auto = typeof auto !== 'undefined' ? auto : false;
    var that = this;
    if (auto){
	this.cache_off = true;
	this.interval = setInterval(function(){
	    that.updateTimeStep(stepsize,window);
	    console.log("auto refresh");
	},1000);
    }
    else{
	clearInterval(this.interval);
	this.updateTimeStep(-1);
	this.cache_off = false;
    }
};
