/*global d3 $ */
function RetinalBrushes(opts, updateCallback){
    this.updateCallback=updateCallback;

    var name=opts.name;
    var id = "#"+name.replace(/\./g,'\\.');

    var margin = {top: 20, right: 20, bottom: 20, left: 20};

    if(opts.args){
    	console.log(opts.args);
    	this._decodeArgs(opts.args);
    }

    var widget = this;
    //Make draggable and resizable
    d3.select(id).attr("class","retbrush resize-drag");

    this.coldrop = d3.select(id).append("div")
    	.attr("class", "retoptions dropzone")
    	.html("color");

    this.xdrop = d3.select(id).append("div")
    	.attr("class", "retoptions dropzone")
    	.html("x");

    this.ydrop = d3.select(id).append("div")
    	.attr("class", "retoptions dropzone")
    	.html("y");

    var labelNames = Object.keys(opts.model._widget);
    this.labels = labelNames.map(function(k, i){
    	var label = d3.select(id).append("div")
    		.style("width", k.length * 8 + "px")
	    	.style("height", 20 + "px")
	    	.style("left", ((i % 3) * 100  + 10) + "px")
	    	.style("top", (Math.floor(i / 4) * 25 + 50) + "px")
	    	.attr("class", "retlabels draggable")
	    	.html(k);

		return label;
    });

    this.retbrush = {
    	color:'',
    	x:'',
    	y:''
    };

    
    interact('.dropzone').dropzone({
        overlap: 0.51,
        ondrop: function(event) {
        	if(widget.retbrush[event.target.textContent] !== '')
        		return;
            event.relatedTarget.classList.add('dropped');
            widget.retbrush[event.target.textContent] = event.relatedTarget.textContent;
            widget.update();
        },
        ondragleave: function(event) {
        	if(widget.retbrush[event.target.textContent] !== event.relatedTarget.textContent)
        		return;
            event.relatedTarget.classList.remove('dropped');
            widget.retbrush[event.target.textContent] = '';
            widget.update();
        }
    });


}

RetinalBrushes.prototype={
	update: function(){
		var widget = this;
		// console.log(this.retbrush);
		this.updateCallback(widget._encodeArgs(), widget.retbrush);
	},

	getSelection: function(){
		return this.retbrush;
	},

	_encodeArgs: function(){
        var args= this.getSelection();
        return JSON.stringify(args);
    },
    
    _decodeArgs: function(s){
        var args = JSON.parse(s);
        this.retbrush = args;
    },
};
