//Constraint Objects for queries
function Constraint(dim){ this.dim = dim;}
Constraint.prototype.add = function(q){};

//Categorical Variable
function CatConstraint(dim,bytesize){
    this.bytesize = (typeof bytesize != "undefined") ? bytesize:8;

    this.selection=[];
    Constraint.call(this,dim);
}
CatConstraint.prototype=new Constraint();
CatConstraint.prototype.constructor=CatConstraint;

CatConstraint.prototype.add = function(q){
    if (this.selection.length < 1 ){
        return q;
    }
    return q.dim(this.dim).sequence(this.selection);    
};

CatConstraint.prototype.toggle = function(addr){
    var idx = this.selection.indexOf(addr);
    if (idx == -1){ //add unselected cat to selection
        this.selection.push(addr);
    }
    else{
        this.selection.splice(idx,1);
    }
};

CatConstraint.prototype.addSelf = function(q){
    var maxidx = Math.pow(2,this.bytesize*8)-1;
    return q.drilldown().dim(this.dim).findAndDive(maxidx,1);
};


//Temporal Variable
function TemporalConstraint(dim,start,end,bintohour){
    Constraint.call(this,dim);
    this.start=start; 
    this.nbins=end-start+1;
    this.selection_start=start; 
    this.selected_bins=this.nbins;
    this.bintohour = bintohour;
    this.binsize=1;
}

TemporalConstraint.prototype=new Constraint();
TemporalConstraint.prototype.constructor=TemporalConstraint;

TemporalConstraint.prototype.binSize = function(binsize){
    this.binsize = binsize;
};

TemporalConstraint.prototype.add = function(q){
    var nbins = this.selected_bins;
    return q.dim(this.dim).tseries(this.selection_start,nbins,1);
};

TemporalConstraint.prototype.addSelf = function(q){
    var minbinsize = Math.ceil(this.nbins/$('#'+this.dim).width()*3);
    var binsize = Math.max(minbinsize,this.binsize);

    this.start = Math.max(this.start,0);
    var nbins = Math.ceil(this.nbins / binsize);

    return q.drilldown().dim(this.dim).tseries(this.start,
                                               binsize,
                                               nbins);
};

TemporalConstraint.prototype.convertTime = function(t,date_offset){
    var diff = t - date_offset;
    var hours = diff / (60*60*1000);
    return hours/this.bintohour;
};

TemporalConstraint.prototype.setSelection = function(start,end,offset){
    if ((start == 0)  && (end ==0)){ //reset
        this.selection_start = this.start;
        this.selected_bins = this.nbins;
    }
    else{ //set to hours
        start = Math.floor(this.convertTime(start,offset));
        end = Math.floor(this.convertTime(end,offset));
        this.selection_start = start;
        this.selected_bins = end-start+1;
    }
};

TemporalConstraint.prototype.setRange = function(start,end,offset){
    start = Math.floor(this.convertTime(start,offset));
    end = Math.floor(this.convertTime(end,offset));
    this.start = start;
    this.nbins = end-start+1;

    this.selection_start = Math.max(this.start,this.selection_start);
    this.selection_bins = Math.min(this.nbins, this.selection_bins);
};

//Spatial Variable
function SpatialConstraint(dim){
    this.boundary=[];
    Constraint.call(this,dim);
}

SpatialConstraint.prototype=new Constraint();
SpatialConstraint.prototype.constructor=SpatialConstraint;

SpatialConstraint.prototype.add = function(q){
    if (this.boundary.length < 3){
        return q;
    }

    //check for 0 area
    var bbox = this.boundary.reduce(function(p,c){
        var b = p;
        b.minx = Math.min(b.minx,c.x); 
        b.maxx = Math.max(b.maxx,c.x); 
        b.miny = Math.min(b.miny,c.y); 
        b.maxy = Math.max(b.maxy,c.y); 
        return b;
    }, {minx:1e9,maxx:-1e9,miny:1e9,maxy:-1e9});
    
    //reject queries with empty bbox
    if ((bbox.maxx-bbox.minx) < 2 || (bbox.maxy-bbox.miny) < 2){
        return q;
    }

    return q.dim(this.dim).sequence(this.boundary.map(function(t){
        return t.raw();
    }));    
};

SpatialConstraint.prototype.addSelf = function(q){
    return q;
};
