function Timer(callback, delay) {
    var timerId, start, remaining = delay;

    this.pause = function() {
        window.clearTimeout(timerId);
        remaining -= new Date() - start;
    };

    this.resume = function() {
        start = new Date();
        timerId = window.setTimeout(callback, remaining);
    };

    this.resume();
}

// var timer = new Timer(function() {
//     alert("Done!");
// }, 1000);

// timer.pause();
// // Do some stuff...
// timer.resume();

//------------------------------------------------------------------------------
// Vec2
//------------------------------------------------------------------------------

function Vec2(x, y) {
    this.x = x;
    this.y = y;
    return this;
}

Vec2.prototype.sub = function(other) {
    return new Vec2(this.x - other.x, this.y - other. y);
}

Vec2.prototype.sum = function(other) {
    return new Vec2(this.x + other.x, this.y + other. y);
}

Vec2.prototype.perp = function() {
    return new Vec2(-this.y, this.x);
}

Vec2.prototype.length  = function() {
    return Math.sqrt(this.x * this.x + this.y * this.y);
}

Vec2.prototype.scaled = function(scalar) {
    return new Vec2(scalar * this.x, scalar * this.y);
}

Vec2.prototype.normalized = function() {
    var length = this.length();
    if (length != 0.0) {
        return new Vec2(this.x/length, this.y/length);
    }
    else {
        return new Vec2(this.x, this.y); // not robust. check this.
    }
}


//------------------------------------------------------------------------------
// MessageStack
//------------------------------------------------------------------------------

function MessageStack() {
    this.messages = [];
    this.dom = null;
}

MessageStack.prototype.push = function(msg) {
    this.messages.push(msg);
    return this;
}

MessageStack.prototype.pop = function() {
    this.messages.pop();
    return this;
}

MessageStack.prototype.last = function() {
    return this.messages[this.messages.length-1];
}

MessageStack.prototype.setDOM = function(dom) {
    this.dom = dom;
    return this;
}

MessageStack.prototype.updateUI = function(state) {

    // debugger;

    this.dom.selectAll("text").data([]).exit().remove()
    // while (this.dom.lastChild) {
    //     this.dom.removeChild(this.dom.lastChild);
    // }

    // debugger;

    this.dom
        .selectAll("txt_lines")
        .data(this.messages)
        .enter()
        .append("text")
        .attr("dy", function(d,i) { return i * 30; } )
        .style("fill","#FFFFFF")
        .text(function (d) { return d; });


//        .style("text-anchor", "middle")

    // this.dom.attr("transform", "translate(" + this.pos.x + "," + this.pos.y + ")");
    return this;
}


//------------------------------------------------------------------------------
// Node
//------------------------------------------------------------------------------

function Node(id, dim, layer) {
    this.id      = id;
    this.dim     = dim;
    this.layer   = layer;
    this.dom     = null;

    this.color   = 0; // used for highlights

    this.objects = [];

    this.pos   = {x: 10 + Math.random() * 480 , y: 10 + Math.random() * 230};

    this.children = {};
    this.content  = null;
    return this;
}

Node.prototype.setVisible = function(flag) {
    if (flag) {
        this.dom.attr("visibility","visible");
    }
    else {
        this.dom.attr("visibility","hidden");
    }
}

Node.prototype.setColor = function(color) {
    this.color = color;
}

Node.prototype.getColor = function() {
    return this.color;
}

Node.prototype.getChildLink = function(label) {
    return (label in this.children) ? this.children[label] : null;
}

Node.prototype.pushObject = function(obj) {
    this.objects.push(obj);
    return this;
}

Node.prototype.popObject = function(obj) {
    this.objects.pop();
    return this;
}

Node.prototype.setChildLink = function(label, child_link) {
    this.children[label] = child_link;
    return this;
}

Node.prototype.getContentLink = function() {
    return this.content;
}

Node.prototype.setContentLink = function(content_link) {
    this.content = content_link;
    return this;
}

Node.prototype.setDOM = function(dom) {
    this.dom = dom;
    return this;
}

Node.prototype.setPos = function(x, y) {
    // debugger;
    this.pos = { x:x, y:y };
    return this;
}


Node.prototype.updateUI = function(state) {
    this.dom.attr("transform", "translate(" + this.pos.x + "," + this.pos.y + ")");

    st = "" + this.id;
    for (var i=0;i<this.objects.length;i++) {
        if (i == 0) {
            st += ": ";
        }
        else {
            st += " ";
        }
        st += "" + this.objects[i];
    }

    this.dom
        .selectAll("text")
        .text(st);

        // .style("stroke","#777777")
        // .style("stroke-width",4)
        // .style("fill","#FFFFFF");

    var stroke = "#777777";
    if (this.color == 1) {
        stroke = "#FF0000";
    }
    else if (this.color == 2) {
        stroke = "#00FF00";
    }

    var circle = this.dom.selectAll("circle");

    // debugger;

    circle.style("stroke",stroke);

    return this;
}

//------------------------------------------------------------------------------
// ChildLink
//------------------------------------------------------------------------------

function ChildLink(parent, child, label, shared) {
    this.parent = parent
    this.child  = child
    this.label  = label
    this.shared = shared
    this.dom       = null
    this.dom_link  = null
    this.dom_label = null
}

ChildLink.prototype.setDOM = function(dom, dom_link, dom_label) {
    this.dom = dom;
    this.dom_link  = dom_link;
    this.dom_label = dom_label;
    return this;
}

ChildLink.prototype.setVisible = function(flag) {
    if (flag) {
        this.dom.attr("visibility","visible");
    }
    else {
        this.dom.attr("visibility","hidden");
    }
}

ChildLink.prototype.updateUI = function(state) {

    data = [ this.parent.pos, this.child.pos ]

    var lineFunction = d3.svg.line()
        .x(function(d) { return d.x; })
        .y(function(d) { return d.y; })
        .interpolate("linear");

    this.dom_link.attr("d", lineFunction(data));

    
    var u = new Vec2(this.parent.pos.x, this.parent.pos.y);
    var v = new Vec2(this.child.pos.x, this.child.pos.y);
    var label_pos = u.sum(v.sub(u).normalized().scaled(30));
    

    // var center = {x: this.parent.pos.x + 0.3 * (this.child.pos.x - this.parent.pos.x),
    //               y: this.parent.pos.y + 0.3 * (this.child.pos.y - this.parent.pos.y)};

    this.dom_label.attr("transform", "translate(" + label_pos.x + "," + label_pos.y + ")");

    return this;
}

//------------------------------------------------------------------------------
// ContentLink
//------------------------------------------------------------------------------

function ContentLink(node, content, shared) {
    this.node     = node
    this.content  = content
    this.shared   = shared
    this.dom      = null
}

ContentLink.prototype.setVisible = function(flag) {
    if (flag) {
        this.dom.attr("visibility","visible");
    }
    else {
        this.dom.attr("visibility","hidden");
    }
}

ContentLink.prototype.setDOM = function(dom) {
    this.dom = dom;
    return this;
}

ContentLink.prototype.updateUI = function(state) {
    // update dom

    data = [ this.node.pos, this.content.pos ];
    if (this.node.layer == 0) {

        var u = new Vec2(this.node.pos.x, this.node.pos.y);
        var v = new Vec2(this.content.pos.x, this.content.pos.y);

        // debugger;

        var uv = v.sub(u);

        // console.log("uv: " + uv.x +"," + uv.y);

        var uv_length = uv.length();

        // console.log("uv_length: " + uv_length);

        var uv_perp_norm = uv.perp().normalized();

        // console.log("uv_perp_norm: " + uv_perp_norm.x +"," + uv_perp_norm.y);

        var mid_point = u.sum(uv.scaled(0.5));

        // console.log("mid_point: " + mid_point.x +","+mid_point.y);

        var sign = u.x < v.x ? -1.0 : 1.0;

        var notch = mid_point.sum(uv_perp_norm.scaled(sign * 0.18 * uv_length));

        // console.log("notch: " + notch.x +","+notch.y);
        
        data = [ this.node.pos, notch, this.content.pos ];

    }

    var lineFunction = d3.svg.line()
        .x(function(d) { return d.x; })
        .y(function(d) { return d.y; })
        .interpolate("basis");

    this.dom
        .attr("d", lineFunction(data));

    return this;
}

//------------------------------------------------------------------------------
// ActionPushMessage
//------------------------------------------------------------------------------

function ActionPushMessage(message_stack, msg) {
    this.message_stack = message_stack;
    this.msg           = msg;
}

ActionPushMessage.prototype.execute = function() {
    this.message_stack.push(this.msg);
    this.message_stack.updateUI();

    console.log("Execute ActionPushMessage " + this.msg);

    return this;
}

ActionPushMessage.prototype.unexecute = function() {
    this.message_stack.pop();
    this.message_stack.updateUI();

    console.log("Undo ActionPushMessage " + this.msg);

    return this;
}

//------------------------------------------------------------------------------
// ActionPopMessage
//------------------------------------------------------------------------------

function ActionPopMessage(message_stack) {
    this.message_stack = message_stack;
    this.undo_msg = this.message_stack.last();
}

ActionPopMessage.prototype.execute = function() {
    this.message_stack.pop();
    this.message_stack.updateUI();

    console.log("Execute ActionPopMessage");

    return this;
}

ActionPopMessage.prototype.unexecute = function() {
    this.message_stack.push(this.undo_msg);
    this.message_stack.updateUI();

    console.log("Undo ActionPopMessage");

    return this;
}

//------------------------------------------------------------------------------
// ActionStore
//------------------------------------------------------------------------------

function ActionStore(node, object) {
    this.node   = node;
    this.object = object;
}

ActionStore.prototype.execute = function() {
    this.node.pushObject(this.object);
    this.node.updateUI();

    console.log("Execute ActionStore");

    return this;
}

ActionStore.prototype.unexecute = function() {
    this.node.popObject();
    this.node.updateUI();

    console.log("Undo ActionStore");

    return this;
}

//------------------------------------------------------------------------------
// ActionStore
//------------------------------------------------------------------------------

function ActionHighlightNode(node, color) {
    this.node      = node;
    this.color     = color;
    this.old_color = node.getColor();
}

ActionHighlightNode.prototype.execute = function() {
    this.node.setColor(this.color);
    this.node.updateUI();

    console.log("Execute ActionHighlightNode");

    return this;
}

ActionHighlightNode.prototype.unexecute = function() {
    this.node.setColor(this.old_color);
    this.node.updateUI();

    console.log("Undo ActionHighlightNode");

    return this;
}


//------------------------------------------------------------------------------
// ActionNewNode
//------------------------------------------------------------------------------

function ActionNewNode(node) {
    this.node  = node
}

ActionNewNode.prototype.execute = function() {
    this.node.setVisible(true);

    console.log("Execute ActionNewNode " + this.node.id);

    return this;
}

ActionNewNode.prototype.unexecute = function() {
    this.node.setVisible(false);

    console.log("Undo ActionNewNode " + this.node.id);

    return this;
}

//------------------------------------------------------------------------------
// ActionSetContentLink
//------------------------------------------------------------------------------

function ActionSetContentLink(content_link) {
    this.previous_content_link = content_link.node.getContentLink();
    this.next_content_link     = content_link;
}

ActionSetContentLink.prototype.execute = function() {
    this.next_content_link.node.setContentLink(this.next_content_link);
    this.next_content_link.setVisible(true);
    if (this.previous_content_link != null) {
        this.previous_content_link.setVisible(false);
    }
    console.log("Execute ActionSetContentLink " + 
                this.next_content_link.node.id + " -> " + 
                this.next_content_link.content.id);

    return this;
}

ActionSetContentLink.prototype.unexecute = function() {
    this.next_content_link.node.setContentLink(this.previous_content_link);
    this.next_content_link.setVisible(false);
    if (this.previous_content_link != null) {
        this.previous_content_link.setVisible(true);
    }

    console.log("Undo ActionSetContentLink " + 
                this.next_content_link.node.id + " -> " + 
                this.next_content_link.content.id);
    return this;
}

//------------------------------------------------------------------------------
// ActionSetChildLink
//------------------------------------------------------------------------------

function ActionSetChildLink(child_link) {
    this.previous_child_link = child_link.parent.getChildLink(child_link.label);
    this.child_link          = child_link;
}

ActionSetChildLink.prototype.execute = function() {
    this.child_link.parent.setChildLink(this.child_link.label, this.child_link);
    if (this.previous_child_link != null) {
        this.previous_child_link.setVisible(false);
    }
    this.child_link.setVisible(true);

    console.log("Execute ActionSetChildLink " + 
                this.child_link.parent.id + " --(" + 
                this.child_link.label + ")-- " +
                this.child_link.child.id);

    return this;
}

ActionSetChildLink.prototype.unexecute = function() {
    this.child_link.parent.setChildLink(this.child_link.label, this.previous_child_link);
    if (this.previous_child_link != null) {
        this.previous_child_link.setVisible(true);
    }
    this.child_link.setVisible(false);


    console.log("Undo ActionSetChildLink " + 
                this.child_link.parent.id + " -- " + 
                this.child_link.label + " -- " +
                this.child_link.child.id);

    return this;
}

//------------------------------------------------------------------------------
// Model
//------------------------------------------------------------------------------


Model.prototype.execute = function(action) {
    if (this.history_index == this.history.length) {
        this.history.push(action);
        this.history_index++;
        action.execute();
    }
    return this;
}

Model.prototype.advance = function() {
    if (this.history_index < this.history.length) {
        this.history[this.history_index].execute();
        this.history_index++;
    }
    return this;
}

Model.prototype.rewind = function() {
    if (this.history_index > 0) {
        this.history_index--;
        this.history[this.history_index].unexecute();
    }
    return this;
}

Model.prototype.first = function() {
    while (this.history_index > 0) {
        this.rewind();
    }
}

Model.prototype.last = function() {
    while (this.history_index < this.history.length) {
        this.advance();
    }
}

function Model(state) {

    this.content_links    = []
    this.child_links      = []
    this.node_map         = {} 
    this.nodes            = []

    this.history          = []
    this.history_index    = 0

    function createNodeDOM(node) {

        var group = state.nodes_layer.selectAll("nodes")
            .data([node])
            .enter()
            .append("g")
            .attr("class", "node")
            // .attr("visibility","hidden")
            .attr("transform", function(d) { return "translate(" + d.pos.x + "," + d.pos.y + ")"; });

        if (node.dim == 3) {
            group.selectAll("rect")
                .data([node])
                .enter()
                .append("rect")
                .attr("width",60)
                .attr("height",30)
                .attr("x",-30)
                .attr("y",-15)
                .style("stroke","#777777")
                .style("stroke-width",4)
                .style("fill","#FFFFFF");
        }
        else {
            group.selectAll("circle")
                .data([node])
                .enter()
                .append("circle")
                .attr("r",15)
                .style("stroke","#777777")
                .style("stroke-width",4)
                .style("fill","#FFFFFF");
        }
        
        group.selectAll("node_circles")
            .data([node])
            .enter()
            .append("text")
            .attr("dy", ".3em")
            .style("fill","#000000")
            .style("text-anchor", "middle")
            .text(function (d) { return d.id; })

        node.setDOM(group); // set document object model

    }

    function createContentLinkDOM(content_link) {

        data = [ content_link.node.pos, content_link.content.pos ]

        var lineFunction = d3.svg.line()
            .x(function(d) { return d.x; })
            .y(function(d) { return d.y; })
            .interpolate("linear");

        var dom = state.content_links_layer.selectAll("content_links")
            .data([0])
            .enter()
            .append("path")
            // .attr("visibility","hidden")
            .attr("d", lineFunction(data))
            .attr("stroke-dasharray", (content_link.shared ? "5" : "5,0"))
            .attr("stroke", "blue")
            .attr("stroke-width", 1)
            .attr("fill", "none");

        content_link.setDOM(dom);

    }

    function createChildLinkDOM(child_link) {

        data = [ child_link.parent.pos, child_link.child.pos ]

        var lineFunction = d3.svg.line()
            .x(function(d) { return d.x; })
            .y(function(d) { return d.y; })
            .interpolate("linear");

        var master_group = state.child_links_layer.selectAll("child_links_text")
            .data([1])
            .enter()
            .append("g")
            .attr("class", "node");

        var dom_link = master_group.selectAll("child_link")
            .data([0])
            .enter()
            .append("path")
            // .attr("visibility","hidden")
            .attr("d", lineFunction(data))
            .attr("stroke-dasharray", (child_link.shared ? "5,5" : "5,0"))
            .attr("stroke", "black")
            .attr("stroke-width", 3)
            .attr("fill", "none");

        var dom_label = master_group.selectAll("child_links_text")
            .data([{x: 0.5*(child_link.parent.pos.x + child_link.child.pos.x),
                    y: 0.5*(child_link.parent.pos.y + child_link.child.pos.y)}])
            .enter()
            .append("g")
            .attr("class", "node")
            // .attr("visibility","hidden")
            .attr("transform", function(d) { return "translate(" + d.x + "," + d.y + ")"; });

        dom_label.selectAll("text_bg")
            .data([child_link])
            .enter()
            .append("rect")
            .attr("transform", function(d) { return "translate(-7,-7)"; })
            .attr("width", 14)
            .attr("height", 14)
            .style("fill-opacity",0.9)
            .style("fill", "#FFFFFF")

        dom_label.selectAll("text")
            .data([child_link])
            .enter()
            .append("text")
            .style("fill", "#000000")
            .style("text-anchor", "middle")
            .attr("dy", ".35em")
            .text( function (d) { return d.label; })

        child_link.setDOM(master_group, dom_link, dom_label);


    }

    function createMessageStackDOM(message_stack) {

        var group = state.message_canvas.selectAll("nodes")
            .data([node])
            .enter()
            .append("g")
            .attr("class", "text")
            // .attr("visibility","hidden")
            .attr("transform", "translate(10,30)");

        message_stack.setDOM(group); // set document object model

    }

    var message_stack = new MessageStack();
    createMessageStackDOM(message_stack);

    for (var i=0;i<state.actions.length;i++) {

        var a = state.actions[i]

        // debugger
        // console.log("Action: " + a.action);

        if (a.action == "new_node") {
            var node = new Node(a.node, a.dim, a.layer);
            this.node_map[a.node] = node;
            this.nodes.push(node)

            createNodeDOM(node);

            this.execute(new ActionNewNode(node));

        }
        else if (a.action == "set_content_link") {
            var content_link = 
                new ContentLink(
                    this.node_map[a.node],
                    this.node_map[a.content],
                    a.shared == 1 ? true : false);
            this.content_links.push(content_link);
            createContentLinkDOM(content_link);

            this.execute(new ActionSetContentLink(content_link));

        }
        else if (a.action == "set_child_link") {
            var child_link = 
                new ChildLink(
                    this.node_map[a.parent],
                    this.node_map[a.child],
                    a.label,
                    a.shared == 1 ? true : false);
            this.child_links.push(child_link);
            createChildLinkDOM(child_link);

            this.execute(new ActionSetChildLink(child_link));
        }
        else if (a.action == "push") {
            this.execute(new ActionPushMessage(message_stack, a.msg));
        }
        else if (a.action == "pop") {
            this.execute(new ActionPopMessage(message_stack));
        }
        else if (a.action == "store") {
            this.execute(new ActionStore(this.node_map[a.node], a.obj));
        }
        else if (a.action == "highlight_node") {
            this.execute(new ActionHighlightNode(this.node_map[a.node], a.color));
        }
    }

    // create layers
    var layers = {}
    for (var i=0;i<this.nodes.length;i++) {
        var node = this.nodes[i]
        key = node.dim + "," + node.layer;
        if (key in layers) {
            layers[key].push(node)
        }
        else {
            layers[key] = [node]
        }
    }

    // reposition the nodes and paths appropriately
    {
        layer_keys = Object.keys(layers);
        layer_keys.sort()

        n = layer_keys.length

        var layer_height = state.canvas_height / n;
        var layer_width  = state.canvas_width;

        var node_width   = 30

        var xmargin      = 50

        function position(i,j,n) {
            var x = layer_width/2.0 - (n * node_width + (n-1) * xmargin)/2.0 + (j + 0.5) * node_width + j * xmargin;
            var y = (i + 0.5) * layer_height;
            return {x: x, y: y};
        }

        for (var i=0;i<layer_keys.length;i++) {
            key = layer_keys[i];
            nodes = layers[key];
            for (var j=0;j<nodes.length;j++) {
                var node = nodes[j];
                var pos = position(i,j,nodes.length);
                node.setPos(pos.x, pos.y);
                node.updateUI(state);
            }
        }

        for (var i=0;i<this.child_links.length;i++) {
            this.child_links[i].updateUI();
        }

        for (var i=0;i<this.content_links.length;i++) {
            this.content_links[i].updateUI();
        }
    }


}



$(document).ready(function() {

    // read actions json object
    d3.json("actions.json", function(error, json) {

        var raw_action_list = json;

        console.log(JSON.stringify(error));
        console.log(JSON.stringify(json));

        var canvas_width  = 640;
        var canvas_height = 720;

        var message_panel_width  = 640;
        var message_panel_height = 720;

        var canvas = d3.select("body")
            .append("svg")
            .attr("width", canvas_width)
            .attr("height",canvas_height)
            .attr("style","background: #DDDDDD")
            .style("font-face","Courier New")
//            .style("font-weight","bold")
            .style("font-size","11px");

        var message_canvas = d3.select("body")
            .append("svg")
            .attr("width", message_panel_width)
            .attr("height",message_panel_height)
            .attr("style","background: #000000")
            .style("font-face","Helvetica Neue")
            .style("font-size","14px");

        var state = {
            message_canvas: message_canvas,
            canvas: canvas,
            canvas_width: canvas_width,
            canvas_height: canvas_height,
            content_links_layer: canvas.append("g"),
            child_links_layer:   canvas.append("g"),
            nodes_layer:         canvas.append("g"),
            index: -1,
            actions: raw_action_list,
            nodes: {}
        };

        var model = new Model(state);
        

        model.first();

        var playing   = true;
        var direction = 1;


        var timer = $.timer(function() { 
            if (playing) {
                if (direction > 0) {
                    model.advance(); 
                }
                else {
                    model.rewind(); 
                }
            }
        }, 125, true);


        function log_svg()
        {
            // Extract the data as SVG text string

            // var svg = document.getElementById("ex1");
            var svg = document.getElementsByTagName("svg")[0];

            svg = svg.cloneNode(true);
            svg.setAttribute("xmlns","http://www.w3.org/2000/svg");
            svg.setAttribute("xmlns:svg","http://www.w3.org/2000/svg");

            //
            tag_names = ["g", "path", "rect", "text", "circle"]

            for (var tag_index=0;tag_index < tag_names.length;tag_index++) {
                var tag_name = tag_names[tag_index];
                // now erase every element with hidden visibility
                var hidden_groups = svg.getElementsByTagName(tag_name);
                for (var i=hidden_groups.length-1;i>=0;i--) {
                    var group = hidden_groups[i];
                    // debugger;
                    if (group.getAttribute("visibility") == "hidden") {

                        debugger;

                        var parent = group.parentNode;

                        parent.removeChild(group);

                        debugger;

                        console.log("Remove Child");
                    }
                }
            }

            debugger;

            var svg_xml = (new XMLSerializer).serializeToString(svg);
            // console.log(svg_xml);

            open("data:image/svg+xml," + "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>" + encodeURIComponent(svg_xml));
        }

        var buttons = [ 
            {name:"First", x:5,   y:15, action: function() { playing = false;     model.first();   } },
            {name:"Prev",  x:70,  y:15, action: function() { playing = false;     model.rewind();  } },
            {name:"<<",    x:135, y:15, action: function() { playing = !playing;  direction=-1;    } },
            {name:">>",    x:200, y:15, action: function() { playing = !playing;  direction=1;     } },
            {name:"Next",  x:265, y:15, action: function() { playing = false;     model.advance(); } },
            {name:"Last",  x:330, y:15, action: function() { playing = false;     model.last();    } },
            {name:"SVG",   x:395, y:15, action: function() { playing = false;     log_svg();       } }
            ];

        canvas.selectAll("buttons")
            .data(buttons)
            .enter()
            .append("text")
            .attr("x", function (d) { return d.x; })
            .attr("y", function (d) { return d.y; })
            .attr("cursor", "pointer")
            .text(     function (d) { return d.name; })
            .on("click", function (d) { d.action(); });

    });


});
