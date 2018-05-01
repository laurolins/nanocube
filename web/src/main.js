/*global $ define require */

require.config({
    baseUrl: '.',
    paths: {
        leaflet: 'lib/leaflet/leaflet',
        leafletdraw: 'lib/leaflet-draw/leaflet.draw',
        jquery: 'lib/jquery/jquery.min',
        d3: 'lib/d3/d3.min',
        interact: 'lib/interact/interact.min',
        colorbrewer: 'lib/colorbrewer/colorbrewer',
        jsep: 'lib/jsep/jsep.min',
        canvaslayer: 'lib/canvaslayer/L.CanvasLayer',
        sprintf: 'lib/sprintf/sprintf.min',
        'resize-drag': 'lib/resize-drag/resize-drag',
        nanocube: 'dist/Nanocube'
    },

    shim: {
        canvaslayer: {
            deps: ['leaflet'],
            exports: 'L'
        },
        leafletdraw: {
            deps: ['leaflet'],
            exports: 'L'
        },
        colorbrewer: {
            exports: 'colorbrewer'
        },
        jsep: {
            exports: 'jsep'
        }
    }
});

//  https://stackoverflow.com/questions/8486099/how-do-i-parse-a-url-query-parameters-in-javascript
function getArgFromUrl() {
    var query = window.location.search.substr(1);
    var result = {};
    query.split("&").forEach(function(part) {
        var item = part.split("=");
        if(item[0] !== ""){
            result[item[0]] = decodeURIComponent(item[1]);
        }
    });
        
    return result;
}

var viewer ;
define(['jquery','nanocube','colorbrewer','resize-drag'],
       function($,Nanocube3,colorbrewer){
    'use strict';
    //read config and pass it to start viewer

    var urlargs = getArgFromUrl();
    urlargs.config = urlargs.config || './config.json';

    $.getJSON(urlargs.config).done(function(config){
        startViewer(config,urlargs);
    }).fail(function(err){
        console.log("failed to parse json"+ err);
    });
    
    function startViewer(config,urlargs){
        $.when.apply($, Object.keys(config.nanocube).map(function(k){
            return Nanocube3.Nanocube.initNanocube(config.nanocube[k].url);
        })).done(function(){
            var nchash = {};
            var ncnames = Object.keys(config.nanocube);
            var args = Array.prototype.slice.call(arguments);
            args.forEach(function(d,i){
                nchash[ncnames[i]] = d;
            });
            viewer = new Nanocube3.Viewer({
                nanocubes: nchash,
                div_id:'#nc',
                config: config,
                urlargs: urlargs
            });
            viewer.update();
        }).fail(function(){
            console.log('failed to parse' + urlargs.config);
        });
    }
});
