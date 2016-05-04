/*global $,d3 */

function GroupedBarChart(opts, getDataCallback, updateCallback){
    this.getDataCallback=getDataCallback;
    this.updateCallback=updateCallback;
    
    var name = opts.name;
    var logaxis = opts.logaxis;

    var margin = {top:20,right:10,left:30,bottom:30};
    var id = '#'+name;

    //setup the d3 margins from the css margin variables
    margin.left = Math.max(margin.left, parseInt($(id).css('margin-left')));
    margin.right = Math.max(margin.right, parseInt($(id).css('margin-right')));
    margin.top = Math.max(margin.top, parseInt($(id).css('margin-top')));
    margin.bottom = Math.max(margin.bottom,
                             parseInt($(id).css('margin-bottom')));
    
    $(id).css('margin','0px 0px 0px 0px');
    $(id).css('margin-left','0px');
    $(id).css('margin-right','0px');
    $(id).css('margin-top','0px');
    $(id).css('margin-bottom','0px');


    this.selection = {global:[]};
    if(opts.args){ // set selection from arguments
        this._decodeArgs(opts.args);
    }

    this.id = id;

    var width = $(id).width() - margin.left - margin.right;
    var height = $(id).height()- margin.top - margin.bottom;
    
    //add svg to the div
    this.svg = d3.select(id).append("svg")
        .attr("width", width + margin.left + margin.right)
        .attr("height", height + margin.top + margin.bottom)
        .append("g").attr("transform","translate(" + margin.left +
                          "," + margin.top + ")");
    
    //add title
    var title = this.svg.append("text").attr('y',-5).text(name);
    
    //axis
    if (logaxis){
	this.x  = d3.scale.log().range([0,width]);
    }
    else{
        this.x  = d3.scale.linear().range([0,width]);
    }
    
    this.y0  = d3.scale.ordinal().rangeRoundBands([0,height],0.05);//cat
    this.y1  = d3.scale.ordinal();   //selections

    this.xAxis = d3.svg.axis()
        .scale(this.x)
        .orient("bottom")
	.ticks(3,',.1s');

    
    this.yAxis = d3.svg.axis()
        .scale(this.y0)
        .orient("left");
    
    //add axis to the svg
    this.svgxaxis = this.svg.append("g")
        .attr("class", "axis x")
        .attr("transform", "translate(0," + (height+3) + ")");
    
    this.svgyaxis = this.svg.append("g")
        .attr("class", "axis y")
        .attr("transform", "translate(-3,0)");
}

GroupedBarChart.prototype={
    getSelection: function(){        
        return this.selection;
    },

    _encodeArgs: function(){
        return JSON.stringify(this.getSelection());
    },
    
    _decodeArgs: function(s){
        this.selection = JSON.parse(s);
    },

    update: function(){        
        var widget = this;
        
        var promises = this.getDataCallback();
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

            widget.redraw(res);
        });
    },

    updateAxis: function(data){    
        var xmin = 0.9 * d3.min(data, function(d){return d.val;});
        var xmax = d3.max(data, function(d){return d.val;});
        
        this.x.domain([xmin,xmax]);
        var cats = data.map(function(d){return d.cat;});
        this.y0.domain(cats);
        this.y1.domain(data.map(function(d){return d.color;}))
            .rangeRoundBands([0, this.y0.rangeBand()]);
        
        
        this.svgxaxis.call(this.xAxis);
        this.svgyaxis.call(this.yAxis);

    },

    flattenData: function(data){
        return Object.keys(data).reduce(function(prev,curr){         
            var color=curr;
            var is_color=/(^#[0-9A-Fa-f]{6}$)|(^#[0-9A-Fa-f]{3}$)/i.test(color);
            if(!is_color){
                color = '#f00';
            }
            var row = Object.keys(data[curr].data).map(function(k){
                var d = data[curr].data[k];
                d.color = color;
                return d;
            });
            return prev.concat(row);
        }, []);
    },


    redraw: function(data){
        var widget = this;

        var flatdata = this.flattenData(data);

        flatdata.sort(function(a,b){
            //numeric sort
            var res = parseFloat(a.cat)-parseFloat(b.cat);
            if (!isNaN(res)){
                return res;
            }
            else{
                return a.cat.localeCompare(b.cat);
            }
        });
        
        this.updateAxis(flatdata);    

        //add the bars back
        var bars = this.svg.selectAll('.bar').data(flatdata);

        bars.enter()
            .append('rect').attr('class', 'bar')
            .on('click', function(d){ //click reaction
                if(!widget.selection.brush){
                    widget.selection.brush = [];
                }
                                
                var idx = widget.selection.brush.findIndex(function(b){
                    return (b.cat == d.cat);
                });
                
                if (idx != -1){
                    widget.selection.brush.splice(idx,1);
                }
                else{
                    widget.selection.brush = [{id:d.id, cat:d.cat}];
                }

                if(widget.selection.brush.length < 1){
                    delete widget.selection.brush;
                }            
                
                widget.update(); //redraw itself
                widget.updateCallback(widget._encodeArgs());            
            }) //toggle callback
            .append("svg:title"); //tooltip
        
        
        bars.attr('x', 0)
            .attr('y', function(d){return widget.y0(d.cat) + //category
                                   widget.y1(d.color);}) //selection group
            .style('fill', function(d){
                if (!widget.selection.brush || //no selection
                    widget.selection.brush.findIndex(function(b){
                        return (b.cat == d.cat); }) != -1){//in selection
                    return d.color;
                }
                else{
                    return 'gray';
                }
            })
            .attr('height',function(d){ return widget.y1.rangeBand(); })
            .transition().duration(500)
            .attr('width',function(d){
                return widget.x(d.val);
            });
        
        bars.select('title')
            .text(function(d){
                return d3.format(',')(d.val);
            });
        
        bars.exit().remove();
    }
};
