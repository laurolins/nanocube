function sinh(x){
    return (Math.exp(x) - Math.exp(-x)) / 2.0;
}

function sec(x){
    return 1.0/Math.cos(x);
}

function tile_to_degree(p,z,flip_y){
    //Ref: http://wiki.openstreetmap.org/wiki/Slippy_map_tilenames
    //"flipped" for the nanocubes system
    var zoom = z;
    var xtile = p.x+0.5;
    var ytile = p.y+0.5;
    if (flip_y){
        ytile = n-1-ytile;
    }

    var n = Math.pow(2,z);
    var lng = xtile / n * 360.0 - 180.0;
    var lat_rad = Math.atan(sinh(Math.PI * (1 - 2 * ytile / n)));
    var lat = lat_rad * 180.0 / Math.PI;

    return {lat:lat, lng:lng};
}

function degree_to_tile(latlng,z,flip_y){
    //Ref: http://wiki.openstreetmap.org/wiki/Slippy_map_tilenames
    //"flipped" for the nanocubes system


    //clip lat lng to boundaries
    var maxlng = 180-1e-6;
    var maxlat = 85.0511;
    latlng.lng = Math.max(-maxlng, latlng.lng);
    latlng.lng = Math.min(maxlng, latlng.lng);
    latlng.lat = Math.max(-maxlat, latlng.lat);
    latlng.lat = Math.min(maxlat, latlng.lat);

    //conversion
    var lon_deg = latlng.lng;
    var lat_rad = latlng.lat / 180 * Math.PI;

    var n = Math.pow(2,z);
    var xtile = n* ((lon_deg + 180) / 360);
    var ytile = n* (1-(Math.log(Math.tan(lat_rad)+sec(lat_rad))/Math.PI))/2;
    xtile = Math.floor(xtile);
    ytile = Math.floor(ytile);

    if (flip_y){
        ytile = n-1-ytile;
    }
    
    //fix negative tiles
    while(xtile < 0){
        xtile += n;
    }
    
    while(ytile < 0){
        ytile += n;
    }
    
    //fix overflow tiles
    while(xtile > n){
        xtile -= n;
    }
    
    while(ytile > n){
        ytile -= n;
    }

    return {x:xtile, y:ytile};
}


function genTileList(coords, zoom){
    return coords.map(function(d){
        var txy = degree_to_tile(d,zoom,true);
        
        var ntiles = Math.pow(2,zoom);

        return new Tile(txy.x,txy.y,zoom);
    });
};

function boundsToTileList(b,zoom){
    var x0 = b.getNorthEast().lng;
    var y0 = b.getNorthEast().lat;
    var x1 = b.getSouthWest().lng;
    var y1 = b.getSouthWest().lat;

    return genTileList([L.latLng(y0,x0),L.latLng(y0,x1),
                        L.latLng(y1,x1),L.latLng(y1,x0)],
                       zoom);
}

function bboxGeoJSON(geojson){
    var coords = geojson.geometry.coordinates[0];
    var bbmin = coords.reduce(function(prev,curr){
       return [Math.min(prev[0],curr[0]),Math.min(prev[1],curr[1])];
    },[Infinity,Infinity]);

    var bbmax = coords.reduce(function(prev,curr){
       return [Math.max(prev[0],curr[0]),Math.max(prev[1],curr[1])];
    },[-Infinity,-Infinity]);
    
    return [bbmin,bbmax];
}
