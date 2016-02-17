/*global define module exports require */

function loadCss(url) {
    var link = document.createElement("link");
    link.type = "text/css";
    link.rel = "stylesheet";
    link.href = url;
    document.getElementsByTagName("head")[0].appendChild(link);
}

(function(root, factory) {    
    if (typeof define === 'function' && define.amd) {
	// AMD. Register as an anonymous module.
	define(['jquery','colorbrewer','d3',
		'jsep','leafletdraw','canvaslayer'], factory);
    } else if (typeof exports === 'object') {
	// Node. Does not work with strict CommonJS, but
	// only CommonJS-like environments that support module.exports,
	// like Node.
	module.exports = factory(require('jquery'),
				 require('colorbrewer'),
				 require('d3'),
				 require('jsep'),
				 require('leaflet'),
				 require('leafletdraw'),
				 require('canvaslayer'));
    } else {
	// Browser globals (root is window)
	root.Nanocube3 = factory(root.$,root.colorbrewer,root.d3,
				 root.jsep,root.L);
    }
} (this, function($,colorbrewer,d3,jsep,L) {
    loadCss('node_modules/leaflet/dist/leaflet.css');
    loadCss('node_modules/leaflet-draw/dist/leaflet.draw.css');

    var Nanocube3 = {};
