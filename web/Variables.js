//Categorical Variable
function CatVar(dim, valnames){
    this.dim = dim;

    //setup constraint
    this.constraints = {};
    this.constraints[0] = new CatConstraint(dim);

    //setup categorical variable
    this.keyaddr = valnames;
    this.addrkey = {};

    var that = this;
    Object.keys(this.keyaddr).forEach(function(k){ 
        that.addrkey[that.keyaddr[k]]=k; 
    });
}

CatVar.prototype.jsonToList=function(json){
    if (json == null){ //nothing to do
        return [];
    }
    var data = json.root.children.map(function(d){
        var addr = d.addr.toString();
        return { value: +d.value, addr: parseInt(addr,16) };
    });

    return data.sort(function(a,b) {return a.addr-b.addr;});
};

CatVar.prototype.update=function(json,id,color){
    var that = this;
    var data = that.jsonToList(json);

    //turn addr to keys
    data.forEach(function(d){d.cat=that.addrkey[d.addr];});
    
    //1 the gui element and redraw()
    if ('widget' in that){
        that.widget.setData(data,id,color);
        that.widget.redraw();
    }
};

CatVar.prototype.removeObsolete=function(k){
    if ('widget' in this){
        this.widget.removeData(k);
    }
};

//Temporal Variable
function TimeVar(dim,date_offset,start,end,bin_to_hour){
    this.dim = dim;
    this.bin_to_hour = bin_to_hour;
    this.date_offset = date_offset;

    //setup constraint
    this.constraints = {};
    this.constraints[0] = new TemporalConstraint(dim,start,end,bin_to_hour);
    this.constraints[0].dim = dim; //this is ugly
}

TimeVar.prototype.setBinSize=function(binsize){
    this.constraints[0].binSize(binsize);
};

TimeVar.prototype.binSizeHour=function(){
    return this.constraints[0].binsize*this.bin_to_hour;
};

TimeVar.prototype.jsonToList=function(json){
    if (json == null){ //nothing to do
        return [];
    }
    var that = this;
    var data = json.root.children.map(function(d){
        var addr = d.addr.toString();
        var start = addr.substring(0, addr.length-8);
        var end = addr.substring(addr.length-8, addr.length);
       
        start = parseInt(start,16);
        end = parseInt(end,16);

        if (isNaN(start)) start=0;
        if (isNaN(end)) end=0;

        var startdate = new Date(that.date_offset);
        
        //Set time in milliseconds from 1970
        startdate.setTime(startdate.getTime()+
                          start*that.bin_to_hour*3600*1000);
        return {date:startdate,value: +d.value};
    });
    
    return data.sort(function(a,b) {return a.date-b.date;});
};

TimeVar.prototype.removeObsolete=function(k){
    if ('widget' in this){
        this.widget.removeData(k);
    }
};


TimeVar.prototype.update=function(json,id,color){
    var that = this;
    var data = that.jsonToList(json);
    
    //update the gui element and redraw()
    if ('widget' in that){
        that.widget.setData({data: data, color: color},id);
        that.widget.redraw();
    }
};

//Spatial Variable
function SpatialVar(dim){
    this.dim = dim;

    //setup constraint
    this.constraints = {};
    this.view_const = new SpatialConstraint(dim);
    this.constraints[this.dim] = this.view_const;
}

SpatialVar.prototype.jsonToList = function(json){
    return [];
};

SpatialVar.prototype.toggleViewConst = function(){
    if (this.dim in this.constraints){
        if (Object.keys(this.constraints) == 1){ //do not toggle
            return;
        }

        this.constraints[this.dim] = null;
        delete this.constraints[this.dim];
    }
    else{
        this.constraints[this.dim] = this.view_const;
    }

};

SpatialVar.prototype.addSelection = function(key,boundary,color){
    this.constraints[key] = new SpatialConstraint(this.dim);
    this.constraints[key].boundary = boundary;
    this.constraints[key].color = color;
};

SpatialVar.prototype.updateSelection = function(key,boundary){
    this.constraints[key].boundary = boundary;
};

SpatialVar.prototype.deleteSelection = function(key){
    delete this.constraints[key];
    if (Object.keys(this.constraints) < 1){ //toggle view
        this.toggleViewConst();
    }
};

SpatialVar.prototype.update=function(){
    if ('heatmap' in this){
        console.log('heatmap redraw');
        this.heatmap.redraw();
    }
};

SpatialVar.prototype.setCurrentView=function(boundary){
    this.view_const.boundary = boundary;
};
