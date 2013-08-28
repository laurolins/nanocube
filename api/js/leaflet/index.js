var map = L.map('map', {
    center: [36.505, -90.09],
    zoom: 3
});

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

// add an OpenStreetMap tile layer
L.tileLayer('http://{s}.tile.osm.org/{z}/{x}/{y}.png', {
    attribution: '&copy; <a href="http://osm.org/copyright">OpenStreetMap</a> contributors'
}).addTo(map);

var canvas = L.tileLayer.canvas();

function colormap(count) {
    var lc = Math.log(count + 1) / Math.log(10),
        u, s;
    if (lc < 1) {
        u = ~~(lc * 255);
        s = "rgba(" + u + ",0,0," + 1 + ")";
    } else if (lc < 2) {
        u = ~~((lc - 1) * 255);
        s = "rgba(255," + u + ",0,1)";
    } else if (lc < 3) {
        s = "yellow";
        u = ~~((lc - 2) * 255);
    } else
        s = "white";
    return s;
}

canvas.drawTile = function(tile, tilePoint) {
    var x = tilePoint.x;
    var y = tilePoint.y;
    var z = this._map._zoom;
    while (x < 0) x += 1 << z;
    while (y < 0) y += 1 << z;
    x = x % (1 << z);
    y = y % (1 << z);
    var resolution = 6;
    var url = 'http://192.168.1.10:29512/tile/' + z + '/' + resolution + '/' + x + '/' + ((1 << z) - 1 - y) + '/0/1';
    
    binary_xhr(url, function(data) {
        if (data === null)
            return;
        var view = new DataView(data);
        var c = tile.getContext("2d");
        for (var i=0; i<data.byteLength/10; ++i) {
            var px = view.getUint8(10*i+1) << (8 - resolution),
                py = 256 - (1 << (8 - resolution)) - (view.getUint8(10*i) << (8 - resolution)),
                count = view.getFloat64(10*i+2, true);
            if (count === 0)
                continue;
            c.fillStyle = colormap(count);
            c.fillRect(px, py, 1 << (8 - resolution), 1 << (8 - resolution));
        }
    });
};
canvas.addTo(map);
