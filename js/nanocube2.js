//-----------------------------------------------------------------------
// binary_xhr
//-----------------------------------------------------------------------

function binary_xhr(url, handler)
{
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
};


//-----------------------------------------------------------------------
// Dimension
//-----------------------------------------------------------------------

function Dimension (obj) {
    this.name   = obj.name
    this.type   = obj.type
    return this;
}

//-----------------------------------------------------------------------
// Nanocube
//-----------------------------------------------------------------------

function Nanocube (opts) {
    this.url        = opts.url;
    this.schema     = null;
    this.dimensions = null;

    var that = this;

    $.getJSON(this.getSchemaQuery(), function( json ) {
        that.schema = json;

        that.dimensions = 
            that.schema.fields
            .filter( function (f) { return f.type.indexOf("nc_dim") == 0; } )
            .map( function (f) { return new Dimension({name: f.name, type: f.type}); } );

        // debugger;

        opts.ready(that);
    });

    return this;
}

Nanocube.prototype.getDimension = function(dim_name) {
    lst = this.dimensions.filter(function (f) { return f.name==dim_name; } );
    return lst[0];
}

Nanocube.prototype.getSchemaQuery = function() {
    return this.url + "/schema";
}

Nanocube.prototype.query = function() {
    return new Query(this);
}

Nanocube.prototype.getFieldNames = function() {
    return this.schema.fields
        .filter(function (f) { return f.type.indexOf("nc_dim") == 0;})
        .map(function (f) { return f.name; });
}

//-----------------------------------------------------------------------
// Query
//-----------------------------------------------------------------------

function Query (nanocube) {
    this.nanocube       = nanocube;
    this.dimension      = null;
    this.drilldown_flag = false;
    this.query_elements = {};
}

Query.prototype.reset = function() {
    this.query_elements = {}
    return this;
}

Query.prototype.dim = function(dim_name) {
    this.dimension = nanocube.getDimension(dim_name);
    return this;
}

Query.prototype.drilldown = function() {
    this.drilldown_flag = true;
    return this;
}

Query.prototype.rollup = function() {
    this.drilldown_flag = false;
    return this;
}

Query.prototype.findAndDive = function(addr, offset) {
    var constraint = 
        (this.drilldown_flag ? "@" : "") 
        + this.dimension.name + "="
        + addr + "+" + offset;
    this.query_elements[this.dimension.name] = constraint;
    return this;
}

Query.prototype.range = function(addr0, addr1) {
    var constraint = 
        (this.drilldown_flag ? "@" : "") 
        + this.dimension.name + "="
        + "[" + addr1 + "+" + offset + "]";
    this.query_elements[this.dimension.name] = constraint;
    return this;
}

Query.prototype.sequence = function(addr_sequence) {
    var constraint = 
        (this.drilldown_flag ? "@" : "") 
        + this.dimension.name + "="
        + "<" + addr_sequence.join(",") + ">";
    this.query_elements[this.dimension.name] = constraint;
    return this;
}

Query.prototype.tseries = function(base, bucket, count) {
    var constraint = 
        (this.drilldown_flag ? "@" : "") 
        + this.dimension.name + "="
        + base + ":" + bucket + ":" + count;
    this.query_elements[this.dimension.name] = constraint;
    return this;
}

Query.prototype.run_query = function(callback) {
    var query_string = [this.nanocube.url,"query"].concat(_.values(this.query_elements)).join("/");
    console.log(query_string);
    d3.json(query_string, callback);
    return this;
}

Query.prototype.run_tile = function(callback) {
    var query_string = [this.nanocube.url,"tile"].concat(_.values(this.query_elements)).join("/");
    console.log(query_string);
    binary_xhr(query_string, callback);
    return this;
}


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
}

//-----------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------

$(document).ready(
    function() {
        nanocube = new Nanocube(
            {
                url:"http://localhost:29512", 
                ready:function (nanocube) {
                    var query = nanocube
                        .query()
                        .dim("src")
                        .drilldown()
                        .findAndDive((new Tile(0,0,0)).raw(),1);
                       // .findAndDive((new Tile(327,1256,11)).raw(),0);
                    
                    // debugger;

                    query.run_tile(function (result) {

                        var record_size = 10;
                        var view = new DataView(result);
                        var n_records = result.byteLength / record_size;

                        var x_array = new Uint8Array(n_records);
                        var y_array = new Uint8Array(n_records);
                        var count_array = new Float64Array(n_records);
                        for (var i=0; i<n_records; ++i) {
                            x_array[i]     = view.getUint8( record_size*i+1 );
                            y_array[i]     = view.getUint8( record_size*i   );
                            count_array[i] = view.getFloat64( record_size*i+2, true );
                            console.log(
                                JSON.stringify({x: x_array[i], y: y_array[i], value: count_array[i]})
                            );
                        }
                        // k({x: x_array, y: y_array, count: count_array});
                    });



                    var query2 = nanocube
                        .query()
                        // .dim("src")
                        // .rollup()
                        // .findAndDive((new Tile(0,0,0)).raw(),8);
                        .dim("src")
                        .rollup()
                        .sequence([(new Tile(0,0,10)).raw(),
                                   (new Tile(0,1023,10)).raw(),
                                   (new Tile(1023,0,10)).raw()]);

                    query2.run_query(function (result) {


                        console.log(result);
                        console.log(JSON.stringify(result));
                        // var record_size = 10;
                        // var view = new DataView(result);
                        // var n_records = result.byteLength / record_size;

                        // var x_array = new Uint8Array(n_records);
                        // var y_array = new Uint8Array(n_records);
                        // var count_array = new Float64Array(n_records);
                        // for (var i=0; i<n_records; ++i) {
                        //     x_array[i]     = view.getUint8( record_size*i+1 );
                        //     y_array[i]     = view.getUint8( record_size*i   );
                        //     count_array[i] = view.getFloat64( record_size*i+2, true );
                        //     console.log(
                        //         JSON.stringify({x: x_array[i], y: y_array[i], value: count_array[i]})
                        //     );
                        // }
                        // k({x: x_array, y: y_array, count: count_array});
                    });



                }
            }
        );
    }
);














//
// console.log(JSON.stringify(query));
// debugger;
//
// Query.prototype.drilldown = function() {
//     if (this.dimension !== null) {
//     }
//     return this.url + "/schema";
// }
//

// Query.prototype.query = function() {
//     return this.url + "/query";
// }

// Query.prototype.getFieldNames = function() {
//     return this.schema.fields
//         .filter(function (f) { return f.type.indexOf("nc_dim") == 0;})
//         .map(function (f) { return f.name; });
// }

// var nanocube = new Nanocube({url:"http://localhost:29512"});

// var query = nanocube
//     .query()
//     .dim("src")
//     .drilldown()
//     .findAndDive(0,8);

// .drilldown()
// .findAndDive(0,8)
// .dim("time")
// .drilldown()
// .tseries(0,24,7);


// nanocube2 = new Nanocube("http://hadoop48.research.att.com");


// fb = "http://graph.facebook.com/lauro.lins"
// nc = "http://localhost:29512/query"

// var obj = { levels:[  ], root:{ addr:0, value:1272000.000000 } };

// // x = JSON.parse("{ levels:[] }");

// $(document).ready(function() {

//     console.log(nanocube1.getSchemaQuery() + "<br>");
//     console.log(nanocube2.getSchemaQuery() + "<br>");
//     console.log(nanocube1.query()          + "<br>");

//     console.log("querying: " + nanocube1.query() + "<br>");

//     $.getJSON(nc, function( json ) {

//         // console.log(JSON.stringify(json));
//         // console.log("done");

//         //  var items = [];
//         // $.each( json, function( key, val ) {
//         //     items.push( "<li id='" + key + "'>" + val + "</li>" );
//         // });
//         // $( "<ul/>", { "class": "my-new-list", html: items.join( "" )}).appendTo( "body" );
//         $("body").append(JSON.stringify(json));
//     })
//     .fail(function (jqXHR, textStatus, errorThrown) {
//         // console.log("failed on query: " + nc);
//         console.log("fail <br>");
//         console.log("text status:   [" + textStatus + "]<br>");
//         console.log("error thrown:  [" + errorThrown + "]<br>");
//         console.log("response text: [" + jqXHR.responseText + "]<br>");
//     })
//     .done(function (jqXHR, textStatus) {
//         // console.log("failed on query: " + nc);
//         console.log( "done" );
//         // console.log(jqXHR.responseText);
//     })
//     .always(function() {
//         console.log( "always" );
//     });


//     var canvas = d3.select("body")
//         .append("svg")
//         .attr("width",500)
//         .attr("height",300);

//     // console.log(JSON.stringify(canvas.attr("width")));

//     var data = [
//         {name:"lauro", value:10}, 
//         {name:"sofia", value:130},
//         {name:"maria", value:20}, 
//         {name:"isis",  value:50}
//     ];


//     //----------------------------------------------------
//     // Get extents of a text box
//     //----------------------------------------------------

//     // function getDefaultFontSize(text, pa){
//     //     pa = pa || document.body;
//     //     var who= document.createElement('div');
//     //     who.style.cssText='display:inline-block; padding:0; line-height:1; position:absolute; visibility:hidden; font-size:1em';
//     //     who.appendChild(document.createTextNode(text));
//     //     pa.appendChild(who);
//     //     var fs= [who.offsetWidth, who.offsetHeight];
//     //     pa.removeChild(who);
//     //     return fs;
//     // }

//     //----------------------------------------------------
//     // Point
//     //----------------------------------------------------

//     function Point(x, y) {
//         if (x === undefined) {
//             this.x = 0;
//             this.x = x;
//         }
//         else {
//             this.x = x;
//         }
//         if (y === undefined) {
//             this.y = this.x;
//         }
//         else {
//             this.y = y;
//         }
//     }

//     Point.prototype.copy = function() {
//         return new Point(this.x, this.y);
//     }

//     Point.prototype.sum = function(p) {
//         this.x += p.x;
//         this.y += p.y;
//         return this;
//     }

//     Point.prototype.sub = function(p) {
//         this.x -= p.x;
//         this.y -= p.y;
//         return this;
//     }


//     //----------------------------------------------------
//     // Plot
//     //----------------------------------------------------

//     function Plot(canvas) {

//         console.log(JSON.stringify(canvas.width));

//         this.x      = 0;
//         this.y      = 0;
//         this.width  = 0 + canvas.attr("width");
//         this.height = 0 + canvas.attr("height");
//         this.canvas = canvas;

//         // d stands for data and s for screen (data and screen coordinates)
//         this.d2s    = function (pd) { return new Point(pd.x, pd.y); }
//         this.s2d    = function (ps) { return new Point(ps.x, ps.y); }
//     };



//     Plot.prototype.setDataRange = function(data) {

//         // map values from data into a vector
//         var values = $.map(data, function (e) { return e.value; });
//         var values_extent = d3.extent(values);

//         var x0d = 0; // values_extent[0];
//         var x1d = values_extent[1] * 1.1;

//         // number of bars define the y range
//         // [0,n]
//         var y0d = 0
//         var y1d = data.length
        
//         this.d2s = function(pd) { 
//             return new Point(
//                 this.x + (pd.x-x0d)/(x1d-x0d) * this.width,
//                 this.y + (pd.y-y0d)/(y1d-y0d) * this.height
//             );  
//         }
//         this.s2w = function(ps) { 
//             return new Point(
//                 x0d + (ps.x-this.x)/(this.width)  * (x1d - x0d),
//                 y0d + (ps.y-this.y)/(this.height) * (y1d - y0d)
//             );  
//         }
//         return this;
//     };



//     Plot.prototype.render = function(data) {

//         var plot = this;

//         var screen_points = $.map(data, function(d,i) { 
//             var p0s = plot.d2s(new Point(0, i + 0.1));
//             var p1s = plot.d2s(new Point(d.value, i + 0.9));
//             return { x: p0s.x, y: p0s.y, w: (p1s.x - p0s.x), h: (p1s.y - p0s.y), name: d.name, value: d.value }
//         });

//         console.log(JSON.stringify(screen_points));

//         this.canvas.append("rect")
//             .attr("width",this.width)
//             .attr("height",this.height)
//             .attr("fill","#888");

//         this.canvas.selectAll("bar")
//               .data(screen_points)
//               .enter()
//               .append("rect")
//               .attr("x", function(d) { return d.x } )
//               .attr("y", function(d) { return d.y } )
//               .attr("width",  function(d) { return d.w } )
//               .attr("height", function(d) { return d.h } )
//               .attr("fill", "#222277" );

//         this.canvas.selectAll("texts")
//               .data(screen_points)
//               .enter()
//               .append("text")
//               .attr("x", function(d) { return d.x + 4} )
//               .attr("y", function(d) { return d.y + d.h/2.0 } )
//               .attr("fill", "#FFFFFF" )
//               .text(function(d) { return d.name + " " + d.value } );


//         // canvas.selectAll("bar")
//         //       .data(screen_points)
//         //       .enter()
//         //       .append("rect").
//         //       .attr("x", function(d) { d.x } )
//         //       .attr("y", function(d) { d.y } )
//         //       .attr("width",  function(d) { d.w } )
//         //       .attr("height", function(d) { d.h } );
//         // );
//         // // assuming data in the format [{name: <aname>, value: <avalue>}]
//         // $.each(data, function(v, i) {
//         // });
//     }

//     //----------------------------------------------------
//     // Prepare visualization
//     //----------------------------------------------------

//     var plot = new Plot(canvas);

//     plot.setDataRange(data).render(data);

//     // var values = $.map(data,function(e) { return e.value; });
//     // var extents = d3.extents(values)
//     // d3.extent()
    

// });


