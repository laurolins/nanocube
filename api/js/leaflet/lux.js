/*
 * Lux: An EDSL for WebGL graphics
 * By Carlos Scheidegger, cscheid@research.att.com
 * 
 * Copyright (c) 2011-2013 AT&T Intellectual Property
 * 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: See github logs.
 *
 */

// Lux depends, at least partially, on the following software libraries:
// - underscore.js
// - webgl-debug
// - webgl-utils
// - typeface.js
// attribution notices and licenses for these libraries follow.

////////////////////////////////////////////////////////////////////////////////
// BEGIN UNDERSCORE.JS NOTICE
// 
// Underscore.js 1.1.7
// (c) 2011 Jeremy Ashkenas, DocumentCloud Inc.
// Underscore is freely distributable under the MIT license.
// Portions of Underscore are inspired or borrowed from Prototype,
// Oliver Steele's Functional, and John Resig's Micro-Templating.
// For all details and documentation:
// http://documentcloud.github.com/underscore
//
// END UNDERSCORE.JS NOTICE
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// BEGIN WEBGL-DEBUG.JS NOTICE
// https://cvs.khronos.org/svn/repos/registry/trunk/public/webgl/sdk/debug/webgl-debug.js
//
// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// END WEBGL-DEBUG.JS NOTICE
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// BEGIN WEBGL-UTILS.JS NOTICE
// https://cvs.khronos.org/svn/repos/registry/trunk/public/webgl/sdk/demos/common/webgl-utils.js
/*
 * Copyright 2010, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
// END WEBGL-UTILS.JS NOTICE
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// BEGIN TYPEFACE.JS NOTICE
/*
typeface.js, version 0.15 | typefacejs.neocracy.org

Copyright (c) 2008 - 2009, David Chester davidchester@gmx.net 

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
 */
// END TYPEFACE.JS NOTICE
////////////////////////////////////////////////////////////////////////////////
Lux = {};
// yucky globals used throughout Lux. I guess this means I lost.
//
////////////////////////////////////////////////////////////////////////////////

Lux._globals = {
    // stores the active webgl context
    ctx: undefined

    // In addition, Lux stores per-context globals inside the
    // WebGL context variable itself, on the field _lux_globals.
};
// stores references to external libraries to avoid namespace pollution

Lux.Lib = {};
(function(){var n=this,t=n._,r={},e=Array.prototype,u=Object.prototype,i=Function.prototype,a=e.push,o=e.slice,c=e.concat,l=u.toString,f=u.hasOwnProperty,s=e.forEach,p=e.map,h=e.reduce,v=e.reduceRight,d=e.filter,g=e.every,m=e.some,y=e.indexOf,b=e.lastIndexOf,x=Array.isArray,_=Object.keys,j=i.bind,w=function(n){return n instanceof w?n:this instanceof w?(this._wrapped=n,void 0):new w(n)};"undefined"!=typeof exports?("undefined"!=typeof module&&module.exports&&(exports=module.exports=w),exports._=w):n._=w,w.VERSION="1.4.4";var A=w.each=w.forEach=function(n,t,e){if(null!=n)if(s&&n.forEach===s)n.forEach(t,e);else if(n.length===+n.length){for(var u=0,i=n.length;i>u;u++)if(t.call(e,n[u],u,n)===r)return}else for(var a in n)if(w.has(n,a)&&t.call(e,n[a],a,n)===r)return};w.map=w.collect=function(n,t,r){var e=[];return null==n?e:p&&n.map===p?n.map(t,r):(A(n,function(n,u,i){e[e.length]=t.call(r,n,u,i)}),e)};var O="Reduce of empty array with no initial value";w.reduce=w.foldl=w.inject=function(n,t,r,e){var u=arguments.length>2;if(null==n&&(n=[]),h&&n.reduce===h)return e&&(t=w.bind(t,e)),u?n.reduce(t,r):n.reduce(t);if(A(n,function(n,i,a){u?r=t.call(e,r,n,i,a):(r=n,u=!0)}),!u)throw new TypeError(O);return r},w.reduceRight=w.foldr=function(n,t,r,e){var u=arguments.length>2;if(null==n&&(n=[]),v&&n.reduceRight===v)return e&&(t=w.bind(t,e)),u?n.reduceRight(t,r):n.reduceRight(t);var i=n.length;if(i!==+i){var a=w.keys(n);i=a.length}if(A(n,function(o,c,l){c=a?a[--i]:--i,u?r=t.call(e,r,n[c],c,l):(r=n[c],u=!0)}),!u)throw new TypeError(O);return r},w.find=w.detect=function(n,t,r){var e;return E(n,function(n,u,i){return t.call(r,n,u,i)?(e=n,!0):void 0}),e},w.filter=w.select=function(n,t,r){var e=[];return null==n?e:d&&n.filter===d?n.filter(t,r):(A(n,function(n,u,i){t.call(r,n,u,i)&&(e[e.length]=n)}),e)},w.reject=function(n,t,r){return w.filter(n,function(n,e,u){return!t.call(r,n,e,u)},r)},w.every=w.all=function(n,t,e){t||(t=w.identity);var u=!0;return null==n?u:g&&n.every===g?n.every(t,e):(A(n,function(n,i,a){return(u=u&&t.call(e,n,i,a))?void 0:r}),!!u)};var E=w.some=w.any=function(n,t,e){t||(t=w.identity);var u=!1;return null==n?u:m&&n.some===m?n.some(t,e):(A(n,function(n,i,a){return u||(u=t.call(e,n,i,a))?r:void 0}),!!u)};w.contains=w.include=function(n,t){return null==n?!1:y&&n.indexOf===y?n.indexOf(t)!=-1:E(n,function(n){return n===t})},w.invoke=function(n,t){var r=o.call(arguments,2),e=w.isFunction(t);return w.map(n,function(n){return(e?t:n[t]).apply(n,r)})},w.pluck=function(n,t){return w.map(n,function(n){return n[t]})},w.where=function(n,t,r){return w.isEmpty(t)?r?null:[]:w[r?"find":"filter"](n,function(n){for(var r in t)if(t[r]!==n[r])return!1;return!0})},w.findWhere=function(n,t){return w.where(n,t,!0)},w.max=function(n,t,r){if(!t&&w.isArray(n)&&n[0]===+n[0]&&65535>n.length)return Math.max.apply(Math,n);if(!t&&w.isEmpty(n))return-1/0;var e={computed:-1/0,value:-1/0};return A(n,function(n,u,i){var a=t?t.call(r,n,u,i):n;a>=e.computed&&(e={value:n,computed:a})}),e.value},w.min=function(n,t,r){if(!t&&w.isArray(n)&&n[0]===+n[0]&&65535>n.length)return Math.min.apply(Math,n);if(!t&&w.isEmpty(n))return 1/0;var e={computed:1/0,value:1/0};return A(n,function(n,u,i){var a=t?t.call(r,n,u,i):n;e.computed>a&&(e={value:n,computed:a})}),e.value},w.shuffle=function(n){var t,r=0,e=[];return A(n,function(n){t=w.random(r++),e[r-1]=e[t],e[t]=n}),e};var k=function(n){return w.isFunction(n)?n:function(t){return t[n]}};w.sortBy=function(n,t,r){var e=k(t);return w.pluck(w.map(n,function(n,t,u){return{value:n,index:t,criteria:e.call(r,n,t,u)}}).sort(function(n,t){var r=n.criteria,e=t.criteria;if(r!==e){if(r>e||r===void 0)return 1;if(e>r||e===void 0)return-1}return n.index<t.index?-1:1}),"value")};var F=function(n,t,r,e){var u={},i=k(t||w.identity);return A(n,function(t,a){var o=i.call(r,t,a,n);e(u,o,t)}),u};w.groupBy=function(n,t,r){return F(n,t,r,function(n,t,r){(w.has(n,t)?n[t]:n[t]=[]).push(r)})},w.countBy=function(n,t,r){return F(n,t,r,function(n,t){w.has(n,t)||(n[t]=0),n[t]++})},w.sortedIndex=function(n,t,r,e){r=null==r?w.identity:k(r);for(var u=r.call(e,t),i=0,a=n.length;a>i;){var o=i+a>>>1;u>r.call(e,n[o])?i=o+1:a=o}return i},w.toArray=function(n){return n?w.isArray(n)?o.call(n):n.length===+n.length?w.map(n,w.identity):w.values(n):[]},w.size=function(n){return null==n?0:n.length===+n.length?n.length:w.keys(n).length},w.first=w.head=w.take=function(n,t,r){return null==n?void 0:null==t||r?n[0]:o.call(n,0,t)},w.initial=function(n,t,r){return o.call(n,0,n.length-(null==t||r?1:t))},w.last=function(n,t,r){return null==n?void 0:null==t||r?n[n.length-1]:o.call(n,Math.max(n.length-t,0))},w.rest=w.tail=w.drop=function(n,t,r){return o.call(n,null==t||r?1:t)},w.compact=function(n){return w.filter(n,w.identity)};var R=function(n,t,r){return A(n,function(n){w.isArray(n)?t?a.apply(r,n):R(n,t,r):r.push(n)}),r};w.flatten=function(n,t){return R(n,t,[])},w.without=function(n){return w.difference(n,o.call(arguments,1))},w.uniq=w.unique=function(n,t,r,e){w.isFunction(t)&&(e=r,r=t,t=!1);var u=r?w.map(n,r,e):n,i=[],a=[];return A(u,function(r,e){(t?e&&a[a.length-1]===r:w.contains(a,r))||(a.push(r),i.push(n[e]))}),i},w.union=function(){return w.uniq(c.apply(e,arguments))},w.intersection=function(n){var t=o.call(arguments,1);return w.filter(w.uniq(n),function(n){return w.every(t,function(t){return w.indexOf(t,n)>=0})})},w.difference=function(n){var t=c.apply(e,o.call(arguments,1));return w.filter(n,function(n){return!w.contains(t,n)})},w.zip=function(){for(var n=o.call(arguments),t=w.max(w.pluck(n,"length")),r=Array(t),e=0;t>e;e++)r[e]=w.pluck(n,""+e);return r},w.object=function(n,t){if(null==n)return{};for(var r={},e=0,u=n.length;u>e;e++)t?r[n[e]]=t[e]:r[n[e][0]]=n[e][1];return r},w.indexOf=function(n,t,r){if(null==n)return-1;var e=0,u=n.length;if(r){if("number"!=typeof r)return e=w.sortedIndex(n,t),n[e]===t?e:-1;e=0>r?Math.max(0,u+r):r}if(y&&n.indexOf===y)return n.indexOf(t,r);for(;u>e;e++)if(n[e]===t)return e;return-1},w.lastIndexOf=function(n,t,r){if(null==n)return-1;var e=null!=r;if(b&&n.lastIndexOf===b)return e?n.lastIndexOf(t,r):n.lastIndexOf(t);for(var u=e?r:n.length;u--;)if(n[u]===t)return u;return-1},w.range=function(n,t,r){1>=arguments.length&&(t=n||0,n=0),r=arguments[2]||1;for(var e=Math.max(Math.ceil((t-n)/r),0),u=0,i=Array(e);e>u;)i[u++]=n,n+=r;return i},w.bind=function(n,t){if(n.bind===j&&j)return j.apply(n,o.call(arguments,1));var r=o.call(arguments,2);return function(){return n.apply(t,r.concat(o.call(arguments)))}},w.partial=function(n){var t=o.call(arguments,1);return function(){return n.apply(this,t.concat(o.call(arguments)))}},w.bindAll=function(n){var t=o.call(arguments,1);return 0===t.length&&(t=w.functions(n)),A(t,function(t){n[t]=w.bind(n[t],n)}),n},w.memoize=function(n,t){var r={};return t||(t=w.identity),function(){var e=t.apply(this,arguments);return w.has(r,e)?r[e]:r[e]=n.apply(this,arguments)}},w.delay=function(n,t){var r=o.call(arguments,2);return setTimeout(function(){return n.apply(null,r)},t)},w.defer=function(n){return w.delay.apply(w,[n,1].concat(o.call(arguments,1)))},w.throttle=function(n,t){var r,e,u,i,a=0,o=function(){a=new Date,u=null,i=n.apply(r,e)};return function(){var c=new Date,l=t-(c-a);return r=this,e=arguments,0>=l?(clearTimeout(u),u=null,a=c,i=n.apply(r,e)):u||(u=setTimeout(o,l)),i}},w.debounce=function(n,t,r){var e,u;return function(){var i=this,a=arguments,o=function(){e=null,r||(u=n.apply(i,a))},c=r&&!e;return clearTimeout(e),e=setTimeout(o,t),c&&(u=n.apply(i,a)),u}},w.once=function(n){var t,r=!1;return function(){return r?t:(r=!0,t=n.apply(this,arguments),n=null,t)}},w.wrap=function(n,t){return function(){var r=[n];return a.apply(r,arguments),t.apply(this,r)}},w.compose=function(){var n=arguments;return function(){for(var t=arguments,r=n.length-1;r>=0;r--)t=[n[r].apply(this,t)];return t[0]}},w.after=function(n,t){return 0>=n?t():function(){return 1>--n?t.apply(this,arguments):void 0}},w.keys=_||function(n){if(n!==Object(n))throw new TypeError("Invalid object");var t=[];for(var r in n)w.has(n,r)&&(t[t.length]=r);return t},w.values=function(n){var t=[];for(var r in n)w.has(n,r)&&t.push(n[r]);return t},w.pairs=function(n){var t=[];for(var r in n)w.has(n,r)&&t.push([r,n[r]]);return t},w.invert=function(n){var t={};for(var r in n)w.has(n,r)&&(t[n[r]]=r);return t},w.functions=w.methods=function(n){var t=[];for(var r in n)w.isFunction(n[r])&&t.push(r);return t.sort()},w.extend=function(n){return A(o.call(arguments,1),function(t){if(t)for(var r in t)n[r]=t[r]}),n},w.pick=function(n){var t={},r=c.apply(e,o.call(arguments,1));return A(r,function(r){r in n&&(t[r]=n[r])}),t},w.omit=function(n){var t={},r=c.apply(e,o.call(arguments,1));for(var u in n)w.contains(r,u)||(t[u]=n[u]);return t},w.defaults=function(n){return A(o.call(arguments,1),function(t){if(t)for(var r in t)null==n[r]&&(n[r]=t[r])}),n},w.clone=function(n){return w.isObject(n)?w.isArray(n)?n.slice():w.extend({},n):n},w.tap=function(n,t){return t(n),n};var I=function(n,t,r,e){if(n===t)return 0!==n||1/n==1/t;if(null==n||null==t)return n===t;n instanceof w&&(n=n._wrapped),t instanceof w&&(t=t._wrapped);var u=l.call(n);if(u!=l.call(t))return!1;switch(u){case"[object String]":return n==t+"";case"[object Number]":return n!=+n?t!=+t:0==n?1/n==1/t:n==+t;case"[object Date]":case"[object Boolean]":return+n==+t;case"[object RegExp]":return n.source==t.source&&n.global==t.global&&n.multiline==t.multiline&&n.ignoreCase==t.ignoreCase}if("object"!=typeof n||"object"!=typeof t)return!1;for(var i=r.length;i--;)if(r[i]==n)return e[i]==t;r.push(n),e.push(t);var a=0,o=!0;if("[object Array]"==u){if(a=n.length,o=a==t.length)for(;a--&&(o=I(n[a],t[a],r,e)););}else{var c=n.constructor,f=t.constructor;if(c!==f&&!(w.isFunction(c)&&c instanceof c&&w.isFunction(f)&&f instanceof f))return!1;for(var s in n)if(w.has(n,s)&&(a++,!(o=w.has(t,s)&&I(n[s],t[s],r,e))))break;if(o){for(s in t)if(w.has(t,s)&&!a--)break;o=!a}}return r.pop(),e.pop(),o};w.isEqual=function(n,t){return I(n,t,[],[])},w.isEmpty=function(n){if(null==n)return!0;if(w.isArray(n)||w.isString(n))return 0===n.length;for(var t in n)if(w.has(n,t))return!1;return!0},w.isElement=function(n){return!(!n||1!==n.nodeType)},w.isArray=x||function(n){return"[object Array]"==l.call(n)},w.isObject=function(n){return n===Object(n)},A(["Arguments","Function","String","Number","Date","RegExp"],function(n){w["is"+n]=function(t){return l.call(t)=="[object "+n+"]"}}),w.isArguments(arguments)||(w.isArguments=function(n){return!(!n||!w.has(n,"callee"))}),"function"!=typeof/./&&(w.isFunction=function(n){return"function"==typeof n}),w.isFinite=function(n){return isFinite(n)&&!isNaN(parseFloat(n))},w.isNaN=function(n){return w.isNumber(n)&&n!=+n},w.isBoolean=function(n){return n===!0||n===!1||"[object Boolean]"==l.call(n)},w.isNull=function(n){return null===n},w.isUndefined=function(n){return n===void 0},w.has=function(n,t){return f.call(n,t)},w.noConflict=function(){return n._=t,this},w.identity=function(n){return n},w.times=function(n,t,r){for(var e=Array(n),u=0;n>u;u++)e[u]=t.call(r,u);return e},w.random=function(n,t){return null==t&&(t=n,n=0),n+Math.floor(Math.random()*(t-n+1))};var M={escape:{"&":"&amp;","<":"&lt;",">":"&gt;",'"':"&quot;","'":"&#x27;","/":"&#x2F;"}};M.unescape=w.invert(M.escape);var S={escape:RegExp("["+w.keys(M.escape).join("")+"]","g"),unescape:RegExp("("+w.keys(M.unescape).join("|")+")","g")};w.each(["escape","unescape"],function(n){w[n]=function(t){return null==t?"":(""+t).replace(S[n],function(t){return M[n][t]})}}),w.result=function(n,t){if(null==n)return null;var r=n[t];return w.isFunction(r)?r.call(n):r},w.mixin=function(n){A(w.functions(n),function(t){var r=w[t]=n[t];w.prototype[t]=function(){var n=[this._wrapped];return a.apply(n,arguments),D.call(this,r.apply(w,n))}})};var N=0;w.uniqueId=function(n){var t=++N+"";return n?n+t:t},w.templateSettings={evaluate:/<%([\s\S]+?)%>/g,interpolate:/<%=([\s\S]+?)%>/g,escape:/<%-([\s\S]+?)%>/g};var T=/(.)^/,q={"'":"'","\\":"\\","\r":"r","\n":"n","	":"t","\u2028":"u2028","\u2029":"u2029"},B=/\\|'|\r|\n|\t|\u2028|\u2029/g;w.template=function(n,t,r){var e;r=w.defaults({},r,w.templateSettings);var u=RegExp([(r.escape||T).source,(r.interpolate||T).source,(r.evaluate||T).source].join("|")+"|$","g"),i=0,a="__p+='";n.replace(u,function(t,r,e,u,o){return a+=n.slice(i,o).replace(B,function(n){return"\\"+q[n]}),r&&(a+="'+\n((__t=("+r+"))==null?'':_.escape(__t))+\n'"),e&&(a+="'+\n((__t=("+e+"))==null?'':__t)+\n'"),u&&(a+="';\n"+u+"\n__p+='"),i=o+t.length,t}),a+="';\n",r.variable||(a="with(obj||{}){\n"+a+"}\n"),a="var __t,__p='',__j=Array.prototype.join,"+"print=function(){__p+=__j.call(arguments,'');};\n"+a+"return __p;\n";try{e=Function(r.variable||"obj","_",a)}catch(o){throw o.source=a,o}if(t)return e(t,w);var c=function(n){return e.call(this,n,w)};return c.source="function("+(r.variable||"obj")+"){\n"+a+"}",c},w.chain=function(n){return w(n).chain()};var D=function(n){return this._chain?w(n).chain():n};w.mixin(w),A(["pop","push","reverse","shift","sort","splice","unshift"],function(n){var t=e[n];w.prototype[n]=function(){var r=this._wrapped;return t.apply(r,arguments),"shift"!=n&&"splice"!=n||0!==r.length||delete r[0],D.call(this,r)}}),A(["concat","join","slice"],function(n){var t=e[n];w.prototype[n]=function(){return D.call(this,t.apply(this._wrapped,arguments))}}),w.extend(w.prototype,{chain:function(){return this._chain=!0,this},value:function(){return this._wrapped}})}).call(this);//Copyright (c) 2009 The Chromium Authors. All rights reserved.
//Use of this source code is governed by a BSD-style license that can be
//found in the LICENSE file.

// Various functions for helping debug WebGL apps.

WebGLDebugUtils = function() {

/**
 * Wrapped logging function.
 * @param {string} msg Message to log.
 */
var log = function(msg) {
  if (window.console && window.console.log) {
    window.console.log(msg);
  }
};

/**
 * Which arguements are enums.
 * @type {!Object.<number, string>}
 */
var glValidEnumContexts = {

  // Generic setters and getters

  'enable': { 0:true },
  'disable': { 0:true },
  'getParameter': { 0:true },

  // Rendering

  'drawArrays': { 0:true },
  'drawElements': { 0:true, 2:true },

  // Shaders

  'createShader': { 0:true },
  'getShaderParameter': { 1:true },
  'getProgramParameter': { 1:true },

  // Vertex attributes

  'getVertexAttrib': { 1:true },
  'vertexAttribPointer': { 2:true },

  // Textures

  'bindTexture': { 0:true },
  'activeTexture': { 0:true },
  'getTexParameter': { 0:true, 1:true },
  'texParameterf': { 0:true, 1:true },
  'texParameteri': { 0:true, 1:true, 2:true },
  'texImage2D': { 0:true, 2:true, 6:true, 7:true },
  'texSubImage2D': { 0:true, 6:true, 7:true },
  'copyTexImage2D': { 0:true, 2:true },
  'copyTexSubImage2D': { 0:true },
  'generateMipmap': { 0:true },

  // Buffer objects

  'bindBuffer': { 0:true },
  'bufferData': { 0:true, 2:true },
  'bufferSubData': { 0:true },
  'getBufferParameter': { 0:true, 1:true },

  // Renderbuffers and framebuffers

  'pixelStorei': { 0:true, 1:true },
  'readPixels': { 4:true, 5:true },
  'bindRenderbuffer': { 0:true },
  'bindFramebuffer': { 0:true },
  'checkFramebufferStatus': { 0:true },
  'framebufferRenderbuffer': { 0:true, 1:true, 2:true },
  'framebufferTexture2D': { 0:true, 1:true, 2:true },
  'getFramebufferAttachmentParameter': { 0:true, 1:true, 2:true },
  'getRenderbufferParameter': { 0:true, 1:true },
  'renderbufferStorage': { 0:true, 1:true },

  // Frame buffer operations (clear, blend, depth test, stencil)

  'clear': { 0:true },
  'depthFunc': { 0:true },
  'blendFunc': { 0:true, 1:true },
  'blendFuncSeparate': { 0:true, 1:true, 2:true, 3:true },
  'blendEquation': { 0:true },
  'blendEquationSeparate': { 0:true, 1:true },
  'stencilFunc': { 0:true },
  'stencilFuncSeparate': { 0:true, 1:true },
  'stencilMaskSeparate': { 0:true },
  'stencilOp': { 0:true, 1:true, 2:true },
  'stencilOpSeparate': { 0:true, 1:true, 2:true, 3:true },

  // Culling

  'cullFace': { 0:true },
  'frontFace': { 0:true }
};

/**
 * Map of numbers to names.
 * @type {Object}
 */
var glEnums = null;

/**
 * Initializes this module. Safe to call more than once.
 * @param {!WebGLRenderingContext} ctx A WebGL context. If
 *    you have more than one context it doesn't matter which one
 *    you pass in, it is only used to pull out constants.
 */
function init(ctx) {
  if (glEnums == null) {
    glEnums = { };
    for (var propertyName in ctx) {
      if (typeof ctx[propertyName] == 'number') {
        glEnums[ctx[propertyName]] = propertyName;
      }
    }
  }
}

/**
 * Checks the utils have been initialized.
 */
function checkInit() {
  if (glEnums == null) {
    throw 'WebGLDebugUtils.init(ctx) not called';
  }
}

/**
 * Returns true or false if value matches any WebGL enum
 * @param {*} value Value to check if it might be an enum.
 * @return {boolean} True if value matches one of the WebGL defined enums
 */
function mightBeEnum(value) {
  checkInit();
  return (glEnums[value] !== undefined);
}

/**
 * Gets an string version of an WebGL enum.
 *
 * Example:
 *   var str = WebGLDebugUtil.glEnumToString(ctx.getError());
 *
 * @param {number} value Value to return an enum for
 * @return {string} The string version of the enum.
 */
function glEnumToString(value) {
  checkInit();
  var name = glEnums[value];
  return (name !== undefined) ? name :
      ("*UNKNOWN WebGL ENUM (0x" + value.toString(16) + ")");
}

/**
 * Returns the string version of a WebGL argument.
 * Attempts to convert enum arguments to strings.
 * @param {string} functionName the name of the WebGL function.
 * @param {number} argumentIndx the index of the argument.
 * @param {*} value The value of the argument.
 * @return {string} The value as a string.
 */
function glFunctionArgToString(functionName, argumentIndex, value) {
  var funcInfo = glValidEnumContexts[functionName];
  if (funcInfo !== undefined) {
    if (funcInfo[argumentIndex]) {
      return glEnumToString(value);
    }
  }
  return value.toString();
}

/**
 * Given a WebGL context returns a wrapped context that calls
 * gl.getError after every command and calls a function if the
 * result is not gl.NO_ERROR.
 *
 * @param {!WebGLRenderingContext} ctx The webgl context to
 *        wrap.
 * @param {!function(err, funcName, args): void} opt_onErrorFunc
 *        The function to call when gl.getError returns an
 *        error. If not specified the default function calls
 *        console.log with a message.
 */
function makeDebugContext(ctx, opt_onErrorFunc) {
  init(ctx);
  opt_onErrorFunc = opt_onErrorFunc || function(err, functionName, args) {
        // apparently we can't do args.join(",");
        var argStr = "";
        for (var ii = 0; ii < args.length; ++ii) {
          argStr += ((ii == 0) ? '' : ', ') +
              glFunctionArgToString(functionName, ii, args[ii]);
        }
        log("WebGL error "+ glEnumToString(err) + " in "+ functionName +
            "(" + argStr + ")");
      };

  // Holds booleans for each GL error so after we get the error ourselves
  // we can still return it to the client app.
  var glErrorShadow = { };

  // Makes a function that calls a WebGL function and then calls getError.
  function makeErrorWrapper(ctx, functionName) {
    return function() {
      var result = ctx[functionName].apply(ctx, arguments);
      var err = ctx.getError();
      if (err != 0) {
        glErrorShadow[err] = true;
        opt_onErrorFunc(err, functionName, arguments);
      }
      return result;
    };
  }

  // Make a an object that has a copy of every property of the WebGL context
  // but wraps all functions.
  var wrapper = {};
  for (var propertyName in ctx) {
    if (typeof ctx[propertyName] == 'function') {
       wrapper[propertyName] = makeErrorWrapper(ctx, propertyName);
     } else {
       wrapper[propertyName] = ctx[propertyName];
     }
  }

  // Override the getError function with one that returns our saved results.
  wrapper.getError = function() {
    for (var err in glErrorShadow) {
      if (glErrorShadow[err]) {
        glErrorShadow[err] = false;
        return err;
      }
    }
    return ctx.NO_ERROR;
  };

  return wrapper;
}

function resetToInitialState(ctx) {
  var numAttribs = ctx.getParameter(ctx.MAX_VERTEX_ATTRIBS);
  var tmp = ctx.createBuffer();
  ctx.bindBuffer(ctx.ARRAY_BUFFER, tmp);
  for (var ii = 0; ii < numAttribs; ++ii) {
    ctx.disableVertexAttribArray(ii);
    ctx.vertexAttribPointer(ii, 4, ctx.FLOAT, false, 0, 0);
    ctx.vertexAttrib1f(ii, 0);
  }
  ctx.deleteBuffer(tmp);

  var numTextureUnits = ctx.getParameter(ctx.MAX_TEXTURE_IMAGE_UNITS);
  for (var ii = 0; ii < numTextureUnits; ++ii) {
    ctx.activeTexture(ctx.TEXTURE0 + ii);
    ctx.bindTexture(ctx.TEXTURE_CUBE_MAP, null);
    ctx.bindTexture(ctx.TEXTURE_2D, null);
  }

  ctx.activeTexture(ctx.TEXTURE0);
  ctx.useProgram(null);
  ctx.bindBuffer(ctx.ARRAY_BUFFER, null);
  ctx.bindBuffer(ctx.ELEMENT_ARRAY_BUFFER, null);
  ctx.bindFramebuffer(ctx.FRAMEBUFFER, null);
  ctx.bindRenderbuffer(ctx.RENDERBUFFER, null);
  ctx.disable(ctx.BLEND);
  ctx.disable(ctx.CULL_FACE);
  ctx.disable(ctx.DEPTH_TEST);
  ctx.disable(ctx.DITHER);
  ctx.disable(ctx.SCISSOR_TEST);
  ctx.blendColor(0, 0, 0, 0);
  ctx.blendEquation(ctx.FUNC_ADD);
  ctx.blendFunc(ctx.ONE, ctx.ZERO);
  ctx.clearColor(0, 0, 0, 0);
  ctx.clearDepth(1);
  ctx.clearStencil(-1);
  ctx.colorMask(true, true, true, true);
  ctx.cullFace(ctx.BACK);
  ctx.depthFunc(ctx.LESS);
  ctx.depthMask(true);
  ctx.depthRange(0, 1);
  ctx.frontFace(ctx.CCW);
  ctx.hint(ctx.GENERATE_MIPMAP_HINT, ctx.DONT_CARE);
  ctx.lineWidth(1);
  ctx.pixelStorei(ctx.PACK_ALIGNMENT, 4);
  ctx.pixelStorei(ctx.UNPACK_ALIGNMENT, 4);
  ctx.pixelStorei(ctx.UNPACK_FLIP_Y_WEBGL, false);
  ctx.pixelStorei(ctx.UNPACK_PREMULTIPLY_ALPHA_WEBGL, false);
  // TODO: Delete this IF.
  if (ctx.UNPACK_COLORSPACE_CONVERSION_WEBGL) {
    ctx.pixelStorei(ctx.UNPACK_COLORSPACE_CONVERSION_WEBGL, ctx.BROWSER_DEFAULT_WEBGL);
  }
  ctx.polygonOffset(0, 0);
  ctx.sampleCoverage(1, false);
  ctx.scissor(0, 0, ctx.canvas.width, ctx.canvas.height);
  ctx.stencilFunc(ctx.ALWAYS, 0, 0xFFFFFFFF);
  ctx.stencilMask(0xFFFFFFFF);
  ctx.stencilOp(ctx.KEEP, ctx.KEEP, ctx.KEEP);
  ctx.viewport(0, 0, ctx.canvas.clientWidth, ctx.canvas.clientHeight);
  ctx.clear(ctx.COLOR_BUFFER_BIT | ctx.DEPTH_BUFFER_BIT | ctx.STENCIL_BUFFER_BIT);

  // TODO: This should NOT be needed but Firefox fails with 'hint'
  while(ctx.getError());
}

function makeLostContextSimulatingContext(ctx) {
  var wrapper_ = {};
  var contextId_ = 1;
  var contextLost_ = false;
  var resourceId_ = 0;
  var resourceDb_ = [];
  var onLost_ = undefined;
  var onRestored_ = undefined;
  var nextOnRestored_ = undefined;

  // Holds booleans for each GL error so can simulate errors.
  var glErrorShadow_ = { };

  function isWebGLObject(obj) {
    //return false;
    return (obj instanceof WebGLBuffer ||
            obj instanceof WebGLFramebuffer ||
            obj instanceof WebGLProgram ||
            obj instanceof WebGLRenderbuffer ||
            obj instanceof WebGLShader ||
            obj instanceof WebGLTexture);
  }

  function checkResources(args) {
    for (var ii = 0; ii < args.length; ++ii) {
      var arg = args[ii];
      if (isWebGLObject(arg)) {
        return arg.__webglDebugContextLostId__ == contextId_;
      }
    }
    return true;
  }

  function clearErrors() {
    var k = Object.keys(glErrorShadow_);
    for (var ii = 0; ii < k.length; ++ii) {
      delete glErrorShdow_[k];
    }
  }

  // Makes a function that simulates WebGL when out of context.
  function makeLostContextWrapper(ctx, functionName) {
    var f = ctx[functionName];
    return function() {
      // Only call the functions if the context is not lost.
      if (!contextLost_) {
        if (!checkResources(arguments)) {
          glErrorShadow_[ctx.INVALID_OPERATION] = true;
          return;
        }
        var result = f.apply(ctx, arguments);
        return result;
      }
    };
  }

  for (var propertyName in ctx) {
    if (typeof ctx[propertyName] == 'function') {
       wrapper_[propertyName] = makeLostContextWrapper(ctx, propertyName);
     } else {
       wrapper_[propertyName] = ctx[propertyName];
     }
  }

  function makeWebGLContextEvent(statusMessage) {
    return {statusMessage: statusMessage};
  }

  function freeResources() {
    for (var ii = 0; ii < resourceDb_.length; ++ii) {
      var resource = resourceDb_[ii];
      if (resource instanceof WebGLBuffer) {
        ctx.deleteBuffer(resource);
      } else if (resource instanceof WebctxFramebuffer) {
        ctx.deleteFramebuffer(resource);
      } else if (resource instanceof WebctxProgram) {
        ctx.deleteProgram(resource);
      } else if (resource instanceof WebctxRenderbuffer) {
        ctx.deleteRenderbuffer(resource);
      } else if (resource instanceof WebctxShader) {
        ctx.deleteShader(resource);
      } else if (resource instanceof WebctxTexture) {
        ctx.deleteTexture(resource);
      }
    }
  }

  wrapper_.loseContext = function() {
    if (!contextLost_) {
      contextLost_ = true;
      ++contextId_;
      while (ctx.getError());
      clearErrors();
      glErrorShadow_[ctx.CONTEXT_LOST_WEBGL] = true;
      setTimeout(function() {
          if (onLost_) {
            onLost_(makeWebGLContextEvent("context lost"));
          }
        }, 0);
    }
  };

  wrapper_.restoreContext = function() {
    if (contextLost_) {
      if (onRestored_) {
        setTimeout(function() {
            freeResources();
            resetToInitialState(ctx);
            contextLost_ = false;
            if (onRestored_) {
              var callback = onRestored_;
              onRestored_ = nextOnRestored_;
              nextOnRestored_ = undefined;
              callback(makeWebGLContextEvent("context restored"));
            }
          }, 0);
      } else {
        throw "You can not restore the context without a listener"
      }
    }
  };

  // Wrap a few functions specially.
  wrapper_.getError = function() {
    if (!contextLost_) {
      var err;
      while (err = ctx.getError()) {
        glErrorShadow_[err] = true;
      }
    }
    for (var err in glErrorShadow_) {
      if (glErrorShadow_[err]) {
        delete glErrorShadow_[err];
        return err;
      }
    }
    return ctx.NO_ERROR;
  };

  var creationFunctions = [
    "createBuffer",
    "createFramebuffer",
    "createProgram",
    "createRenderbuffer",
    "createShader",
    "createTexture"
  ];
  for (var ii = 0; ii < creationFunctions.length; ++ii) {
    var functionName = creationFunctions[ii];
    wrapper_[functionName] = function(f) {
      return function() {
        if (contextLost_) {
          return null;
        }
        var obj = f.apply(ctx, arguments);
        obj.__webglDebugContextLostId__ = contextId_;
        resourceDb_.push(obj);
        return obj;
      };
    }(ctx[functionName]);
  }

  var functionsThatShouldReturnNull = [
    "getActiveAttrib",
    "getActiveUniform",
    "getBufferParameter",
    "getContextAttributes",
    "getAttachedShaders",
    "getFramebufferAttachmentParameter",
    "getParameter",
    "getProgramParameter",
    "getProgramInfoLog",
    "getRenderbufferParameter",
    "getShaderParameter",
    "getShaderInfoLog",
    "getShaderSource",
    "getTexParameter",
    "getUniform",
    "getUniformLocation",
    "getVertexAttrib"
  ];
  for (var ii = 0; ii < functionsThatShouldReturnNull.length; ++ii) {
    var functionName = functionsThatShouldReturnNull[ii];
    wrapper_[functionName] = function(f) {
      return function() {
        if (contextLost_) {
          return null;
        }
        return f.apply(ctx, arguments);
      }
    }(wrapper_[functionName]);
  }

  var isFunctions = [
    "isBuffer",
    "isEnabled",
    "isFramebuffer",
    "isProgram",
    "isRenderbuffer",
    "isShader",
    "isTexture"
  ];
  for (var ii = 0; ii < isFunctions.length; ++ii) {
    var functionName = isFunctions[ii];
    wrapper_[functionName] = function(f) {
      return function() {
        if (contextLost_) {
          return false;
        }
        return f.apply(ctx, arguments);
      }
    }(wrapper_[functionName]);
  }

  wrapper_.checkFramebufferStatus = function(f) {
    return function() {
      if (contextLost_) {
        return ctx.FRAMEBUFFER_UNSUPPORTED;
      }
      return f.apply(ctx, arguments);
    };
  }(wrapper_.checkFramebufferStatus);

  wrapper_.getAttribLocation = function(f) {
    return function() {
      if (contextLost_) {
        return -1;
      }
      return f.apply(ctx, arguments);
    };
  }(wrapper_.getAttribLocation);

  wrapper_.getVertexAttribOffset = function(f) {
    return function() {
      if (contextLost_) {
        return 0;
      }
      return f.apply(ctx, arguments);
    };
  }(wrapper_.getVertexAttribOffset);

  wrapper_.isContextLost = function() {
    return contextLost_;
  };

  function wrapEvent(listener) {
    if (typeof(listener) == "function") {
      return listener;
    } else {
      return function(info) {
        listener.handleEvent(info);
      }
    }
  }

  wrapper_.registerOnContextLostListener = function(listener) {
    onLost_ = wrapEvent(listener);
  };

  wrapper_.registerOnContextRestoredListener = function(listener) {
    if (contextLost_) {
      nextOnRestored_ = wrapEvent(listener);
    } else {
      onRestored_ = wrapEvent(listener);
    }
  }

  return wrapper_;
}

return {
  /**
   * Initializes this module. Safe to call more than once.
   * @param {!WebGLRenderingContext} ctx A WebGL context. If
   *    you have more than one context it doesn't matter which one
   *    you pass in, it is only used to pull out constants.
   */
  'init': init,

  /**
   * Returns true or false if value matches any WebGL enum
   * @param {*} value Value to check if it might be an enum.
   * @return {boolean} True if value matches one of the WebGL defined enums
   */
  'mightBeEnum': mightBeEnum,

  /**
   * Gets an string version of an WebGL enum.
   *
   * Example:
   *   WebGLDebugUtil.init(ctx);
   *   var str = WebGLDebugUtil.glEnumToString(ctx.getError());
   *
   * @param {number} value Value to return an enum for
   * @return {string} The string version of the enum.
   */
  'glEnumToString': glEnumToString,

  /**
   * Converts the argument of a WebGL function to a string.
   * Attempts to convert enum arguments to strings.
   *
   * Example:
   *   WebGLDebugUtil.init(ctx);
   *   var str = WebGLDebugUtil.glFunctionArgToString('bindTexture', 0, gl.TEXTURE_2D);
   *
   * would return 'TEXTURE_2D'
   *
   * @param {string} functionName the name of the WebGL function.
   * @param {number} argumentIndx the index of the argument.
   * @param {*} value The value of the argument.
   * @return {string} The value as a string.
   */
  'glFunctionArgToString': glFunctionArgToString,

  /**
   * Given a WebGL context returns a wrapped context that calls
   * gl.getError after every command and calls a function if the
   * result is not NO_ERROR.
   *
   * You can supply your own function if you want. For example, if you'd like
   * an exception thrown on any GL error you could do this
   *
   *    function throwOnGLError(err, funcName, args) {
   *      throw WebGLDebugUtils.glEnumToString(err) + " was caused by call to" +
   *            funcName;
   *    };
   *
   *    ctx = WebGLDebugUtils.makeDebugContext(
   *        canvas.getContext("webgl"), throwOnGLError);
   *
   * @param {!WebGLRenderingContext} ctx The webgl context to wrap.
   * @param {!function(err, funcName, args): void} opt_onErrorFunc The function
   *     to call when gl.getError returns an error. If not specified the default
   *     function calls console.log with a message.
   */
  'makeDebugContext': makeDebugContext,

  /**
   * Given a WebGL context returns a wrapped context that adds 4
   * functions.
   *
   * ctx.loseContext:
   *   simulates a lost context event.
   *
   * ctx.restoreContext:
   *   simulates the context being restored.
   *
   * ctx.registerOnContextLostListener(listener):
   *   lets you register a listener for context lost. Use instead
   *   of addEventListener('webglcontextlostevent', listener);
   *
   * ctx.registerOnContextRestoredListener(listener):
   *   lets you register a listener for context restored. Use
   *   instead of addEventListener('webglcontextrestored',
   *   listener);
   *
   * @param {!WebGLRenderingContext} ctx The webgl context to wrap.
   */
  'makeLostContextSimulatingContext': makeLostContextSimulatingContext,

  /**
   * Resets a context to the initial state.
   * @param {!WebGLRenderingContext} ctx The webgl context to
   *     reset.
   */
  'resetToInitialState': resetToInitialState
};

}();
/*
 * Copyright 2010, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


/**
 * @fileoverview This file contains functions every webgl program will need
 * a version of one way or another.
 *
 * Instead of setting up a context manually it is recommended to
 * use. This will check for success or failure. On failure it
 * will attempt to present an approriate message to the user.
 *
 *       gl = WebGLUtils.setupWebGL(canvas);
 *
 * For animated WebGL apps use of setTimeout or setInterval are
 * discouraged. It is recommended you structure your rendering
 * loop like this.
 *
 *       function render() {
 *         window.requestAnimFrame(render, canvas);
 *
 *         // do rendering
 *         ...
 *       }
 *       render();
 *
 * This will call your rendering function up to the refresh rate
 * of your display but will stop rendering if your app is not
 * visible.
 */

WebGLUtils = function() {

/**
 * Creates the HTML for a failure message
 * @param {string} canvasContainerId id of container of th
 *        canvas.
 * @return {string} The html.
 */
var makeFailHTML = function(msg) {
  return '' +
    '<table style="background-color: #8CE; width: 100%; height: 100%;"><tr>' +
    '<td align="center">' +
    '<div style="display: table-cell; vertical-align: middle;">' +
    '<div style="">' + msg + '</div>' +
    '</div>' +
    '</td></tr></table>';
};

/**
 * Message for getting a webgl browser
 * @type {string}
 */
var GET_A_WEBGL_BROWSER = '' +
  'This page requires a browser that supports WebGL.<br/>' +
  '<a href="http://get.webgl.org">Click here to upgrade your browser.</a>';

/**
 * Mesasge for need better hardware
 * @type {string}
 */
var OTHER_PROBLEM = '' +
  "It doesn't appear your computer can support WebGL.<br/>" +
  '<a href="http://get.webgl.org/troubleshooting/">Click here for more information.</a>';

/**
 * Creates a webgl context. If creation fails it will
 * change the contents of the container of the <canvas>
 * tag to an error message with the correct links for WebGL.
 * @param {Element} canvas. The canvas element to create a
 *     context from.
 * @param {WebGLContextCreationAttirbutes} opt_attribs Any
 *     creation attributes you want to pass in.
 * @return {WebGLRenderingContext} The created context.
 */
var setupWebGL = function(canvas, opt_attribs) {
  function showLink(str) {
    var container = canvas.parentNode;
    if (container) {
      container.innerHTML = makeFailHTML(str);
    }
  };

  if (!window.WebGLRenderingContext) {
    showLink(GET_A_WEBGL_BROWSER);
    return null;
  }

  var context = create3DContext(canvas, opt_attribs);
  if (!context) {
    showLink(OTHER_PROBLEM);
  }
  return context;
};

/**
 * Creates a webgl context.
 * @param {!Canvas} canvas The canvas tag to get context
 *     from. If one is not passed in one will be created.
 * @return {!WebGLContext} The created context.
 */
var create3DContext = function(canvas, opt_attribs) {
  var names = ["webgl", "experimental-webgl", "webkit-3d", "moz-webgl"];
  var context = null;
  for (var ii = 0; ii < names.length; ++ii) {
    try {
      context = canvas.getContext(names[ii], opt_attribs);
    } catch(e) {}
    if (context) {
      break;
    }
  }
  return context;
};

return {
  create3DContext: create3DContext,
  setupWebGL: setupWebGL
};
}();

/**
 * Provides requestAnimationFrame in a cross browser way.
 */
window.requestAnimFrame = (function() {
  return window.requestAnimationFrame ||
         window.webkitRequestAnimationFrame ||
         window.mozRequestAnimationFrame ||
         window.oRequestAnimationFrame ||
         window.msRequestAnimationFrame ||
         function(/* function FrameRequestCallback */ callback, /* DOMElement Element */ element) {
           window.setTimeout(callback, 1000/60);
         };
})();


Lux.Lib.tessellate = (function() {

var Module = {};
// var CompiledModule = (function() {
function da(c){throw c}var na=void 0,a=!0,b=null,l=!1;try{this.Module=Module}catch(pa){this.Module=Module={}}var va="object"===typeof process&&"function"===typeof require,Ta="object"===typeof window,Ua="function"===typeof importScripts,Va=!Ta&&!va&&!Ua;if(va){Module.print=(function(c){process.stdout.write(c+"\n")});Module.printErr=(function(c){process.stderr.write(c+"\n")});var db=require("fs"),lb=require("path");Module.read=(function(c){var c=lb.normalize(c),e=db.readFileSync(c).toString();!e&&c!=lb.resolve(c)&&(c=path.join(__dirname,"..","src",c),e=db.readFileSync(c).toString());return e});Module.load=(function(c){Ab(read(c))});Module.arguments||(Module.arguments=process.argv.slice(2))}Va&&(Module.print=print,"undefined"!=typeof printErr&&(Module.printErr=printErr),Module.read="undefined"!=typeof read?read:(function(c){snarf(c)}),Module.arguments||("undefined"!=typeof scriptArgs?Module.arguments=scriptArgs:"undefined"!=typeof arguments&&(Module.arguments=arguments)));Ta&&!Ua&&(Module.print||(Module.print=(function(c){console.log(c)})),Module.printErr||(Module.printErr=(function(c){console.log(c)})));if(Ta||Ua){Module.read=(function(c){var e=new XMLHttpRequest;e.open("GET",c,l);e.send(b);return e.responseText}),Module.arguments||"undefined"!=typeof arguments&&(Module.arguments=arguments)}Ua&&(Module.print||(Module.print=(function(){})),Module.load=importScripts);!Ua&&!Ta&&!va&&!Va&&da("Unknown runtime environment. Where are we?");function Ab(c){eval.call(b,c)}"undefined"==!Module.load&&Module.read&&(Module.load=(function(c){Ab(Module.read(c))}));Module.print||(Module.print=(function(){}));Module.printErr||(Module.printErr=Module.print);Module.arguments||(Module.arguments=[]);Module.print=Module.print;Module.c=Module.printErr;Module.preRun||(Module.preRun=[]);Module.postRun||(Module.postRun=[]);function Eb(c){if(1==Jb){return 1}var e={"%i1":1,"%i8":1,"%i16":2,"%i32":4,"%i64":8,"%float":4,"%double":8}["%"+c];e||("*"==c.charAt(c.length-1)?e=Jb:"i"==c[0]&&(c=parseInt(c.substr(1)),Kb(0==c%8),e=c/8));return e}var Lb;function Ob(){var c=[],e=0;this.u=(function(d){d&=255;e&&(c.push(d),e--);if(0==c.length){if(128>d){return String.fromCharCode(d)}c.push(d);e=191<d&&224>d?1:2;return""}if(0<e){return""}var d=c[0],f=c[1],g=c[2],d=191<d&&224>d?String.fromCharCode((d&31)<<6|f&63):String.fromCharCode((d&15)<<12|(f&63)<<6|g&63);c.length=0;return d});this.D=(function(c){for(var c=unescape(encodeURIComponent(c)),e=[],g=0;g<c.length;g++){e.push(c.charCodeAt(g))}return e})}function Pb(c){var e=y;y=y+c|0;y=y+3>>2<<2;return e}function Qb(c){var e=Rb;Rb=Rb+c|0;Rb=Rb+3>>2<<2;Rb>=Sb&&Tb("Cannot enlarge memory arrays. Either (1) compile with -s TOTAL_MEMORY=X with X higher than the current value, (2) compile with ALLOW_MEMORY_GROWTH which adjusts the size at runtime but prevents some optimizations, or (3) set Module.TOTAL_MEMORY before the program runs.");return e}var Jb=4,Ub={},Vb=1,Wb={},Xb,Yb;function Tb(c){Module.print(c+":\n"+Error().stack);da("Assertion: "+c)}function Kb(c,e){c||Tb("Assertion failed: "+e)}var Zb=this;Module.ccall=(function(c,e,d,f){return $b(bc(c),e,d,f)});function bc(c){try{var e=eval("_"+c)}catch(d){try{e=Zb.Module["_"+c]}catch(f){}}Kb(e,"Cannot call unknown function "+c+" (perhaps LLVM optimizations or closure removed it?)");return e}function $b(c,e,d,f){function g(c,d){if("string"==d){if(c===b||c===na||0===c){return 0}h||(h=y);var e=Pb(c.length+1);cc(c,e);return e}return"array"==d?(h||(h=y),e=Pb(c.length),dc(c,e),e):c}var h=0,i=0,f=f?f.map((function(c){return g(c,d[i++])})):[];c=c.apply(b,f);"string"==e?e=ec(c):(Kb("array"!=e),e=c);h&&(y=h);return e}Module.cwrap=(function(c,e,d){var f=bc(c);return(function(){return $b(f,e,d,Array.prototype.slice.call(arguments))})});function fc(c,e,d){d=d||"i8";"*"===d.charAt(d.length-1)&&(d="i32");switch(d){case"i1":gc[c]=e;break;case"i8":gc[c]=e;break;case"i16":hc[c>>1]=e;break;case"i32":B[c>>2]=e;break;case"i64":Yb=[e>>>0,Math.min(Math.floor(e/4294967296),4294967295)>>>0];B[c>>2]=Yb[0];B[c+4>>2]=Yb[1];break;case"float":ic[c>>2]=e;break;case"double":D[E>>3]=e;B[c>>2]=B[E>>2];B[c+4>>2]=B[E+4>>2];break;default:Tb("invalid type for setValue: "+d)}}Module.setValue=fc;Module.getValue=(function(c,e){e=e||"i8";"*"===e.charAt(e.length-1)&&(e="i32");switch(e){case"i1":return gc[c];case"i8":return gc[c];case"i16":return hc[c>>1];case"i32":return B[c>>2];case"i64":return B[c>>2];case"float":return ic[c>>2];case"double":return B[E>>2]=B[c>>2],B[E+4>>2]=B[c+4>>2],D[E>>3];default:Tb("invalid type for setValue: "+e)}return b});var jc=1,kc=2,G=3;Module.ALLOC_NORMAL=0;Module.ALLOC_STACK=jc;Module.ALLOC_STATIC=kc;Module.ALLOC_NONE=G;lc=(function(c,e,d){for(d=c+d;c<d;){gc[c++]=e}});function I(c,e,d,f){var g,h;"number"===typeof c?(g=a,h=c):(g=l,h=c.length);var i="string"===typeof e?e:b,d=d==G?f:[O,Pb,Qb][d===na?kc:d](Math.max(h,i?1:e.length));if(g){return lc(d,0,h),d}if("i8"===i){return mc.set(new Uint8Array(c),d),d}for(g=0;g<h;){var j=c[g];"function"===typeof j&&(j=Ub.J(j));f=i||e[g];0===f?g++:("i64"==f&&(f="i32"),fc(d+g,j,f),g+=Eb(f))}return d}Module.allocate=I;function ec(c,e){for(var d=new Ob,f="undefined"==typeof e,g="",h=0,i;;){i=mc[c+h];if(f&&0==i){break}g+=d.u(i);h+=1;if(!f&&h==e){break}}return g}Module.Pointer_stringify=ec;Module.Array_stringify=(function(c){for(var e="",d=0;d<c.length;d++){e+=String.fromCharCode(c[d])}return e});var qc=4096,gc,mc,hc,rc,B,sc,ic,D,y,Rb,tc=Module.TOTAL_STACK||5242880,Sb=Module.TOTAL_MEMORY||16777216;Kb(!!Int32Array&&!!Float64Array&&!!(new Int32Array(1)).subarray&&!!(new Int32Array(1)).set,"Cannot fallback to non-typed array case: Code is too specialized");var uc=new ArrayBuffer(Sb);gc=new Int8Array(uc);hc=new Int16Array(uc);B=new Int32Array(uc);mc=new Uint8Array(uc);rc=new Uint16Array(uc);sc=new Uint32Array(uc);ic=new Float32Array(uc);D=new Float64Array(uc);B[0]=255;Kb(255===mc[0]&&0===mc[3],"Typed arrays 2 must be run on a little-endian system");Module.HEAP=na;Module.HEAP8=gc;Module.HEAP16=hc;Module.HEAP32=B;Module.HEAPU8=mc;Module.HEAPU16=rc;Module.HEAPU32=sc;Module.HEAPF32=ic;Module.HEAPF64=D;y=4*Math.ceil(.25);var E,vc=I(12,"i8",jc);E=8*Math.ceil(vc/8);Kb(0==E%8);Rb=tc;Kb(Rb<Sb);var xc=I(wc("(null)"),"i8",jc);function yc(c){for(;0<c.length;){var e=c.shift(),d=e.l;if("number"===typeof d){if(e.i===na){P[d]()}else{(e=[e.i])&&e.length?P[d].apply(b,e):P[d]()}}else{d(e.i===na?b:e.i)}}}var zc=[],Ac=[],Bc=[];function wc(c,e,d){c=(new Ob).D(c);d&&(c.length=d);e||c.push(0);return c}Module.intArrayFromString=wc;Module.intArrayToString=(function(c){for(var e=[],d=0;d<c.length;d++){var f=c[d];255<f&&(f&=255);e.push(String.fromCharCode(f))}return e.join("")});function cc(c,e,d){c=wc(c,d);for(d=0;d<c.length;){gc[e+d]=c[d],d+=1}}Module.writeStringToMemory=cc;function dc(c,e){for(var d=0;d<c.length;d++){gc[e+d]=c[d]}}Module.writeArrayToMemory=dc;function Cc(c,e){return 0<=c?c:32>=e?2*Math.abs(1<<e-1)+c:Math.pow(2,e)+c}function Dc(c,e){if(0>=c){return c}var d=32>=e?Math.abs(1<<e-1):Math.pow(2,e-1);if(c>=d&&(32>=e||c>d)){c=-2*d+c}return c}var Ec=0,Fc={},Gc=l,Hc=b;function Ic(c){Ec++;Module.monitorRunDependencies&&Module.monitorRunDependencies(Ec);c?(Kb(!Fc[c]),Fc[c]=1,Hc===b&&"undefined"!==typeof setInterval&&(Hc=setInterval((function(){var c=l,d;for(d in Fc){c||(c=a,Module.c("still waiting on run dependencies:")),Module.c("dependency: "+d)}c&&Module.c("(end of list)")}),6e3))):Module.c("warning: run dependency added without ID")}Module.addRunDependency=Ic;function Jc(c){Ec--;Module.monitorRunDependencies&&Module.monitorRunDependencies(Ec);c?(Kb(Fc[c]),delete Fc[c]):Module.c("warning: run dependency removed without ID");0==Ec&&(Hc!==b&&(clearInterval(Hc),Hc=b),!Gc&&Kc&&Lc())}Module.removeRunDependency=Jc;Module.preloadedImages={};Module.preloadedAudios={};Kb(Rb==tc);Kb(tc==tc);Rb+=2648;Kb(Rb<Sb);var Mc;I(24,"i8",G,5242880);I([69,100,103,101,83,105,103,110,40,32,100,115,116,85,112,44,32,116,101,115,115,45,62,101,118,101,110,116,44,32,111,114,103,85,112,32,41,32,60,61,32,48,0],"i8",G,5242904);I([101,45,62,79,114,103,32,61,61,32,118,0],"i8",G,5242948);I([33,32,86,101,114,116,69,113,40,32,100,115,116,76,111,44,32,100,115,116,85,112,32,41,0],"i8",G,5242960);I([99,104,105,108,100,32,60,61,32,112,113,45,62,109,97,120,0],"i8",G,5242988);I([118,45,62,112,114,101,118,32,61,61,32,118,80,114,101,118,0],"i8",G,5243008);I([69,82,82,79,82,44,32,99,97,110,39,116,32,104,97,110,100,108,101,32,37,100,10,0],"i8",G,5243028);I([114,101,103,80,114,101,118,45,62,119,105,110,100,105,110,103,78,117,109,98,101,114,32,45,32,101,45,62,119,105,110,100,105,110,103,32,61,61,32,114,101,103,45,62,119,105,110,100,105,110,103,78,117,109,98,101,114,0],"i8",G,5243052);I([99,117,114,114,32,60,32,112,113,45,62,109,97,120,32,38,38,32,112,113,45,62,107,101,121,115,91,99,117,114,114,93,32,33,61,32,78,85,76,76,0],"i8",G,5243112);I([116,101,115,115,109,111,110,111,46,99,0],"i8",G,5243156);I([102,45,62,112,114,101,118,32,61,61,32,102,80,114,101,118,32,38,38,32,102,45,62,97,110,69,100,103,101,32,61,61,32,78,85,76,76,32,38,38,32,102,45,62,100,97,116,97,32,61,61,32,78,85,76,76,0],"i8",G,5243168);I([86,101,114,116,76,101,113,40,32,101,45,62,79,114,103,44,32,101,45,62,68,115,116,32,41,0],"i8",G,5243228);I([99,117,114,114,32,33,61,32,76,79,78,71,95,77,65,88,0],"i8",G,5243256);I([101,45,62,76,102,97,99,101,32,61,61,32,102,0],"i8",G,5243276);I([114,101,103,45,62,101,85,112,45,62,119,105,110,100,105,110,103,32,61,61,32,48,0],"i8",G,5243292);I([76,69,81,40,32,42,42,40,105,43,49,41,44,32,42,42,105,32,41,0],"i8",G,5243316);I([115,119,101,101,112,46,99,0],"i8",G,5243336);I([101,45,62,79,110,101,120,116,45,62,83,121,109,45,62,76,110,101,120,116,32,61,61,32,101,0],"i8",G,5243344);I([114,101,103,45,62,119,105,110,100,105,110,103,78,117,109,98,101,114,32,61,61,32,48,0],"i8",G,5243372);I([112,113,32,33,61,32,78,85,76,76,0],"i8",G,5243396);I([46,47,112,114,105,111,114,105,116,121,113,45,104,101,97,112,46,99,0],"i8",G,5243408);I([101,45,62,76,110,101,120,116,45,62,79,110,101,120,116,45,62,83,121,109,32,61,61,32,101,0],"i8",G,5243428);I([43,43,102,105,120,101,100,69,100,103,101,115,32,61,61,32,49,0],"i8",G,5243456);I([112,114,105,111,114,105,116,121,113,46,99,0],"i8",G,5243476);I([103,101,111,109,46,99,0],"i8",G,5243488);I([115,105,122,101,32,61,61,32,49,0],"i8",G,5243496);I([101,45,62,83,121,109,45,62,83,121,109,32,61,61,32,101,0],"i8",G,5243508);I([108,111,45,62,76,110,101,120,116,32,33,61,32,117,112,0],"i8",G,5243528);I([114,101,103,45,62,102,105,120,85,112,112,101,114,69,100,103,101,0],"i8",G,5243544);I([104,67,117,114,114,32,62,61,32,49,32,38,38,32,104,67,117,114,114,32,60,61,32,112,113,45,62,109,97,120,32,38,38,32,104,91,104,67,117,114,114,93,46,107,101,121,32,33,61,32,78,85,76,76,0],"i8",G,5243564);I([84,114,97,110,115,76,101,113,40,32,117,44,32,118,32,41,32,38,38,32,84,114,97,110,115,76,101,113,40,32,118,44,32,119,32,41,0],"i8",G,5243620);I([115,105,122,101,32,61,61,32,48,0],"i8",G,5243660);I([101,84,111,112,76,101,102,116,32,33,61,32,101,84,111,112,82,105,103,104,116,0],"i8",G,5243672);I([101,45,62,83,121,109,32,33,61,32,101,0],"i8",G,5243696);I([84,79,76,69,82,65,78,67,69,95,78,79,78,90,69,82,79,0],"i8",G,5243708);I([70,65,76,83,69,0],"i8",G,5243728);I([33,32,86,101,114,116,69,113,40,32,101,85,112,45,62,68,115,116,44,32,101,76,111,45,62,68,115,116,32,41,0],"i8",G,5243736);I([117,112,45,62,76,110,101,120,116,32,33,61,32,117,112,32,38,38,32,117,112,45,62,76,110,101,120,116,45,62,76,110,101,120,116,32,33,61,32,117,112,0],"i8",G,5243768);I([114,101,110,100,101,114,46,99,0],"i8",G,5243812);I([105,115,101,99,116,46,115,32,60,61,32,77,65,88,40,32,111,114,103,76,111,45,62,115,44,32,111,114,103,85,112,45,62,115,32,41,0],"i8",G,5243824);I([118,78,101,119,32,33,61,32,78,85,76,76,0],"i8",G,5243864);I([77,73,78,40,32,100,115,116,76,111,45,62,115,44,32,100,115,116,85,112,45,62,115,32,41,32,60,61,32,105,115,101,99,116,46,115,0],"i8",G,5243880);I([101,45,62,76,110,101,120,116,32,33,61,32,101,0],"i8",G,5243920);I([102,78,101,119,32,33,61,32,78,85,76,76,0],"i8",G,5243936);I([105,115,101,99,116,46,116,32,60,61,32,77,65,88,40,32,111,114,103,76,111,45,62,116,44,32,100,115,116,76,111,45,62,116,32,41,0],"i8",G,5243952);I([102,114,101,101,95,104,97,110,100,108,101,32,33,61,32,76,79,78,71,95,77,65,88,0],"i8",G,5243992);I([101,45,62,83,121,109,45,62,110,101,120,116,32,61,61,32,101,80,114,101,118,45,62,83,121,109,32,38,38,32,101,45,62,83,121,109,32,61,61,32,38,109,101,115,104,45,62,101,72,101,97,100,83,121,109,32,38,38,32,101,45,62,83,121,109,45,62,83,121,109,32,61,61,32,101,32,38,38,32,101,45,62,79,114,103,32,61,61,32,78,85,76,76,32,38,38,32,101,45,62,68,115,116,32,61,61,32,78,85,76,76,32,38,38,32,101,45,62,76,102,97,99,101,32,61,61,32,78,85,76,76,32,38,38,32,101,45,62,82,102,97,99,101,32,61,61,32,78,85,76,76,0],"i8",G,5244016);I([77,73,78,40,32,111,114,103,85,112,45,62,116,44,32,100,115,116,85,112,45,62,116,32,41,32,60,61,32,105,115,101,99,116,46,116,0],"i8",G,5244168);I([86,101,114,116,76,101,113,40,32,117,44,32,118,32,41,32,38,38,32,86,101,114,116,76,101,113,40,32,118,44,32,119,32,41,0],"i8",G,5244208);I([101,45,62,68,115,116,32,33,61,32,78,85,76,76,0],"i8",G,5244244);I([33,32,114,101,103,85,112,45,62,102,105,120,85,112,112,101,114,69,100,103,101,32,38,38,32,33,32,114,101,103,76,111,45,62,102,105,120,85,112,112,101,114,69,100,103,101,0],"i8",G,5244260);I([101,45,62,79,114,103,32,33,61,32,78,85,76,76,0],"i8",G,5244308);I([102,45,62,109,97,114,107,101,100,0],"i8",G,5244324);I([111,114,103,85,112,32,33,61,32,116,101,115,115,45,62,101,118,101,110,116,32,38,38,32,111,114,103,76,111,32,33,61,32,116,101,115,115,45,62,101,118,101,110,116,0],"i8",G,5244336);I([101,45,62,83,121,109,45,62,110,101,120,116,32,61,61,32,101,80,114,101,118,45,62,83,121,109,0],"i8",G,5244384);I([69,100,103,101,83,105,103,110,40,32,100,115,116,76,111,44,32,116,101,115,115,45,62,101,118,101,110,116,44,32,111,114,103,76,111,32,41,32,62,61,32,48,0],"i8",G,5244412);I([118,45,62,112,114,101,118,32,61,61,32,118,80,114,101,118,32,38,38,32,118,45,62,97,110,69,100,103,101,32,61,61,32,78,85,76,76,32,38,38,32,118,45,62,100,97,116,97,32,61,61,32,78,85,76,76,0],"i8",G,5244456);I([102,45,62,112,114,101,118,32,61,61,32,102,80,114,101,118,0],"i8",G,5244516);I([109,101,115,104,46,99,0],"i8",G,5244536);I(468,"i8",G,5244544);I([95,95,103,108,95,116,114,97,110,115,83,105,103,110,0],"i8",G,5245012);I([95,95,103,108,95,116,114,97,110,115,69,118,97,108,0],"i8",G,5245028);I([95,95,103,108,95,114,101,110,100,101,114,77,101,115,104,0],"i8",G,5245044);I([95,95,103,108,95,112,113,83,111,114,116,73,110,115,101,114,116,0],"i8",G,5245060);I([95,95,103,108,95,112,113,83,111,114,116,73,110,105,116,0],"i8",G,5245080);I([95,95,103,108,95,112,113,83,111,114,116,68,101,108,101,116,101,80,114,105,111,114,105,116,121,81,0],"i8",G,5245096);I([95,95,103,108,95,112,113,83,111,114,116,68,101,108,101,116,101,0],"i8",G,5245124);I([95,95,103,108,95,112,113,72,101,97,112,73,110,115,101,114,116,0],"i8",G,5245144);I([95,95,103,108,95,112,113,72,101,97,112,68,101,108,101,116,101,0],"i8",G,5245164);I([95,95,103,108,95,109,101,115,104,84,101,115,115,101,108,108,97,116,101,77,111,110,111,82,101,103,105,111,110,0],"i8",G,5245184);I([95,95,103,108,95,109,101,115,104,67,104,101,99,107,77,101,115,104,0],"i8",G,5245216);I([95,95,103,108,95,101,100,103,101,83,105,103,110,0],"i8",G,5245236);I([95,95,103,108,95,101,100,103,101,69,118,97,108,0],"i8",G,5245252);I([82,101,110,100,101,114,84,114,105,97,110,103,108,101,0],"i8",G,5245268);I([82,101,110,100,101,114,83,116,114,105,112,0],"i8",G,5245284);I([82,101,110,100,101,114,70,97,110,0],"i8",G,5245296);I([82,101,109,111,118,101,68,101,103,101,110,101,114,97,116,101,70,97,99,101,115,0],"i8",G,5245308);I([77,97,107,101,86,101,114,116,101,120,0],"i8",G,5245332);I([77,97,107,101,70,97,99,101,0],"i8",G,5245344);I([73,115,87,105,110,100,105,110,103,73,110,115,105,100,101,0],"i8",G,5245356);I([70,108,111,97,116,68,111,119,110,0],"i8",G,5245372);I([70,105,120,85,112,112,101,114,69,100,103,101,0],"i8",G,5245384);I([68,111,110,101,69,100,103,101,68,105,99,116,0],"i8",G,5245400);I([68,101,108,101,116,101,82,101,103,105,111,110,0],"i8",G,5245416);I([67,111,110,110,101,99,116,76,101,102,116,68,101,103,101,110,101,114,97,116,101,0],"i8",G,5245432);I([67,104,101,99,107,70,111,114,76,101,102,116,83,112,108,105,99,101,0],"i8",G,5245456);I([67,104,101,99,107,70,111,114,73,110,116,101,114,115,101,99,116,0],"i8",G,5245476);I([65,100,100,82,105,103,104,116,69,100,103,101,115,0],"i8",G,5245496);I([0,0,0,63,0,0,0,63,0,0,0,0,0,0,0,0],"i8",G,5245512);function S(c,e,d,f){da("Assertion failed: "+(f?ec(f):"unknown condition")+", at: "+[c?ec(c):"unknown filename",e,d?ec(d):"unknown function"]+" at "+Error().stack)}function X(c){da({C:a,id:B[c>>2],value:1})}function lc(c,e,d){var c=c|0,e=e|0,d=d|0,f=0,g=0,h=0,i=0,f=c+d|0;if(d|0){i=c&3;g=e|e<<8|e<<16|e<<24;h=f&-4;if(i){for(i=c+4-i|0;(c|0)<(i|0);){gc[c]=e,c=c+1|0}}for(;(c|0)<(h|0);){B[c>>2]=g,c=c+4|0}}for(;(c|0)<(f|0);){gc[c]=e,c=c+1|0}}var Nc=13,Oc=9,Pc=22,Qc=5,Rc=21,Sc=6;function Tc(c){Uc||(Uc=I([0],"i32",kc));B[Uc>>2]=c}var Uc,Vc=I(1,"i32*",jc),Wc=I(1,"i32*",jc);Mc=I(1,"i32*",jc);var Xc=I(1,"i32*",jc),Yc=2,Zc=[b],$c=a;function ad(c,e){if("string"!==typeof c){return b}e===na&&(e="/");c&&"/"==c[0]&&(e="");for(var d=(e+"/"+c).split("/").reverse(),f=[""];d.length;){var g=d.pop();""==g||"."==g||(".."==g?1<f.length&&f.pop():f.push(g))}return 1==f.length?"/":f.join("/")}function bd(c,e,d){var f={B:l,k:l,error:0,name:b,path:b,object:b,r:l,t:b,s:b},c=ad(c);if("/"==c){f.B=a,f.k=f.r=a,f.name="/",f.path=f.t="/",f.object=f.s=cd}else{if(c!==b){for(var d=d||0,c=c.slice(1).split("/"),g=cd,h=[""];c.length;){1==c.length&&g.d&&(f.r=a,f.t=1==h.length?"/":h.join("/"),f.s=g,f.name=c[0]);var i=c.shift();if(g.d){if(g.v){if(!g.a.hasOwnProperty(i)){f.error=2;break}}else{f.error=Nc;break}}else{f.error=20;break}g=g.a[i];if(g.link&&!(e&&0==c.length)){if(40<d){f.error=40;break}f=ad(g.link,h.join("/"));f=bd([f].concat(c).join("/"),e,d+1);break}h.push(i);0==c.length&&(f.k=a,f.path=h.join("/"),f.object=g)}}}return f}function dd(c){ed();c=bd(c,na);if(c.k){return c.object}Tc(c.error);return b}function fd(c,e,d,f,g){c||(c="/");"string"===typeof c&&(c=dd(c));c||(Tc(Nc),da(Error("Parent path must exist.")));c.d||(Tc(20),da(Error("Parent must be a folder.")));!c.write&&!$c&&(Tc(Nc),da(Error("Parent folder must be writeable.")));if(!e||"."==e||".."==e){Tc(2),da(Error("Name must not be empty."))}c.a.hasOwnProperty(e)&&(Tc(17),da(Error("Can't overwrite object.")));c.a[e]={v:f===na?a:f,write:g===na?l:g,timestamp:Date.now(),A:Yc++};for(var h in d){d.hasOwnProperty(h)&&(c.a[e][h]=d[h])}return c.a[e]}function gd(c,e,d,f){return fd(c,e,{d:a,b:l,a:{}},d,f)}function hd(c,e,d,f){c=dd(c);c===b&&da(Error("Invalid parent."));for(e=e.split("/").reverse();e.length;){var g=e.pop();g&&(c.a.hasOwnProperty(g)||gd(c,g,d,f),c=c.a[g])}return c}function id(c,e,d,f,g){d.d=l;return fd(c,e,d,f,g)}function jd(c,e,d,f,g){if("string"===typeof d){for(var h=Array(d.length),i=0,j=d.length;i<j;++i){h[i]=d.charCodeAt(i)}d=h}d={b:l,a:d.subarray?d.subarray(0):d};return id(c,e,d,f,g)}function kd(c,e,d,f){!d&&!f&&da(Error("A device must have at least one callback defined."));return id(c,e,{b:a,input:d,e:f},Boolean(d),Boolean(f))}function ed(){cd||(cd={v:a,write:a,d:a,b:l,timestamp:Date.now(),A:1,a:{}})}var ld,cd;function md(c,e,d){var f=Zc[c];if(f){if(f.g){if(0>d){return Tc(Pc),-1}if(f.object.b){if(f.object.e){for(var g=0;g<d;g++){try{f.object.e(gc[e+g])}catch(h){return Tc(Qc),-1}}f.object.timestamp=Date.now();return g}Tc(Sc);return-1}g=f.position;c=Zc[c];if(!c||c.object.b){Tc(Oc),e=-1}else{if(c.g){if(c.object.d){Tc(Rc),e=-1}else{if(0>d||0>g){Tc(Pc),e=-1}else{for(var i=c.object.a;i.length<g;){i.push(0)}for(var j=0;j<d;j++){i[g+j]=mc[e+j]}c.object.timestamp=Date.now();e=j}}}else{Tc(Nc),e=-1}}-1!=e&&(f.position+=e);return e}Tc(Nc);return-1}Tc(Oc);return-1}function Y(){da("abort() at "+Error().stack)}function nd(){switch(8){case 8:return qc;case 54:;case 56:;case 21:;case 61:;case 63:;case 22:;case 67:;case 23:;case 24:;case 25:;case 26:;case 27:;case 69:;case 28:;case 101:;case 70:;case 71:;case 29:;case 30:;case 199:;case 75:;case 76:;case 32:;case 43:;case 44:;case 80:;case 46:;case 47:;case 45:;case 48:;case 49:;case 42:;case 82:;case 33:;case 7:;case 108:;case 109:;case 107:;case 112:;case 119:;case 121:return 200809;case 13:;case 104:;case 94:;case 95:;case 34:;case 35:;case 77:;case 81:;case 83:;case 84:;case 85:;case 86:;case 87:;case 88:;case 89:;case 90:;case 91:;case 94:;case 95:;case 110:;case 111:;case 113:;case 114:;case 115:;case 116:;case 117:;case 118:;case 120:;case 40:;case 16:;case 79:;case 19:return-1;case 92:;case 93:;case 5:;case 72:;case 6:;case 74:;case 92:;case 93:;case 96:;case 97:;case 98:;case 99:;case 102:;case 103:;case 105:return 1;case 38:;case 66:;case 50:;case 51:;case 4:return 1024;case 15:;case 64:;case 41:return 32;case 55:;case 37:;case 17:return 2147483647;case 18:;case 1:return 47839;case 59:;case 57:return 99;case 68:;case 58:return 2048;case 0:return 2097152;case 3:return 65536;case 14:return 32768;case 73:return 32767;case 39:return 16384;case 60:return 1e3;case 106:return 700;case 52:return 256;case 62:return 255;case 2:return 100;case 65:return 64;case 36:return 20;case 100:return 16;case 20:return 6;case 53:return 4}Tc(Pc);return-1}function od(c){pd||(Rb=Rb+4095>>12<<12,pd=a);var e=Rb;0!=c&&Qb(c);return e}var pd,qd=l,rd,sd,td,ud;zc.unshift({l:(function(){if(!Module.noFSInit&&!ld){var c,e,d,f=(function(c){c===b||10===c?(e.h(e.buffer.join("")),e.buffer=[]):e.buffer.push(j.u(c))});Kb(!ld,"FS.init was previously called. If you want to initialize later with custom parameters, remove any earlier calls (note that one is automatically added to the generated code)");ld=a;ed();c=c||Module.stdin;e=e||Module.stdout;d=d||Module.stderr;var g=a,h=a,i=a;c||(g=l,c=(function(){if(!c.j||!c.j.length){var d;"undefined"!=typeof window&&"function"==typeof window.prompt?(d=window.prompt("Input: "),d===b&&(d=String.fromCharCode(0))):"function"==typeof readline&&(d=readline());d||(d="");c.j=wc(d+"\n",a)}return c.j.shift()}));var j=new Ob;e||(h=l,e=f);e.h||(e.h=Module.print);e.buffer||(e.buffer=[]);d||(i=l,d=f);d.h||(d.h=Module.print);d.buffer||(d.buffer=[]);try{gd("/","tmp",a,a)}catch(k){}var f=gd("/","dev",a,a),m=kd(f,"stdin",c),q=kd(f,"stdout",b,e);d=kd(f,"stderr",b,d);kd(f,"tty",c,e);Zc[1]={path:"/dev/stdin",object:m,position:0,p:a,g:l,o:l,q:!g,error:l,n:l,w:[]};Zc[2]={path:"/dev/stdout",object:q,position:0,p:l,g:a,o:l,q:!h,error:l,n:l,w:[]};Zc[3]={path:"/dev/stderr",object:d,position:0,p:l,g:a,o:l,q:!i,error:l,n:l,w:[]};Kb(128>Math.max(Vc,Wc,Mc));B[Vc>>2]=1;B[Wc>>2]=2;B[Mc>>2]=3;hd("/","dev/shm/tmp",a,a);for(g=Zc.length;g<Math.max(Vc,Wc,Mc)+4;g++){Zc[g]=b}Zc[Vc]=Zc[1];Zc[Wc]=Zc[2];Zc[Mc]=Zc[3];I([I([0,0,0,0,Vc,0,0,0,Wc,0,0,0,Mc,0,0,0],"void*",kc)],"void*",G,Xc)}})});Ac.push({l:(function(){$c=l})});Bc.push({l:(function(){ld&&(Zc[2]&&0<Zc[2].object.e.buffer.length&&Zc[2].object.e(10),Zc[3]&&0<Zc[3].object.e.buffer.length&&Zc[3].object.e(10))})});Module.FS_createFolder=gd;Module.FS_createPath=hd;Module.FS_createDataFile=jd;Module.FS_createPreloadedFile=(function(c,e,d,f,g,h,i,j){function k(c){return{jpg:"image/jpeg",png:"image/png",bmp:"image/bmp",ogg:"audio/ogg",wav:"audio/wav",mp3:"audio/mpeg"}[c.substr(-3)]}function m(d){function k(d){j||jd(c,e,d,f,g);h&&h();Jc("cp "+n)}var m=l;Module.preloadPlugins.forEach((function(c){!m&&c.canHandle(n)&&(c.handle(d,n,k,(function(){i&&i();Jc("cp "+n)})),m=a)}));m||k(d)}if(!rd){rd=a;try{new Blob,sd=a}catch(q){sd=l,console.log("warning: no blob constructor, cannot create blobs with mimetypes")}td="undefined"!=typeof MozBlobBuilder?MozBlobBuilder:"undefined"!=typeof WebKitBlobBuilder?WebKitBlobBuilder:!sd?console.log("warning: no BlobBuilder"):b;ud="undefined"!=typeof window?window.URL?window.URL:window.webkitURL:console.log("warning: cannot create object URLs");Module.preloadPlugins||(Module.preloadPlugins=[]);Module.preloadPlugins.push({canHandle:(function(c){return c.substr(-4)in{".jpg":1,".png":1,".bmp":1}}),handle:(function(c,d,e,f){var g=b;if(sd){try{g=new Blob([c],{type:k(d)})}catch(h){var i="Blob constructor present but fails: "+h+"; falling back to blob builder";Lb||(Lb={});Lb[i]||(Lb[i]=1,Module.c(i))}}g||(g=new td,g.append((new Uint8Array(c)).buffer),g=g.getBlob());var j=ud.createObjectURL(g),m=new Image;m.onload=(function(){Kb(m.complete,"Image "+d+" could not be decoded");var f=document.createElement("canvas");f.width=m.width;f.height=m.height;f.getContext("2d").drawImage(m,0,0);Module.preloadedImages[d]=f;ud.revokeObjectURL(j);e&&e(c)});m.onerror=(function(){console.log("Image "+j+" could not be decoded");f&&f()});m.src=j})});Module.preloadPlugins.push({canHandle:(function(c){return c.substr(-4)in{".ogg":1,".wav":1,".mp3":1}}),handle:(function(c,d,e,f){function g(f){i||(i=a,Module.preloadedAudios[d]=f,e&&e(c))}function h(){i||(i=a,Module.preloadedAudios[d]=new Audio,f&&f())}var i=l;if(sd){try{var j=new Blob([c],{type:k(d)})}catch(m){return h()}var j=ud.createObjectURL(j),n=new Audio;n.addEventListener("canplaythrough",(function(){g(n)}),l);n.onerror=(function(){if(!i){console.log("warning: browser could not fully decode audio "+d+", trying slower base64 approach");for(var e="",f=0,h=0,j=0;j<c.length;j++){f=f<<8|c[j];for(h+=8;6<=h;){var k=f>>h-6&63,h=h-6,e=e+"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[k]}}2==h?(e+="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[(f&3)<<4],e+="=="):4==h&&(e+="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[(f&15)<<2],e+="=");n.src="data:audio/x-"+d.substr(-3)+";base64,"+e;g(n)}});n.src=j;setTimeout((function(){g(n)}),1e4)}else{return h()}})})}for(var n,p=[c,e],r=p[0],s=1;s<p.length;s++){"/"!=r[r.length-1]&&(r+="/"),r+=p[s]}"/"==r[0]&&(r=r.substr(1));n=r;Ic("cp "+n);if("string"==typeof d){var t=i,u=(function(){t?t():da('Loading data file "'+d+'" failed.')}),v=new XMLHttpRequest;v.open("GET",d,a);v.responseType="arraybuffer";v.onload=(function(){if(200==v.status){var c=v.response;Kb(c,'Loading data file "'+d+'" failed (no arrayBuffer).');c=new Uint8Array(c);m(c);Jc("al "+d)}else{u()}});v.onerror=u;v.send(b);Ic("al "+d)}else{m(d)}});Module.FS_createLazyFile=(function(c,e,d,f,g){if("undefined"!==typeof XMLHttpRequest){Ua||da("Cannot do synchronous binary XHRs outside webworkers in modern browsers. Use --embed-file or --preload-file in emcc");var h=(function(c,d){this.length=d;this.m=c;this.f=[]});h.prototype.H=(function(c){this.K=c});var i=new XMLHttpRequest;i.open("HEAD",d,l);i.send(b);200<=i.status&&300>i.status||304===i.status||da(Error("Couldn't load "+d+". Status: "+i.status));var j=Number(i.getResponseHeader("Content-length")),k,m=1048576;if(!((k=i.getResponseHeader("Accept-Ranges"))&&"bytes"===k)){m=j}var q=new h(m,j);q.H((function(c){var e=c*q.m,f=(c+1)*q.m-1,f=Math.min(f,j-1);if("undefined"===typeof q.f[c]){var g=q.f;e>f&&da(Error("invalid range ("+e+", "+f+") or no bytes requested!"));f>j-1&&da(Error("only "+j+" bytes available! programmer error!"));var h=new XMLHttpRequest;h.open("GET",d,l);j!==m&&h.setRequestHeader("Range","bytes="+e+"-"+f);"undefined"!=typeof Uint8Array&&(h.responseType="arraybuffer");h.overrideMimeType&&h.overrideMimeType("text/plain; charset=x-user-defined");h.send(b);200<=h.status&&300>h.status||304===h.status||da(Error("Couldn't load "+d+". Status: "+h.status));e=h.response!==na?new Uint8Array(h.response||[]):wc(h.responseText||"",a);g[c]=e}"undefined"===typeof q.f[c]&&da(Error("doXHR failed!"));return q.f[c]}));h={b:l,a:q}}else{h={b:l,url:d}}return id(c,e,h,f,g)});Module.FS_createLink=(function(c,e,d,f,g){return id(c,e,{b:l,link:d},f,g)});Module.FS_createDevice=kd;Tc(0);Module.requestFullScreen=(function(){function c(){}function e(){var c=l;if((document.webkitFullScreenElement||document.webkitFullscreenElement||document.mozFullScreenElement||document.mozFullscreenElement||document.fullScreenElement||document.fullscreenElement)===d){d.G=d.requestPointerLock||d.mozRequestPointerLock||d.webkitRequestPointerLock,d.G(),c=a}if(Module.onFullScreen){Module.onFullScreen(c)}}var d=Module.canvas;document.addEventListener("fullscreenchange",e,l);document.addEventListener("mozfullscreenchange",e,l);document.addEventListener("webkitfullscreenchange",e,l);document.addEventListener("pointerlockchange",c,l);document.addEventListener("mozpointerlockchange",c,l);document.addEventListener("webkitpointerlockchange",c,l);d.F=d.requestFullScreen||d.mozRequestFullScreen||(d.webkitRequestFullScreen?(function(){d.webkitRequestFullScreen(Element.ALLOW_KEYBOARD_INPUT)}):b);d.F()});Module.requestAnimationFrame=(function(c){window.requestAnimationFrame||(window.requestAnimationFrame=window.requestAnimationFrame||window.mozRequestAnimationFrame||window.webkitRequestAnimationFrame||window.msRequestAnimationFrame||window.oRequestAnimationFrame||window.setTimeout);window.requestAnimationFrame(c)});Module.pauseMainLoop=(function(){});Module.resumeMainLoop=(function(){qd&&(qd=l,b())});var P=[0,0,vd,0,wd,0,xd,0,yd,0,zd,0,Ad,0,Bd,0,Cd,0,Dd,0,Ed,0,Fd,0,Gd,0,Hd,0,Id,0,Jd,0,Kd,0,Ld,0,Md,0,Nd,0,Od,0,Pd,0,Qd,0,Rd,0,Sd,0,Td,0];function Ud(c,e){var d=c+8|0,f=B[d>>2],g=e+8|0,h=B[g>>2];B[B[f+4>>2]+12>>2]=e;B[B[h+4>>2]+12>>2]=c;B[d>>2]=h;B[g>>2]=f}function Vd(c,e,d){var f;f=c+16|0;for(c=c+12|0;;){var g=B[e+8>>2],e=B[g>>2];if(0==(e|0)){break}if(0==(P[B[f>>2]](B[c>>2],e,d)|0)){e=g}else{break}}c=O(12);f=c>>2;if(0==(c|0)){return 0}B[f]=d;d=(g+4|0)>>2;B[f+1]=B[d];B[B[d]+8>>2]=c;B[f+2]=g;return B[d]=c}function Wd(c){var e=O(60),d=O(60),f=O(28),g=0==(e|0),h=0==(d|0),i=0==(f|0);if(!(g|h|i)){return g=Xd(c+88|0),0==(g|0)?(Z(e),Z(d),Z(f),c=0):(h=c|0,Yd(e,g,h),Yd(d,B[g+4>>2],h),Zd(f,g,c+60|0),c=g),c}g||Z(e);h||Z(d);if(i){return 0}Z(f);return 0}function Xd(c){var e,d=O(64);e=d>>2;if(0==(d|0)){return 0}var f=d+32|0,g=B[c+4>>2],c=g>>>0<c>>>0?g:c,g=c+4|0,h=B[B[g>>2]>>2];B[f>>2]=h;B[B[h+4>>2]>>2]=d;B[e]=c;B[B[g>>2]>>2]=f;B[e+1]=f;B[e+2]=d;B[e+3]=f;c=(d+16|0)>>2;B[c]=0;B[c+1]=0;B[c+2]=0;B[c+3]=0;B[e+9]=d;B[e+10]=f;B[e+11]=d;e=(d+48|0)>>2;B[e]=0;B[e+1]=0;B[e+2]=0;B[e+3]=0;return d}Xd.X=1;function Yd(c,e,d){0==(c|0)&&S(5244536,141,5245332,5243864);var f=d+4|0,g=B[f>>2];B[c+4>>2]=g;B[g>>2]=c;B[c>>2]=d;B[f>>2]=c;B[c+8>>2]=e;B[c+12>>2]=0;for(d=e;!(B[d+16>>2]=c,d=B[d+8>>2],(d|0)==(e|0));){}}function Zd(c,e,d){var f=c>>2;0==(c|0)&&S(5244536,174,5245344,5243936);var g=d+4|0,h=B[g>>2];B[f+1]=h;B[h>>2]=c;B[f]=d;B[g>>2]=c;B[f+2]=e;B[f+3]=0;B[f+4]=0;B[f+5]=0;B[f+6]=B[d+24>>2];for(d=e;!(B[d+20>>2]=c,d=B[d+12>>2],(d|0)==(e|0));){}}function $d(c,e){var d,f;if((c|0)==(e|0)){return 1}d=B[e+16>>2];f=(c+16|0)>>2;var g=B[f];(d|0)==(g|0)?g=0:(ae(d,g),g=1);var h=B[e+20>>2];d=(c+20|0)>>2;var i=B[d];(h|0)==(i|0)?h=0:(be(h,i),h=1);Ud(e,c);if(0==(g|0)){g=O(60);if(0==(g|0)){return 0}Yd(g,e,B[f]);B[B[f]+8>>2]=c}if(0!=(h|0)){return 1}f=O(28);if(0==(f|0)){return 0}Zd(f,e,B[d]);B[B[d]+8>>2]=c;return 1}function ae(c,e){for(var d=B[c+8>>2],f=d;!(B[f+16>>2]=e,f=B[f+8>>2],(f|0)==(d|0));){}d=B[c+4>>2];f=B[c>>2];B[f+4>>2]=d;B[d>>2]=f;Z(c)}function be(c,e){for(var d=B[c+8>>2],f=d;!(B[f+20>>2]=e,f=B[f+12>>2],(f|0)==(d|0));){}d=B[c+4>>2];f=B[c>>2];B[f+4>>2]=d;B[d>>2]=f;Z(c)}function ce(c){var e,d;d=(c+4|0)>>2;var f=B[d];e=(c+20|0)>>2;var g=B[e],h=f+20|0,i=B[h>>2];(g|0)==(i|0)?g=0:(be(g,i),g=1);i=c+8|0;if((B[i>>2]|0)==(c|0)){ae(B[c+16>>2],0)}else{var j=B[d];B[B[j+20>>2]+8>>2]=B[j+12>>2];B[B[c+16>>2]+8>>2]=B[i>>2];Ud(c,B[B[d]+12>>2]);if(0==(g|0)){d=O(28);if(0==(d|0)){return 0}Zd(d,c,B[e])}}d=f+8|0;(B[d>>2]|0)==(f|0)?(ae(B[f+16>>2],0),be(B[h>>2],0)):(h=f+4|0,B[B[e]+8>>2]=B[B[h>>2]+12>>2],B[B[f+16>>2]+8>>2]=B[d>>2],Ud(f,B[B[h>>2]+12>>2]));de(c);return 1}ce.X=1;function de(c){var e=B[c+4>>2],c=e>>>0<c>>>0?e:c,e=B[c>>2],d=B[B[c+4>>2]>>2];B[B[e+4>>2]>>2]=d;B[B[d+4>>2]>>2]=e;Z(c)}function ee(c){var e,d;d=Xd(c);if(0==(d|0)){d=0}else{var f=B[d+4>>2];Ud(d,B[c+12>>2]);e=d+16|0;B[e>>2]=B[B[c+4>>2]+16>>2];var g=O(60);0==(g|0)?d=0:(Yd(g,f,B[e>>2]),e=B[c+20>>2],B[f+20>>2]=e,B[d+20>>2]=e)}if(0==(d|0)){return 0}f=B[d+4>>2];d=(c+4|0)>>2;e=B[d];Ud(e,B[B[e+4>>2]+12>>2]);Ud(B[d],f);B[B[d]+16>>2]=B[f+16>>2];e=(f+4|0)>>2;g=B[e];B[B[g+16>>2]+8>>2]=g;B[B[e]+20>>2]=B[B[d]+20>>2];B[f+28>>2]=B[c+28>>2];B[B[e]+28>>2]=B[B[d]+28>>2];return f}function fe(c,e){var d,f,g=Xd(c);f=g>>2;if(0==(g|0)){return 0}var h=B[f+1],i=B[e+20>>2];d=(c+20|0)>>2;var j=B[d];(i|0)==(j|0)?i=0:(be(i,j),i=1);Ud(g,B[c+12>>2]);Ud(h,e);B[f+4]=B[B[c+4>>2]+16>>2];B[h+16>>2]=B[e+16>>2];j=B[d];B[h+20>>2]=j;B[f+5]=j;B[B[d]+8>>2]=h;if(i){return g}f=O(28);if(0==(f|0)){return 0}Zd(f,g,B[d]);return g}function ge(c){var e,d,f=B[c+8>>2],g=B[f+12>>2];for(d=g>>2;;){var h=B[d+3];B[d+5]=0;e=(g+4|0)>>2;if(0==(B[B[e]+20>>2]|0)){var i=B[d+2];d=B[d+4];(i|0)==(g|0)?ae(d,0):(B[d+8>>2]=i,Ud(g,B[B[e]+12>>2]));i=B[e];e=i>>2;d=B[e+2];var j=B[e+4];(d|0)==(i|0)?ae(j,0):(B[j+8>>2]=d,Ud(i,B[B[e+1]+12>>2]));de(g)}if((g|0)==(f|0)){break}else{g=h,d=g>>2}}f=B[c+4>>2];g=B[c>>2];B[g+4>>2]=f;B[f>>2]=g;Z(c)}ge.X=1;function he(c){var e=c+60|0,d=B[e>>2],f=(d|0)==(e|0);a:do{if(!f){for(var g=d;;){var h=B[g>>2];Z(g);if((h|0)==(e|0)){break a}else{g=h}}}}while(0);e=c|0;d=B[c>>2];f=(d|0)==(e|0);a:do{if(!f){for(g=d;;){if(h=B[g>>2],Z(g),(h|0)==(e|0)){break a}else{g=h}}}}while(0);e=c+88|0;d=B[e>>2];if((d|0)!=(e|0)){for(;!(f=B[d>>2],Z(d),(f|0)==(e|0));){d=f}}Z(c)}function ie(c){var e,d,f,g=c>>2;d=0;var h=c+60|0,i=c|0,j=c+88|0,k=B[h>>2],m=(k|0)==(h|0),q=(B[k+4>>2]|0)==(h|0);a:do{if(m){e=q}else{for(var n=k,p=q;;){p||S(5244536,753,5245216,5244516);for(var p=n+8|0,r=B[p>>2];;){var s=r+4|0,t=B[s>>2];(t|0)==(r|0)?(S(5244536,756,5245216,5243696),s=B[s>>2]):s=t;(B[s+4>>2]|0)!=(r|0)&&S(5244536,757,5245216,5243508);s=r+12|0;(B[B[B[s>>2]+8>>2]+4>>2]|0)!=(r|0)&&S(5244536,758,5245216,5243428);(B[B[B[r+8>>2]+4>>2]+12>>2]|0)!=(r|0)&&S(5244536,759,5245216,5243344);(B[r+20>>2]|0)!=(n|0)&&S(5244536,760,5245216,5243276);r=B[s>>2];if((r|0)==(B[p>>2]|0)){break}}p=B[n>>2];r=(B[p+4>>2]|0)==(n|0);if((p|0)==(h|0)){e=r;break a}else{n=p,p=r}}}}while(0);e?0!=(B[g+17]|0)?d=162:0!=(B[g+18]|0)&&(d=162):d=162;162==d&&S(5244536,764,5245216,5243168);h=B[g];k=(h|0)==(i|0);m=(B[h+4>>2]|0)==(i|0);a:do{if(k){var u=m}else{q=h;for(e=m;;){e||S(5244536,768,5245216,5243008);e=q+8|0;for(n=B[e>>2];!(p=n+4|0,r=B[p>>2],(r|0)==(n|0)?(S(5244536,771,5245216,5243696),p=B[p>>2]):p=r,(B[p+4>>2]|0)!=(n|0)&&S(5244536,772,5245216,5243508),(B[B[B[n+12>>2]+8>>2]+4>>2]|0)!=(n|0)&&S(5244536,773,5245216,5243428),p=n+8|0,(B[B[B[p>>2]+4>>2]+12>>2]|0)!=(n|0)&&S(5244536,774,5245216,5243344),(B[n+16>>2]|0)!=(q|0)&&S(5244536,775,5245216,5242948),n=B[p>>2],(n|0)==(B[e>>2]|0));){}e=B[q>>2];n=(B[e+4>>2]|0)==(q|0);if((e|0)==(i|0)){u=n;break a}else{q=e,e=n}}}}while(0);u?0!=(B[g+2]|0)?d=182:0!=(B[g+3]|0)&&(d=182):d=182;182==d&&S(5244536,779,5245216,5244456);i=B[j>>2];u=(i|0)==(j|0);h=i+4|0;k=B[h>>2];m=(B[k>>2]|0)==(B[g+23]|0);a:do{if(u){var v=k;f=v>>2;var C=m}else{q=i;d=q>>2;e=h;e>>=2;p=m;for(n=k;;){if(p||(S(5244536,783,5245216,5244384),n=B[e]),(n|0)==(q|0)&&(S(5244536,784,5245216,5243696),n=B[e]),(B[n+4>>2]|0)!=(q|0)&&S(5244536,785,5245216,5243508),0==(B[d+4]|0)&&S(5244536,786,5245216,5244308),0==(B[B[e]+16>>2]|0)&&S(5244536,787,5245216,5244244),(B[B[B[d+3]+8>>2]+4>>2]|0)!=(q|0)&&S(5244536,788,5245216,5243428),(B[B[B[d+2]+4>>2]+12>>2]|0)!=(q|0)&&S(5244536,789,5245216,5243344),q=B[d],e=q+4|0,n=B[e>>2],p=(B[n>>2]|0)==(B[d+1]|0),(q|0)==(j|0)){v=n;f=v>>2;C=p;break a}else{d=q>>2,e>>=2}}}}while(0);(!(C&&(v|0)==(c+120|0))||!((B[f+1]|0)==(j|0)&&0==(B[g+26]|0)&&0==(B[f+4]|0)&&0==(B[g+27]|0)&&0==(B[f+5]|0)))&&S(5244536,795,5245216,5244016)}ie.X=1;function je(c,e){var d,f,g,h,i,j,k=y;y=y+72|0;j=k>>2;var m=k+12;i=m>>2;var q=k+24;h=q>>2;var n=k+36;g=n>>2;var p=k+48;f=p>>2;var r=k+60;d=r>>2;if(0!=(B[c+120>>2]|0)){r=1,p=e,d=14}else{ke(k,e);var s=B[j];if(1<(s|0)){var t=s,s=B[j+1],u=B[j+2]}else{t=1,s=e,u=14}j=e+12|0;ke(m,B[j>>2]);m=B[i];(m|0)>(t|0)?(s=B[i+1],u=B[i+2]):m=t;i=e+8|0;ke(q,B[B[i>>2]+4>>2]);q=B[h];(q|0)>(m|0)?(s=B[h+1],h=B[h+2]):(q=m,h=u);le(n,e);n=B[g];(n|0)>(q|0)?(q=B[g+1],g=B[g+2]):(n=q,q=s,g=h);le(p,B[j>>2]);p=B[f];(p|0)>(n|0)?(j=B[f+1],f=B[f+2]):(p=n,j=q,f=g);le(r,B[B[i>>2]+4>>2]);r=B[d];(r|0)>(p|0)?(p=B[d+1],d=B[d+2]):(r=p,p=j,d=f)}P[d](c,p,r);y=k}je.X=1;function me(c,e){var d,f=c>>2,g=B[f+740];if(48==(g|0)){P[B[f+33]](4)}else{P[g](4,B[f+756])}g=0==(e|0);a:do{if(!g){var h=c+120|0,i=c+2968|0,j=c+140|0,k=c+3024|0,m=c+2964|0,q=c+136|0,n=e;for(d=-1;;){var p=n+8|0,r=d;d=B[p>>2];for(d>>=2;;){if(0==(B[h>>2]|0)){var s=r}else{if(s=0==(B[B[B[d+1]+20>>2]+24>>2]|0)&1,(r|0)==(s|0)){s=r}else{if(r=B[m>>2],34==(r|0)){P[B[q>>2]](s)}else{P[r](s,B[k>>2])}}}r=B[i>>2];if(6==(r|0)){P[B[j>>2]](B[B[d+4]+12>>2])}else{P[r](B[B[d+4]+12>>2],B[k>>2])}d=B[d+3];if((d|0)==(B[p>>2]|0)){break}else{r=s,d>>=2}}n=B[n+16>>2];if(0==(n|0)){break a}else{d=s}}}}while(0);g=B[f+743];if(10==(g|0)){P[B[f+36]]()}else{P[g](B[f+756])}}me.X=1;function ne(c,e){var d,f,g=e+60|0,h=B[g>>2];if((h|0)!=(g|0)){var i=c+2960|0,j=c+132|0,k=c+2968|0,m=c+140|0;f=(c+3024|0)>>2;for(var q=c+2972|0,n=c+144|0;;){do{if(0!=(B[h+24>>2]|0)){var p=B[i>>2];if(48==(p|0)){P[B[j>>2]](2)}else{P[p](2,B[f])}p=h+8|0;d=B[p>>2];for(d>>=2;;){var r=B[k>>2];if(6==(r|0)){P[B[m>>2]](B[B[d+4]+12>>2])}else{P[r](B[B[d+4]+12>>2],B[f])}d=B[d+3];if((d|0)==(B[p>>2]|0)){break}else{d>>=2}}p=B[q>>2];if(10==(p|0)){P[B[n>>2]]()}else{P[p](B[f])}}}while(0);h=B[h>>2];if((h|0)==(g|0)){break}}}}ne.X=1;function ke(c,e){var d=e+20|0,f=B[d>>2],g=0==(B[f+24>>2]|0);a:do{if(g){var h=0,i=0}else{for(var j=0,k=0,m=e,q=d,n=f;;){if(0!=(B[n+20>>2]|0)){h=j;i=k;break a}B[n+16>>2]=k;k=B[q>>2];B[k+20>>2]=1;j=j+1|0;m=B[m+8>>2];q=m+20|0;n=B[q>>2];if(0==(B[n+24>>2]|0)){h=j;i=k;break a}}}}while(0);d=e+4|0;f=B[B[d>>2]+20>>2];g=0==(B[f+24>>2]|0);a:do{if(g){var p=h,r=i,s=e}else{j=h;n=i;q=e;k=d;for(m=f;;){if(0!=(B[m+20>>2]|0)){p=j;r=n;s=q;break a}B[m+16>>2]=n;n=B[B[k>>2]+20>>2];B[n+20>>2]=1;j=j+1|0;q=B[B[k>>2]+12>>2];k=q+4|0;m=B[B[k>>2]+20>>2];if(0==(B[m+24>>2]|0)){p=j;r=n;s=q;break a}}}}while(0);if(0!=(r|0)){for(h=r;!(B[h+20>>2]=0,h=B[h+16>>2],0==(h|0));){}}B[(c|0)>>2]=p;B[(c+4|0)>>2]=s;p=c+8|0;B[p>>2]=12}ke.X=1;function le(c,e){var d,f,g,h=e+20|0,i=B[h>>2],j=0==(B[i+24>>2]|0);a:do{if(j){var k=e,m=0,q=0}else{var n=0;d=0;f=e;g=h;for(var p=i;;){if(0!=(B[p+20>>2]|0)){k=f;m=d;q=n;break a}B[p+16>>2]=d;p=B[g>>2];B[p+20>>2]=1;d=n|1;f=B[B[f+12>>2]+4>>2];var r=f+20|0;g=B[r>>2]>>2;if(0==(B[g+6]|0)){k=f;m=p;q=d;break a}if(0!=(B[g+5]|0)){k=f;m=p;q=d;break a}B[g+4]=p;d=B[r>>2];B[d+20>>2]=1;n=n+2|0;f=B[f+8>>2];g=f+20|0;p=B[g>>2];if(0==(B[p+24>>2]|0)){k=f;m=d;q=n;break a}}}}while(0);h=e+4|0;i=B[B[h>>2]+20>>2];j=0==(B[i+24>>2]|0);a:do{if(j){var s=e,t=m,u=0}else{n=0;p=m;g=e;d=h;for(f=i;;){if(0!=(B[f+20>>2]|0)){s=g;t=p;u=n;break a}B[f+16>>2]=p;p=B[B[d>>2]+20>>2];B[p+20>>2]=1;g=n|1;r=B[B[d>>2]+12>>2];f=(r+4|0)>>2;d=B[B[f]+20>>2]>>2;if(0==(B[d+6]|0)){s=r;t=p;u=g;break a}if(0!=(B[d+5]|0)){s=r;t=p;u=g;break a}B[d+4]=p;p=B[B[f]+20>>2];B[p+20>>2]=1;n=n+2|0;g=B[B[B[f]+8>>2]+4>>2];d=g+4|0;f=B[B[d>>2]+20>>2];if(0==(B[f+24>>2]|0)){s=g;t=p;u=n;break a}}}}while(0);m=u+q|0;0==(q&1|0)?(k=B[k+4>>2],q=m):0==(u&1|0)?(k=s,q=m):(k=B[s+8>>2],q=m-1|0);if(0!=(t|0)){for(;!(B[t+20>>2]=0,t=B[t+16>>2],0==(t|0));){}}B[(c|0)>>2]=q;B[(c+4|0)>>2]=k;t=c+8|0;B[t>>2]=44}le.X=1;function oe(c){var e,d=c>>2,f=y;y=y+24|0;var g=c+160|0;e=(c+156|0)>>2;var h=B[e],i=c+28*h+160|0;if(3>(h|0)){return y=f,1}var j=c+16|0,j=(B[E>>2]=B[j>>2],B[E+4>>2]=B[j+4>>2],D[E>>3]),k=f|0;D[E>>3]=j;B[k>>2]=B[E>>2];B[k+4>>2]=B[E+4>>2];var m=c+24|0,m=(B[E>>2]=B[m>>2],B[E+4>>2]=B[m+4>>2],D[E>>3]),q=f+8|0;D[E>>3]=m;B[q>>2]=B[E>>2];B[q+4>>2]=B[E+4>>2];var q=c+32|0,q=(B[E>>2]=B[q>>2],B[E+4>>2]=B[q+4>>2],D[E>>3]),n=f+16|0;D[E>>3]=q;B[n>>2]=B[E>>2];B[n+4>>2]=B[E+4>>2];0==j&&0==m&0==q&&pe(c,k,0);j=pe(c,k,1);if(0==(j|0)){d=1}else{if(2==(j|0)){d=0}else{k=B[d+24];if(100132==(k|0)){if(0>(j|0)){return y=f,1}}else{if(100133==(k|0)){if(0<(j|0)){return y=f,1}}else{if(100134==(k|0)){return y=f,1}}}k=B[d+740];if(48==(k|0)){P[B[d+33]](0==(B[d+31]|0)?3<(B[e]|0)?6:4:2)}else{P[k](0==(B[d+31]|0)?3<(B[e]|0)?6:4:2,B[d+756])}e=(c+2968|0)>>2;k=B[e];if(6==(k|0)){P[B[d+35]](B[d+46])}else{P[k](B[d+46],B[d+756])}j=0<(j|0);a:do{if(j){if(1<(h|0)){k=c+140|0;m=c+3024|0;for(q=c+188|0;;){n=B[e];if(6==(n|0)){P[B[k>>2]](B[q+24>>2])}else{P[n](B[q+24>>2],B[m>>2])}q=q+28|0;if(q>>>0>=i>>>0){break a}}}}else{if(q=h-1|0,0<(q|0)){k=c+140|0;m=c+3024|0;for(q=c+28*q+160|0;;){n=B[e];if(6==(n|0)){P[B[k>>2]](B[q+24>>2])}else{P[n](B[q+24>>2],B[m>>2])}q=q-28|0;if(q>>>0<=g>>>0){break a}}}}}while(0);c=B[d+743];if(10==(c|0)){P[B[d+36]]()}else{P[c](B[d+756])}d=1}}y=f;return d}oe.X=1;function pe(c,e,d){var f,g,h,i=e>>2,j=0;f=B[c+156>>2];var k=c+28*f+160|0,m=0!=(d|0);m||(d=e>>2,B[d]=0,B[d+1]=0,B[d+2]=0,B[d+3]=0,B[d+4]=0,B[d+5]=0);var q=c+188|0;h=(c+160|0)>>2;g=(c+168|0)>>2;d=(c+176|0)>>2;if(2>=(f|0)){var n;return 0}var p=c+204|0,r=c+196|0;f=q|0;var s=(B[E>>2]=B[d],B[E+4>>2]=B[d+1],D[E>>3]),t=(B[E>>2]=B[p>>2],B[E+4>>2]=B[p+4>>2],D[E>>3]),p=(B[E>>2]=B[g],B[E+4>>2]=B[g+1],D[E>>3]),u=(B[E>>2]=B[r>>2],B[E+4>>2]=B[r+4>>2],D[E>>3]),r=(B[E>>2]=B[h],B[E+4>>2]=B[h+1],D[E>>3]),v=(B[E>>2]=B[f>>2],B[E+4>>2]=B[f+4>>2],D[E>>3])-r;f=(e+8|0)>>2;var e=(e+16|0)>>2,C=(B[E>>2]=B[i],B[E+4>>2]=B[i+1],D[E>>3]),z=t-s,x=u-p,u=q,q=0,A=c+216|0;a:for(;;){c=(B[E>>2]=B[f],B[E+4>>2]=B[f+1],D[E>>3]);t=(B[E>>2]=B[e],B[E+4>>2]=B[e+1],D[E>>3]);if(m){var w=u,u=A}else{var H=z,Q=x,J=v,K=u,L=A,F=C,U=r,aa=p,W=s,V=c,M=t,j=364;break}for(;;){var T=u|0,T=(B[E>>2]=B[T>>2],B[E+4>>2]=B[T+4>>2],D[E>>3])-r,N=w+36|0,N=(B[E>>2]=B[N>>2],B[E+4>>2]=B[N+4>>2],D[E>>3])-p,R=w+44|0,R=(B[E>>2]=B[R>>2],B[E+4>>2]=B[R+4>>2],D[E>>3])-s,ja=(v*N-x*T)*t+C*(x*R-z*N)+c*(z*T-v*R);if(0!=ja){break}A=u+28|0;if(A>>>0<k>>>0){z=R,x=N,v=T,w=u,u=A}else{n=q;j=375;break a}}if(0<ja){if(0>(q|0)){n=2;j=377;break}else{c=1}}else{if(0<(q|0)){n=2;j=379;break}else{c=-1}}t=u+28|0;if(t>>>0<k>>>0){z=R,x=N,v=T,q=c,A=t}else{n=c;j=378;break}}if(375==j||377==j||378==j||379==j){return n}if(364==j){for(;;){j=L|0;U=(B[E>>2]=B[j>>2],B[E+4>>2]=B[j+4>>2],D[E>>3])-U;j=K+36|0;aa=(B[E>>2]=B[j>>2],B[E+4>>2]=B[j+4>>2],D[E>>3])-aa;K=K+44|0;K=(B[E>>2]=B[K>>2],B[E+4>>2]=B[K+4>>2],D[E>>3])-W;W=Q*K-H*aa;H=H*U-J*K;Q=J*aa-Q*U;0>Q*M+F*W+V*H?(J=F-W,D[E>>3]=J,B[i]=B[E>>2],B[i+1]=B[E+4>>2],H=V-H,D[E>>3]=H,B[f]=B[E>>2],B[f+1]=B[E+4>>2],F=J,M-=Q):(J=F+W,D[E>>3]=J,B[i]=B[E>>2],B[i+1]=B[E+4>>2],H+=V,D[E>>3]=H,B[f]=B[E>>2],B[f+1]=B[E+4>>2],F=J,M=Q+M);V=H;D[E>>3]=M;B[e]=B[E>>2];B[e+1]=B[E+4>>2];j=L+28|0;if(j>>>0>=k>>>0){n=q;break}W=(B[E>>2]=B[h],B[E+4>>2]=B[h+1],D[E>>3]);H=K;Q=aa;J=U;K=L;L=j;U=W;aa=(B[E>>2]=B[g],B[E+4>>2]=B[g+1],D[E>>3]);W=(B[E>>2]=B[d],B[E+4>>2]=B[d+1],D[E>>3])}return n}}pe.X=1;function Bd(c,e,d){1!=(d|0)&&S(5243812,243,5245268,5243496);c=c+128|0;e=(e+20|0)>>2;B[B[e]+16>>2]=B[c>>2];B[c>>2]=B[e];B[B[e]+20>>2]=1}function Qd(c,e,d){var f,g,h,i=e>>2,j=c>>2;h=B[j+740];if(48==(h|0)){P[B[j+33]](5)}else{P[h](5,B[j+756])}h=(c+2968|0)>>2;var k=B[h];if(6==(k|0)){P[B[j+35]](B[B[i+4]+12>>2])}else{P[k](B[B[i+4]+12>>2],B[j+756])}k=B[h];if(6==(k|0)){P[B[j+35]](B[B[B[i+1]+16>>2]+12>>2])}else{P[k](B[B[B[i+1]+16>>2]+12>>2],B[j+756])}i=B[i+5];k=0==(B[i+24>>2]|0);a:do{if(k){var m=d}else{var q=c+140|0,n=c+3024|0;g=e;var p=d;for(f=i;;){f=f+20|0;if(0!=(B[f>>2]|0)){m=p;break a}B[f>>2]=1;f=p-1|0;g=B[B[g+12>>2]+4>>2]>>2;var r=B[h];if(6==(r|0)){P[B[q>>2]](B[B[g+4]+12>>2])}else{P[r](B[B[g+4]+12>>2],B[n>>2])}r=B[g+5];if(0==(B[r+24>>2]|0)){m=f;break a}r=r+20|0;if(0!=(B[r>>2]|0)){m=f;break a}B[r>>2]=1;p=p-2|0;g=B[g+2];f=g>>2;r=B[h];if(6==(r|0)){P[B[q>>2]](B[B[B[f+1]+16>>2]+12>>2])}else{P[r](B[B[B[f+1]+16>>2]+12>>2],B[n>>2])}f=B[f+5];if(0==(B[f+24>>2]|0)){m=p;break a}}}}while(0);0!=(m|0)&&S(5243812,328,5245284,5243660);c=B[j+743];if(10==(c|0)){P[B[j+36]]()}else{P[c](B[j+756])}}Qd.X=1;function Sd(){}function Ld(){}function xd(){}function zd(){}function Fd(){}function Td(){}function Id(){}function Md(){}function Rd(){}function Od(){}function Pd(){}function wd(){}function Gd(){}function Ad(c,e,d){var f,g,h=e>>2,i=c>>2;g=B[i+740];if(48==(g|0)){P[B[i+33]](6)}else{P[g](6,B[i+756])}g=(c+2968|0)>>2;var j=B[g];if(6==(j|0)){P[B[i+35]](B[B[h+4]+12>>2])}else{P[j](B[B[h+4]+12>>2],B[i+756])}j=B[g];if(6==(j|0)){P[B[i+35]](B[B[B[h+1]+16>>2]+12>>2])}else{P[j](B[B[B[h+1]+16>>2]+12>>2],B[i+756])}h=B[h+5];j=0==(B[h+24>>2]|0);a:do{if(j){var k=d}else{var m=c+140|0,q=c+3024|0,n=e,p=d;for(f=h;;){f=f+20|0;if(0!=(B[f>>2]|0)){k=p;break a}B[f>>2]=1;p=p-1|0;n=B[n+8>>2];f=n>>2;var r=B[g];if(6==(r|0)){P[B[m>>2]](B[B[B[f+1]+16>>2]+12>>2])}else{P[r](B[B[B[f+1]+16>>2]+12>>2],B[q>>2])}f=B[f+5];if(0==(B[f+24>>2]|0)){k=p;break a}}}}while(0);0!=(k|0)&&S(5243812,300,5245296,5243660);c=B[i+743];if(10==(c|0)){P[B[i+36]]()}else{P[c](B[i+756])}}Ad.X=1;function qe(){var c,e,d=O(3028);e=d>>2;if(0==(d|0)){return 0}B[e]=0;c=(d+16|0)>>2;var f=d+88|0;D[E>>3]=0;B[f>>2]=B[E>>2];B[f+4>>2]=B[E+4>>2];B[c]=0;B[c+1]=0;B[c+2]=0;B[c+3]=0;B[c+4]=0;B[c+5]=0;B[e+24]=100130;B[e+30]=0;B[e+31]=0;B[e+33]=28;B[e+34]=36;B[e+35]=46;B[e+36]=40;B[e+3]=42;B[e+29]=4;B[e+37]=24;B[e+740]=48;B[e+741]=34;B[e+742]=6;B[e+743]=10;B[e+744]=22;B[e+745]=50;B[e+756]=0;return d}qe.X=1;function re(c,e){var d,f,g,h=c|0,i=B[h>>2];if((i|0)!=(e|0)){g=(c+2976|0)>>2;f=(c+12|0)>>2;for(d=(c+3024|0)>>2;;){if(i>>>0<e>>>0){if(0==(i|0)){i=B[g];if(22==(i|0)){P[B[f]](100151)}else{P[i](100151,B[d])}se(c,0)}else{if(1==(i|0)){i=B[g];if(22==(i|0)){P[B[f]](100152)}else{P[i](100152,B[d])}te(c)}}}else{if(2==(i|0)){i=B[g];if(22==(i|0)){P[B[f]](100154)}else{P[i](100154,B[d])}ue(c)}else{if(1==(i|0)){i=B[g];if(22==(i|0)){P[B[f]](100153)}else{P[i](100153,B[d])}var i=c,j=i+8|0,k=B[j>>2];0!=(k|0)&&he(k);B[i>>2]=0;B[i+4>>2]=0;B[j>>2]=0}}}i=B[h>>2];if((i|0)==(e|0)){break}}}}re.X=1;function ve(c,e,d){c>>=2;if(100105==(e|0)){B[c+29]=0==(d|0)?4:d}else{if(100106==(e|0)){B[c+740]=0==(d|0)?48:d}else{if(100101==(e|0)){B[c+35]=0==(d|0)?46:d}else{if(100103==(e|0)){B[c+3]=0==(d|0)?42:d}else{if(100107==(e|0)){B[c+742]=0==(d|0)?6:d}else{if(100100==(e|0)){B[c+33]=0==(d|0)?28:d}else{if(100111==(e|0)){B[c+745]=0==(d|0)?50:d}else{if(100112==(e|0)){B[c+37]=0==(d|0)?24:d}else{if(100104==(e|0)){B[c+34]=0==(d|0)?36:d,B[c+30]=0!=(d|0)&1}else{if(100110==(e|0)){B[c+741]=0==(d|0)?34:d,B[c+30]=0!=(d|0)&1}else{if(100102==(e|0)){B[c+36]=0==(d|0)?40:d}else{if(100108==(e|0)){B[c+743]=0==(d|0)?10:d}else{if(100109==(e|0)){B[c+744]=0==(d|0)?22:d}else{if(e=B[c+744],22==(e|0)){P[B[c+3]](100900)}else{P[e](100900,B[c+756])}}}}}}}}}}}}}}}ve.X=1;function we(c,e,d){var f=c>>2,g=y;y=y+24|0;2!=(B[f]|0)&&re(c,2);if(0!=(B[f+38]|0)){if(0!=(xe(c)|0)){B[f+1]=0}else{c=B[f+744];if(22==(c|0)){P[B[f+3]](100902)}else{P[c](100902,B[f+756])}y=g;return}}var h=(B[E>>2]=B[e>>2],B[E+4>>2]=B[e+4>>2],D[E>>3]),i=-1e+150>h,j=i?-1e+150:h,k=1e+150<j,h=g|0;D[E>>3]=k?1e+150:j;B[h>>2]=B[E>>2];B[h+4>>2]=B[E+4>>2];var j=e+8|0,m=(B[E>>2]=B[j>>2],B[E+4>>2]=B[j+4>>2],D[E>>3]),q=(j=-1e+150>m)?-1e+150:m,m=1e+150<q,n=g+8|0;D[E>>3]=m?1e+150:q;B[n>>2]=B[E>>2];B[n+4>>2]=B[E+4>>2];var e=e+16|0,q=(B[E>>2]=B[e>>2],B[E+4>>2]=B[e+4>>2],D[E>>3]),q=(e=-1e+150>q)?-1e+150:q,n=1e+150<q,p=g+16|0;D[E>>3]=n?1e+150:q;B[p>>2]=B[E>>2];B[p+4>>2]=B[E+4>>2];if(i|k|j|m|e|n){if(i=B[f+744],22==(i|0)){P[B[f+3]](100155)}else{P[i](100155,B[f+756])}}if(0==(B[f+2]|0)){if(100>(B[f+39]|0)){f=(c+156|0)>>2;i=B[f];B[(c+184>>2)+(7*i|0)]=d;d=(B[E>>2]=B[h>>2],B[E+4>>2]=B[h+4>>2],D[E>>3]);k=c+28*i+160|0;D[E>>3]=d;B[k>>2]=B[E>>2];B[k+4>>2]=B[E+4>>2];d=h+8|0;d=(B[E>>2]=B[d>>2],B[E+4>>2]=B[d+4>>2],D[E>>3]);k=c+28*i+168|0;D[E>>3]=d;B[k>>2]=B[E>>2];B[k+4>>2]=B[E+4>>2];h=h+16|0;h=(B[E>>2]=B[h>>2],B[E+4>>2]=B[h+4>>2],D[E>>3]);c=c+28*i+176|0;D[E>>3]=h;B[c>>2]=B[E>>2];B[c+4>>2]=B[E+4>>2];B[f]=B[f]+1|0;y=g;return}if(0==(xe(c)|0)){c=B[f+744];if(22==(c|0)){P[B[f+3]](100902)}else{P[c](100902,B[f+756])}y=g;return}}if(0==(ye(c,h,d)|0)){if(c=B[f+744],22==(c|0)){P[B[f+3]](100902)}else{P[c](100902,B[f+756])}}y=g}we.X=1;function xe(c){var e=0,d;var f,g;d=O(152);f=d>>2;if(0==(d|0)){d=0}else{g=d+60|0;var h=d+88|0,i=d+120|0;B[f+1]=d;B[f]=d;B[f+2]=0;B[f+3]=0;B[f+16]=g;B[g>>2]=g;g=(d+68|0)>>2;B[g]=0;B[g+1]=0;B[g+2]=0;B[g+3]=0;B[g+4]=0;B[h>>2]=h;B[f+23]=i;g=(d+96|0)>>2;B[g]=0;B[g+1]=0;B[g+2]=0;B[g+3]=0;B[g+4]=0;B[g+5]=0;B[i>>2]=i;B[f+31]=h;f=(d+128|0)>>2;B[f]=0;B[f+1]=0;B[f+2]=0;B[f+3]=0;B[f+4]=0;B[f+5]=0}B[c+8>>2]=d;if(0==(d|0)){var j;return 0}d=c+156|0;f=c+28*B[d>>2]+160|0;for(h=c+160|0;h>>>0<f>>>0;){if(0==(ye(c,h|0,B[h+24>>2])|0)){j=0;e=579;break}else{h=h+28|0}}if(579==e){return j}B[d>>2]=0;B[c+152>>2]=0;return 1}function ye(c,e,d){var f,g=c+4|0;f=B[g>>2];if(0==(f|0)){if(c=Wd(B[c+8>>2]),0==(c|0)||0==($d(c,B[c+4>>2])|0)){return 0}}else{if(0==(ee(f)|0)){return 0}c=B[f+12>>2]}f=(c+16|0)>>2;B[B[f]+12>>2]=d;var d=(B[E>>2]=B[e>>2],B[E+4>>2]=B[e+4>>2],D[E>>3]),h=B[f]+16|0;D[E>>3]=d;B[h>>2]=B[E>>2];B[h+4>>2]=B[E+4>>2];d=e+8|0;d=(B[E>>2]=B[d>>2],B[E+4>>2]=B[d+4>>2],D[E>>3]);h=B[f]+24|0;D[E>>3]=d;B[h>>2]=B[E>>2];B[h+4>>2]=B[E+4>>2];e=e+16|0;e=(B[E>>2]=B[e>>2],B[E+4>>2]=B[e+4>>2],D[E>>3]);f=B[f]+32|0;D[E>>3]=e;B[f>>2]=B[E>>2];B[f+4>>2]=B[E+4>>2];B[c+28>>2]=1;B[B[c+4>>2]+28>>2]=-1;B[g>>2]=c;return 1}function se(c,e){var d=c|0;0!=(B[d>>2]|0)&&re(c,0);B[d>>2]=1;B[c+156>>2]=0;B[c+152>>2]=0;B[c+8>>2]=0;B[c+3024>>2]=e}function te(c){var e=c|0;1!=(B[e>>2]|0)&&re(c,1);B[e>>2]=2;B[c+4>>2]=0;0<(B[c+156>>2]|0)&&(B[c+152>>2]=1)}function ue(c){var e=c|0;2!=(B[e>>2]|0)&&re(c,2);B[e>>2]=1}function Hd(c,e){var d=c+40|0,d=(B[E>>2]=B[d>>2],B[E+4>>2]=B[d+4>>2],D[E>>3]),f=e+40|0,f=(B[E>>2]=B[f>>2],B[E+4>>2]=B[f+4>>2],D[E>>3]);if(d<f){return 1}if(d!=f){return 0}d=c+48|0;f=e+48|0;d=(B[E>>2]=B[d>>2],B[E+4>>2]=B[d+4>>2],D[E>>3])<=(B[E>>2]=B[f>>2],B[E+4>>2]=B[f+4>>2],D[E>>3]);return d&1}function ze(c,e,d){var f,g,h=0;g=(c+40|0)>>2;var i=(B[E>>2]=B[g],B[E+4>>2]=B[g+1],D[E>>3]);f=(e+40|0)>>2;var j=(B[E>>2]=B[f],B[E+4>>2]=B[f+1],D[E>>3]);if(i<j){h=618}else{if(i!=j){h=621}else{var h=c+48|0,k=e+48|0,h=(B[E>>2]=B[h>>2],B[E+4>>2]=B[h+4>>2],D[E>>3])>(B[E>>2]=B[k>>2],B[E+4>>2]=B[k+4>>2],D[E>>3])?621:618}}if(618==h){if(k=d+40|0,k=(B[E>>2]=B[k>>2],B[E+4>>2]=B[k+4>>2],D[E>>3]),j<k){var m=j,q=i,n=k}else{if(j!=k){h=621}else{var p=e+48|0,r=d+48|0;(B[E>>2]=B[p>>2],B[E+4>>2]=B[p+4>>2],D[E>>3])>(B[E>>2]=B[r>>2],B[E+4>>2]=B[r+4>>2],D[E>>3])?h=621:(m=j,q=i,n=k)}}}621==h&&(S(5243488,61,5245252,5244208),m=(B[E>>2]=B[f],B[E+4>>2]=B[f+1],D[E>>3]),n=d+40|0,q=(B[E>>2]=B[g],B[E+4>>2]=B[g+1],D[E>>3]),n=(B[E>>2]=B[n>>2],B[E+4>>2]=B[n+4>>2],D[E>>3]));q=m-q;m=n-m;g=q+m;if(0>=g){return 0}e=e+48|0;e=(B[E>>2]=B[e>>2],B[E+4>>2]=B[e+4>>2],D[E>>3]);q<m?(c=c+48|0,c=(B[E>>2]=B[c>>2],B[E+4>>2]=B[c+4>>2],D[E>>3]),d=d+48|0,c=e-c+(c-(B[E>>2]=B[d>>2],B[E+4>>2]=B[d+4>>2],D[E>>3]))*(q/g)):(d=d+48|0,d=(B[E>>2]=B[d>>2],B[E+4>>2]=B[d+4>>2],D[E>>3]),c=c+48|0,c=e-d+(d-(B[E>>2]=B[c>>2],B[E+4>>2]=B[c+4>>2],D[E>>3]))*(m/g));return c}ze.X=1;function Ae(c,e,d){var f,g,h=0;g=(c+40|0)>>2;var i=(B[E>>2]=B[g],B[E+4>>2]=B[g+1],D[E>>3]);f=(e+40|0)>>2;var j=(B[E>>2]=B[f],B[E+4>>2]=B[f+1],D[E>>3]);if(i<j){h=633}else{if(i!=j){h=636}else{var h=c+48|0,k=e+48|0,h=(B[E>>2]=B[h>>2],B[E+4>>2]=B[h+4>>2],D[E>>3])>(B[E>>2]=B[k>>2],B[E+4>>2]=B[k+4>>2],D[E>>3])?636:633}}if(633==h){if(k=d+40|0,k=(B[E>>2]=B[k>>2],B[E+4>>2]=B[k+4>>2],D[E>>3]),j<k){var m=j,q=i,n=k}else{if(j!=k){h=636}else{var p=e+48|0,r=d+48|0;(B[E>>2]=B[p>>2],B[E+4>>2]=B[p+4>>2],D[E>>3])>(B[E>>2]=B[r>>2],B[E+4>>2]=B[r+4>>2],D[E>>3])?h=636:(m=j,q=i,n=k)}}}636==h&&(S(5243488,85,5245236,5244208),m=(B[E>>2]=B[f],B[E+4>>2]=B[f+1],D[E>>3]),n=d+40|0,q=(B[E>>2]=B[g],B[E+4>>2]=B[g+1],D[E>>3]),n=(B[E>>2]=B[n>>2],B[E+4>>2]=B[n+4>>2],D[E>>3]));g=m-q;m=n-m;if(0>=g+m){return 0}e=e+48|0;e=(B[E>>2]=B[e>>2],B[E+4>>2]=B[e+4>>2],D[E>>3]);d=d+48|0;c=c+48|0;return c=g*(e-(B[E>>2]=B[d>>2],B[E+4>>2]=B[d+4>>2],D[E>>3]))+m*(e-(B[E>>2]=B[c>>2],B[E+4>>2]=B[c+4>>2],D[E>>3]))}Ae.X=1;function Be(c,e,d){var f,g,h=0;g=(c+48|0)>>2;var i=(B[E>>2]=B[g],B[E+4>>2]=B[g+1],D[E>>3]);f=(e+48|0)>>2;var j=(B[E>>2]=B[f],B[E+4>>2]=B[f+1],D[E>>3]);if(i<j){h=645}else{if(i!=j){h=648}else{var h=c+40|0,k=e+40|0,h=(B[E>>2]=B[h>>2],B[E+4>>2]=B[h+4>>2],D[E>>3])>(B[E>>2]=B[k>>2],B[E+4>>2]=B[k+4>>2],D[E>>3])?648:645}}if(645==h){if(k=d+48|0,k=(B[E>>2]=B[k>>2],B[E+4>>2]=B[k+4>>2],D[E>>3]),j<k){var m=j,q=i,n=k}else{if(j!=k){h=648}else{var p=e+40|0,r=d+40|0;(B[E>>2]=B[p>>2],B[E+4>>2]=B[p+4>>2],D[E>>3])>(B[E>>2]=B[r>>2],B[E+4>>2]=B[r+4>>2],D[E>>3])?h=648:(m=j,q=i,n=k)}}}648==h&&(S(5243488,116,5245028,5243620),m=(B[E>>2]=B[f],B[E+4>>2]=B[f+1],D[E>>3]),n=d+48|0,q=(B[E>>2]=B[g],B[E+4>>2]=B[g+1],D[E>>3]),n=(B[E>>2]=B[n>>2],B[E+4>>2]=B[n+4>>2],D[E>>3]));q=m-q;m=n-m;g=q+m;if(0>=g){return 0}e=e+40|0;e=(B[E>>2]=B[e>>2],B[E+4>>2]=B[e+4>>2],D[E>>3]);q<m?(c=c+40|0,c=(B[E>>2]=B[c>>2],B[E+4>>2]=B[c+4>>2],D[E>>3]),d=d+40|0,c=e-c+(c-(B[E>>2]=B[d>>2],B[E+4>>2]=B[d+4>>2],D[E>>3]))*(q/g)):(d=d+40|0,d=(B[E>>2]=B[d>>2],B[E+4>>2]=B[d+4>>2],D[E>>3]),c=c+40|0,c=e-d+(d-(B[E>>2]=B[c>>2],B[E+4>>2]=B[c+4>>2],D[E>>3]))*(m/g));return c}Be.X=1;function Ce(c,e,d){var f,g,h=0;g=(c+48|0)>>2;var i=(B[E>>2]=B[g],B[E+4>>2]=B[g+1],D[E>>3]);f=(e+48|0)>>2;var j=(B[E>>2]=B[f],B[E+4>>2]=B[f+1],D[E>>3]);if(i<j){h=660}else{if(i!=j){h=663}else{var h=c+40|0,k=e+40|0,h=(B[E>>2]=B[h>>2],B[E+4>>2]=B[h+4>>2],D[E>>3])>(B[E>>2]=B[k>>2],B[E+4>>2]=B[k+4>>2],D[E>>3])?663:660}}if(660==h){if(k=d+48|0,k=(B[E>>2]=B[k>>2],B[E+4>>2]=B[k+4>>2],D[E>>3]),j<k){var m=j,q=i,n=k}else{if(j!=k){h=663}else{var p=e+40|0,r=d+40|0;(B[E>>2]=B[p>>2],B[E+4>>2]=B[p+4>>2],D[E>>3])>(B[E>>2]=B[r>>2],B[E+4>>2]=B[r+4>>2],D[E>>3])?h=663:(m=j,q=i,n=k)}}}663==h&&(S(5243488,140,5245012,5243620),m=(B[E>>2]=B[f],B[E+4>>2]=B[f+1],D[E>>3]),n=d+48|0,q=(B[E>>2]=B[g],B[E+4>>2]=B[g+1],D[E>>3]),n=(B[E>>2]=B[n>>2],B[E+4>>2]=B[n+4>>2],D[E>>3]));g=m-q;m=n-m;if(0>=g+m){return 0}e=e+40|0;e=(B[E>>2]=B[e>>2],B[E+4>>2]=B[e+4>>2],D[E>>3]);d=d+40|0;c=c+40|0;return c=g*(e-(B[E>>2]=B[d>>2],B[E+4>>2]=B[d+4>>2],D[E>>3]))+m*(e-(B[E>>2]=B[c>>2],B[E+4>>2]=B[c+4>>2],D[E>>3]))}Ce.X=1;function De(c){for(var e=0,e=2,d={},f={2:(function(c){e=38;h=c}),I:0};;){try{switch(e){case 2:var g=c+2984|0,h=(Xb=Vb++,d[Xb]=1,Wb[Xb]=e,B[g>>2]=Xb,0),e=38;break;case 38:e=0==(h|0)?6:3;break;case 3:var i=B[(c+2976|0)>>2],e=22==(i|0)?5:4;break;case 4:P[i](100902,B[(c+3024|0)>>2]);e=37;break;case 5:P[B[(c+12|0)>>2]](100902);e=37;break;case 6:var j=c|0,e=1==(B[j>>2]|0)?8:7;break;case 7:re(c,1);e=8;break;case 8:B[j>>2]=0;var k=c+8|0,e=0==(B[k>>2]|0)?9:15;break;case 9:e=0==(B[(c+120|0)>>2]|0)?10:13;break;case 10:e=24==(B[(c+148|0)>>2]|0)?11:13;break;case 11:e=0==(oe(c)|0)?13:12;break;case 12:B[(c+3024|0)>>2]=0;e=37;break;case 13:e=0==(xe(c)|0)?14:15;break;case 14:X(g);case 15:Ie(c);e=0==(Je(c)|0)?16:17;break;case 16:X(g);case 17:var m=B[k>>2],e=0==(B[(c+100|0)>>2]|0)?18:36;break;case 18:var q=c+124|0,e=0==(B[q>>2]|0)?20:19;break;case 19:var n;var p=na,r=0,s=m+88|0,t=B[s>>2];if((t|0)==(s|0)){var u=1;n=u}else{for(var v=t,p=v>>2;;){var C=B[p],z=B[B[p+5]+24>>2];if((B[B[B[p+1]+20>>2]+24>>2]|0)==(z|0)){if(0==(ce(v)|0)){u=0;r=1507;break}}else{B[p+7]=0!=(z|0)?1:-1}if((C|0)==(s|0)){u=1;r=1509;break}else{v=C,p=v>>2}}n=1509==r||1507==r?u:na}e=21;break;case 20:var p=na,r=0,s=m+60|0,x=B[s>>2];if((x|0)==(s|0)){var A=1;n=A}else{v=x;for(p=v>>2;;){var w=B[p];if(0!=(B[p+6]|0)&&0==(Ke(B[p+2])|0)){A=0;r=1493;break}if((w|0)==(s|0)){A=1;r=1492;break}else{v=w,p=v>>2}}n=1492==r||1493==r?A:na}e=21;break;case 21:e=0==(n|0)?22:23;break;case 22:X(g);case 23:ie(m);e=28==(B[(c+132|0)>>2]|0)?24:31;break;case 24:e=40==(B[(c+144|0)>>2]|0)?25:31;break;case 25:e=46==(B[(c+140|0)>>2]|0)?26:31;break;case 26:e=36==(B[(c+136|0)>>2]|0)?27:31;break;case 27:e=48==(B[(c+2960|0)>>2]|0)?28:31;break;case 28:e=10==(B[(c+2972|0)>>2]|0)?29:31;break;case 29:e=6==(B[(c+2968|0)>>2]|0)?30:31;break;case 30:e=34==(B[(c+2964|0)>>2]|0)?34:31;break;case 31:e=0==(B[q>>2]|0)?33:32;break;case 32:ne(c,m);e=34;break;case 33:p=c;v=m;s=r=na;s=(p+128|0)>>2;B[s]=0;var v=v+60|0,H=v|0,Q=B[H>>2];if((Q|0)!=(v|0)){for(var J=Q;;){B[J+20>>2]=0;var K=B[J>>2];if((K|0)==(v|0)){break}else{J=K}}var L=B[H>>2],H=(L|0)==(v|0);a:do{if(!H){J=L;for(r=J>>2;;){if(0!=(B[r+6]|0)){var F=J+20|0;0==(B[F>>2]|0)&&(je(p,B[r+2]),0==(B[F>>2]|0)&&S(5243812,100,5245044,5244324))}var U=B[r];if((U|0)==(v|0)){break a}else{J=U,r=J>>2}}}}while(0);var aa=B[s];0!=(aa|0)&&(me(p,aa),B[s]=0)}e=34;break;case 34:var W=c+148|0,e=24==(B[W>>2]|0)?36:35;break;case 35:var p=m+60|0,V=B[p>>2];if((V|0)!=(p|0)){for(r=V;;){var M=B[r>>2];0==(B[r+24>>2]|0)&&ge(r);if((M|0)==(p|0)){break}else{r=M}}}P[B[W>>2]](m);B[k>>2]=0;B[(c+3024|0)>>2]=0;e=37;break;case 36:he(m);B[(c+3024|0)>>2]=0;B[k>>2]=0;e=37;break;case 37:return}}catch(T){(!T.C||!(T.id in d))&&da(T),f[Wb[T.id]](T.value)}}}De.X=1;function Le(c){var e=c+8|0,e=(B[E>>2]=B[e>>2],B[E+4>>2]=B[e+4>>2],D[E>>3]),d=(B[E>>2]=B[c>>2],B[E+4>>2]=B[c+4>>2],D[E>>3]),e=(0>e?-e:e)>(0>d?-d:d)&1,d=c+16|0,d=(B[E>>2]=B[d>>2],B[E+4>>2]=B[d+4>>2],D[E>>3]),d=0>d?-d:d,c=(e<<3)+c|0,c=(B[E>>2]=B[c>>2],B[E+4>>2]=B[c+4>>2],D[E>>3]);return 0<=c?(c=d>c)?2:e:(c=d>-c)?2:e}function Me(c){var e,d,f;e=B[c+8>>2];var g=e+60|0,h=0;d=g;a:for(;;){for(var i=d;;){var j=B[i>>2];if((j|0)==(g|0)){break a}var k=B[j+8>>2];if(1>(B[k+28>>2]|0)){i=j}else{f=k;f>>=2;var m=h;break}}for(;;){var q=B[f+4],n=q+40|0,i=B[B[f+1]+16>>2],p=i+40|0,n=(B[E>>2]=B[n>>2],B[E+4>>2]=B[n+4>>2],D[E>>3])-(B[E>>2]=B[p>>2],B[E+4>>2]=B[p+4>>2],D[E>>3]),q=q+48|0,i=i+48|0,i=m+n*((B[E>>2]=B[q>>2],B[E+4>>2]=B[q+4>>2],D[E>>3])+(B[E>>2]=B[i>>2],B[E+4>>2]=B[i+4>>2],D[E>>3])),q=B[f+3];if((q|0)==(k|0)){h=i;d=j;continue a}else{f=q,f>>=2,m=i}}}g=e|0;if(0>h){e=B[e>>2];h=(e|0)==(g|0);a:do{if(!h){for(j=e;;){if(d=(j+48|0)>>2,k=-(B[E>>2]=B[d],B[E+4>>2]=B[d+1],D[E>>3]),D[E>>3]=k,B[d]=B[E>>2],B[d+1]=B[E+4>>2],d=B[j>>2],(d|0)==(g|0)){break a}else{j=d}}}}while(0);e=(c+64|0)>>2;h=-(B[E>>2]=B[e],B[E+4>>2]=B[e+1],D[E>>3]);D[E>>3]=h;B[e]=B[E>>2];B[e+1]=B[E+4>>2];e=(c+72|0)>>2;h=-(B[E>>2]=B[e],B[E+4>>2]=B[e+1],D[E>>3]);D[E>>3]=h;B[e]=B[E>>2];B[e+1]=B[E+4>>2];c=(c+80|0)>>2;e=-(B[E>>2]=B[c],B[E+4>>2]=B[c+1],D[E>>3]);D[E>>3]=e;B[c]=B[E>>2];B[c+1]=B[E+4>>2]}}Me.X=1;function Ne(c,e,d,f,g){var h,i,j,k,m,q,n=0,p=c+40|0,r=(B[E>>2]=B[p>>2],B[E+4>>2]=B[p+4>>2],D[E>>3]),s=e+40|0,t=(B[E>>2]=B[s>>2],B[E+4>>2]=B[s+4>>2],D[E>>3]);do{if(r<t){var u=c,v=e}else{if(r==t){var C=c+48|0,z=e+48|0;if((B[E>>2]=B[C>>2],B[E+4>>2]=B[C+4>>2],D[E>>3])<=(B[E>>2]=B[z>>2],B[E+4>>2]=B[z+4>>2],D[E>>3])){u=c;v=e;break}}u=e;v=c}}while(0);var x=d+40|0,A=(B[E>>2]=B[x>>2],B[E+4>>2]=B[x+4>>2],D[E>>3]),w=f+40|0,H=(B[E>>2]=B[w>>2],B[E+4>>2]=B[w+4>>2],D[E>>3]);do{if(A<H){var Q=d,J=f,K=A}else{if(A==H){var L=d+48|0,F=f+48|0;if((B[E>>2]=B[L>>2],B[E+4>>2]=B[L+4>>2],D[E>>3])<=(B[E>>2]=B[F>>2],B[E+4>>2]=B[F+4>>2],D[E>>3])){Q=d;J=f;K=A;break}}Q=f;J=d;K=H}}while(0);var U=u+40|0,aa=(B[E>>2]=B[U>>2],B[E+4>>2]=B[U+4>>2],D[E>>3]);do{if(aa<K){var W=u,V=v,M=Q,T=J,N=K}else{if(aa==K){var R=u+48|0,ja=Q+48|0;if((B[E>>2]=B[R>>2],B[E+4>>2]=B[R+4>>2],D[E>>3])<=(B[E>>2]=B[ja>>2],B[E+4>>2]=B[ja+4>>2],D[E>>3])){W=u;V=v;M=Q;T=J;N=K;break}}W=Q;V=J;M=u;T=v;N=aa}}while(0);q=(M+40|0)>>2;m=(V+40|0)>>2;var wa=(B[E>>2]=B[m],B[E+4>>2]=B[m+1],D[E>>3]);do{if(N<wa){n=708}else{if(N==wa){var mb=M+48|0,rb=V+48|0;if((B[E>>2]=B[mb>>2],B[E+4>>2]=B[mb+4>>2],D[E>>3])<=(B[E>>2]=B[rb>>2],B[E+4>>2]=B[rb+4>>2],D[E>>3])){n=708;break}}var Na=g+40|0;D[E>>3]=.5*(N+wa);B[Na>>2]=B[E>>2];B[Na+4>>2]=B[E+4>>2]}}while(0);a:do{if(708==n){k=(T+40|0)>>2;var eb=(B[E>>2]=B[k],B[E+4>>2]=B[k+1],D[E>>3]);do{if(wa>=eb){if(wa==eb){var Ea=V+48|0,xa=T+48|0;if((B[E>>2]=B[Ea>>2],B[E+4>>2]=B[Ea+4>>2],D[E>>3])<=(B[E>>2]=B[xa>>2],B[E+4>>2]=B[xa+4>>2],D[E>>3])){break}}var Wa=Ae(W,M,V),nb=Ae(W,T,V);if(0>Wa-nb){var qa=-Wa,ra=nb}else{qa=Wa,ra=-nb}var ob=0>qa?0:qa,$=0>ra?0:ra;if(ob>$){var ba=(B[E>>2]=B[k],B[E+4>>2]=B[k+1],D[E>>3]),ka=ba+((B[E>>2]=B[q],B[E+4>>2]=B[q+1],D[E>>3])-ba)*($/($+ob))}else{var ya=(B[E>>2]=B[q],B[E+4>>2]=B[q+1],D[E>>3]),za=(B[E>>2]=B[k],B[E+4>>2]=B[k+1],D[E>>3]),ka=0==$?.5*(ya+za):ya+(za-ya)*(ob/($+ob))}var Oa=g+40|0;D[E>>3]=ka;B[Oa>>2]=B[E>>2];B[Oa+4>>2]=B[E+4>>2];break a}}while(0);var Xa=ze(W,M,V),Fa=ze(M,V,T);if(0>Xa+Fa){var Aa=-Xa,Ga=-Fa}else{Aa=Xa,Ga=Fa}var Ha=0>Aa?0:Aa,fa=0>Ga?0:Ga;if(Ha>fa){var sa=(B[E>>2]=B[m],B[E+4>>2]=B[m+1],D[E>>3]),sb=sa+((B[E>>2]=B[q],B[E+4>>2]=B[q+1],D[E>>3])-sa)*(fa/(fa+Ha))}else{var la=(B[E>>2]=B[q],B[E+4>>2]=B[q+1],D[E>>3]),ma=(B[E>>2]=B[m],B[E+4>>2]=B[m+1],D[E>>3]),sb=0==fa?.5*(la+ma):la+(ma-la)*(Ha/(fa+Ha))}var fb=g+40|0;D[E>>3]=sb;B[fb>>2]=B[E>>2];B[fb+4>>2]=B[E+4>>2]}}while(0);var tb=W+48|0,ga=(B[E>>2]=B[tb>>2],B[E+4>>2]=B[tb+4>>2],D[E>>3]),ta=V+48|0,Pa=(B[E>>2]=B[ta>>2],B[E+4>>2]=B[ta+4>>2],D[E>>3]);do{if(ga<Pa){var ca=W,ua=V}else{if(ga==Pa){var Ya=W+40|0;if((B[E>>2]=B[Ya>>2],B[E+4>>2]=B[Ya+4>>2],D[E>>3])<=(B[E>>2]=B[m],B[E+4>>2]=B[m+1],D[E>>3])){ca=W;ua=V;break}}ca=V;ua=W}}while(0);var pb=M+48|0,Za=(B[E>>2]=B[pb>>2],B[E+4>>2]=B[pb+4>>2],D[E>>3]),ub=T+48|0,vb=(B[E>>2]=B[ub>>2],B[E+4>>2]=B[ub+4>>2],D[E>>3]);do{if(Za<vb){var Ia=M,gb=T,ea=Za}else{if(Za==vb){var ha=T+40|0;if((B[E>>2]=B[q],B[E+4>>2]=B[q+1],D[E>>3])<=(B[E>>2]=B[ha>>2],B[E+4>>2]=B[ha+4>>2],D[E>>3])){Ia=M;gb=T;ea=Za;break}}Ia=T;gb=M;ea=vb}}while(0);var Qa=ca+48|0,Ra=(B[E>>2]=B[Qa>>2],B[E+4>>2]=B[Qa+4>>2],D[E>>3]);do{if(Ra<ea){var oa=ca,ia=ua,Ja=Ia,Ba=gb}else{if(Ra==ea){var wb=ca+40|0,Ka=Ia+40|0;if((B[E>>2]=B[wb>>2],B[E+4>>2]=B[wb+4>>2],D[E>>3])<=(B[E>>2]=B[Ka>>2],B[E+4>>2]=B[Ka+4>>2],D[E>>3])){oa=ca;ia=ua;Ja=Ia;Ba=gb;break}}oa=Ia;ia=gb;Ja=ca;Ba=ua}}while(0);j=(Ja+48|0)>>2;var hb=(B[E>>2]=B[j],B[E+4>>2]=B[j+1],D[E>>3]);i=(ia+48|0)>>2;var Sa=(B[E>>2]=B[i],B[E+4>>2]=B[i+1],D[E>>3]);do{if(hb>=Sa){if(hb==Sa){var $a=Ja+40|0,Ca=ia+40|0;if((B[E>>2]=B[$a>>2],B[E+4>>2]=B[$a+4>>2],D[E>>3])<=(B[E>>2]=B[Ca>>2],B[E+4>>2]=B[Ca+4>>2],D[E>>3])){break}}var Bb=g+48|0;D[E>>3]=.5*(hb+Sa);B[Bb>>2]=B[E>>2];B[Bb+4>>2]=B[E+4>>2];return}}while(0);h=(Ba+48|0)>>2;var xb=(B[E>>2]=B[h],B[E+4>>2]=B[h+1],D[E>>3]);do{if(Sa>=xb){if(Sa==xb){var ib=ia+40|0,La=Ba+40|0;if((B[E>>2]=B[ib>>2],B[E+4>>2]=B[ib+4>>2],D[E>>3])<=(B[E>>2]=B[La>>2],B[E+4>>2]=B[La+4>>2],D[E>>3])){break}}var jb=Ce(oa,Ja,ia),Da=Ce(oa,Ba,ia);if(0>jb-Da){var qb=-jb,ab=Da}else{qb=jb,ab=-Da}var kb=0>qb?0:qb,bb=0>ab?0:ab;if(kb>bb){var Ma=(B[E>>2]=B[h],B[E+4>>2]=B[h+1],D[E>>3]),Fb=Ma+((B[E>>2]=B[j],B[E+4>>2]=B[j+1],D[E>>3])-Ma)*(bb/(bb+kb))}else{var yb=(B[E>>2]=B[j],B[E+4>>2]=B[j+1],D[E>>3]),Hb=(B[E>>2]=B[h],B[E+4>>2]=B[h+1],D[E>>3]),Fb=0==bb?.5*(yb+Hb):yb+(Hb-yb)*(kb/(bb+kb))}var Ib=g+48|0;D[E>>3]=Fb;B[Ib>>2]=B[E>>2];B[Ib+4>>2]=B[E+4>>2];return}}while(0);var zb=Be(oa,Ja,ia),cb=Be(Ja,ia,Ba);if(0>zb+cb){var Cb=-zb,Gb=-cb}else{Cb=zb,Gb=cb}var Mb=0>Cb?0:Cb,Db=0>Gb?0:Gb;if(Mb>Db){var Nb=(B[E>>2]=B[i],B[E+4>>2]=B[i+1],D[E>>3]),nc=Nb+((B[E>>2]=B[j],B[E+4>>2]=B[j+1],D[E>>3])-Nb)*(Db/(Db+Mb))}else{var ac=(B[E>>2]=B[j],B[E+4>>2]=B[j+1],D[E>>3]),oc=(B[E>>2]=B[i],B[E+4>>2]=B[i+1],D[E>>3]),nc=0==Db?.5*(ac+oc):ac+(oc-ac)*(Mb/(Db+Mb))}var pc=g+48|0;D[E>>3]=nc;B[pc>>2]=B[E>>2];B[pc+4>>2]=B[E+4>>2]}Ne.X=1;function Ie(c){var e=y;y=y+24|0;var d=B[c+8>>2],f=d|0,g=c+16|0,g=(B[E>>2]=B[g>>2],B[E+4>>2]=B[g+4>>2],D[E>>3]),h=e|0;D[E>>3]=g;B[h>>2]=B[E>>2];B[h+4>>2]=B[E+4>>2];var i=c+24|0,i=(B[E>>2]=B[i>>2],B[E+4>>2]=B[i+4>>2],D[E>>3]),j=e+8|0;D[E>>3]=i;B[j>>2]=B[E>>2];B[j+4>>2]=B[E+4>>2];var j=c+32|0,j=(B[E>>2]=B[j>>2],B[E+4>>2]=B[j+4>>2],D[E>>3]),k=e+16|0;D[E>>3]=j;B[k>>2]=B[E>>2];B[k+4>>2]=B[E+4>>2];0==g?0==i&0==j?(Oe(d,h),g=1):g=0:g=0;var i=c+40|0,j=c+64|0,m=Le(h),h=(m<<3)+c+40|0;D[E>>3]=0;B[h>>2]=B[E>>2];B[h+4>>2]=B[E+4>>2];h=(m+1|0)%3;k=(h<<3)+c+40|0;D[E>>3]=1;B[k>>2]=B[E>>2];B[k+4>>2]=B[E+4>>2];var k=(m+2|0)%3,q=(k<<3)+c+40|0;D[E>>3]=0;B[q>>2]=B[E>>2];B[q+4>>2]=B[E+4>>2];q=(m<<3)+c+64|0;D[E>>3]=0;B[q>>2]=B[E>>2];B[q+4>>2]=B[E+4>>2];m=(m<<3)+e|0;m=0<(B[E>>2]=B[m>>2],B[E+4>>2]=B[m+4>>2],D[E>>3]);h=(h<<3)+c+64|0;D[E>>3]=0;B[h>>2]=B[E>>2];B[h+4>>2]=B[E+4>>2];h=(k<<3)+c+64|0;D[E>>3]=m?1:-1;B[h>>2]=B[E>>2];B[h+4>>2]=B[E+4>>2];d=B[d>>2];h=(d|0)==(f|0);a:do{if(!h){for(var k=c+48|0,m=c+56|0,q=c+72|0,n=c+80|0,p=d;;){var r=p+16|0,r=(B[E>>2]=B[r>>2],B[E+4>>2]=B[r+4>>2],D[E>>3]),s=r*(B[E>>2]=B[i>>2],B[E+4>>2]=B[i+4>>2],D[E>>3]),t=p+24|0,t=(B[E>>2]=B[t>>2],B[E+4>>2]=B[t+4>>2],D[E>>3]),u=s+t*(B[E>>2]=B[k>>2],B[E+4>>2]=B[k+4>>2],D[E>>3]),s=p+32|0,s=(B[E>>2]=B[s>>2],B[E+4>>2]=B[s+4>>2],D[E>>3]),u=u+s*(B[E>>2]=B[m>>2],B[E+4>>2]=B[m+4>>2],D[E>>3]),v=p+40|0;D[E>>3]=u;B[v>>2]=B[E>>2];B[v+4>>2]=B[E+4>>2];r=r*(B[E>>2]=B[j>>2],B[E+4>>2]=B[j+4>>2],D[E>>3])+t*(B[E>>2]=B[q>>2],B[E+4>>2]=B[q+4>>2],D[E>>3])+s*(B[E>>2]=B[n>>2],B[E+4>>2]=B[n+4>>2],D[E>>3]);t=p+48|0;D[E>>3]=r;B[t>>2]=B[E>>2];B[t+4>>2]=B[E+4>>2];p=B[p>>2];if((p|0)==(f|0)){break a}}}}while(0);0!=(g|0)&&Me(c);y=e}Ie.X=1;function Oe(c,e){var d,f,g,h,i,j,k,m,q,n=y;y=y+96|0;var p=n+24,r=n+48;h=n+72;var s=n+84;d=c|0;q=(n+16|0)>>2;D[E>>3]=-2e+150;B[q]=B[E>>2];B[q+1]=B[E+4>>2];m=(n+8|0)>>2;D[E>>3]=-2e+150;B[m]=B[E>>2];B[m+1]=B[E+4>>2];k=(n|0)>>2;D[E>>3]=-2e+150;B[k]=B[E>>2];B[k+1]=B[E+4>>2];f=(p+16|0)>>2;D[E>>3]=2e+150;B[f]=B[E>>2];B[f+1]=B[E+4>>2];j=(p+8|0)>>2;D[E>>3]=2e+150;B[j]=B[E>>2];B[j+1]=B[E+4>>2];i=(p|0)>>2;D[E>>3]=2e+150;B[i]=B[E>>2];B[i+1]=B[E+4>>2];var t=B[c>>2],u=(t|0)==(d|0);a:do{if(u){var v=-2e+150,C=2e+150,z=-2e+150,x=2e+150,A=-2e+150;g=2e+150}else{for(var w=s|0,H=h|0,Q=s+4|0,J=h+4|0,K=s+8|0,L=h+8|0,F=t,U=2e+150,aa=-2e+150,W=2e+150,V=-2e+150,M=2e+150,T=-2e+150;;){var N=F+16|0,N=(B[E>>2]=B[N>>2],B[E+4>>2]=B[N+4>>2],D[E>>3]);N<U&&(D[E>>3]=N,B[i]=B[E>>2],B[i+1]=B[E+4>>2],B[w>>2]=F,U=N);N>aa&&(D[E>>3]=N,B[k]=B[E>>2],B[k+1]=B[E+4>>2],B[H>>2]=F,aa=N);N=F+24|0;N=(B[E>>2]=B[N>>2],B[E+4>>2]=B[N+4>>2],D[E>>3]);N<W&&(D[E>>3]=N,B[j]=B[E>>2],B[j+1]=B[E+4>>2],B[Q>>2]=F,W=N);N>V&&(D[E>>3]=N,B[m]=B[E>>2],B[m+1]=B[E+4>>2],B[J>>2]=F,V=N);N=F+32|0;N=(B[E>>2]=B[N>>2],B[E+4>>2]=B[N+4>>2],D[E>>3]);N<M&&(D[E>>3]=N,B[f]=B[E>>2],B[f+1]=B[E+4>>2],B[K>>2]=F,M=N);N>T&&(D[E>>3]=N,B[q]=B[E>>2],B[q+1]=B[E+4>>2],B[L>>2]=F,T=N);F=B[F>>2];if((F|0)==(d|0)){v=V;C=W;z=aa;x=U;A=T;g=M;break a}}}}while(0);i=v-C>z-x&1;j=(i<<3)+n|0;f=(i<<3)+p|0;A=A-g>(B[E>>2]=B[j>>2],B[E+4>>2]=B[j+4>>2],D[E>>3])-(B[E>>2]=B[f>>2],B[E+4>>2]=B[f+4>>2],D[E>>3])?2:i;p=(A<<3)+p|0;g=(A<<3)+n|0;if((B[E>>2]=B[p>>2],B[E+4>>2]=B[p+4>>2],D[E>>3])<(B[E>>2]=B[g>>2],B[E+4>>2]=B[g+4>>2],D[E>>3])){f=B[s+(A<<2)>>2];k=B[h+(A<<2)>>2];h=f+16|0;p=(B[E>>2]=B[h>>2],B[E+4>>2]=B[h+4>>2],D[E>>3]);h=(k+16|0)>>2;s=(B[E>>2]=B[h],B[E+4>>2]=B[h+1],D[E>>3]);A=p-s;p=r|0;D[E>>3]=A;B[p>>2]=B[E>>2];B[p+4>>2]=B[E+4>>2];g=f+24|0;j=(B[E>>2]=B[g>>2],B[E+4>>2]=B[g+4>>2],D[E>>3]);g=(k+24|0)>>2;i=(B[E>>2]=B[g],B[E+4>>2]=B[g+1],D[E>>3]);j-=i;m=r+8|0;D[E>>3]=j;B[m>>2]=B[E>>2];B[m+4>>2]=B[E+4>>2];f=f+32|0;m=(B[E>>2]=B[f>>2],B[E+4>>2]=B[f+4>>2],D[E>>3]);f=(k+32|0)>>2;k=(B[E>>2]=B[f],B[E+4>>2]=B[f+1],D[E>>3]);m-=k;r=r+16|0;D[E>>3]=m;B[r>>2]=B[E>>2];B[r+4>>2]=B[E+4>>2];do{if(!u){r=e+8|0;q=e+16|0;v=0;C=t;w=s;x=i;for(z=k;;){var R=C+16|0,R=(B[E>>2]=B[R>>2],B[E+4>>2]=B[R+4>>2],D[E>>3])-w,w=C+24|0,x=(B[E>>2]=B[w>>2],B[E+4>>2]=B[w+4>>2],D[E>>3])-x,w=C+32|0,w=(B[E>>2]=B[w>>2],B[E+4>>2]=B[w+4>>2],D[E>>3])-z,z=j*w-x*m,w=R*m-w*A,R=x*A-R*j,x=R*R+z*z+w*w;x>v?(D[E>>3]=z,B[e>>2]=B[E>>2],B[e+4>>2]=B[E+4>>2],D[E>>3]=w,B[r>>2]=B[E>>2],B[r+4>>2]=B[E+4>>2],D[E>>3]=R,B[q>>2]=B[E>>2],B[q+4>>2]=B[E+4>>2],R=x):R=v;C=B[C>>2];if((C|0)==(d|0)){break}z=(B[E>>2]=B[h],B[E+4>>2]=B[h+1],D[E>>3]);v=R;w=z;x=(B[E>>2]=B[g],B[E+4>>2]=B[g+1],D[E>>3]);z=(B[E>>2]=B[f],B[E+4>>2]=B[f+1],D[E>>3])}if(0<R){y=n;return}}}while(0);d=e>>2;B[d]=0;B[d+1]=0;B[d+2]=0;B[d+3]=0;B[d+4]=0;B[d+5]=0;d=(Le(p)<<3)+e|0;D[E>>3]=1;B[d>>2]=B[E>>2];B[d+4>>2]=B[E+4>>2]}else{t=e+16|0,d=e>>2,B[d]=0,B[d+1]=0,B[d+2]=0,B[d+3]=0,D[E>>3]=1,B[t>>2]=B[E>>2],B[t+4>>2]=B[E+4>>2]}y=n}Oe.X=1;function Pe(c,e,d){var f=B[c+(d<<2)>>2],g=d>>1,h=0==(g|0);a:do{if(h){var i=d}else{for(var j=(f<<3)+e|0,k=d,m=g;;){var q=B[c+(m<<2)>>2],n=B[e+(q<<3)>>2],p=n+40|0,r=(B[E>>2]=B[p>>2],B[E+4>>2]=B[p+4>>2],D[E>>3]),p=B[j>>2],s=p+40|0,s=(B[E>>2]=B[s>>2],B[E+4>>2]=B[s+4>>2],D[E>>3]);if(r<s){i=k;break a}if(r==s&&(n=n+48|0,p=p+48|0,(B[E>>2]=B[n>>2],B[E+4>>2]=B[n+4>>2],D[E>>3])<=(B[E>>2]=B[p>>2],B[E+4>>2]=B[p+4>>2],D[E>>3]))){i=k;break a}B[c+(k<<2)>>2]=q;B[e+(q<<3)+4>>2]=k;q=m>>1;if(0==(q|0)){i=m;break a}else{k=m,m=q}}}}while(0);B[c+(i<<2)>>2]=f;B[e+(f<<3)+4>>2]=i}function Qe(){var c,e,d=O(28);e=d>>2;if(0==(d|0)){return 0}B[e+2]=0;B[e+3]=32;var f=O(132);c=d>>2;B[c]=f;if(0==(f|0)){return Z(d),0}var f=O(264),g=d+4|0;B[g>>2]=f;0==(f|0)?(Z(B[c]),Z(d),c=0):(B[e+5]=0,B[e+4]=0,B[e+6]=26,B[B[c]+4>>2]=1,B[B[g>>2]+8>>2]=0,c=d);return c}function Re(c){Z(B[c+4>>2]);Z(B[c>>2]);Z(c)}function Se(c,e){var d,f,g,h=0,i=B[c>>2];g=i>>2;d=B[c+4>>2];f=d>>2;for(var j=B[(e<<2>>2)+g],k=(j<<3)+d|0,m=c+8|0,q=c+12|0,n=e;;){var p=n<<1,r=B[m>>2];do{if((p|0)<(r|0)){var s=p|1,t=B[(B[(s<<2>>2)+g]<<3>>2)+f],u=t+40|0,v=(B[E>>2]=B[u>>2],B[E+4>>2]=B[u+4>>2],D[E>>3]),u=B[(B[(p<<2>>2)+g]<<3>>2)+f],C=u+40|0,C=(B[E>>2]=B[C>>2],B[E+4>>2]=B[C+4>>2],D[E>>3]);if(v>=C){if(v!=C){s=p;break}t=t+48|0;u=u+48|0;if((B[E>>2]=B[t>>2],B[E+4>>2]=B[t+4>>2],D[E>>3])>(B[E>>2]=B[u>>2],B[E+4>>2]=B[u+4>>2],D[E>>3])){s=p;break}}}else{s=p}}while(0);(s|0)>(B[q>>2]|0)&&(S(5243408,112,5245372,5242988),r=B[m>>2]);p=B[(s<<2>>2)+g];if((s|0)>(r|0)){h=842;break}r=B[k>>2];t=r+40|0;u=(B[E>>2]=B[t>>2],B[E+4>>2]=B[t+4>>2],D[E>>3]);t=B[(p<<3>>2)+f];v=t+40|0;v=(B[E>>2]=B[v>>2],B[E+4>>2]=B[v+4>>2],D[E>>3]);if(u<v){h=843;break}if(u==v&&(r=r+48|0,t=t+48|0,(B[E>>2]=B[r>>2],B[E+4>>2]=B[r+4>>2],D[E>>3])<=(B[E>>2]=B[t>>2],B[E+4>>2]=B[t+4>>2],D[E>>3]))){h=844;break}B[(n<<2>>2)+g]=p;B[((p<<3)+4>>2)+f]=n;n=s}842==h?(f=((n<<2)+i|0)>>2,B[f]=j,d=((j<<3)+d+4|0)>>2,B[d]=n):843==h?(f=((n<<2)+i|0)>>2,B[f]=j,d=((j<<3)+d+4|0)>>2,B[d]=n):844==h&&(f=((n<<2)+i|0)>>2,B[f]=j,d=((j<<3)+d+4|0)>>2,B[d]=n)}Se.X=1;function Te(c,e){var d,f,g;d=c+8|0;var h=B[d>>2]+1|0;B[d>>2]=h;g=(c+12|0)>>2;var i=B[g];if((h<<1|0)>(i|0)){f=(c|0)>>2;var j=B[f],k=c+4|0;d=k>>2;var m=B[d];B[g]=i<<1;i=0==(j|0)?O(i<<3|4):Ue(j,i<<3|4);B[f]=i;if(0==(i|0)){return B[f]=j,2147483647}f=0==(B[d]|0)?O((B[g]<<3)+8|0):Ue(B[d],(B[g]<<3)+8|0);B[d]=f;if(0!=(f|0)){d=k>>2}else{return B[d]=m,2147483647}}else{d=(c+4|0)>>2}k=c+16|0;m=B[k>>2];0==(m|0)?k=h:(B[k>>2]=B[B[d]+(m<<3)+4>>2],k=m);m=c|0;B[B[m>>2]+(h<<2)>>2]=k;B[B[d]+(k<<3)+4>>2]=h;B[B[d]+(k<<3)>>2]=e;0!=(B[c+20>>2]|0)&&Pe(B[m>>2],B[d],h);if(2147483647!=(k|0)){return k}S(5243408,207,5245144,5243992);return 2147483647}Te.X=1;function Ve(c){var e,d=B[c>>2],f=B[c+4>>2],g=d+4|0,h=B[g>>2],i=(h<<3)+f|0,j=B[i>>2];e=(c+8|0)>>2;var k=B[e];if(0>=(k|0)){return j}d=B[d+(k<<2)>>2];B[g>>2]=d;B[f+(d<<3)+4>>2]=1;B[i>>2]=0;g=c+16|0;B[f+(h<<3)+4>>2]=B[g>>2];B[g>>2]=h;f=B[e]-1|0;B[e]=f;if(0>=(f|0)){return j}Se(c,1);return j}function We(c,e){var d,f,g,h;g=0;d=c|0;var i=B[d>>2],j=c+4|0,k=B[j>>2];h=k>>2;0<(e|0)?(B[c+12>>2]|0)<(e|0)?g=872:0==(B[(e<<3>>2)+h]|0)&&(g=872):g=872;872==g&&S(5243408,241,5245164,5243564);g=((e<<3)+k+4|0)>>2;var m=B[g];f=(c+8|0)>>2;var q=B[i+(B[f]<<2)>>2],n=(m<<2)+i|0;B[n>>2]=q;B[((q<<3)+4>>2)+h]=m;q=B[f]-1|0;B[f]=q;if((m|0)<=(q|0)){do{if(2<=(m|0)){f=B[(B[i+(m>>1<<2)>>2]<<3>>2)+h];var q=f+40|0,p=(B[E>>2]=B[q>>2],B[E+4>>2]=B[q+4>>2],D[E>>3]),q=B[(B[n>>2]<<3>>2)+h],r=q+40|0,r=(B[E>>2]=B[r>>2],B[E+4>>2]=B[r+4>>2],D[E>>3]);if(p>=r){if(p==r&&(h=f+48|0,i=q+48|0,(B[E>>2]=B[h>>2],B[E+4>>2]=B[h+4>>2],D[E>>3])<=(B[E>>2]=B[i>>2],B[E+4>>2]=B[i+4>>2],D[E>>3]))){break}Pe(B[d>>2],B[j>>2],m);d=(e<<3)+k|0;d>>=2;B[d]=0;d=c+16|0;d>>=2;j=B[d];B[g]=j;B[d]=e;return}}}while(0);Se(c,m)}d=((e<<3)+k|0)>>2;B[d]=0;d=(c+16|0)>>2;j=B[d];B[g]=j;B[d]=e}We.X=1;function Xe(){var c,e=O(28);c=e>>2;if(0==(e|0)){return 0}var d=Qe();B[e>>2]=d;if(0==(d|0)){return Z(e),0}d=O(128);B[c+1]=d;0==(d|0)?(Re(B[e>>2]),Z(e),c=0):(B[c+3]=0,B[c+4]=32,B[c+5]=0,B[c+6]=26,c=e);return c}function Ye(c){0==(c|0)&&S(5243476,78,5245096,5243396);var e=B[c>>2];0!=(e|0)&&Re(e);e=B[c+8>>2];0!=(e|0)&&Z(e);e=B[c+4>>2];0!=(e|0)&&Z(e);Z(c)}function Ze(c){var e,d,f=y;y=y+400|0;var g=f|0;d=(c+12|0)>>2;var h=O((B[d]<<2)+4|0),i=c+8|0;B[i>>2]=h;if(0==(h|0)){var j=0;y=f;return j}var k=(B[d]-1<<2)+h|0,m=h>>>0>k>>>0;a:do{if(!m){for(var q=h,n=B[c+4>>2];;){B[q>>2]=n;var p=q+4|0;if(p>>>0>k>>>0){break a}else{q=p,n=n+4|0}}}}while(0);B[f>>2]=h;B[f+4>>2]=k;for(var r=f+8|0,s=2016473283,t=g;;){var u=B[t>>2],v=B[r-8+4>>2],C=v>>>0>(u+40|0)>>>0;a:do{if(C){for(var z=v,x=t,A=s,w=u;;){for(var H=z,Q=z+4|0,J=x,K=A,L=w;;){var F=(1539415821*K&-1)+1|0,U=L,aa=((F>>>0)%((H-U+4>>2|0)>>>0)<<2)+L|0,W=B[aa>>2];B[aa>>2]=B[L>>2];B[L>>2]=W;for(var V=Q,M=L-4|0;;){var T=M+4|0,N=B[T>>2],R=B[N>>2],ja=R+40|0,wa=(B[E>>2]=B[ja>>2],B[E+4>>2]=B[ja+4>>2],D[E>>3]),mb=B[W>>2],rb=mb+40|0,Na=(B[E>>2]=B[rb>>2],B[E+4>>2]=B[rb+4>>2],D[E>>3]),eb=wa<Na;b:do{if(eb){var Ea=M,xa=T;e=xa>>2;var Wa=N}else{for(var nb=mb+48|0,qa=M,ra=T,ob=R,$=wa,ba=N;;){if($==Na){var ka=ob+48|0;if((B[E>>2]=B[ka>>2],B[E+4>>2]=B[ka+4>>2],D[E>>3])<=(B[E>>2]=B[nb>>2],B[E+4>>2]=B[nb+4>>2],D[E>>3])){Ea=qa;xa=ra;e=xa>>2;Wa=ba;break b}}var ya=ra+4|0,za=B[ya>>2],Oa=B[za>>2],Xa=Oa+40|0,Fa=(B[E>>2]=B[Xa>>2],B[E+4>>2]=B[Xa+4>>2],D[E>>3]);if(Fa<Na){Ea=ra;xa=ya;e=xa>>2;Wa=za;break b}else{qa=ra,ra=ya,ob=Oa,$=Fa,ba=za}}}}while(0);var Aa=V-4|0,Ga=B[Aa>>2],Ha=B[Ga>>2],fa=Ha+40|0,sa=(B[E>>2]=B[fa>>2],B[E+4>>2]=B[fa+4>>2],D[E>>3]),sb=Na<sa;b:do{if(sb){var la=V,ma=Aa,fb=Ga}else{for(var tb=mb+48|0,ga=V,ta=Aa,Pa=Ha,ca=sa,ua=Ga;;){if(Na==ca){var Ya=Pa+48|0;if((B[E>>2]=B[tb>>2],B[E+4>>2]=B[tb+4>>2],D[E>>3])<=(B[E>>2]=B[Ya>>2],B[E+4>>2]=B[Ya+4>>2],D[E>>3])){la=ga;ma=ta;fb=ua;break b}}var pb=ta-4|0,Za=B[pb>>2],ub=B[Za>>2],vb=ub+40|0,Ia=(B[E>>2]=B[vb>>2],B[E+4>>2]=B[vb+4>>2],D[E>>3]);if(Na<Ia){la=ta;ma=pb;fb=Za;break b}else{ga=ta,ta=pb,Pa=ub,ca=Ia,ua=Za}}}}while(0);B[e]=fb;B[ma>>2]=Wa;if(xa>>>0<ma>>>0){V=ma,M=xa}else{break}}var gb=B[e];B[e]=Wa;B[ma>>2]=gb;var ea=J|0;if((xa-U|0)<(H-ma|0)){break}B[ea>>2]=L;B[J+4>>2]=Ea;var ha=J+8|0;if(z>>>0>(la+40|0)>>>0){J=ha,K=F,L=la}else{var Qa=ha,Ra=F,oa=la,ia=z;break a}}B[ea>>2]=la;B[J+4>>2]=z;var Ja=J+8|0;if(Ea>>>0>(L+40|0)>>>0){z=Ea,x=Ja,A=F,w=L}else{Qa=Ja;Ra=F;oa=L;ia=Ea;break a}}}else{Qa=t,Ra=s,oa=u,ia=v}}while(0);var Ba=oa+4|0,wb=Ba>>>0>ia>>>0;a:do{if(!wb){for(var Ka=Ba;;){var hb=B[Ka>>2],Sa=Ka>>>0>oa>>>0;b:do{if(Sa){for(var $a=Ka;;){var Ca=B[hb>>2],Bb=Ca+40|0,xb=(B[E>>2]=B[Bb>>2],B[E+4>>2]=B[Bb+4>>2],D[E>>3]),ib=$a-4|0,La=B[ib>>2],jb=B[La>>2],Da=jb+40|0,qb=(B[E>>2]=B[Da>>2],B[E+4>>2]=B[Da+4>>2],D[E>>3]);if(xb<qb){var ab=$a;break b}if(xb==qb){var kb=Ca+48|0,bb=jb+48|0;if((B[E>>2]=B[kb>>2],B[E+4>>2]=B[kb+4>>2],D[E>>3])<=(B[E>>2]=B[bb>>2],B[E+4>>2]=B[bb+4>>2],D[E>>3])){ab=$a;break b}}B[$a>>2]=La;if(ib>>>0>oa>>>0){$a=ib}else{ab=ib;break b}}}else{ab=Ka}}while(0);B[ab>>2]=hb;var Ma=Ka+4|0;if(Ma>>>0>ia>>>0){break a}else{Ka=Ma}}}}while(0);var Fb=Qa-8|0;if(Fb>>>0<g>>>0){break}else{r=Qa,s=Ra,t=Fb}}B[c+16>>2]=B[d];B[c+20>>2]=1;var yb=B[c>>2],Hb=B[yb+8>>2],Ib=0<(Hb|0);a:do{if(Ib){for(var zb=Hb;;){Se(yb,zb);var cb=zb-1|0;if(0<(cb|0)){zb=cb}else{break a}}}}while(0);B[yb+20>>2]=1;var Cb=B[i>>2],Gb=B[d]-1|0,Mb=(Gb<<2)+Cb|0;if(0<(Gb|0)){var Db=Cb}else{return j=1,y=f,j}for(;;){var Nb=Db+4|0,nc=B[B[Nb>>2]>>2],ac=nc+40|0,oc=(B[E>>2]=B[ac>>2],B[E+4>>2]=B[ac+4>>2],D[E>>3]),pc=B[B[Db>>2]>>2],Ee=pc+40|0,Fe=(B[E>>2]=B[Ee>>2],B[E+4>>2]=B[Ee+4>>2],D[E>>3]);do{if(oc>=Fe){if(oc==Fe){var Ge=nc+48|0,He=pc+48|0;if((B[E>>2]=B[Ge>>2],B[E+4>>2]=B[Ge+4>>2],D[E>>3])<=(B[E>>2]=B[He>>2],B[E+4>>2]=B[He+4>>2],D[E>>3])){break}}S(5243476,164,5245080,5243316)}}while(0);if(Nb>>>0<Mb>>>0){Db=Nb}else{j=1;break}}y=f;return j}Ze.X=1;function $e(c){var e=B[c+12>>2];if(0==(e|0)){return e=B[c>>2],e=B[B[e+4>>2]+(B[B[e>>2]+4>>2]<<3)>>2]}e=B[B[B[c+8>>2]+(e-1<<2)>>2]>>2];c=B[c>>2]>>2;if(0!=(B[c+2]|0)){var c=B[B[c+1]+(B[B[c]+4>>2]<<3)>>2],d=c+40|0,d=(B[E>>2]=B[d>>2],B[E+4>>2]=B[d+4>>2],D[E>>3]),f=e+40|0,f=(B[E>>2]=B[f>>2],B[E+4>>2]=B[f+4>>2],D[E>>3]);if(d<f||d==f&&(d=c+48|0,f=e+48|0,(B[E>>2]=B[d>>2],B[E+4>>2]=B[d+4>>2],D[E>>3])<=(B[E>>2]=B[f>>2],B[E+4>>2]=B[f+4>>2],D[E>>3]))){return c}}return e}$e.X=1;function af(c,e){var d;if(0!=(B[c+20>>2]|0)){var f=Te(B[c>>2],e);return f}var g=c+12|0,f=B[g>>2];d=f+1|0;B[g>>2]=d;var g=c+16|0,h=B[g>>2];if((d|0)>=(h|0)){d=(c+4|0)>>2;var i=B[d];B[g>>2]=h<<1;g=0==(i|0)?O(h<<3):Ue(i,h<<3);B[d]=g;if(0==(g|0)){return B[d]=i,2147483647}}2147483647==(f|0)&&S(5243476,194,5245060,5243256);B[B[c+4>>2]+(f<<2)>>2]=e;return f^-1}function bf(c){var e,d=0,f=c+12|0,g=B[f>>2];if(0==(g|0)){var h=Ve(B[c>>2]);return h}var i=B[c+8>>2],j=B[B[i+(g-1<<2)>>2]>>2],c=B[c>>2];e=c>>2;do{if(0==(B[e+2]|0)){var k=g}else{k=B[B[e+1]+(B[B[e]+4>>2]<<3)>>2];e=k+40|0;e=(B[E>>2]=B[e>>2],B[E+4>>2]=B[e+4>>2],D[E>>3]);var m=j+40|0,m=(B[E>>2]=B[m>>2],B[E+4>>2]=B[m+4>>2],D[E>>3]);if(e>=m){if(e!=m){k=g;break}k=k+48|0;e=j+48|0;if((B[E>>2]=B[k>>2],B[E+4>>2]=B[k+4>>2],D[E>>3])>(B[E>>2]=B[e>>2],B[E+4>>2]=B[e+4>>2],D[E>>3])){k=g;break}}return h=Ve(c)}}while(0);for(;;){g=k-1|0;B[f>>2]=g;if(0>=(g|0)){h=j;d=981;break}if(0==(B[B[i+(k-2<<2)>>2]>>2]|0)){k=g}else{h=j;d=983;break}}if(981==d||983==d){return h}}bf.X=1;function Je(c){var e;B[c+100>>2]=0;cf(c);var d=0,f=Xe();e=(c+108|0)>>2;B[e]=f;if(0==(f|0)){e=0}else{for(var g=B[c+8>>2],h=g|0,g=g|0;;){g=B[g>>2];if((g|0)==(h|0)){d=1016;break}var i=af(f,g);B[g+56>>2]=i;if(2147483647==(i|0)){break}else{g|=0}}1016==d&&0!=(Ze(f)|0)?e=1:(Ye(B[e]),e=B[e]=0)}if(0==(e|0)){return 0}d=O(20);e=d>>2;0==(d|0)?e=0:(B[e]=0,B[e+1]=d,B[e+2]=d,B[e+3]=c,B[e+4]=18,e=d);B[c+104>>2]=e;0==(e|0)?X(c+2984|0):(df(c,-4e+150),df(c,4e+150));e=(c+108|0)>>2;d=bf(B[e]);f=0==(d|0);a:do{if(!f){for(h=d;;){var g=h,i=B[e],j=$e(i),k=0==(j|0);b:do{if(!k){for(var m=h+40|0,q=h+48|0,n=h+8|0,p=i,r=j;;){var s=r+40|0;if((B[E>>2]=B[s>>2],B[E+4>>2]=B[s+4>>2],D[E>>3])!=(B[E>>2]=B[m>>2],B[E+4>>2]=B[m+4>>2],D[E>>3])){break b}r=r+48|0;if((B[E>>2]=B[r>>2],B[E+4>>2]=B[r+4>>2],D[E>>3])!=(B[E>>2]=B[q>>2],B[E+4>>2]=B[q+4>>2],D[E>>3])){break b}p=bf(p);ef(c,B[n>>2],B[p+8>>2]);p=B[e];r=$e(p);if(0==(r|0)){break b}}}}while(0);ff(c,g);h=bf(B[e]);if(0==(h|0)){break a}}}}while(0);B[c+112>>2]=B[B[B[B[B[c+104>>2]+4>>2]>>2]>>2]+16>>2];d=c+104|0;f=B[d>>2];h=B[B[f+4>>2]>>2];if(0==(h|0)){var t=f}else{f=0;g=h;for(h=g>>2;;){if(0==(B[h+4]|0)&&(0==(B[h+6]|0)&&S(5243336,1188,5245400,5243544),i=f+1|0,0!=(f|0)&&S(5243336,1189,5245400,5243456),f=i),0!=(B[h+2]|0)&&S(5243336,1191,5245400,5243372),gf(g),g=B[d>>2],h=B[B[g+4>>2]>>2],0==(h|0)){t=g;break}else{g=h,h=g>>2}}}d=t|0;f=B[t+4>>2];if((f|0)!=(d|0)){for(;!(h=B[f+4>>2],Z(f),(h|0)==(d|0));){f=h}}Z(t);Ye(B[e]);var c=c+8|0,u;t=0;e=B[c>>2]+60|0;d=B[e>>2];if((d|0)==(e|0)){u=1}else{for(f=d;;){d=B[f>>2];f=B[f+8>>2];h=f+12|0;g=B[h>>2];(g|0)==(f|0)?(S(5243336,1290,5245308,5243920),h=B[h>>2]):h=g;if((B[h+12>>2]|0)==(f|0)&&(h=f+8|0,g=B[h>>2]+28|0,B[g>>2]=B[g>>2]+B[f+28>>2]|0,h=B[B[h>>2]+4>>2]+28|0,B[h>>2]=B[h>>2]+B[B[f+4>>2]+28>>2]|0,0==(ce(f)|0))){u=0;t=1043;break}if((d|0)==(e|0)){u=1;t=1042;break}else{f=d}}u=1042==t||1043==t?u:na}if(0==(u|0)){return 0}ie(B[c>>2]);return 1}Je.X=1;function gf(c){var e=c|0;0!=(B[c+24>>2]|0)&&0!=(B[B[e>>2]+28>>2]|0)&&S(5243336,158,5245416,5243292);B[B[e>>2]+24>>2]=0;var e=B[c+4>>2],d=e+8|0,f=e+4|0;B[B[f>>2]+8>>2]=B[d>>2];B[B[d>>2]+4>>2]=B[f>>2];Z(e);Z(c)}function cf(c){var e,d=0,f=B[c+8>>2]+88|0,g=B[f>>2];if((g|0)!=(f|0)){e=g>>2;a:for(;;){var h=B[e],i=B[e+3],j=B[e+4],k=j+40|0;e=B[B[e+1]+16>>2];var m=e+40|0;do{if((B[E>>2]=B[k>>2],B[E+4>>2]=B[k+4>>2],D[E>>3])==(B[E>>2]=B[m>>2],B[E+4>>2]=B[m+4>>2],D[E>>3])){var q=j+48|0,n=e+48|0;if((B[E>>2]=B[q>>2],B[E+4>>2]=B[q+4>>2],D[E>>3])!=(B[E>>2]=B[n>>2],B[E+4>>2]=B[n+4>>2],D[E>>3])){q=g,n=i}else{if(n=i+12|0,(B[n>>2]|0)==(g|0)){q=g,n=i}else{ef(c,i,g);if(0==(ce(g)|0)){d=1054;break a}q=i;n=B[n>>2]}}}else{q=g,n=i}}while(0);if((B[n+12>>2]|0)==(q|0)){if((n|0)==(q|0)){g=h}else{if((n|0)==(h|0)){d=1060}else{if((n|0)==(B[h+4>>2]|0)){d=1060}else{var p=h}}1060==d&&(d=0,p=B[h>>2]);if(0==(ce(n)|0)){d=1062;break}else{g=p}}if((q|0)==(g|0)){d=1065}else{if((q|0)==(B[g+4>>2]|0)){d=1065}else{var r=g}}1065==d&&(d=0,r=B[g>>2]);if(0==(ce(q)|0)){d=1068;break}else{g=r}}else{g=h}if((g|0)==(f|0)){d=1071;break}else{e=g>>2}}1054==d?X(c+2984|0):1071!=d&&(1068==d?X(c+2984|0):1062==d&&X(c+2984|0))}}cf.X=1;function ef(c,e,d){var f,g=y;y=y+32|0;var h=g+16;f=g>>2;B[f]=0;B[f+1]=0;B[f+2]=0;B[f+3]=0;f=h>>2;B[f]=B[1311378];B[f+1]=B[1311379];B[f+2]=B[1311380];B[f+3]=B[1311381];f=B[e+16>>2];var i=g|0;B[i>>2]=B[f+12>>2];B[g+4>>2]=B[B[d+16>>2]+12>>2];hf(c,f,i,h|0,0);0==($d(e,d)|0)?X(c+2984|0):y=g}function ff(c,e){var d=0;B[c+112>>2]=e;for(var f=B[e+8>>2],g=f;;){var h=B[g+24>>2];if(0!=(h|0)){break}g=B[g+8>>2];if((g|0)==(f|0)){d=1081;break}}1081==d?jf(c,e):(d=kf(h),0==(d|0)&&X(c+2984|0),h=B[B[B[d+4>>2]+8>>2]>>2],f=B[h>>2],h=lf(c,h,0),g=B[h+8>>2],(g|0)==(f|0)?mf(c,d,h):nf(c,d,g,f,f,1))}function jf(c,e){var d,f,g=y;y=y+28|0;f=(e+8|0)>>2;B[g>>2]=B[B[f]+4>>2];var h=B;var i=B[c+104>>2];d=0;for(var j=i+16|0,k=i+12|0,i=i|0;;){var m=B[i+4>>2],i=B[m>>2];if(0==(i|0)){d=26;break}if(0==(P[B[j>>2]](B[k>>2],g,i)|0)){i=m}else{d=25;break}}h=h[(26==d||25==d?m:na)>>2];d=h>>2;var j=B[B[B[d+1]+8>>2]>>2],k=B[d],m=B[j>>2],q=k+4|0;if(0==Ae(B[B[q>>2]+16>>2],e,B[k+16>>2])){of(c,h,e)}else{var m=B[m+4>>2],i=B[m+16>>2],n=i+40|0,n=(B[E>>2]=B[n>>2],B[E+4>>2]=B[n+4>>2],D[E>>3]),q=B[B[q>>2]+16>>2],p=q+40|0,p=(B[E>>2]=B[p>>2],B[E+4>>2]=B[p+4>>2],D[E>>3]);do{if(n<p){var r=h}else{if(n==p){var r=i+48|0,s=q+48|0;if((B[E>>2]=B[r>>2],B[E+4>>2]=B[r+4>>2],D[E>>3])<=(B[E>>2]=B[s>>2],B[E+4>>2]=B[s+4>>2],D[E>>3])){r=h;break}}r=j}}while(0);if(0==(B[d+3]|0)&&0==(B[r+24>>2]|0)){var t=B[f];nf(c,h,t,t,0,1);y=g;return}(r|0)==(h|0)?(f=fe(B[B[f]+4>>2],B[k+12>>2]),0!=(f|0)?t=f:X(c+2984|0)):(f=fe(B[B[m+8>>2]+4>>2],B[f]),0==(f|0)?X(c+2984|0):t=B[f+4>>2]);0==(B[r+24>>2]|0)?(t=pf(c,h,t),f=B[B[t>>2]+28>>2]+B[B[B[B[t+4>>2]+4>>2]>>2]+8>>2]|0,B[t+8>>2]=f,B[t+12>>2]=qf(B[c+96>>2],f)):0==(rf(r,t)|0)&&X(c+2984|0);ff(c,e)}y=g}jf.X=1;function kf(c){for(var e=B[B[c>>2]+16>>2];;){var d=B[B[B[c+4>>2]+4>>2]>>2],f=d,g=B[d>>2];if((B[g+16>>2]|0)==(e|0)){c=f}else{break}}if(0==(B[d+24>>2]|0)){return f}e=d+4|0;g=fe(B[B[B[B[B[e>>2]+8>>2]>>2]>>2]+4>>2],B[g+12>>2]);return 0==(g|0)||0==(rf(f,g)|0)?0:f=B[B[B[e>>2]+4>>2]>>2]}kf.X=1;function qf(c,e){if(100130==(c|0)){var d=e&1}else{100134==(c|0)?d=2<(e+1|0)>>>0&1:100132==(c|0)?d=0<(e|0)&1:100133==(c|0)?d=e>>>31:100131==(c|0)?d=0!=(e|0)&1:(S(5243336,253,5245356,5243728),d=0)}return d}function lf(c,e,d){var f=0,g=B[e>>2];if((e|0)==(d|0)){var h;return g}for(;;){B[e+24>>2]=0;var i=B[B[B[e+4>>2]+8>>2]>>2],j=i,k=i,m=B[k>>2];if((B[m+16>>2]|0)==(B[g+16>>2]|0)){i=g+8|0}else{if(0==(B[i+24>>2]|0)){f=1138;break}i=g+8|0;m=fe(B[B[i>>2]+4>>2],B[m+4>>2]);if(0==(m|0)){f=1140;break}if(0==(rf(j,m)|0)){f=1142;break}}if((B[i>>2]|0)!=(m|0)){if(0==($d(B[B[m+4>>2]+12>>2],m)|0)){f=1145;break}if(0==($d(g,m)|0)){f=1147;break}}sf(e);k=B[k>>2];if((j|0)==(d|0)){h=k;f=1151;break}else{e=j,g=k}}if(1138==f){return sf(e),g}if(1140==f){X(c+2984|0)}else{if(1147==f){X(c+2984|0)}else{if(1142==f){X(c+2984|0)}else{if(1145==f){X(c+2984|0)}else{if(1151==f){return h}}}}}}lf.X=1;function mf(c,e,d){var f,g=0,h=d+8|0,i=B[h>>2],j=B[B[B[e+4>>2]+8>>2]>>2],k=B[e>>2],m=B[j>>2];f=(m+4|0)>>2;(B[B[k+4>>2]+16>>2]|0)!=(B[B[f]+16>>2]|0)&&tf(c,e);var q=k+16|0,n=B[q>>2],p=n+40|0,r=(B[E>>2]=B[p>>2],B[E+4>>2]=B[p+4>>2],D[E>>3]),p=c+112|0,s=B[p>>2],t=s+40|0,t=(B[E>>2]=B[t>>2],B[E+4>>2]=B[t+4>>2],D[E>>3]);if(r==t){if(n=n+48|0,r=s+48|0,(B[E>>2]=B[n>>2],B[E+4>>2]=B[n+4>>2],D[E>>3])!=(B[E>>2]=B[r>>2],B[E+4>>2]=B[r+4>>2],D[E>>3])){var u=0,v=e,C=i,z=s,x=t}else{0==($d(B[B[i+4>>2]+12>>2],k)|0)&&X(c+2984|0),e=kf(e),0==(e|0)?X(c+2984|0):(v=B[B[B[e+4>>2]+8>>2]>>2],C=B[v>>2],lf(c,v,j),z=B[p>>2],x=z+40|0,u=1,v=e,x=(B[E>>2]=B[x>>2],B[E+4>>2]=B[x+4>>2],D[E>>3]))}}else{u=0,v=e,C=i,z=s,x=t}m=B[m+16>>2];p=m+40|0;p=(B[E>>2]=B[p>>2],B[E+4>>2]=B[p+4>>2],D[E>>3]);if(p==x){if(x=m+48|0,z=z+48|0,(B[E>>2]=B[x>>2],B[E+4>>2]=B[x+4>>2],D[E>>3])!=(B[E>>2]=B[z>>2],B[E+4>>2]=B[z+4>>2],D[E>>3])){g=1167}else{if(0==($d(d,B[B[f]+12>>2])|0)){X(c+2984|0)}else{var A=lf(c,j,0)}}}else{g=1167}if(1167==g){if(0!=(u|0)){A=d}else{d=B[q>>2];j=d+40|0;j=(B[E>>2]=B[j>>2],B[E+4>>2]=B[j+4>>2],D[E>>3]);if(p<j){g=1172}else{if(p!=j){var w=k}else{j=m+48|0,d=d+48|0,(B[E>>2]=B[j>>2],B[E+4>>2]=B[j+4>>2],D[E>>3])>(B[E>>2]=B[d>>2],B[E+4>>2]=B[d+4>>2],D[E>>3])?w=k:g=1172}}1172==g&&(w=B[B[f]+12>>2]);f=fe(B[B[h>>2]+4>>2],w);0==(f|0)&&X(c+2984|0);g=B[f+8>>2];nf(c,v,f,g,g,0);B[B[B[f+4>>2]+24>>2]+24>>2]=1;uf(c,v);return}}nf(c,v,B[A+8>>2],C,C,1)}mf.X=1;function nf(c,e,d,f,g,h){for(var i,j,k,m=0;;){k=B[d+16>>2];var q=k+40|0,q=(B[E>>2]=B[q>>2],B[E+4>>2]=B[q+4>>2],D[E>>3]),n=d+4|0,p=B[n>>2],r=B[p+16>>2],s=r+40|0,s=(B[E>>2]=B[s>>2],B[E+4>>2]=B[s+4>>2],D[E>>3]);do{if(q<s){var t=p}else{if(q==s){var t=k+48|0,u=r+48|0;if((B[E>>2]=B[t>>2],B[E+4>>2]=B[t+4>>2],D[E>>3])<=(B[E>>2]=B[u>>2],B[E+4>>2]=B[u+4>>2],D[E>>3])){t=p;break}}S(5243336,361,5245496,5243228);t=B[n>>2]}}while(0);pf(c,e,t);d=B[d+8>>2];if((d|0)==(f|0)){break}}f=B[B[B[e+4>>2]+8>>2]>>2];d=B[B[f>>2]+4>>2];q=0==(g|0)?B[d+8>>2]:g;n=(B[d+16>>2]|0)==(B[q+16>>2]|0);a:do{if(n){p=c+96|0;r=e;s=q;k=s>>2;var t=1,v=f;j=v>>2;u=d;g=u>>2;b:for(;;){if((B[g+2]|0)!=(s|0)){if(0==($d(B[B[g+1]+12>>2],u)|0)){m=1191;break}if(0==($d(B[B[k+1]+12>>2],u)|0)){m=1193;break}}i=(u+28|0)>>2;var C=B[r+8>>2]-B[i]|0;B[j+2]=C;B[j+3]=qf(B[p>>2],C);B[r+20>>2]=1;do{if(0==(t|0)&&0!=(vf(c,r)|0)&&(B[i]=B[i]+B[k+7]|0,C=B[g+1]+28|0,B[C>>2]=B[C>>2]+B[B[k+1]+28>>2]|0,gf(r),0==(ce(s)|0))){m=1198;break b}}while(0);j=B[B[B[j+1]+8>>2]>>2];i=B[B[j>>2]+4>>2];if((B[i+16>>2]|0)==(B[g+4]|0)){r=v,s=u,k=s>>2,t=0,v=j,j=v>>2,u=i,g=u>>2}else{var z=v,x=j,A=i;break a}}1191==m?X(c+2984|0):1193==m?X(c+2984|0):1198==m&&X(c+2984|0)}else{z=e,x=f,A=d}}while(0);B[z+20>>2]=1;(B[z+8>>2]-B[A+28>>2]|0)!=(B[x+8>>2]|0)&&S(5243336,403,5245496,5243052);0!=(h|0)&&uf(c,z)}nf.X=1;function pf(c,e,d){var f,g=O(28);f=g>>2;0==(g|0)&&X(c+2984|0);B[f]=d;e=Vd(B[c+104>>2],B[e+4>>2],g);B[f+1]=e;if(0==(e|0)){X(c+2984|0)}else{return B[f+6]=0,B[f+4]=0,B[f+5]=0,B[d+24>>2]=g}}function vf(c,e){var d;d=e+4|0;var f=B[B[B[d>>2]+8>>2]>>2],g=B[e>>2],h=B[f>>2],i=g+16|0,j=B[i>>2],k=j+40|0,m=(B[E>>2]=B[k>>2],B[E+4>>2]=B[k+4>>2],D[E>>3]),k=h+16|0,q=B[k>>2],n=q+40|0,n=(B[E>>2]=B[n>>2],B[E+4>>2]=B[n+4>>2],D[E>>3]);do{if(m>=n){if(m==n){var p=j+48|0,r=q+48|0;if((B[E>>2]=B[p>>2],B[E+4>>2]=B[p+4>>2],D[E>>3])<=(B[E>>2]=B[r>>2],B[E+4>>2]=B[r+4>>2],D[E>>3])){break}}p=g+4|0;if(0>Ae(B[B[p>>2]+16>>2],q,j)){return g=0}B[e+20>>2]=1;B[B[B[B[d>>2]+4>>2]>>2]+20>>2]=1;0==(ee(B[p>>2])|0)&&X(c+2984|0);if(0==($d(B[B[h+4>>2]+12>>2],g)|0)){X(c+2984|0)}else{return g=1}}}while(0);d=(h+4|0)>>2;if(0<Ae(B[B[d]+16>>2],j,q)){return 0}i=B[i>>2];j=i+40|0;k=B[k>>2];h=k+40|0;if((B[E>>2]=B[j>>2],B[E+4>>2]=B[j+4>>2],D[E>>3])==(B[E>>2]=B[h>>2],B[E+4>>2]=B[h+4>>2],D[E>>3])){if(j=i+48|0,h=k+48|0,(B[E>>2]=B[j>>2],B[E+4>>2]=B[j+4>>2],D[E>>3])==(B[E>>2]=B[h>>2],B[E+4>>2]=B[h+4>>2],D[E>>3])){if((i|0)==(k|0)){return 1}f=B[c+108>>2];i=B[i+56>>2];k=0;if(-1<(i|0)){We(B[f>>2],i)}else{i^=-1;j=f+4|0;if((B[f+16>>2]|0)>(i|0)){if(h=B[j>>2],0==(B[h+(i<<2)>>2]|0)){k=988}else{var s=h}}else{k=988}988==k&&(S(5243476,254,5245124,5243112),s=B[j>>2]);B[s+(i<<2)>>2]=0;s=f+12|0;k=B[s>>2];if(0<(k|0)){for(f=B[f+8>>2];;){k=k-1|0;if(0!=(B[B[f+(k<<2)>>2]>>2]|0)){break}B[s>>2]=k;if(0>=(k|0)){break}}}}ef(c,B[B[d]+12>>2],g);return 1}}0==(ee(B[d])|0)&&X(c+2984|0);0==($d(g,B[B[d]+12>>2])|0)&&X(c+2984|0);B[f+20>>2]=1;return B[e+20>>2]=1}vf.X=1;function uf(c,e){var d,f,g=0,h=c+112|0,i=B[B[B[e+4>>2]+8>>2]>>2],j=e;a:for(;;){if(0!=(B[i+20>>2]|0)){j=i,i=B[B[B[i+4>>2]+8>>2]>>2]}else{if(0==(B[j+20>>2]|0)){var k=B[B[B[j+4>>2]+4>>2]>>2];if(0==(k|0)){g=1266;break}if(0==(B[k+20>>2]|0)){g=1267;break}else{var m=j;f=m>>2;var q=k;d=q>>2}}else{m=i,f=m>>2,q=j,d=q>>2}B[d+5]=0;var n=B[d],p=B[f];do{if((B[B[n+4>>2]+16>>2]|0)==(B[B[p+4>>2]+16>>2]|0)){var i=p,j=n,k=m,r=q}else{if(0==(wf(c,q)|0)){i=p,j=n,k=m,r=q}else{if(0!=(B[f+6]|0)){gf(m);if(0==(ce(p)|0)){g=1246;break a}k=B[B[B[d+1]+8>>2]>>2];i=B[k>>2];j=n;r=q}else{if(0==(B[d+6]|0)){i=p,j=n,k=m,r=q}else{gf(q);if(0==(ce(n)|0)){g=1250;break a}r=B[B[B[f+1]+4>>2]>>2];i=p;j=B[r>>2];k=m}}}}}while(0);f=j+16|0;m=i+16|0;d=(B[f>>2]|0)==(B[m>>2]|0);b:do{if(!d){q=B[B[j+4>>2]+16>>2];n=B[B[i+4>>2]+16>>2];do{if((q|0)!=(n|0)&&0==(B[r+24>>2]|0)&&0==(B[k+24>>2]|0)&&(p=B[h>>2],(q|0)==(p|0)|(n|0)==(p|0))){if(0==(tf(c,r)|0)){break b}else{g=1265;break a}}}while(0);vf(c,r)}}while(0);if((B[f>>2]|0)!=(B[m>>2]|0)){i=k,j=r}else{if(f=j+4|0,m=B[i+4>>2],(B[B[f>>2]+16>>2]|0)!=(B[m+16>>2]|0)){i=k,j=r}else{i=i+28|0;B[i>>2]=B[i>>2]+B[j+28>>2]|0;i=m+28|0;B[i>>2]=B[i>>2]+B[B[f>>2]+28>>2]|0;gf(r);if(0==(ce(j)|0)){g=1262;break}i=k;j=B[B[B[k+4>>2]+4>>2]>>2]}}}}1266!=g&&1267!=g&&(1250==g?X(c+2984|0):1262==g?X(c+2984|0):1265!=g&&1246==g&&X(c+2984|0))}uf.X=1;function wf(c,e){var d,f=e>>2,g=e+4|0,h=B[B[B[g>>2]+8>>2]>>2],i=B[f],j=B[h>>2],k=i+4|0,m=B[B[k>>2]+16>>2];d=m+40|0;var q=(B[E>>2]=B[d>>2],B[E+4>>2]=B[d+4>>2],D[E>>3]);d=(j+4|0)>>2;var n=B[B[d]+16>>2],p=n+40|0,p=(B[E>>2]=B[p>>2],B[E+4>>2]=B[p+4>>2],D[E>>3]);if(q==p){var r=m+48|0,s=n+48|0;(B[E>>2]=B[r>>2],B[E+4>>2]=B[r+4>>2],D[E>>3])!=(B[E>>2]=B[s>>2],B[E+4>>2]=B[s+4>>2],D[E>>3])?(k=m,m=q):(S(5243336,581,5245456,5243736),k=B[B[k>>2]+16>>2],m=k+40|0,n=B[B[d]+16>>2],p=n+40|0,m=(B[E>>2]=B[m>>2],B[E+4>>2]=B[m+4>>2],D[E>>3]),p=(B[E>>2]=B[p>>2],B[E+4>>2]=B[p+4>>2],D[E>>3]))}else{k=m,m=q}do{if(m>=p){if(m==p&&(p=k+48|0,m=n+48|0,(B[E>>2]=B[p>>2],B[E+4>>2]=B[p+4>>2],D[E>>3])<=(B[E>>2]=B[m>>2],B[E+4>>2]=B[m+4>>2],D[E>>3]))){break}if(0<Ae(n,k,B[j+16>>2])){return f=0}B[h+20>>2]=1;B[f+5]=1;g=ee(j);0==(g|0)&&X(c+2984|0);0==($d(B[i+12>>2],B[d])|0)&&X(c+2984|0);B[B[B[g+4>>2]+20>>2]+24>>2]=B[f+3];return f=1}}while(0);if(0>Ae(k,n,B[i+16>>2])){return 0}B[f+5]=1;B[B[B[B[g>>2]+4>>2]>>2]+20>>2]=1;i=ee(i);0==(i|0)&&X(c+2984|0);0==($d(B[d],i)|0)&&X(c+2984|0);B[B[i+20>>2]+24>>2]=B[f+3];return 1}wf.X=1;function xf(c){for(var e=B[B[B[c>>2]+4>>2]+16>>2];;){var d=c=B[B[B[c+4>>2]+4>>2]>>2];if((B[B[B[c>>2]+4>>2]+16>>2]|0)==(e|0)){c=d}else{break}}return d}function yf(c,e,d,f){var g,h;h=e+40|0;g=(B[E>>2]=B[h>>2],B[E+4>>2]=B[h+4>>2],D[E>>3]);h=c+40|0;h=(B[E>>2]=B[h>>2],B[E+4>>2]=B[h+4>>2],D[E>>3]);g-=h;var i=e+48|0,j=(B[E>>2]=B[i>>2],B[E+4>>2]=B[i+4>>2],D[E>>3]),i=c+48|0,i=(B[E>>2]=B[i>>2],B[E+4>>2]=B[i+4>>2],D[E>>3]),j=j-i;g=(0>g?-g:g)+(0>j?-j:j);j=d+40|0;h=(B[E>>2]=B[j>>2],B[E+4>>2]=B[j+4>>2],D[E>>3])-h;j=d+48|0;i=(B[E>>2]=B[j>>2],B[E+4>>2]=B[j+4>>2],D[E>>3])-i;i=(0>h?-h:h)+(0>i?-i:i);h=g+i;i=.5*i/h;ic[f>>2]=i;j=.5*g/h;h=(f+4|0)>>2;ic[h]=j;var k=e+16|0,m=d+16|0;g=(c+16|0)>>2;i=i*(B[E>>2]=B[k>>2],B[E+4>>2]=B[k+4>>2],D[E>>3])+j*(B[E>>2]=B[m>>2],B[E+4>>2]=B[m+4>>2],D[E>>3])+(B[E>>2]=B[g],B[E+4>>2]=B[g+1],D[E>>3]);D[E>>3]=i;B[g]=B[E>>2];B[g+1]=B[E+4>>2];i=e+24|0;j=d+24|0;g=(c+24|0)>>2;i=ic[f>>2]*(B[E>>2]=B[i>>2],B[E+4>>2]=B[i+4>>2],D[E>>3])+ic[h]*(B[E>>2]=B[j>>2],B[E+4>>2]=B[j+4>>2],D[E>>3])+(B[E>>2]=B[g],B[E+4>>2]=B[g+1],D[E>>3]);D[E>>3]=i;B[g]=B[E>>2];B[g+1]=B[E+4>>2];e=e+32|0;d=d+32|0;c=(c+32|0)>>2;f=ic[f>>2]*(B[E>>2]=B[e>>2],B[E+4>>2]=B[e+4>>2],D[E>>3])+ic[h]*(B[E>>2]=B[d>>2],B[E+4>>2]=B[d+4>>2],D[E>>3])+(B[E>>2]=B[c],B[E+4>>2]=B[c+1],D[E>>3]);D[E>>3]=f;B[c]=B[E>>2];B[c+1]=B[E+4>>2]}yf.X=1;function hf(c,e,d,f,g){var h=c>>2,i=y;y=y+24|0;var j=e+16|0,k=(B[E>>2]=B[j>>2],B[E+4>>2]=B[j+4>>2],D[E>>3]),j=i|0;D[E>>3]=k;B[j>>2]=B[E>>2];B[j+4>>2]=B[E+4>>2];var k=e+24|0,k=(B[E>>2]=B[k>>2],B[E+4>>2]=B[k+4>>2],D[E>>3]),m=i+8|0;D[E>>3]=k;B[m>>2]=B[E>>2];B[m+4>>2]=B[E+4>>2];k=e+32|0;k=(B[E>>2]=B[k>>2],B[E+4>>2]=B[k+4>>2],D[E>>3]);m=i+16|0;D[E>>3]=k;B[m>>2]=B[E>>2];B[m+4>>2]=B[E+4>>2];k=e+12|0;e=k>>2;B[e]=0;m=B[h+745];if(50==(m|0)){P[B[h+29]](j,d,f,k)}else{P[m](j,d,f,k,B[h+756])}if(0==(B[e]|0)){if(0==(g|0)){B[e]=B[d>>2]}else{if(c=c+100|0,0==(B[c>>2]|0)){d=B[h+744];if(22==(d|0)){P[B[h+3]](100156)}else{P[d](100156,B[h+756])}B[c>>2]=1}}}y=i}hf.X=1;function sf(c){var e=B[c>>2],d=B[e+20>>2];B[d+24>>2]=B[c+12>>2];B[d+8>>2]=e;gf(c)}function rf(c,e){var d=c+24|0;0==(B[d>>2]|0)&&S(5243336,171,5245384,5243544);var f=c|0;if(0==(ce(B[f>>2])|0)){return 0}B[d>>2]=0;B[f>>2]=e;B[e+24>>2]=c;return 1}function tf(c,e){var d,f,g,h,i,j,k,m,q,n,p,r,s,t,u,v,C,z,x=0,A=y;y=y+60|0;z=(e+4|0)>>2;var w=B[B[B[z]+8>>2]>>2];C=w>>2;var H=e|0,Q=B[H>>2],J=B[C];v=(Q+16|0)>>2;var K=B[v];u=(J+16|0)>>2;var L=B[u];t=(Q+4|0)>>2;var F=B[B[t]+16>>2];s=(J+4|0)>>2;var U=B[B[s]+16>>2];r=(U+40|0)>>2;p=(F+40|0)>>2;if((B[E>>2]=B[r],B[E+4>>2]=B[r+1],D[E>>3])==(B[E>>2]=B[p],B[E+4>>2]=B[p+1],D[E>>3])){var aa=U+48|0,W=F+48|0;(B[E>>2]=B[aa>>2],B[E+4>>2]=B[aa+4>>2],D[E>>3])==(B[E>>2]=B[W>>2],B[E+4>>2]=B[W+4>>2],D[E>>3])&&S(5243336,628,5245476,5242960)}n=(c+112|0)>>2;0<Ae(F,B[n],K)&&S(5243336,629,5245476,5242904);0>Ae(U,B[n],L)&&S(5243336,630,5245476,5244412);var V=B[n];(K|0)==(V|0)|(L|0)==(V|0)&&S(5243336,631,5245476,5244336);0==(B[e+24>>2]|0)?0!=(B[C+6]|0)&&(x=1339):x=1339;1339==x&&S(5243336,632,5245476,5244260);if((K|0)==(L|0)){var M=0;y=A;return M}q=(K+48|0)>>2;var T=(B[E>>2]=B[q],B[E+4>>2]=B[q+1],D[E>>3]);m=(F+48|0)>>2;var N=(B[E>>2]=B[m],B[E+4>>2]=B[m+1],D[E>>3]);k=(L+48|0)>>2;var R=(B[E>>2]=B[k],B[E+4>>2]=B[k+1],D[E>>3]);j=(U+48|0)>>2;var ja=(B[E>>2]=B[j],B[E+4>>2]=B[j+1],D[E>>3]);if((T>N?N:T)>(R<ja?ja:R)){return M=0,y=A,M}i=(K+40|0)>>2;var wa=(B[E>>2]=B[i],B[E+4>>2]=B[i+1],D[E>>3]);h=(L+40|0)>>2;var mb=(B[E>>2]=B[h],B[E+4>>2]=B[h+1],D[E>>3]);if(wa<mb){x=1344}else{if(wa!=mb|T>R){if(0>Ae(F,L,K)){return M=0,y=A,M}}else{x=1344}}if(1344==x&&0<Ae(U,K,L)){return M=0,y=A,M}Ne(F,K,U,L,A);var rb=(B[E>>2]=B[q],B[E+4>>2]=B[q+1],D[E>>3]),Na=(B[E>>2]=B[m],B[E+4>>2]=B[m+1],D[E>>3]);g=(A+48|0)>>2;var eb=(B[E>>2]=B[g],B[E+4>>2]=B[g+1],D[E>>3]);(rb>Na?Na:rb)>eb&&S(5243336,651,5245476,5244168);var Ea=(B[E>>2]=B[k],B[E+4>>2]=B[k+1],D[E>>3]),xa=(B[E>>2]=B[j],B[E+4>>2]=B[j+1],D[E>>3]);eb>(Ea<xa?xa:Ea)&&S(5243336,652,5245476,5243952);var Wa=(B[E>>2]=B[r],B[E+4>>2]=B[r+1],D[E>>3]),nb=(B[E>>2]=B[p],B[E+4>>2]=B[p+1],D[E>>3]);f=(A+40|0)>>2;var qa=(B[E>>2]=B[f],B[E+4>>2]=B[f+1],D[E>>3]);(Wa>nb?nb:Wa)>qa&&S(5243336,653,5245476,5243880);var ra=(B[E>>2]=B[h],B[E+4>>2]=B[h+1],D[E>>3]),ob=(B[E>>2]=B[i],B[E+4>>2]=B[i+1],D[E>>3]);qa>(ra<ob?ob:ra)&&S(5243336,654,5245476,5243824);var $=B[n],ba=$+40|0,ka=(B[E>>2]=B[ba>>2],B[E+4>>2]=B[ba+4>>2],D[E>>3]);if(qa<ka){var ya=$+48|0,za=(B[E>>2]=B[ya>>2],B[E+4>>2]=B[ya+4>>2],D[E>>3]),x=1358}else{if(qa!=ka){var Oa=qa,Xa=eb}else{var Fa=$+48|0,Aa=(B[E>>2]=B[Fa>>2],B[E+4>>2]=B[Fa+4>>2],D[E>>3]);eb>Aa?(Oa=qa,Xa=eb):(za=Aa,x=1358)}}1358==x&&(D[E>>3]=ka,B[f]=B[E>>2],B[f+1]=B[E+4>>2],D[E>>3]=za,B[g]=B[E>>2],B[g+1]=B[E+4>>2],Oa=ka,Xa=za);var Ga=(B[E>>2]=B[i],B[E+4>>2]=B[i+1],D[E>>3]),Ha=(B[E>>2]=B[h],B[E+4>>2]=B[h+1],D[E>>3]);if(Ga<Ha){var fa=K,sa=Ga}else{Ga==Ha&&(B[E>>2]=B[q],B[E+4>>2]=B[q+1],D[E>>3])<=(B[E>>2]=B[k],B[E+4>>2]=B[k+1],D[E>>3])?(fa=K,sa=Ga):(fa=L,sa=Ha)}if(sa<Oa){var sb=fa+48|0,la=(B[E>>2]=B[sb>>2],B[E+4>>2]=B[sb+4>>2],D[E>>3]),x=1367}else{if(sa!=Oa){var ma=Oa,fb=Xa}else{var tb=fa+48|0,ga=(B[E>>2]=B[tb>>2],B[E+4>>2]=B[tb+4>>2],D[E>>3]);ga>Xa?(ma=Oa,fb=Xa):(la=ga,x=1367)}}1367==x&&(D[E>>3]=sa,B[f]=B[E>>2],B[f+1]=B[E+4>>2],D[E>>3]=la,B[g]=B[E>>2],B[g+1]=B[E+4>>2],ma=sa,fb=la);if(ma==Ga){if(fb!=(B[E>>2]=B[q],B[E+4>>2]=B[q+1],D[E>>3])){x=1370}}else{x=1370}if(1370==x&&!(ma==Ha&&fb==(B[E>>2]=B[k],B[E+4>>2]=B[k+1],D[E>>3]))){if((B[E>>2]=B[p],B[E+4>>2]=B[p+1],D[E>>3])==ka){var ta=$+48|0;if((B[E>>2]=B[m],B[E+4>>2]=B[m+1],D[E>>3])==(B[E>>2]=B[ta>>2],B[E+4>>2]=B[ta+4>>2],D[E>>3])){var Pa=$,ca=ka,x=1377}else{x=1375}}else{x=1375}if(1375==x&&0>Ae(F,$,A)){var ua=B[n],Ya=ua+40|0,Pa=ua,ca=(B[E>>2]=B[Ya>>2],B[E+4>>2]=B[Ya+4>>2],D[E>>3]),x=1377}if(1377==x){if((B[E>>2]=B[r],B[E+4>>2]=B[r+1],D[E>>3])==ca){var pb=Pa+48|0;if((B[E>>2]=B[j],B[E+4>>2]=B[j+1],D[E>>3])!=(B[E>>2]=B[pb>>2],B[E+4>>2]=B[pb+4>>2],D[E>>3])){x=1379}}else{x=1379}if(1379!=x||0<Ae(U,Pa,A)){0==(ee(B[t])|0)&&X(c+2984|0);0==(ee(B[s])|0)&&X(c+2984|0);0==($d(B[B[s]+12>>2],Q)|0)&&X(c+2984|0);var Za=(B[E>>2]=B[f],B[E+4>>2]=B[f+1],D[E>>3]),ub=B[v]+40|0;D[E>>3]=Za;B[ub>>2]=B[E>>2];B[ub+4>>2]=B[E+4>>2];var vb=(B[E>>2]=B[g],B[E+4>>2]=B[g+1],D[E>>3]),Ia=B[v]+48|0;D[E>>3]=vb;B[Ia>>2]=B[E>>2];B[Ia+4>>2]=B[E+4>>2];d=(c+108|0)>>2;var gb=af(B[d],B[v]);B[B[v]+56>>2]=gb;var ea=B[v];2147483647==(B[ea+56>>2]|0)&&(Ye(B[d]),B[d]=0,X(c+2984|0));var ha,Qa,Ra=y;y=y+32|0;Qa=Ra>>2;var oa=Ra+16,ia=Ra|0;B[ia>>2]=B[K+12>>2];B[Qa+1]=B[F+12>>2];B[Qa+2]=B[L+12>>2];B[Qa+3]=B[U+12>>2];var Ja=oa|0;ha=(ea+16|0)>>2;B[ha]=0;B[ha+1]=0;B[ha+2]=0;B[ha+3]=0;B[ha+4]=0;B[ha+5]=0;yf(ea,K,F,Ja);yf(ea,L,U,oa+8|0);hf(c,ea,ia,Ja,1);y=Ra;B[C+5]=1;B[e+20>>2]=1;B[B[B[B[z]+4>>2]>>2]+20>>2]=1;M=0;y=A;return M}}var Ba=B[n];if((U|0)==(Ba|0)){0==(ee(B[t])|0)&&X(c+2984|0);0==($d(B[s],Q)|0)&&X(c+2984|0);var wb=kf(e);0==(wb|0)&&X(c+2984|0);var Ka=B[B[B[wb+4>>2]+8>>2]>>2],hb=B[Ka>>2];lf(c,Ka,w);nf(c,wb,B[B[hb+4>>2]+12>>2],hb,hb,1);M=1;y=A;return M}if((F|0)==(Ba|0)){0==(ee(B[s])|0)&&X(c+2984|0);0==($d(B[Q+12>>2],B[B[s]+12>>2])|0)&&X(c+2984|0);var Sa=xf(e),$a=B[B[B[B[B[B[Sa+4>>2]+8>>2]>>2]>>2]+4>>2]+8>>2];B[H>>2]=B[B[s]+12>>2];nf(c,Sa,B[lf(c,e,0)+8>>2],B[B[t]+8>>2],$a,1);M=1;y=A;return M}if(0<=Ae(F,Ba,A)){if(B[e+20>>2]=1,B[B[B[B[z]+4>>2]>>2]+20>>2]=1,0==(ee(B[t])|0)){X(c+2984|0)}else{var Ca=B[n]+40|0,Bb=(B[E>>2]=B[Ca>>2],B[E+4>>2]=B[Ca+4>>2],D[E>>3]),xb=B[v]+40|0;D[E>>3]=Bb;B[xb>>2]=B[E>>2];B[xb+4>>2]=B[E+4>>2];var ib=B[n]+48|0,La=(B[E>>2]=B[ib>>2],B[E+4>>2]=B[ib+4>>2],D[E>>3]),jb=B[v]+48|0;D[E>>3]=La;B[jb>>2]=B[E>>2];B[jb+4>>2]=B[E+4>>2]}}if(0<Ae(U,B[n],A)){return M=0,y=A,M}B[C+5]=1;B[e+20>>2]=1;0==(ee(B[s])|0)&&X(c+2984|0);var Da=B[n]+40|0,qb=(B[E>>2]=B[Da>>2],B[E+4>>2]=B[Da+4>>2],D[E>>3]),ab=B[u]+40|0;D[E>>3]=qb;B[ab>>2]=B[E>>2];B[ab+4>>2]=B[E+4>>2];var kb=B[n]+48|0,bb=(B[E>>2]=B[kb>>2],B[E+4>>2]=B[kb+4>>2],D[E>>3]),Ma=B[u]+48|0;D[E>>3]=bb;B[Ma>>2]=B[E>>2];B[Ma+4>>2]=B[E+4>>2];M=0;y=A;return M}vf(c,e);M=0;y=A;return M}tf.X=1;function yd(){}function Dd(c,e,d){var c=B[c+112>>2],e=B[e>>2]>>2,f=B[d>>2],d=f>>2,g=B[B[e+1]+16>>2],f=f+4|0,h=B[B[f>>2]+16>>2],i=(h|0)==(c|0);if((g|0)!=(c|0)){return e=B[e+4],c=i?0<=Ae(g,c,e)&1:ze(g,c,e)>=ze(B[B[f>>2]+16>>2],c,B[d+4])&1}if(!i){return c=0>=Ae(h,c,B[d+4])&1}e=B[e+4];g=e+40|0;g=(B[E>>2]=B[g>>2],B[E+4>>2]=B[g+4>>2],D[E>>3]);d=B[d+4];f=d+40|0;f=(B[E>>2]=B[f>>2],B[E+4>>2]=B[f+4>>2],D[E>>3]);do{if(g>=f){if(g==f&&(g=e+48|0,f=d+48|0,(B[E>>2]=B[g>>2],B[E+4>>2]=B[g+4>>2],D[E>>3])<=(B[E>>2]=B[f>>2],B[E+4>>2]=B[f+4>>2],D[E>>3]))){break}return c=0<=Ae(c,d,e)&1}}while(0);return c=0>=Ae(c,e,d)&1}Dd.X=1;function Ke(c){var e,d,f,g=0,h=B[c+12>>2];(h|0)==(c|0)?g=1443:(B[h+12>>2]|0)==(c|0)?g=1443:(e=c,f=e>>2);1443==g&&(S(5243156,82,5245184,5243768),e=c,f=e>>2);for(;;){var c=B[B[f+1]+16>>2],h=c+40|0,h=(B[E>>2]=B[h>>2],B[E+4>>2]=B[h+4>>2],D[E>>3]),i=B[f+4],j=i+40|0,j=(B[E>>2]=B[j>>2],B[E+4>>2]=B[j+4>>2],D[E>>3]);if(h>=j){if(h!=j){var k=e;d=k>>2;var m=i,q=j,n=c,p=h;break}var r=c+48|0,s=i+48|0;if((B[E>>2]=B[r>>2],B[E+4>>2]=B[r+4>>2],D[E>>3])>(B[E>>2]=B[s>>2],B[E+4>>2]=B[s+4>>2],D[E>>3])){k=e;d=k>>2;m=i;q=j;n=c;p=h;break}}e=B[B[f+2]+4>>2];f=e>>2}for(;;){if(q>=p){if(q!=p){break}m=m+48|0;q=n+48|0;if((B[E>>2]=B[m>>2],B[E+4>>2]=B[m+4>>2],D[E>>3])>(B[E>>2]=B[q>>2],B[E+4>>2]=B[q+4>>2],D[E>>3])){break}}k=B[d+3];m=B[k+16>>2];q=m+40|0;n=B[B[k+4>>2]+16>>2];f=n+40|0;d=k>>2;q=(B[E>>2]=B[q>>2],B[E+4>>2]=B[q+4>>2],D[E>>3]);p=(B[E>>2]=B[f>>2],B[E+4>>2]=B[f+4>>2],D[E>>3])}m=B[B[d+2]+4>>2];d=(B[d+3]|0)==(m|0);a:do{if(d){var t=k,u=m}else{q=m;f=k;b:for(;;){p=q+16|0;n=q+12|0;for(e=n>>2;;){c=B[B[f+4>>2]+16>>2];h=c+40|0;i=(B[E>>2]=B[h>>2],B[E+4>>2]=B[h+4>>2],D[E>>3]);h=B[p>>2];j=h+40|0;j=(B[E>>2]=B[j>>2],B[E+4>>2]=B[j+4>>2],D[E>>3]);if(i<j){break}if(i==j&&(c=c+48|0,h=h+48|0,(B[E>>2]=B[c>>2],B[E+4>>2]=B[c+4>>2],D[E>>3])<=(B[E>>2]=B[h>>2],B[E+4>>2]=B[h+4>>2],D[E>>3]))){break}c=(B[e]|0)==(f|0);c:do{if(c){var v=f}else{for(h=f;;){var i=h+8|0,j=B[B[i>>2]+4>>2],r=B[j+16>>2],s=r+40|0,s=(B[E>>2]=B[s>>2],B[E+4>>2]=B[s+4>>2],D[E>>3]),C=B[B[j+4>>2]+16>>2],z=C+40|0,z=(B[E>>2]=B[z>>2],B[E+4>>2]=B[z+4>>2],D[E>>3]);do{if(s<z){var x=j}else{if(s==z){var x=r+48|0,A=C+48|0;if((B[E>>2]=B[x>>2],B[E+4>>2]=B[x+4>>2],D[E>>3])<=(B[E>>2]=B[A>>2],B[E+4>>2]=B[A+4>>2],D[E>>3])){x=j;break}}if(0>Ae(B[B[h+4>>2]+16>>2],B[h+16>>2],r)){v=h;break c}x=B[B[i>>2]+4>>2]}}while(0);h=fe(h,x);if(0==(h|0)){var w=0,g=1484;break b}h=B[h+4>>2];if((B[e]|0)==(h|0)){v=h;break c}}}}while(0);f=B[v+12>>2];if((B[f+12>>2]|0)==(q|0)){t=f;u=q;break a}}e=B[e];p=(e|0)==(f|0);c:do{if(p){var H=q}else{c=q;h=n;for(i=e;;){j=B[B[i+4>>2]+16>>2];r=j+40|0;r=(B[E>>2]=B[r>>2],B[E+4>>2]=B[r+4>>2],D[E>>3]);s=B[i+16>>2];C=s+40|0;C=(B[E>>2]=B[C>>2],B[E+4>>2]=B[C+4>>2],D[E>>3]);do{if(r<C){z=i}else{if(r==C&&(z=j+48|0,x=s+48|0,(B[E>>2]=B[z>>2],B[E+4>>2]=B[z+4>>2],D[E>>3])<=(B[E>>2]=B[x>>2],B[E+4>>2]=B[x+4>>2],D[E>>3]))){z=i;break}if(0<Ae(B[c+16>>2],B[B[c+4>>2]+16>>2],j)){H=c;break c}z=B[h>>2]}}while(0);c=fe(z,c);if(0==(c|0)){w=0;g=1483;break b}c=B[c+4>>2];h=c+12|0;i=B[h>>2];if((i|0)==(f|0)){H=c;break c}}}}while(0);q=B[B[H+8>>2]+4>>2];if((B[f+12>>2]|0)==(q|0)){t=f;u=q;break a}}if(1483==g||1484==g){return w}}}while(0);v=u+12|0;H=B[v>>2];(H|0)==(t|0)?(S(5243156,118,5245184,5243528),v=B[v>>2]):v=H;if((B[v+12>>2]|0)==(t|0)){return 1}for(;;){u=fe(v,u);if(0==(u|0)){w=0;g=1481;break}u=B[u+4>>2];v=B[u+12>>2];if((B[v+12>>2]|0)==(t|0)){w=1;g=1482;break}}if(1481==g||1482==g){return w}}Ke.X=1;function zf(c,e,d){var f,g=O(32);f=g>>2;c=(c+16|0)>>2;B[f+7]=B[c];D[E>>3]=e;B[g>>2]=B[E>>2];B[g+4>>2]=B[E+4>>2];e=g+8|0;D[E>>3]=d;B[e>>2]=B[E>>2];B[e+4>>2]=B[E+4>>2];d=g+16|0;D[E>>3]=0;B[d>>2]=B[E>>2];B[d+4>>2]=B[E+4>>2];d=B[c];B[f+6]=0==(d|0)?0:B[d+24>>2]+1|0;return B[c]=g}function Af(c,e,d,f){var g,h=O(16);g=h>>2;var i=c|0;B[g+3]=B[i>>2];B[g]=e;B[g+1]=d;B[g+2]=f;c=c+4|0;B[c>>2]=B[c>>2]+1|0;B[i>>2]=h}function Ed(c,e){var d;d=e+12|0;var f=B[d>>2];if(0==(f|0)){B[d>>2]=c}else{d=(e+8|0)>>2;var g=B[d];0!=(g|0)&&Af(e,B[f+24>>2],B[g+24>>2],B[c+24>>2]);B[d]=c}}function of(c,e,d){var f,g,h=B[e>>2];g=h>>2;var i=B[g+4];f=i+40|0;var j=(B[E>>2]=B[f>>2],B[E+4>>2]=B[f+4>>2],D[E>>3]);f=d+40|0;f=(B[E>>2]=B[f>>2],B[E+4>>2]=B[f+4>>2],D[E>>3]);if(j==f&&(i=i+48|0,j=d+48|0,(B[E>>2]=B[i>>2],B[E+4>>2]=B[i+4>>2],D[E>>3])==(B[E>>2]=B[j>>2],B[E+4>>2]=B[j+4>>2],D[E>>3]))){S(5243336,957,5245432,5243708);ef(c,h,B[d+8>>2]);return}var i=B[g+1],j=B[i+16>>2],k=j+40|0;do{if((B[E>>2]=B[k>>2],B[E+4>>2]=B[k+4>>2],D[E>>3])==f){var m=j+48|0,q=d+48|0;if((B[E>>2]=B[m>>2],B[E+4>>2]=B[m+4>>2],D[E>>3])==(B[E>>2]=B[q>>2],B[E+4>>2]=B[q+4>>2],D[E>>3])){S(5243336,978,5245432,5243708);g=xf(e);i=e=B[B[B[g+4>>2]+8>>2]>>2];j=B[B[e>>2]+4>>2];h=B[j+8>>2];f=h>>2;if(0==(B[e+24>>2]|0)){var n=j}else{(h|0)==(j|0)&&S(5243336,987,5245432,5243672),gf(i),0==(ce(j)|0)?X(c+2984|0):n=B[B[f+1]+12>>2]}0==($d(B[d+8>>2],n)|0)&&X(c+2984|0);d=B[B[f+1]+16>>2];e=d+40|0;e=(B[E>>2]=B[e>>2],B[E+4>>2]=B[e+4>>2],D[E>>3]);f=B[f+4];i=f+40|0;i=(B[E>>2]=B[i>>2],B[E+4>>2]=B[i+4>>2],D[E>>3]);do{if(e<i){j=h}else{if(e==i&&(j=d+48|0,k=f+48|0,(B[E>>2]=B[j>>2],B[E+4>>2]=B[j+4>>2],D[E>>3])<=(B[E>>2]=B[k>>2],B[E+4>>2]=B[k+4>>2],D[E>>3]))){j=h;break}j=0}}while(0);nf(c,g,B[n+8>>2],h,j,1);return}}}while(0);0==(ee(i)|0)&&X(c+2984|0);n=e+24|0;0!=(B[n>>2]|0)&&(0==(ce(B[g+2])|0)?X(c+2984|0):B[n>>2]=0);0==($d(B[d+8>>2],h)|0)&&X(c+2984|0);ff(c,d)}of.X=1;function df(c,e){var d,f,g=O(28);f=g>>2;0==(g|0)&&X(c+2984|0);var h=Wd(B[c+8>>2]);0==(h|0)&&X(c+2984|0);d=h+16|0;var i=B[d>>2]+40|0;D[E>>3]=4e+150;B[i>>2]=B[E>>2];B[i+4>>2]=B[E+4>>2];d=B[d>>2]+48|0;D[E>>3]=e;B[d>>2]=B[E>>2];B[d+4>>2]=B[E+4>>2];d=(h+4|0)>>2;i=B[B[d]+16>>2]+40|0;D[E>>3]=-4e+150;B[i>>2]=B[E>>2];B[i+4>>2]=B[E+4>>2];i=B[B[d]+16>>2]+48|0;D[E>>3]=e;B[i>>2]=B[E>>2];B[i+4>>2]=B[E+4>>2];B[c+112>>2]=B[B[d]+16>>2];B[f]=h;B[f+2]=0;B[f+3]=0;B[f+6]=0;B[f+4]=1;B[f+5]=0;h=B[c+104>>2];g=Vd(h,h|0,g);B[f+1]=g;0==(g|0)&&X(c+2984|0)}df.X=1;function Jd(c,e){var d,f,g;g=(e+8|0)>>2;var h=B[g];if(0==(h|0)){B[g]=c}else{f=(e+12|0)>>2;var i=B[f];0!=(i|0)&&(d=(e+24|0)>>2,0==(B[d]|0)?Af(e,B[h+24>>2],B[i+24>>2],B[c+24>>2]):Af(e,B[i+24>>2],B[h+24>>2],B[c+24>>2]),B[d]=0==(B[d]|0)&1,B[g]=B[f]);B[f]=c}}function Cd(c,e){var d,f;f=(e+12|0)>>2;var g=B[f];if(0==(g|0)){B[f]=c}else{d=(e+8|0)>>2;var h=B[d];0==(h|0)?B[d]=c:(Af(e,B[g+24>>2],B[h+24>>2],B[c+24>>2]),B[f]=0,B[d]=0)}}function Nd(c,e){P[B[e+28>>2]](c,e)}function vd(c,e){var d=e>>2,f=y;B[d+3]=0;B[d+2]=0;B[d+6]=0;if(6==(c|0)){B[d+7]=20}else{if(5==(c|0)){B[d+7]=30}else{if(4==(c|0)){B[d+7]=16}else{for(var g=B[Mc>>2],h=(Xb=y,y=y+4|0,B[Xb>>2]=c,Xb),i=(function(c){var d;"double"===c?d=(B[E>>2]=B[h+k>>2],B[E+4>>2]=B[h+(k+4)>>2],D[E>>3]):"i64"==c?d=[B[h+k>>2],B[h+(k+4)>>2]]:(c="i32",d=B[h+k>>2]);k+=Math.max(Eb(c),Jb);return d}),j=5243028,k=0,m=[],q,n;;){var p=j;q=gc[j];if(0===q){break}n=gc[j+1];if(37==q){var r=l,s=l,t=l,u=l;a:for(;;){switch(n){case 43:r=a;break;case 45:s=a;break;case 35:t=a;break;case 48:if(u){break a}else{u=a;break};default:break a}j++;n=gc[j+1]}var v=0;if(42==n){v=i("i32"),j++,n=gc[j+1]}else{for(;48<=n&&57>=n;){v=10*v+(n-48),j++,n=gc[j+1]}}var C=l;if(46==n){var z=0,C=a;j++;n=gc[j+1];if(42==n){z=i("i32"),j++}else{for(;;){n=gc[j+1];if(48>n||57<n){break}z=10*z+(n-48);j++}}n=gc[j+1]}else{z=6}var x;switch(String.fromCharCode(n)){case"h":n=gc[j+2];104==n?(j++,x=1):x=2;break;case"l":n=gc[j+2];108==n?(j++,x=8):x=4;break;case"L":;case"q":;case"j":x=8;break;case"z":;case"t":;case"I":x=4;break;default:x=b}x&&j++;n=gc[j+1];if(-1!="diuoxXp".split("").indexOf(String.fromCharCode(n))){p=100==n||105==n;x=x||4;var A=q=i("i"+8*x),w;8==x&&(q=117==n?(q[0]>>>0)+4294967296*(q[1]>>>0):(q[0]>>>0)+4294967296*(q[1]|0));4>=x&&(q=(p?Dc:Cc)(q&Math.pow(256,x)-1,8*x));var H=Math.abs(q),p="";if(100==n||105==n){w=8==x&&Bf?Bf.stringify(A[0],A[1],b):Dc(q,8*x).toString(10)}else{if(117==n){w=8==x&&Bf?Bf.stringify(A[0],A[1],a):Cc(q,8*x).toString(10),q=Math.abs(q)}else{if(111==n){w=(t?"0":"")+H.toString(8)}else{if(120==n||88==n){p=t?"0x":"";if(8==x&&Bf){w=(A[1]>>>0).toString(16)+(A[0]>>>0).toString(16)}else{if(0>q){q=-q;w=(H-1).toString(16);A=[];for(t=0;t<w.length;t++){A.push((15-parseInt(w[t],16)).toString(16))}for(w=A.join("");w.length<2*x;){w="f"+w}}else{w=H.toString(16)}}88==n&&(p=p.toUpperCase(),w=w.toUpperCase())}else{112==n&&(0===H?w="(nil)":(p="0x",w=H.toString(16)))}}}}if(C){for(;w.length<z;){w="0"+w}}for(r&&(p=0>q?"-"+p:"+"+p);p.length+w.length<v;){s?w+=" ":u?w="0"+w:p=" "+p}w=p+w;w.split("").forEach((function(c){m.push(c.charCodeAt(0))}))}else{if(-1!="fFeEgG".split("").indexOf(String.fromCharCode(n))){q=i("double");if(isNaN(q)){w="nan",u=l}else{if(isFinite(q)){C=l;x=Math.min(z,20);if(103==n||71==n){C=a,z=z||1,x=parseInt(q.toExponential(x).split("e")[1],10),z>x&&-4<=x?(n=(103==n?"f":"F").charCodeAt(0),z-=x+1):(n=(103==n?"e":"E").charCodeAt(0),z--),x=Math.min(z,20)}if(101==n||69==n){w=q.toExponential(x),/[eE][-+]\d$/.test(w)&&(w=w.slice(0,-1)+"0"+w.slice(-1))}else{if(102==n||70==n){w=q.toFixed(x)}}p=w.split("e");if(C&&!t){for(;1<p[0].length&&-1!=p[0].indexOf(".")&&("0"==p[0].slice(-1)||"."==p[0].slice(-1));){p[0]=p[0].slice(0,-1)}}else{for(t&&-1==w.indexOf(".")&&(p[0]+=".");z>x++;){p[0]+="0"}}w=p[0]+(1<p.length?"e"+p[1]:"");69==n&&(w=w.toUpperCase());r&&0<=q&&(w="+"+w)}else{w=(0>q?"-":"")+"inf",u=l}}for(;w.length<v;){w=s?w+" ":u&&("-"==w[0]||"+"==w[0])?w[0]+"0"+w.slice(1):(u?"0":" ")+w}97>n&&(w=w.toUpperCase());w.split("").forEach((function(c){m.push(c.charCodeAt(0))}))}else{if(115==n){u=r=i("i8*")||xc;u|=0;n=0;for(n=u;gc[n]|0;){n=n+1|0}u=n-u|0;C&&(u=Math.min(u,z));if(!s){for(;u<v--;){m.push(32)}}for(t=0;t<u;t++){m.push(mc[r++])}if(s){for(;u<v--;){m.push(32)}}}else{if(99==n){for(s&&m.push(i("i8"));0<--v;){m.push(32)}s||m.push(i("i8"))}else{if(110==n){s=i("i32*"),B[s>>2]=m.length}else{if(37==n){m.push(q)}else{for(t=p;t<j+2;t++){m.push(gc[t])}}}}}}}j+=2}else{m.push(q),j+=1}}i=y;j=I(m,"i8",jc);w=1*m.length;0!=w&&-1==md(g,j,w)&&Zc[g]&&(Zc[g].error=a);y=i;B[d+7]=8}}}y=f}function Kd(c,e,d,f,g){e=c+8|0;B[f>>2]=zf(g,(B[E>>2]=B[c>>2],B[E+4>>2]=B[c+4>>2],D[E>>3]),(B[E>>2]=B[e>>2],B[E+4>>2]=B[e+4>>2],D[E>>3]))}function Cf(c,e,d,f,g){var h;h=(c+16|0)>>2;var i=B[B[h]+24>>2]+1|0;B[f>>2]=i;var j=c+4|0,f=B[j>>2];B[g>>2]=f;B[e>>2]=O(i<<4);g=B[j>>2];g=0==(g|0)?0:O(12*g&-1);B[d>>2]=g;g=B[h];i=0==(g|0);a:do{if(!i){for(j=g;;){var k=j|0,k=(B[E>>2]=B[k>>2],B[E+4>>2]=B[k+4>>2],D[E>>3]),j=(B[j+24>>2]<<4)+B[e>>2]|0;D[E>>3]=k;B[j>>2]=B[E>>2];B[j+4>>2]=B[E+4>>2];j=B[h];k=j+8|0;k=(B[E>>2]=B[k>>2],B[E+4>>2]=B[k+4>>2],D[E>>3]);j=((B[j+24>>2]<<1|1)<<3)+B[e>>2]|0;D[E>>3]=k;B[j>>2]=B[E>>2];B[j+4>>2]=B[E+4>>2];j=B[h];k=B[j+28>>2];Z(j);B[h]=k;if(0==(k|0)){break a}else{j=k}}}}while(0);c=(c|0)>>2;e=B[c];if(0!=(e|0)){for(;;){h=3*f&-1;B[B[d>>2]+(h-3<<2)>>2]=B[e>>2];B[B[d>>2]+(h-2<<2)>>2]=B[B[c]+4>>2];B[B[d>>2]+(h-1<<2)>>2]=B[B[c]+8>>2];e=B[c];h=B[e+12>>2];Z(e);B[c]=h;if(0==(h|0)){break}f=f-1|0;e=h}}}Cf.X=1;Module._tessellate=(function(c,e,d,f,g,h){var i=qe(),j,k=O(32);j=k>>2;B[j]=0;B[j+1]=0;B[j+2]=0;B[j+3]=0;B[j+4]=0;B[j+7]=8;B[j+6]=0;ve(i,100107,38);ve(i,100106,2);ve(i,100111,32);se(i,k);h=h-4|0;for(j=g;;){g=j+4|0;j=B[j>>2];var m=B[g>>2];te(i);var q=(j|0)==(m|0);a:do{if(!q){for(var n=j;;){var p=n+8|0,p=zf(k,(B[E>>2]=B[n>>2],B[E+4>>2]=B[n+4>>2],D[E>>3]),(B[E>>2]=B[p>>2],B[E+4>>2]=B[p+4>>2],D[E>>3])),n=n+16|0;we(i,p|0,p);if((n|0)==(m|0)){break a}}}}while(0);ue(i);if((g|0)==(h|0)){break}else{j=g}}De(i);Cf(k,c,d,e,f);Z(k);0!=(B[i>>2]|0)&&re(i,0);Z(i)});function O(c){do{if(245>c>>>0){var e=11>c>>>0?16:c+11&-8,d=e>>>3,f=B[1311136],g=f>>>(d>>>0);if(0!=(g&3|0)){var h=(g&1^1)+d|0,i=h<<1,e=(i<<2)+5244584|0,d=(i+2<<2)+5244584|0,c=B[d>>2],i=c+8|0,g=B[i>>2];(e|0)==(g|0)?B[1311136]=f&(1<<h^-1):g>>>0<B[1311140]>>>0?Y():(B[d>>2]=g,B[g+12>>2]=e);h<<=3;B[c+4>>2]=h|3;h=c+(h|4)|0;B[h>>2]|=1;return i}if(e>>>0>B[1311138]>>>0){if(0==(g|0)){if(0==(B[1311137]|0)){f=e;break}i=Df(e);if(0==(i|0)){f=e;break}return i}var c=2<<d,c=g<<d&(c|-c),d=(c&-c)-1|0,c=d>>>12&16,g=d>>>(c>>>0),d=g>>>5&8,j=g>>>(d>>>0),g=j>>>2&4,k=j>>>(g>>>0),j=k>>>1&2,k=k>>>(j>>>0),m=k>>>1&1,g=(d|c|g|j|m)+(k>>>(m>>>0))|0,c=g<<1,j=(c<<2)+5244584|0,k=(c+2<<2)+5244584|0,d=B[k>>2],c=d+8|0,m=B[c>>2];(j|0)==(m|0)?B[1311136]=f&(1<<g^-1):m>>>0<B[1311140]>>>0?Y():(B[k>>2]=m,B[m+12>>2]=j);g<<=3;f=g-e|0;B[d+4>>2]=e|3;j=d;d=j+e|0;B[j+(e|4)>>2]=f|1;B[j+g>>2]=f;g=B[1311138];0!=(g|0)&&(e=B[1311141],m=g>>>3,j=m<<1,g=(j<<2)+5244584|0,k=B[1311136],m=1<<m,0==(k&m|0)?(B[1311136]=k|m,i=g,h=(j+2<<2)+5244584|0):(j=(j+2<<2)+5244584|0,k=B[j>>2],k>>>0<B[1311140]>>>0?Y():(i=k,h=j)),B[h>>2]=e,B[i+12>>2]=e,B[e+8>>2]=i,B[e+12>>2]=g);B[1311138]=f;B[1311141]=d;return i=c}f=e}else{if(4294967231<c>>>0){f=-1}else{if(f=c+11&-8,0!=(B[1311137]|0)&&(e=Ef(f),0!=(e|0))){return i=e}}}}while(0);h=B[1311138];f>>>0>h>>>0?(i=B[1311139],f>>>0<i>>>0?(i=i-f|0,B[1311139]=i,h=B[1311142],B[1311142]=h+f|0,B[f+(h+4)>>2]=i|1,B[h+4>>2]=f|3,i=h+8|0):i=Ff(f)):(e=h-f|0,i=B[1311141],15<e>>>0?(B[1311141]=i+f|0,B[1311138]=e,B[f+(i+4)>>2]=e|1,B[i+h>>2]=e,B[i+4>>2]=f|3):(B[1311138]=0,B[1311141]=0,B[i+4>>2]=h|3,h=h+(i+4)|0,B[h>>2]|=1),i=i+8|0);return i}Module._malloc=O;O.X=1;function Df(c){var e,d,f=B[1311137],g=(f&-f)-1|0,f=g>>>12&16,h=g>>>(f>>>0),g=h>>>5&8;d=h>>>(g>>>0);var h=d>>>2&4,i=d>>>(h>>>0);d=i>>>1&2;var i=i>>>(d>>>0),j=i>>>1&1,f=h=g=B[((g|f|h|d|j)+(i>>>(j>>>0))<<2)+5244848>>2];d=f>>2;for(g=(B[g+4>>2]&-8)-c|0;;){i=B[h+16>>2];if(0==(i|0)){if(h=B[h+20>>2],0==(h|0)){break}else{d=h}}else{d=i}i=(B[d+4>>2]&-8)-c|0;j=i>>>0<g>>>0;h=d;f=j?d:f;d=f>>2;g=j?i:g}var i=f,k=B[1311140];i>>>0<k>>>0&&Y();h=i+c|0;i>>>0<h>>>0||Y();var j=B[d+6],m=B[d+3],q=(m|0)==(f|0);a:do{if(q){var n=f+20|0,p=B[n>>2];do{if(0==(p|0)){var r=f+16|0,s=B[r>>2];if(0==(s|0)){var t=0;e=t>>2;break a}}else{s=p,r=n}}while(0);for(;;){if(n=s+20|0,p=B[n>>2],0!=(p|0)){s=p,r=n}else{if(n=s+16|0,p=B[n>>2],0==(p|0)){break}else{s=p,r=n}}}r>>>0<k>>>0?Y():(B[r>>2]=0,t=s,e=t>>2)}else{s=B[d+2],s>>>0<k>>>0?Y():(B[s+12>>2]=m,B[m+8>>2]=s,t=m,e=t>>2)}}while(0);k=0==(j|0);a:do{if(!k){m=f+28|0;q=(B[m>>2]<<2)+5244848|0;do{if((f|0)==(B[q>>2]|0)){if(B[q>>2]=t,0==(t|0)){B[1311137]&=1<<B[m>>2]^-1;break a}}else{if(j>>>0<B[1311140]>>>0&&Y(),s=j+16|0,(B[s>>2]|0)==(f|0)?B[s>>2]=t:B[j+20>>2]=t,0==(t|0)){break a}}}while(0);t>>>0<B[1311140]>>>0&&Y();B[e+6]=j;m=B[d+4];0!=(m|0)&&(m>>>0<B[1311140]>>>0?Y():(B[e+4]=m,B[m+24>>2]=t));m=B[d+5];0!=(m|0)&&(m>>>0<B[1311140]>>>0?Y():(B[e+5]=m,B[m+24>>2]=t))}}while(0);if(16>g>>>0){var u=g+c|0;B[d+1]=u|3;u=u+(i+4)|0;B[u>>2]|=1;return u=f+8|0}B[d+1]=c|3;B[c+(i+4)>>2]=g|1;B[i+g+c>>2]=g;e=B[1311138];if(0!=(e|0)){c=B[1311141];i=e>>>3;t=i<<1;e=(t<<2)+5244584|0;d=B[1311136];i=1<<i;if(0==(d&i|0)){B[1311136]=d|i;var u=e,v=(t+2<<2)+5244584|0}else{t=(t+2<<2)+5244584|0,d=B[t>>2],d>>>0<B[1311140]>>>0?Y():(u=d,v=t)}B[v>>2]=c;B[u+12>>2]=c;B[c+8>>2]=u;B[c+12>>2]=e}B[1311138]=g;B[1311141]=h;return u=f+8|0}Df.X=1;function Ff(c){var e,d=0;0==(B[1310720]|0)&&Gf();var f=0==(B[1311246]&4|0);a:do{if(f){var g=B[1311142];if(0==(g|0)){d=1716}else{if(g=Hf(g),0==(g|0)){d=1716}else{var h=B[1310722],h=c+47-B[1311139]+h&-h;if(2147483647>h>>>0){var d=od(h),i=(d|0)==(B[g>>2]+B[g+4>>2]|0);e=i?d:-1;var i=i?h:0,j=d,k=h,d=1723}else{var m=0}}}if(1716==d){if(g=od(0),-1==(g|0)){m=0}else{var h=B[1310722],h=h+(c+47)&-h,q=g,n=B[1310721],p=n-1|0,h=0==(p&q|0)?h:h-q+(p+q&-n)|0;2147483647>h>>>0?(d=od(h),e=(i=(d|0)==(g|0))?g:-1,i=i?h:0,j=d,k=h,d=1723):m=0}}b:do{if(1723==d){d=-k|0;if(-1!=(e|0)){var r=i,s=e,d=1736;break a}do{if(-1!=(j|0)&2147483647>k>>>0){if(k>>>0<(c+48|0)>>>0){if(m=B[1310722],m=c+47-k+m&-m,2147483647>m>>>0){if(-1==(od(m)|0)){od(d);m=i;break b}else{m=m+k|0}}else{m=k}}else{m=k}}else{m=k}}while(0);if(-1!=(j|0)){r=m;s=j;d=1736;break a}B[1311246]|=4;var t=i,d=1733;break a}}while(0);B[1311246]|=4;t=m}else{t=0}d=1733}while(0);1733==d&&(f=B[1310722],f=f+(c+47)&-f,2147483647>f>>>0&&(f=od(f),e=od(0),-1!=(e|0)&-1!=(f|0)&f>>>0<e>>>0&&(e=e-f|0,f=(i=e>>>0>(c+40|0)>>>0)?f:-1,-1!=(f|0)&&(r=i?e:t,s=f,d=1736))));do{if(1736==d){t=B[1311244]+r|0;B[1311244]=t;t>>>0>B[1311245]>>>0&&(B[1311245]=t);t=B[1311142];f=0==(t|0);a:do{if(f){e=B[1311140];0==(e|0)|s>>>0<e>>>0&&(B[1311140]=s);B[1311247]=s;B[1311248]=r;B[1311250]=0;B[1311145]=B[1310720];B[1311144]=-1;for(e=0;!(i=e<<1,j=(i<<2)+5244584|0,B[(i+3<<2)+5244584>>2]=j,B[(i+2<<2)+5244584>>2]=j,e=e+1|0,32==(e|0));){}If(s,r-40|0)}else{i=5244988;for(e=i>>2;;){var u=B[e],v=i+4|0,C=B[v>>2];if((s|0)==(u+C|0)){d=1744;break}i=B[e+2];if(0==(i|0)){break}else{e=i>>2}}do{if(1744==d&&0==(B[e+3]&8|0)&&(i=t,i>>>0>=u>>>0&i>>>0<s>>>0)){B[v>>2]=C+r|0;If(B[1311142],B[1311139]+r|0);break a}}while(0);s>>>0<B[1311140]>>>0&&(B[1311140]=s);e=s+r|0;for(i=5244988;;){var z=i|0;if((B[z>>2]|0)==(e|0)){d=1752;break}j=B[i+8>>2];if(0==(j|0)){break}else{i=j}}if(1752==d&&0==(B[i+12>>2]&8|0)){return B[z>>2]=s,u=i+4|0,B[u>>2]=B[u>>2]+r|0,c=Jf(s,e,c)}Kf(s,r)}}while(0);t=B[1311139];if(t>>>0>c>>>0){return r=t-c|0,B[1311139]=r,u=s=B[1311142],B[1311142]=u+c|0,B[c+(u+4)>>2]=r|1,B[s+4>>2]=c|3,c=s+8|0}}}while(0);B[Uc>>2]=12;return 0}Ff.X=1;function Ef(c){var e,d,f,g,h,i=c>>2,j=0,k=-c|0,m=c>>>8;if(0==(m|0)){var q=0}else{if(16777215<c>>>0){q=31}else{var n=(m+1048320|0)>>>16&8,p=m<<n,r=(p+520192|0)>>>16&4,s=p<<r,t=(s+245760|0)>>>16&2,u=14-(r|n|t)+(s<<t>>>15)|0,q=c>>>((u+7|0)>>>0)&1|u<<1}}var v=B[(q<<2)+5244848>>2],C=0==(v|0);a:do{if(C){var z=0,x=k,A=0}else{var w=31==(q|0)?0:25-(q>>>1)|0,H=0,Q=k,J=v;h=J>>2;for(var K=c<<w,L=0;;){var F=B[h+1]&-8,U=F-c|0;if(U>>>0<Q>>>0){if((F|0)==(c|0)){z=J;x=U;A=J;break a}else{var aa=J,W=U}}else{aa=H,W=Q}var V=B[h+5],M=B[((K>>>31<<2)+16>>2)+h],T=0==(V|0)|(V|0)==(M|0)?L:V;if(0==(M|0)){z=aa;x=W;A=T;break a}else{H=aa,Q=W,J=M,h=J>>2,K<<=1,L=T}}}}while(0);if(0==(A|0)&0==(z|0)){var N=2<<q,R=B[1311137]&(N|-N);if(0==(R|0)){var ja=0;return ja}var wa=(R&-R)-1|0,mb=wa>>>12&16,rb=wa>>>(mb>>>0),Na=rb>>>5&8,eb=rb>>>(Na>>>0),Ea=eb>>>2&4,xa=eb>>>(Ea>>>0),Wa=xa>>>1&2,nb=xa>>>(Wa>>>0),qa=nb>>>1&1,ra=B[((Na|mb|Ea|Wa|qa)+(nb>>>(qa>>>0))<<2)+5244848>>2]}else{ra=A}var ob=0==(ra|0);a:do{if(ob){var $=x,ba=z;g=ba>>2}else{var ka=ra;f=ka>>2;for(var ya=x,za=z;;){var Oa=(B[f+1]&-8)-c|0,Xa=Oa>>>0<ya>>>0,Fa=Xa?Oa:ya,Aa=Xa?ka:za,Ga=B[f+4];if(0!=(Ga|0)){ka=Ga,f=ka>>2,ya=Fa,za=Aa}else{var Ha=B[f+5];if(0==(Ha|0)){$=Fa;ba=Aa;g=ba>>2;break a}else{ka=Ha,f=ka>>2,ya=Fa,za=Aa}}}}}while(0);if(0==(ba|0)||$>>>0>=(B[1311138]-c|0)>>>0){return ja=0}var fa=ba;d=fa>>2;var sa=B[1311140];fa>>>0<sa>>>0&&Y();var sb=fa+c|0;fa>>>0<sb>>>0||Y();var la=B[g+6],ma=B[g+3],fb=(ma|0)==(ba|0);a:do{if(fb){var tb=ba+20|0,ga=B[tb>>2];do{if(0==(ga|0)){var ta=ba+16|0,Pa=B[ta>>2];if(0==(Pa|0)){var ca=0;e=ca>>2;break a}else{var ua=Pa,Ya=ta}}else{ua=ga,Ya=tb}}while(0);for(;;){var pb=ua+20|0,Za=B[pb>>2];if(0!=(Za|0)){ua=Za,Ya=pb}else{var ub=ua+16|0,vb=B[ub>>2];if(0==(vb|0)){break}else{ua=vb,Ya=ub}}}Ya>>>0<sa>>>0?Y():(B[Ya>>2]=0,ca=ua,e=ca>>2)}else{var Ia=B[g+2];Ia>>>0<sa>>>0?Y():(B[Ia+12>>2]=ma,B[ma+8>>2]=Ia,ca=ma,e=ca>>2)}}while(0);var gb=0==(la|0);a:do{if(gb){var ea=ba}else{var ha=ba+28|0,Qa=(B[ha>>2]<<2)+5244848|0;do{if((ba|0)==(B[Qa>>2]|0)){if(B[Qa>>2]=ca,0==(ca|0)){B[1311137]&=1<<B[ha>>2]^-1;ea=ba;break a}}else{la>>>0<B[1311140]>>>0&&Y();var Ra=la+16|0;(B[Ra>>2]|0)==(ba|0)?B[Ra>>2]=ca:B[la+20>>2]=ca;if(0==(ca|0)){ea=ba;break a}}}while(0);ca>>>0<B[1311140]>>>0&&Y();B[e+6]=la;var oa=B[g+4];0!=(oa|0)&&(oa>>>0<B[1311140]>>>0?Y():(B[e+4]=oa,B[oa+24>>2]=ca));var ia=B[g+5];0==(ia|0)?ea=ba:ia>>>0<B[1311140]>>>0?Y():(B[e+5]=ia,B[ia+24>>2]=ca,ea=ba)}}while(0);do{if(16>$>>>0){var Ja=$+c|0;B[ea+4>>2]=Ja|3;var Ba=Ja+(fa+4)|0;B[Ba>>2]|=1}else{B[ea+4>>2]=c|3;B[i+(d+1)]=$|1;B[($>>2)+d+i]=$;var wb=$>>>3;if(256>$>>>0){var Ka=wb<<1,hb=(Ka<<2)+5244584|0,Sa=B[1311136],$a=1<<wb;if(0==(Sa&$a|0)){B[1311136]=Sa|$a;var Ca=hb,Bb=(Ka+2<<2)+5244584|0}else{var xb=(Ka+2<<2)+5244584|0,ib=B[xb>>2];ib>>>0<B[1311140]>>>0?Y():(Ca=ib,Bb=xb)}B[Bb>>2]=sb;B[Ca+12>>2]=sb;B[i+(d+2)]=Ca;B[i+(d+3)]=hb}else{var La=sb,jb=$>>>8;if(0==(jb|0)){var Da=0}else{if(16777215<$>>>0){Da=31}else{var qb=(jb+1048320|0)>>>16&8,ab=jb<<qb,kb=(ab+520192|0)>>>16&4,bb=ab<<kb,Ma=(bb+245760|0)>>>16&2,Fb=14-(kb|qb|Ma)+(bb<<Ma>>>15)|0,Da=$>>>((Fb+7|0)>>>0)&1|Fb<<1}}var yb=(Da<<2)+5244848|0;B[i+(d+7)]=Da;B[i+(d+5)]=0;B[i+(d+4)]=0;var Hb=B[1311137],Ib=1<<Da;if(0==(Hb&Ib|0)){B[1311137]=Hb|Ib,B[yb>>2]=La,B[i+(d+6)]=yb,B[i+(d+3)]=La,B[i+(d+2)]=La}else{for(var zb=$<<(31==(Da|0)?0:25-(Da>>>1)|0),cb=B[yb>>2];(B[cb+4>>2]&-8|0)!=($|0);){var Cb=(zb>>>31<<2)+cb+16|0,Gb=B[Cb>>2];if(0==(Gb|0)){j=1831;break}else{zb<<=1,cb=Gb}}if(1831==j){if(Cb>>>0<B[1311140]>>>0){Y()}else{B[Cb>>2]=La;B[i+(d+6)]=cb;B[i+(d+3)]=La;B[i+(d+2)]=La;break}}var Mb=cb+8|0,Db=B[Mb>>2],Nb=B[1311140];cb>>>0<Nb>>>0&&Y();Db>>>0<Nb>>>0?Y():(B[Db+12>>2]=La,B[Mb>>2]=La,B[i+(d+2)]=Db,B[i+(d+3)]=cb,B[i+(d+6)]=0)}}}}while(0);return ja=ea+8|0}Ef.X=1;function Lf(){var c;0==(B[1310720]|0)&&Gf();c=B[1311142];if(0!=(c|0)){var e=B[1311139];if(40<e>>>0){var d=B[1310722],f=(Math.floor(((e-41+d|0)>>>0)/(d>>>0))-1)*d&-1,g=Hf(c);if(0==(B[g+12>>2]&8|0)&&(e=od(0),c=(g+4|0)>>2,(e|0)==(B[g>>2]+B[c]|0)&&(f=od(-(2147483646<f>>>0?-2147483648-d|0:f)|0),d=od(0),-1!=(f|0)&d>>>0<e>>>0&&(f=e-d|0,(e|0)!=(d|0))))){B[c]=B[c]-f|0;B[1311244]=B[1311244]-f|0;If(B[1311142],B[1311139]-f|0);return}}B[1311139]>>>0>B[1311143]>>>0&&(B[1311143]=-1)}}Lf.X=1;function Z(c){var e,d,f,g,h,i,j,k=c>>2,m=0;if(0!=(c|0)){var q=c-8|0,n=B[1311140];q>>>0<n>>>0&&Y();var p=B[c-4>>2],r=p&3;1==(r|0)&&Y();var s=p&-8;j=s>>2;var t=c+(s-8)|0,u=0==(p&1|0);a:do{if(u){var v=B[q>>2];if(0==(r|0)){return}var C=-8-v|0;i=C>>2;var z=c+C|0,x=z,A=v+s|0;z>>>0<n>>>0&&Y();if((x|0)==(B[1311141]|0)){h=(c+(s-4)|0)>>2;if(3!=(B[h]&3|0)){var w=x;g=w>>2;var H=A;break}B[1311138]=A;B[h]&=-2;B[i+(k+1)]=A|1;B[t>>2]=A;return}var Q=v>>>3;if(256>v>>>0){var J=B[i+(k+2)],K=B[i+(k+3)];if((J|0)==(K|0)){B[1311136]&=1<<Q^-1;w=x;g=w>>2;H=A;break}var L=(Q<<3)+5244584|0;(J|0)!=(L|0)&J>>>0<n>>>0&&Y();if((K|0)==(L|0)|K>>>0>=n>>>0){B[J+12>>2]=K;B[K+8>>2]=J;w=x;g=w>>2;H=A;break}else{Y()}}var F=z,U=B[i+(k+6)],aa=B[i+(k+3)],W=(aa|0)==(F|0);b:do{if(W){var V=C+(c+20)|0,M=B[V>>2];do{if(0==(M|0)){var T=C+(c+16)|0,N=B[T>>2];if(0==(N|0)){var R=0;f=R>>2;break b}else{var ja=N,wa=T}}else{ja=M,wa=V}}while(0);for(;;){var mb=ja+20|0,rb=B[mb>>2];if(0!=(rb|0)){ja=rb,wa=mb}else{var Na=ja+16|0,eb=B[Na>>2];if(0==(eb|0)){break}else{ja=eb,wa=Na}}}wa>>>0<n>>>0?Y():(B[wa>>2]=0,R=ja,f=R>>2)}else{var Ea=B[i+(k+2)];Ea>>>0<n>>>0?Y():(B[Ea+12>>2]=aa,B[aa+8>>2]=Ea,R=aa,f=R>>2)}}while(0);if(0==(U|0)){w=x,g=w>>2,H=A}else{var xa=C+(c+28)|0,Wa=(B[xa>>2]<<2)+5244848|0;do{if((F|0)==(B[Wa>>2]|0)){if(B[Wa>>2]=R,0==(R|0)){B[1311137]&=1<<B[xa>>2]^-1;w=x;g=w>>2;H=A;break a}}else{U>>>0<B[1311140]>>>0&&Y();var nb=U+16|0;(B[nb>>2]|0)==(F|0)?B[nb>>2]=R:B[U+20>>2]=R;if(0==(R|0)){w=x;g=w>>2;H=A;break a}}}while(0);R>>>0<B[1311140]>>>0&&Y();B[f+6]=U;var qa=B[i+(k+4)];0!=(qa|0)&&(qa>>>0<B[1311140]>>>0?Y():(B[f+4]=qa,B[qa+24>>2]=R));var ra=B[i+(k+5)];0==(ra|0)?(w=x,g=w>>2,H=A):ra>>>0<B[1311140]>>>0?Y():(B[f+5]=ra,B[ra+24>>2]=R,w=x,g=w>>2,H=A)}}else{w=q,g=w>>2,H=s}}while(0);var ob=w;d=ob>>2;ob>>>0<t>>>0||Y();var $=c+(s-4)|0,ba=B[$>>2];0==(ba&1|0)&&Y();do{if(0==(ba&2|0)){if((t|0)==(B[1311142]|0)){var ka=B[1311139]+H|0;B[1311139]=ka;B[1311142]=w;B[g+1]=ka|1;(w|0)==(B[1311141]|0)&&(B[1311141]=0,B[1311138]=0);if(ka>>>0<=B[1311143]>>>0){return}Lf();return}if((t|0)==(B[1311141]|0)){var ya=B[1311138]+H|0;B[1311138]=ya;B[1311141]=w;B[g+1]=ya|1;B[(ya>>2)+d]=ya;return}var za=(ba&-8)+H|0,Oa=ba>>>3,Xa=256>ba>>>0;a:do{if(Xa){var Fa=B[k+j],Aa=B[((s|4)>>2)+k];if((Fa|0)==(Aa|0)){B[1311136]&=1<<Oa^-1}else{var Ga=(Oa<<3)+5244584|0;(Fa|0)!=(Ga|0)&&Fa>>>0<B[1311140]>>>0&&Y();(Aa|0)!=(Ga|0)&&Aa>>>0<B[1311140]>>>0&&Y();B[Fa+12>>2]=Aa;B[Aa+8>>2]=Fa}}else{var Ha=t,fa=B[j+(k+4)],sa=B[((s|4)>>2)+k],sb=(sa|0)==(Ha|0);b:do{if(sb){var la=s+(c+12)|0,ma=B[la>>2];do{if(0==(ma|0)){var fb=s+(c+8)|0,tb=B[fb>>2];if(0==(tb|0)){var ga=0;e=ga>>2;break b}else{var ta=tb,Pa=fb}}else{ta=ma,Pa=la}}while(0);for(;;){var ca=ta+20|0,ua=B[ca>>2];if(0!=(ua|0)){ta=ua,Pa=ca}else{var Ya=ta+16|0,pb=B[Ya>>2];if(0==(pb|0)){break}else{ta=pb,Pa=Ya}}}Pa>>>0<B[1311140]>>>0?Y():(B[Pa>>2]=0,ga=ta,e=ga>>2)}else{var Za=B[k+j];Za>>>0<B[1311140]>>>0?Y():(B[Za+12>>2]=sa,B[sa+8>>2]=Za,ga=sa,e=ga>>2)}}while(0);if(0!=(fa|0)){var ub=s+(c+20)|0,vb=(B[ub>>2]<<2)+5244848|0;do{if((Ha|0)==(B[vb>>2]|0)){if(B[vb>>2]=ga,0==(ga|0)){B[1311137]&=1<<B[ub>>2]^-1;break a}}else{fa>>>0<B[1311140]>>>0&&Y();var Ia=fa+16|0;(B[Ia>>2]|0)==(Ha|0)?B[Ia>>2]=ga:B[fa+20>>2]=ga;if(0==(ga|0)){break a}}}while(0);ga>>>0<B[1311140]>>>0&&Y();B[e+6]=fa;var gb=B[j+(k+2)];0!=(gb|0)&&(gb>>>0<B[1311140]>>>0?Y():(B[e+4]=gb,B[gb+24>>2]=ga));var ea=B[j+(k+3)];0!=(ea|0)&&(ea>>>0<B[1311140]>>>0?Y():(B[e+5]=ea,B[ea+24>>2]=ga))}}}while(0);B[g+1]=za|1;B[(za>>2)+d]=za;if((w|0)!=(B[1311141]|0)){var ha=za}else{B[1311138]=za;return}}else{B[$>>2]=ba&-2,B[g+1]=H|1,ha=B[(H>>2)+d]=H}}while(0);var Qa=ha>>>3;if(256>ha>>>0){var Ra=Qa<<1,oa=(Ra<<2)+5244584|0,ia=B[1311136],Ja=1<<Qa;if(0==(ia&Ja|0)){B[1311136]=ia|Ja;var Ba=oa,wb=(Ra+2<<2)+5244584|0}else{var Ka=(Ra+2<<2)+5244584|0,hb=B[Ka>>2];hb>>>0<B[1311140]>>>0?Y():(Ba=hb,wb=Ka)}B[wb>>2]=w;B[Ba+12>>2]=w;B[g+2]=Ba;B[g+3]=oa}else{var Sa=w,$a=ha>>>8;if(0==($a|0)){var Ca=0}else{if(16777215<ha>>>0){Ca=31}else{var Bb=($a+1048320|0)>>>16&8,xb=$a<<Bb,ib=(xb+520192|0)>>>16&4,La=xb<<ib,jb=(La+245760|0)>>>16&2,Da=14-(ib|Bb|jb)+(La<<jb>>>15)|0,Ca=ha>>>((Da+7|0)>>>0)&1|Da<<1}}var qb=(Ca<<2)+5244848|0;B[g+7]=Ca;B[g+5]=0;B[g+4]=0;var ab=B[1311137],kb=1<<Ca;do{if(0==(ab&kb|0)){B[1311137]=ab|kb,B[qb>>2]=Sa,B[g+6]=qb,B[g+3]=w,B[g+2]=w}else{for(var bb=ha<<(31==(Ca|0)?0:25-(Ca>>>1)|0),Ma=B[qb>>2];(B[Ma+4>>2]&-8|0)!=(ha|0);){var Fb=(bb>>>31<<2)+Ma+16|0,yb=B[Fb>>2];if(0==(yb|0)){m=1984;break}else{bb<<=1,Ma=yb}}if(1984==m){if(Fb>>>0<B[1311140]>>>0){Y()}else{B[Fb>>2]=Sa;B[g+6]=Ma;B[g+3]=w;B[g+2]=w;break}}var Hb=Ma+8|0,Ib=B[Hb>>2],zb=B[1311140];Ma>>>0<zb>>>0&&Y();Ib>>>0<zb>>>0?Y():(B[Ib+12>>2]=Sa,B[Hb>>2]=Sa,B[g+2]=Ib,B[g+3]=Ma,B[g+6]=0)}}while(0);var cb=B[1311144]-1|0;B[1311144]=cb;if(0==(cb|0)){for(var Cb=5244996;;){var Gb=B[Cb>>2];if(0==(Gb|0)){break}else{Cb=Gb+8|0}}B[1311144]=-1}}}}Module._free=Z;Z.X=1;function Hf(c){var e,d=0,f=5244988;for(e=f>>2;;){var g=B[e];if(g>>>0<=c>>>0&&(g+B[e+1]|0)>>>0>c>>>0){var h=f,d=2026;break}e=B[e+2];if(0==(e|0)){h=0;d=2027;break}else{f=e,e=f>>2}}if(2027==d||2026==d){return h}}function If(c,e){var d=c+8|0,d=0==(d&7|0)?0:-d&7,f=e-d|0;B[1311142]=c+d|0;B[1311139]=f;B[d+(c+4)>>2]=f|1;B[e+(c+4)>>2]=40;B[1311143]=B[1310724]}function Ue(c,e){var d,f,g=0;if(4294967231<e>>>0){return B[Uc>>2]=12,0}var h=c-8|0;f=(c-4|0)>>2;var i=B[f],j=i&-8,k=j-8|0,m=c+k|0;h>>>0<B[1311140]>>>0&&Y();var q=i&3;1!=(q|0)&-8<(k|0)||Y();d=(c+(j-4)|0)>>2;0==(B[d]&1|0)&&Y();k=11>e>>>0?16:e+11&-8;if(0==(q|0)){var n,i=B[h+4>>2]&-8;n=256>k>>>0?0:i>>>0>=(k+4|0)>>>0&&(i-k|0)>>>0<=B[1310722]<<1>>>0?h:0;var p=0,g=2054}else{j>>>0<k>>>0?(m|0)==(B[1311142]|0)&&(d=B[1311139]+j|0,d>>>0>k>>>0&&(g=d-k|0,B[f]=k|i&1|2,B[c+(k-4)>>2]=g|1,B[1311142]=c+(k-8)|0,B[1311139]=g,n=h,p=0,g=2054)):(g=j-k|0,15<g>>>0?(B[f]=k|i&1|2,B[c+(k-4)>>2]=g|3,B[d]|=1,n=h,p=c+k|0):(n=h,p=0),g=2054)}if(2054==g&&0!=(n|0)){return 0!=(p|0)&&Z(p),n+8|0}h=O(e);if(0==(h|0)){return 0}f=j-(0==(B[f]&3|0)?8:4)|0;Mf(h,c,f>>>0<e>>>0?f:e);Z(c);return h}Ue.X=1;function Gf(){if(0==(B[1310720]|0)){var c=nd();0!=(c-1&c|0)&&Y();B[1310722]=c;B[1310721]=c;B[1310723]=-1;B[1310724]=2097152;B[1310725]=0;B[1311246]=0;var c=B,e=Math.floor(Date.now()/1e3);c[1310720]=e&-16^1431655768}}function Jf(c,e,d){var f,g,h,i=e>>2,j=c>>2,k=0,m=c+8|0,m=0==(m&7|0)?0:-m&7;g=e+8|0;var q=0==(g&7|0)?0:-g&7;h=q>>2;var n=e+q|0,p=m+d|0;g=p>>2;var p=c+p|0,r=n-(c+m)-d|0;B[(m+4>>2)+j]=d|3;if((n|0)==(B[1311142]|0)){return k=B[1311139]+r|0,B[1311139]=k,B[1311142]=p,B[g+(j+1)]=k|1,c=c+(m|8)|0}if((n|0)==(B[1311141]|0)){return k=B[1311138]+r|0,B[1311138]=k,B[1311141]=p,B[g+(j+1)]=k|1,B[(k>>2)+j+g]=k,c=c+(m|8)|0}var s=B[h+(i+1)];if(1==(s&3|0)){var d=s&-8,t=s>>>3,s=256>s>>>0;a:do{if(s){var u=B[((q|8)>>2)+i],v=B[h+(i+3)];if((u|0)==(v|0)){B[1311136]&=1<<t^-1}else{var C=(t<<3)+5244584|0;(u|0)!=(C|0)&&u>>>0<B[1311140]>>>0&&Y();(v|0)!=(C|0)&&v>>>0<B[1311140]>>>0&&Y();B[u+12>>2]=v;B[v+8>>2]=u}}else{var u=n,v=B[((q|24)>>2)+i],C=B[h+(i+3)],z=(C|0)==(u|0);b:do{if(z){var x=q|16,A=x+(e+4)|0,w=B[A>>2];do{if(0==(w|0)){var H=e+x|0,Q=B[H>>2];if(0==(Q|0)){var J=0;f=J>>2;break b}}else{Q=w,H=A}}while(0);for(;;){if(x=Q+20|0,A=B[x>>2],0!=(A|0)){Q=A,H=x}else{if(x=Q+16|0,A=B[x>>2],0==(A|0)){break}else{Q=A,H=x}}}H>>>0<B[1311140]>>>0?Y():(B[H>>2]=0,J=Q,f=J>>2)}else{Q=B[((q|8)>>2)+i],Q>>>0<B[1311140]>>>0?Y():(B[Q+12>>2]=C,B[C+8>>2]=Q,J=C,f=J>>2)}}while(0);if(0!=(v|0)){C=q+(e+28)|0;z=(B[C>>2]<<2)+5244848|0;do{if((u|0)==(B[z>>2]|0)){if(B[z>>2]=J,0==(J|0)){B[1311137]&=1<<B[C>>2]^-1;break a}}else{if(v>>>0<B[1311140]>>>0&&Y(),Q=v+16|0,(B[Q>>2]|0)==(u|0)?B[Q>>2]=J:B[v+20>>2]=J,0==(J|0)){break a}}}while(0);J>>>0<B[1311140]>>>0&&Y();B[f+6]=v;u=q|16;v=B[(u>>2)+i];0!=(v|0)&&(v>>>0<B[1311140]>>>0?Y():(B[f+4]=v,B[v+24>>2]=J));u=B[(u+4>>2)+i];0!=(u|0)&&(u>>>0<B[1311140]>>>0?Y():(B[f+5]=u,B[u+24>>2]=J))}}}while(0);f=e+(d|q)|0;e=d+r|0}else{f=n,e=r}f=f+4|0;B[f>>2]&=-2;B[g+(j+1)]=e|1;B[(e>>2)+j+g]=e;f=e>>>3;if(256>e>>>0){var K=f<<1,k=(K<<2)+5244584|0,e=B[1311136];f=1<<f;if(0==(e&f|0)){B[1311136]=e|f;var L=k,F=(K+2<<2)+5244584|0}else{K=(K+2<<2)+5244584|0,e=B[K>>2],e>>>0<B[1311140]>>>0?Y():(L=e,F=K)}B[F>>2]=p;B[L+12>>2]=p;B[g+(j+2)]=L;B[g+(j+3)]=k;return c=c+(m|8)|0}F=e>>>8;0==(F|0)?F=0:16777215<e>>>0?F=31:(L=(F+1048320|0)>>>16&8,f=F<<L,F=(f+520192|0)>>>16&4,f<<=F,i=(f+245760|0)>>>16&2,L=14-(F|L|i)+(f<<i>>>15)|0,F=e>>>((L+7|0)>>>0)&1|L<<1);L=(F<<2)+5244848|0;B[g+(j+7)]=F;B[g+(j+5)]=0;B[g+(j+4)]=0;f=B[1311137];i=1<<F;if(0==(f&i|0)){return B[1311137]=f|i,B[L>>2]=p,B[g+(j+6)]=L,B[g+(j+3)]=p,B[g+(j+2)]=p,c=c+(m|8)|0}F=e<<(31==(F|0)?0:25-(F>>>1)|0);for(L=B[L>>2];(B[L+4>>2]&-8|0)!=(e|0);){if(K=(F>>>31<<2)+L+16|0,f=B[K>>2],0==(f|0)){k=2141;break}else{F<<=1,L=f}}if(2141==k){return K>>>0<B[1311140]>>>0&&Y(),B[K>>2]=p,B[g+(j+6)]=L,B[g+(j+3)]=p,B[g+(j+2)]=p,c=c+(m|8)|0}k=L+8|0;K=B[k>>2];F=B[1311140];L>>>0<F>>>0&&Y();K>>>0<F>>>0&&Y();B[K+12>>2]=p;B[k>>2]=p;B[g+(j+2)]=K;B[g+(j+3)]=L;B[g+(j+6)]=0;return c=c+(m|8)|0}Jf.X=1;function Mf(c,e,d){if(0==((e^c)&3|0)){var f=0==(d|0),g=0==(c&3|0)|f;a:do{if(g){var h=d,i=c,j=e,k=f}else{for(var m=d,q=c,n=e;;){var p=n+1|0,r=q+1|0;gc[q]=gc[n];m=m-1|0;q=0==(m|0);if(0==(r&3|0)|q){h=m;i=r;j=p;k=q;break a}else{q=r,n=p}}}}while(0);if(k){return c}d=i;e=3<h>>>0;a:do{if(e){i=h;k=d;for(p=j;;){if(f=p+4|0,g=k+4|0,B[k>>2]=B[p>>2],i=i-4|0,3<i>>>0){k=g,p=f}else{var s=i,t=g,u=f;break a}}}else{s=h,t=d,u=j}}while(0);h=u}else{h=e,t=c,s=d}if(0==(s|0)){return c}for(;!(gc[t]=gc[h],s=s-1|0,0==(s|0));){t=t+1|0,h=h+1|0}return c}Module._memcpy=Mf;Mf.X=1;function Kf(c,e){var d,f,g=0,h=B[1311142];f=h>>2;var i=Hf(h),j=B[i>>2];d=B[i+4>>2];var i=j+d|0,k=j+(d-39)|0,j=j+(d-47)+(0==(k&7|0)?0:-k&7)|0,j=j>>>0<(h+16|0)>>>0?h:j,k=j+8|0;d=k>>2;If(c,e-40|0);B[j+4>>2]=27;B[d]=B[1311247];B[d+1]=B[1311248];B[d+2]=B[1311249];B[d+3]=B[1311250];B[1311247]=c;B[1311248]=e;B[1311250]=0;B[1311249]=k;d=j+28|0;B[d>>2]=7;k=(j+32|0)>>>0<i>>>0;a:do{if(k){for(var m=d;;){var q=m+4|0;B[q>>2]=7;if((m+8|0)>>>0<i>>>0){m=q}else{break a}}}}while(0);if((j|0)!=(h|0)){if(i=j-h|0,j=i+(h+4)|0,B[j>>2]&=-2,B[f+1]=i|1,B[h+i>>2]=i,j=i>>>3,256>i>>>0){var n=j<<1,g=(n<<2)+5244584|0,i=B[1311136],j=1<<j;if(0==(i&j|0)){B[1311136]=i|j;var p=g,r=(n+2<<2)+5244584|0}else{n=(n+2<<2)+5244584|0,i=B[n>>2],i>>>0<B[1311140]>>>0?Y():(p=i,r=n)}B[r>>2]=h;B[p+12>>2]=h;B[f+2]=p;B[f+3]=g}else{if(r=i>>>8,0==(r|0)?r=0:16777215<i>>>0?r=31:(p=(r+1048320|0)>>>16&8,j=r<<p,r=(j+520192|0)>>>16&4,j<<=r,d=(j+245760|0)>>>16&2,p=14-(r|p|d)+(j<<d>>>15)|0,r=i>>>((p+7|0)>>>0)&1|p<<1),p=(r<<2)+5244848|0,B[f+7]=r,B[f+5]=0,B[f+4]=0,j=B[1311137],d=1<<r,0==(j&d|0)){B[1311137]=j|d,B[p>>2]=h,B[f+6]=p,B[f+3]=h,B[f+2]=h}else{r=i<<(31==(r|0)?0:25-(r>>>1)|0);for(p=B[p>>2];(B[p+4>>2]&-8|0)!=(i|0);){if(n=(r>>>31<<2)+p+16|0,j=B[n>>2],0==(j|0)){g=2193;break}else{r<<=1,p=j}}2193==g?(n>>>0<B[1311140]>>>0&&Y(),B[n>>2]=h,B[f+6]=p,B[f+3]=h,B[f+2]=h):(g=p+8|0,n=B[g>>2],r=B[1311140],p>>>0<r>>>0&&Y(),n>>>0<r>>>0&&Y(),B[n+12>>2]=h,B[g>>2]=h,B[f+2]=n,B[f+3]=p,B[f+6]=0)}}}}Kf.X=1;var Bf=b;Module.z=(function(c){function e(){for(var c=0;3>c;c++){f.push(0)}}var d=c.length+1,f=[I(wc("/bin/this.program"),"i8",kc)];e();for(var g=0;g<d-1;g+=1){f.push(I(wc(c[g]),"i8",kc)),e()}f.push(0);f=I(f,"i32",kc);return Module._main(d,f,0)});function Lc(c){function e(){var d=0;Gc=a;Module._main&&(yc(Ac),d=Module.z(c),Module.noExitRuntime||yc(Bc));if(Module.postRun){for("function"==typeof Module.postRun&&(Module.postRun=[Module.postRun]);0<Module.postRun.length;){Module.postRun.pop()()}}return d}c=c||Module.arguments;if(0<Ec){return Module.c("run() called, but dependencies remain, so not running"),0}if(Module.preRun){"function"==typeof Module.preRun&&(Module.preRun=[Module.preRun]);var d=Module.preRun;Module.preRun=[];for(var f=d.length-1;0<=f;f--){d[f]()}if(0<Ec){return 0}}return Module.setStatus?(Module.setStatus("Running..."),setTimeout((function(){setTimeout((function(){Module.setStatus("")}),1);e()}),1),0):e()}Module.run=Module.L=Lc;if(Module.preInit){for("function"==typeof Module.preInit&&(Module.preInit=[Module.preInit]);0<Module.preInit.length;){Module.preInit.pop()()}}yc(zc);var Kc=a;Module.noInitialRun&&(Kc=l);Kc&&Lc()




//   return CompiledModule;
// })();
var tessellate = (function() {

var c_tessellate = Module.cwrap('tessellate', 'void', ['number', 'number', 'number', 
                                                       'number', 'number', 'number']);
var tessellate = function(loops)
{
    var i;
    if (loops.length === 0)
        throw "Expected at least one loop";

    var vertices = [];
    var boundaries = [0];

    for (var l=0; l<loops.length; ++l) {
        var loop = loops[l];
        if (loop.length % 2 !== 0)
            throw "Expected even number of coordinates";
        vertices.push.apply(vertices, loop);
        boundaries.push(vertices.length);
    }

    var p = Module._malloc(vertices.length * 8);

    for (i=0; i<vertices.length; ++i)
        Module.setValue(p+i*8, vertices[i], "double");

    var contours = Module._malloc(boundaries.length * 4);
    for (i=0; i<boundaries.length; ++i)
        Module.setValue(contours + 4 * i, p + 8 * boundaries[i], 'i32');

    var ppcoordinates_out = Module._malloc(4);
    var pptris_out = Module._malloc(4);
    var pnverts = Module._malloc(4);
    var pntris = Module._malloc(4);

    c_tessellate(ppcoordinates_out, pnverts, pptris_out, pntris, 
                 contours, contours+4*boundaries.length);

    var pcoordinates_out = Module.getValue(ppcoordinates_out, 'i32');
    var ptris_out = Module.getValue(pptris_out, 'i32');

    var nverts = Module.getValue(pnverts, 'i32');
    var ntris = Module.getValue(pntris, 'i32');

    var result_vertices = new Float64Array(nverts * 2);
    var result_triangles = new Int32Array(ntris * 3);

    for (i=0; i<2*nverts; ++i) {
        result_vertices[i] = Module.getValue(pcoordinates_out + i*8, 'double');
    }
    for (i=0; i<3*ntris; ++i) {
        result_triangles[i] = Module.getValue(ptris_out + i*4, 'i32');
    }
    Module._free(pnverts);
    Module._free(pntris);
    Module._free(ppcoordinates_out);
    Module._free(pptris_out);
    Module._free(pcoordinates_out);
    Module._free(ptris_out);
    Module._free(p);
    Module._free(contours);
    return {
        vertices: result_vertices,
        triangles: result_triangles
    };
};

return tessellate;

})();
  return tessellate;
})();
// Some underscore.js extensions

// build objects from key-value pair lists
_.mixin({
    build: function(iterable) {
        var result = {};
        _.each(iterable, function(v) {
            result[v[0]] = v[1];
        });
        return result;
    }
});
// The linalg module is inspired by glMatrix and Sylvester.

// The goal is to be comparably fast as glMatrix but as close to
// Sylvester's generality as possible for
// low-dimensional linear algebra (vec2, 3, 4, mat2, 3, 4).

// In particular, I believe it is possible to have a "fast when
// needed" and "convenient when acceptable" Javascript vector library.

//////////////////////////////////////////////////////////////////////////////

var vec = {};
var mat = {};
vec.eps = 1e-6;
mat.eps = 1e-6;
var vec2 = {};

vec2.create = function()
{
    var result = new Float32Array(2);
    result.buffer._type = 'vec2';
    return result;
};

vec2.copy = function(vec)
{
    var result = new Float32Array(2);
    result.buffer._type = 'vec2';
    result[0] = vec[0];
    result[1] = vec[1];
    return result;
};

vec2.make = vec2.copy;

vec2.equal_eps = function(v1, v2)
{
    return Math.abs(v1[0] - v2[0]) < vec.eps &&
        Math.abs(v1[1] - v2[1]) < vec.eps;
};

vec2.equal = function(v1, v2)
{
    return v1[0] === v2[0] && v1[1] === v2[1];
};

vec2.random = function()
{
    var result = vec2.make([Math.random(), Math.random()]);
    return result;
};

vec2.set = function(dest, vec)
{
    dest[0] = vec[0];
    dest[1] = vec[1];
    return dest;
};

vec2.plus = function(v1, v2)
{
    var result = new Float32Array(2);
    result.buffer._type = 'vec2';
    result[0] = v1[0] + v2[0];
    result[1] = v1[1] + v2[1];
    return result;
};

vec2.add = function(dest, other)
{
    dest[0] += other[0];
    dest[1] += other[1];
    return dest;
};

vec2.minus = function(v1, v2)
{
    var result = new Float32Array(2);
    result.buffer._type = 'vec2';
    result[0] = v1[0] - v2[0];
    result[1] = v1[1] - v2[1];
    return result;
};

vec2.subtract = function(dest, other)
{
    dest[0] -= other[0];
    dest[1] -= other[1];
    return dest;
};

vec2.negative = function(v)
{
    var result = new Float32Array(2);
    result.buffer._type = 'vec2';
    result[0] = -v[0];
    result[1] = -v[1];
    return result;
};

vec2.negate = function(dest)
{
    dest[0] = -dest[0];
    dest[1] = -dest[1];
    return dest;
};

vec2.scaling = function(vec, val)
{
    var result = new Float32Array(2);
    result.buffer._type = 'vec2';
    result[0] = vec[0]*val;
    result[1] = vec[1]*val;
    return result;
};

vec2.scale = function(dest, val)
{
    dest[0] *= val;
    dest[1] *= val;
    return dest;
};

vec2.schur_product = function(v1, v2)
{
    var result = new Float32Array(2);
    result.buffer._type = 'vec2';
    result[0] = v1[0] * v2[0];
    result[1] = v1[1] * v2[1];
    return result;
};

vec2.schur_multiply = function(dest, other)
{
    dest[0] *= other[0];
    dest[1] *= other[1];
    return dest;
};

vec2.normalized = function(vec)
{
    var result = new Float32Array(2);
    result.buffer._type = 'vec2';
    var x = vec[0], y = vec[1];
    var len = Math.sqrt(x*x + y*y);
    if (!len)
        return result;
    if (len == 1) {
        result[0] = x;
        result[1] = y;
        return result;
    }
    result[0] = x / len;
    result[1] = y / len;
    return result;
};

vec2.normalize = function(dest)
{
    var x = dest[0], y = dest[1];
    var len = Math.sqrt(x*x + y*y);
    if (!len) {
        dest[0] = dest[1] = 0;
        return dest;
    }
    dest[0] /= len;
    dest[1] /= len;
    return dest;
};

vec2.length = function(vec)
{
    var x = vec[0], y = vec[1];
    return Math.sqrt(x*x + y*y);
};

vec2.length2 = function(vec)
{
    return vec[0] * vec[0] + vec[1] * vec[1];
};

vec2.dot = function(v1, v2)
{
    return v1[0] * v2[0] + v1[1] * v2[1];
};

vec2.map = function(vec, f) {
    return vec2.make(_.map(vec, f));
};

vec2.str = function(v) { return "[" + v[0] + ", " + v[1] + "]"; };

vec2.cross = function(v0, v1) {
    return v0[0] * v1[1] - v0[1] * v1[0];
};
var vec3 = {};

vec3.create = function()
{
    var result = new Float32Array(3);
    result.buffer._type = 'vec3';
    return result;
};

vec3.copy = function(vec)
{
    var result = new Float32Array(3);
    result.buffer._type = 'vec3';
    result[0] = vec[0];
    result[1] = vec[1];
    result[2] = vec[2];
    return result;
};

vec3.make = vec3.copy;

vec3.equal_eps = function(v1, v2)
{
    return Math.abs(v1[0] - v2[0]) < vec.eps &&
           Math.abs(v1[1] - v2[1]) < vec.eps &&
           Math.abs(v1[2] - v2[2]) < vec.eps;
};

vec3.equal = function(v1, v2)
{
    return v1[0] === v2[0] && v1[1] === v2[1] && v1[2] === v2[2];
};

vec3.random = function()
{
    var result = vec3.make([Math.random(), Math.random(), Math.random()]);
    return result;
};

vec3.set = function(dest, vec)
{
    dest[0] = vec[0];
    dest[1] = vec[1];
    dest[2] = vec[2];
    return dest;
};

vec3.plus = function(v1, v2)
{
    var result = new Float32Array(3);
    result.buffer._type = 'vec3';
    result[0] = v1[0] + v2[0];
    result[1] = v1[1] + v2[1];
    result[2] = v1[2] + v2[2];
    return result;
};

vec3.add = function(dest, other)
{
    dest[0] += other[0];
    dest[1] += other[1];
    dest[2] += other[2];
    return dest;
};

vec3.minus = function(v1, v2)
{
    var result = new Float32Array(3);
    result.buffer._type = 'vec3';
    result[0] = v1[0] - v2[0];
    result[1] = v1[1] - v2[1];
    result[2] = v1[2] - v2[2];
    return result;
};

vec3.subtract = function(dest, other)
{
    dest[0] -= other[0];
    dest[1] -= other[1];
    dest[2] -= other[2];
    return dest;
};

vec3.negative = function(v)
{
    var result = new Float32Array(3);
    result.buffer._type = 'vec3';
    result[0] = -v[0];
    result[1] = -v[1];
    result[2] = -v[2];
    return result;
};

vec3.negate = function(dest)
{
    dest[0] = -dest[0];
    dest[1] = -dest[1];
    dest[2] = -dest[2];
    return dest;
};

vec3.scaling = function(vec, val)
{
    var result = new Float32Array(3);
    result.buffer._type = 'vec3';
    result[0] = vec[0]*val;
    result[1] = vec[1]*val;
    result[2] = vec[2]*val;
    return result;
};

vec3.scale = function(dest, val)
{
    dest[0] *= val;
    dest[1] *= val;
    dest[2] *= val;
    return dest;
};

vec3.schur_product = function(v1, v2)
{
    var result = new Float32Array(3);
    result.buffer._type = 'vec3';
    result[0] = v1[0] * v2[0];
    result[1] = v1[1] * v2[1];
    result[2] = v1[2] * v2[2];
    return result;
};

vec3.schur_multiply = function(dest, other)
{
    dest[0] *= other[0];
    dest[1] *= other[1];
    dest[2] *= other[2];
    return dest;
};

vec3.normalized = function(vec)
{
    var result = new Float32Array(3);
    result.buffer._type = 'vec3';
    var x = vec[0], y = vec[1], z = vec[2];
    var len = Math.sqrt(x*x + y*y + z*z);
    if (!len)
        return result;
    if (len == 1) {
        result[0] = x;
        result[1] = y;
        result[2] = z;
        return result;
    }
    result[0] = x / len;
    result[1] = y / len;
    result[2] = z / len;
    return result;
};

vec3.normalize = function(dest)
{
    var x = dest[0], y = dest[1], z = dest[2];
    var len = Math.sqrt(x*x + y*y + z*z);
    if (!len) {
        dest[0] = dest[1] = dest[2] = 0;
        return dest;
    }
    dest[0] /= len;
    dest[1] /= len;
    dest[2] /= len;
    return dest;
};

vec3.cross = function(v1, v2)
{
    var x1 = v1[0], y1 = v1[1], z1 = v1[2];
    var x2 = v2[0], y2 = v2[1], z2 = v2[2];
    var result = new Float32Array(3);
    result.buffer._type = 'vec3';
    result[0] = y1 * z2 - z1 * y2;
    result[1] = z1 * x2 - x1 * z2;
    result[2] = x1 * y2 - y1 * x2;
    return result;
};

vec3.length = function(vec)
{
    var x = vec[0], y = vec[1], z = vec[2];
    return Math.sqrt(x*x + y*y + z*z);
};

vec3.length2 = function(vec)
{
    return vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2];
};

vec3.dot = function(v1, v2)
{
    return v1[0] * v2[0] + 
           v1[1] * v2[1] + 
           v1[2] * v2[2];
};

vec3.map = function(vec, f) {
    return vec3.make(_.map(vec, f));
};

vec3.str = function(v) { 
    return "[" + v[0] + ", " + v[1] + ", " + v[2] + "]";
};
var vec4 = {};

vec4.create = function()
{
    var result = new Float32Array(4);
    result.buffer._type = 'vec4';
    return result;
};

vec4.copy = function(vec)
{
    var result = new Float32Array(4);
    result.buffer._type = 'vec4';
    result[0] = vec[0];
    result[1] = vec[1];
    result[2] = vec[2];
    result[3] = vec[3];
    return result;
};

vec4.make = vec4.copy;

vec4.random = function() {
    var lst = [Math.random(), Math.random(), Math.random(), Math.random()];
    return vec4.make(lst);
};

vec4.equal_eps = function(v1, v2)
{
    return Math.abs(v1[0] - v2[0]) < vec.eps &&
        Math.abs(v1[1] - v2[1]) < vec.eps &&
        Math.abs(v1[2] - v2[2]) < vec.eps &&
        Math.abs(v1[3] - v2[3]) < vec.eps;
};

vec4.equal = function(v1, v2)
{
    return v1[0] === v2[0] && v1[1] === v2[1] && v1[2] === v2[2] && v1[3] === v2[3];
};

vec4.set = function(dest, vec)
{
    dest[0] = vec[0];
    dest[1] = vec[1];
    dest[2] = vec[2];
    dest[3] = vec[3];
    return dest;
};

vec4.plus = function(v1, v2)
{
    var result = new Float32Array(4);
    result.buffer._type = 'vec4';
    result[0] = v1[0] + v2[0];
    result[1] = v1[1] + v2[1];
    result[2] = v1[2] + v2[2];
    result[3] = v1[3] + v2[3];
    return result;
};

vec4.add = function(dest, other)
{
    dest[0] += other[0];
    dest[1] += other[1];
    dest[2] += other[2];
    dest[3] += other[3];
    return dest;
};

vec4.minus = function(v1, v2)
{
    var result = new Float32Array(4);
    result.buffer._type = 'vec4';
    result[0] = v1[0] - v2[0];
    result[1] = v1[1] - v2[1];
    result[2] = v1[2] - v2[2];
    result[3] = v1[3] - v2[3];
    return result;
};

vec4.subtract = function(dest, other)
{
    dest[0] -= other[0];
    dest[1] -= other[1];
    dest[2] -= other[2];
    dest[3] -= other[3];
    return dest;
};

vec4.negative = function(v)
{
    var result = new Float32Array(4);
    result.buffer._type = 'vec4';
    result[0] = -v[0];
    result[1] = -v[1];
    result[2] = -v[2];
    result[3] = -v[3];
    return result;
};

vec4.negate = function(dest)
{
    dest[0] = -dest[0];
    dest[1] = -dest[1];
    dest[2] = -dest[2];
    dest[3] = -dest[3];
    return dest;
};

vec4.scaling = function(vec, val)
{
    var result = new Float32Array(4);
    result.buffer._type = 'vec4';
    result[0] = vec[0]*val;
    result[1] = vec[1]*val;
    result[2] = vec[2]*val;
    result[3] = vec[3]*val;
    return result;
};

vec4.scale = function(dest, val)
{
    dest[0] *= val;
    dest[1] *= val;
    dest[2] *= val;
    dest[3] *= val;
    return dest;
};

vec4.schur_product = function(v1, v2)
{
    var result = new Float32Array(4);
    result.buffer._type = 'vec4';
    result[0] = v1[0] * v2[0];
    result[1] = v1[1] * v2[1];
    result[2] = v1[2] * v2[2];
    result[3] = v1[3] * v2[3];
    return result;
};

vec4.schur_multiply = function(dest, other)
{
    dest[0] *= other[0];
    dest[1] *= other[1];
    dest[2] *= other[2];
    dest[3] *= other[3];
    return dest;
};

vec4.normalized = function(vec)
{
    var result = new Float32Array(4);
    result.buffer._type = 'vec4';
    var x = vec[0], y = vec[1], z = vec[2], w = vec[3];
    var len = Math.sqrt(x*x + y*y + z*z + w*w);
    if (!len)
        return result;
    if (len == 1) {
        result[0] = x;
        result[1] = y;
        result[2] = z;
        result[3] = w;
        return result;
    }
    result[0] = x / len;
    result[1] = y / len;
    result[2] = z / len;
    result[3] = w / len;
    return result;
};

vec4.normalize = function(dest)
{
    var x = dest[0], y = dest[1], z = dest[2], w = dest[3];
    var len = Math.sqrt(x*x + y*y + z*z + w*w);
    if (!len) {
        dest[0] = dest[1] = dest[2] = dest[3] = 0;
        return dest;
    }
    dest[0] /= len;
    dest[1] /= len;
    dest[2] /= len;
    dest[3] /= len;
    return dest;
};

vec4.length = function(vec)
{
    var x = vec[0], y = vec[1], z = vec[2], w = vec[3];
    return Math.sqrt(x*x + y*y + z*z + w*w);
};

vec4.length2 = function(vec)
{
    return vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2] + vec[3] * vec[3];
};

vec4.dot = function(v1, v2)
{
    return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2] + v1[3] * v2[3];
};

vec4.map = function(vec, f) {
    return vec4.make(_.map(vec, f));
};

vec4.str = function(v) { 
    return "[" + v[0] + ", " + v[1] + ", " + v[2] + ", " + v[3] + "]";
};
var mat2 = {};

mat2.create = function()
{
    var result = new Float32Array(4);
    result.buffer._type = 'mat2';
    return result;
};

mat2.copy = function(mat)
{
    var result = new Float32Array(4);
    result.buffer._type = 'mat2';
    result[0] = mat[0];
    result[1] = mat[1];
    result[2] = mat[2];
    result[3] = mat[3];
    return result;
};
mat2.make = mat2.copy;

mat2.equal = function(v1, v2)
{
    return Math.abs(v1[0] - v2[0]) < mat.eps &&
        Math.abs(v1[1] - v2[1]) < mat.eps &&
        Math.abs(v1[2] - v2[2]) < mat.eps &&
        Math.abs(v1[3] - v2[3]) < mat.eps;
};

mat2.random = function()
{
    var result = new Float32Array(4);
    result.buffer._type = 'mat2';
    result[0] = Math.random();
    result[1] = Math.random();
    result[2] = Math.random();
    result[3] = Math.random();
    return result;
};

mat2.set = function(dest, mat)
{
    dest[0] = mat[0];
    dest[1] = mat[1];
    dest[2] = mat[2];
    dest[3] = mat[3];
    return dest;
};

(function() {
var _identity = new Float32Array([1,0,0,1]);

mat2.identity = function()
{
    var result = new Float32Array(_identity);
    result.buffer._type = 'mat2';
    return result;
};

mat2.set_identity = function(mat)
{
    mat2.set(mat, _identity);
    return mat;
};
})();

mat2.transpose = function(mat)
{
    var result = new Float32Array(4);
    result.buffer._type = 'mat2';
    result[0] = mat[0];
    result[1] = mat[2];
    result[2] = mat[1];
    result[3] = mat[3];
    return result;
};

mat2.set_transpose = function(dest, mat)
{
    if (mat == dest) {
        var a01 = mat[1];
        dest[1] = mat[2];
        dest[2] = a01;
        return dest;
    } else {
        dest[0] = mat[0];
        dest[1] = mat[2];
        dest[2] = mat[1];
        dest[3] = mat[3];
        return dest;
    }
};

mat2.determinant = function(mat)
{
    return mat[0]*mat[3] - mat[1]*mat[2];
};

// From glMatrix
mat2.inverse = function(mat)
{
    var result = new Float32Array(4);
    result.buffer._type = 'mat2';
	
    var a00 = mat[0], a01 = mat[1];
    var a10 = mat[2], a11 = mat[3];
    
    // Calculate the determinant (inlined to avoid double-caching)
    var det = (a00*a11 - a01*a10);
    if (det === 0)
        throw new Error("Singular matrix");

    result[0] =  a11/det;
    result[1] = -a01/det;
    result[2] = -a10/det;
    result[3] =  a00/det;

    return result;
};

mat2.invert = function(mat)
{
    var a00 = mat[0], a01 = mat[1];
    var a10 = mat[2], a11 = mat[3];
    
    // Calculate the determinant (inlined to avoid double-caching)
    var det = (a00*a11 - a01*a10);
    if (det === 0)
        throw new Error("Singular matrix");

    mat[0] =  a11/det;
    mat[1] = -a01/det;
    mat[2] = -a10/det;
    mat[3] =  a00/det;

    return mat;
};

mat2.as_mat4 = function(mat)
{
    var result = new Float32Array(16);
    result.buffer._type = 'mat4';
    result[0]  = mat[0];
    result[1]  = mat[1];
    result[4]  = mat[2];
    result[5]  = mat[3];
    return result;
};

mat2.as_mat3 = function(mat)
{
    var result = new Float32Array(9);
    result.buffer._type = 'mat3';
    result[0] = mat[0];
    result[1] = mat[1];
    result[3] = mat[2];
    result[4] = mat[3];
    return result;
};

// from glMatrix
mat2.product = function(m1, m2)
{
    var result = new Float32Array(4);
    result.buffer._type = 'mat2';

    // Cache the matrix values (makes for huge speed increases!)
    var a00 = m1[0],  a01 = m1[1];
    var a10 = m1[2],  a11 = m1[3];
    
    var b00 = m2[0],  b01 = m2[1];
    var b10 = m2[2],  b11 = m2[3];
    
    result[0] = b00*a00 + b01*a10;
    result[1] = b00*a01 + b01*a11;
    result[2] = b10*a00 + b11*a10;
    result[3] = b10*a01 + b11*a11;
    
    return result;
};

// from glMatrix
mat2.multiply = function(dest, other)
{
    // Cache the matrix values (makes for huge speed increases!)
    var a00 = dest[0],  a01 = dest[1]; 
    var a10 = dest[2],  a11 = dest[3]; 
    
    var b00 = other[0],  b01 = other[1]; 
    var b10 = other[2],  b11 = other[3]; 
    
    dest[0] = b00*a00 + b01*a10;
    dest[1] = b00*a01 + b01*a11;
    dest[2] = b10*a00 + b11*a10;
    dest[3] = b10*a01 + b11*a11;
    
    return dest;
};

mat2.product_vec = function(mat, vec)
{
    var result = new Float32Array(2);
    result.buffer._type = 'vec2';
    var x = vec[0], y = vec[1];
    result[0] = mat[0]*x + mat[2]*y;
    result[1] = mat[1]*x + mat[3]*y;
    return result;
};


mat2.multiply_vec = function(mat, vec)
{
    var x = vec[0], y = vec[1];
    vec[0] = mat[0]*x + mat[2]*y;
    vec[1] = mat[1]*x + mat[3]*y;
    return vec;
};

mat2.frobenius_norm = function(mat)
{
    return Math.sqrt(mat[0] * mat[0] +
                     mat[1] * mat[1] +
                     mat[2] * mat[2] +
                     mat[3] * mat[3]);
};

mat2.map = function(mat, f)
{
    return mat2.make(_.map(mat, f));
};

mat2.str = function(mat)
{
    return "[ [" + mat[0] + "] [" + mat[2] + "] ]\n" +
        "[ [" + mat[1] + "] [" + mat[3] + "] ]";
};

var mat3 = {};

mat3.create = function()
{
    var result = new Float32Array(9);
    result.buffer._type = 'mat3';
    return result;
};

mat3.copy = function(mat)
{
    var result = new Float32Array(9);
    result.buffer._type = 'mat3';
    result[0] = mat[0];
    result[1] = mat[1];
    result[2] = mat[2];
    result[3] = mat[3];
    result[4] = mat[4];
    result[5] = mat[5];
    result[6] = mat[6];
    result[7] = mat[7];
    result[8] = mat[8];
    return result;
};
mat3.make = mat3.copy;

mat3.equal = function(v1, v2)
{
    return Math.abs(v1[0] - v2[0]) < mat.eps &&
        Math.abs(v1[1] - v2[1]) < mat.eps &&
        Math.abs(v1[2] - v2[2]) < mat.eps &&
        Math.abs(v1[3] - v2[3]) < mat.eps &&
        Math.abs(v1[4] - v2[4]) < mat.eps &&
        Math.abs(v1[5] - v2[5]) < mat.eps &&
        Math.abs(v1[6] - v2[6]) < mat.eps &&
        Math.abs(v1[7] - v2[7]) < mat.eps &&
        Math.abs(v1[8] - v2[8]) < mat.eps;
};

mat3.random = function()
{
    var result = new Float32Array(9);
    result.buffer._type = 'mat3';
    result[0] = Math.random();
    result[1] = Math.random();
    result[2] = Math.random();
    result[3] = Math.random();
    result[4] = Math.random();
    result[5] = Math.random();
    result[6] = Math.random();
    result[7] = Math.random();
    result[8] = Math.random();
    return result;
};

mat3.set = function(dest, mat)
{
    dest[0] = mat[0];
    dest[1] = mat[1];
    dest[2] = mat[2];
    dest[3] = mat[3];
    dest[4] = mat[4];
    dest[5] = mat[5];
    dest[6] = mat[6];
    dest[7] = mat[7];
    dest[8] = mat[8];
    return dest;
};

(function() {
var _identity = new Float32Array([1,0,0,
                                  0,1,0,
                                  0,0,1]);

mat3.identity = function()
{
    var result = new Float32Array(_identity);
    result.buffer._type = 'mat3';
    return result;
};

mat3.set_identity = function(mat)
{
    mat3.set(mat, _identity);
    return mat;
};
})();

mat3.transpose = function(mat)
{
    var result = new Float32Array(9);
    result.buffer._type = 'mat3';
    result[0] = mat[0];
    result[1] = mat[3];
    result[2] = mat[6];
    result[3] = mat[1];
    result[4] = mat[4];
    result[5] = mat[7];
    result[6] = mat[2];
    result[7] = mat[5];
    result[8] =  mat[8];
    return result;
};

mat3.set_transpose = function(dest, mat)
{
    if (mat == dest) {
        var a01 = mat[1], a02 = mat[2];
        var a12 = mat[5];
        dest[1] = mat[3];
        dest[2] = mat[6];
        dest[3] = a01;
        dest[5] = mat[7];
        dest[6] = a02;
        dest[7] = a12;
        return dest;
    } else {
        dest[0] = mat[0];
        dest[1] = mat[3];
        dest[2] = mat[6];
        dest[3] = mat[1];
        dest[4] = mat[4];
        dest[5] = mat[7];
        dest[6] = mat[2];
        dest[7] = mat[5];
        dest[8] = mat[8];
        return dest;
    }
};

mat3.determinant = function(mat)
{
    var a00 = mat[0], a01 = mat[1], a02 = mat[2];
    var a10 = mat[3], a11 = mat[4], a12 = mat[5];
    var a20 = mat[6], a21 = mat[7], a22 = mat[8];
    
    return a00*a11*a22 + a01*a12*a20 + a02*a10*a21
        - a02*a11*a20 - a01*a10*a22 - a00*a12*a21;
};

// From glMatrix
mat3.inverse = function(mat)
{
    var result = new Float32Array(9);
    result.buffer._type = 'mat3';

    var a00 = mat[0], a01 = mat[3], a02 = mat[6];
    var a10 = mat[1], a11 = mat[4], a12 = mat[7];
    var a20 = mat[2], a21 = mat[5], a22 = mat[8];
    
    // Calculate the determinant (inlined to avoid double-caching)
    // var det = mat3.determinant(mat);
    var det = a00*a11*a22 + a01*a12*a20 + a02*a10*a21
        - a02*a11*a20 - a01*a10*a22 - a00*a12*a21;
    if (det === 0)
        throw new Error("Singular matrix");

    result[0] = ( a11*a22 - a12*a21)/det;
    result[1] = (-a10*a22 + a12*a20)/det;
    result[2] = ( a10*a21 - a11*a20)/det;
    result[3] = (-a01*a22 + a02*a21)/det;
    result[4] = ( a00*a22 - a02*a20)/det;
    result[5] = (-a00*a21 + a01*a20)/det;
    result[6] = ( a01*a12 - a02*a11)/det;
    result[7] = (-a00*a12 + a02*a10)/det;
    result[8] = ( a00*a11 - a01*a10)/det;

    return result;
};

// From glMatrix
mat3.invert = function(mat)
{
    var a00 = mat[0], a01 = mat[3], a02 = mat[6];
    var a10 = mat[1], a11 = mat[4], a12 = mat[7];
    var a20 = mat[2], a21 = mat[5], a22 = mat[8];
    
    // Calculate the determinant (inlined to avoid double-caching)
    var det = a00*a11*a22 + a01*a12*a20 + a02*a10*a21
        - a02*a11*a20 - a01*a10*a22 - a00*a12*a21;
    if (det === 0)
        throw new Error("Singular mat3");

    mat[0] = ( a11*a22 - a12*a21)/det;
    mat[1] = (-a10*a22 + a12*a20)/det;
    mat[2] = ( a10*a21 - a11*a20)/det;
    mat[3] = (-a01*a22 + a02*a21)/det;
    mat[4] = ( a00*a22 - a02*a20)/det;
    mat[5] = (-a00*a21 + a01*a20)/det;
    mat[6] = ( a01*a12 - a02*a11)/det;
    mat[7] = (-a00*a12 + a02*a10)/det;
    mat[8] = ( a00*a11 - a01*a10)/det;

    return mat;
};

mat3.as_mat4 = function(mat)
{
    var result = new Float32Array(9);
    result.buffer._type = 'mat4';
    result[0]  = mat[0];
    result[1]  = mat[1];
    result[2]  = mat[2];
    result[4]  = mat[3];
    result[5]  = mat[4];
    result[6]  = mat[5];
    result[8]  = mat[6];
    result[9]  = mat[7];
    result[10] = mat[8];
    return result;
};

mat3.as_mat2 = function(mat)
{
    var result = new Float32Array(4);
    result.buffer._type = 'mat2';
    result[0] = mat[0];
    result[1] = mat[1];
    result[2] = mat[3];
    result[3] = mat[4];
    return result;
};

// from glMatrix
mat3.product = function(m1, m2)
{
    var result = new Float32Array(9);
    result.buffer._type = 'mat3';

    // Cache the matrix values (makes for huge speed increases!)
    var a00 = m1[0],  a01 = m1[1],  a02 = m1[2];
    var a10 = m1[3],  a11 = m1[4],  a12 = m1[5];
    var a20 = m1[6],  a21 = m1[7],  a22 = m1[8];
    
    var b00 = m2[0],  b01 = m2[1],  b02 = m2[2];
    var b10 = m2[3],  b11 = m2[4],  b12 = m2[5];
    var b20 = m2[6],  b21 = m2[7],  b22 = m2[8];
    
    result[0] = b00*a00 + b01*a10 + b02*a20;
    result[1] = b00*a01 + b01*a11 + b02*a21;
    result[2] = b00*a02 + b01*a12 + b02*a22;
    result[3] = b10*a00 + b11*a10 + b12*a20;
    result[4] = b10*a01 + b11*a11 + b12*a21;
    result[5] = b10*a02 + b11*a12 + b12*a22;
    result[6] = b20*a00 + b21*a10 + b22*a20;
    result[7] = b20*a01 + b21*a11 + b22*a21;
    result[8] = b20*a02 + b21*a12 + b22*a22;
    
    return result;
};

// from glMatrix
mat3.multiply = function(dest, other)
{
    // Cache the matrix values (makes for huge speed increases!)
    var a00 = dest[0],  a01 = dest[1],  a02 = dest[2]; 
    var a10 = dest[3],  a11 = dest[4],  a12 = dest[5]; 
    var a20 = dest[6],  a21 = dest[7],  a22 = dest[8];
    
    var b00 = other[0],  b01 = other[1],  b02 = other[2]; 
    var b10 = other[3],  b11 = other[4],  b12 = other[5]; 
    var b20 = other[6],  b21 = other[7],  b22 = other[8];
    
    dest[0] = b00*a00 + b01*a10 + b02*a20;
    dest[1] = b00*a01 + b01*a11 + b02*a21;
    dest[2] = b00*a02 + b01*a12 + b02*a22;
    dest[3] = b10*a00 + b11*a10 + b12*a20;
    dest[4] = b10*a01 + b11*a11 + b12*a21;
    dest[5] = b10*a02 + b11*a12 + b12*a22;
    dest[6] = b20*a00 + b21*a10 + b22*a20;
    dest[7] = b20*a01 + b21*a11 + b22*a21;
    dest[8] = b20*a02 + b21*a12 + b22*a22;
    
    return dest;
};

mat3.product_vec = function(mat, vec)
{
    var result = new Float32Array(3);
    result.buffer._type = 'vec3';
    var x = vec[0], y = vec[1], z = vec[2];
    result[0] = mat[0]*x + mat[3]*y + mat[6]*z;
    result[1] = mat[1]*x + mat[4]*y + mat[7]*z;
    result[2] = mat[2]*x + mat[5]*y + mat[8]*z;
    return result;
};

mat3.multiply_vec = function(mat, vec)
{
    var x = vec[0], y = vec[1], z = vec[2];
    vec[0] = mat[0]*x + mat[3]*y + mat[6]*z;
    vec[1] = mat[1]*x + mat[4]*y + mat[7]*z;
    vec[2] = mat[2]*x + mat[5]*y + mat[8]*z;
    return vec;
};

mat3.frobenius_norm = function(mat)
{
    return Math.sqrt(mat[0] * mat[0] +
                     mat[1] * mat[1] +
                     mat[2] * mat[2] +
                     mat[3] * mat[3] +
                     mat[4] * mat[4] +
                     mat[5] * mat[5] +
                     mat[6] * mat[6] +
                     mat[7] * mat[7] +
                     mat[8] * mat[8]);

};

mat3.map = function(mat, f)
{
    return mat3.make(_.map(mat, f));
};

mat3.str = function(mat)
{
    return "[ [" + mat[0] + "] [" + mat[3] + "] [" + mat[6] + "] ]\n" +
        "[ [" + mat[1] + "] [" + mat[4] + "] [" + mat[7] + "] ]\n" +
        "[ [" + mat[2] + "] [" + mat[5] + "] [" + mat[8] + "] ]";
};

var mat4 = {};

mat4.create = function(mat)
{
    var result = new Float32Array(16);
    result.buffer._type = 'mat4';
    return result;
};

mat4.copy = function(mat)
{
    var result = new Float32Array(16);
    result.buffer._type = 'mat4';
    result[0] = mat[0];
    result[1] = mat[1];
    result[2] = mat[2];
    result[3] = mat[3];
    result[4] = mat[4];
    result[5] = mat[5];
    result[6] = mat[6];
    result[7] = mat[7];
    result[8] = mat[8];
    result[9] = mat[9];
    result[10] = mat[10];
    result[11] = mat[11];
    result[12] = mat[12];
    result[13] = mat[13];
    result[14] = mat[14];
    result[15] = mat[15];
    return result;
};
mat4.make = mat4.copy;

mat4.equal = function(v1, v2)
{
    return Math.abs(v1[0] - v2[0]) < mat.eps &&
        Math.abs(v1[1] - v2[1]) < mat.eps &&
        Math.abs(v1[2] - v2[2]) < mat.eps &&
        Math.abs(v1[3] - v2[3]) < mat.eps &&
        Math.abs(v1[4] - v2[4]) < mat.eps &&
        Math.abs(v1[5] - v2[5]) < mat.eps &&
        Math.abs(v1[6] - v2[6]) < mat.eps &&
        Math.abs(v1[7] - v2[7]) < mat.eps &&
        Math.abs(v1[8] - v2[8]) < mat.eps &&
        Math.abs(v1[9] - v2[9]) < mat.eps &&
        Math.abs(v1[10]- v2[10]) < mat.eps &&
        Math.abs(v1[11]- v2[11]) < mat.eps &&
        Math.abs(v1[12]- v2[12]) < mat.eps &&
        Math.abs(v1[13]- v2[13]) < mat.eps &&
        Math.abs(v1[14]- v2[14]) < mat.eps &&
        Math.abs(v1[15]- v2[15]) < mat.eps;
};

mat4.random = function()
{
    var result = mat4.create();
    for (var i=0; i<16; ++i) {
        result[i] = Math.random();
    }
    return result;
};

mat4.set = function(dest, mat)
{
    dest[0] = mat[0];
    dest[1] = mat[1];
    dest[2] = mat[2];
    dest[3] = mat[3];
    dest[4] = mat[4];
    dest[5] = mat[5];
    dest[6] = mat[6];
    dest[7] = mat[7];
    dest[8] = mat[8];
    dest[9] = mat[9];
    dest[10] = mat[10];
    dest[11] = mat[11];
    dest[12] = mat[12];
    dest[13] = mat[13];
    dest[14] = mat[14];
    dest[15] = mat[15];
    return dest;
};

(function() {
var _identity = new Float32Array([1,0,0,0,
                                  0,1,0,0,
                                  0,0,1,0,
                                  0,0,0,1]);

mat4.identity = function()
{
    var result = new Float32Array(_identity);
    result.buffer._type = 'mat4';
    return result;
};

mat4.set_identity = function(mat)
{
    mat4.set(mat, _identity);
    return mat;
};
})();

mat4.transpose = function(mat)
{
    var result = new Float32Array(16);
    result.buffer._type = 'mat4';
    result[0] = mat[0];
    result[1] = mat[4];
    result[2] = mat[8];
    result[3] = mat[12];
    result[4] = mat[1];
    result[5] = mat[5];
    result[6] = mat[9];
    result[7] = mat[13];
    result[8] =  mat[2];
    result[9] =  mat[6];
    result[10] = mat[10];
    result[11] = mat[14];
    result[12] = mat[3];
    result[13] = mat[7];
    result[14] = mat[11];
    result[15] = mat[15];
    return result;
};

mat4.set_transpose = function(dest, mat)
{
    if (mat == dest) {
        var a01 = mat[1], a02 = mat[2], a03 = mat[3];
        var a12 = mat[6], a13 = mat[7];
        var a23 = mat[11];
        dest[1] = mat[4];
        dest[2] = mat[8];
        dest[3] = mat[12];
        dest[4] = a01;
        dest[6] = mat[9];
        dest[7] = mat[13];
        dest[8] = a02;
        dest[9] = a03;
        dest[11] = mat[14];
        dest[12] = a03;
        dest[13] = a13;
        dest[14] = a23;
        return dest;
    } else {
        dest[0] = mat[0];
        dest[1] = mat[4];
        dest[2] = mat[8];
        dest[3] = mat[12];
        dest[4] = mat[1];
        dest[5] = mat[5];
        dest[6] = mat[9];
        dest[7] = mat[13];
        dest[8] = mat[2];
        dest[9] = mat[6];
        dest[10] = mat[10];
        dest[11] = mat[14];
        dest[12] = mat[3];
        dest[13] = mat[7];
        dest[14] = mat[11];
        dest[15] = mat[15];
        return dest;
    }
};

// From glMatrix
mat4.determinant = function(mat)
{
    var a00 = mat[0], a01 = mat[1], a02 = mat[2], a03 = mat[3];
    var a10 = mat[4], a11 = mat[5], a12 = mat[6], a13 = mat[7];
    var a20 = mat[8], a21 = mat[9], a22 = mat[10], a23 = mat[11];
    var a30 = mat[12], a31 = mat[13], a32 = mat[14], a33 = mat[15];
    
    return a30*a21*a12*a03 - a20*a31*a12*a03 - a30*a11*a22*a03 + a10*a31*a22*a03 +
	a20*a11*a32*a03 - a10*a21*a32*a03 - a30*a21*a02*a13 + a20*a31*a02*a13 +
	a30*a01*a22*a13 - a00*a31*a22*a13 - a20*a01*a32*a13 + a00*a21*a32*a13 +
	a30*a11*a02*a23 - a10*a31*a02*a23 - a30*a01*a12*a23 + a00*a31*a12*a23 +
	a10*a01*a32*a23 - a00*a11*a32*a23 - a20*a11*a02*a33 + a10*a21*a02*a33 +
	a20*a01*a12*a33 - a00*a21*a12*a33 - a10*a01*a22*a33 + a00*a11*a22*a33;
};

// From glMatrix
mat4.inverse = function(mat)
{
    var result = new Float32Array(16);
    result.buffer._type = 'mat4';
	
    var a00 = mat[0], a01 = mat[1], a02 = mat[2], a03 = mat[3];
    var a10 = mat[4], a11 = mat[5], a12 = mat[6], a13 = mat[7];
    var a20 = mat[8], a21 = mat[9], a22 = mat[10], a23 = mat[11];
    var a30 = mat[12], a31 = mat[13], a32 = mat[14], a33 = mat[15];
	
    var b00 = a00*a11 - a01*a10;
    var b01 = a00*a12 - a02*a10;
    var b02 = a00*a13 - a03*a10;
    var b03 = a01*a12 - a02*a11;
    var b04 = a01*a13 - a03*a11;
    var b05 = a02*a13 - a03*a12;
    var b06 = a20*a31 - a21*a30;
    var b07 = a20*a32 - a22*a30;
    var b08 = a20*a33 - a23*a30;
    var b09 = a21*a32 - a22*a31;
    var b10 = a21*a33 - a23*a31;
    var b11 = a22*a33 - a23*a32;
    
    // Calculate the determinant (inlined to avoid double-caching)
    var det = (b00*b11 - b01*b10 + b02*b09 + b03*b08 - b04*b07 + b05*b06);
    
    result[0] = (a11*b11 - a12*b10 + a13*b09)/det;
    result[1] = (-a01*b11 + a02*b10 - a03*b09)/det;
    result[2] = (a31*b05 - a32*b04 + a33*b03)/det;
    result[3] = (-a21*b05 + a22*b04 - a23*b03)/det;
    result[4] = (-a10*b11 + a12*b08 - a13*b07)/det;
    result[5] = (a00*b11 - a02*b08 + a03*b07)/det;
    result[6] = (-a30*b05 + a32*b02 - a33*b01)/det;
    result[7] = (a20*b05 - a22*b02 + a23*b01)/det;
    result[8] = (a10*b10 - a11*b08 + a13*b06)/det;
    result[9] = (-a00*b10 + a01*b08 - a03*b06)/det;
    result[10] = (a30*b04 - a31*b02 + a33*b00)/det;
    result[11] = (-a20*b04 + a21*b02 - a23*b00)/det;
    result[12] = (-a10*b09 + a11*b07 - a12*b06)/det;
    result[13] = (a00*b09 - a01*b07 + a02*b06)/det;
    result[14] = (-a30*b03 + a31*b01 - a32*b00)/det;
    result[15] = (a20*b03 - a21*b01 + a22*b00)/det;
    
    return result;
};

// From glMatrix
mat4.invert = function(mat)
{
    var a00 = mat[0], a01 = mat[1], a02 = mat[2], a03 = mat[3];
    var a10 = mat[4], a11 = mat[5], a12 = mat[6], a13 = mat[7];
    var a20 = mat[8], a21 = mat[9], a22 = mat[10], a23 = mat[11];
    var a30 = mat[12], a31 = mat[13], a32 = mat[14], a33 = mat[15];
	
    var b00 = a00*a11 - a01*a10;
    var b01 = a00*a12 - a02*a10;
    var b02 = a00*a13 - a03*a10;
    var b03 = a01*a12 - a02*a11;
    var b04 = a01*a13 - a03*a11;
    var b05 = a02*a13 - a03*a12;
    var b06 = a20*a31 - a21*a30;
    var b07 = a20*a32 - a22*a30;
    var b08 = a20*a33 - a23*a30;
    var b09 = a21*a32 - a22*a31;
    var b10 = a21*a33 - a23*a31;
    var b11 = a22*a33 - a23*a32;
    
    // Calculate the determinant (inlined to avoid double-caching)
    var det = (b00*b11 - b01*b10 + b02*b09 + b03*b08 - b04*b07 + b05*b06);
    
    mat[0] = (a11*b11 - a12*b10 + a13*b09)/det;
    mat[1] = (-a01*b11 + a02*b10 - a03*b09)/det;
    mat[2] = (a31*b05 - a32*b04 + a33*b03)/det;
    mat[3] = (-a21*b05 + a22*b04 - a23*b03)/det;
    mat[4] = (-a10*b11 + a12*b08 - a13*b07)/det;
    mat[5] = (a00*b11 - a02*b08 + a03*b07)/det;
    mat[6] = (-a30*b05 + a32*b02 - a33*b01)/det;
    mat[7] = (a20*b05 - a22*b02 + a23*b01)/det;
    mat[8] = (a10*b10 - a11*b08 + a13*b06)/det;
    mat[9] = (-a00*b10 + a01*b08 - a03*b06)/det;
    mat[10] = (a30*b04 - a31*b02 + a33*b00)/det;
    mat[11] = (-a20*b04 + a21*b02 - a23*b00)/det;
    mat[12] = (-a10*b09 + a11*b07 - a12*b06)/det;
    mat[13] = (a00*b09 - a01*b07 + a02*b06)/det;
    mat[14] = (-a30*b03 + a31*b01 - a32*b00)/det;
    mat[15] = (a20*b03 - a21*b01 + a22*b00)/det;
    
    return mat;
};

mat4.as_mat3 = function(mat)
{
    var result = new Float32Array(9);
    result.buffer._type = 'mat3';
    result[0] = mat[0];
    result[1] = mat[1];
    result[2] = mat[2];
    result[3] = mat[4];
    result[4] = mat[5];
    result[5] = mat[6];
    result[6] = mat[8];
    result[7] = mat[9];
    result[8] = mat[10];
    return result;
};

mat4.as_mat2 = function(mat)
{
    var result = new Float32Array(4);
    result.buffer._type = 'mat2';
    result[0] = mat[0];
    result[1] = mat[1];
    result[2] = mat[4];
    result[3] = mat[5];
    return result;
};


// from glMatrix
mat4.as_inverse_transpose_mat3 = function(mat)
{
    // Cache the matrix values (makes for huge speed increases!)
    var a00 = mat[0], a01 = mat[4], a02 = mat[8];
    var a10 = mat[1], a11 = mat[5], a12 = mat[9];
    var a20 = mat[2], a21 = mat[6], a22 = mat[10];
	
    var b01 =  a22*a11-a12*a21;
    var b11 = -a22*a10+a12*a20;
    var b21 =  a21*a10-a11*a20;
		
    var d = a00*b01 + a01*b11 + a02*b21;
    if (!d) throw new Error("singular matrix");

    var result = new Float32Array(9);
    result.buffer._type = 'mat3';
	
    result[0] = b01/d;
    result[1] = (-a22*a01 + a02*a21)/d;
    result[2] = ( a12*a01 - a02*a11)/d;
    result[3] = b11/d;
    result[4] = ( a22*a00 - a02*a20)/d;
    result[5] = (-a12*a00 + a02*a10)/d;
    result[6] = b21/d;
    result[7] = (-a21*a00 + a01*a20)/d;
    result[8] = ( a11*a00 - a01*a10)/d;
	
    return result;
};

// from glMatrix
mat4.product = function(m1, m2)
{
    var result = new Float32Array(16);
    result.buffer._type = 'mat4';

    // Cache the matrix values (makes for huge speed increases!)
    var a00 = m1[0],  a01 = m1[1],  a02 = m1[2],  a03 = m1[3];
    var a10 = m1[4],  a11 = m1[5],  a12 = m1[6],  a13 = m1[7];
    var a20 = m1[8],  a21 = m1[9],  a22 = m1[10], a23 = m1[11];
    var a30 = m1[12], a31 = m1[13], a32 = m1[14], a33 = m1[15];
    
    var b00 = m2[0],  b01 = m2[1],  b02 = m2[2],  b03 = m2[3];
    var b10 = m2[4],  b11 = m2[5],  b12 = m2[6],  b13 = m2[7];
    var b20 = m2[8],  b21 = m2[9],  b22 = m2[10], b23 = m2[11];
    var b30 = m2[12], b31 = m2[13], b32 = m2[14], b33 = m2[15];
    
    result[0]  = b00*a00 + b01*a10 + b02*a20 + b03*a30;
    result[1]  = b00*a01 + b01*a11 + b02*a21 + b03*a31;
    result[2]  = b00*a02 + b01*a12 + b02*a22 + b03*a32;
    result[3]  = b00*a03 + b01*a13 + b02*a23 + b03*a33;
    result[4]  = b10*a00 + b11*a10 + b12*a20 + b13*a30;
    result[5]  = b10*a01 + b11*a11 + b12*a21 + b13*a31;
    result[6]  = b10*a02 + b11*a12 + b12*a22 + b13*a32;
    result[7]  = b10*a03 + b11*a13 + b12*a23 + b13*a33;
    result[8]  = b20*a00 + b21*a10 + b22*a20 + b23*a30;
    result[9]  = b20*a01 + b21*a11 + b22*a21 + b23*a31;
    result[10] = b20*a02 + b21*a12 + b22*a22 + b23*a32;
    result[11] = b20*a03 + b21*a13 + b22*a23 + b23*a33;
    result[12] = b30*a00 + b31*a10 + b32*a20 + b33*a30;
    result[13] = b30*a01 + b31*a11 + b32*a21 + b33*a31;
    result[14] = b30*a02 + b31*a12 + b32*a22 + b33*a32;
    result[15] = b30*a03 + b31*a13 + b32*a23 + b33*a33;
    
    return result;
};

// from glMatrix
mat4.multiply = function(dest, other)
{
    // Cache the matrix values (makes for huge speed increases!)
    var a00 = dest[0],  a01 = dest[1],  a02 = dest[2],  a03 = dest[3];
    var a10 = dest[4],  a11 = dest[5],  a12 = dest[6],  a13 = dest[7];
    var a20 = dest[8],  a21 = dest[9],  a22 = dest[10], a23 = dest[11];
    var a30 = dest[12], a31 = dest[13], a32 = dest[14], a33 = dest[15];
    
    var b00 = other[0],  b01 = other[1],  b02 = other[2],  b03 = other[3];
    var b10 = other[4],  b11 = other[5],  b12 = other[6],  b13 = other[7];
    var b20 = other[8],  b21 = other[9],  b22 = other[10], b23 = other[11];
    var b30 = other[12], b31 = other[13], b32 = other[14], b33 = other[15];
    
    dest[0]  = b00*a00 + b01*a10 + b02*a20 + b03*a30;
    dest[1]  = b00*a01 + b01*a11 + b02*a21 + b03*a31;
    dest[2]  = b00*a02 + b01*a12 + b02*a22 + b03*a32;
    dest[3]  = b00*a03 + b01*a13 + b02*a23 + b03*a33;
    dest[4]  = b10*a00 + b11*a10 + b12*a20 + b13*a30;
    dest[5]  = b10*a01 + b11*a11 + b12*a21 + b13*a31;
    dest[6]  = b10*a02 + b11*a12 + b12*a22 + b13*a32;
    dest[7]  = b10*a03 + b11*a13 + b12*a23 + b13*a33;
    dest[8]  = b20*a00 + b21*a10 + b22*a20 + b23*a30;
    dest[9]  = b20*a01 + b21*a11 + b22*a21 + b23*a31;
    dest[10] = b20*a02 + b21*a12 + b22*a22 + b23*a32;
    dest[11] = b20*a03 + b21*a13 + b22*a23 + b23*a33;
    dest[12] = b30*a00 + b31*a10 + b32*a20 + b33*a30;
    dest[13] = b30*a01 + b31*a11 + b32*a21 + b33*a31;
    dest[14] = b30*a02 + b31*a12 + b32*a22 + b33*a32;
    dest[15] = b30*a03 + b31*a13 + b32*a23 + b33*a33;
    
    return dest;
};

mat4.product_vec = function(mat, vec)
{
    var result = new Float32Array(4);
    result.buffer._type = 'vec4';
    var x = vec[0], y = vec[1], z = vec[2], w = vec[3];
    result[0] = mat[0]*x + mat[4]*y + mat[8]*z  + mat[12]*w;
    result[1] = mat[1]*x + mat[5]*y + mat[9]*z  + mat[13]*w;
    result[2] = mat[2]*x + mat[6]*y + mat[10]*z + mat[14]*w;
    result[3] = mat[3]*x + mat[7]*y + mat[11]*z + mat[15]*w;
    return result;
};

mat4.multiply_vec = function(mat, vec)
{
    var x = vec[0], y = vec[1], z = vec[2], w = vec[3];
    vec[0] = mat[0]*x + mat[4]*y + mat[8]*z  + mat[12]*w;
    vec[1] = mat[1]*x + mat[5]*y + mat[9]*z  + mat[13]*w;
    vec[2] = mat[2]*x + mat[6]*y + mat[10]*z + mat[14]*w;
    vec[3] = mat[3]*x + mat[7]*y + mat[11]*z + mat[15]*w;
    return vec;
};

mat4.multiply_vec3 = function(mat, vec)
{
    var x = vec[0], y = vec[1], z = vec[2];
    vec[0] = mat[0]*x + mat[4]*y + mat[8]*z;
    vec[1] = mat[1]*x + mat[5]*y + mat[9]*z;
    vec[2] = mat[2]*x + mat[6]*y + mat[10]*z;
    return vec;
};

// from glMatrix
mat4.translation_of = function(mat, vec)
{
    var result = new Float32Array(16);
    result.buffer._type = 'mat4';
    var x = vec[0], y = vec[1], z = vec[2];
    var a00 = mat[0], a01 = mat[1], a02 = mat[2], a03 = mat[3];
    var a10 = mat[4], a11 = mat[5], a12 = mat[6], a13 = mat[7];
    var a20 = mat[8], a21 = mat[9], a22 = mat[10], a23 = mat[11];
    result[0] = a00;
    result[1] = a01;
    result[2] = a02;
    result[3] = a03;
    result[4] = a10;
    result[5] = a11;
    result[6] = a12;
    result[7] = a13;
    result[8] = a20;
    result[9] = a21;
    result[10] = a22;
    result[11] = a23;
    result[12] = a00*x + a10*y + a20*z + mat[12];
    result[13] = a01*x + a11*y + a21*z + mat[13];
    result[14] = a02*x + a12*y + a22*z + mat[14];
    result[15] = a03*x + a13*y + a23*z + mat[15];
    return result;
};

mat4.translation = function(vec)
{
    var result = new Float32Array(16);
    result.buffer._type = 'mat4';
    result[0] = result[5] = result[10] = result[15] = 1;    
    result[12] = vec[0];
    result[13] = vec[1];
    result[14] = vec[2];
    return result;
};

mat4.translate = function(mat, vec)
{
    var x = vec[0], y = vec[1], z = vec[2];
    mat[12] = mat[0]*x + mat[4]*y + mat[8]*z + mat[12];
    mat[13] = mat[1]*x + mat[5]*y + mat[9]*z + mat[13];
    mat[14] = mat[2]*x + mat[6]*y + mat[10]*z + mat[14];
    mat[15] = mat[3]*x + mat[7]*y + mat[11]*z + mat[15];
    return mat;
};

mat4.scaling_of = function(mat, vec)
{
    var result = new Float32Array(16);
    result.buffer._type = 'mat4';
    var x = vec[0], y = vec[1], z = vec[2];
    result[0] =  mat[0]  * x;
    result[1] =  mat[1]  * x;
    result[2] =  mat[2]  * x;
    result[3] =  mat[3]  * x;
    result[4] =  mat[4]  * y;
    result[5] =  mat[5]  * y;
    result[6] =  mat[6]  * y;
    result[7] =  mat[7]  * y;
    result[8] =  mat[8]  * z;
    result[9] =  mat[9]  * z;
    result[10] = mat[10] * z;
    result[11] = mat[11] * z;
    result[12] = mat[12];
    result[13] = mat[13];
    result[14] = mat[14];
    result[15] = mat[15];
    return result;
};

mat4.scaling = function(mat, vec)
{
    var result = new Float32Array(16);
    result[0] =  vec[0];
    result[5] =  vec[1];
    result[10] = vec[2];
    result[15] = 1;
    return result;
};

mat4.scale = function(mat, vec)
{
    var result = new Float32Array(16);
    result.buffer._type = 'mat4';
    var x = vec[0], y = vec[1], z = vec[2];
    mat[0]  *= x;
    mat[1]  *= x;
    mat[2]  *= x;
    mat[3]  *= x;
    mat[4]  *= y;
    mat[5]  *= y;
    mat[6]  *= y;
    mat[7]  *= y;
    mat[8]  *= z;
    mat[9]  *= z;
    mat[10] *= z;
    mat[11] *= z;
    return result;
};

// from glMatrix
mat4.rotation_of = function(mat, angle, axis)
{
    var x = axis[0], y = axis[1], z = axis[2];
    var len = Math.sqrt(x*x + y*y + z*z);
    if (!len) { throw new Error("zero-length axis"); }
    if (len != 1) {
	x /= len; 
	y /= len; 
	z /= len;
    }
    
    var s = Math.sin(angle);
    var c = Math.cos(angle);
    var t = 1-c;
    
    // Cache the matrix values (makes for huge speed increases!)
    var a00 = mat[0], a01 = mat[1], a02 = mat[2],  a03 = mat[3];
    var a10 = mat[4], a11 = mat[5], a12 = mat[6],  a13 = mat[7];
    var a20 = mat[8], a21 = mat[9], a22 = mat[10], a23 = mat[11];
    
    // Construct the elements of the rotation matrix
    var b00 = x*x*t + c, b01 = y*x*t + z*s, b02 = z*x*t - y*s;
    var b10 = x*y*t - z*s, b11 = y*y*t + c, b12 = z*y*t + x*s;
    var b20 = x*z*t + y*s, b21 = y*z*t - x*s, b22 = z*z*t + c;

    var result = new Float32Array(16);    
    result.buffer._type = 'mat4';
    
    // Perform rotation-specific matrix multiplication
    result[0]  = a00*b00 + a10*b01 + a20*b02;
    result[1]  = a01*b00 + a11*b01 + a21*b02;
    result[2]  = a02*b00 + a12*b01 + a22*b02;
    result[3]  = a03*b00 + a13*b01 + a23*b02;
    
    result[4]  = a00*b10 + a10*b11 + a20*b12;
    result[5]  = a01*b10 + a11*b11 + a21*b12;
    result[6]  = a02*b10 + a12*b11 + a22*b12;
    result[7]  = a03*b10 + a13*b11 + a23*b12;
    
    result[8]  = a00*b20 + a10*b21 + a20*b22;
    result[9]  = a01*b20 + a11*b21 + a21*b22;
    result[10] = a02*b20 + a12*b21 + a22*b22;
    result[11] = a03*b20 + a13*b21 + a23*b22;

    result[12] = mat[12];
    result[13] = mat[13];
    result[14] = mat[14];
    result[15] = mat[15];
    return result;
};

mat4.rotation = function(angle, axis)
{
    var x = axis[0], y = axis[1], z = axis[2];
    var len = Math.sqrt(x*x + y*y + z*z);
    if (!len) { throw new Error("zero-length axis"); }
    if (len != 1) {
	x /= len; 
	y /= len; 
	z /= len;
    }
    
    var s = Math.sin(angle);
    var c = Math.cos(angle);
    var t = 1-c;
    
    // Cache the matrix values (makes for huge speed increases!)
    var a00 = 1, a01 = 0, a02 = 0, a03 = 0;
    var a10 = 0, a11 = 1, a12 = 0, a13 = 0;
    var a20 = 0, a21 = 0, a22 = 1, a23 = 0;
    
    // Construct the elements of the rotation matrix
    var b00 = x*x*t + c, b01 = y*x*t + z*s, b02 = z*x*t - y*s;
    var b10 = x*y*t - z*s, b11 = y*y*t + c, b12 = z*y*t + x*s;
    var b20 = x*z*t + y*s, b21 = y*z*t - x*s, b22 = z*z*t + c;

    var result = new Float32Array(16);    
    result.buffer._type = 'mat4';
    
    // Perform rotation-specific matrix multiplication
    result[0]  = x*x*t + c;
    result[1]  = y*x*t + z*s;
    result[2]  = z*x*t - y*s;
    result[4]  = x*y*t - z*s;
    result[5]  = y*y*t + c;
    result[6]  = z*y*t + x*s;
    result[8]  = x*z*t + y*s;
    result[9]  = y*z*t - x*s;
    result[10] = z*z*t + c;
    result[15] = 1;

    return result;
};

mat4.rotate = function(mat, angle, axis)
{
    var x = axis[0], y = axis[1], z = axis[2];
    var len = Math.sqrt(x*x + y*y + z*z);
    if (!len) { throw new Error("zero-length axis"); }
    if (len != 1) {
	x /= len; 
	y /= len; 
	z /= len;
    }
    
    var s = Math.sin(angle);
    var c = Math.cos(angle);
    var t = 1-c;
    
    // Cache the matrix values (makes for huge speed increases!)
    var a00 = mat[0], a01 = mat[1], a02 = mat[2],  a03 = mat[3];
    var a10 = mat[4], a11 = mat[5], a12 = mat[6],  a13 = mat[7];
    var a20 = mat[8], a21 = mat[9], a22 = mat[10], a23 = mat[11];
    
    // Construct the elements of the rotation matrix
    var b00 = x*x*t + c, b01 = y*x*t + z*s, b02 = z*x*t - y*s;
    var b10 = x*y*t - z*s, b11 = y*y*t + c, b12 = z*y*t + x*s;
    var b20 = x*z*t + y*s, b21 = y*z*t - x*s, b22 = z*z*t + c;
    
    // Perform rotation-specific matrix multiplication
    mat[0]  = a00*b00 + a10*b01 + a20*b02;
    mat[1]  = a01*b00 + a11*b01 + a21*b02;
    mat[2]  = a02*b00 + a12*b01 + a22*b02;
    mat[3]  = a03*b00 + a13*b01 + a23*b02;
    
    mat[4]  = a00*b10 + a10*b11 + a20*b12;
    mat[5]  = a01*b10 + a11*b11 + a21*b12;
    mat[6]  = a02*b10 + a12*b11 + a22*b12;
    mat[7]  = a03*b10 + a13*b11 + a23*b12;
    
    mat[8]  = a00*b20 + a10*b21 + a20*b22;
    mat[9]  = a01*b20 + a11*b21 + a21*b22;
    mat[10] = a02*b20 + a12*b21 + a22*b22;
    mat[11] = a03*b20 + a13*b21 + a23*b22;

    mat[12] = mat[12];
    mat[13] = mat[13];
    mat[14] = mat[14];
    mat[15] = mat[15];
    return mat;
};

mat4.frustum = function(left, right, bottom, top, near, far)
{
    var result = new Float32Array(16);
    result.buffer._type = 'mat4';
    var rl = (right - left);
    var tb = (top - bottom);
    var fn = (far - near);
    result[0] = (near*2) / rl;
    result[5] = (near*2) / tb;
    result[8] = (right + left) / rl;
    result[9] = (top + bottom) / tb;
    result[10] = -(far + near) / fn;
    result[11] = -1;
    result[14] = -(far*near*2) / fn;
    return result;
};

mat4.perspective = function(fovy, aspect, near, far)
{
    var top = near*Math.tan(fovy*Math.PI / 360.0);
    var right = top*aspect;
    return mat4.frustum(-right, right, -top, top, near, far);
};

mat4.ortho = function(left, right, bottom, top, near, far)
{
    var result = new Float32Array(16);
    result.buffer._type = 'mat4';
    var rl = (right - left);
    var tb = (top - bottom);
    var fn = (far - near);
    result[0] = 2 / rl;
    result[5] = 2 / tb;
    result[10] = -2 / fn;
    result[12] = -(left + right) / rl;
    result[13] = -(top + bottom) / tb;
    result[14] = -(far + near) / fn;
    result[15] = 1;
    return result;
};

mat4.lookAt = function(eye, center, up)
{
    var result = new Float32Array(16);
    result.buffer._type = 'mat4';
    
    var eyex = eye[0],
    eyey = eye[1],
    eyez = eye[2],
    upx = up[0],
    upy = up[1],
    upz = up[2],
    centerx = center[0],
    centery = center[1],
    centerz = center[2];

    if (eyex == centerx && eyey == centery && eyez == centerz) {
	return mat4.identity();
    }
    
    var z0,z1,z2,x0,x1,x2,y0,y1,y2,len;
    
    //vec3.direction(eye, center, z);
    z0 = eyex - center[0];
    z1 = eyey - center[1];
    z2 = eyez - center[2];
    
    // normalize (no check needed for 0 because of early return)
    len = Math.sqrt(z0*z0 + z1*z1 + z2*z2);
    z0 /= len;
    z1 /= len;
    z2 /= len;
    
    //vec3.normalize(vec3.cross(up, z, x));
    x0 = upy*z2 - upz*z1;
    x1 = upz*z0 - upx*z2;
    x2 = upx*z1 - upy*z0;
    if ((len = Math.sqrt(x0*x0 + x1*x1 + x2*x2))) {
	x0 /= len;
	x1 /= len;
	x2 /= len;
    }
    
    //vec3.normalize(vec3.cross(z, x, y));
    y0 = z1*x2 - z2*x1;
    y1 = z2*x0 - z0*x2;
    y2 = z0*x1 - z1*x0;
    
    if ((len = Math.sqrt(y0*y0 + y1*y1 + y2*y2))) {
	y0 /= len;
	y1 /= len;
	y2 /= len;
    }
    
    result[0]  = x0;
    result[1]  = y0;
    result[2]  = z0;
    result[4]  = x1;
    result[5]  = y1;
    result[6]  = z1;
    result[8]  = x2;
    result[9]  = y2;
    result[10] = z2;
    result[12] = -(x0*eyex + x1*eyey + x2*eyez);
    result[13] = -(y0*eyex + y1*eyey + y2*eyez);
    result[14] = -(z0*eyex + z1*eyey + z2*eyez);
    result[15] = 1;
    
    return result;
};

mat4.frobenius_norm = function(mat)
{
    return Math.sqrt(mat[0] * mat[0] +
                     mat[1] * mat[1] +
                     mat[2] * mat[2] +
                     mat[3] * mat[3] +
                     mat[4] * mat[4] +
                     mat[5] * mat[5] +
                     mat[6] * mat[6] +
                     mat[7] * mat[7] +
                     mat[8] * mat[8] +
                     mat[9] * mat[9] +
                     mat[10] * mat[10] +
                     mat[11] * mat[11] +
                     mat[12] * mat[12] +
                     mat[13] * mat[13] +
                     mat[14] * mat[14] +
                     mat[15] * mat[15]);
};

mat4.map = function(mat, f)
{
    return mat4.make(_.map(mat, f));
};

mat4.str = function(mat)
{
    return "[ [" + mat[0] + "] [" + mat[4] + "]" + "[ [" + mat[8] + "] [" + mat[12] + "]\n" +
        "[ [" + mat[1] + "] [" + mat[5] + "]" + "[ [" + mat[9] + "] [" + mat[13] + "]\n" +
        "[ [" + mat[2] + "] [" + mat[6] + "]" + "[ [" + mat[10] + "] [" + mat[14] + "]\n" +
        "[ [" + mat[3] + "] [" + mat[7] + "]" + "[ [" + mat[11] + "] [" + mat[15] + "] ]";
};

// A thin veneer of polymorphic convenience over the fast vec classes
// for when you can get away with a little slowness.

vec[2] = vec2;
vec[3] = vec3;
vec[4] = vec4;
vec2.mat = mat2;
vec3.mat = mat3;
vec4.mat = mat4;
vec.eps = 1e-6;

vec.make = function(v)
{
    return vec[v.length].make(v);
};

vec.equal_eps = function(v1, v2)
{
    if (v1.length != v2.length) {
        throw new Error("mismatched lengths");
    }
    return vec[v1.length].equal_eps(v1, v2);
};

vec.equal = function(v1, v2)
{
    if (v1.length != v2.length) {
        throw new Error("mismatched lengths");
    }
    return vec[v1.length].equal(v1, v2);
};

vec.plus = function(v1, v2)
{
    if (v1.length != v2.length) {
        throw new Error("mismatched lengths");
    }
    return vec[v1.length].plus(v1, v2);
};

vec.minus = function(v1, v2)
{
    if (v1.length != v2.length) {
        throw new Error("mismatched lengths");
    }
    return vec[v1.length].minus(v1, v2);
};

vec.negative = function(v)
{
    return vec[v.length].negative(v);
};

vec.scaling = function(v, val)
{
    return vec[v.length].scaling(v, val);
};

vec.schur_product = function(v1, v2)
{
    if (v1.length != v2.length) {
        throw new Error("mismatched lengths");
    }
    return vec[v1.length].schur_product(v1, v2);
};

vec.normalized = function(v)
{
    return vec[v.length].normalized(v);
};

vec.length = function(v)
{
    return vec[v.length].length(v);
};

vec.length2 = function(v)
{
    return vec[v.length].length2(v);
};

vec.dot = function(v1, v2)
{
    if (v1.length != v2.length) {
        throw new Error("mismatched lengths");
    }
    return vec[v1.length].dot(v1, v2);
};

vec.map = function(c, f)
{
    return vec[c.length].map(c, f);
};

/*
// strictly speaking, this is unnecessary, since only vec3.cross exists.
// However, to force vec3.* to be written alongside vec.* would mean that
// some code would be written
// x = vec.normalized(foo);
// y = vec.normalized(bar);
// z = vec3.cross(x, y);

// instead of

// z = vec.cross(x, y);

// The notational uniformity of the latter wins
*/

vec.cross = function(v1, v2)
{
    return vec[v1.length].cross(v1, v2);
};

vec.str = function(v)
{
    return vec[v.length].str(v);
};
(function() {

mat[2] = mat2;
mat[3] = mat3;
mat[4] = mat4;
mat2.vec = vec2;
mat3.vec = vec3;
mat4.vec = vec4;
mat.eps = 1e-6;

function to_dim(l)
{
    switch (l) {
    case 4: return 2;
    case 9: return 3;
    case 16: return 4;
    }
    throw new Error("bad length");
}

mat.make = function(v)
{
    return mat[to_dim(v.length)].make(v);
};

mat.map = function(c, f)
{
    return mat[to_dim(c.length)].map(c, f);
};

mat.equal = function(m1, m2)
{
    if (m1.length != m2.length) {
        throw new Error("mismatched lengths: " + m1.length + ", " + m2.length);
    }
    return mat[to_dim(m1.length)].equal(m1, m2);
};

mat.str = function(m1)
{
    return mat[to_dim(m1.length)].str(m1);
};

})();
// run-time type information helper functions
// 
// All of this would be unnecessary if Javascript was SML. Alas,
// Javascript is no SML.
// 
//////////////////////////////////////////////////////////////////////////////

// returns false if object is not a Shade expression, or returns
// the AST type of the shade expression.
//
// For example, in some instances it is useful to know whether the
// float value comes from a constant or a GLSL uniform or an attribute 
// buffer.
Lux.is_shade_expression = function(obj)
{
    return typeof obj === 'function' && obj._lux_expression && obj.expression_type;
};

//////////////////////////////////////////////////////////////////////////////
// http://javascript.crockford.com/remedial.html

// Notice that lux_typeOf is NOT EXACTLY equal to
// 
//   http://javascript.crockford.com/remedial.html
//
// In particular, lux_typeOf will return "object" if given Shade expressions
// 
// Shade expressions are actually functions with a bunch of extra methods.
// 
// This is something of a hack, but it is the simplest way I know of to get
// operator() overloading, which turns out to be notationally quite powerful.
//

function lux_typeOf(value) 
{
    var s = typeof value;
    if (s === 'function' && value._lux_expression)
        return 'object'; // shade expression
    if (s === 'object') {
        if (value) {
            if (typeof value.length === 'number'
                 && !(value.propertyIsEnumerable('length'))
                 && typeof value.splice === 'function')  { // typed array
                s = 'array';
            }
        } else {
            s = 'null';
        }
    }
    return s;
}
/*
 * Lux.attribute_buffer_view builds an attribute_buffer object from an
 * Lux.buffer object, instead of an array (or typed array). The main
 * use case for attribute_buffer_view is to allow one to build
 * several attribute_buffer_views over the same Lux.buffer, for efficient
 * strided attribute buffers (which share the same buffer)
 * 
 * The main difference between calling Lux.attribute_buffer_view and
 * Lux.attribute_buffer is that attribute_buffer_view takes a "buffer"
 * parameter instead of an "array" parameter.
 * 
 */

Lux.attribute_buffer_view = function(opts)
{
    var ctx = Lux._globals.ctx;
    opts = _.defaults(opts, {
        item_size: 3,
        item_type: 'float',
        normalized: false,
        keep_array: false,
        stride: 0,
        offset: 0
    });

    if (_.isUndefined(opts.buffer)) {
        throw new Error("opts.buffer must be defined");
    }

    var itemSize = opts.item_size;
    if ([1,2,3,4].indexOf(itemSize) === -1) {
        throw new Error("opts.item_size must be one of 1, 2, 3, or 4");
    }

    var normalized = opts.normalized;
    if (lux_typeOf(normalized) !== "boolean") {
        throw new Error("opts.normalized must be boolean");
    }

    var gl_enum_typed_array_map = {
        'float': { webgl_enum: ctx.FLOAT, typed_array_ctor: Float32Array, size: 4 },
        'short': { webgl_enum: ctx.SHORT, typed_array_ctor: Int16Array, size: 2 },
        'ushort': { webgl_enum: ctx.UNSIGNED_SHORT, typed_array_ctor: Uint16Array, size: 2 },
        'byte': { webgl_enum: ctx.BYTE, typed_array_ctor: Int8Array, size: 1 },
        'ubyte': { webgl_enum: ctx.UNSIGNED_BYTE, typed_array_ctor: Uint8Array, size: 1 }
    };

    var itemType = gl_enum_typed_array_map[opts.item_type];
    if (_.isUndefined(itemType)) {
        throw new Error("opts.item_type must be 'float', 'short', 'ushort', 'byte' or 'ubyte'");
    }

    function convert_array(array) {
        var numItems;
        if (array.constructor === Array) {
            if (array.length % itemSize) {
                throw new Error("set: attribute_buffer expected length to be a multiple of " + 
                    itemSize + ", got " + array.length + " instead.");
            }
            array = new itemType.typed_array_ctor(array);
        } else if (array.constructor === itemType._typed_array_ctor) {
            if (array.length % itemSize) {
                throw new Error("set: attribute_buffer expected length to be a multiple of " + 
                    itemSize + ", got " + array.length + " instead.");
            }
        } else if (opts.vertex_array.constructor === ArrayBuffer) {
            array = opts.vertex_array;
        }
        return array;
    }

    var result = {
        buffer: opts.buffer,
        itemSize: itemSize,
        normalized: normalized,
        numItems: opts.buffer.byteLength / (opts.stride || itemSize * itemType.size),
        stride: opts.stride,
        offset: opts.offset,
        _ctx: ctx,
        _shade_type: 'attribute_buffer',
        _webgl_type: itemType.webgl_enum,
        _typed_array_ctor: itemType.typed_array_ctor,
        _word_length: itemType.size,
        _item_byte_length: opts.stride || itemType.size * itemSize,
        set: function(vertex_array) {
            vertex_array = convert_array(vertex_array);
            this.buffer.set(vertex_array);
            this.numItems = this.buffer.byteLength / (this.stride || this.itemSize * this._word_length);
            if (opts.keep_array) {
                this.array = this.buffer.array;
            }
        },
        set_region: function() {
            throw new Error("currently unimplemented");
        },
        //////////////////////////////////////////////////////////////////////
        // These methods are only for internal use within Lux
        bind: function(attribute) {
            Lux.set_context(ctx);
            ctx.bindBuffer(ctx.ARRAY_BUFFER, this.buffer);
            ctx.vertexAttribPointer(attribute, this.itemSize, this._webgl_type, normalized, this.stride, this.offset);
        },
        draw: function(primitive) {
            Lux.set_context(ctx);
            ctx.drawArrays(primitive, 0, this.numItems);
        },
        bind_and_draw: function(attribute, primitive) {
            // here we inline the calls to bind and draw to shave a redundant set_context.
            Lux.set_context(ctx);
            ctx.bindBuffer(ctx.ARRAY_BUFFER, this.buffer);
            ctx.vertexAttribPointer(attribute, this.itemSize, this._webgl_type, normalized, this.stride, this.offset);
            ctx.drawArrays(primitive, 0, this.numItems);
        }
    };
    if (opts.keep_array)
        result.array = result.buffer.array;
    return result;
};
/*
 * Lux.attribute_buffer creates the structures necessary for Lux to handle 
 * per-vertex data.
 * 
 * Typically these will be vertex positions, normals, texture coordinates, 
 * colors, etc.
 * 
 * options: 
 * 
 *   vertex_array is the data array to be used. It must be one of the following 
 *     datatypes:
 * 
 *     - a javascript array of values, (which will be converted to a typed array
 *     of the appropriate type)
 * 
 *     - a typed array whose type matches the passed type below
 * 
 *     - an ArrayBuffer of the appropriate size
 * 
 *   item_size is the number of elements to be associated with each vertex
 * 
 *   item_type is the data type of each element. Default is 'float', for
 *     IEEE 754 32-bit floating point numbers.
 * 
 *   usage follows the WebGL bufferData call. From the man page for bufferData:
 * 
 *     Specifies the expected usage pattern of the data store. The symbolic 
 *     constant must be STREAM_DRAW, STATIC_DRAW, or DYNAMIC_DRAW.
 * 
 *   keep_array tells Lux.attribute_buffer to keep a copy of the buffer in 
 *   Javascript. This will be stored in the returned object, in the "array" 
 *   property. It is useful for javascript-side inspection, or as a convenient
 *   place to keep the array stashed in case you need it.
 * 
 *   stride: if stride is non-zero, WebGL will skip an arbitrary number of 
 *   bytes per element. This is used to specify many different attributes which
 *   share a single buffer (which gives memory locality advantages in some
 *   GPU architectures). stride uses *bytes* as units, so be aware of datatype
 *   conversions.
 * 
 *   offset: gives the offset into the buffer at which to access the data,
 *   again used to specify different attributes sharing a single buffer.
 *   offset uses *bytes* as units, so be aware of datatype conversions.
 * 
 * 
 * Example usage:
 * 
 *   // associate three 32-bit floating-point values with each vertex
 *   var position_attribute = Lux.attribute_buffer({
 *       vertex_array: [1,0,0, 0,1,0, 1,0,0],
 *       // item_size: 3 is the default
 *       // item_type: 'float' is the default
 *   })
 * 
 *   // associate four 8-bit unsigned bytes with each vertex
 *   var color_attribute = Lux.attribute_buffer({
 *       vertex_array: [1,0,0,1, 1,1,0,1, 1,1,1,1],
 *       item_size: 4,
 *       item_type: 'ubyte', // the default item_type is 'float'
 *       normalized: true // when 
 *   });
 *   ...
 * 
 *   var triangle = Lux.model({
 *       type: 'triangles',
 *       position: position_attribute,
 *       color: color_attribute
 *   })
 */

Lux.attribute_buffer = function(opts)
{
    var ctx = Lux._globals.ctx;
    opts = _.defaults(opts, {
        item_size: 3,
        item_type: 'float',
        usage: ctx.STATIC_DRAW,
        normalized: false,
        keep_array: false,
        stride: 0,
        offset: 0
    });

    var itemSize = opts.item_size;
    if ([1,2,3,4].indexOf(itemSize) === -1) {
        throw new Error("opts.item_size must be one of 1, 2, 3, or 4");
    }

    var gl_enum_typed_array_map = {
        'float': { webgl_enum: ctx.FLOAT, typed_array_ctor: Float32Array, size: 4 },
        'short': { webgl_enum: ctx.SHORT, typed_array_ctor: Int16Array, size: 2 },
        'ushort': { webgl_enum: ctx.UNSIGNED_SHORT, typed_array_ctor: Uint16Array, size: 2 },
        'byte': { webgl_enum: ctx.BYTE, typed_array_ctor: Int8Array, size: 1 },
        'ubyte': { webgl_enum: ctx.UNSIGNED_BYTE, typed_array_ctor: Uint8Array, size: 1 }
    };

    var itemType = gl_enum_typed_array_map[opts.item_type];
    if (_.isUndefined(itemType)) {
        throw new Error("opts.item_type must be 'float', 'short', 'ushort', 'byte' or 'ubyte'");
    }

    if (_.isUndefined(opts.vertex_array)) {
        throw new Error("opts.vertex_array must be defined");
    }

    function convert_array(array) {
        var numItems;
        if (array.constructor === Array) {
            if (array.length % itemSize) {
                throw new Error("set: attribute_buffer expected length to be a multiple of " + 
                    itemSize + ", got " + array.length + " instead.");
            }
            array = new itemType.typed_array_ctor(array);
        } else if (array.constructor === itemType.typed_array_ctor) {
            if (array.length % itemSize) {
                throw new Error("set: attribute_buffer expected length to be a multiple of " + 
                    itemSize + ", got " + array.length + " instead.");
            }
        } else if (opts.vertex_array.constructor === ArrayBuffer) {
            array = opts.vertex_array;
        } else {
            throw new Error("Unrecognized array type for attribute_buffer");
        }
        return array;
    }

    var array = convert_array(opts.vertex_array);
    var buffer = Lux.buffer({
        usage: opts.usage,
        array: array,
        keep_array: opts.keep_array
    });

    return Lux.attribute_buffer_view(_.defaults(opts, {
        buffer: buffer
    }));
};
Lux.buffer = function(opts)
{
    var ctx = Lux._globals.ctx;
    opts = _.defaults(opts, {
        usage: ctx.STATIC_DRAW,
        keep_array: false
    });

    if (_.isUndefined(opts.array)) {
        throw new Error("opts.array must be defined");
    }

    var usage = opts.usage;
    if ([ctx.STATIC_DRAW, ctx.DYNAMIC_DRAW, ctx.STREAM_DRAW].indexOf(usage) === -1) {
        throw new Error("opts.usage must be one of STATIC_DRAW, DYNAMIC_DRAW, STREAM_DRAW");
    }

    var result = ctx.createBuffer();
    result.usage = usage;
    result.set = function(array) {
        ctx.bindBuffer(ctx.ARRAY_BUFFER, this);
        ctx.bufferData(ctx.ARRAY_BUFFER, array, this.usage);
        if (opts.keep_array) {
            this.array = array;
        }
        this.byteLength = array.byteLength;
    };
    result.set(opts.array);
    result.set_region = function() {
        throw new Error("currently unimplemented");
    };

    return result;
};
(function() {

var previous_batch_opts = {};
Lux.get_current_batch_opts = function()
{
    return previous_batch_opts;
};

Lux.unload_batch = function()
{
    if (!previous_batch_opts._ctx)
        return;
    var ctx = previous_batch_opts._ctx;
    if (previous_batch_opts.attributes) {
        for (var key in previous_batch_opts.attributes) {
            ctx.disableVertexAttribArray(previous_batch_opts.program[key]);
        }
        _.each(previous_batch_opts.program.uniforms, function (uniform) {
            delete uniform._lux_active_uniform;
        });
    }
    // FIXME setting line width belongs somewhere else, but I'm not quite sure where.
    // resets line width
    if (previous_batch_opts.line_width)
        ctx.lineWidth(1.0);

    // reset the opengl capabilities which are determined by
    // Lux.DrawingMode.*
    ctx.disable(ctx.DEPTH_TEST);
    ctx.disable(ctx.BLEND);
    ctx.depthMask(true);

    previous_batch_opts = {};
};

function draw_it(batch_opts)
{
    if (_.isUndefined(batch_opts))
        throw new Error("drawing mode undefined");

    // When the batch_options object is different from the one previously drawn,
    // we must set up the appropriate state for drawing.
    if (batch_opts.batch_id !== previous_batch_opts.batch_id) {
        var attributes = batch_opts.attributes || {};
        var uniforms = batch_opts.uniforms || {};
        var program = batch_opts.program;
        var key;

        Lux.unload_batch();
        previous_batch_opts = batch_opts;
        batch_opts.set_caps();

        var ctx = batch_opts._ctx;
        ctx.useProgram(program);

        for (key in attributes) {
            var attr = program[key];
            if (!_.isUndefined(attr)) {
                ctx.enableVertexAttribArray(attr);
                var buffer = attributes[key].get();
                if (!buffer) {
                    throw new Error("Unset Shade.attribute " + attributes[key]._attribute_name);
                }
                buffer.bind(attr);
            }
        }
        
        var currentActiveTexture = 0;
        _.each(program.uniforms, function(uniform) {
            var key = uniform.uniform_name;
            var call = uniform.uniform_call,
                value = uniform.get();
            if (_.isUndefined(value)) {
                throw new Error("parameter " + key + " has not been set.");
            }
            var t = Shade.Types.type_of(value);
            if (t.equals(Shade.Types.other_t)) {
                uniform._lux_active_uniform = (function(uid, cat) {
                    return function(v) {
                        ctx.activeTexture(ctx.TEXTURE0 + cat);
                        ctx.bindTexture(ctx.TEXTURE_2D, v);
                        ctx.uniform1i(uid, cat);
                    };
                })(program[key], currentActiveTexture);
                currentActiveTexture++;
            } else if (t.equals(Shade.Types.float_t) || 
                       t.equals(Shade.Types.bool_t) ||
                       t.repr().substr(0,3) === "vec") {
                uniform._lux_active_uniform = (function(call, uid) {
                    return function(v) {
                        call.call(ctx, uid, v);
                    };
                })(ctx[call], program[key]);
            } else if (t.repr().substr(0,3) === "mat") {
                uniform._lux_active_uniform = (function(call, uid) {
                    return function(v) {
                        ctx[call](uid, false, v);
                    };
                })(call, program[key]);
            } else {
                throw new Error("could not figure out parameter type! " + t);
            }
            uniform._lux_active_uniform(value);
        });
    }

    batch_opts.draw_chunk();
}

var largest_batch_id = 1;

Lux.bake = function(model, appearance, opts)
{
    appearance = Shade.canonicalize_program_object(appearance);
    opts = _.defaults(opts || {}, {
        force_no_draw: false,
        force_no_pick: false,
        force_no_unproject: false
    });
    var ctx = model._ctx || Lux._globals.ctx;

    if (_.isUndefined(appearance.gl_FragColor)) {
        appearance.gl_FragColor = Shade.vec(1,1,1,1);
    }

    // these are necessary outputs which must be compiled by Shade.program
    function is_program_output(key)
    {
        return ["color", "position", "point_size",
                "gl_FragColor", "gl_Position", "gl_PointSize"].indexOf(key) != -1;
    };

    if (appearance.gl_Position.type.equals(Shade.Types.vec2)) {
        appearance.gl_Position = Shade.vec(appearance.gl_Position, 0, 1);
    } else if (appearance.gl_Position.type.equals(Shade.Types.vec3)) {
        appearance.gl_Position = Shade.vec(appearance.gl_Position, 1);
    } else if (!appearance.gl_Position.type.equals(Shade.Types.vec4)) {
        throw new Error("position appearance attribute must be vec2, vec3 or vec4");
    }

    var batch_id = Lux.fresh_pick_id();

    function build_attribute_arrays_obj(prog) {
        return _.build(_.map(
            prog.attribute_buffers, function(v) { return [v._attribute_name, v]; }
        ));
    }

    function process_appearance(val_key_function) {
        var result = {};
        _.each(appearance, function(value, key) {
            if (is_program_output(key)) {
                result[key] = val_key_function(value, key);
            }
        });
        return Shade.program(result);
    }

    function create_draw_program() {
        return process_appearance(function(value, key) {
            return value;
        });
    }

    function create_pick_program() {
        var pick_id;
        if (appearance.pick_id)
            pick_id = Shade(appearance.pick_id);
        else {
            pick_id = Shade(Shade.id(batch_id));
        }
        return process_appearance(function(value, key) {
            if (key === 'gl_FragColor') {
                var pick_if = (appearance.pick_if || 
                               Shade(value).swizzle("a").gt(0));
                return pick_id.discard_if(Shade.not(pick_if));
            } else
                return value;
        });
    }

    /* Lux unprojecting uses the render-as-depth technique suggested
     by Benedetto et al. in the SpiderGL paper in the context of
     shadow mapping:

     SpiderGL: A JavaScript 3D Graphics Library for Next-Generation
     WWW

     Marco Di Benedetto, Federico Ponchio, Fabio Ganovelli, Roberto
     Scopigno. Visual Computing Lab, ISTI-CNR

     http://vcg.isti.cnr.it/Publications/2010/DPGS10/spidergl.pdf

     FIXME: Perhaps there should be an option of doing this directly as
     render-to-float-texture.

     */
    
    function create_unproject_program() {
        return process_appearance(function(value, key) {
            if (key === 'gl_FragColor') {
                var position_z = appearance.gl_Position.swizzle('z'),
                    position_w = appearance.gl_Position.swizzle('w');
                var normalized_z = position_z.div(position_w).add(1).div(2);

                // normalized_z ranges from 0 to 1.

                // an opengl z-buffer actually stores information as
                // 1/z, so that more precision is spent on the close part
                // of the depth range. Here, we are storing z, and so our efficiency won't be great.
                // 
                // However, even 1/z is only an approximation to the ideal scenario, and 
                // if we're already doing this computation on a shader, it might be worthwhile to use
                // Thatcher Ulrich's suggestion about constant relative precision using 
                // a logarithmic mapping:

                // http://tulrich.com/geekstuff/log_depth_buffer.txt

                // This mapping, incidentally, is more directly interpretable as
                // linear interpolation in log space.

                var result_rgba = Shade.vec(
                    normalized_z,
                    normalized_z.mul(1 << 8),
                    normalized_z.mul(1 << 16),
                    normalized_z.mul(1 << 24)
                );
                return result_rgba;
            } else
                return value;
        });
    }

    var primitive_types = {
        points: ctx.POINTS,
        line_strip: ctx.LINE_STRIP,
        line_loop: ctx.LINE_LOOP,
        lines: ctx.LINES,
        triangle_strip: ctx.TRIANGLE_STRIP,
        triangle_fan: ctx.TRIANGLE_FAN,
        triangles: ctx.TRIANGLES
    };

    var primitive_type = primitive_types[model.type];
    var elements = model.elements;
    var draw_chunk;
    if (lux_typeOf(elements) === 'number') {
        draw_chunk = function() {
            // it's important to use "model.elements" here instead of "elements" because
            // the indirection captures the fact that the model might have been updated with
            // a different number of elements, by changing the attribute buffers.
            // 
            // FIXME This is a phenomentally bad way to go about this problem, but let's go with it for now.
            ctx.drawArrays(primitive_type, 0, model.elements);
        };
    } else {
        if (elements._shade_type === 'attribute_buffer') {
            draw_chunk = function() {
                model.elements.draw(primitive_type);
            };
        } else if (elements._shade_type === 'element_buffer') {
            draw_chunk = function() {
                model.elements.bind_and_draw(primitive_type);
            };
        } else
            throw new Error("model.elements must be a number, an element buffer or an attribute buffer");
    }

    // FIXME the batch_id field in the batch_opts objects is not
    // the same as the batch_id in the batch itself. 
    // 
    // The former is used to avoid state switching, while the latter is
    // a generic automatic id which might be used for picking, for
    // example.
    // 
    // This should not lead to any problems right now but might be confusing to
    // readers.

    function create_batch_opts(program, caps_name) {
        function ensure_parameter(v) {
            if (lux_typeOf(v) === 'number')
                return Shade.parameter("float", v);
            else if (Lux.is_shade_expression(v) === 'parameter')
                return v;
            else throw new Error("expected float or parameter, got " + v + " instead.");
        }
        var result = {
            _ctx: ctx,
            program: program,
            attributes: build_attribute_arrays_obj(program),
            set_caps: function() {
                var ctx = Lux._globals.ctx;
                var mode_caps = ((appearance.mode && appearance.mode[caps_name]) ||
                       Lux.DrawingMode.standard[caps_name]);
                mode_caps();
                if (this.line_width) {
                    ctx.lineWidth(this.line_width.get());
                }
            },
            draw_chunk: draw_chunk,
            batch_id: largest_batch_id++
        };
        if (!_.isUndefined(appearance.line_width))
            result.line_width = ensure_parameter(appearance.line_width);
        return result;
    }

    var draw_opts, pick_opts, unproject_opts;

    if (!opts.force_no_draw)
        draw_opts = create_batch_opts(create_draw_program(), "set_draw_caps");

    if (!opts.force_no_pick)
        pick_opts = create_batch_opts(create_pick_program(), "set_pick_caps");

    if (!opts.force_no_unproject)
        unproject_opts = create_batch_opts(create_unproject_program(), "set_unproject_caps");

    var which_opts = [ draw_opts, pick_opts, unproject_opts ];

    var result = {
        model: model,
        batch_id: batch_id,
        draw: function() {
            draw_it(which_opts[ctx._lux_globals.batch_render_mode]);
        },
        // in case you want to force the behavior, or that
        // single array lookup is too slow for you.
        _draw: function() {
            draw_it(draw_opts);
        },
        _pick: function() {
            draw_it(pick_opts);
        },
        // for debugging purposes
        _batch_opts: function() { return which_opts; }
    };
    return result;
};
})();
Lux.batch_list = function(lst)
{
    lst = lst.slice().reverse();
    return {
        list: lst,
        draw: function() {
            var i=this.list.length;
            var lst = this.list;
            while (i--) {
                lst[i].draw();
            }
        }
    };
};
Lux.conditional_batch = function(batch, condition)
{
    return {
        draw: function() {
            if (condition()) batch.draw();
        }
    };
};

Lux.conditional_actor = function(opts)
{
    opts = _.clone(opts);
    opts.bake = function(model, changed_appearance) {
        return Lux.conditional_batch(Lux.bake(model, changed_appearance), opts.condition);
    };
    return Lux.actor(opts);
};
Lux.bake_many = function(model_list, 
                         appearance_function,
                         model_callback)
{
    var scratch_model = _.clone(model_list[0]);
    var batch = Lux.bake(scratch_model, appearance_function(scratch_model));
    return model_callback ? {
        draw: function() {
            _.each(model_list, function(model, i) {
                _.each(scratch_model.attributes, function(v, k) {
                    v.set(model[k].get());
                });
                scratch_model.elements.set(model.elements.array);
                model_callback(model, i);
                batch.draw();
            });
        }
    }:{
        draw: function() {
            _.each(model_list, function(model, i) {
                _.each(scratch_model.attributes, function(v, k) {
                    v.set(model[k].get());
                });
                scratch_model.elements.set(model.elements.array);
                batch.draw();
            });
        }
    };
};
// FIXME make API similar to Lux.attribute_buffer
Lux.element_buffer = function(vertex_array)
{
    var ctx = Lux._globals.ctx;
    var result = ctx.createBuffer();
    result._ctx = ctx;
    result._shade_type = 'element_buffer';
    result.itemSize = 1;
    var draw_enum;

    //////////////////////////////////////////////////////////////////////////
    // These methods are only for internal use within Lux

    result.set = function(vertex_array) {
        Lux.set_context(ctx);
        var typedArray;
        var typed_array_ctor;
        var has_extension = ctx._lux_globals.webgl_extensions.OES_element_index_uint;
        if (has_extension)
            typed_array_ctor = Uint32Array;
        else
            typed_array_ctor = Uint16Array;

        if (vertex_array.constructor.name === 'Array') {
            typedArray = new typed_array_ctor(vertex_array);
        } else {
            if (has_extension) {
                if (vertex_array.constructor !== Uint16Array &&
                    vertex_array.constructor !== Uint32Array) {
                    throw new Error("Lux.element_buffer.set requires either a plain list, a Uint16Array, or a Uint32Array");
                }
            } else {
                if (vertex_array.constructor !== Uint16Array) {
                    throw new Error("Lux.element_buffer.set requires either a plain list or a Uint16Array");
                }
            }
            typedArray = vertex_array;
        }
        ctx.bindBuffer(ctx.ELEMENT_ARRAY_BUFFER, this);
        ctx.bufferData(ctx.ELEMENT_ARRAY_BUFFER, typedArray, ctx.STATIC_DRAW);
        if (typedArray.constructor === Uint16Array)
            draw_enum = ctx.UNSIGNED_SHORT;
        else if (typedArray.constructor === Uint32Array)
            draw_enum = ctx.UNSIGNED_INT;
        else
            throw new Error("internal error: expecting typed array to be either Uint16 or Uint32");
        this.array = typedArray;
        this.numItems = typedArray.length;
    };
    result.set(vertex_array);

    result.bind = function() {
        ctx.bindBuffer(ctx.ELEMENT_ARRAY_BUFFER, this);
    };
    result.draw = function(primitive) {
        ctx.drawElements(primitive, this.numItems, draw_enum, 0);
    };
    result.bind_and_draw = function(primitive) {
        this.bind();
        this.draw(primitive);
    };
    return result;
};
// Call this to get a guaranteed unique range of picking ids.
// Useful to avoid name conflicts between automatic ids and
// user-defined ids.

(function() {

var latest_pick_id = 1;

Lux.fresh_pick_id = function(quantity)
{
    quantity = quantity || 1;
    var result = latest_pick_id;
    latest_pick_id += quantity;
    return result;
};

})();
Lux.id_buffer = function(vertex_array)
{
    if (lux_typeOf(vertex_array) !== 'array')
        throw new Error("id_buffer expects array of integers");
    var typedArray = new Int32Array(vertex_array);
    var byteArray = new Uint8Array(typedArray.buffer);
    return Lux.attribute_buffer({
        vertex_array: byteArray, 
        item_size: 4, 
        item_type: 'ubyte', 
        normalized: true
    });
};
(function() {

function initialize_context_globals(gl)
{
    gl._lux_globals = {};

    // batches can currently be rendered in "draw" or "pick" mode.
    // draw: 0
    // pick: 1
    // these are indices into an array defined inside Lux.bake
    // For legibility, they should be strings, but for speed, they'll be integers.
    gl._lux_globals.batch_render_mode = 0;

    // epoch is the initial time being tracked by the context.
    gl._lux_globals.epoch = new Date().getTime() / 1000;

    gl._lux_globals.devicePixelRatio = undefined;

    // Optional, enabled WebGL extensions go here.
    gl._lux_globals.webgl_extensions = {};

    // from https://developer.mozilla.org/en-US/docs/JavaScript/Typed_arrays/DataView
    gl._lux_globals.little_endian = (function() {
        var buffer = new ArrayBuffer(2);
        new DataView(buffer).setInt16(0, 256, true);
        return new Int16Array(buffer)[0] === 256;
    })();
}

////////////////////////////////////////////////////////////////////////////////

function polyfill_event(event, gl)
{
    // polyfill event.offsetX and offsetY in Firefox,
    // according to http://bugs.jquery.com/ticket/8523
    if(typeof event.offsetX === "undefined" || typeof event.offsetY === "undefined") {
        var targetOffset = $(event.target).offset();
        event.offsetX = event.pageX - targetOffset.left;
        event.offsetY = event.pageY - targetOffset.top;
    }
    
    event.luxX = event.offsetX * gl._lux_globals.devicePixelRatio;
    event.luxY = gl.viewportHeight - event.offsetY * gl._lux_globals.devicePixelRatio;
}

Lux.init = function(opts)
{
    opts = _.defaults(opts || {}, {
        clearColor: [1,1,1,0],
        clearDepth: 1.0,
        attributes: {
            alpha: true,
            depth: true,
            preserveDrawingBuffer: true
        },
        highDPS: true
    });

    var canvas = opts.canvas;
    if (_.isUndefined(canvas)) {
        var q = $("canvas");
        if (q.length === 0) {
            throw new Error("no canvas elements found in document");
        }
        if (q.length > 1) {
            throw new Error("More than one canvas element found in document; please specify a canvas option in Lux.init");
        }
        canvas = q[0];
    }

    canvas.unselectable = true;
    canvas.onselectstart = function() { return false; };
    var gl;

    var devicePixelRatio = 1;

    if (opts.highDPS) {
        devicePixelRatio = window.devicePixelRatio || 1;
        canvas.style.width = canvas.width;
        canvas.style.height = canvas.height;
        canvas.width = (canvas.clientWidth || canvas.width) * devicePixelRatio;
        canvas.height = (canvas.clientHeight || canvas.height) * devicePixelRatio;
    }

    try {
        if ("attributes" in opts) {
            gl = WebGLUtils.setupWebGL(canvas, opts.attributes);
            var x = gl.getContextAttributes();
            for (var key in opts.attributes) {
                if (opts.attributes[key] !== x[key]) {
                    throw new Error("requested attribute " + 
                           key + ": " + opts.attributes[key] +
                           " could not be satisfied");
                }
            }
        } else
            gl = WebGLUtils.setupWebGL(canvas);
        if (!gl)
            throw new Error("failed context creation");
        initialize_context_globals(gl);
        if ("interactor" in opts) {
            opts.interactor.resize && opts.interactor.resize(canvas.width, canvas.height);
            for (var key in opts.interactor.events) {
                if (opts[key]) {
                    opts[key] = (function(handler, interactor_handler) {
                        return function(event) {
                            var v = handler(event);
                            return v && interactor_handler(event);
                        };
                    })(opts[key], opts.interactor.events[key]);
                } else {
                    opts[key] = opts.interactor.events[key];
                }
            }
        }
        
        if (opts.debugging) {
            var throwOnGLError = function(err, funcName, args) {
                throw new Error(WebGLDebugUtils.glEnumToString(err) + 
                    " was caused by call to " + funcName);
            };
            gl = WebGLDebugUtils.makeDebugContext(gl, throwOnGLError, opts.tracing);
        }

        gl.viewportWidth = canvas.width;
        gl.viewportHeight = canvas.height;

        //////////////////////////////////////////////////////////////////////
        // event handling

        var canvas_events = ["mouseover", "mousemove", "mousedown", "mouseout", 
                             "mouseup", "dblclick"];
        _.each(canvas_events, function(ename) {
            var listener = opts[ename];
            function internal_listener(event) {
                polyfill_event(event, gl);
                if (!Lux.Scene.on(ename, event, gl))
                    return false;
                if (listener)
                    return listener(event);
                return true;
            }
            canvas.addEventListener(ename, Lux.on_context(gl, internal_listener), false);
        });
        
        if (!_.isUndefined(opts.mousewheel)) {
            $(canvas).bind('mousewheel', function(event, delta, deltaX, deltaY) {
                polyfill_event(event, gl);
                return opts.mousewheel(event, delta, deltaX, deltaY);
            });
        };

        //////////////////////////////////////////////////////////////////////

        var ext;
        var exts = gl.getSupportedExtensions();
        _.each(["OES_texture_float", "OES_standard_derivatives"], function(ext) {
            if (exts.indexOf(ext) === -1 ||
                (gl.getExtension(ext)) === null) { // must call this to enable extension
                alert(ext + " is not available on your browser/computer! " +
                      "Lux will not work, sorry.");
                throw new Error("insufficient GPU support");
            }
        });
        _.each(["WEBKIT_EXT_texture_filter_anisotropic",
                "EXT_texture_filter_anisotropic"], 
               function(ext) {
                   if (exts.indexOf(ext) !== -1 && (gl.getExtension(ext) !== null)) {
                       gl._lux_globals.webgl_extensions.EXT_texture_filter_anisotropic = true;
                       gl.TEXTURE_MAX_ANISOTROPY_EXT     = 0x84FE;
                       gl.MAX_TEXTURE_MAX_ANISOTROPY_EXT = 0x84FF;
                   }
               });
        if (exts.indexOf("OES_element_index_uint") !== -1 &&
            gl.getExtension("OES_element_index_uint") !== null) {
            gl._lux_globals.webgl_extensions.OES_element_index_uint = true;
        }
    } catch(e) {
        alert(e);
        throw e;
    }
    if (!gl) {
        alert("Could not initialize WebGL, sorry :-(");
        throw new Error("failed initalization");
    }

    gl._lux_globals.devicePixelRatio = devicePixelRatio;

    Lux.set_context(gl);

    gl.resize = function(width, height) {
        this.parameters.width.set(width);
        this.parameters.height.set(height);
        if (opts.highDPS) {
            this.viewportWidth = width * devicePixelRatio;
            this.viewportHeight = height * devicePixelRatio;
            this.canvas.style.width = width;
            this.canvas.style.height = height;
            this.canvas.width = this.canvas.clientWidth * devicePixelRatio;
            this.canvas.height = this.canvas.clientHeight * devicePixelRatio;
            if (opts.resize)
                opts.resize(width, height);
        } else {
            this.viewportWidth = width;
            this.viewportHeight = height;
            this.canvas.width = width;
            this.canvas.height = height;
            if (opts.resize)
                opts.resize(width, height);
        }
    };
    gl.parameters = {};
    if (opts.highDPS) {
        gl.parameters.width = Shade.parameter("float", gl.viewportWidth / devicePixelRatio);
        gl.parameters.height = Shade.parameter("float", gl.viewportHeight / devicePixelRatio);
    } else {
        gl.parameters.width = Shade.parameter("float", gl.viewportWidth);
        gl.parameters.height = Shade.parameter("float", gl.viewportHeight);
    }
    gl.parameters.now = Shade.parameter("float", gl._lux_globals.epoch);
    gl.parameters.frame_duration = Shade.parameter("float", 0);

    gl._lux_globals.scene = Lux.default_scene({
        context: gl,
        clearColor: opts.clearColor,
        clearDepth: opts.clearDepth,
        pre_draw: function() {
            var raw_t = new Date().getTime() / 1000;
            var new_t = raw_t - gl._lux_globals.epoch;
            var old_t = gl.parameters.now.get();
            gl.parameters.frame_duration.set(new_t - old_t);
            gl.parameters.now.set(new_t);
            gl.viewport(0, 0, gl.viewportWidth, gl.viewportHeight);
        }
    });

    if ("interactor" in opts) {
        gl._lux_globals.scene.add(opts.interactor.scene);
        gl._lux_globals.scene = opts.interactor.scene;
    }

    return gl;
};

})();
Lux.identity = function()
{
    return mat4.identity();
};

Lux.translation = function(v)
{
    function t_3x3(ar) {
        var r = mat3.create();
        r[6] = ar[0];
        r[7] = ar[1];
        return r;
    }
    function t_4x4(ar) {
        return mat4.translation(ar);
    }
    if (v.length === 3) return t_4x4(v);
    else if (arguments.length === 3) return t_4x4(arguments);
    else if (v.length === 2) return t_3x3(v);
    else if (arguments.length === 2) return t_3x3(arguments);

    throw new Error("invalid vector size for translation");
};

Lux.scaling = function (v)
{
    var ar;
    function s_3x3(ar) {
        var r = mat3.create();
        r[0] = ar[0];
        r[4] = ar[1];
        return r;
    }
    function s_4x4(ar) {
        return mat4.scaling(ar);
    }

    if (v.length === 3) return s_4x4(v);
    else if (arguments.length === 3) return s_4x4(arguments);
    else if (v.length === 2) return s_3x3(v);
    else if (arguments.length === 2) return s_3x3(arguments);

    throw new Error("invalid size for scale");
};

Lux.rotation = function(angle, axis)
{
    return mat4.rotation(angle, axis);
};

Lux.look_at = function(ex, ey, ez, cx, cy, cz, ux, uy, uz)
{
    return mat4.lookAt([ex, ey, ez], [cx, cy, cz], [ux, uy, uz]);
};

Lux.perspective = mat4.perspective;

Lux.frustum = mat4.frustum;

Lux.ortho = mat4.ortho;

Lux.shear = function(xf, yf)
{
    return mat4.create([1, 0, xf, 0,
                        0, 1, yf, 0,
                        0, 0, 1, 0,
                        0, 0, 0, 1]);
};
// This function is fairly ugly, but I'd rather this function be ugly
// than the code which calls it be ugly.
Lux.model = function(input)
{
    var n_elements;
    function push_into(array, dimension) {
        return function(el) {
            var v = el.constant_value();
            for (var i=0; i<dimension; ++i)
                array.push(v[i]);
        };
    }

    var result = {
        add: function(k, v) {
            // First we handle the mandatory keys: "type" and "elements"
            if (k === 'type')
                // example: 'type: "triangles"'
                result.type = v;
            else if (k === 'elements') {
                if (v._shade_type === 'element_buffer') {
                    // example: 'elements: Lux.element_buffer(...)'
                    result.elements = v;
                } else if (lux_typeOf(v) === 'number') {
                    // example: 'elements: 4'
                    result.elements = v;
                } else { // if (lux_typeOf(v) === 'array') {
                    // example: 'elements: [0, 1, 2, 3]'
                    // example: 'elements: new Int16Array([0, 1, 2, 3])'
                    // example: 'elements: new Int32Array([0, 1, 2, 3])' (WebGL only supports 16-bit elements, so Lux converts this to a 16-bit element array)
                    result.elements = Lux.element_buffer(v);
                } 
            }
            // Then we handle the model attributes. They can be ...
            else if (v._shade_type === 'attribute_buffer') { // ... attribute buffers,
                // example: 'vertex: Lux.attribute_buffer(...)'
                result[k] = Shade(v);
                result.attributes[k] = result[k];
                n_elements = v.numItems;
            } else if (lux_typeOf(v) === "array") { // ... or a list of per-vertex things
                var buffer;
                // These things can be shade vecs
                if (lux_typeOf(v[0]) !== "array" && v[0]._lux_expression) {
                    // example: 'color: [Shade.color('white'), Shade.color('blue'), ...]
                    // assume it's a list of shade vecs, assume they all have the same dimension
                    // FIXME: check this
                    var dimension = v[0].type.vec_dimension();
                    var new_v = [];
                    _.each(v, push_into(new_v, dimension));
                    buffer = Lux.attribute_buffer({
                        vertex_array: new_v, 
                        item_size: dimension
                    });
                    result[k] = Shade(buffer);
                    result.attributes[k] = result[k];
                    n_elements = buffer.numItems;
                } else {
                    // Or they can be a single list of plain numbers, in which case we're passed 
                    // a pair, the first element being the list, the second 
                    // being the per-element size
                    // example: 'color: [[1,0,0, 0,1,0, 0,0,1], 3]'
                    buffer = Lux.attribute_buffer({
                        vertex_array: v[0], 
                        item_size: v[1]
                    });
                    result[k] = Shade(buffer);
                    result.attributes[k] = result[k];
                    n_elements = buffer.numItems;
                }
            } else {
                // if it's not any of the above things, then it's either a single shade expression
                // or a function which returns one. In any case, we just assign it to the key
                // and leave the user to fend for his poor self.
                result[k] = v;
            }
        },
        attributes: {}
    };

    for (var k in input) {
        var v = input[k];
        result.add(k, v);
    }
    if (!("elements" in result)) {
        // populate automatically using some sensible guess inferred from the attributes above
        if (_.isUndefined(n_elements)) {
            throw new Error("could not figure out how many elements are in this model; "
                + "consider passing an 'elements' field");
        } else {
            result.elements = n_elements;
        }
    }
    if (!("type" in result)) {
        result.add("type", "triangles");
    }
    result._ctx = Lux._globals.ctx;
    return result;
};
(function() {

var rb;

Lux.Picker = {
    draw_pick_scene: function(callback) {
        var ctx = Lux._globals.ctx;
        if (!rb) {
            rb = Lux.render_buffer({
                width: ctx.viewportWidth,
                height: ctx.viewportHeight,
                mag_filter: ctx.NEAREST,
                min_filter: ctx.NEAREST
            });
        }

        callback = callback || function() { Lux._globals.ctx._lux_globals.scene.draw(); };
        var old_scene_render_mode = ctx._lux_globals.batch_render_mode;
        ctx._lux_globals.batch_render_mode = 1;
        try {
            rb.with_bound_buffer(callback);
        } finally {
            ctx._lux_globals.batch_render_mode = old_scene_render_mode;
        }
    },
    pick: function(x, y) {
        var ctx = Lux._globals.ctx;
        var buf = new ArrayBuffer(4);
        var result_bytes = new Uint8Array(4);
        rb.with_bound_buffer(function() {
            ctx.readPixels(x, y, 1, 1, ctx.RGBA, ctx.UNSIGNED_BYTE, 
                           result_bytes);
        });
        var result_words = new Uint32Array(result_bytes.buffer);
        return result_words[0];
    }
};

})();
Lux.profile = function(name, seconds, onstart, onend) {
    if (onstart) onstart();
    console.profile(name);
    setTimeout(function() {
        console.profileEnd();
        if (onend) onend();
    }, seconds * 1000);
};
Lux.program = function(vs_src, fs_src)
{
    var ctx = Lux._globals.ctx;
    function getShader(shader_type, str)
    {
        var shader = ctx.createShader(shader_type);
        ctx.shaderSource(shader, str);
        ctx.compileShader(shader);
        if (!ctx.getShaderParameter(shader, ctx.COMPILE_STATUS)) {
            alert(ctx.getShaderInfoLog(shader));
            console.log("Error message: ");
            console.log(ctx.getShaderInfoLog(shader));
            console.log("Failing shader: ");
            console.log(str);
            throw new Error("failed compilation");
        }
        return shader;
    }

    var vertex_shader = getShader(ctx.VERTEX_SHADER, vs_src), 
        fragment_shader = getShader(ctx.FRAGMENT_SHADER, fs_src);

    var shaderProgram = ctx.createProgram();
    ctx.attachShader(shaderProgram, vertex_shader);
    ctx.attachShader(shaderProgram, fragment_shader);
    ctx.linkProgram(shaderProgram);
    
    if (!ctx.getProgramParameter(shaderProgram, ctx.LINK_STATUS)) {
        alert("Could not link program");
        console.log("Error message: ");
        console.log(ctx.getProgramInfoLog(shaderProgram));
        console.log("Failing shader pair:");
        console.log("Vertex shader");
        console.log(vs_src);
        console.log("Fragment shader");
        console.log(fs_src);
        throw new Error("failed link");
    }

    var active_parameters = ctx.getProgramParameter(shaderProgram, ctx.ACTIVE_UNIFORMS);
    var array_name_regexp = /.*\[0\]/;
    var info;
    for (var i=0; i<active_parameters; ++i) {
        info = ctx.getActiveUniform(shaderProgram, i);
        if (array_name_regexp.test(info.name)) {
            var array_name = info.name.substr(0, info.name.length-3);
            shaderProgram[array_name] = ctx.getUniformLocation(shaderProgram, array_name);
        } else {
            shaderProgram[info.name] = ctx.getUniformLocation(shaderProgram, info.name);
        }
    }
    var active_attributes = ctx.getProgramParameter(shaderProgram, ctx.ACTIVE_ATTRIBUTES);
    for (i=0; i<active_attributes; ++i) {
        info = ctx.getActiveAttrib(shaderProgram, i);
        shaderProgram[info.name] = ctx.getAttribLocation(shaderProgram, info.name);
    }
    return shaderProgram;    
};
Lux.render_buffer = function(opts)
{
    opts = _.defaults(opts || {}, {
        context: Lux._globals.ctx,
        width: 512,
        height: 512,
        mag_filter: Lux.texture.linear,
        min_filter: Lux.texture.linear,
        mipmaps: false,
        max_anisotropy: 1,
        wrap_s: Lux.texture.clamp_to_edge,
        wrap_t: Lux.texture.clamp_to_edge,
        clearColor: [0,0,0,1],
        clearDepth: 1.0
    });
    var ctx = opts.context;
    var frame_buffer = ctx.createFramebuffer();

    // Weird:
    // http://www.khronos.org/registry/gles/specs/2.0/es_full_spec_2.0.25.pdf
    // Page 118
    // 
    // Seems unenforced in my implementations of WebGL, even though 
    // the WebGL spec defers to GLSL ES spec.
    // 
    // if (opts.width != opts.height)
    //     throw new Error("renderbuffers must be square (blame GLSL ES!)");

    var rttTexture = Lux.texture(opts);

    frame_buffer.init = function(width, height) {
        Lux.set_context(ctx);
        this.width  = opts.width;
        this.height = opts.height;
        ctx.bindFramebuffer(ctx.FRAMEBUFFER, this);
        var renderbuffer = ctx.createRenderbuffer();
        ctx.bindRenderbuffer(ctx.RENDERBUFFER, renderbuffer);
        ctx.renderbufferStorage(ctx.RENDERBUFFER, ctx.DEPTH_COMPONENT16, this.width, this.height);

        ctx.framebufferTexture2D(ctx.FRAMEBUFFER, ctx.COLOR_ATTACHMENT0, ctx.TEXTURE_2D, rttTexture, 0);
        ctx.framebufferRenderbuffer(ctx.FRAMEBUFFER, ctx.DEPTH_ATTACHMENT, ctx.RENDERBUFFER, renderbuffer);
        var status = ctx.checkFramebufferStatus(ctx.FRAMEBUFFER);
        try {
            switch (status) {
            case ctx.FRAMEBUFFER_COMPLETE:
                break;
            case ctx.FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
                throw new Error("incomplete framebuffer: FRAMEBUFFER_INCOMPLETE_ATTACHMENT");
            case ctx.FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
                throw new Error("incomplete framebuffer: FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT");
            case ctx.FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
                throw new Error("incomplete framebuffer: FRAMEBUFFER_INCOMPLETE_DIMENSIONS");
            case ctx.FRAMEBUFFER_UNSUPPORTED:
                throw new Error("incomplete framebuffer: FRAMEBUFFER_UNSUPPORTED");
            default:
                throw new Error("incomplete framebuffer: " + status);
            }
        } finally {
            ctx.bindTexture(ctx.TEXTURE_2D, null);
            ctx.bindRenderbuffer(ctx.RENDERBUFFER, null);
            ctx.bindFramebuffer(ctx.FRAMEBUFFER, null);
        }
    };

    frame_buffer.init(opts.width, opts.height);
    frame_buffer._shade_type = 'render_buffer';
    frame_buffer.texture = rttTexture;
    frame_buffer.resize = function(width, height) {
        opts.width = width;
        opts.height = height;
        this.texture.init(opts);
        this.init(width, height);
    };
    frame_buffer.with_bound_buffer = function(what) {
        var v = ctx.getParameter(ctx.VIEWPORT);
        try {
            ctx.bindFramebuffer(ctx.FRAMEBUFFER, this);
            ctx.viewport(0, 0, this.width, this.height);
            return what();
        } finally {
            ctx.viewport(v[0], v[1], v[2], v[3]);
            ctx.bindFramebuffer(ctx.FRAMEBUFFER, null);
        }
    };
    frame_buffer.screen_actor = function(opts) {
        opts = _.defaults(opts, {
            mode: Lux.DrawingMode.standard
        });
        var with_texel_at_uv = opts.texel_function;
        var mode = opts.mode;
        var that = this;
        var sq = Lux.Models.square();
        mode = mode || Lux.DrawingMode.standard;
        return Lux.actor({
            model: sq,
            appearance: {
                screen_position: sq.vertex.mul(2).sub(1),
                color: with_texel_at_uv(function(offset) {
                    var texcoord = sq.tex_coord;
                    if (arguments.length > 0)
                        texcoord = texcoord.add(offset);
                    return Shade.texture2D(that.texture, texcoord);
                }),
                mode: mode
            },
            bake: opts.bake
        });
    };
    
    var old_v;
    frame_buffer.scene = Lux.default_scene({
        clearColor: opts.clearColor,
        clearDepth: opts.clearDepth,
        context: ctx,
        pre_draw: function() {
            old_v = ctx.getParameter(ctx.VIEWPORT);
            ctx.bindFramebuffer(ctx.FRAMEBUFFER, frame_buffer);
            ctx.viewport(0, 0, frame_buffer.width, frame_buffer.height);
        },
        post_draw: function() {
            ctx.viewport(old_v[0], old_v[1], old_v[2], old_v[3]);
            ctx.bindFramebuffer(ctx.FRAMEBUFFER, null);
        }
    });

    return frame_buffer;
};
Lux.set_context = function(the_ctx)
{
    Lux._globals.ctx = the_ctx;
    // Shade.set_context(the_ctx);
};
/*
 * Lux.on_context returns a wrapped callback that guarantees that the passed
 * callback will be invoked with the given current context. 
 * 
 * This is primarily used to safeguard pieces of code that need to work under
 * multiple active WebGL contexts.
 */
Lux.on_context = function(the_ctx, f)
{
    return function() {
        Lux.set_context(the_ctx);
        f.apply(this, arguments);
    };
};
//////////////////////////////////////////////////////////////////////////////
// load texture from DOM element or URL. 
// BEWARE SAME-DOMAIN POLICY!

Lux.texture = function(opts)
{
    var ctx = Lux._globals.ctx;
    var texture = ctx.createTexture();

    texture._shade_type = 'texture';
    // Each texture has to be associated with a particular context, so we
    // store that in ._ctx
    // FIXME: This must be true for other WebGL resources as well. Are we checking them?
    texture._ctx = ctx;

    texture.init = Lux.on_context(ctx, function(opts) {
        var ctx = Lux._globals.ctx;
        var has_mipmaps = _.isUndefined(opts.mipmaps) || opts.mipmaps;
        opts = _.defaults(opts, {
            onload: function() {},
            max_anisotropy: opts.mipmaps ? 2 : 1,
            mipmaps: true,
            mag_filter: Lux.texture.linear,
            min_filter: opts.mipmaps ? Lux.texture.linear_mipmap_linear : Lux.texture.linear,
            wrap_s: Lux.texture.clamp_to_edge,
            wrap_t: Lux.texture.clamp_to_edge,
            format: Lux.texture.rgba,
            type: Lux.texture.unsigned_byte
        });
        this.width = opts.width;
        this.height = opts.height;

        this.ready = false;
        var that = this;

        /*
         * Texture.load:
         * 
         *   Replaces a rectangle of a Lux texture with a given image.
         * 
         *   This is useful to store a large set of rectangular images into a single texture, for example.
         * 
         *   Example usage:
         * 
         *   * Load an image from a URL:
         * 
         *     texture.load({
         *       src: "http://www.example.com/image.png"
         *     })
         * 
         *   * Invoke a callback when image is successfully loaded:
         * 
         *     texture.load({
         *       src: "http://www.example.com/image.png",
         *       onload: function(image) { 
         *         alert("image has now loaded into texture!");
         *       }
         *     })
         * 
         *     The parameter passed to the callback is the image, canvas 
         *     or buffer loaded into the texture, and in
         *     the callback, 'this' points to the texture. In other words,
         *     the callback is called with "onload.call(texture, image)"
         *        
         *   * Specify an offset:
         * 
         *     texture.load({
         *       src: "http://www.example.com/image.png",
         *       x_offset: 64,
         *       y_offset: 32
         *     })
         * 
         *   * Load an image from an existing element in the DOM:
         * 
         *     texture.load({
         *       img: document.getElementById("image-element")
         *     });
         *
         *     texture.load({
         *       canvas: document.getElementById("canvas-element")
         *     });
         * 
         *   * Load an image from a TypedArray buffer (currently only supports 8-bit RGBA or 32-bit float RGBA):
         * 
         *     Lux.load({
         *       width: 128,
         *       height: 128,
         *       buffer: new Uint8Array(128 * 128 * 4)
         *     });
         */
        this.load = function(opts) {
            opts = _.defaults(opts, {
                onload: function() {},
                x_offset: 0,
                y_offset: 0,
                transform_image: function(i) { return i; }
            });

            var texture = this;
            var onload = opts.onload;
            var x_offset = opts.x_offset;
            var y_offset = opts.y_offset;

            function image_handler(image) {
                image = opts.transform_image(image);
                var ctx = texture._ctx;
                Lux.set_context(texture._ctx);
                ctx.bindTexture(ctx.TEXTURE_2D, texture);
                ctx.pixelStorei(ctx.UNPACK_FLIP_Y_WEBGL, true);
                if (_.isUndefined(that.width)) {
                    that.width = image.width;
                    that.height = image.height;
                    ctx.texImage2D(ctx.TEXTURE_2D, 0, opts.format,
                                   that.width, that.height,
                                   0, opts.format, opts.type, null);
                }
                ctx.texSubImage2D(ctx.TEXTURE_2D, 0, x_offset, y_offset,
                                  ctx.RGBA, ctx.UNSIGNED_BYTE, image);
                if (opts.mipmaps)
                    ctx.generateMipmap(ctx.TEXTURE_2D);
                Lux.unload_batch();
                that.ready = true;
                onload.call(texture, image);
            }

            function buffer_handler()
            {
                var ctx = texture._ctx;
                Lux.set_context(texture._ctx);
                ctx.bindTexture(ctx.TEXTURE_2D, texture);
                if (_.isUndefined(opts.buffer)) {
                    if (x_offset !== 0 || y_offset !== 0) {
                        throw new Error("texture.load cannot be called with nonzero offsets and no data");
                    }
                    ctx.texImage2D(ctx.TEXTURE_2D, 0, opts.format,
                                   that.width, that.height,
                                   0, opts.format, opts.type, null);
                } else {
                    var type;
                    var ctor = opts.buffer.constructor.name;
                    var map = {
                        "Uint8Array": ctx.UNSIGNED_BYTE,
                        "Float32Array": ctx.FLOAT
                    };
                    if (_.isUndefined(map[ctor])) {
                        throw new Error("opts.buffer must be either Uint8Array or Float32Array");
                    }
                    ctx.texSubImage2D(ctx.TEXTURE_2D, 0, x_offset, y_offset, 
                                      opts.width, opts.height,
                                      ctx.RGBA, map[ctor], opts.buffer);
                }
                if (opts.mipmaps)
                    ctx.generateMipmap(ctx.TEXTURE_2D);
                that.ready = true;
                Lux.unload_batch();
                onload.call(texture, opts.buffer);
            }

            if (opts.src) {
                var image = new Image();
                image.onload = function() {
                    image_handler(image);
                };
                // CORS support
                if (opts.crossOrigin)
                    image.crossOrigin = opts.crossOrigin;
                image.src = opts.src;
            } else if (opts.canvas) {
                image_handler(opts.canvas);
            } else if (opts.img) {
                if (opts.img.isComplete) {
                    image_handler(opts.img);
                } else {
                    var old_onload = texture.image.onload || function() {};
                    opts.img.onload = function() {
                        image_handler(opts.img);
                        old_onload();
                    };
                }
            } else {
                buffer_handler();
            }
        };
        
        Lux.set_context(ctx);
        ctx.bindTexture(ctx.TEXTURE_2D, that);
        ctx.pixelStorei(ctx.UNPACK_PREMULTIPLY_ALPHA_WEBGL, false);
        ctx.texImage2D(ctx.TEXTURE_2D, 0, opts.format,
                       that.width, that.height,
                       0, opts.format, opts.type, null);
        ctx.texParameteri(ctx.TEXTURE_2D, ctx.TEXTURE_MAG_FILTER, opts.mag_filter);
        ctx.texParameteri(ctx.TEXTURE_2D, ctx.TEXTURE_MIN_FILTER, opts.min_filter);
        ctx.texParameteri(ctx.TEXTURE_2D, ctx.TEXTURE_WRAP_S, opts.wrap_s);
        ctx.texParameteri(ctx.TEXTURE_2D, ctx.TEXTURE_WRAP_T, opts.wrap_t);
        if (ctx._lux_globals.webgl_extensions.EXT_texture_filter_anisotropic &&
            opts.max_anisotropy > 1 && opts.mipmaps) {
            ctx.texParameterf(ctx.TEXTURE_2D, ctx.TEXTURE_MAX_ANISOTROPY_EXT, opts.max_anisotropy);
        }

        delete this.buffer;
        delete this.image;

        this.load(opts);
    });

    texture.init(opts);

    return texture;
};

//////////////////////////////////////////////////////////////////////////////
// texture-related enums go here

// mag_filter
Lux.texture.nearest                = 0x2600;
Lux.texture.linear                 = 0x2601;

// min_filter 
Lux.texture.nearest_mipmap_nearest = 0x2700;
Lux.texture.linear_mipmap_nearest  = 0x2701;
Lux.texture.nearest_mipmap_linear  = 0x2702;
Lux.texture.linear_mipmap_linear   = 0x2703;

// wrap_s and wrap_t
Lux.texture.repeat                 = 0x2901;
Lux.texture.clamp_to_edge          = 0x812F;
Lux.texture.mirrored_repeat        = 0x8370;

// format
Lux.texture.depth_component        = 0x1902;
Lux.texture.alpha                  = 0x1906;
Lux.texture.rgb                    = 0x1907;
Lux.texture.rgba                   = 0x1908;
Lux.texture.luminance              = 0x1909;
Lux.texture.luminance_alpha        = 0x190A;

// type
Lux.texture.unsigned_byte          = 0x1401;
Lux.texture.unsigned_short_4_4_4_4 = 0x8033;
Lux.texture.unsigned_short_5_5_5_1 = 0x8034;
Lux.texture.unsigned_short_5_6_5   = 0x8363;
Lux.texture["float"]               = 0x1406;
(function() {

var rb;
var depth_value;
var clear_batch;
    
Lux.Unprojector = {
    draw_unproject_scene: function(callback) {
        var ctx = Lux._globals.ctx;
        if (!rb) {
            rb = Lux.render_buffer({
                width: ctx.viewportWidth,
                height: ctx.viewportHeight,
                TEXTURE_MAG_FILTER: ctx.NEAREST,
                TEXTURE_MIN_FILTER: ctx.NEAREST
            });
        }
        // In addition to clearing the depth buffer, we need to fill
        // the color buffer with
        // the right depth value. We do it via the batch below.

        if (!clear_batch) {
            var xy = Shade(Lux.attribute_buffer({
                vertex_array: [-1, -1,   1, -1,   -1,  1,   1,  1], 
                item_size: 2}));
            var model = Lux.model({
                type: "triangle_strip",
                elements: 4,
                vertex: xy
            });
            depth_value = Shade.parameter("float");
            clear_batch = Lux.bake(model, {
                position: Shade.vec(xy, depth_value),
                color: Shade.vec(1,1,1,1)
            });
        }

        callback = callback || ctx._lux_globals.display_callback;
        var old_scene_render_mode = ctx._lux_globals.batch_render_mode;
        ctx._lux_globals.batch_render_mode = 2;
        rb.with_bound_buffer(function() {
            var old_clear_color = ctx.getParameter(ctx.COLOR_CLEAR_VALUE);
            var old_clear_depth = ctx.getParameter(ctx.DEPTH_CLEAR_VALUE);
            ctx.clearColor(old_clear_depth,
                           old_clear_depth / (1 << 8),
                           old_clear_depth / (1 << 16),
                           old_clear_depth / (1 << 24));
            ctx.clear(ctx.DEPTH_BUFFER_BIT | ctx.COLOR_BUFFER_BIT);
            try {
                callback();
            } finally {
                ctx.clearColor(old_clear_color[0],
                               old_clear_color[1],
                               old_clear_color[2],
                               old_clear_color[3]);
                ctx._lux_globals.batch_render_mode = old_scene_render_mode;
            }
        });
    },

    unproject: function(x, y) {
        var ctx = Lux._globals.ctx;
        var buf = new ArrayBuffer(4);
        var result_bytes = new Uint8Array(4);
        ctx.readPixels(x, y, 1, 1, ctx.RGBA, ctx.UNSIGNED_BYTE, 
                       result_bytes);
        rb.with_bound_buffer(function() {
            ctx.readPixels(x, y, 1, 1, ctx.RGBA, ctx.UNSIGNED_BYTE, 
                           result_bytes);
        });
        return result_bytes[0] / 256 + 
            result_bytes[1] / (1 << 16) + 
            result_bytes[2] / (1 << 24);
        // +  result_bytes[3] / (1 << 32);
    }
};

})();
Lux.Net = {};

(function() {

var handle_many = function(url, handler, self_call) {
    var obj = {};
    var done = _.after(url.length, handler);
    function piecemeal_handler(result, internal_url) {
        obj[internal_url] = result;
        done(obj);
    }
    _.each(url, function(internal_url) {
        self_call(internal_url, piecemeal_handler);
    });
};


/*
 * Lux.Net.ajax issues AJAX requests.
 * 
 * It takes as parameters
 * 
 *  url (string or list of strings): urls to fetch
 * 
 *  handler (function(buffer or dictionary of (url: buffer))): a callback
 *  which gets invoked when all requests finish. If a single URL was passed,
 *  the callback is called with the single response eturned. If a list of URLs
 *  were passed, then an object is returned, mapping the URLs as passed to
 *  the responses.
 *  
 * FIXME Lux.Net.ajax has no error handling.
 */

Lux.Net.ajax = function(url, handler)
{
    var current_context = Lux._globals.ctx;

    if (lux_typeOf(url) === "array")
        return handle_many(url, handler, Lux.Net.ajax);

    var xhr = new XMLHttpRequest;

    xhr.open("GET", url, true);

    var ready = false;
    xhr.onreadystatechange = function() {
        if (xhr.readyState === 4 && xhr.status === 200 && !ready) {
            Lux.set_context(current_context);
            handler(xhr.response, url);
            ready = true;
        }
    };
    xhr.send(null);
    return undefined;
};
/*
 * Lux.Net.json issues JSON AJAX requests.
 * 
 * It takes as parameters
 * 
 *  url (string or list of strings): urls to fetch
 * 
 *  handler (function(buffer or dictionary of (url: buffer))): a callback
 *  which gets invoked when all requests finish. If a single URL was passed,
 *  the callback is called with the single JSON object returned. If a list of URLs
 *  were passed, then an object is returned, mapping the URLs as passed to
 *  the responses.
 *  
 * FIXME Lux.Net.json has no error handling.
 */

Lux.Net.json = function(url, handler)
{
    if (lux_typeOf(url) === "array")
        return handle_many(url, handler, Lux.Net.json);

    var xhr = new XMLHttpRequest;

    xhr.open("GET", url, true);

    var ready = false;
    xhr.onreadystatechange = function() {
        if (xhr.readyState === 4 && xhr.status === 200 && !ready) {
            handler(JSON.parse(xhr.response), url);
            ready = true;
        }
    };
    xhr.send(null);
};
/*
 * Lux.Net.binary issues binary AJAX requests, which can be
 * used to load data into Lux more efficiently than through the
 * regular text or JSON AJAX interfaces. It returns ArrayBuffer objects.
 * 
 * It takes as parameters
 * 
 *  url (string or list of strings): urls to fetch
 * 
 *  handler (function(ArrayBuffer or dictionary of (url: ArrayBuffer))): a callback
 *  which gets invoked when all requests finish. If a single URL was passed,
 *  the callback is called with the single buffer returned. If a list of URLs
 *  were passed, then an object is returned, mapping the URLs as passed to
 *  the buffers.
 *  
 * FIXME Lux.Net.binary has no error handling.
 */

// based on http://calumnymmo.wordpress.com/2010/12/22/so-i-decided-to-wait/
// Update 2013-04-24; Firefox now seems to behave in the same way as Chrome.

Lux.Net.binary = function(url, handler)
{
    var current_context = Lux._globals.ctx;

    if (lux_typeOf(url) === "array")
        return handle_many(url, handler, Lux.Net.binary);

    var xhr = new window.XMLHttpRequest();
    var ready = false;
    xhr.onreadystatechange = function() {
        Lux.set_context(current_context);
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
})();
// drawing mode objects can be part of the parameters passed to 
// Lux.bake, in order for the batch to automatically set the capabilities.
// This lets us specify blending, depth-testing, etc. at bake time.

/* FIXME This is double dispatch done wrong. See lux.org for details.
 */

Lux.DrawingMode = {};
Lux.DrawingMode.additive = {
    set_draw_caps: function()
    {
        var ctx = Lux._globals.ctx;
        ctx.enable(ctx.BLEND);
        ctx.blendFunc(ctx.SRC_ALPHA, ctx.ONE);
        ctx.enable(ctx.DEPTH_TEST);
        ctx.depthFunc(ctx.LESS);
        ctx.depthMask(false);
    },
    set_pick_caps: function()
    {
        var ctx = Lux._globals.ctx;
        ctx.enable(ctx.DEPTH_TEST);
        ctx.depthFunc(ctx.LESS);
        ctx.depthMask(false);
    },
    set_unproject_caps: function()
    {
        var ctx = Lux._globals.ctx;
        ctx.enable(ctx.DEPTH_TEST);
        ctx.depthFunc(ctx.LESS);
        ctx.depthMask(false);
    }
};
// over is the standard porter-duff over operator

// NB: since over is associative but not commutative, we need
// back-to-front rendering for correct results,
// and then the depth buffer is not necessary. 
// 
// In the case of incorrect behavior (that is, when contents are not
// rendered back-to-front), it is not clear which of the two incorrect 
// behaviors is preferable:
// 
// 1. that depth buffer writing be enabled, and some things which should
// be rendered "behind" alpha-blended simply disappear (this gets
// worse the more transparent objects get)
//
// 2. that depth buffer writing be disabled, and some things which would be
// entirely occluded by others simply appear (this gets worse the more opaque
// objects get)
//
// These two behaviors correspond respectively to 
// Lux.DrawingMode.over_with_depth and Lux.DrawingMode.over

Lux.DrawingMode.over = {
    set_draw_caps: function()
    {
        var ctx = Lux._globals.ctx;
        ctx.enable(ctx.BLEND);
        ctx.blendFuncSeparate(ctx.SRC_ALPHA, ctx.ONE_MINUS_SRC_ALPHA, 
                              ctx.ONE, ctx.ONE_MINUS_SRC_ALPHA);
        ctx.enable(ctx.DEPTH_TEST);
        ctx.depthFunc(ctx.LESS);
        ctx.depthMask(false);
    },
    set_pick_caps: function()
    {
        var ctx = Lux._globals.ctx;
        ctx.enable(ctx.DEPTH_TEST);
        ctx.depthFunc(ctx.LESS);
        ctx.depthMask(false);
    },
    set_unproject_caps: function()
    {
        var ctx = Lux._globals.ctx;
        ctx.enable(ctx.DEPTH_TEST);
        ctx.depthFunc(ctx.LESS);
        ctx.depthMask(false);
    }
};

Lux.DrawingMode.over_with_depth = {
    set_draw_caps: function()
    {
        var ctx = Lux._globals.ctx;
        ctx.enable(ctx.BLEND);
        ctx.blendFuncSeparate(ctx.SRC_ALPHA, ctx.ONE_MINUS_SRC_ALPHA, 
                              ctx.ONE, ctx.ONE_MINUS_SRC_ALPHA);
        ctx.enable(ctx.DEPTH_TEST);
        ctx.depthFunc(ctx.LEQUAL);
    },
    set_pick_caps: function()
    {
        var ctx = Lux._globals.ctx;
        ctx.enable(ctx.DEPTH_TEST);
        ctx.depthFunc(ctx.LEQUAL);
    },
    set_unproject_caps: function()
    {
        var ctx = Lux._globals.ctx;
        ctx.enable(ctx.DEPTH_TEST);
        ctx.depthFunc(ctx.LEQUAL);
    }
};

Lux.DrawingMode.over_no_depth = {
    set_draw_caps: function()
    {
        var ctx = Lux._globals.ctx;
        ctx.enable(ctx.BLEND);
        ctx.blendFuncSeparate(ctx.SRC_ALPHA, ctx.ONE_MINUS_SRC_ALPHA, 
                              ctx.ONE, ctx.ONE_MINUS_SRC_ALPHA);
        ctx.disable(ctx.DEPTH_TEST);
        ctx.depthMask(false);
    },
    set_pick_caps: function()
    {
        var ctx = Lux._globals.ctx;
        ctx.disable(ctx.DEPTH_TEST);
        ctx.depthMask(false);
    },
    set_unproject_caps: function()
    {
        var ctx = Lux._globals.ctx;
        ctx.disable(ctx.DEPTH_TEST);
        ctx.depthMask(false);
    }
};
Lux.DrawingMode.standard = {
    set_draw_caps: function()
    {
        var ctx = Lux._globals.ctx;
        ctx.enable(ctx.DEPTH_TEST);
        ctx.depthFunc(ctx.LESS);
        ctx.disable(ctx.BLEND);
    },
    set_pick_caps: function()
    { 
        var ctx = Lux._globals.ctx;
        ctx.enable(ctx.DEPTH_TEST);
        ctx.depthFunc(ctx.LESS);
        ctx.disable(ctx.BLEND);
   },
    set_unproject_caps: function()
    {
        var ctx = Lux._globals.ctx;
        ctx.enable(ctx.DEPTH_TEST);
        ctx.depthFunc(ctx.LESS);
        ctx.disable(ctx.BLEND);
    }
};
/*
 * Lux.DrawingMode.pass is used whenever depth testing needs to be off;
 * 
 * Lux.DrawingMode.pass disables *writing* to the depth test as well
 * 
 */

Lux.DrawingMode.pass = {
    set_draw_caps: function()
    {
        var ctx = Lux._globals.ctx;
        ctx.disable(ctx.DEPTH_TEST);
        ctx.depthMask(false);
        ctx.disable(ctx.BLEND);
    },
    set_pick_caps: function()
    { 
        var ctx = Lux._globals.ctx;
        ctx.disable(ctx.DEPTH_TEST);
        ctx.depthMask(false);
        ctx.disable(ctx.BLEND);
    },
    set_unproject_caps: function()
    {
        var ctx = Lux._globals.ctx;
        ctx.disable(ctx.DEPTH_TEST);
        ctx.depthMask(false);
        ctx.disable(ctx.BLEND);
    }
};
Lux.Data = {};
Lux.Data.table = function(obj) {
    obj = _.defaults(obj || {}, {
        number_columns: []
    });
    if (_.isUndefined(obj.data)) throw new Error("data is a required field");
    if (_.isUndefined(obj.data)) throw new Error("columns is a required field");
    function table() {
    };
    table.prototype = {
        is_numeric_row_complete: function(row) {
            for (var i=0; i<this.number_columns.length; ++i) {
                var col = this.columns[i];
                var val = row[col];
                if (typeof val !== "number")
                    return false;
            }
            return this.number_columns.length > 0;
        }
    };
    var result = new table();
    for (var key in obj) {
        result[key] = obj[key];
    }
    return result;
};
Lux.Data.texture_table = function(table)
{
    var elements = [];
    for (var row_ix = 0; row_ix < table.data.length; ++row_ix) {
        var row = table.data[row_ix];
        if (!table.is_numeric_row_complete(row))
            continue;
        for (var col_ix = 0; col_ix < table.number_columns.length; ++col_ix) {
            var col_name = table.columns[table.number_columns[col_ix]];
            var val = row[col_name];
            if (typeof val !== "number")
                throw new Error("texture_table requires numeric values");
            elements.push(val);
        }
    }

    var table_ncols = table.number_columns.length;
    // can't be table.data.length because not all rows are valid.
    var table_nrows = elements.length / table.number_columns.length;
    var texture_width = 1;

    return Lux.Data.texture_array({
        n_rows: table_nrows,
        n_cols: table_ncols,
        elements: elements
    });
};
/*
   texture array takes an object with fields:

     n_cols (integer): number of columns in the 2D array of data
     n_rows (integer): number of rows in the 2D array of data
     elements (array, Float32Array): list of elements in the array

   and returns an object with four fields:

   n_cols (integer): number of columns in the data

   n_rows (integer): number of rows in the data

   at (function(Shade(int), Shade(int)) -> Shade(float)): returns the
   value stored at given row and column

   index (function(Shade(int), Shade(int)) -> Shade(vec3)): returns
   the index of the value stored at given row and column. This is a
   three dimensional vector.  The first two coordinates store the
   texture coordinate, and the fourth coordinate stores the
   channel. This is necessary to take advantage of RGBA float
   textures, which have the widest support on WebGL-capable hardware.

   For example, luminance float textures appear to clamp to [0,1], at
   least on Chrome 15 on Linux.

 */

Lux.Data.texture_array = function(opts)
{
    var ctx = Lux._globals.ctx;
    var elements = opts.elements;
    var n_cols = opts.n_cols;
    var n_rows = opts.n_rows;

    var texture_width = 1;
    while (4 * texture_width * texture_width < elements.length) {
        texture_width = texture_width * 2;
    }
    var texture_height = Math.ceil(elements.length / (4 * texture_width));

    var new_elements;
    if (texture_width * texture_height === elements.length) {
        // no chance this will ever happen in practice, but hey, 
        // a man can dream
        if (lux_typeOf(elements) === "array") {
            new_elements = new Float32Array(elements);
        } else
            new_elements = elements;
    } else {
        new_elements = new Float32Array(texture_width * texture_height * 4);
        for (var i=0; i<elements.length; ++i)
            new_elements[i] = elements[i];
    }

    var texture = Lux.texture({
        width: texture_width,
        height: texture_height,
        buffer: new_elements,
        type: ctx.FLOAT,
        format: ctx.RGBA,
        min_filter: ctx.NEAREST,
        mag_filter: ctx.NEAREST
    });

    var index = Shade(function(row, col) {
        var linear_index    = row.mul(n_cols).add(col);
        var in_texel_offset = linear_index.mod(4);
        var texel_index     = linear_index.div(4).floor();
        var x               = texel_index.mod(texture_width);
        var y               = texel_index.div(texture_width).floor();
        var result          = Shade.vec(x, y, in_texel_offset);
        return result;
    });
    var at = Shade(function(row, col) {
        // returns Shade expression with value at row, col
        var ix = index(row, col);
        var uv = ix.swizzle("xy")
            .add(Shade.vec(0.5, 0.5))
            .div(Shade.vec(texture_width, texture_height))
            ;
        return Shade.texture2D(texture, uv).at(ix.z());
    });

    return {
        n_rows: n_rows,
        n_cols: n_cols,
        at: at,
        index: index
    };
};
Lux.Data.array_1d = function(array)
{
    var ctx = Lux._globals.ctx;

    var elements = array;
    var texture_width = 1;
    while (4 * texture_width * texture_width < elements.length) {
        texture_width = texture_width * 2;
    }
    var texture_height = Math.ceil(elements.length / (4 * texture_width));
    var new_elements;
    if (texture_width * texture_height === elements.length) {
        if (lux_typeOf(elements) === "array") {
            new_elements = new Float32Array(elements);
        } else
            new_elements = elements;
    } else {
        new_elements = new Float32Array(texture_width * texture_height * 4);
        for (var i=0; i<elements.length; ++i)
            new_elements[i] = elements[i];
    }

    var texture = Lux.texture({
        width: texture_width,
        height: texture_height,
        buffer: new_elements,
        type: ctx.FLOAT,
        format: ctx.RGBA,
        min_filter: ctx.NEAREST,
        min_filter: ctx.NEAREST
    });

    var index = Shade(function(linear_index) {
        var in_texel_offset = linear_index.mod(4);
        var texel_index = linear_index.div(4).floor();
        var x = texel_index.mod(texture_width);
        var y = texel_index.div(texture_width).floor();
        var result = Shade.vec(x, y, in_texel_offset);
        return result;
    });

    var at = Shade(function(linear_index) {
        var ix = index(linear_index);
        var uv = ix.swizzle("xy")
            .add(Shade.vec(0.5, 0.5))
            .div(Shade.vec(texture_width, texture_height))
            ;
        return Shade.texture2D(texture, uv).at(ix.z());
    });
    return {
        length: new_elements.length,
        at: at,
        index: index
    };
};
Lux.UI = {};
/*
 * Lux.UI.parameter_slider is a function to help create UI elements
 * that control Shade.parameter objects. 
 *
 * The result of calling Lux.UI.parameter_slider is a Shade.parameter,
 * either freshly created, or the one passed as input.
 *
 * Lux.UI.parameter_slider requires the "element" field in its options.
 * 
 * opts.element is the HTML element used by jquery-ui to create the slider. That
 *   object needs to have the correct CSS class assigned to it ahead of calling
 *   this function.
 * 
 * opts.parameter is the Shade.parameter object under control. if opts.parameter
 *   is undefined, Lux.UI.parameter_slider creates the Shade.parameter.
 * 
 * opts.change is a user-defined callback to the slider change event.
 * opts.slide is a user-defined callback to the slider slide event.
 * 
 *   Both event handlers are passed the HTML element, the parameter object, 
 *   and the new value, in that order.
 * 
 * opts.min is the minimum value allowed by the slider
 * opts.max is the maximum value allowed by the slider
 * opts.value is the starting value of the slider and parameter
 * opts.orientation is the slider's orientation, either "horizontal" or "vertical"
 *
 * Lux.UI.parameter_slider uses jquery-ui sliders, and so assumes
 * jquery-ui in addition to jquery.  If you know of a better
 * lightweight gui library, let me know as well.
 */

Lux.UI.parameter_slider = function(opts)
{
    opts = _.defaults(opts, {
        min: 0,
        max: 1,
        orientation: "horizontal",
        slide: function() {},
        change: function() {}
    });
    var element = opts.element;
    if (_.isUndefined(opts.element)) {
        throw new Error("parameter_slider requires an element option");
    }
    if (_.isUndefined(opts.parameter)) {
        opts.parameter = Shade.parameter("float", opts.min);
    }
    if (!_.isUndefined(opts.value)) {
        opts.parameter.set(opts.value);
    }
    var parameter  = opts.parameter,
        slider_min = 0, 
        slider_max = 1000;

    function to_slider(v) {
        return (v-opts.min) / (opts.max - opts.min) * 
            (slider_max - slider_min) + slider_min;
    }
    function to_parameter(v) {
        return (v-slider_min) / (slider_max - slider_min) *
            (opts.max - opts.min) + opts.min;
    }
    $(element).slider({
        min: slider_min,
        max: slider_max,
        value: to_slider(parameter.get()),
        orientation: opts.orientation,
        slide: function() {
            var v = to_parameter($(element).slider("value"));
            parameter.set(v);
            opts.slide(element, parameter, v);
            Lux.Scene.invalidate();
        },
        change: function() {
            var v = to_parameter($(element).slider("value"));
            parameter.set(v);
            opts.change(element, parameter, v);
            Lux.Scene.invalidate();
        }
    });
    return parameter;
};
Lux.UI.parameter_checkbox = function(opts)
{
    opts = _.defaults(opts, {
        toggle: function() {}
    });
    var element = opts.element;
    var parameter = opts.parameter;

    function on_click(event) {
        parameter.set(~~event.target.checked);
        console.log(parameter.get());
        opts.toggle(event);
        Lux.Scene.invalidate();
    }

    $(element).button().click(on_click);
};
/*
 * A Lux interactor is an object that exposes a list of events that
 * Lux.init uses to hook up to canvas event handlers.
 * 
 * Lux.UI.center_zoom_interactor provides event handlers for the
 * common interaction mode of zooming and panning. Its main visible variables
 * are center and zoom Shade.parameter objects, together with a Shade.camera
 * that computes the appropriate projection matrix.
 * 
 * usage examples:
 *   demos/beauty_of_roots
 * 
 */

Lux.UI.center_zoom_interactor = function(opts)
{
    opts = _.defaults(opts || {}, {
        mousemove: function() {},
        mouseup: function() {},
        mousedown: function() {},
        mousewheel: function() {},
        dblclick: function() {},
        center: vec.make([0,0]),
        zoom: 1,
        widest_zoom: 0.1,
        width: 100,
        height: 100
    });

    var height = opts.height;
    var width = opts.width;

    var aspect_ratio = Shade.parameter("float", width/height);
    var center = Shade.parameter("vec2", opts.center);
    var zoom = Shade.parameter("float", opts.zoom);
    var camera = Shade.Camera.ortho({
        left: opts.left,
        right: opts.right,
        top: opts.top,
        bottom: opts.bottom,
        center: center,
        zoom: zoom,
        aspect_ratio: aspect_ratio
    });

    var prev_mouse_pos, down_mouse_pos;
    var current_button = 0;

    function dblclick(event) {
        internal_move(result.width/2-event.offsetX, event.offsetY-result.height/2);
        zoom.set(zoom.get() * 2);
        internal_move(event.offsetX-result.width/2, result.height/2-event.offsetY);
        Lux.Scene.invalidate();
        opts.dblclick(event);
    }

    function mousedown(event) {
        if (_.isUndefined(event.buttons)) {
            // webkit
            current_button = event.which;
        } else {
            // firefox
            current_button = event.buttons;
        }

        prev_mouse_pos = [event.offsetX, event.offsetY];
        down_mouse_pos = [event.offsetX, event.offsetY];
        opts.mousedown(event);
    }

    function mouseup(event) {
        current_button = 0;
        opts.mouseup(event);
    }

    // c stores the compensation for the kahan compensated sum
    var c = vec.make([0, 0]);

    // f computes the change in the center position, relative to the
    // current camera parameters. Since camera is a Lux expression,
    // to get the javascript value we create a Shade function and
    // use js_evaluate.
    var f = Shade(function (delta_vec) {
        return result.camera.unproject(Shade.vec(0,0))
            .sub(result.camera.unproject(delta_vec));
    }).js_evaluate;

    var internal_move = function(dx, dy) {
        var ctx = Lux._globals.ctx;
        var v = vec.make([2*dx/ctx.parameters.width.get(), 
                          2*dy/ctx.parameters.height.get()]);
        var negdelta = f(v);
        // we use a kahan compensated sum here:
        // http://en.wikipedia.org/wiki/Kahan_summation_algorithm
        // to accumulate minute changes in the center that come from deep zooms.
        var y = vec.minus(negdelta, c);
        var t = vec.plus(center.get(), y);
        c = vec.minus(vec.minus(t, center.get()), y);
        center.set(t);
    };

    function mousemove(event) {
        if ((current_button & 1) && !event.shiftKey) {
            internal_move(event.offsetX - prev_mouse_pos[0], 
                        -(event.offsetY - prev_mouse_pos[1]));
            Lux.Scene.invalidate();
        } else if ((current_button & 1) && event.shiftKey) {
            internal_move(result.width/2-down_mouse_pos[0], down_mouse_pos[1]-result.height/2);
            var new_value = Math.max(opts.widest_zoom, zoom.get() * (1.0 + (event.offsetY - prev_mouse_pos[1]) / 240));
            zoom.set(new_value);
            internal_move(down_mouse_pos[0]-result.width/2, result.height/2-down_mouse_pos[1]);
            Lux.Scene.invalidate();
        }
        prev_mouse_pos = [ event.offsetX, event.offsetY ];
        opts.mousemove(event);
    }

    // FIXME mousewheel madness
    function mousewheel(event, delta, deltaX, deltaY) {
        internal_move(result.width/2-event.offsetX, event.offsetY-result.height/2);
	var new_value = Math.max(opts.widest_zoom, zoom.get() * (1.0 + deltaY/10));
        // var new_value = Math.max(opts.widest_zoom, zoom.get() * (1.0 + event.wheelDelta / 1200));
        zoom.set(new_value);
        internal_move(event.offsetX-result.width/2, result.height/2-event.offsetY);
        // opts.mousewheel(event);
        Lux.Scene.invalidate();
        return false;
    }

    function resize(w, h) {
        result.resize(w, h);
    }

    // implement transform stack inverse requirements
    var transform = function(appearance) {
        if (_.isUndefined(appearance.position))
            return appearance;
        var new_appearance = _.clone(appearance);
        new_appearance.position = result.project(new_appearance.position);
        return new_appearance;
    };
    transform.inverse = function(appearance) {
        if (_.isUndefined(appearance.position))
            return appearance;
        var new_appearance = _.clone(appearance);
        new_appearance.position = result.unproject(new_appearance.position);
        return new_appearance;
    };
    transform.inverse.inverse = transform;

    var result = {
        camera: camera,
        center: center,
        zoom: zoom,
        width: width,
        height: height,

        transform: transform,

        scene: Lux.scene({
            transform: transform
        }),

        project: function(pt) {
            return this.camera.project(pt);
        },

        unproject: function(pt) {
            return this.camera.unproject(pt);
        },

        resize: function(w, h) {
            aspect_ratio.set(w/h);
            this.width = w;
            this.height = h;
        },

        // Transitions between two projections using van Wijk and Nuij's scale-space geodesics
        // from "Smooth and Efficient zooming and panning", IEEE Infovis 2003.
        transition_to: function(new_center, new_zoom, seconds) {
            if (_.isUndefined(seconds))
                seconds = 3;
            new_zoom = 1.0 / new_zoom;
            var old_zoom = 1.0 / zoom.get(),
                old_center = center.get();
            var start = (new Date()).getTime() / 1000.0;
            var rho = 1.6;
            var direction = vec.minus(new_center, old_center);
            var d = vec.length(direction);

            if (d < 1e-6) {
                console.log("unimplemented"); 
                return;
            }

            var u = [0, d],
                w = [old_zoom, new_zoom],
                b = [(w[1] * w[1] - w[0] * w[0] + Math.pow(rho, 4) * Math.pow(u[1] - u[0], 2)) / (2 * w[0] * rho * rho * (u[1] - u[0])),
                     (w[1] * w[1] - w[0] * w[0] - Math.pow(rho, 4) * Math.pow(u[1] - u[0], 2)) / (2 * w[1] * rho * rho * (u[1] - u[0]))];
            var r = [Math.log(-b[0] + Math.sqrt(b[0] * b[0] + 1)),
                     Math.log(-b[1] + Math.sqrt(b[1] * b[1] + 1))];
            var S = (r[1] - r[0]) / rho;
            
            function cosh(x) {
                return 0.5 * (Math.exp(x) + Math.exp(-x));
            }
            function sinh(x) {
                return 0.5 * (Math.exp(x) - Math.exp(-x));
            }
            function tanh(x) {
                return sinh(x) / cosh(x);
            }

            var that = this;

            var ticker = Lux.Scene.animate(function() {
                var now = Date.now() / 1000.0;
                var s = (now - start) / seconds * S;
                var u_s = (w[0] / (rho * rho)) * (cosh(r[0]) * tanh(rho * s + r[0]) - sinh(r[0])) + u[0];
                var w_s = w[0] * cosh(r[0]) / cosh(rho * s + r[0]);
                var this_center = vec.plus(old_center, vec.scaling(direction, u_s / d));
                var this_zoom = w_s;
                that.center.set(this_center);
                that.zoom.set(1.0 / this_zoom);
                if (s >= S) {
                    that.center.set(new_center);
                    that.zoom.set(1.0 / new_zoom);
                    ticker.stop();
                    return;
                }
            });
        },

        events: {
            mousedown: mousedown,
            mouseup: mouseup,
            mousemove: mousemove,
            mousewheel: mousewheel,
            dblclick: dblclick,
            resize: resize
        }
    };

    return result;
};
/*
 * Shade is the javascript DSL for writing GLSL shaders, part of Lux.
 * 
 */

// FIXME: fix the constant-index-expression hack I've been using to get around
// restrictions. This will eventually be plugged by webgl implementors.

// FIXME: Move this object inside Lux's main object.

var Shade = function(exp)
{
    return Shade.make(exp);
};

(function() {

Shade.debug = false;
/*
 * Shade.Debug contains code that helps with debugging Shade
   expressions, the Shade-to-GLSL compiler, etc.
 */
Shade.Debug = {};
/*
 * Shade.Debug.walk walks an expression dag and calls 'visit' on each
 * expression in a bottom-up order. The return values of 'visit' are
 * passed to the higher-level invocations of 'visit', and so is the
 * dictionary of references that is used to resolve multiple node
 * references. Shade.Debug.walk will only call 'visit' once per node,
 * even if visit can be reached by many different paths in the dag.
 * 
 * If 'revisit' is passed, then it is called every time a node is 
 * revisited. 
 * 
 * Shade.Debug.walk returns the dictionary of references.
 */

Shade.Debug.walk = function(exp, visit, revisit) {
    var refs = {};
    function internal_walk_no_revisit(node) {
        if (!_.isUndefined(refs[node.guid])) {
            return refs[node.guid];
        }
        var parent_results = _.map(node.parents, internal_walk_no_revisit);
        var result = visit(node, parent_results, refs);
        refs[node.guid] = result;
        return result;
    };
    function internal_walk_revisit(node) {
        if (!_.isUndefined(refs[node.guid])) {
            return revisit(node, _.map(node.parents, function(exp) {
                return refs[exp.guid];
            }), refs);
        }
        var parent_results = _.map(node.parents, internal_walk_revisit);
        var result = visit(node, parent_results, refs);
        refs[node.guid] = result;
        return result;
    }
    if (!_.isUndefined(revisit))
        internal_walk_revisit(exp);
    else
        internal_walk_no_revisit(exp);
    return refs;
};
/*
 * from_json produces a JSON object that satisfies the following property:
 * 
 * if j is a Shade expresssion,
 * 
 * Shade.Debug.from_json(f.json()) equals f, up to guid renaming
 */
Shade.Debug.from_json = function(json)
{
    var refs = {};
    function build_node(json_node) {
        var parent_nodes = _.map(json_node.parents, function(parent) {
            return refs[parent.guid];
        });
        switch (json_node.type) {
        case "constant": 
            return Shade.constant.apply(undefined, json_node.values);
        case "struct":
            return Shade.struct(_.build(_.zip(json_node.fields, json_node.parents)));
        case "parameter":
            return Shade.parameter(json_node.parameter_type);
        case "attribute":
            return Shade.attribute(json_node.attribute_type);
        case "varying":
            return Shade.varying(json_node.varying_name, json_node.varying_type);
        case "index":
            return parent_nodes[0].at(parent_nodes[1]);
        };

        // swizzle
        var m = json_node.type.match(/swizzle{(.+)}$/);
        if (m) return parent_nodes[0].swizzle(m[1]);

        // field
        m = json_node.type.match(/struct-accessor{(.+)}$/);
        if (m) return parent_nodes[0].field(m[1]);

        var f = Shade[json_node.type];
        if (_.isUndefined(f)) {
            throw new Error("from_json: unimplemented type '" + json_node.type + "'");
        }
        return f.apply(undefined, parent_nodes);
    }
    function walk_json(json_node) {
        if (json_node.type === 'reference')
            return refs[json_node.guid];
        _.each(json_node.parents, walk_json);
        var new_node = build_node(json_node);
        refs[json_node.guid] = new_node;
        return new_node;
    }
    return walk_json(json);
};
/*
 * Shade.Debug._json_builder is a helper function used internally by
 * the Shade infrastructure to build JSON objects through
 * Shade.Debug.walk visitors.
 * 
 */
Shade.Debug._json_builder = function(type, parent_reprs_fun) {
    parent_reprs_fun = parent_reprs_fun || function (i) { return i; };
    return function(parent_reprs, refs) {
        if (!_.isUndefined(refs[this.guid]))
            return { type: "reference",
                     guid: this.guid };
        else {
            var result = { type: type || this._json_key(),
                           guid: this.guid,
                           parents: parent_reprs };
            return parent_reprs_fun.call(this, result);
        }
    };
};
//////////////////////////////////////////////////////////////////////////////
// make converts objects which can be meaningfully interpreted as
// Exp values to the appropriate Exp values, giving us some poor-man
// static polymorphism

Shade.make = function(value)
{
    if (_.isUndefined(value)) {
        return undefined;
    }
    var t = lux_typeOf(value);
    if (t === 'string') {
        // Did you accidentally say exp1 + exp2 when you meant
        // exp1.add(exp2)?
        throw new Error("strings are not valid shade expressions");
    } else if (t === 'boolean' || t === 'number') {
        if (isNaN(value)) {
            // Did you accidentally say exp1 / exp2 or exp1 - exp2 when you meant
            // exp1.div(exp2) or exp1.sub(exp2)?
            throw new Error("nans are not valid in shade expressions");
        }
        return Shade.constant(value);
    } else if (t === 'array') {
        return Shade.seq(value);
    } else if (t === 'function') {
        /* lifts the passed function to a "shade function".
        
         In other words, this creates a function that replaces every
         passed parameter p by Shade.make(p) This way, we save a lot of
         typing and errors. If a javascript function is expected to
         take shade values and produce shade expressions as a result,
         simply wrap that function around a call to Shade.make()

         FIXME: Document js_evaluate appropriately. This is a cool feature!

         */

        var result = function() {
            var wrapped_arguments = [];
            for (var i=0; i<arguments.length; ++i) {
                wrapped_arguments.push(Shade.make(arguments[i]));
            }
            return Shade.make(value.apply(this, wrapped_arguments));
        };

        var args_type_cache = {};
        var create_parameterized_function = function(shade_function, types) {
            var parameters = _.map(types, function(t) {
                return Shade.parameter(t);
            });
            var expression = shade_function.apply(this, parameters);
            return function() {
                for (var i=0; i<arguments.length; ++i)
                    parameters[i].set(arguments[i]);
                return expression.evaluate();
            };
        };

        result.js_evaluate = function() {
            var args_types = [];
            var args_type_string;
            for (var i=0; i<arguments.length; ++i) {
                args_types.push(Shade.Types.type_of(arguments[i]));
            }
            args_type_string = _.map(args_types, function(t) { return t.repr(); }).join(",");
            if (_.isUndefined(args_type_cache[args_type_string]))
                args_type_cache[args_type_string] = create_parameterized_function(result, args_types);
            return args_type_cache[args_type_string].apply(result, arguments);
        };
        return result;
    }
    t = Shade.Types.type_of(value);
    if (t.is_vec() || t.is_mat()) {
        return Shade.constant(value);
    } else if (value._shade_type === 'attribute_buffer') {
        return Shade.attribute_from_buffer(value);
    } else if (value._shade_type === 'render_buffer') {
        return Shade.sampler2D_from_texture(value.texture);
    } else if (value._shade_type === 'texture') {
        return Shade.sampler2D_from_texture(value);
    } else if (t.equals(Shade.Types.other_t)) { // FIXME struct types 
        return Shade.struct(value);
    }

    return value;
};


// only memoizes on value of first argument, so will fail if function
// takes more than one argument!!
Shade.memoize_on_field = function(field_name, fun, key_fun)
{
    key_fun = key_fun || function(i) { return i; };
    return function() {
        if (_.isUndefined(this._caches[field_name])) {
            this._caches[field_name] = {};
        }
        if (_.isUndefined(this._caches[field_name][arguments[0]])) {
            this._caches[field_name][arguments[0]] = fun.apply(this, arguments);
        }
        return this._caches[field_name][arguments[0]];
    };
};
Shade.memoize_on_guid_dict = function(if_not_found) {
    function evaluate(cache) {
        if (_.isUndefined(cache))
            cache = {};
        var t = cache[this.guid];
        if (_.isUndefined(t)) {
            t = if_not_found.call(this, cache);
            cache[this.guid] = t;
        }
        return t;
    };
    return evaluate;
};
// Shade.unknown encodes a Shade expression whose value
// is not determinable at compile time.
//
// This is used only internally by the compiler

(function() {
    var obj = { _caches: {} };
    obj.fun = Shade.memoize_on_field("_cache", function(type) {
        return Shade._create_concrete_value_exp({
            parents: [],
            type: type,
            evaluate: function() { throw new Error("<unknown> does not support evaluation"); },
            value: function() { throw new Error("<unknown> should never get to compilation"); }
        });
    }, function(type) { 
        return type.repr();
    });
    Shade.unknown = function(type) {
        return obj.fun(type);
    };
})();
Shade.Camera = {};
Shade.Camera.perspective = function(opts)
{
    opts = _.defaults(opts || {}, {
        look_at: [Shade.vec(0, 0, 0), 
                  Shade.vec(0, 0, -1), 
                  Shade.vec(0, 1, 0)],
        field_of_view_y: 45,
        near_distance: 0.1,
        far_distance: 100
    });
    
    var field_of_view_y = opts.field_of_view_y;
    var near_distance = opts.near_distance;
    var far_distance = opts.far_distance;
    var aspect_ratio;
    if (opts.aspect_ratio)
        aspect_ratio = opts.aspect_ratio;
    else {
        var ctx = Lux._globals.ctx;
        if (_.isUndefined(ctx)) {
            throw new Error("aspect_ratio is only optional with an active Lux context");
        }
        aspect_ratio = ctx.viewportWidth / ctx.viewportHeight;
    }

    var view = Shade.look_at(opts.look_at[0], opts.look_at[1], opts.look_at[2]);
    var projection = Shade.perspective_matrix(field_of_view_y, aspect_ratio, near_distance, far_distance);
    var vp_parameter = Shade.mul(projection, view);
    var result = function(obj) {
        return result.project(obj);
    };
    result.project = function(model_vertex) {
        return vp_parameter.mul(model_vertex);
    };
    result.eye_vertex = function(model_vertex) {
        var t = model_vertex.type;
        return view.mul(model_vertex);
    };
    return result;
};
/*
 * FIXME Shade.Camera.ortho currently mixes a view matrix
 * with the projection matrix. This must be factored out.
 */

Shade.Camera.ortho = function(opts)
{
    opts = _.defaults(opts || {}, {
        left: -1,
        right: 1,
        bottom: -1,
        top: 1,
        near: -1,
        far: 1,
        center: Shade.vec(0,0),
        zoom: Shade(1)
    });

    var viewport_ratio;
    var ctx;
    if (opts.aspect_ratio)
        viewport_ratio = opts.aspect_ratio;
    else {
        ctx = Lux._globals.ctx;
        if (_.isUndefined(ctx)) {
            throw new Error("aspect_ratio is only optional with an active Lux context");
        }
        viewport_ratio = ctx.viewportWidth / ctx.viewportHeight;
    };

    var left, right, bottom, top;
    var near = opts.near;
    var far = opts.far;

    left = opts.left;
    right = opts.right;
    bottom = opts.bottom;
    top = opts.top;

    var view_ratio = Shade.sub(right, left).div(Shade.sub(top, bottom));
    var l_or_p = view_ratio.gt(viewport_ratio); // letterbox or pillarbox

    var cx = Shade.add(right, left).div(2);
    var cy = Shade.add(top, bottom).div(2);
    var half_width = Shade.sub(right, left).div(2);
    var half_height = Shade.sub(top, bottom).div(2);
    var corrected_half_width = half_height.mul(viewport_ratio);
    var corrected_half_height = half_width.div(viewport_ratio);

    var l = l_or_p.ifelse(left,  cx.sub(corrected_half_width));
    var r = l_or_p.ifelse(right, cx.add(corrected_half_width));
    var b = l_or_p.ifelse(cy.sub(corrected_half_height), bottom);
    var t = l_or_p.ifelse(cy.add(corrected_half_height), top);
    var m = Shade.ortho(l, r, b, t, near, far);

    function replace_xy_with(vec, new_vec) {
        if (vec.type === Shade.Types.vec2)
            return new_vec;
        else if (vec.type === Shade.Types.vec3)
            return Shade.vec(new_vec, vec.z());
        else if (vec.type === Shade.Types.vec4)
            return Shade.vec(new_vec, vec.swizzle("zw"));
        else
            throw new Error("Shade.ortho requires vec2, vec3, or vec4s");
    };

    var view_xform = Shade(function(model_vertex) {
        var new_v = model_vertex.swizzle("xy").sub(opts.center).mul(opts.zoom);
        return replace_xy_with(model_vertex, new_v);
    });
    var view_xform_invert = Shade(function(view_vertex) {
        var new_v = view_vertex.swizzle("xy").div(opts.zoom).add(opts.center);
        return replace_xy_with(view_vertex, new_v);
    });

    function result(obj) {
        return result.project(obj);
    }
    result.model_to_view = view_xform;
    result.view_to_device = function(view_vertex) {
        return m.mul(view_vertex);
    };
    result.project = function(model_vertex) {
        return m.mul(view_xform(model_vertex));
    };
    result.unproject = function(normalized_view_pos) {
        // var inv_m = Shade.Scale.linear({
        //     domain: [Shade.vec(-1,-1,-1),
        //              Shade.vec( 1, 1, 1)],
        //     range: [Shade.vec(l, b, near),
        //             Shade.vec(r, t, far)]});
        var inv_m = Shade.Scale.linear({
            domain: [Shade.vec(-1,-1),
                     Shade.vec( 1, 1)],
            range: [Shade.vec(l, b),
                    Shade.vec(r, t)]});
        return view_xform_invert(inv_m(normalized_view_pos));
        // var ctx = Lux._globals.ctx;
        // var screen_size = Shade.vec(ctx.parameters.width, ctx.parameters.height);
        // var view_vtx = min.add(max.sub(min).mul(screen_pos.div(screen_size)));
        // return view_xform_invert(view_vtx);
    };
    return result;
};
// Specifying colors in shade in an easier way

(function() {

var css_colors = {
    "aliceblue":            "#F0F8FF",
    "antiquewhite":         "#FAEBD7",
    "aqua":                 "#00FFFF",
    "aquamarine":           "#7FFFD4",
    "azure":                "#F0FFFF",
    "beige":                "#F5F5DC",
    "bisque":               "#FFE4C4",
    "black":                "#000000",
    "blanchedalmond":       "#FFEBCD",
    "blue":                 "#0000FF",
    "blueviolet":           "#8A2BE2",
    "brown":                "#A52A2A",
    "burlywood":            "#DEB887",
    "cadetblue":            "#5F9EA0",
    "chartreuse":           "#7FFF00",
    "chocolate":            "#D2691E",
    "coral":                "#FF7F50",
    "cornflowerblue":       "#6495ED",
    "cornsilk":             "#FFF8DC",
    "crimson":              "#DC143C",
    "cyan":                 "#00FFFF",
    "darkblue":             "#00008B",
    "darkcyan":             "#008B8B",
    "darkgoldenrod":        "#B8860B",
    "darkgray":             "#A9A9A9",
    "darkgrey":             "#A9A9A9",
    "darkgreen":            "#006400",
    "darkkhaki":            "#BDB76B",
    "darkmagenta":          "#8B008B",
    "darkolivegreen":       "#556B2F",
    "darkorange":           "#FF8C00",
    "darkorchid":           "#9932CC",
    "darkred":              "#8B0000",
    "darksalmon":           "#E9967A",
    "darkseagreen":         "#8FBC8F",
    "darkslateblue":        "#483D8B",
    "darkslategray":        "#2F4F4F",
    "darkslategrey":        "#2F4F4F",
    "darkturquoise":        "#00CED1",
    "darkviolet":           "#9400D3",
    "deeppink":             "#FF1493",
    "deepskyblue":          "#00BFFF",
    "dimgray":              "#696969",
    "dimgrey":              "#696969",
    "dodgerblue":           "#1E90FF",
    "firebrick":            "#B22222",
    "floralwhite":          "#FFFAF0",
    "forestgreen":          "#228B22",
    "fuchsia":              "#FF00FF",
    "gainsboro":            "#DCDCDC",
    "ghostwhite":           "#F8F8FF",
    "gold":                 "#FFD700",
    "goldenrod":            "#DAA520",
    "gray":                 "#808080",
    "grey":                 "#808080",
    "green":                "#008000",
    "greenyellow":          "#ADFF2F",
    "honeydew":             "#F0FFF0",
    "hotpink":              "#FF69B4",
    "indianred":            "#CD5C5C",
    "indigo":               "#4B0082",
    "ivory":                "#FFFFF0",
    "khaki":                "#F0E68C",
    "lavender":             "#E6E6FA",
    "lavenderblush":        "#FFF0F5",
    "lawngreen":            "#7CFC00",
    "lemonchiffon":         "#FFFACD",
    "lightblue":            "#ADD8E6",
    "lightcoral":           "#F08080",
    "lightcyan":            "#E0FFFF",
    "lightgoldenrodyellow": "#FAFAD2",
    "lightgray":            "#D3D3D3",
    "lightgrey":            "#D3D3D3",
    "lightgreen":           "#90EE90",
    "lightpink":            "#FFB6C1",
    "lightsalmon":          "#FFA07A",
    "lightseagreen":        "#20B2AA",
    "lightskyblue":         "#87CEFA",
    "lightslategray":       "#778899",
    "lightslategrey":       "#778899",
    "lightsteelblue":       "#B0C4DE",
    "lightyellow":          "#FFFFE0",
    "lime":                 "#00FF00",
    "limegreen":            "#32CD32",
    "linen":                "#FAF0E6",
    "magenta":              "#FF00FF",
    "maroon":               "#800000",
    "mediumaquamarine":     "#66CDAA",
    "mediumblue":           "#0000CD",
    "mediumorchid":         "#BA55D3",
    "mediumpurple":         "#9370D8",
    "mediumseagreen":       "#3CB371",
    "mediumslateblue":      "#7B68EE",
    "mediumspringgreen":    "#00FA9A",
    "mediumturquoise":      "#48D1CC",
    "mediumvioletred":      "#C71585",
    "midnightblue":         "#191970",
    "mintcream":            "#F5FFFA",
    "mistyrose":            "#FFE4E1",
    "moccasin":             "#FFE4B5",
    "navajowhite":          "#FFDEAD",
    "navy":                 "#000080",
    "oldlace":              "#FDF5E6",
    "olive":                "#808000",
    "olivedrab":            "#6B8E23",
    "orange":               "#FFA500",
    "orangered":            "#FF4500",
    "orchid":               "#DA70D6",
    "palegoldenrod":        "#EEE8AA",
    "palegreen":            "#98FB98",
    "paleturquoise":        "#AFEEEE",
    "palevioletred":        "#D87093",
    "papayawhip":           "#FFEFD5",
    "peachpuff":            "#FFDAB9",
    "peru":                 "#CD853F",
    "pink":                 "#FFC0CB",
    "plum":                 "#DDA0DD",
    "powderblue":           "#B0E0E6",
    "purple":               "#800080",
    "red":                  "#FF0000",
    "rosybrown":            "#BC8F8F",
    "royalblue":            "#4169E1",
    "saddlebrown":          "#8B4513",
    "salmon":               "#FA8072",
    "sandybrown":           "#F4A460",
    "seagreen":             "#2E8B57",
    "seashell":             "#FFF5EE",
    "sienna":               "#A0522D",
    "silver":               "#C0C0C0",
    "skyblue":              "#87CEEB",
    "slateblue":            "#6A5ACD",
    "slategray":            "#708090",
    "slategrey":            "#708090",
    "snow":                 "#FFFAFA",
    "springgreen":          "#00FF7F",
    "steelblue":            "#4682B4",
    "tan":                  "#D2B48C",
    "teal":                 "#008080",
    "thistle":              "#D8BFD8",
    "tomato":               "#FF6347",
    "turquoise":            "#40E0D0",
    "violet":               "#EE82EE",
    "wheat":                "#F5DEB3",
    "white":                "#FFFFFF",
    "whitesmoke":           "#F5F5F5",
    "yellow":               "#FFFF00",
    "yellowgreen":          "#9ACD32"
};

var rgb_re = / *rgb *\( *(\d+) *, *(\d+) *, *(\d+) *\) */;
Shade.color = function(spec, alpha)
{
    if (_.isUndefined(alpha))
        alpha = 1;
    if (spec[0] === '#') {
        if (spec.length === 4) {
            return Shade.vec(parseInt(spec[1], 16) / 15,
                             parseInt(spec[2], 16) / 15,
                             parseInt(spec[3], 16) / 15, alpha);
        } else if (spec.length == 7) {
            return Shade.vec(parseInt(spec.substr(1,2), 16) / 255,
                             parseInt(spec.substr(3,2), 16) / 255,
                             parseInt(spec.substr(5,2), 16) / 255, alpha);
        } else
            throw new Error("hex specifier must be either #rgb or #rrggbb");
    }
    var m = rgb_re.exec(spec);
    if (m) {
        return Shade.vec(parseInt(m[1], 10) / 255,
                         parseInt(m[2], 10) / 255,
                         parseInt(m[3], 10) / 255, alpha);
    }
    if (spec in css_colors)
        return Shade.color(css_colors[spec], alpha);
    throw new Error("unrecognized color specifier " + spec);
};
}());
/*
 A range expression represents a finite stream of values. 

 It is meant
 to be an abstraction over looping, and provides a few ways to combine values.

 Currently the only operations supported are plain stream
 transformations (like "map") and fold (like "reduce").

 It should be possible to add, at the very least, "filter", "scan", and "firstWhich".

 nb: nested loops will require deep changes to the infrastructure, and
 won't be supported for a while.

 Currently, looping is fairly untested.
*/

(function() {

Shade.loop_variable = function(type, force_no_declare)
{
    return Shade._create_concrete_exp({
        parents: [],
        type: type,
        expression_type: "loop_variable",
        glsl_expression: function() {
            return this.glsl_name;
        },
        compile: function(ctx) {
            if (_.isUndefined(force_no_declare))
                ctx.global_scope.add_declaration(type.declare(this.glsl_name));
        },
        loop_variable_dependencies: Shade.memoize_on_field("_loop_variable_dependencies", function () {
            return [this];
        }),
        evaluate: function() {
            throw new Error("evaluate undefined for loop_variable");
        }
    });
};

function BasicRange(range_begin, range_end, value, condition, termination)
{
    this.begin = Shade.make(range_begin).as_int();
    this.end = Shade.make(range_end).as_int();
    this.value = value || function(index) { return index; };
    this.condition = condition || function() { return Shade.make(true); };
    this.termination = termination || function() { return Shade.make(false); };
};

Shade.range = function(range_begin, range_end, value, condition, termination)
{
    return new BasicRange(range_begin, range_end, value, condition, termination);
};

BasicRange.prototype.transform = function(xform)
{
    var that = this;
    return Shade.range(
        this.begin,
        this.end,
        function (i) {
            var input = that.value(i);
            var result = xform(input, i);
            return result;
        },
        this.condition,
        this.termination
    );
};

BasicRange.prototype.filter = function(new_condition)
{
    var that = this;
    return Shade.range(
        this.begin,
        this.end,
        this.value,
        function (value, i) {
            var old_condition = that.condition(value, i);
            var result = Shade.and(old_condition, new_condition(value, i));
            return result;
        },
        this.termination
    );
};

BasicRange.prototype.break_if = function(new_termination)
{
    var that = this;
    return Shade.range(
        this.begin,
        this.end,
        this.value,
        this.condition,
        function (value, i) {
            var old_termination = that.termination(value, i);
            var result = Shade.or(old_termination, new_termination(value, i));
            return result;
        }
    );
};

BasicRange.prototype.fold = Shade(function(operation, starting_value)
{
    var index_variable = Shade.loop_variable(Shade.Types.int_t, true);
    var accumulator_value = Shade.loop_variable(starting_value.type, true);
    var element_value = this.value(index_variable);
    var condition_value = this.condition(element_value, index_variable);
    var termination_value = this.termination(element_value, index_variable);
    var result_type = accumulator_value.type;
    var operation_value = operation(accumulator_value, element_value);
    // FIXME: instead of refusing to compile, we should transform
    // violating expressions to a transformed index variable loop 
    // with a termination condition
    if (!this.begin.is_constant())
        throw new Error("WebGL restricts loop index variable initialization to be constant");
    if (!this.end.is_constant())
        throw new Error("WebGL restricts loop index termination check to be constant");

    var result = Shade._create_concrete_exp({
        has_scope: true,
        patch_scope: function() {
            var index_variable = this.parents[2];
            var accumulator_value = this.parents[3];
            var element_value = this.parents[4];
            var operation_value = this.parents[6];
            var condition_value = this.parents[7];
            var termination_value = this.parents[8];
            var that = this;

            function patch_internal(exp) {
                _.each(exp.sorted_sub_expressions(), function(node) {
                    if (_.any(node.loop_variable_dependencies(), function(dep) {
                        return dep.glsl_name === index_variable.glsl_name ||
                            dep.glsl_name === accumulator_value.glsl_name;
                    })) {
                        node.scope = that.scope;
                    };
                });
            }

            _.each([element_value, operation_value, condition_value, termination_value], patch_internal);
        },
        parents: [this.begin, this.end, 
                  index_variable, accumulator_value, element_value,
                  starting_value, operation_value,
                  condition_value, termination_value
                 ],
        type: result_type,
        element: Shade.memoize_on_field("_element", function(i) {
            if (this.type.is_pod()) {
                if (i === 0)
                    return this;
                else
                    throw new Error(this.type.repr() + " is an atomic type");
            } else
                return this.at(i);
        }),
        loop_variable_dependencies: Shade.memoize_on_field("_loop_variable_dependencies", function () {
            return [];
        }),
        compile: function(ctx) {
            var beg = this.parents[0];
            var end = this.parents[1];
            var index_variable = this.parents[2];
            var accumulator_value = this.parents[3];
            var element_value = this.parents[4];
            var starting_value = this.parents[5];
            var operation_value = this.parents[6];
            var condition = this.parents[7];
            var termination = this.parents[8];
            var must_evaluate_condition = !(condition.is_constant() && (condition.constant_value() === true));
            var must_evaluate_termination = !(termination.is_constant() && (termination.constant_value() === false));

            ctx.global_scope.add_declaration(accumulator_value.type.declare(accumulator_value.glsl_name));
            ctx.strings.push(this.type.repr(), this.glsl_name, "() {\n");
            ctx.strings.push("    ",accumulator_value.glsl_name, "=", starting_value.glsl_expression(), ";\n");

            ctx.strings.push("    for (int",
                             index_variable.glsl_expression(),"=",beg.glsl_expression(),";",
                             index_variable.glsl_expression(),"<",end.glsl_expression(),";",
                             "++",index_variable.glsl_expression(),") {\n");

            _.each(this.scope.declarations, function(exp) {
                ctx.strings.push("        ", exp, ";\n");
            });
            if (must_evaluate_condition) {
                ctx.strings.push("      if (", condition.glsl_expression(), ") {\n");
            }
            _.each(this.scope.initializations, function(exp) {
                ctx.strings.push("        ", exp, ";\n");
            });
            ctx.strings.push("        ",
                             accumulator_value.glsl_expression(),"=",
                             operation_value.glsl_expression() + ";\n");
            if (must_evaluate_termination) {
                ctx.strings.push("        if (", termination.glsl_expression(), ") break;\n");
            }
            if (must_evaluate_condition) {
                ctx.strings.push("      }\n");
            }
            ctx.strings.push("    }\n");
            ctx.strings.push("    return", accumulator_value.glsl_expression(), ";\n");
            ctx.strings.push("}\n");

            if (this.children_count > 1) {
                this.precomputed_value_glsl_name = ctx.request_fresh_glsl_name();
                ctx.global_scope.add_declaration(this.type.declare(this.precomputed_value_glsl_name));
                ctx.global_scope.add_initialization(this.precomputed_value_glsl_name + " = " + this.glsl_name + "()");
            }
        },
        glsl_expression: function() {
            if (this.children_count > 1) {
                return this.precomputed_value_glsl_name;
            } else {
                return this.glsl_name + "()";
            }
        },
        evaluate: function() {
            throw new Error("evaluate currently undefined for looping expressions");
        }
    });

    return result;
});

//////////////////////////////////////////////////////////////////////////////

BasicRange.prototype.sum = function()
{
    var this_begin_v = this.value(this.begin);

    return this.fold(Shade.add, this_begin_v.type.zero);
};

BasicRange.prototype.max = function()
{
    var this_begin_v = this.value(this.begin);
    return this.fold(Shade.max, this_begin_v.type.minus_infinity);
};

BasicRange.prototype.average = function()
{
    // special-case average when we know the total number of samples in advance
    // 
    // this is ugly, but how could I make it better?
    var s = this.sum();
    if ((s.parents[7].is_constant() &&
         s.parents[7].constant_value() === true) &&
        (s.parents[8].is_constant() &&
         s.parents[8].constant_value() === false)) {
        if (s.type.equals(Shade.Types.int_t)) s = s.as_float();
        return s.div(this.end.sub(this.begin).as_float());
    } else {
        var xf = this.transform(function(v) {
            return Shade({
                s1: 1,
                sx: v
            });
        });
        var sum_result = xf.sum();
        var sx = sum_result("sx");
        if (sx.type.equals(Shade.Types.int_t)) {
            sx = sx.as_float();
        }
        return sx.div(sum_result("s1"));
    }
};

Shade.locate = Shade(function(accessor, target, left, right, nsteps)
{
    function halfway(a, b) { return a.as_float().add(b.as_float()).div(2).as_int(); };

    nsteps = nsteps || right.sub(left).log2().ceil();
    var base = Shade.range(0, nsteps);
    var mid = halfway(left, right);
    var initial_state = Shade({
        l: left.as_int(),
        r: right.as_int(),
        m: mid.as_int(),
        vl: accessor(left),
        vr: accessor(right),
        vm: accessor(mid)
    });
    return base.fold(function(state, i) {
        var right_nm = halfway(state("m"), state("r"));
        var left_nm = halfway(state("l"), state("m"));
        return state("vm").lt(target).ifelse(Shade({
            l: state("m"), vl: state("vm"),
            m: right_nm,   vm: accessor(right_nm),
            r: state("r"), vr: state("vr")
        }), Shade({
            l: state("l"), vl: state("vl"),
            m: left_nm,    vm: accessor(left_nm),
            r: state("m"), vr: state("vm")
        }));
    }, initial_state);
});

})();
Shade.unique_name = function() {
    var counter = 0;
    return function() {
        counter = counter + 1;
        return "_unique_name_" + counter;
    };
}();
//////////////////////////////////////////////////////////////////////////////
// roll-your-own prototypal inheritance

Shade._create = (function() {
    var guid = 0;
    return function(base_type, new_obj)
    {
        // function F() {
        //     for (var key in new_obj) {
        //         this[key] = new_obj[key];
        //     }
        //     this.guid = "GUID_" + guid;

        //     // this is where memoize_on_field stashes results. putting
        //     // them all in a single member variable makes it easy to
        //     // create a clean prototype
        //     this._caches = {};

        //     guid += 1;
        // }
        // F.prototype = base_type;
        // return new F();

        var result = function() {
            return result.call_operator.apply(result, _.toArray(arguments));
        };

        for (var key in new_obj) {
            result[key] = new_obj[key];
        }
        result.guid = guid;

        // this is where memoize_on_field stashes results. putting
        // them all in a single member variable makes it easy to
        // create a clean prototype
        result._caches = {};

        guid += 1;
        result.__proto__ = base_type;
        return result;
    };
})();

Shade._create_concrete = function(base, requirements)
{
    function create_it(new_obj) {
        for (var i=0; i<requirements.length; ++i) {
            var field = requirements[i];
            if (!(field in new_obj)) {
                throw new Error("new expression missing " + requirements[i]);
            }
            if (_.isUndefined(new_obj[field])) {
                throw new Error("field '" + field + "' cannot be undefined");
            }
        }
        return Shade._create(base, new_obj);
    }
    return create_it;
};
Shade.Types = {};
// Shade.Types.type_of implements the following spec:
// 
// for all shade values s such that s.evaluate() equals v,
// s.type.equals(Shade.Types.type_of(v))

// In addition, if there is no s such that s.evaluate() equals v,
// then Shade.Types.type_of returns other_t. That's a kludge,
// but is convenient.
Shade.Types.type_of = function(v)
{
    var t = typeof v;
    if (t === "boolean") {
        return Shade.Types.bool_t;
    } else if (t === "number") {
        return Shade.Types.float_t;
    } else if (Lux.is_shade_expression(v)) {
        return Shade.Types.shade_t;
    } else if (_.isUndefined(v)) {
        return Shade.Types.undefined_t;
    } else if (!_.isUndefined(v.buffer) && v.buffer._type) {
        return Shade.Types[v.buffer._type];
    } else {
        return Shade.Types.other_t;
    }
};
// <rant> How I wish I had algebraic data types. </rant>
Shade.Types.base_t = {
    is_floating: function() { return false; },
    is_integral: function() { return false; },
    is_array: function()    { return false; },
    // POD = plain old data (ints, bools, floats)
    is_pod: function()      { return false; },
    is_vec: function()      { return false; },
    is_mat: function()      { return false; },
    vec_dimension: function() { 
        throw new Error("is_vec() === false, cannot call vec_dimension");
    },
    is_function: function() { return false; },
    is_struct:   function() { return false; },
    is_sampler:  function() { return false; },
    equals: function(other) {
        if (_.isUndefined(other))
            throw new Error("type cannot be compared to undefined");
        return this.repr() == other.repr();
    },
    swizzle: function(pattern) {
        throw new Error("type '" + this.repr() + "' does not support swizzling");
    },
    element_type: function(i) {
        throw new Error("invalid call: atomic expression");
    },
    declare: function(glsl_name) {
        return this.repr() + " " + glsl_name;
    }
    // repr
    // 
    // for arrays:
    //   array_base
    //   array_size
    // 
    // for function types:
    //   function_return_type
    //   function_parameter
    //   function_parameter_count
    // 
    // for structs:
    //   fields

    // value_equals
    //   value_equals is a function that takes two parameters as produced
    //   by the constant_value() or evaluate() method of an object with
    //   the given type, and tests their equality.
};
(function() {

function is_valid_basic_type(repr) {
    if (repr === 'float') return true;
    if (repr === 'int') return true;
    if (repr === 'bool') return true;
    if (repr === 'void') return true;
    if (repr === 'sampler2D') return true;
    if (repr.substring(0, 3) === 'mat' &&
        (Number(repr[3]) > 1 && 
         Number(repr[3]) < 5)) return true;
    if (repr.substring(0, 3) === 'vec' &&
        (Number(repr[3]) > 1 && 
         Number(repr[3]) < 5)) return true;
    if (repr.substring(0, 4) === 'bvec' &&
        (Number(repr[4]) > 1 && 
         Number(repr[4]) < 5)) return true;
    if (repr.substring(0, 4) === 'ivec' &&
        (Number(repr[4]) > 1 && 
         Number(repr[4]) < 5)) return true;
    // if (repr === '__auto__') return true;
    return false;
}

Shade.Types.basic = function(repr) {
    if (!is_valid_basic_type(repr)) {
        throw new Error("invalid basic type '" + repr + "'");
    }
    return Shade.Types[repr];
};

Shade.Types._create_basic = function(repr) { 
    return Shade._create(Shade.Types.base_t, {
        declare: function(glsl_name) { return repr + " " + glsl_name; },
        repr: function() { return repr; },
        swizzle: function(pattern) {
            if (!this.is_vec()) {
                throw new Error("swizzle requires a vec");
            }
            var base_repr = this.repr();
            var base_size = Number(base_repr[base_repr.length-1]);

            var valid_re, group_res;
            switch (base_size) {
            case 2:
                valid_re = /[rgxyst]+/;
                group_res = [ /[rg]/, /[xy]/, /[st]/ ];
                break;
            case 3:
                valid_re = /[rgbxyzstp]+/;
                group_res = [ /[rgb]/, /[xyz]/, /[stp]/ ];
                break;
            case 4:
                valid_re = /[rgbazxyzwstpq]+/;
                group_res = [ /[rgba]/, /[xyzw]/, /[stpq]/ ];
                break;
            default:
                throw new Error("internal error on swizzle");
            }
            if (!pattern.match(valid_re)) {
                throw new Error("invalid swizzle pattern '" + pattern + "'");
            }
            var count = 0;
            for (var i=0; i<group_res.length; ++i) {
                if (pattern.match(group_res[i])) count += 1;
            }
            if (count != 1) {
                throw new Error("swizzle pattern '" + pattern + 
                       "' belongs to more than one group");
            }
            if (pattern.length === 1) {
                return this.array_base();
            } else {
                var type_str = base_repr.substring(0, base_repr.length-1) + pattern.length;
                return Shade.Types[type_str];
            }
        },
        is_pod: function() {
            var repr = this.repr();
            return ["float", "bool", "int"].indexOf(repr) !== -1;
        },
        is_vec: function() {
            var repr = this.repr();
            if (repr.substring(0, 3) === "vec")
                return true;
            if (repr.substring(0, 4) === "ivec")
                return true;
            if (repr.substring(0, 4) === "bvec")
                return true;
            return false;
        },
        is_mat: function() {
            var repr = this.repr();
            if (repr.substring(0, 3) === "mat")
                return true;
            return false;
        },
        vec_dimension: function() {
            var repr = this.repr();
            if (repr.substring(0, 3) === "vec")
                return parseInt(repr[3], 10);
            if (repr.substring(0, 4) === "ivec" ||
                repr.substring(0, 4) === "bvec")
                return parseInt(repr[4], 10);
            if (this.repr() === 'float'
                || this.repr() === 'int'
                || this.repr() === 'bool')
                // This is convenient: assuming vec_dimension() === 1 for POD 
                // lets me pretend floats, ints and bools are vec1, ivec1 and bvec1.
                // 
                // However, this might have
                // other bad consequences I have not thought of.
                //
                // For example, I cannot make float_t.is_vec() be true, because
                // this would allow sizzling from a float, which GLSL disallows.
                return 1;
            if (!this.is_vec()) {
                throw new Error("is_vec() === false, cannot call vec_dimension");
            }
            throw new Error("internal error");
        },
        is_array: function() {
            var repr = this.repr();
            if (repr.substring(0, 3) === "mat")
                return true;
            if (this.is_vec())
                return true;
            return false;
        },
        array_base: function() {
            var repr = this.repr();
            if (repr.substring(0, 3) === "mat")
                return Shade.Types["vec" + repr[3]];
            if (repr.substring(0, 3) === "vec")
                return Shade.Types.float_t;
            if (repr.substring(0, 4) === "bvec")
                return Shade.Types.bool_t;
            if (repr.substring(0, 4) === "ivec")
                return Shade.Types.int_t;
            if (repr === "float")
                return Shade.Types.float_t;
            throw new Error("datatype not array");
        },
        size_for_vec_constructor: function() {
            var repr = this.repr();
            if (this.is_array())
                return this.array_size();
            if (repr === 'float' ||
                repr === 'bool' ||
                repr === 'int')
                return 1;
            throw new Error("not usable inside vec constructor");
        },
        array_size: function() {
            if (this.is_vec())
                return this.vec_dimension();
            var repr = this.repr();
            if (repr.substring(0, 3) === "mat")  
                return parseInt(repr[3], 10);
            throw new Error("datatype not array");
        },
        is_floating: function() {
            var repr = this.repr();
            if (repr === "float")
                return true;
            if (repr.substring(0, 3) === "vec")
                return true;
            if (repr.substring(0, 3) === "mat")
                return true;
            return false;
        },
        is_integral: function() {
            var repr = this.repr();
            if (repr === "int")
                return true;
            if (repr.substring(0, 4) === "ivec")
                return true;
            return false;
        },
        is_sampler: function() {
            var repr = this.repr();
            if (repr === 'sampler2D')
                return true;
            return false;
        },
        element_type: function(i) {
            if (this.is_pod()) {
                if (i === 0)
                    return this;
                else
                    throw new Error("invalid call: " + this.repr() + " is atomic");
            } else if (this.is_vec()) {
                var f = this.repr()[0];
                var d = this.array_size();
                if (i < 0 || i >= d) {
                    throw new Error("invalid call: " + this.repr() + 
                                    " has no element " + i);
                }
                if (f === 'v')
                    return Shade.Types.float_t;
                else if (f === 'b')
                    return Shade.Types.bool_t;
                else if (f === 'i')
                    return Shade.Types.int_t;
                else
                    throw new Error("internal error");
            } else
                // FIXME implement this
                throw new Error("unimplemented for mats");
        },
        value_equals: function(v1, v2) {
            if (this.is_pod())
                return v1 === v2;
            if (this.is_vec())
                return vec.equal(v1, v2);
            if (this.is_mat())
                return mat.equal(v1, v2);
            throw new Error("bad type for equality comparison: " + this.repr());
        }
    });
};

})();
Shade.Types.array = function(base_type, size) {
    return Shade._create(Shade.Types.base_t, {
        is_array: function() { return true; },
        declare: function(glsl_name) {
            return base_type.declare(glsl_name) + "[" + size + "]";
        },
        repr: function() {
            return base_type.repr() + "[" + size + "]";
        },
        array_size: function() {
            return size;
        },
        array_base: function() {
            return base_type;
        }
    });
};
Shade.Types.function_t = function(return_type, param_types) {
    return Shade._create(Shade.Types.base_t, {
        repr: function() {
            return "(" + return_type.repr() + ")("
                + ", ".join(param_types.map(function (o) { 
                    return o.repr(); 
                }));
        },
        is_function: function() {
            return true;
        },
        function_return_type: function() {
            return return_type;
        },
        function_parameter: function(i) {
            return param_types[i];
        },
        function_parameter_count: function() {
            return param_types.length;
        }
    });
};
(function() {

    var simple_types = 
        ["mat2", "mat3", "mat4",
         "vec2", "vec3", "vec4",
         "ivec2", "ivec3", "ivec4",
         "bvec2", "bvec3", "bvec4"];

    for (var i=0; i<simple_types.length; ++i) {
        Shade.Types[simple_types[i]] = Shade.Types._create_basic(simple_types[i]);
    }

    Shade.Types.float_t   = Shade.Types._create_basic('float');
    Shade.Types.bool_t    = Shade.Types._create_basic('bool');
    Shade.Types.int_t     = Shade.Types._create_basic('int');

    Shade.Types.sampler2D = Shade.Types._create_basic('sampler2D');
    Shade.Types.void_t    = Shade.Types._create_basic('void');

    // create aliases so that x === y.repr() implies Shade.Types[x] === y
    Shade.Types["float"] = Shade.Types.float_t;
    Shade.Types["bool"]  = Shade.Types.bool_t;
    Shade.Types["int"]   = Shade.Types.int_t;
    Shade.Types["void"]  = Shade.Types.void_t;

    // represents other "non-constant" types. kludgy, but hey.
    Shade.Types.undefined_t = Shade.Types._create_basic('<undefined>');
    Shade.Types.shade_t     = Shade.Types._create_basic('<shade>');
    Shade.Types.other_t     = Shade.Types._create_basic('<other>');
})();
(function () {

var _structs = {};

function _register_struct(type) {
    var t = type._struct_key;
    var v = _structs[t];
    if (v !== undefined) {
        throw new Error("type " + t + " already registered as " + v.internal_type_name);
    }
    _structs[t] = type;
};

var struct_key = function(obj) {
    return _.map(obj, function(value, key) {
        if (value.is_function()) {
            throw new Error("function types not allowed inside struct");
        }
        if (value.is_sampler()) {
            throw new Error("sampler types not allowed inside struct");
        }
        if (value.is_struct()) {
            return "[" + key + ":" + value.internal_type_name + "]";
        }
        return "[" + key + ":" + value.repr() + "]";
    }).sort().join("");
};

function field_indices(obj) {
    var lst = _.map(obj, function(value, key) {
        return [key, value.repr()];
    });
    return lst.sort(function(v1, v2) {
        if (v1[0] < v2[0]) return -1;
        if (v1[0] > v2[0]) return 1;
        if (v1[1] < v2[1]) return -1;
        if (v1[1] > v2[1]) return 1;
        return 0;
    });
};

Shade.Types.struct = function(fields) {
    var key = struct_key(fields);
    var t = _structs[key];
    if (t) return t;
    var field_index = {};
    _.each(field_indices(fields), function(v, i) {
        field_index[v[0]] = i;
    });
    var result = Shade._create(Shade.Types.struct_t, {
        fields: fields,
        field_index: field_index,
        _struct_key: key
    });
    result.internal_type_name = 'type_' + result.guid;
    _register_struct(result);

    _.each(["zero", "infinity", "minus_infinity"], function(value) {
        if (_.all(fields, function(v, k) { return !_.isUndefined(v[value]); })) {
            var c = {};
            _.each(fields, function(v, k) {
                c[k] = v[value];
            });
            result[value] = Shade.struct(c);
        }
    });

    return result;
};

Shade.Types.struct_t = Shade._create(Shade.Types.base_t, {
    is_struct: function() { return true; },
    repr: function() { return this.internal_type_name; }
});

})();
Shade.VERTEX_PROGRAM_COMPILE = 1;
Shade.FRAGMENT_PROGRAM_COMPILE = 2;
Shade.UNSET_PROGRAM_COMPILE = 3;

function new_scope()
{
    return {
        declarations: [],
        initializations: [],
        enclosing_scope: undefined,
        
        // make all declarations 
        // global since names are unique anyway
        add_declaration: function(exp) {
            // this.declarations.push(exp);
            this.enclosing_scope.add_declaration(exp);
        },
        add_initialization: function(exp) {
            this.initializations.push(exp);
        },
        show: function() {
            return "(Scope decls " 
                + String(this.declarations)
                + " inits "
                + String(this.initializations)
                + " enclosing "
                + this.enclosing_scope.show()
                + " )";
        }
    };
};

Shade.CompilationContext = function(compile_type)
{
    return {
        freshest_glsl_name: 0,
        compile_type: compile_type || Shade.UNSET_PROGRAM_COMPILE,
        float_precision: "highp",
        strings: [],
        global_decls: [],
        declarations: { uniform: {},
                        attribute: {},
                        varying: {}
                      },
        declared_struct_types: {},
        // min_version: -1,
        source: function() {
            return this.strings.join(" ");
        },
        request_fresh_glsl_name: function() {
            var int_name = this.freshest_glsl_name++;
            return "glsl_name_" + int_name;
        },
        declare: function(decltype, glsl_name, type, declmap) {
            if (_.isUndefined(type)) {
                throw new Error("must define type");
            }
            if (!(glsl_name in declmap)) {
                declmap[glsl_name] = type;
                this.strings.push(decltype + " " + type.declare(glsl_name) + ";\n");
            } else {
                var existing_type = declmap[glsl_name];
                if (!existing_type.equals(type)) {
                    throw new Error("compile error: different expressions use "
                           + "conflicting types for '" + decltype + " " + glsl_name
                           + "': '" + existing_type.repr() + "', '"
                           + type.repr() + "'");
                }
            }
        },
        declare_uniform: function(glsl_name, type) {
            this.declare("uniform", glsl_name, type, this.declarations.uniform);
        },
        declare_varying: function(glsl_name, type) {
            this.declare("varying", glsl_name, type, this.declarations.varying);
        },
        declare_attribute: function(glsl_name, type) {
            this.declare("attribute", glsl_name, type, this.declarations.attribute);
        },
        declare_struct: function(type) {
            var that = this;
            if (!_.isUndefined(this.declared_struct_types[type.internal_type_name]))
                return;
            _.each(type.fields, function(v) {
                if (v.is_struct() && 
                    _.isUndefined(this.declared_struct_types[type.internal_type_name])) {
                    throw new Error("internal error; declare_struct found undeclared internal struct");
                }
            });
            this.global_decls.push("struct", type.internal_type_name, "{\n");
            var internal_decls = [];
            _.each(type.field_index, function(i, k) {
                internal_decls[i] = type.fields[k].declare(k);
            });
            _.each(internal_decls, function(v) {
                that.global_decls.push("    ",v, ";\n");
            });
            this.global_decls.push("};\n");
            this.declared_struct_types[type.internal_type_name] = true;
        },
        compile: function(fun) {
            var that = this;
            this.global_decls = [];

            this.global_scope = {
                initializations: [],
                add_declaration: function(exp) {
                    that.global_decls.push(exp, ";\n");
                },
                add_initialization: function(exp) {
                    this.initializations.push(exp);
                },
                show: function() {
                    return "(Global scope)";
                }
            };

            var topo_sort = fun.sorted_sub_expressions();
            var i;
            var p = this.strings.push;
            _.each(topo_sort, function(n) {
                n.children_count = 0;
                n.is_unconditional = false;
                n.glsl_name = that.request_fresh_glsl_name();
                n.set_requirements(this);
                if (n.type.is_struct()) {
                    that.declare_struct(n.type);
                }
                for (var j=0; j<n.parents.length; ++j) {
                    n.parents[j].children_count++;
                    // adds base scope to objects which have them.
                    // FIXME currently all scope objects point directly to global scope
                    n.scope = n.has_scope ? new_scope() : that.global_scope;
                }
            });
            // top-level node is always unconditional.
            topo_sort[topo_sort.length-1].is_unconditional = true;
            // top-level node has global scope.
            topo_sort[topo_sort.length-1].scope = this.global_scope;
            i = topo_sort.length;
            while (i--) {
                var n = topo_sort[i];
                n.propagate_conditions();
                for (var j=0; j<n.parents.length; ++j) {
                    if (n.parents[j].has_scope)
                        n.parents[j].scope.enclosing_scope = n.scope;
                }
                n.patch_scope();
            }
            for (i=0; i<topo_sort.length; ++i) {
                topo_sort[i].compile(this);
            }

            var args = [0, 0];
            args.push.apply(args, this.global_decls);
            this.strings.splice.apply(this.strings, args);
            this.strings.splice(0, 0, "precision",this.float_precision,"float;\n");
            this.strings.splice(0, 0, "#extension GL_OES_standard_derivatives : enable\n");
            this.strings.push("void main() {\n");
            _.each(this.global_scope.initializations, function(exp) {
                that.strings.push("    ", exp, ";\n");
            });
            this.strings.push("    ", fun.glsl_expression(), ";\n", "}\n");
            // for (i=0; i<this.initialization_exprs.length; ++i)
            //     this.strings.push("    ", this.initialization_exprs[i], ";\n");
            // this.strings.push("    ", fun.glsl_expression(), ";\n", "}\n");
        },
        add_initialization: function(expr) {
            this.global_scope.initializations.push(expr);
        },
        value_function: function() {
            var that = this;
            this.strings.push(arguments[0].type.repr(),
                              arguments[0].glsl_name,
                              "(");
            _.each(arguments[0].loop_variable_dependencies(), function(exp, i) {
                if (i > 0)
                    that.strings.push(',');
                that.strings.push('int', exp.glsl_name);
            });
            this.strings.push(") {\n",
                              "    return ");
            for (var i=1; i<arguments.length; ++i) {
                this.strings.push(arguments[i]);
            }
            this.strings.push(";\n}\n");
        },
        void_function: function() {
            this.strings.push("void",
                              arguments[0].glsl_name,
                              "() {\n",
                              "    ");
            for (var i=1; i<arguments.length; ++i) {
                this.strings.push(arguments[i]);
            }
            this.strings.push(";\n}\n");
        }
    };
};
Shade.Exp = {
    glsl_expression: function() {
        return this.glsl_name + "()";
    },
    parent_is_unconditional: function(i) {
        return true;
    },
    propagate_conditions: function() {
        // the condition for an execution of a node is the
        // disjunction of the conjunction of all its children and their respective
        // edge conditions
        for (var i=0; i<this.parents.length; ++i)
            this.parents[i].is_unconditional = (
                this.parents[i].is_unconditional ||
                    (this.is_unconditional && 
                     this.parent_is_unconditional(i)));

    },
    set_requirements: function() {},

    // returns all sub-expressions in topologically-sorted order
    sorted_sub_expressions: Shade.memoize_on_field("_sorted_sub_expressions", function() {
        var so_far = [];
        var visited_guids = [];
        var topological_sort_internal = function(exp) {
            var guid = exp.guid;
            if (visited_guids[guid]) {
                return;
            }
            var parents = exp.parents;
            var i = parents.length;
            while (i--) {
                topological_sort_internal(parents[i]);
            }
            so_far.push(exp);
            visited_guids[guid] = true;
        };
        topological_sort_internal(this);
        return so_far;
    }),

    //////////////////////////////////////////////////////////////////////////
    // javascript-side evaluation of Shade expressions

    evaluate: function() {
        throw new Error("internal error: evaluate undefined for " + this.expression_type);
    },
    is_constant: function() {
        return false;
    },
    constant_value: Shade.memoize_on_field("_constant_value", function() {
        if (!this.is_constant())
            throw new Error("constant_value called on non-constant expression");
        return this.evaluate();
    }),
    element_is_constant: function(i) {
        return false;
    },
    element_constant_value: function(i) {
        throw new Error("invalid call: no constant elements");
    },

    //////////////////////////////////////////////////////////////////////////
    // element access for compound expressions

    element: function(i) {
        // FIXME. Why doesn't this check for is_pod and use this.at()?
        throw new Error("invalid call: atomic expression");
    },

    //////////////////////////////////////////////////////////////////////////
    // some sugar

    add: function(op) {
        return Shade.add(this, op);
    },
    mul: function(op) {
        return Shade.mul(this, op);
    },
    div: function(op) {
        return Shade.div(this, op);
    },
    mod: function(op) {
        return Shade.mod(this, op);
    },
    sub: function(op) {
        return Shade.sub(this, op);
    },
    norm: function() {
        return Shade.norm(this);
    },
    distance: function(other) {
        return Shade.distance(this, other);
    },
    dot: function(other) {
        return Shade.dot(this, other);
    },
    cross: function(other) {
        return Shade.cross(this, other);
    },
    normalize: function() {
        return Shade.normalize(this);
    },
    reflect: function(other) {
        return Shade.reflect(this, other);
    },
    refract: function(o1, o2) {
        return Shade.refract(this, o1, o2);
    },
    texture2D: function(coords) {
        return Shade.texture2D(this, coords);
    },
    clamp: function(mn, mx) {
        return Shade.clamp(this, mn, mx);
    },
    min: function(other) {
        return Shade.min(this, other);
    },
    max: function(other) {
        return Shade.max(this, other);
    },

    per_vertex: function() {
        return Shade.per_vertex(this);
    },
    discard_if: function(condition) {
        return Shade.discard_if(this, condition);
    },

    // overload this to overload exp(foo)
    call_operator: function() {
        if (this.type.is_struct()) {
            return this.field(arguments[0]);
        }
        return this.mul.apply(this, arguments);
    },

    // all sugar for funcs_1op is defined later on in the source

    //////////////////////////////////////////////////////////////////////////

    as_int: function() {
        if (this.type.equals(Shade.Types.int_t))
            return this;
        var parent = this;
        return Shade._create_concrete_value_exp({
            parents: [parent],
            type: Shade.Types.int_t,
            value: function() { return "int(" + this.parents[0].glsl_expression() + ")"; },
            is_constant: function() { return this.parents[0].is_constant(); },
            evaluate: Shade.memoize_on_guid_dict(function(cache) {
                var v = this.parents[0].evaluate(cache);
                return Math.floor(v);
            }),
            expression_type: "cast(int)"
        });
    },
    as_bool: function() {
        if (this.type.equals(Shade.Types.bool_t))
            return this;
        var parent = this;
        return Shade._create_concrete_value_exp({
            parents: [parent],
            type: Shade.Types.bool_t,
            value: function() { return "bool(" + this.parents[0].glsl_expression() + ")"; },
            is_constant: function() { return this.parents[0].is_constant(); },
            evaluate: Shade.memoize_on_guid_dict(function(cache) {
                var v = this.parents[0].evaluate(cache);
                return ~~v;
            }),
            expression_type: "cast(bool)"
        });
    },
    as_float: function() {
        if (this.type.equals(Shade.Types.float_t))
            return this;
        var parent = this;
        return Shade._create_concrete_value_exp({
            parents: [parent],
            type: Shade.Types.float_t,
            value: function() { return "float(" + this.parents[0].glsl_expression() + ")"; },
            is_constant: function() { return this.parents[0].is_constant(); },
            evaluate: Shade.memoize_on_guid_dict(function(cache) {
                var v = this.parents[0].evaluate(cache);
                return Number(v);
            }),
            expression_type: "cast(float)"
        });
    },
    swizzle: function(pattern) {
        function swizzle_pattern_to_indices(pattern) {
            function to_index(v) {
                switch (v.toLowerCase()) {
                case 'r': return 0;
                case 'g': return 1;
                case 'b': return 2;
                case 'a': return 3;
                case 'x': return 0;
                case 'y': return 1;
                case 'z': return 2;
                case 'w': return 3;
                case 's': return 0;
                case 't': return 1;
                case 'p': return 2;
                case 'q': return 3;
                default: throw new Error("invalid swizzle pattern");
                }
            }
            var result = [];
            for (var i=0; i<pattern.length; ++i) {
                result.push(to_index(pattern[i]));
            }
            return result;
        }
        
        var parent = this;
        var indices = swizzle_pattern_to_indices(pattern);
        return Shade._create_concrete_exp( {
            parents: [parent],
            type: parent.type.swizzle(pattern),
            expression_type: "swizzle{" + pattern + "}",
            glsl_expression: function() {
                if (this._must_be_function_call)
                    return this.glsl_name + "()";
                else
                    return this.parents[0].glsl_expression() + "." + pattern; 
            },
            is_constant: Shade.memoize_on_field("_is_constant", function () {
                var that = this;
                return _.all(indices, function(i) {
                    return that.parents[0].element_is_constant(i);
                });
            }),
            constant_value: Shade.memoize_on_field("_constant_value", function() {
                var that = this;
                var ar = _.map(indices, function(i) {
                    return that.parents[0].element_constant_value(i);
                });
                if (ar.length === 1)
                    return ar[0];
                var d = this.type.vec_dimension();
                var ctor = vec[d];
                if (_.isUndefined(ctor))
                    throw new Error("bad vec dimension " + d);
                return ctor.make(ar);
            }),
            evaluate: Shade.memoize_on_guid_dict(function(cache) {
                if (this.is_constant())
                    return this.constant_value();
                if (this.type.is_pod()) {
                    return this.parents[0].element(indices[0]).evaluate(cache);
                } else {
                    var that = this;
                    var ar = _.map(indices, function(index) {
                        return that.parents[0].element(index).evaluate(cache);
                    });
                    var d = this.type.vec_dimension();
                    var ctor = vec[d];
                    if (_.isUndefined(ctor))
                        throw new Error("bad vec dimension " + d);
                    return ctor.make(ar);
                }
            }),
            element: function(i) {
                return this.parents[0].element(indices[i]);
            },
            element_is_constant: Shade.memoize_on_field("_element_is_constant", function(i) {
                return this.parents[0].element_is_constant(indices[i]);
            }),
            element_constant_value: Shade.memoize_on_field("_element_constant_value", function(i) {
                return this.parents[0].element_constant_value(indices[i]);
            }),
            compile: function(ctx) {
                if (this._must_be_function_call) {
                    this.precomputed_value_glsl_name = ctx.request_fresh_glsl_name();
                    ctx.strings.push(this.type.declare(this.precomputed_value_glsl_name), ";\n");
                    ctx.add_initialization(this.precomputed_value_glsl_name + " = " + 
                                           this.parents[0].glsl_expression() + "." + pattern);
                    ctx.value_function(this, this.precomputed_value_glsl_name);
                }
            }
        });
    },
    at: function(index) {
        var parent = this;
        index = Shade.make(index);
        // this "works around" current constant index restrictions in webgl
        // look for it to get broken in the future as this hole is plugged.
        index._must_be_function_call = true;
        if (!index.type.equals(Shade.Types.float_t) &&
            !index.type.equals(Shade.Types.int_t)) {
            throw new Error("at expects int or float, got '" + 
                            index.type.repr() + "' instead");
        }
        return Shade._create_concrete_exp( {
            parents: [parent, index],
            type: parent.type.array_base(),
            expression_type: "index",
            glsl_expression: function() {
                if (this.parents[1].type.is_integral()) {
                    return this.parents[0].glsl_expression() + 
                        "[" + this.parents[1].glsl_expression() + "]"; 
                } else {
                    return this.parents[0].glsl_expression() + 
                        "[int(" + this.parents[1].glsl_expression() + ")]"; 
                }
            },
            is_constant: function() {
                if (!this.parents[1].is_constant())
                    return false;
                var ix = Math.floor(this.parents[1].constant_value());
                return (this.parents[1].is_constant() &&
                        this.parents[0].element_is_constant(ix));
            },
            evaluate: Shade.memoize_on_guid_dict(function(cache) {
                var ix = Math.floor(this.parents[1].evaluate(cache));
                var parent_value = this.parents[0].evaluate();
                return parent_value[ix];
                // return this.parents[0].element(ix).evaluate(cache);
            }),

            element: Shade.memoize_on_field("_element", function(i) {
                // FIXME I suspect that a bug here might still arise
                // out of some interaction between the two conditions
                // described below. The right fix will require rewriting the whole
                // constant-folding system :) so it will be a while.

                var array = this.parents[0], 
                    index = this.parents[1];

                if (!index.is_constant()) {
                    // If index is not constant, then we use the following equation:
                    // element(Array(a_1 .. a_n).at(ix), i) ==
                    // Array(element(a_1, i) .. element(a_n, i)).at(ix)
                    var elts = _.map(array.parents, function(parent) {
                        return parent.element(i);
                    });
                    return Shade.array(elts).at(index);
                }
                var index_value = this.parents[1].constant_value();
                var x = this.parents[0].element(index_value);

                // the reason for the (if x === this) checks here is that sometimes
                // the only appropriate description of an element() of an
                // opaque object (uniforms and attributes, notably) is an at() call.
                // This means that (this.parents[0].element(ix) === this) is
                // sometimes true, and we're stuck in an infinite loop.
                if (x === this) {
                    return x.at(i);
                } else
                    return x.element(i);
            }),
            element_is_constant: Shade.memoize_on_field("_element_is_constant", function(i) {
                if (!this.parents[1].is_constant()) {
                    return false;
                }
                var ix = this.parents[1].constant_value();
                var x = this.parents[0].element(ix);
                if (x === this) {
                    return false;
                } else
                    return x.element_is_constant(i);
            }),
            element_constant_value: Shade.memoize_on_field("_element_constant_value", function(i) {
                var ix = this.parents[1].constant_value();
                var x = this.parents[0].element(ix);
                if (x === this) {
                    throw new Error("internal error: would have gone into an infinite loop here.");
                }
                return x.element_constant_value(i);
            }),
            compile: function() {}
        });
    },
    field: function(field_name) {
        if (!this.type.is_struct()) {
            throw new Error("field() only valid on struct types");
        }
        var index = this.type.field_index[field_name];
        if (_.isUndefined(index)) {
            throw new Error("field " + field_name + " not existent");
        }

        return Shade._create_concrete_value_exp({
            parents: [this],
            type: this.type.fields[field_name],
            expression_type: "struct-accessor{" + field_name + "}",
            value: function() {
                return "(" + this.parents[0].glsl_expression() + "." + field_name + ")";
            },
            evaluate: Shade.memoize_on_guid_dict(function(cache) {
                var struct_value = this.parents[0].evaluate(cache);
                return struct_value[field_name];
            }),
            is_constant: Shade.memoize_on_field("_is_constant", function() {
                // this is conservative for many situations, but hey.
                return this.parents[0].is_constant();
            }),
            element: function(i) {
                return this.at(i);
            }
        });
    },
    _lux_expression: true, // used by lux_typeOf
    expression_type: "other",
    _uniforms: [],

    //////////////////////////////////////////////////////////////////////////

    attribute_buffers: function() {
        return _.flatten(this.sorted_sub_expressions().map(function(v) { 
            return v.expression_type === 'attribute' ? [v] : [];
        }));
    },
    uniforms: function() {
        return _.flatten(this.sorted_sub_expressions().map(function(v) { 
            return v._uniforms; 
        }));
    },

    //////////////////////////////////////////////////////////////////////////
    // simple re-writing of shaders, useful for moving expressions
    // around, such as the things we move around when attributes are 
    // referenced in fragment programs
    // 
    // NB: it's easy to create bad expressions with these.
    //
    // The general rule is that types should be preserved (although
    // that might not *always* be the case)
    find_if: function(check) {
        return _.select(this.sorted_sub_expressions(), check);
    },

    replace_if: function(check, replacement) {
        // this code is not particularly clear, but this is a compiler
        // hot-path, bear with me.
        var subexprs = this.sorted_sub_expressions();
        var replaced_pairs = {};
        function parent_replacement(x) {
            if (!(x.guid in replaced_pairs)) {
                return x;
            } else
                return replaced_pairs[x.guid];
        }
        var latest_replacement, replaced;
        for (var i=0; i<subexprs.length; ++i) {
            var exp = subexprs[i];
            if (check(exp)) {
                latest_replacement = replacement(exp);
                replaced_pairs[exp.guid] = latest_replacement;
            } else {
                replaced = false;
                for (var j=0; j<exp.parents.length; ++j) {
                    if (exp.parents[j].guid in replaced_pairs) {
                        latest_replacement = Shade._create(exp, {
                            parents: _.map(exp.parents, parent_replacement)
                        });
                        replaced_pairs[exp.guid] = latest_replacement;
                        replaced = true;
                        break;
                    }
                }
                if (!replaced) {
                    latest_replacement = exp;
                }
            }
        }
        return latest_replacement;
    },

    //////////////////////////////////////////////////////////////////////////
    // debugging infrastructure

    json: function() {
        function helper_f(node, parents, refs) { return node._json_helper(parents, refs); };
        var refs = Shade.Debug.walk(this, helper_f, helper_f);
        return refs[this.guid];
    },
    _json_helper: Shade.Debug._json_builder(),    
    _json_key: function() { return this.expression_type; },
    
    debug_print: function(do_what) {
        var lst = [];
        var refs = {};
        function _debug_print(which, indent) {
            var i;
            var str = new Array(indent+2).join(" "); // This is python's '" " * indent'
            // var str = "";
            // for (var i=0; i<indent; ++i) { str = str + ' '; }
            if (which.parents.length === 0) 
                lst.push(str + "[" + which.expression_type + ":" + which.guid + "]"
                            // + "[is_constant: " + which.is_constant() + "]"
                            + " ()");
            else {
                lst.push(str + "[" + which.expression_type + ":" + which.guid + "]"
                            // + "[is_constant: " + which.is_constant() + "]"
                            + " (");
                for (i=0; i<which.parents.length; ++i) {
                    if (refs[which.parents[i].guid])
                        lst.push(str + "  {{" + which.parents[i].guid + "}}");
                    else {
                        _debug_print(which.parents[i], indent + 2);
                        refs[which.parents[i].guid] = 1;
                    }
                }
                lst.push(str + ')');
            }
        };
        _debug_print(this, 0);
        do_what = do_what || function(l) {
            return l.join("\n");
        };
        return do_what(lst);
    },

    locate: function(predicate) {
        var sub_exprs = this.sorted_sub_expressions();
        for (var i=0; i<sub_exprs.length; ++i) {
            if (predicate(sub_exprs[i]))
                return sub_exprs[i];
        }
        return undefined;
    },

    //////////////////////////////////////////////////////////////////////////
    // fields
    
    // if stage is "vertex" then this expression will be hoisted to the vertex shader
    stage: null,

    // if has_scope is true, then the expression has its own scope
    // (like for-loops)
    has_scope: false,
    patch_scope: function () {},
    loop_variable_dependencies: Shade.memoize_on_field("_loop_variable_dependencies", function () {
        var parent_deps = _.map(this.parents, function(v) {
            return v.loop_variable_dependencies();
        });
        if (parent_deps.length === 0)
            return [];
        else {
            var result_with_duplicates = parent_deps[0].concat.apply(parent_deps[0], parent_deps.slice(1));
            var guids = [];
            var result = [];
            _.each(result_with_duplicates, function(n) {
                if (!guids[n.guid]) {
                    guids[n.guid] = true;
                    result.push(n);
                }
            });
            return result;
        }
    })
};

_.each(["r", "g", "b", "a",
        "x", "y", "z", "w",
        "s", "t", "p", "q"], function(v) {
            Shade.Exp[v] = function() {
                return this.swizzle(v);
            };
        });

Shade._create_concrete_exp = Shade._create_concrete(Shade.Exp, ["parents", "compile", "type"]);
/*
 * FIXME the webgl compiler seems to be having trouble with the
 * conditional expressions in longer shaders.  Temporarily, then, I
 * will replace all "unconditional" checks with "true". The end effect
 * is that the shader always evaluates potentially unused sides of a
 * conditional expression if they're used in two or more places in
 * the shader.
 */

Shade.ValueExp = Shade._create(Shade.Exp, {
    is_constant: Shade.memoize_on_field("_is_constant", function() {
        return _.all(this.parents, function(v) {
            return v.is_constant();
        });
    }),
    element_is_constant: Shade.memoize_on_field("_element_is_constant", function(i) {
        return this.is_constant();
    }),
    element_constant_value: Shade.memoize_on_field("_element_constant_value", function (i) {
        return this.element(i).constant_value();
    }),
    _must_be_function_call: false,
    glsl_expression: function() {
        var unconditional = true; // see comment on top
        if (this._must_be_function_call) {
            return this.glsl_name + "(" + _.map(this.loop_variable_dependencies(), function(exp) {
                return exp.glsl_name;
            }).join(",") + ")";
        }
        // this.children_count will be undefined if object was built
        // during compilation (lifted operators for structs will do that, for example)
        if (_.isUndefined(this.children_count) || this.children_count <= 1)
            return this.value();
        if (unconditional)
            return this.precomputed_value_glsl_name;
        else
            return this.glsl_name + "()";
    },
    // For types which are not POD, element(i) returns a Shade expression
    // whose value is equivalent to evaluating the i-th element of the
    // expression itself. for example:
    // Shade.add(vec1, vec2).element(0) -> Shade.add(vec1.element(0), vec2.element(0));
    element: function(i) {
        if (this.type.is_pod()) {
            if (i === 0)
                return this;
            else
                throw new Error(this.type.repr() + " is an atomic type, got this: " + i);
        } else {
            this.debug_print();
            throw new Error("Internal error; this should have been overriden.");
        }
    },
    compile: function(ctx) {
        var unconditional = true; // see comment on top
        if (this._must_be_function_call) {
            if (unconditional) {
                if (this.children_count > 1) {
                    this.precomputed_value_glsl_name = ctx.request_fresh_glsl_name();
                    this.scope.add_declaration(this.type.declare(this.precomputed_value_glsl_name));
                    this.scope.add_initialization(this.precomputed_value_glsl_name + " = " + this.value());
                    ctx.value_function(this, this.precomputed_value_glsl_name);
                } else {
                    ctx.value_function(this, this.value());
                }
            } else {
                if (this.children_count > 1) {
                    this.precomputed_value_glsl_name = ctx.request_fresh_glsl_name();
                    this.has_precomputed_value_glsl_name = ctx.request_fresh_glsl_name();
                    this.scope.add_declaration(this.type.declare(this.precomputed_value_glsl_name));
                    this.scope.add_declaration(Shade.Types.bool_t.declare(this.has_precomputed_value_glsl_name));
                    this.scope.add_initialization(this.has_precomputed_value_glsl_name + " = false");

                    ctx.value_function(this, "(" + this.has_precomputed_value_glsl_name + "?"
                                       + this.precomputed_value_glsl_name + ": (("
                                       + this.has_precomputed_value_glsl_name + "=true),("
                                       + this.precomputed_value_glsl_name + "="
                                       + this.value() + ")))");
                } else
                    ctx.value_function(this, this.value());
            }
        } else {
            if (unconditional) {
                if (this.children_count > 1) {
                    this.precomputed_value_glsl_name = ctx.request_fresh_glsl_name();
                    this.scope.add_declaration(this.type.declare(this.precomputed_value_glsl_name));
                    this.scope.add_initialization(this.precomputed_value_glsl_name + " = " + this.value());
                } else {
                    // don't emit anything, all is taken care by glsl_expression()
                }
            } else {
                if (this.children_count > 1) {
                    this.precomputed_value_glsl_name = ctx.request_fresh_glsl_name();
                    this.has_precomputed_value_glsl_name = ctx.request_fresh_glsl_name();
                    this.scope.add_declaration(this.type.declare(this.precomputed_value_glsl_name));
                    this.scope.add_declaration(Shade.Types.bool_t.declare(this.has_precomputed_value_glsl_name));
                    this.scope.add_initialization(this.has_precomputed_value_glsl_name + " = false");
                    ctx.value_function(this, "(" + this.has_precomputed_value_glsl_name + "?"
                                       + this.precomputed_value_glsl_name + ": (("
                                       + this.has_precomputed_value_glsl_name + "=true),("
                                       + this.precomputed_value_glsl_name + "="
                                       + this.value() + ")))");
                } else {
                    // don't emit anything, all is taken care by glsl_expression()
                }
            }
        }
    }, call_operator: function(other) {
        return this.mul(other);
    }
});
Shade._create_concrete_value_exp = Shade._create_concrete(Shade.ValueExp, ["parents", "type", "value"]);
Shade.swizzle = function(exp, pattern)
{
    return Shade(exp).swizzle(pattern);
};
// Shade.constant creates a constant value in the Shade language.
// 
// This value can be one of:
// - a single float: 
//    Shade.constant(1)
//    Shade.constant(3.0, Shade.Types.float_t)
// - a single integer:
//    Shade.constant(1, Shade.Types.int_t)
// - a boolean:
//    Shade.constant(false);
// - a GLSL vec2, vec3 or vec4 (of floating point values):
//    Shade.constant(2, vec.make([1, 2]));
// - a GLSL matrix of dimensions 2x2, 3x3, 4x4 (Lux currently does not support GLSL rectangular matrices):
//    Shade.constant(2, mat.make([1, 0, 0, 1]));
// - an array
// - a struct

Shade.constant = function(v, type)
{
    var mat_length_to_dimension = {16: 4, 9: 3, 4: 2, 1: 1};

    var constant_tuple_fun = function(type, args)
    {
        function to_glsl(type, args) {
            // this seems incredibly ugly, but we need something
            // like it, so that numbers are appropriately promoted to floats
            // in GLSL's syntax.

            var string_args = _.map(args, function(arg) {
                var v = String(arg);
                if (lux_typeOf(arg) === "number" && v.indexOf(".") === -1) {
                    return v + ".0";
                } else
                    return v;
            });
            return type + '(' + _.toArray(string_args).join(', ') + ')';
        }

        function matrix_row(i) {
            var sz = type.array_size();
            var result = [];
            for (var j=0; j<sz; ++j) {
                result.push(args[i + j*sz]);
            }
            return result;
        }

        return Shade._create_concrete_exp( {
            glsl_expression: function(glsl_name) {
                return to_glsl(this.type.repr(), args);
            },
            expression_type: "constant{" + args + "}",
            is_constant: function() { return true; },
            element: Shade.memoize_on_field("_element", function(i) {
                if (this.type.is_pod()) {
                    if (i === 0)
                        return this;
                    else
                        throw new Error(this.type.repr() + " is an atomic type, got this: " + i);
                } else if (this.type.is_vec()) {
                    return Shade.constant(args[i]);
                } else {
                    return Shade.vec.apply(matrix_row(i));
                }
            }),
            element_is_constant: function(i) {
                return true;
            },
            element_constant_value: Shade.memoize_on_field("_element_constant_value", function(i) {
                if (this.type.equals(Shade.Types.float_t)) {
                    if (i === 0)
                        return args[0];
                    else
                        throw new Error("float is an atomic type");
                } if (this.type.is_vec()) {
                    return args[i];
                }
                return vec[this.type.array_size()].make(matrix_row(i));
            }),
            evaluate: Shade.memoize_on_guid_dict(function(cache) {
                // FIXME boolean_vector
                if (this.type.is_pod())
                    return args[0];
                if (this.type.equals(Shade.Types.vec2) ||
                    this.type.equals(Shade.Types.vec3) ||
                    this.type.equals(Shade.Types.vec4))
                    return vec[args.length].make(args);
                if (this.type.equals(Shade.Types.mat2) ||
                    this.type.equals(Shade.Types.mat3) ||
                    this.type.equals(Shade.Types.mat4))
                    return mat[mat_length_to_dimension[args.length]].make(args);
                else
                    throw new Error("internal error: constant of unknown type");
            }),
            compile: function(ctx) {},
            parents: [],
            type: type,

            //////////////////////////////////////////////////////////////////
            // debugging

            _json_helper: Shade.Debug._json_builder("constant", function(obj) {
                obj.values = args;
                return obj;
            })
        });
    };

    // FIXME refactor this since type_of result is now a Shade.Types.*
    var t = Shade.Types.type_of(v);
    var d, computed_t;
    if (t.equals(Shade.Types.float_t)) {
        if (type && !(type.equals(Shade.Types.float_t) ||
                      type.equals(Shade.Types.int_t))) {
            throw new Error("expected specified type for numbers to be float or int," +
                   " got " + type.repr() + " instead.");
        }
        return constant_tuple_fun(type || Shade.Types.float_t, [v]);
    } else if (t.equals(Shade.Types.bool_t)) {
        if (type && !type.equals(Shade.Types.bool_t))
            throw new Error("boolean constants cannot be interpreted as " + 
                   type.repr());
        return constant_tuple_fun(Shade.Types.bool_t, [v]);
    } else if (t.repr().substr(0,3) === 'vec') {
        d = v.length;
        if (d < 2 && d > 4)
            throw new Error("invalid length for constant vector: " + v);
        var el_ts = _.map(v, function(t) { return lux_typeOf(t); });
        if (!_.all(el_ts, function(t) { return t === el_ts[0]; })) {
            throw new Error("not all constant params have the same types");
        }
        if (el_ts[0] === "number") {
            computed_t = Shade.Types['vec' + d];
            if (type && !computed_t.equals(type)) {
                throw new Error("passed constant must have type " + computed_t.repr()
                    + ", but was request to have incompatible type " 
                    + type.repr());
            }
            return constant_tuple_fun(computed_t, v);
        }
        else
            throw new Error("bad datatype for constant: " + el_ts[0]);
    } else if (t.repr().substr(0,3) === 'mat') {
        d = mat_length_to_dimension[v.length];
        computed_t = Shade.Types['mat' + d];
        if (type && !computed_t.equals(type)) {
            throw new Error("passed constant must have type " + computed_t.repr()
                            + ", but was requested to have incompatible type " 
                            + type.repr());
        }
        return constant_tuple_fun(computed_t, v);
    } else if (type.is_struct()) {
        var obj = {};
        _.each(v, function(val, k) {
            obj[k] = Shade.constant(val, type.fields[k]);
        });
        return Shade.struct(obj);
    } else {
        throw new Error("type error: constant should be bool, number, vector, matrix, array or struct. got " + t
                        + " instead");
    }
    throw new Error("internal error: Shade.Types.type_of returned bogus value");
};

Shade.as_int = function(v) { return Shade.make(v).as_int(); };
Shade.as_bool = function(v) { return Shade.make(v).as_bool(); };
Shade.as_float = function(v) { return Shade.make(v).as_float(); };

// Shade.array denotes an array of Lux values of the same type:
//    Shade.array([2, 3, 4, 5, 6]);

Shade.array = function(v)
{
    var t = lux_typeOf(v);
    if (t !== 'array')
        throw new Error("type error: need array");

    var new_v = v.map(Shade.make);
    var array_size = new_v.length;
    if (array_size === 0) {
        throw new Error("array constant must be non-empty");
    }

    var new_types = new_v.map(function(t) { return t.type; });
    var array_type = Shade.Types.array(new_types[0], array_size);
    if (_.any(new_types, function(t) { return !t.equals(new_types[0]); })) {
        throw new Error("array elements must have identical types");
    }
    return Shade._create_concrete_exp( {
        parents: new_v,
        type: array_type,
        array_element_type: new_types[0],
        expression_type: "constant", // FIXME: is there a reason this is not "array"?

        evaluate: Shade.memoize_on_guid_dict(function(cache) {
            return _.map(this.parents, function(e) {
                return e.evaluate(cache);
            });
        }),
        
        glsl_expression: function() { return this.glsl_name; },
        compile: function (ctx) {
            this.array_initializer_glsl_name = ctx.request_fresh_glsl_name();
            ctx.strings.push(this.type.declare(this.glsl_name), ";\n");
            ctx.strings.push("void", this.array_initializer_glsl_name, "(void) {\n");
            for (var i=0; i<this.parents.length; ++i) {
                ctx.strings.push("    ", this.glsl_name, "[", i, "] =",
                                 this.parents[i].glsl_expression(), ";\n");
            }
            ctx.strings.push("}\n");
            ctx.add_initialization(this.array_initializer_glsl_name + "()");
        },
        is_constant: function() { return false; }, 
        element: function(i) {
            return this.parents[i];
        },
        element_is_constant: function(i) {
            return this.parents[i].is_constant();
        },
        element_constant_value: function(i) {
            return this.parents[i].constant_value();
        },
        locate: function(target, xform) {
            var that = this;
            xform = xform || function(x) { return x; };
            return Shade.locate(function(i) { return xform(that.at(i.as_int())); }, target, 0, array_size-1);
        },

        _json_key: function() { return "array"; }
    });
};
// Shade.struct denotes a heterogeneous structure of Shade values:
//   Shade.struct({foo: Shade.vec(1,2,3), bar: Shade.struct({baz: 1, bah: false})});

Shade.struct = function(obj)
{
    var vs = _.map(obj, function(v) { return Shade.make(v); });
    var ks = _.keys(obj);
    var types = _.map(vs, function(v) { return v.type; });
    var t = {};
    _.each(ks, function(k, i) {
        t[k] = types[i];
    });
    var struct_type = Shade.Types.struct(t), new_vs = [], new_ks = [];

    // javascript object order is arbitrary;
    // make sure structs follow the type field order, which is unique
    _.each(struct_type.field_index, function(index, key) {
        var old_index = ks.indexOf(key);
        new_vs[index] = vs[old_index];
        new_ks[index] = key;
    });
    vs = new_vs;
    ks = new_ks;
    
    var result = Shade._create_concrete_value_exp({
        parents: vs,
        fields: ks,
        type: struct_type,
        expression_type: "struct",
        value: function() {
            return [this.type.internal_type_name, "(",
                    this.parents.map(function(t) {
                        return t.glsl_expression();
                    }).join(", "),
                    ")"].join(" ");
        },
        evaluate: Shade.memoize_on_guid_dict(function(cache) {
            var result = {};
            var that = this;
            _.each(this.parents, function(v, i) {
                result[that.fields[i]] = v.evaluate(cache);
            });
            return result;
        }),
        field: function(field_name) {
            var index = this.type.field_index[field_name];
            if (_.isUndefined(index)) {
                throw new Error("field " + field_name + " not existent");
            }

            /* Since field_name is always an immediate string, 
             it will never need to be "computed" on a shader.            
             This means that in this case, its value can always
             be resolved in compile time and 
             val(constructor(foo=bar).foo) is always val(bar).
             */

            return this.parents[index];
        },
        call_operator: function(v) {
            return this.field(v);
        },
        _json_helper: Shade.Debug._json_builder("struct", function(obj) {
            obj.fields = ks;
            return obj;
        })
    });

    // _.each(ks, function(k) {
    //     // I can't use _.has because result is actually a javascript function..
    //     if (!_.isUndefined(result[k])) {
    //         console.log("Warning: Field",k,"is reserved. JS struct notation (a.b) will not be usable");
    //     } else
    //         result[k] = result.field(k);
    // });
    return result;
};

/* Shade.set is essentially an internal method for Shade. Don't use it
   unless you know exactly what you're doing.
 */

Shade.set = function(exp, name)
{
    exp = Shade(exp);
    var type = exp.type;
    return Shade._create_concrete_exp({
        expression_type: "set",
        compile: function(ctx) {
            if ((name === "gl_FragColor" ||
                 (name.substring(0, 11) === "gl_FragData")) &&
                ctx.compile_type !== Shade.FRAGMENT_PROGRAM_COMPILE) {
                throw new Error("gl_FragColor and gl_FragData assignment"
                       + " only allowed on fragment shaders");
            }
            if ((name === "gl_Position" ||
                 name === "gl_PointSize") &&
                ctx.compile_type !== Shade.VERTEX_PROGRAM_COMPILE) {
                throw new Error("gl_Position and gl_PointSize assignment "
                       + "only allowed on vertex shaders");
            }
            if ((ctx.compile_type !== Shade.VERTEX_PROGRAM_COMPILE) &&
                (name !== "gl_FragColor") &&
                (name.substring(0, 11) !== "gl_FragData")) {
                throw new Error("the only allowed output variables on a fragment"
                       + " shader are gl_FragColor and gl_FragData[]");
            }
            if (name !== "gl_FragColor" &&
                name !== "gl_Position" &&
                name !== "gl_PointSize" &&
                name.substring(0, 11) !== "gl_FragData") {
                ctx.declare_varying(name, type);
            }
            ctx.void_function(this, "(", name, "=", this.parents[0].glsl_expression(), ")");
        },
        type: Shade.Types.void_t,
        parents: [exp],
        evaluate: Shade.memoize_on_guid_dict(function(cache) {
            return this.parents[0].evaluate(cache);
        })
    });
};
Shade.parameter = function(type, v)
{
    var call_lookup = [
        [Shade.Types.float_t, "uniform1f"],
        [Shade.Types.int_t, "uniform1i"],
        [Shade.Types.bool_t, "uniform1i"],
        [Shade.Types.sampler2D, "uniform1i"],
        [Shade.Types.vec2, "uniform2fv"],
        [Shade.Types.vec3, "uniform3fv"],
        [Shade.Types.vec4, "uniform4fv"],
        [Shade.Types.mat2, "uniformMatrix2fv"],
        [Shade.Types.mat3, "uniformMatrix3fv"],
        [Shade.Types.mat4, "uniformMatrix4fv"]
    ];

    var uniform_name = Shade.unique_name();
    if (_.isUndefined(type)) throw new Error("parameter requires type");
    if (typeof type === 'string') type = Shade.Types[type];
    if (_.isUndefined(type)) throw new Error("parameter requires valid type");

    // the local variable value stores the actual value of the
    // parameter to be used by the GLSL uniform when it is set.
    var value;

    var call = _.detect(call_lookup, function(p) { return type.equals(p[0]); });
    if (!_.isUndefined(call)) {
        call = call[1];
    } else {
        throw new Error("Unsupported type " + type.repr() + " for parameter.");
    }
    var result = Shade._create_concrete_exp({
        parents: [],
        watchers: [],
        type: type,
        expression_type: 'parameter',
        glsl_expression: function() {
            if (this._must_be_function_call) {
                return this.glsl_name + "()";
            } else
                return uniform_name; 
        },
        evaluate: function() {
            return value;
        },
        element: Shade.memoize_on_field("_element", function(i) {
            if (this.type.is_pod()) {
                if (i === 0)
                    return this;
                else
                    throw new Error(this.type.repr() + " is an atomic type");
            } else
                return this.at(i);
        }),
        compile: function(ctx) {
            ctx.declare_uniform(uniform_name, this.type);
            if (this._must_be_function_call) {
                this.precomputed_value_glsl_name = ctx.request_fresh_glsl_name();
                ctx.strings.push(this.type.declare(this.precomputed_value_glsl_name), ";\n");
                ctx.add_initialization(this.precomputed_value_glsl_name + " = " + uniform_name);
                ctx.value_function(this, this.precomputed_value_glsl_name);
            }
        },
        set: function(v) {
            // Ideally, we'd like to do type checking here, but I'm concerned about
            // performance implications. setting a uniform might be a hot path
            // then again, Shade.Types.type_of is unlikely to be particularly fast.
            // FIXME check performance
            var t = Shade.Types.type_of(v);
            if (t === "shade_expression")
                v = v.evaluate();
            value = v;
            if (this._lux_active_uniform) {
                this._lux_active_uniform(v);
            }
            _.each(this.watchers, function(f) { f(v); });
        },
        get: function(v) {
            return value;
        },
        watch: function(callback) {
            this.watchers.push(callback);
        },
        unwatch: function(callback) {
            this.watchers.splice(this.watchers.indexOf(callback), 1);
        },
        uniform_call: call,
        uniform_name: uniform_name,

        //////////////////////////////////////////////////////////////////////
        // debugging

        _json_helper: Shade.Debug._json_builder("parameter", function(obj) {
            obj.parameter_type = type.repr();
            return obj;
        })
    });
    result._uniforms = [result];
    result.set(v);
    return result;
};
Shade.sampler2D_from_texture = function(texture)
{
    return texture._shade_expression || function() {
        var result = Shade.parameter("sampler2D");
        result.set(texture);
        texture._shade_expression = result;
        // FIXME: What if the same texture is bound to many samplers?!
        return result;
    }();
};

Shade.attribute_from_buffer = function(buffer)
{
    return buffer._shade_expression || function() {
        var itemTypeMap = [ undefined, Shade.Types.float_t, Shade.Types.vec2, Shade.Types.vec3, Shade.Types.vec4 ];
        var itemType = itemTypeMap[buffer.itemSize];
        var result = Shade.attribute(itemType);
        buffer._shade_expression = result;
        result.set(buffer);
        return result;
    }();
};

Shade.attribute = function(type)
{
    var name = Shade.unique_name();
    if (_.isUndefined(type)) throw new Error("attribute requires type");
    if (typeof type === 'string') type = Shade.Types[type];
    if (_.isUndefined(type)) throw new Error("attribute requires valid type");
    var bound_buffer;

    return Shade._create_concrete_exp( {
        parents: [],
        type: type,
        expression_type: 'attribute',
        element: Shade.memoize_on_field("_element", function(i) {
            if (this.type.equals(Shade.Types.float_t)) {
                if (i === 0)
                    return this;
                else
                    throw new Error("float is an atomic type");
            } else
                return this.at(i);
        }),
        evaluate: function() {
            throw new Error("client-side evaluation of attributes is currently unsupported");
        },
        glsl_expression: function() { 
            if (this._must_be_function_call) {
                return this.glsl_name + "()";
            } else
                return name; 
        },
        compile: function(ctx) {
            ctx.declare_attribute(name, this.type);
            if (this._must_be_function_call) {
                this.precomputed_value_glsl_name = ctx.request_fresh_glsl_name();
                ctx.strings.push(this.type.declare(this.precomputed_value_glsl_name), ";\n");
                ctx.add_initialization(this.precomputed_value_glsl_name + " = " + name);
                ctx.value_function(this, this.precomputed_value_glsl_name);
            }
        },
        get: function() {
            return bound_buffer;
        },
        set: function(buffer) {
            // FIXME buffer typechecking
            var batch_opts = Lux.get_current_batch_opts();
            if (batch_opts.program && (name in batch_opts.program)) {
                var ctx = batch_opts._ctx;
                buffer.bind(batch_opts.program[name]);
            }
            bound_buffer = buffer;
        },
        _attribute_name: name,

        //////////////////////////////////////////////////////////////////////
        // debugging

        _json_helper: Shade.Debug._json_builder("attribute", function(obj) {
            obj.attribute_type = type.repr();
            return obj;
        })

    });
};
Shade.varying = function(name, type)
{
    if (_.isUndefined(type)) throw new Error("varying requires type");
    if (lux_typeOf(type) === 'string') type = Shade.Types[type];
    if (_.isUndefined(type)) throw new Error("varying requires valid type");
    var allowed_types = [
        Shade.Types.float_t,
        Shade.Types.vec2,
        Shade.Types.vec3,
        Shade.Types.vec4,
        Shade.Types.mat2,
        Shade.Types.mat3,
        Shade.Types.mat4
    ];
    if (!_.any(allowed_types, function(t) { return t.equals(type); })) {
        throw new Error("varying does not support type '" + type.repr() + "'");
    }
    return Shade._create_concrete_exp( {
        parents: [],
        type: type,
        expression_type: 'varying',
        _varying_name: name,
        element: Shade.memoize_on_field("_element", function(i) {
            if (this.type.is_pod()) {
                if (i === 0)
                    return this;
                else
                    throw new Error(this.type.repr() + " is an atomic type");
            } else
                return this.at(i);
        }),
        glsl_expression: function() { 
            if (this._must_be_function_call) {
                return this.glsl_name + "()";
            } else
                return name; 
        },
        evaluate: function() {
            throw new Error("evaluate unsupported for varying expressions");
        },
        compile: function(ctx) {
            ctx.declare_varying(name, this.type);
            if (this._must_be_function_call) {
                this.precomputed_value_glsl_name = ctx.request_fresh_glsl_name();
                ctx.strings.push(this.type.declare(this.precomputed_value_glsl_name), ";\n");
                ctx.add_initialization(this.precomputed_value_glsl_name + " = " + name);
                ctx.value_function(this, this.precomputed_value_glsl_name);
            }
        },

        //////////////////////////////////////////////////////////////////////
        // debugging

        _json_helper: Shade.Debug._json_builder("varying", function(obj) {
            obj.varying_type = type.repr();
            obj.varying_name = name;
            return obj;
        })
    });
};

Shade.fragCoord = function() {
    return Shade._create_concrete_exp({
        expression_type: "builtin_input{gl_FragCoord}",
        parents: [],
        type: Shade.Types.vec4,
        glsl_expression: function() { return "gl_FragCoord"; },
        evaluate: function() {
            throw new Error("evaluate undefined for fragCoord");
        },
        compile: function(ctx) {
        },
        json_key: function() { return "fragCoord"; }
    });
};
Shade.pointCoord = function() {
    return Shade._create_concrete_exp({
        expression_type: "builtin_input{gl_PointCoord}",
        parents: [],
        type: Shade.Types.vec2,
        glsl_expression: function() { return "gl_PointCoord"; },
        compile: function(ctx) {
        },
        evaluate: function() {
            throw new Error("evaluate undefined for pointCoord");
        },
        _json_key: function() { return 'pointCoord'; }
    });
};
Shade.round_dot = function(color) {
    var outside_dot = Shade.pointCoord().sub(Shade.vec(0.5, 0.5)).norm().gt(0.25);
    return Shade.make(color).discard_if(outside_dot);
};
(function() {

var operator = function(exp1, exp2, 
                        operator_name, type_resolver,
                        evaluator,
                        element_evaluator,
                        shade_name)
{
    var resulting_type = type_resolver(exp1.type, exp2.type);
    return Shade._create_concrete_value_exp( {
        parents: [exp1, exp2],
        type: resulting_type,
        expression_type: "operator" + operator_name,
        _json_key: function() { return shade_name; },
        value: function () {
            var p1 = this.parents[0], p2 = this.parents[1];
            if (this.type.is_struct()) {
                return "(" + this.type.repr() + "(" +
                    _.map(this.type.fields, function (v,k) {
                        return p1.field(k).glsl_expression() + " " + operator_name + " " +
                            p2.field(k).glsl_expression();
                    }).join(", ") + "))";
            } else {
                return "(" + this.parents[0].glsl_expression() + " " + operator_name + " " +
                    this.parents[1].glsl_expression() + ")";
            }
        },
        evaluate: Shade.memoize_on_guid_dict(function(cache) {
            return evaluator(this, cache);
        }),
        element: Shade.memoize_on_field("_element", function(i) {
            return element_evaluator(this, i);
        }),
        element_constant_value: Shade.memoize_on_field("_element_constant_value", function(i) {
            return this.element(i).constant_value();
        }),
        element_is_constant: Shade.memoize_on_field("_element_is_constant", function(i) {
            return this.element(i).is_constant();
        })
    });
};

Shade.add = function() {
    if (arguments.length === 0) throw new Error("add needs at least one argument");
    if (arguments.length === 1) return arguments[0];
    function add_type_resolver(t1, t2) {
        var type_list = [
            [Shade.Types.vec4, Shade.Types.vec4, Shade.Types.vec4],
            [Shade.Types.mat4, Shade.Types.mat4, Shade.Types.mat4],
            [Shade.Types.float_t, Shade.Types.float_t, Shade.Types.float_t],
            [Shade.Types.vec4, Shade.Types.float_t, Shade.Types.vec4],
            [Shade.Types.float_t, Shade.Types.vec4, Shade.Types.vec4],
            [Shade.Types.mat4, Shade.Types.float_t, Shade.Types.mat4],
            [Shade.Types.float_t, Shade.Types.mat4, Shade.Types.mat4],

            [Shade.Types.vec3, Shade.Types.vec3, Shade.Types.vec3],
            [Shade.Types.mat3, Shade.Types.mat3, Shade.Types.mat3],
            [Shade.Types.float_t, Shade.Types.float_t, Shade.Types.float_t],
            [Shade.Types.vec3, Shade.Types.float_t, Shade.Types.vec3],
            [Shade.Types.float_t, Shade.Types.vec3, Shade.Types.vec3],
            [Shade.Types.mat3, Shade.Types.float_t, Shade.Types.mat3],
            [Shade.Types.float_t, Shade.Types.mat3, Shade.Types.mat3],

            [Shade.Types.vec2, Shade.Types.vec2, Shade.Types.vec2],
            [Shade.Types.mat2, Shade.Types.mat2, Shade.Types.mat2],
            [Shade.Types.float_t, Shade.Types.float_t, Shade.Types.float_t],
            [Shade.Types.vec2, Shade.Types.float_t, Shade.Types.vec2],
            [Shade.Types.float_t, Shade.Types.vec2, Shade.Types.vec2],
            [Shade.Types.mat2, Shade.Types.float_t, Shade.Types.mat2],
            [Shade.Types.float_t, Shade.Types.mat2, Shade.Types.mat2],
            
            [Shade.Types.int_t, Shade.Types.int_t, Shade.Types.int_t]
        ];
        for (var i=0; i<type_list.length; ++i)
            if (t1.equals(type_list[i][0]) &&
                t2.equals(type_list[i][1]))
                return type_list[i][2];

        // if t1 and t2 are the same struct and all fields admit
        // addition, then a+b is field-wise a+b
        if (t1.is_struct() && t2.is_struct() && t1.equals(t2) &&
            _.all(t1.fields, function(v, k) {
                try {
                    add_type_resolver(v, v);
                    return true;
                } catch (e) {
                    return false;
                }
            })) {
            return t1;
        }
        throw new Error("type mismatch on add: unexpected types  '"
                   + t1.repr() + "' and '" + t2.repr() + "'.");
    }
    var current_result = Shade.make(arguments[0]);
    function evaluator(exp, cache) {
        var exp1 = exp.parents[0], exp2 = exp.parents[1];
        var vt;
        if (exp1.type.is_vec())
            vt = vec[exp1.type.vec_dimension()];
        else if (exp2.type.is_vec())
            vt = vec[exp2.type.vec_dimension()];
        var v1 = exp1.evaluate(cache), v2 = exp2.evaluate(cache);
        if (exp1.type.equals(Shade.Types.int_t) && 
            exp2.type.equals(Shade.Types.int_t))
            return v1 + v2;
        if (exp1.type.equals(Shade.Types.float_t) &&
            exp2.type.equals(Shade.Types.float_t))
            return v1 + v2;
        if (exp2.type.equals(Shade.Types.float_t))
            return vt.map(v1, function(x) { 
                return x + v2;
            });
        if (exp1.type.equals(Shade.Types.float_t))
            return vt.map(v2, function(x) {
                return v1 + x;
            });
        if (vt) {
            return vt.plus(v1, v2);
        } else {
            if (!exp1.type.is_struct())
                throw new Error("internal error, was expecting a struct here");
            var s = {};
            _.each(v1, function(v, k) {
                s[k] = evaluator(Shade.add(exp1.field(k), exp2.field(k)));
            });
            return s;
        }
    };
    function element_evaluator(exp, i) {
        var e1 = exp.parents[0], e2 = exp.parents[1];
        var v1, v2;
        var t1 = e1.type, t2 = e2.type;
        if (t1.is_pod() && t2.is_pod()) {
            if (i === 0)
                return exp;
            else
                throw new Error("i > 0 in pod element");
        }
        if (e1.type.is_vec() || e1.type.is_mat())
            v1 = e1.element(i);
        else
            v1 = e1;
        if (e2.type.is_vec() || e2.type.is_vec())
            v2 = e2.element(i);
        else
            v2 = e2;
        return operator(v1, v2, "+", add_type_resolver, evaluator, element_evaluator, "add");
    }
    for (var i=1; i<arguments.length; ++i) {
        current_result = operator(current_result, Shade.make(arguments[i]),
                                  "+", add_type_resolver, evaluator,
                                  element_evaluator, "add");
    }
    return current_result;
};

Shade.sub = function() {
    if (arguments.length === 0) throw new Error("sub needs at least two arguments");
    if (arguments.length === 1) throw new Error("unary minus unimplemented");
    function sub_type_resolver(t1, t2) {
        var type_list = [
            [Shade.Types.vec4, Shade.Types.vec4, Shade.Types.vec4],
            [Shade.Types.mat4, Shade.Types.mat4, Shade.Types.mat4],
            [Shade.Types.float_t, Shade.Types.float_t, Shade.Types.float_t],
            [Shade.Types.vec4, Shade.Types.float_t, Shade.Types.vec4],
            [Shade.Types.float_t, Shade.Types.vec4, Shade.Types.vec4],
            [Shade.Types.mat4, Shade.Types.float_t, Shade.Types.mat4],
            [Shade.Types.float_t, Shade.Types.mat4, Shade.Types.mat4],

            [Shade.Types.vec3, Shade.Types.vec3, Shade.Types.vec3],
            [Shade.Types.mat3, Shade.Types.mat3, Shade.Types.mat3],
            [Shade.Types.float_t, Shade.Types.float_t, Shade.Types.float_t],
            [Shade.Types.vec3, Shade.Types.float_t, Shade.Types.vec3],
            [Shade.Types.float_t, Shade.Types.vec3, Shade.Types.vec3],
            [Shade.Types.mat3, Shade.Types.float_t, Shade.Types.mat3],
            [Shade.Types.float_t, Shade.Types.mat3, Shade.Types.mat3],

            [Shade.Types.vec2, Shade.Types.vec2, Shade.Types.vec2],
            [Shade.Types.mat2, Shade.Types.mat2, Shade.Types.mat2],
            [Shade.Types.float_t, Shade.Types.float_t, Shade.Types.float_t],
            [Shade.Types.vec2, Shade.Types.float_t, Shade.Types.vec2],
            [Shade.Types.float_t, Shade.Types.vec2, Shade.Types.vec2],
            [Shade.Types.mat2, Shade.Types.float_t, Shade.Types.mat2],
            [Shade.Types.float_t, Shade.Types.mat2, Shade.Types.mat2],
            
            [Shade.Types.int_t, Shade.Types.int_t, Shade.Types.int_t]
        ];
        for (var i=0; i<type_list.length; ++i)
            if (t1.equals(type_list[i][0]) &&
                t2.equals(type_list[i][1]))
                return type_list[i][2];
        // if t1 and t2 are the same struct and all fields admit
        // subtraction, then a-b is field-wise a-b
        if (t1.is_struct() && t2.is_struct() && t1.equals(t2) &&
            _.all(t1.fields, function(v, k) {
                try {
                    sub_type_resolver(v, v);
                    return true;
                } catch (e) {
                    return false;
                }
            })) {
            return t1;
        }
        throw new Error("type mismatch on sub: unexpected types  '"
                   + t1.repr() + "' and '" + t2.repr() + "'.");
    }
    function evaluator(exp, cache) {
        var exp1 = exp.parents[0], exp2 = exp.parents[1];
        var vt;
        if (exp1.type.is_vec())
            vt = vec[exp1.type.vec_dimension()];
        else if (exp2.type.is_vec())
            vt = vec[exp2.type.vec_dimension()];
        var v1 = exp1.evaluate(cache), v2 = exp2.evaluate(cache);
        if (exp1.type.equals(Shade.Types.int_t) && 
            exp2.type.equals(Shade.Types.int_t))
            return v1 - v2;
        if (exp1.type.equals(Shade.Types.float_t) &&
            exp2.type.equals(Shade.Types.float_t))
            return v1 - v2;
        if (exp2.type.equals(Shade.Types.float_t))
            return vt.map(v1, function(x) { 
                return x - v2; 
            });
        if (exp1.type.equals(Shade.Types.float_t))
            return vt.map(v2, function(x) {
                return v1 - x;
            });
        return vt.minus(v1, v2);
    }
    function element_evaluator(exp, i) {
        var e1 = exp.parents[0], e2 = exp.parents[1];
        var v1, v2;
        var t1 = e1.type, t2 = e2.type;
        if (t1.is_pod() && t2.is_pod()) {
            if (i === 0)
                return exp;
            else
                throw new Error("i > 0 in pod element");
        }
        if (e1.type.is_vec() || e1.type.is_mat())
            v1 = e1.element(i);
        else
            v1 = e1;
        if (e2.type.is_vec() || e2.type.is_vec())
            v2 = e2.element(i);
        else
            v2 = e2;
        return operator(v1, v2, "-", sub_type_resolver, evaluator, element_evaluator, "sub");
    }
    var current_result = Shade.make(arguments[0]);
    for (var i=1; i<arguments.length; ++i) {
        current_result = operator(current_result, Shade.make(arguments[i]),
                                  "-", sub_type_resolver, evaluator,
                                  element_evaluator, "sub");
    }
    return current_result;
};

Shade.div = function() {
    if (arguments.length === 0) throw new Error("div needs at least two arguments");
    function div_type_resolver(t1, t2) {
        if (_.isUndefined(t1))
            throw new Error("internal error: t1 multiplication with undefined type");
        if (_.isUndefined(t2))
            throw new Error("internal error: t2 multiplication with undefined type");
        var type_list = [
            [Shade.Types.vec4, Shade.Types.vec4, Shade.Types.vec4],
            [Shade.Types.mat4, Shade.Types.mat4, Shade.Types.mat4],
            [Shade.Types.float_t, Shade.Types.float_t, Shade.Types.float_t],
            [Shade.Types.vec4, Shade.Types.float_t, Shade.Types.vec4],
            [Shade.Types.float_t, Shade.Types.vec4, Shade.Types.vec4],
            [Shade.Types.mat4, Shade.Types.float_t, Shade.Types.mat4],
            [Shade.Types.float_t, Shade.Types.mat4, Shade.Types.mat4],

            [Shade.Types.vec3, Shade.Types.vec3, Shade.Types.vec3],
            [Shade.Types.mat3, Shade.Types.mat3, Shade.Types.mat3],
            [Shade.Types.float_t, Shade.Types.float_t, Shade.Types.float_t],
            [Shade.Types.vec3, Shade.Types.float_t, Shade.Types.vec3],
            [Shade.Types.float_t, Shade.Types.vec3, Shade.Types.vec3],
            [Shade.Types.mat3, Shade.Types.float_t, Shade.Types.mat3],
            [Shade.Types.float_t, Shade.Types.mat3, Shade.Types.mat3],

            [Shade.Types.vec2, Shade.Types.vec2, Shade.Types.vec2],
            [Shade.Types.mat2, Shade.Types.mat2, Shade.Types.mat2],
            [Shade.Types.float_t, Shade.Types.float_t, Shade.Types.float_t],
            [Shade.Types.vec2, Shade.Types.float_t, Shade.Types.vec2],
            [Shade.Types.float_t, Shade.Types.vec2, Shade.Types.vec2],
            [Shade.Types.mat2, Shade.Types.float_t, Shade.Types.mat2],
            [Shade.Types.float_t, Shade.Types.mat2, Shade.Types.mat2],

            [Shade.Types.int_t, Shade.Types.int_t, Shade.Types.int_t]
        ];
        for (var i=0; i<type_list.length; ++i)
            if (t1.equals(type_list[i][0]) &&
                t2.equals(type_list[i][1]))
                return type_list[i][2];
        throw new Error("type mismatch on div: unexpected types '"
                   + t1.repr() + "' and '" + t2.repr() + "'");
    }
    function evaluator(exp, cache) {
        var exp1 = exp.parents[0];
        var exp2 = exp.parents[1];
        var v1 = exp1.evaluate(cache);
        var v2 = exp2.evaluate(cache);
        var vt, mt;
        if (exp1.type.is_array()) {
            vt = vec[exp1.type.array_size()];
            mt = mat[exp1.type.array_size()];
        } else if (exp2.type.is_array()) {
            vt = vec[exp2.type.array_size()];
            mt = mat[exp2.type.array_size()];
        }
        var t1 = Shade.Types.type_of(v1), t2 = Shade.Types.type_of(v2);
        var k1 = t1.is_vec() ? "vector" :
                 t1.is_mat() ? "matrix" :
                 t1.is_pod() ? "number" : "BAD";
        var k2 = t2.is_vec() ? "vector" :
                 t2.is_mat() ? "matrix" :
                 t2.is_pod() ? "number" : "BAD";
        var dispatch = {
            number: { number: function (x, y) { 
                                  if (exp1.type.equals(Shade.Types.int_t))
                                      return ~~(x / y);
                                  else
                                      return x / y;
                              },
                      vector: function (x, y) { 
                          return vt.map(y, function(v) {
                              return x/v;
                          });
                      },
                      matrix: function (x, y) { 
                          return mt.map(y, function(v) {
                              return x/v;
                          });
                      }
                    },
            vector: { number: function (x, y) { return vt.scaling(x, 1/y); },
                      vector: function (x, y) { 
                          return vt.map(y, function(v,i) {
                              return x[i]/v;
                          });
                      },
                      matrix: function (x, y) {
                          throw new Error("internal error, can't evaluate vector/matrix");
                      }
                    },
            matrix: { number: function (x, y) { return mt.scaling(x, 1/y); },
                      vector: function (x, y) { 
                          throw new Error("internal error, can't evaluate matrix/vector");
                      },
                      matrix: function (x, y) { 
                          throw new Error("internal error, can't evaluate matrix/matrix");
                      }
                    }
        };
        return dispatch[k1][k2](v1, v2);
    }
    function element_evaluator(exp, i) {
        var e1 = exp.parents[0], e2 = exp.parents[1];
        var v1, v2;
        var t1 = e1.type, t2 = e2.type;
        if (t1.is_pod() && t2.is_pod()) {
            if (i === 0)
                return exp;
            else
                throw new Error("i > 0 in pod element");
        }
        if (e1.type.is_vec() || e1.type.is_mat())
            v1 = e1.element(i);
        else
            v1 = e1;
        if (e2.type.is_vec() || e2.type.is_vec())
            v2 = e2.element(i);
        else
            v2 = e2;
        return operator(v1, v2, "/", div_type_resolver, evaluator, element_evaluator, "div");
    }
    var current_result = Shade.make(arguments[0]);
    for (var i=1; i<arguments.length; ++i) {
        current_result = operator(current_result, Shade.make(arguments[i]),
                                  "/", div_type_resolver, evaluator, element_evaluator,
                                  "div");
    }
    return current_result;
};

Shade.mul = function() {
    if (arguments.length === 0) throw new Error("mul needs at least one argument");
    if (arguments.length === 1) return arguments[0];
    function mul_type_resolver(t1, t2) {
        if (_.isUndefined(t1))
            throw new Error("t1 multiplication with undefined type?");
        if (_.isUndefined(t2))
            throw new Error("t2 multiplication with undefined type?");
        var type_list = [
            [Shade.Types.vec4, Shade.Types.vec4, Shade.Types.vec4],
            [Shade.Types.mat4, Shade.Types.mat4, Shade.Types.mat4],
            [Shade.Types.mat4, Shade.Types.vec4, Shade.Types.vec4],
            [Shade.Types.vec4, Shade.Types.mat4, Shade.Types.vec4],
            [Shade.Types.float_t, Shade.Types.float_t, Shade.Types.float_t],
            [Shade.Types.vec4, Shade.Types.float_t, Shade.Types.vec4],
            [Shade.Types.float_t, Shade.Types.vec4, Shade.Types.vec4],
            [Shade.Types.mat4, Shade.Types.float_t, Shade.Types.mat4],
            [Shade.Types.float_t, Shade.Types.mat4, Shade.Types.mat4],

            [Shade.Types.vec3, Shade.Types.vec3, Shade.Types.vec3],
            [Shade.Types.mat3, Shade.Types.mat3, Shade.Types.mat3],
            [Shade.Types.mat3, Shade.Types.vec3, Shade.Types.vec3],
            [Shade.Types.vec3, Shade.Types.mat3, Shade.Types.vec3],
            [Shade.Types.float_t, Shade.Types.float_t, Shade.Types.float_t],
            [Shade.Types.vec3, Shade.Types.float_t, Shade.Types.vec3],
            [Shade.Types.float_t, Shade.Types.vec3, Shade.Types.vec3],
            [Shade.Types.mat3, Shade.Types.float_t, Shade.Types.mat3],
            [Shade.Types.float_t, Shade.Types.mat3, Shade.Types.mat3],

            [Shade.Types.vec2, Shade.Types.vec2, Shade.Types.vec2],
            [Shade.Types.mat2, Shade.Types.mat2, Shade.Types.mat2],
            [Shade.Types.mat2, Shade.Types.vec2, Shade.Types.vec2],
            [Shade.Types.vec2, Shade.Types.mat2, Shade.Types.vec2],
            [Shade.Types.float_t, Shade.Types.float_t, Shade.Types.float_t],
            [Shade.Types.vec2, Shade.Types.float_t, Shade.Types.vec2],
            [Shade.Types.float_t, Shade.Types.vec2, Shade.Types.vec2],
            [Shade.Types.mat2, Shade.Types.float_t, Shade.Types.mat2],
            [Shade.Types.float_t, Shade.Types.mat2, Shade.Types.mat2],
            
            [Shade.Types.int_t, Shade.Types.int_t, Shade.Types.int_t]
        ];
        for (var i=0; i<type_list.length; ++i)
            if (t1.equals(type_list[i][0]) &&
                t2.equals(type_list[i][1]))
                return type_list[i][2];
        throw new Error("type mismatch on mul: unexpected types  '"
                   + t1.repr() + "' and '" + t2.repr() + "'.");
    }
    function evaluator(exp, cache) {
        var exp1 = exp.parents[0];
        var exp2 = exp.parents[1];
        var v1 = exp1.evaluate(cache);
        var v2 = exp2.evaluate(cache);
        var vt, mt;
        if (exp1.type.is_array()) {
            vt = vec[exp1.type.array_size()];
            mt = mat[exp1.type.array_size()];
        } else if (exp2.type.is_array()) {
            vt = vec[exp2.type.array_size()];
            mt = mat[exp2.type.array_size()];
        }
        var t1 = Shade.Types.type_of(v1), t2 = Shade.Types.type_of(v2);
        var k1 = t1.is_vec() ? "vector" :
                 t1.is_mat() ? "matrix" :
                 t1.is_pod() ? "number" : "BAD";
        var k2 = t2.is_vec() ? "vector" :
                 t2.is_mat() ? "matrix" :
                 t2.is_pod() ? "number" : "BAD";
        var dispatch = {
            number: { number: function (x, y) { return x * y; },
                      vector: function (x, y) { return vt.scaling(y, x); },
                      matrix: function (x, y) { return mt.scaling(y, x); }
                    },
            vector: { number: function (x, y) { return vt.scaling(x, y); },
                      vector: function (x, y) {
                          return vt.schur_product(x, y);
                      },
                      matrix: function (x, y) {
                          return mt.product_vec(mt.transpose(y), x);
                      }
                    },
            matrix: { number: function (x, y) { return mt.scaling(x, y); },
                      vector: function (x, y) { return mt.product_vec(x, y); },
                      matrix: function (x, y) { return mt.product(x, y); }
                    }
        };
        return dispatch[k1][k2](v1, v2);
    }
    function element_evaluator(exp, i) {
        var e1 = exp.parents[0], e2 = exp.parents[1];
        var v1, v2;
        var t1 = e1.type, t2 = e2.type;
        if (t1.is_pod() && t2.is_pod()) {
            if (i === 0)
                return exp;
            else
                throw new Error("i > 0 in pod element");
        }
        function value_kind(t) {
            if (t.is_pod())
                return "pod";
            if (t.is_vec())
                return "vec";
            if (t.is_mat())
                return "mat";
            throw new Error("internal error: not pod, vec or mat");
        }
        var k1 = value_kind(t1), k2 = value_kind(t2);
        var dispatch = {
            "pod": { 
                "pod": function() { 
                    throw new Error("internal error, pod pod"); 
                },
                "vec": function() { 
                    v1 = e1; v2 = e2.element(i); 
                    return operator(v1, v2, "*", mul_type_resolver, evaluator, element_evaluator, "mul");
                },
                "mat": function() { 
                    v1 = e1; v2 = e2.element(i); 
                    return operator(v1, v2, "*", mul_type_resolver, evaluator, element_evaluator, "mul");
                }
            },
            "vec": { 
                "pod": function() { 
                    v1 = e1.element(i); v2 = e2; 
                    return operator(v1, v2, "*", mul_type_resolver, evaluator, element_evaluator, "mul");
                },
                "vec": function() { 
                    v1 = e1.element(i); v2 = e2.element(i); 
                    return operator(v1, v2, "*", mul_type_resolver, evaluator, element_evaluator, "mul");
                },
                "mat": function() {
                    // FIXME should we have a mat_dimension?
                    return Shade.dot(e1, e2.element(i));
                }
            },
            "mat": { 
                "pod": function() { 
                    v1 = e1.element(i); v2 = e2;
                    return operator(v1, v2, "*", mul_type_resolver, evaluator, element_evaluator, "mul");
                },
                "vec": function() {
                    // FIXME should we have a mat_dimension?
                    var d = t1.array_size();
                    var row;
                    if (d === 2) {
                        row = Shade.vec(e1.element(0).element(i),
                                        e1.element(1).element(i));
                    } else if (d === 3) {
                        row = Shade.vec(e1.element(0).element(i),
                                        e1.element(1).element(i),
                                        e1.element(2).element(i));
                    } else if (d === 4) {
                        row = Shade.vec(e1.element(0).element(i),
                                        e1.element(1).element(i),
                                        e1.element(2).element(i),
                                        e1.element(3).element(i));
                    } else
                        throw new Error("bad dimension for mat " + d);
                    return Shade.dot(row, e2);
                    // var row = e1.element(i);
                    // return Shade.dot(row, e2);
                },
                "mat": function() {
                    var col = e2.element(i);
                    return operator(e1, col, "*", mul_type_resolver, evaluator, element_evaluator,
                                    "mul");
                }
            }
        };
        return dispatch[k1][k2]();
    };
    var current_result = Shade.make(arguments[0]);
    for (var i=1; i<arguments.length; ++i) {
        if (current_result.type.equals(Shade.Types.mat4)) {
            if (arguments[i].type.equals(Shade.Types.vec2)) {
                arguments[i] = Shade.vec(arguments[i], 0, 1);
            } else if (arguments[i].type.equals(Shade.Types.vec3)) {
                arguments[i] = Shade.vec(arguments[i], 1);
            }
        }
        current_result = operator(current_result, Shade.make(arguments[i]),
                                  "*", mul_type_resolver, evaluator, element_evaluator, "mul");
    }
    return current_result;
};
})();
Shade.neg = function(x)
{
    return Shade.sub(0, x);
};
Shade.Exp.neg = function() { return Shade.neg(this); };
Shade.vec = function()
{
    var parents = [];
    var parent_offsets = [];
    var total_size = 0;
    var vec_type;
    for (var i=0; i<arguments.length; ++i) {
        var arg = Shade.make(arguments[i]);
        parents.push(arg);
        parent_offsets.push(total_size);
        if (_.isUndefined(vec_type))
            vec_type = arg.type.element_type(0);
        else if (!vec_type.equals(arg.type.element_type(0)))
            throw new Error("vec requires equal types");
        total_size += arg.type.size_for_vec_constructor();
    }
    parent_offsets.push(total_size);
    if (total_size < 1 || total_size > 4) {
        throw new Error("vec constructor requires resulting width to be between "
            + "1 and 4, got " + total_size + " instead");
    }
    var type;
    if (vec_type.equals(Shade.Types.float_t)) {
        type = Shade.Types["vec" + total_size];
    } else if (vec_type.equals(Shade.Types.int_t)) {
        type = Shade.Types["ivec" + total_size];
    } else if (vec_type.equals(Shade.Types.bool_t)) {
        type = Shade.Types["bvec" + total_size];
    } else {
        throw new Error("vec type must be bool, int, or float");
    }
    
    return Shade._create_concrete_value_exp({
        parents: parents,
        parent_offsets: parent_offsets,
        type: type,
        expression_type: 'vec',
        size: total_size,
        element: function(i) {
            var old_i = i;
            for (var j=0; j<this.parents.length; ++j) {
                var sz = this.parent_offsets[j+1] - this.parent_offsets[j];
                if (i < sz)
                    return this.parents[j].element(i);
                i = i - sz;
            }
            throw new Error("element " + old_i + " out of bounds (size=" 
                + total_size + ")");
        },
        element_is_constant: function(i) {
            var old_i = i;
            for (var j=0; j<this.parents.length; ++j) {
                var sz = this.parent_offsets[j+1] - this.parent_offsets[j];
                if (i < sz)
                    return this.parents[j].element_is_constant(i);
                i = i - sz;
            }
            throw new Error("element " + old_i + " out of bounds (size=" 
                + total_size + ")");
        },
        element_constant_value: function(i) {
            var old_i = i;
            for (var j=0; j<this.parents.length; ++j) {
                var sz = this.parent_offsets[j+1] - this.parent_offsets[j];
                if (i < sz)
                    return this.parents[j].element_constant_value(i);
                i = i - sz;
            }
            throw new Error("element " + old_i + " out of bounds (size=" 
                + total_size + ")");
        },
        evaluate: Shade.memoize_on_guid_dict(function(cache) {
            var result = [];
            var parent_values = _.each(this.parents, function(v) {
                var c = v.evaluate(cache);
                if (lux_typeOf(c) === 'number')
                    result.push(c);
                else
                    for (var i=0; i<c.length; ++i)
                        result.push(c[i]);
            });
            return vec[result.length].make(result);
        }),
        value: function() {
            return this.type.repr() + "(" +
                this.parents.map(function (t) {
                    return t.glsl_expression();
                }).join(", ") + ")";
        }
    });
};
Shade.mat = function()
{
    var parents = [];
    var rows = arguments.length, cols;

    for (var i=0; i<arguments.length; ++i) {
        var arg = arguments[i];
        // if (!(arg.expression_type === 'vec')) {
        //     throw new Error("mat only takes vecs as parameters");
        // }
        parents.push(arg);
        if (i === 0)
            cols = arg.type.size_for_vec_constructor();
        else if (cols !== arg.type.size_for_vec_constructor())
            throw new Error("mat: all vecs must have same dimension");
    }

    if (cols !== rows) {
        throw new Error("non-square matrices currently not supported");
    }

    if (rows < 1 || rows > 4) {
        throw new Error("mat constructor requires resulting dimension to be between "
            + "2 and 4");
    }
    var type = Shade.Types["mat" + rows];
    return Shade._create_concrete_value_exp( {
        parents: parents,
        type: type,
        expression_type: 'mat',
        size: rows,
        element: function(i) {
            return this.parents[i];
        },
        element_is_constant: function(i) {
            return this.parents[i].is_constant();
        },
        element_constant_value: function(i) {
            return this.parents[i].constant_value();
        },
        evaluate: Shade.memoize_on_guid_dict(function(cache) {
            var result = [];
            var ll = _.each(this.parents, function(v) {
                v = v.evaluate(cache);
                for (var i=0; i<v.length; ++i) {
                    result.push(v[i]);
                }
            });
            return mat[this.type.array_size()].make(result);
        }),
        value: function() {
            return this.type.repr() + "(" +
                this.parents.map(function (t) { 
                    return t.glsl_expression(); 
                }).join(", ") + ")";
        }
    });
};

Shade.mat3 = function(m)
{
    var t = m.type;
    if (t.equals(Shade.Types.mat2)) {
        return Shade.mat(Shade.vec(m.at(0), 0),
                         Shade.vec(m.at(1), 0),
                         Shade.vec(0, 0, 1));
    } else if (t.equals(Shade.Types.mat3)) {
        return m;
    } else if (t.equals(Shade.Types.mat4)) {
        return Shade.mat(m.element(0).swizzle("xyz"),
                         m.element(1).swizzle("xyz"),
                         m.element(2).swizzle("xyz"));
    } else {
        throw new Error("need matrix to convert to mat3");
    }
};
// per_vertex is an identity operation value-wise, but it tags the AST
// so the optimizer can do its thing.
Shade.per_vertex = function(exp)
{
    exp = Shade.make(exp);
    return Shade._create_concrete_exp({
        expression_name: "per_vertex",
        parents: [exp],
        type: exp.type,
        stage: "vertex",
        glsl_expression: function() { return this.parents[0].glsl_expression(); },
        evaluate: function () { return this.parents[0].evaluate(); },
        compile: function () {}
    });
};
(function() {

function zipWith(f, v1, v2)
{
    return _.map(_.zip(v1, v2),
                 function(v) { return f(v[0], v[1]); });
}

function zipWith3(f, v1, v2, v3)
{
    return _.map(_.zip(v1, v2, v3),
                 function(v) { return f(v[0], v[1], v[2]); });
}

//////////////////////////////////////////////////////////////////////////////
// common functions

function builtin_glsl_function(opts)
{
    var name = opts.name;
    var shade_name = opts.shade_name || opts.name;
    var evaluator = opts.evaluator;
    var type_resolving_list = opts.type_resolving_list;
    var element_function = opts.element_function;
    var element_constant_evaluator = opts.element_constant_evaluator;

    for (var i=0; i<type_resolving_list.length; ++i)
        for (var j=0; j<type_resolving_list[i].length; ++j) {
            var t = type_resolving_list[i][j];
            if (_.isUndefined(t))
                throw new Error("undefined type in type_resolver");
        }

    // takes a list of lists of possible argument types, returns a function to 
    // resolve those types.
    function type_resolver_from_list(lst)
    {
        var param_length = lst[0].length - 1;
        return function() {
            if (arguments.length != param_length) {
                throw new Error("expected " + param_length + " arguments, got "
                    + arguments.length + " instead.");
            }
            for (var i=0; i<lst.length; ++i) {
                var this_params = lst[i];
                var matched = true;
                for (var j=0; j<param_length; ++j) {
                    if (!this_params[j].equals(arguments[j].type)) {
                        matched = false;
                        break;
                    }
                }
                if (matched)
                    return this_params[param_length];
            }
            var types = _.map(_.toArray(arguments).slice(0, arguments.length),
                  function(x) { return x.type.repr(); }).join(", ");
            throw new Error("could not find appropriate type match for (" + types + ")");
        };
    }

    return function() {
        var resolver = type_resolver_from_list(type_resolving_list);
        var type, canon_args = [];
        for (i=0; i<arguments.length; ++i) {
            canon_args.push(Shade.make(arguments[i]));
        }
        try {
            type = resolver.apply(this, canon_args);
        } catch (err) {
            throw new Error("type error on " + name + ": " + err);
        }
        var obj = {
            parents: canon_args,
            expression_type: "builtin_function{" + name + "}",
            type: type,
            
            value: function() {
                return [name, "(",
                        this.parents.map(function(t) { 
                            return t.glsl_expression(); 
                        }).join(", "),
                        ")"].join(" ");
            },
            _json_helper: Shade.Debug._json_builder(shade_name)
        };

        if (evaluator) {
            obj.evaluate = Shade.memoize_on_guid_dict(function(cache) {
                return evaluator(this, cache);
            });
        } else {
            throw new Error("Internal error: Builtin '" + name + "' has no evaluator?!");
        }

        obj.constant_value = Shade.memoize_on_field("_constant_value", function() {
            if (!this.is_constant())
                throw new Error("constant_value called on non-constant expression");
            return evaluator(this);
        });

        if (element_function) {
            obj.element = function(i) {
                return element_function(this, i);
            };
            if (element_constant_evaluator) {
                obj.element_is_constant = function(i) {
                    return element_constant_evaluator(this, i);
                };
            } else {
                obj.element_is_constant = function(i) {
                    if (this.guid === 489) {
                        debugger;
                    }
                    return this.element(i).is_constant();
                };
            }
        }
        return Shade._create_concrete_value_exp(obj);
    };
}

function common_fun_1op(fun_name, evaluator) {
    var result = builtin_glsl_function({
        name: fun_name,
        type_resolving_list: [
            [Shade.Types.float_t, Shade.Types.float_t],
            [Shade.Types.vec2, Shade.Types.vec2],
            [Shade.Types.vec3, Shade.Types.vec3],
            [Shade.Types.vec4, Shade.Types.vec4]
        ], 
        evaluator: evaluator,
        element_function: function(exp, i) {
            return result(exp.parents[0].element(i));
        }
    });
    return result;
}

function common_fun_2op(fun_name, evaluator) {
    var result = builtin_glsl_function({
        name: fun_name, 
        type_resolving_list: [
            [Shade.Types.float_t, Shade.Types.float_t, Shade.Types.float_t],
            [Shade.Types.vec2, Shade.Types.vec2, Shade.Types.vec2],
            [Shade.Types.vec3, Shade.Types.vec3, Shade.Types.vec3],
            [Shade.Types.vec4, Shade.Types.vec4, Shade.Types.vec4]
        ], 
        evaluator: evaluator, 
        element_function: function(exp, i) {
            return result(exp.parents[0].element(i), exp.parents[1].element(i));
        }
    });
    return result;
}

// angle and trig, some common, some exponential,
var funcs_1op = {
    "radians": function(v) { return v * Math.PI / 180; },
    "degrees": function(v) { return v / Math.PI * 180; }, 
    "sin": Math.sin,
    "cos": Math.cos, 
    "tan": Math.tan, 
    "asin": Math.asin, 
    "acos": Math.acos, 
    "abs": Math.abs,
    "sign": function(v) { if (v < 0) return -1;
                          if (v === 0) return 0;
                          return 1;
                        }, 
    "floor": Math.floor,
    "ceil": Math.ceil,
    "fract": function(v) { return v - Math.floor(v); },
    "exp": Math.exp, 
    "log": Math.log, 
    "exp2": function(v) { return Math.exp(v * Math.log(2)); },
    "log2": function(v) { return Math.log(v) / Math.log(2); },
    "sqrt": Math.sqrt,
    "inversesqrt": function(v) { return 1 / Math.sqrt(v); }
};

_.each(funcs_1op, function (evaluator_1, fun_name) {
    function evaluator(exp, cache) {
        if (exp.type.equals(Shade.Types.float_t))
            return evaluator_1(exp.parents[0].evaluate(cache));
        else {
            var c = exp.parents[0].evaluate(cache);
            return vec.map(c, evaluator_1);
        }
    }
    Shade[fun_name] = common_fun_1op(fun_name, evaluator);
    Shade.Exp[fun_name] = function(fun) {
        return function() {
            return fun(this);
        };
    }(Shade[fun_name]);
});

function atan1_evaluator(exp, cache)
{
    var v1 = exp.parents[0].evaluate(cache);
    if (exp.type.equals(Shade.Types.float_t))
        return Math.atan(v1);
    else {
        return vec.map(c, Math.atan);
    }
}

function common_fun_2op_evaluator(fun)
{
    return function(exp, cache) {
        var v1 = exp.parents[0].evaluate(cache);
        var v2 = exp.parents[1].evaluate(cache);
        if (exp.type.equals(Shade.Types.float_t))
            return fun(v1, v2);
        else {
            var result = [];
            for (var i=0; i<v1.length; ++i) {
                result.push(fun(v1[i], v2[i]));
            }
            return vec.make(result);
        }
    };
}

function atan()
{
    if (arguments.length == 1) {
        return common_fun_1op("atan", atan1_evaluator)(arguments[0]);
    } else if (arguments.length == 2) {
        var c = common_fun_2op_evaluator(Math.atan2);
        return common_fun_2op("atan", c)(arguments[0], arguments[1]);
    } else {
        throw new Error("atan expects 1 or 2 parameters, got " + arguments.length
                        + " instead.");
    }
}

function broadcast_elements(exp, i) {
    return _.map(exp.parents, function(parent) {
        return parent.type.is_vec() ? parent.element(i) : parent;
    });
}

Shade.atan = atan;
Shade.Exp.atan = function() { return Shade.atan(this); };
Shade.pow = common_fun_2op("pow", common_fun_2op_evaluator(Math.pow));
Shade.Exp.pow = function(other) { return Shade.pow(this, other); };

function mod_min_max_evaluator(op) {
    return function(exp, cache) {
        var values = _.map(exp.parents, function (p) {
            return p.evaluate(cache);
        });
        if (exp.parents[0].type.equals(Shade.Types.float_t))
            return op.apply(op, values);
        else if (exp.parents[0].type.equals(Shade.Types.int_t))
            return op.apply(op, values);
        else if (exp.parents[0].type.equals(exp.parents[1].type)) {
            return vec.make(zipWith(op, values[0], values[1]));
        } else {
            return vec.map(values[0], function(v) {
                return op(v, values[1]);
            });
        }
    };
}

_.each({
    "mod": function(a,b) { return a % b; },
    "min": Math.min,
    "max": Math.max
}, function(op, k) {
    var result = builtin_glsl_function({
        name: k, 
        type_resolving_list: [
            [Shade.Types.float_t,  Shade.Types.float_t, Shade.Types.float_t],
            [Shade.Types.vec2,     Shade.Types.vec2,    Shade.Types.vec2],
            [Shade.Types.vec3,     Shade.Types.vec3,    Shade.Types.vec3],
            [Shade.Types.vec4,     Shade.Types.vec4,    Shade.Types.vec4],
            [Shade.Types.float_t,  Shade.Types.float_t, Shade.Types.float_t],
            [Shade.Types.vec2,     Shade.Types.float_t, Shade.Types.vec2],
            [Shade.Types.vec3,     Shade.Types.float_t, Shade.Types.vec3],
            [Shade.Types.vec4,     Shade.Types.float_t, Shade.Types.vec4]
        ], 
        evaluator: mod_min_max_evaluator(op),
        element_function: function(exp, i) {
            return result.apply(this, broadcast_elements(exp, i));
        }
    });
    Shade[k] = result;
});

function clamp_evaluator(exp, cache)
{
    function clamp(v, mn, mx) {
        return Math.max(mn, Math.min(mx, v));
    }

    var e1 = exp.parents[0];
    var e2 = exp.parents[1];
    var e3 = exp.parents[2];
    var v1 = e1.evaluate(cache);
    var v2 = e2.evaluate(cache);
    var v3 = e3.evaluate(cache);

    if (e1.type.equals(Shade.Types.float_t)) {
        return clamp(v1, v2, v3);
    } else if (e1.type.equals(e2.type)) {
        return vec.make(zipWith3(clamp, v1, v2, v3));
    } else {
        return vec.map(v1, function(v) {
            return clamp(v, v2, v3);
        });
    }
}

var clamp = builtin_glsl_function({
    name: "clamp", 
    type_resolving_list: [
        [Shade.Types.float_t, Shade.Types.float_t, Shade.Types.float_t, Shade.Types.float_t],
        [Shade.Types.vec2,    Shade.Types.vec2,    Shade.Types.vec2,    Shade.Types.vec2],
        [Shade.Types.vec3,    Shade.Types.vec3,    Shade.Types.vec3,    Shade.Types.vec3],
        [Shade.Types.vec4,    Shade.Types.vec4,    Shade.Types.vec4,    Shade.Types.vec4],
        [Shade.Types.vec2,    Shade.Types.float_t, Shade.Types.float_t, Shade.Types.vec2],
        [Shade.Types.vec3,    Shade.Types.float_t, Shade.Types.float_t, Shade.Types.vec3],
        [Shade.Types.vec4,    Shade.Types.float_t, Shade.Types.float_t, Shade.Types.vec4]], 
    evaluator: clamp_evaluator,
    element_function: function (exp, i) {
        return Shade.clamp.apply(this, broadcast_elements(exp, i));
    }
});

Shade.clamp = clamp;

function mix_evaluator(exp, cache)
{
    function mix(left, right, u) {
        return (1-u) * left + u * right;
    }
    var e1 = exp.parents[0];
    var e2 = exp.parents[1];
    var e3 = exp.parents[2];
    var v1 = e1.evaluate(cache);
    var v2 = e2.evaluate(cache);
    var v3 = e3.evaluate(cache);
    if (e1.type.equals(Shade.Types.float_t)) {
        return mix(v1, v2, v3);
    } else if (e2.type.equals(e3.type)) {
        return vec.make(zipWith3(mix, v1, v2, v3));
    } else {
        return vec.make(zipWith(function(v1, v2) {
            return mix(v1, v2, v3);
        }, v1, v2));
    }
}

var mix = builtin_glsl_function({ 
    name: "mix", 
    type_resolving_list: [
        [Shade.Types.float_t, Shade.Types.float_t, Shade.Types.float_t, Shade.Types.float_t],
        [Shade.Types.vec2,    Shade.Types.vec2,    Shade.Types.vec2,    Shade.Types.vec2],
        [Shade.Types.vec3,    Shade.Types.vec3,    Shade.Types.vec3,    Shade.Types.vec3],
        [Shade.Types.vec4,    Shade.Types.vec4,    Shade.Types.vec4,    Shade.Types.vec4],
        [Shade.Types.vec2,    Shade.Types.vec2,    Shade.Types.float_t, Shade.Types.vec2],
        [Shade.Types.vec3,    Shade.Types.vec3,    Shade.Types.float_t, Shade.Types.vec3],
        [Shade.Types.vec4,    Shade.Types.vec4,    Shade.Types.float_t, Shade.Types.vec4]],
    evaluator: mix_evaluator,
    element_function: function(exp, i) {
        return Shade.mix.apply(this, broadcast_elements(exp, i));
    }
});
Shade.mix = mix;

var step = builtin_glsl_function({
    name: "step", 
    type_resolving_list: [
        [Shade.Types.float_t, Shade.Types.float_t, Shade.Types.float_t],
        [Shade.Types.vec2,    Shade.Types.vec2,    Shade.Types.vec2],
        [Shade.Types.vec3,    Shade.Types.vec3,    Shade.Types.vec3],
        [Shade.Types.vec4,    Shade.Types.vec4,    Shade.Types.vec4],
        [Shade.Types.float_t, Shade.Types.vec2,    Shade.Types.vec2],
        [Shade.Types.float_t, Shade.Types.vec3,    Shade.Types.vec3],
        [Shade.Types.float_t, Shade.Types.vec4,    Shade.Types.vec4]], 
    evaluator: function(exp, cache) {
        function step(edge, x) {
            if (x < edge) return 0.0; else return 1.0;
        }
        var e1 = exp.parents[0];
        var e2 = exp.parents[1];
        var v1 = e1.evaluate(cache);
        var v2 = e2.evaluate(cache);
        if (e2.type.equals(Shade.Types.float_t)) {
            return step(v1, v2);
        } if (e1.type.equals(e2.type)) {
            return vec.make(zipWith(step, v1, v2));
        } else {
            return vec.map(v2, function(v) { 
                return step(v1, v);
            });
        }
    },
    element_function: function(exp, i) {
        return Shade.step.apply(this, broadcast_elements(exp, i));
    }
});
Shade.step = step;

var smoothstep = builtin_glsl_function({
    name: "smoothstep", 
    type_resolving_list: [
        [Shade.Types.float_t, Shade.Types.float_t, Shade.Types.float_t, Shade.Types.float_t],
        [Shade.Types.vec2,    Shade.Types.vec2,    Shade.Types.vec2,    Shade.Types.vec2],
        [Shade.Types.vec3,    Shade.Types.vec3,    Shade.Types.vec3,    Shade.Types.vec3],
        [Shade.Types.vec4,    Shade.Types.vec4,    Shade.Types.vec4,    Shade.Types.vec4],
        [Shade.Types.float_t, Shade.Types.float_t, Shade.Types.vec2,    Shade.Types.vec2],
        [Shade.Types.float_t, Shade.Types.float_t, Shade.Types.vec3,    Shade.Types.vec3],
        [Shade.Types.float_t, Shade.Types.float_t, Shade.Types.vec4,    Shade.Types.vec4]], 
    evaluator: function(exp, cache) {
        var edge0 = exp.parents[0];
        var edge1 = exp.parents[1];
        var x = exp.parents[2];
        var t = Shade.clamp(x.sub(edge0).div(edge1.sub(edge0)), 0, 1);
        // FIXME this is cute but will be inefficient
        return t.mul(t).mul(Shade.sub(3, t.mul(2))).evaluate(cache);
    }, element_function: function(exp, i) {
        return Shade.smoothstep.apply(this, broadcast_elements(exp, i));
    }
});
Shade.smoothstep = smoothstep;

var norm = builtin_glsl_function({
    name: "length", 
    shade_name: "norm",
    type_resolving_list: [
        [Shade.Types.float_t, Shade.Types.float_t],
        [Shade.Types.vec2,    Shade.Types.float_t],
        [Shade.Types.vec3,    Shade.Types.float_t],
        [Shade.Types.vec4,    Shade.Types.float_t]], 
    evaluator: function(exp, cache) {
        var v = exp.parents[0].evaluate(cache);
        if (exp.parents[0].type.equals(Shade.Types.float_t))
            return Math.abs(v);
        else
            return vec.length(v);
    }});
Shade.norm = norm;

var distance = builtin_glsl_function({
    name: "distance", 
    type_resolving_list: [
        [Shade.Types.float_t, Shade.Types.float_t, Shade.Types.float_t],
        [Shade.Types.vec2,    Shade.Types.vec2,    Shade.Types.float_t],
        [Shade.Types.vec3,    Shade.Types.vec3,    Shade.Types.float_t],
        [Shade.Types.vec4,    Shade.Types.vec4,    Shade.Types.float_t]], 
    evaluator: function(exp, cache) {
        return exp.parents[0].sub(exp.parents[1]).norm().evaluate(cache);
    }});
Shade.distance = distance;

var dot = builtin_glsl_function({
    name: "dot", 
    type_resolving_list: [
        [Shade.Types.float_t, Shade.Types.float_t, Shade.Types.float_t],
        [Shade.Types.vec2,    Shade.Types.vec2,    Shade.Types.float_t],
        [Shade.Types.vec3,    Shade.Types.vec3,    Shade.Types.float_t],
        [Shade.Types.vec4,    Shade.Types.vec4,    Shade.Types.float_t]],
    evaluator: function (exp, cache) {
        var v1 = exp.parents[0].evaluate(cache),
            v2 = exp.parents[1].evaluate(cache);
        if (exp.parents[0].type.equals(Shade.Types.float_t)) {
            return v1 * v2;
        } else {
            return vec.dot(v1, v2);
        }
    }});
Shade.dot = dot;

var cross = builtin_glsl_function({
    name: "cross", 
    type_resolving_list: [[Shade.Types.vec3, Shade.Types.vec3, Shade.Types.vec3]], 
    evaluator: function(exp, cache) {
        return vec3.cross(exp.parents[0].evaluate(cache),
                          exp.parents[1].evaluate(cache));
    }, element_function: function (exp, i) {
        var v1 = exp.parents[0].length;
        var v2 = exp.parents[1].length;
        if        (i === 0) { return v1.at(1).mul(v2.at(2)).sub(v1.at(2).mul(v2.at(1)));
        } else if (i === 1) { return v1.at(2).mul(v2.at(0)).sub(v1.at(0).mul(v2.at(2)));
        } else if (i === 2) { return v1.at(0).mul(v2.at(1)).sub(v1.at(1).mul(v2.at(0)));
        } else
            throw new Error("invalid element " + i + " for cross");
    }
});
Shade.cross = cross;

var normalize = builtin_glsl_function({
    name: "normalize", 
    type_resolving_list: [
        [Shade.Types.float_t, Shade.Types.float_t],
        [Shade.Types.vec2, Shade.Types.vec2],
        [Shade.Types.vec3, Shade.Types.vec3],
        [Shade.Types.vec4, Shade.Types.vec4]], 
    evaluator: function(exp, cache) {
        return exp.parents[0].div(exp.parents[0].norm()).evaluate(cache);
    }, element_function: function(exp, i) {
        return exp.parents[0].div(exp.parents[0].norm()).element(i);
    }
});
Shade.normalize = normalize;

var faceforward = builtin_glsl_function({
    name: "faceforward", 
    type_resolving_list: [
        [Shade.Types.float_t, Shade.Types.float_t, Shade.Types.float_t, Shade.Types.float_t],
        [Shade.Types.vec2, Shade.Types.vec2, Shade.Types.vec2, Shade.Types.vec2],
        [Shade.Types.vec3, Shade.Types.vec3, Shade.Types.vec3, Shade.Types.vec3],
        [Shade.Types.vec4, Shade.Types.vec4, Shade.Types.vec4, Shade.Types.vec4]], 
    evaluator: function(exp, cache) {
        var N = exp.parents[0];
        var I = exp.parents[1];
        var Nref = exp.parents[2];
        if (Nref.dot(I).evaluate(cache) < 0)
            return N.evaluate(cache);
        else
            return Shade.sub(0, N).evaluate(cache);
    }, element_function: function(exp, i) {
        var N = exp.parents[0];
        var I = exp.parents[1];
        var Nref = exp.parents[2];
        return Shade.ifelse(Nref.dot(I).lt(0),
                            N, Shade.neg(N)).element(i);
    }
});
Shade.faceforward = faceforward;

var reflect = builtin_glsl_function({
    name: "reflect", 
    type_resolving_list: [
        [Shade.Types.float_t, Shade.Types.float_t, Shade.Types.float_t],
        [Shade.Types.vec2, Shade.Types.vec2, Shade.Types.vec2],
        [Shade.Types.vec3, Shade.Types.vec3, Shade.Types.vec3],
        [Shade.Types.vec4, Shade.Types.vec4, Shade.Types.vec4]], 
    evaluator: function(exp, cache) {
        var I = exp.parents[0];
        var N = exp.parents[1];
        return I.sub(Shade.mul(2, N.dot(I), N)).evaluate(cache);
    }, element_function: function(exp, i) {
        var I = exp.parents[0];
        var N = exp.parents[1];
        return I.sub(Shade.mul(2, N.dot(I), N)).element(i);
    }
});
Shade.reflect = reflect;

var refract = builtin_glsl_function({
    name: "refract", 
    type_resolving_list: [
        [Shade.Types.float_t, Shade.Types.float_t, Shade.Types.float_t, Shade.Types.float_t],
        [Shade.Types.vec2, Shade.Types.vec2, Shade.Types.float_t, Shade.Types.vec2],
        [Shade.Types.vec3, Shade.Types.vec3, Shade.Types.float_t, Shade.Types.vec3],
        [Shade.Types.vec4, Shade.Types.vec4, Shade.Types.float_t, Shade.Types.vec4]],
    evaluator: function(exp, cache) {
        var I = exp.parents[0];
        var N = exp.parents[1];
        var eta = exp.parents[2];
        
        var k = Shade.sub(1.0, Shade.mul(eta, eta, Shade.sub(1.0, N.dot(I).mul(N.dot(I)))));
        // FIXME This is cute but inefficient
        if (k.evaluate(cache) < 0.0) {
            return vec[I.type.array_size()].create();
        } else {
            return eta.mul(I).sub(eta.mul(N.dot(I)).add(k.sqrt()).mul(N)).evaluate(cache);
        }
    }, element_function: function(exp, i) {
        var I = exp.parents[0];
        var N = exp.parents[1];
        var eta = exp.parents[2];
        var k = Shade.sub(1.0, Shade.mul(eta, eta, Shade.sub(1.0, N.dot(I).mul(N.dot(I)))));
        var refraction = eta.mul(I).sub(eta.mul(N.dot(I)).add(k.sqrt()).mul(N));
        var zero;
        switch (I.type.array_size()) {
        case 2: zero = Shade.vec(0,0); break;
        case 3: zero = Shade.vec(0,0,0); break;
        case 4: zero = Shade.vec(0,0,0,0); break;
        default: throw new Error("internal error");
        };
        return Shade.ifelse(k.lt(0), zero, refraction).element(i);
    }
});
Shade.refract = refract;

var texture2D = builtin_glsl_function({
    name: "texture2D", 
    type_resolving_list: [[Shade.Types.sampler2D, Shade.Types.vec2, Shade.Types.vec4]],
    element_function: function(exp, i) { return exp.at(i); },

    // This line below is necessary to prevent an infinite loop
    // because we're expressing element_function as exp.at();
    element_constant_evaluator: function(exp, i) { return false; },

    evaluator: function(exp) {
        throw new Error("evaluate unsupported on texture2D expressions");
    }
});
Shade.texture2D = texture2D;

_.each(["dFdx", "dFdy", "fwidth"], function(cmd) {
    var fun = builtin_glsl_function({
        name: cmd,
        type_resolving_list: [
            [Shade.Types.float_t, Shade.Types.float_t],
            [Shade.Types.vec2, Shade.Types.vec2],
            [Shade.Types.vec3, Shade.Types.vec3],
            [Shade.Types.vec4, Shade.Types.vec4]
        ],

        // This line below is necessary to prevent an infinite loop
        // because we're expressing element_function as exp.at();
        element_function: function(exp, i) { return exp.at(i); },

        element_constant_evaluator: function(exp, i) { return false; },

        evaluator: function(exp) {
            throw new Error("evaluate unsupported on " + cmd + " expressions");
        }
    });
    Shade[cmd] = fun;
    Shade.Exp[cmd] = function() {
        return Shade[cmd](this);
    };
});

Shade.equal = builtin_glsl_function({
    name: "equal", 
    type_resolving_list: [
        [Shade.Types.vec2, Shade.Types.vec2, Shade.Types.bool_t],
        [Shade.Types.vec3, Shade.Types.vec3, Shade.Types.bool_t],
        [Shade.Types.vec4, Shade.Types.vec4, Shade.Types.bool_t],
        [Shade.Types.ivec2, Shade.Types.ivec2, Shade.Types.bool_t],
        [Shade.Types.ivec3, Shade.Types.ivec3, Shade.Types.bool_t],
        [Shade.Types.ivec4, Shade.Types.ivec4, Shade.Types.bool_t],
        [Shade.Types.bvec2, Shade.Types.bvec2, Shade.Types.bool_t],
        [Shade.Types.bvec3, Shade.Types.bvec3, Shade.Types.bool_t],
        [Shade.Types.bvec4, Shade.Types.bvec4, Shade.Types.bool_t]], 
    evaluator: function(exp, cache) {
        var left = exp.parents[0].evaluate(cache);
        var right = exp.parents[1].evaluate(cache);
        return (_.all(zipWith(function (x, y) { return x === y; }),
                      left, right));
    }});
Shade.Exp.equal = function(other) { return Shade.equal(this, other); };

Shade.notEqual = builtin_glsl_function({
    name: "notEqual", 
    type_resolving_list: [
        [Shade.Types.vec2, Shade.Types.vec2, Shade.Types.bool_t],
        [Shade.Types.vec3, Shade.Types.vec3, Shade.Types.bool_t],
        [Shade.Types.vec4, Shade.Types.vec4, Shade.Types.bool_t],
        [Shade.Types.ivec2, Shade.Types.ivec2, Shade.Types.bool_t],
        [Shade.Types.ivec3, Shade.Types.ivec3, Shade.Types.bool_t],
        [Shade.Types.ivec4, Shade.Types.ivec4, Shade.Types.bool_t],
        [Shade.Types.bvec2, Shade.Types.bvec2, Shade.Types.bool_t],
        [Shade.Types.bvec3, Shade.Types.bvec3, Shade.Types.bool_t],
        [Shade.Types.bvec4, Shade.Types.bvec4, Shade.Types.bool_t]], 
    evaluator: function(exp, cache) {
        var left = exp.parents[0].evaluate(cache);
        var right = exp.parents[1].evaluate(cache);
        return !(_.all(zipWith(function (x, y) { return x === y; }),
                       left, right));
    }});
Shade.Exp.notEqual = function(other) { return Shade.notEqual(this, other); };

Shade.lessThan = builtin_glsl_function({
    name: "lessThan", 
    type_resolving_list: [
        [Shade.Types.vec2, Shade.Types.vec2, Shade.Types.bvec2],
        [Shade.Types.vec3, Shade.Types.vec3, Shade.Types.bvec3],
        [Shade.Types.vec4, Shade.Types.vec4, Shade.Types.bvec4],
        [Shade.Types.ivec2, Shade.Types.ivec2, Shade.Types.bvec2],
        [Shade.Types.ivec3, Shade.Types.ivec3, Shade.Types.bvec3],
        [Shade.Types.ivec4, Shade.Types.ivec4, Shade.Types.bvec4]], 
    evaluator: function(exp, cache) {
        var left = exp.parents[0].evaluate(cache);
        var right = exp.parents[1].evaluate(cache);
        return _.map(left, function(x, i) { return x < right[i]; });
    }, element_function: function(exp, i) {
        return Shade.lt.apply(this, broadcast_elements(exp, i));
    }
});
Shade.Exp.lessThan = function(other) { return Shade.lessThan(this, other); };

Shade.lessThanEqual = builtin_glsl_function({
    name: "lessThanEqual", 
    type_resolving_list: [
        [Shade.Types.vec2, Shade.Types.vec2, Shade.Types.bvec2],
        [Shade.Types.vec3, Shade.Types.vec3, Shade.Types.bvec3],
        [Shade.Types.vec4, Shade.Types.vec4, Shade.Types.bvec4],
        [Shade.Types.ivec2, Shade.Types.ivec2, Shade.Types.bvec2],
        [Shade.Types.ivec3, Shade.Types.ivec3, Shade.Types.bvec3],
        [Shade.Types.ivec4, Shade.Types.ivec4, Shade.Types.bvec4]], 
    evaluator: function(exp, cache) {
        var left = exp.parents[0].evaluate(cache);
        var right = exp.parents[1].evaluate(cache);
        return _.map(left, function(x, i) { return x <= right[i]; });
    }, element_function: function(exp, i) {
        return Shade.le.apply(this, broadcast_elements(exp, i));
    }
});
Shade.Exp.lessThanEqual = function(other) { 
    return Shade.lessThanEqual(this, other); 
};

Shade.greaterThan = builtin_glsl_function({
    name: "greaterThan", 
    type_resolving_list: [
        [Shade.Types.vec2, Shade.Types.vec2, Shade.Types.bvec2],
        [Shade.Types.vec3, Shade.Types.vec3, Shade.Types.bvec3],
        [Shade.Types.vec4, Shade.Types.vec4, Shade.Types.bvec4],
        [Shade.Types.ivec2, Shade.Types.ivec2, Shade.Types.bvec2],
        [Shade.Types.ivec3, Shade.Types.ivec3, Shade.Types.bvec3],
        [Shade.Types.ivec4, Shade.Types.ivec4, Shade.Types.bvec4]], 
    evaluator: function(exp, cache) {
        var left = exp.parents[0].evaluate(cache);
        var right = exp.parents[1].evaluate(cache);
        return _.map(left, function(x, i) { return x > right[i]; });
    }, element_function: function(exp, i) {
        return Shade.gt.apply(this, broadcast_elements(exp, i));
    }
});
Shade.Exp.greaterThan = function(other) {
    return Shade.greaterThan(this, other);
};

Shade.greaterThanEqual = builtin_glsl_function({
    name: "greaterThanEqual", 
    type_resolving_list: [
        [Shade.Types.vec2, Shade.Types.vec2, Shade.Types.bvec2],
        [Shade.Types.vec3, Shade.Types.vec3, Shade.Types.bvec3],
        [Shade.Types.vec4, Shade.Types.vec4, Shade.Types.bvec4],
        [Shade.Types.ivec2, Shade.Types.ivec2, Shade.Types.bvec2],
        [Shade.Types.ivec3, Shade.Types.ivec3, Shade.Types.bvec3],
        [Shade.Types.ivec4, Shade.Types.ivec4, Shade.Types.bvec4]], 
    evaluator: function(exp, cache) {
        var left = exp.parents[0].evaluate(cache);
        var right = exp.parents[1].evaluate(cache);
        return _.map(left, function(x, i) { return x >= right[i]; });
    }, element_function: function(exp, i) {
        return Shade.ge.apply(this, broadcast_elements(exp, i));
    }
});
Shade.Exp.greaterThanEqual = function(other) {
    return Shade.greaterThanEqual(this, other);
};

Shade.all = builtin_glsl_function({
    name: "all", 
    type_resolving_list: [
        [Shade.Types.bvec2, Shade.Types.bool_t],
        [Shade.Types.bvec3, Shade.Types.bool_t],
        [Shade.Types.bvec4, Shade.Types.bool_t]], 
    evaluator: function(exp, cache) {
        var v = exp.parents[0].evaluate(cache);
        return _.all(v, function(x) { return x; });
    }});
Shade.Exp.all = function() { return Shade.all(this); };

Shade.any = builtin_glsl_function({
    name: "any", 
    type_resolving_list: [
        [Shade.Types.bvec2, Shade.Types.bool_t],
        [Shade.Types.bvec3, Shade.Types.bool_t],
        [Shade.Types.bvec4, Shade.Types.bool_t]], 
    evaluator: function(exp, cache) {
        var v = exp.parents[0].evaluate(cache);
        return _.any(v, function(x) { return x; });
    }});
Shade.Exp.any = function() { return Shade.any(this); };

Shade.matrixCompMult = builtin_glsl_function({
    name: "matrixCompMult", 
    type_resolving_list: [
        [Shade.Types.mat2, Shade.Types.mat2, Shade.Types.mat2],
        [Shade.Types.mat3, Shade.Types.mat3, Shade.Types.mat3],
        [Shade.Types.mat4, Shade.Types.mat4, Shade.Types.mat4]], 
    evaluator: function(exp, cache) {
        var v1 = exp.parents[0].evaluate(cache);
        var v2 = exp.parents[1].evaluate(cache);
        return mat.map(v1, function(x, i) { return x * v2[i]; });
    }, element_function: function(exp, i) {
        var v1 = exp.parents[0];
        var v2 = exp.parents[1];
        return v1.element(i).mul(v2.element(i));
    }
});
Shade.Exp.matrixCompMult = function(other) {
    return Shade.matrixCompMult(this, other);
};

Shade.Types.int_t.zero   = Shade.constant(0, Shade.Types.int_t);
Shade.Types.float_t.zero = Shade.constant(0);
Shade.Types.vec2.zero    = Shade.constant(vec2.make([0,0]));
Shade.Types.vec3.zero    = Shade.constant(vec3.make([0,0,0]));
Shade.Types.vec4.zero    = Shade.constant(vec4.make([0,0,0,0]));
Shade.Types.mat2.zero    = Shade.constant(mat2.make([0,0,0,0]));
Shade.Types.mat3.zero    = Shade.constant(mat3.make([0,0,0,0,0,0,0,0,0]));
Shade.Types.mat4.zero    = Shade.constant(mat4.make([0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]));

// according to the GLSL ES spec, for highp numbers the limit for ints is 2^16, and for floats, 2^52 ~= 10^18
Shade.Types.int_t.infinity   = Shade.constant(65535, Shade.Types.int_t);
Shade.Types.float_t.infinity = Shade.constant(1e18);
Shade.Types.vec2.infinity    = Shade.constant(vec2.make([1e18,1e18]));
Shade.Types.vec3.infinity    = Shade.constant(vec3.make([1e18,1e18,1e18]));
Shade.Types.vec4.infinity    = Shade.constant(vec4.make([1e18,1e18,1e18,1e18]));
Shade.Types.mat2.infinity    = Shade.constant(mat2.make([1e18,1e18,1e18,1e18]));
Shade.Types.mat3.infinity    = Shade.constant(mat3.make([1e18,1e18,1e18,1e18,1e18,1e18,1e18,1e18,1e18]));
Shade.Types.mat4.infinity    = Shade.constant(mat4.make([1e18,1e18,1e18,1e18,1e18,1e18,1e18,1e18,1e18,1e18,1e18,1e18,1e18,1e18,1e18,1e18]));

Shade.Types.int_t.minus_infinity   = Shade.constant(-65535, Shade.Types.int_t);
Shade.Types.float_t.minus_infinity = Shade.constant(-1e18);
Shade.Types.vec2.minus_infinity    = Shade.constant(vec2.make([-1e18,-1e18]));
Shade.Types.vec3.minus_infinity    = Shade.constant(vec3.make([-1e18,-1e18,-1e18]));
Shade.Types.vec4.minus_infinity    = Shade.constant(vec4.make([-1e18,-1e18,-1e18,-1e18]));
Shade.Types.mat2.minus_infinity    = Shade.constant(mat2.make([-1e18,-1e18,-1e18,-1e18]));
Shade.Types.mat3.minus_infinity    = Shade.constant(mat3.make([-1e18,-1e18,-1e18,-1e18,-1e18,-1e18,-1e18,-1e18,-1e18]));
Shade.Types.mat4.minus_infinity    = Shade.constant(mat4.make([-1e18,-1e18,-1e18,-1e18,-1e18,-1e18,-1e18,-1e18,-1e18,-1e18,-1e18,-1e18,-1e18,-1e18,-1e18,-1e18]));

})();
Shade.seq = function(parents)
{
    if (parents.length == 1) {
        return parents[0];
    }
    return Shade._create_concrete_exp({
        expression_name: "seq",
        parents: parents,
        glsl_expression: function(glsl_name) {
            return this.parents.map(function (n) { return n.glsl_expression(); }).join("; ");
        },
        type: Shade.Types.void_t,
        compile: function (ctx) {},
        evaluate: Shade.memoize_on_guid_dict(function(cache) {
            return this.parents[this.parents.length-1].evaluate(cache);
        })
    });
};
Shade.Optimizer = {};

Shade.Optimizer.debug = false;

Shade.Optimizer._debug_passes = false;

Shade.Optimizer.transform_expression = function(operations)
{
    return function(v) {
        var old_v;
        for (var i=0; i<operations.length; ++i) {
            if (Shade.debug && Shade.Optimizer._debug_passes) {
                old_v = v;
            }
            var test = operations[i][0];
            var fun = operations[i][1];
            var old_guid = v.guid;
            if (operations[i][3]) {
                var this_old_guid;
                do {
                    this_old_guid = v.guid;
                    v = v.replace_if(test, fun);
                } while (v.guid !== this_old_guid);
            } else {
                v = v.replace_if(test, fun);
            }
            var new_guid = v.guid;
            if (Shade.debug && Shade.Optimizer._debug_passes &&
                old_guid != new_guid) {
                console.log("Pass",operations[i][2],"succeeded");
                console.log("Before: ");
                old_v.debug_print();
                console.log("After: ");
                v.debug_print();
            }
        }
        return v;
    };
};

Shade.Optimizer.is_constant = function(exp)
{
    return exp.is_constant();
};

Shade.Optimizer.replace_with_constant = function(exp)
{
    var v = exp.constant_value();
    var result = Shade.constant(v, exp.type);
    if (!exp.type.equals(result.type)) {
        throw new Error("Shade.constant internal error: type was not preserved");
    }
    return result;
};

Shade.Optimizer.is_zero = function(exp)
{
    if (!exp.is_constant())
        return false;
    var v = exp.constant_value();
    var t = Shade.Types.type_of(v);
    if (t.is_pod())
        return v === 0;
    if (t.is_vec())
        return _.all(v, function (x) { return x === 0; });
    if (t.is_mat())
        return _.all(v, function (x) { return x === 0; });
    return false;
};

Shade.Optimizer.is_mul_identity = function(exp)
{
    if (!exp.is_constant())
        return false;
    var v = exp.constant_value();
    var t = Shade.Types.type_of(v);
    if (t.is_pod())
        return v === 1;
    if (t.is_vec()) {
        switch (v.length) {
        case 2: return vec.equal(v, vec.make([1,1]));
        case 3: return vec.equal(v, vec.make([1,1,1]));
        case 4: return vec.equal(v, vec.make([1,1,1,1]));
        default:
            throw new Error("bad vec length: " + v.length);
        }
    }
    if (t.is_mat())
        return mat.equal(v, mat[Math.sqrt(v.length)].identity());
    return false;
};

Shade.Optimizer.is_times_zero = function(exp)
{
    return exp.expression_type === 'operator*' &&
        (Shade.Optimizer.is_zero(exp.parents[0]) ||
         Shade.Optimizer.is_zero(exp.parents[1]));
};

Shade.Optimizer.is_plus_zero = function(exp)
{
    return exp.expression_type === 'operator+' &&
        (Shade.Optimizer.is_zero(exp.parents[0]) ||
         Shade.Optimizer.is_zero(exp.parents[1]));
};

Shade.Optimizer.replace_with_nonzero = function(exp)
{
    if (Shade.Optimizer.is_zero(exp.parents[0]))
        return exp.parents[1];
    if (Shade.Optimizer.is_zero(exp.parents[1]))
        return exp.parents[0];
    throw new Error("internal error: no zero value on input to replace_with_nonzero");
};


Shade.Optimizer.is_times_one = function(exp)
{
    if (exp.expression_type !== 'operator*')
        return false;
    var t1 = exp.parents[0].type, t2 = exp.parents[1].type;
    var ft = Shade.Types.float_t;
    if (t1.equals(t2)) {
        return (Shade.Optimizer.is_mul_identity(exp.parents[0]) ||
                Shade.Optimizer.is_mul_identity(exp.parents[1]));
    } else if (!t1.equals(ft) && t2.equals(ft)) {
        return Shade.Optimizer.is_mul_identity(exp.parents[1]);
    } else if (t1.equals(ft) && !t2.equals(ft)) {
        return Shade.Optimizer.is_mul_identity(exp.parents[0]);
    } else if (t1.is_vec() && t2.is_mat()) {
        return Shade.Optimizer.is_mul_identity(exp.parents[1]);
    } else if (t1.is_mat() && t2.is_vec()) {
        return Shade.Optimizer.is_mul_identity(exp.parents[0]);
    } else {
        throw new Error("internal error on Shade.Optimizer.is_times_one");
    }
};

Shade.Optimizer.replace_with_notone = function(exp)
{
    var t1 = exp.parents[0].type, t2 = exp.parents[1].type;
    var ft = Shade.Types.float_t;
    if (t1.equals(t2)) {
        if (Shade.Optimizer.is_mul_identity(exp.parents[0])) {
            return exp.parents[1];
        } else if (Shade.Optimizer.is_mul_identity(exp.parents[1])) {
            return exp.parents[0];
        } else {
            throw new Error("internal error on Shade.Optimizer.replace_with_notone");
        }
    } else if (!t1.equals(ft) && t2.equals(ft)) {
        return exp.parents[0];
    } else if (t1.equals(ft) && !t2.equals(ft)) {
        return exp.parents[1];
    } else if (t1.is_vec() && t2.is_mat()) {
        return exp.parents[0];
    } else if (t1.is_mat() && t2.is_vec()) {
        return exp.parents[1];
    }
    throw new Error("internal error: no is_mul_identity value on input to replace_with_notone");
};

Shade.Optimizer.replace_with_zero = function(x)
{
    if (x.type.equals(Shade.Types.float_t))
        return Shade.constant(0);
    if (x.type.equals(Shade.Types.int_t))
        return Shade.as_int(0);
    if (x.type.equals(Shade.Types.vec2))
        return Shade.constant(vec2.create());
    if (x.type.equals(Shade.Types.vec3))
        return Shade.constant(vec3.create());
    if (x.type.equals(Shade.Types.vec4))
        return Shade.constant(vec4.create());
    if (x.type.equals(Shade.Types.mat2))
        return Shade.constant(mat2.create());
    if (x.type.equals(Shade.Types.mat3))
        return Shade.constant(mat3.create());
    if (x.type.equals(Shade.Types.mat4))
        return Shade.constant(mat4.create());
    throw new Error("internal error: not a type replaceable with zero");
};

Shade.Optimizer.vec_at_constant_index = function(exp)
{
    if (exp.expression_type !== "index")
        return false;
    if (!exp.parents[1].is_constant())
        return false;
    var v = exp.parents[1].constant_value();
    if (lux_typeOf(v) !== "number")
        return false;
    var t = exp.parents[0].type;
    if (t.equals(Shade.Types.vec2) && (v >= 0) && (v <= 1))
        return true;
    if (t.equals(Shade.Types.vec3) && (v >= 0) && (v <= 2))
        return true;
    if (t.equals(Shade.Types.vec4) && (v >= 0) && (v <= 3))
        return true;
    return false;
};

Shade.Optimizer.replace_vec_at_constant_with_swizzle = function(exp)
{
    var v = exp.parents[1].constant_value();
    if (v === 0) return exp.parents[0].swizzle("x");
    if (v === 1) return exp.parents[0].swizzle("y");
    if (v === 2) return exp.parents[0].swizzle("z");
    if (v === 3) return exp.parents[0].swizzle("w");
    throw new Error("internal error on Shade.Optimizer.replace_vec_at_constant_with_swizzle");
};

Shade.Optimizer.is_logical_and_with_constant = function(exp)
{
    return (exp.expression_type === "operator&&" &&
            exp.parents[0].is_constant());
};

Shade.Optimizer.replace_logical_and_with_constant = function(exp)
{
    if (exp.parents[0].constant_value()) {
        return exp.parents[1];
    } else {
        return Shade.make(false);
    }
};

Shade.Optimizer.is_logical_or_with_constant = function(exp)
{
    return (exp.expression_type === "operator||" &&
            exp.parents[0].is_constant());
};

Shade.Optimizer.replace_logical_or_with_constant = function(exp)
{
    if (exp.parents[0].constant_value()) {
        return Shade.make(true);
    } else {
        return exp.parents[1];
    }
};

Shade.Optimizer.is_never_discarding = function(exp)
{
    return (exp.expression_type === "discard_if" &&
            exp.parents[0].is_constant() &&
            !exp.parents[0].constant_value());
};

Shade.Optimizer.remove_discard = function(exp)
{
    return exp.parents[1];
};

Shade.Optimizer.is_known_branch = function(exp)
{
    var result = (exp.expression_type === "ifelse" &&
                  exp.parents[0].is_constant());
    return result;
};

Shade.Optimizer.prune_ifelse_branch = function(exp)
{
    if (exp.parents[0].constant_value()) {
        return exp.parents[1];
    } else {
        return exp.parents[2];
    }
};

// We provide saner names for program targets so users don't
// need to memorize gl_FragColor, gl_Position and gl_PointSize.
//
// However, these names should still work, in case the users
// want to have GLSL-familiar names.
Shade.canonicalize_program_object = function(program_obj)
{
    var result = {};
    var canonicalization_map = {
        'color': 'gl_FragColor',
        'position': 'gl_Position',
        'screen_position': 'gl_Position',
        'point_size': 'gl_PointSize'
    };

    _.each(program_obj, function(v, k) {
        var transposed_key = (k in canonicalization_map) ?
            canonicalization_map[k] : k;
        result[transposed_key] = v;
    });
    return result;
};

//////////////////////////////////////////////////////////////////////////////
/*
 * Shade.program is the main procedure that compiles a Shade
 * appearance object (which is an object with fields containing Shade
 * expressions like 'position' and 'color') to a WebGL program (a pair
 * of vertex and fragment shaders). It performs a variety of optimizations and
 * program transformations to support a more uniform programming model.
 * 
 * The sequence of transformations is as follows:
 * 
 *  - An appearance object is first canonicalized (which transforms names like 
 *    color to gl_FragColor)
 * 
 *  - There are some expressions that are valid in vertex shader contexts but 
 *    invalid in fragment shader contexts, and vice-versa (eg. attributes can 
 *    only be read in vertex shaders; dFdx can only be evaluated in fragment 
 *    shaders; the discard statement can only appear in a fragment shader). 
 *    This means we must move expressions around:
 * 
 *    - expressions that can be hoisted from the vertex shader to the fragment 
 *      shader are hoisted. Currently, this only includes discard_if 
 *      statements.
 * 
 *    - expressions that must be hoisted from the fragment-shader computations 
 *      to the vertex-shader computations are hoisted. For example, WebGL 
 *      attributes can only be read on vertex shaders, and so Shade.program 
 *      introduces a varying variable to communicate the value to the fragment 
 *      shader.
 * 
 *  - At the end of this stage, some fragment-shader only expressions might 
 *    remain on vertex-shader computations. These are invalid WebGL programs and
 *    Shade.program must fail here (The canonical example is: 
 *
 *    {
 *        position: Shade.dFdx(attribute)
 *    })
 * 
 *  - After relocating expressions, vertex and fragment shaders are optimized
 *    using a variety of simple expression rewriting (constant folding, etc).
 */

Shade.program = function(program_obj)
{
    program_obj = Shade.canonicalize_program_object(program_obj);
    var vp_obj = {}, fp_obj = {};

    _.each(program_obj, function(v, k) {
        v = Shade.make(v);
        if (k === 'gl_FragColor') {
            if (!v.type.equals(Shade.Types.vec4)) {
                throw new Error("color attribute must be of type vec4, got " +
                    v.type.repr() + " instead");
            }
            fp_obj.gl_FragColor = v;
        } else if (k === 'gl_Position') {
            if (!v.type.equals(Shade.Types.vec4)) {
                throw new Error("position attribute must be of type vec4, got " +
                    v.type.repr() + " instead");
            }
            vp_obj.gl_Position = v;
        } else if (k === 'gl_PointSize') {
            if (!v.type.equals(Shade.Types.float_t)) {
                throw new Error("color attribute must be of type float, got " +
                    v.type.repr() + " instead");
            }
            vp_obj.gl_PointSize = v;
        } else if (k.substr(0, 3) === 'gl_') {
            // FIXME: Can we sensibly work around these?
            throw new Error("gl_* are reserved GLSL names");
        } else
            vp_obj[k] = v;
    });

    var vp_compile = Shade.CompilationContext(Shade.VERTEX_PROGRAM_COMPILE),
        fp_compile = Shade.CompilationContext(Shade.FRAGMENT_PROGRAM_COMPILE);

    var vp_exprs = [], fp_exprs = [];

    function is_attribute(x) {
        return x.expression_type === 'attribute';
    }
    function is_varying(x) {
        return x.expression_type === 'varying';
    }
    function is_per_vertex(x) {
        return x.stage === 'vertex';
    }
    var varying_names = [];
    function hoist_to_varying(exp) {
        var varying_name = Shade.unique_name();
        vp_obj[varying_name] = exp;
        varying_names.push(varying_name);
        var result = Shade.varying(varying_name, exp.type);
        if (exp._must_be_function_call) {
            result._must_be_function_call = true;
        }
        return result;
    }

    //////////////////////////////////////////////////////////////////////////
    // moving discard statements on vertex program to fragment program

    var shade_values_vp_obj = Shade(_.object(_.filter(
        _.pairs(vp_obj), function(lst) {
            var k = lst[0], v = lst[1];
            return Lux.is_shade_expression(v);
        })));

    var vp_discard_conditions = {};
    shade_values_vp_obj = shade_values_vp_obj.replace_if(function(x) {
        return x.expression_type === 'discard_if';
    }, function(exp) {
        vp_discard_conditions[exp.parents[1].guid] = exp.parents[1];
        return exp.parents[0];
    });

    var disallowed_vertex_expressions = shade_values_vp_obj.find_if(function(x) {
        if (x.expression_type === 'builtin_function{dFdx}') return true;
        if (x.expression_type === 'builtin_function{dFdy}') return true;
        if (x.expression_type === 'builtin_function{fwidth}') return true;
        return false;
    });
    if (disallowed_vertex_expressions.length > 0) {
        throw "'" + disallowed_vertex_expressions[0] + "' not allowed in vertex expression";
    }

    vp_obj = _.object(shade_values_vp_obj.fields, shade_values_vp_obj.parents);
    vp_discard_conditions = _.values(vp_discard_conditions);

    if (vp_discard_conditions.length) {
        var vp_discard_condition = _.reduce(vp_discard_conditions, function(a, b) {
            return a.or(b);
        }).ifelse(1, 0).gt(0);
        fp_obj.gl_FragColor = fp_obj.gl_FragColor.discard_if(vp_discard_condition);
    }

    

    var common_sequence = [
        [Shade.Optimizer.is_times_zero, Shade.Optimizer.replace_with_zero, 
         "v * 0", true],
        [Shade.Optimizer.is_times_one, Shade.Optimizer.replace_with_notone, 
         "v * 1", true],
        [Shade.Optimizer.is_plus_zero, Shade.Optimizer.replace_with_nonzero,
         "v + 0", true],
        [Shade.Optimizer.is_never_discarding,
         Shade.Optimizer.remove_discard, "discard_if(false)"],
        [Shade.Optimizer.is_known_branch,
         Shade.Optimizer.prune_ifelse_branch, "constant?a:b", true],
        [Shade.Optimizer.vec_at_constant_index, 
         Shade.Optimizer.replace_vec_at_constant_with_swizzle, "vec[constant_ix]"],
        [Shade.Optimizer.is_constant,
         Shade.Optimizer.replace_with_constant, "constant folding"],
        [Shade.Optimizer.is_logical_or_with_constant,
         Shade.Optimizer.replace_logical_or_with_constant, "constant||v", true],
        [Shade.Optimizer.is_logical_and_with_constant,
         Shade.Optimizer.replace_logical_and_with_constant, "constant&&v", true]];

    // explicit per-vertex hoisting must happen before is_attribute hoisting,
    // otherwise we might end up reading from a varying in the vertex program,
    // which is undefined behavior
    var fp_sequence = [
        [is_per_vertex, hoist_to_varying, "per-vertex hoisting"],
        [is_attribute, hoist_to_varying, "attribute hoisting"]  
    ];
    fp_sequence.push.apply(fp_sequence, common_sequence);
    var vp_sequence = common_sequence;
    var fp_optimize = Shade.Optimizer.transform_expression(fp_sequence);
    var vp_optimize = Shade.Optimizer.transform_expression(vp_sequence);

    var used_varying_names = [];
    _.each(fp_obj, function(v, k) {
        try {
            v = fp_optimize(v);
        } catch (e) {
            console.error("fragment program optimization crashed. This is a bug. Please send the following JSON object in the bug report:");
            console.error(JSON.stringify(v.json()));
            throw e;
        }
        used_varying_names.push.apply(used_varying_names,
                                      _.map(v.find_if(is_varying),
                                            function (v) { 
                                                return v._varying_name;
                                            }));
        fp_exprs.push(Shade.set(v, k));
    });

    _.each(vp_obj, function(v, k) {
        var new_v;
        if ((varying_names.indexOf(k) === -1) ||
            (used_varying_names.indexOf(k) !== -1)) {
            try {
                new_v = vp_optimize(v);
            } catch (e) {
                console.error("vertex program optimization crashed. This is a bug. Please send the following JSON object in the bug report:");
                console.error(JSON.stringify(v.json()));
                throw e;
            }
            vp_exprs.push(Shade.set(new_v, k));
        }
    });

    var vp_exp = Shade.seq(vp_exprs);
    var fp_exp = Shade.seq(fp_exprs);

    vp_compile.compile(vp_exp);
    fp_compile.compile(fp_exp);
    var vp_source = vp_compile.source(),
        fp_source = fp_compile.source();
    if (Shade.debug) {
        if (Shade.debug && Shade.Optimizer._debug_passes) {
            console.log("Vertex program final AST:");
            vp_exp.debug_print();
        }
        console.log("Vertex program source:");
        console.log(vp_source);
        // vp_exp.debug_print();
        
        if (Shade.debug && Shade.Optimizer._debug_passes) {
            console.log("Fragment program final AST:");
            fp_exp.debug_print();
        }
        console.log("Fragment program source:");
        console.log(fp_source);
        // fp_exp.debug_print();
    }
    var result = Lux.program(vp_source, fp_source);
    result.attribute_buffers = vp_exp.attribute_buffers();
    result.uniforms = _.union(vp_exp.uniforms(), fp_exp.uniforms());
    return result;
};
Shade.round = Shade.make(function(v) {
    return v.add(0.5).floor();
});
Shade.Exp.round = function() { return Shade.round(this); };
Shade.Utils = {};
// given a list of values, returns a function which, when given a
// value between 0 and 1, returns the appropriate linearly interpolated
// value.

// Hat function reconstruction

Shade.Utils.lerp = function(lst) {
    var new_lst = _.toArray(lst);
    new_lst.push(new_lst[new_lst.length-1]);
    // repeat last to make index calc easier
    return function(v) {
        var colors_exp = Shade.array(new_lst);
        v = Shade.clamp(v, 0, 1).mul(new_lst.length-2);
        var u = v.fract();
        var ix = v.floor();
        return Shade.mix(colors_exp.at(ix),
                         colors_exp.at(ix.add(1)),
                         u);
    };
};
// given a list of values, returns a function which, when given a
// value between 0 and l, returns the value of the index;

// box function reconstruction

Shade.Utils.choose = function(lst) {
    var new_lst = _.toArray(lst);
    var vals_exp = Shade.array(new_lst);
    return function(v) {
        v = Shade.clamp(v, 0, new_lst.length-1).floor().as_int();
        return vals_exp.at(v);
    };
};
// FIXME remove from API
Shade.Utils.linear = function(f1, f2, t1, t2)
{
    console.log("Shade.Utils.linear is deprecated; use Shade.Scale.linear instead");
    var df = Shade.sub(f2, f1), dt = Shade.sub(t2, t1);
    return function(x) {
        return Shade.make(x).sub(f1).mul(dt.div(df)).add(t1);
    };
};
// returns a linear transformation of the coordinates such that the given list of values
// fits between [0, 1]

Shade.Utils.fit = function(data) {
    // this makes float attribute buffers work, but it might be confusing to the
    // user that there exist values v for which Shade.Utils.fit(v) works,
    // but Shade.Utils.fit(Shade.make(v)) does not
    var t = data._shade_type;
    if (t === 'attribute_buffer') {
        if (data.itemSize !== 1)
            throw new Error("only dimension-1 attribute buffers are supported");
        if (_.isUndefined(data.array))
            throw new Error("Shade.Utils.fit on attribute buffers requires keep_array:true in options");
        data = data.array;
    }

    var min = _.min(data), max = _.max(data);
    return Shade.Scale.linear({domain: [min, max]},
                              {range: [0, 1]});
};
// replicates OpenGL's fog functionality

(function() {

var default_color = Shade.vec(0,0,0,0);

Shade.gl_fog = function(opts)
{
    opts = _.defaults(opts, { mode: "exp",
                              density: 1,
                              start: 0,
                              end: 1,
                              fog_color: default_color,
                              per_vertex: false
                            });
    var mode = opts.mode || "exp";
    var fog_color = Shade.make(opts.fog_color);
    var color = opts.color;
    var z = Shade.make(opts.z);
    var f, density, start;

    if (opts.mode === "exp") {
        density = Shade.make(opts.density);
        start = Shade.make(opts.start);
        f = z.sub(start).mul(density).exp();
    } else if (mode === "exp2") {
        density = Shade.make(opts.density);
        start = Shade.make(opts.start);
        f = z.sub(start).min(0).mul(density);
        f = f.mul(f);
        f = f.neg().exp();
    } else if (mode === "linear") {
        start = Shade.make(opts.start);
        var end = Shade.make(opts.end);
        end = Shade.make(end);
        start = Shade.make(start);
        f = end.sub(z).div(end.sub(start));
    }
    f = f.clamp(0, 1);
    if (opts.per_vertex)
        f = f.per_vertex();
    return Shade.mix(fog_color, color, f);
};

})();
Shade.cosh = Shade(function(v)
{
    return v.exp().add(v.neg().exp()).div(2);
});
Shade.Exp.cosh = function() { return Shade.cosh(this); };
Shade.sinh = Shade(function(v)
{
    return v.exp().sub(v.neg().exp()).div(2);
});
Shade.Exp.sinh = function() { return Shade.sinh(this); };
Shade.tanh = Shade(function(v)
{
    return v.sinh().div(v.cosh());
});
Shade.Exp.tanh = function() { return Shade.tanh(this); };
(function() {

var logical_operator_binexp = function(exp1, exp2, operator_name, evaluator,
                                       parent_is_unconditional, shade_name)
{
    parent_is_unconditional = parent_is_unconditional ||
        function (i) { return true; };
    return Shade._create_concrete_value_exp({
        parents: [exp1, exp2],
        type: Shade.Types.bool_t,
        expression_type: "operator" + operator_name,
        value: function() {
            return "(" + this.parents[0].glsl_expression() + " " + operator_name + " " +
                this.parents[1].glsl_expression() + ")";
        },
        evaluate: Shade.memoize_on_guid_dict(function(cache) {
            return evaluator(this, cache);
        }),
        parent_is_unconditional: parent_is_unconditional,
        _json_key: function() { return shade_name; }
    });
};

var lift_binfun_to_evaluator = function(binfun) {
    return function(exp, cache) {
        var exp1 = exp.parents[0], exp2 = exp.parents[1];
        return binfun(exp1.evaluate(cache), exp2.evaluate(cache));
    };
};

var logical_operator_exp = function(operator_name, binary_evaluator,
                                    parent_is_unconditional, shade_name)
{
    return function() {
        if (arguments.length === 0) 
            throw new Error("operator " + operator_name 
                   + " requires at least 1 parameter");
        if (arguments.length === 1) return Shade(arguments[0]).as_bool();
        var first = Shade(arguments[0]);
        if (!first.type.equals(Shade.Types.bool_t))
            throw new Error("operator " + operator_name + 
                   " requires booleans, got argument 1 as " +
                   arguments[0].type.repr() + " instead.");
        var current_result = first;
        for (var i=1; i<arguments.length; ++i) {
            var next = Shade(arguments[i]);
            if (!next.type.equals(Shade.Types.bool_t))
                throw new Error("operator " + operator_name + 
                       " requires booleans, got argument " + (i+1) +
                       " as " + next.type.repr() + " instead.");
            current_result = logical_operator_binexp(
                current_result, next,
                operator_name, binary_evaluator,
                parent_is_unconditional, shade_name);
        }
        return current_result;
    };
};

Shade.or = logical_operator_exp(
    "||", lift_binfun_to_evaluator(function(a, b) { return a || b; }),
    function(i) { return i === 0; }, "or"
);

Shade.Exp.or = function(other)
{
    return Shade.or(this, other);
};

Shade.and = logical_operator_exp(
    "&&", lift_binfun_to_evaluator(function(a, b) { return a && b; }),
    function(i) { return i === 0; }, "and"
);

Shade.Exp.and = function(other)
{
    return Shade.and(this, other);
};

Shade.xor = logical_operator_exp(
    "^^", lift_binfun_to_evaluator(function(a, b) { return ~~(a ^ b); }), undefined, "xor");
Shade.Exp.xor = function(other)
{
    return Shade.xor(this, other);
};

Shade.not = Shade(function(exp)
{
    if (!exp.type.equals(Shade.Types.bool_t)) {
        throw new Error("logical_not requires bool expression");
    }
    return Shade._create_concrete_value_exp({
        parents: [exp],
        type: Shade.Types.bool_t,
        expression_type: "operator!",
        value: function() {
            return "(!" + this.parents[0].glsl_expression() + ")";
        },
        evaluate: Shade.memoize_on_guid_dict(function(cache) {
            return !this.parents[0].evaluate(cache);
        }),
        _json_key: function() { return "not"; }
    });
});

Shade.Exp.not = function() { return Shade.not(this); };

var comparison_operator_exp = function(operator_name, type_checker, binary_evaluator, shade_name)
{
    return Shade(function(first, second) {
        type_checker(first.type, second.type);

        return logical_operator_binexp(
            first, second, operator_name, binary_evaluator, undefined, shade_name);
    });
};

var inequality_type_checker = function(name) {
    return function(t1, t2) {
        if (!(t1.equals(Shade.Types.float_t) && 
              t2.equals(Shade.Types.float_t)) &&
            !(t1.equals(Shade.Types.int_t) && 
              t2.equals(Shade.Types.int_t)))
            throw new Error("operator" + name + 
                   " requires two ints or two floats, got " +
                   t1.repr() + " and " + t2.repr() +
                   " instead.");
    };
};

var equality_type_checker = function(name) {
    return function(t1, t2) {
        if (!t1.equals(t2))
            throw new Error("operator" + name +
                   " requires same types, got " +
                   t1.repr() + " and " + t2.repr() +
                   " instead.");
        if (t1.is_array() && !t1.is_vec() && !t1.is_mat())
            throw new Error("operator" + name +
                   " does not support arrays");
    };
};

Shade.lt = comparison_operator_exp("<", inequality_type_checker("<"),
    lift_binfun_to_evaluator(function(a, b) { return a < b; }), "lt");
Shade.Exp.lt = function(other) { return Shade.lt(this, other); };

Shade.le = comparison_operator_exp("<=", inequality_type_checker("<="),
    lift_binfun_to_evaluator(function(a, b) { return a <= b; }), "le");
Shade.Exp.le = function(other) { return Shade.le(this, other); };

Shade.gt = comparison_operator_exp(">", inequality_type_checker(">"),
    lift_binfun_to_evaluator(function(a, b) { return a > b; }), "gt");
Shade.Exp.gt = function(other) { return Shade.gt(this, other); };

Shade.ge = comparison_operator_exp(">=", inequality_type_checker(">="),
    lift_binfun_to_evaluator(function(a, b) { return a >= b; }), "ge");
Shade.Exp.ge = function(other) { return Shade.ge(this, other); };

Shade.eq = comparison_operator_exp("==", equality_type_checker("=="),
    lift_binfun_to_evaluator(function(a, b) {
        if (lux_typeOf(a) === 'array') {
            return _.all(_.map(_.zip(a, b),
                               function(v) { return v[0] === v[1]; }),
                         function (x) { return x; });
        }
        return Shade.Types.type_of(a).value_equals(a, b);
    }), "eq");
Shade.Exp.eq = function(other) { return Shade.eq(this, other); };

Shade.ne = comparison_operator_exp("!=", equality_type_checker("!="),
    lift_binfun_to_evaluator(function(a, b) { 
        if (lux_typeOf(a) === 'array') {
            return _.any(_.map(_.zip(a, b),
                               function(v) { return v[0] !== v[1]; } ),
                         function (x) { return x; });
        }
        return !Shade.Types.type_of(a).value_equals(a, b);
    }), "ne");
Shade.Exp.ne = function(other) { return Shade.ne(this, other); };

// component-wise comparisons are defined on builtins.js

})();
Shade.ifelse = function(condition, if_true, if_false)
{
    condition = Shade.make(condition);
    if_true = Shade.make(if_true);
    if_false = Shade.make(if_false);

    if (!if_true.type.equals(if_false.type))
        throw new Error("ifelse return expressions must have same types");
    if (!condition.type.equals(condition.type))
        throw new Error("ifelse condition must be of type bool");

    return Shade._create_concrete_value_exp( {
        parents: [condition, if_true, if_false],
        type: if_true.type,
        expression_type: "ifelse",
        // FIXME: works around Chrome Bug ID 103053
        _must_be_function_call: true,
        value: function() {
            return "(" + this.parents[0].glsl_expression() + "?"
                + this.parents[1].glsl_expression() + ":"
                + this.parents[2].glsl_expression() + ")";
        },
        /*
         * The methods is_constant(), constant_value() and evaluate() for
         * Shade.ifelse are designed to handle cases like the following:
         * 
         * Shade.ifelse(Shade.parameter("bool"), 3, 3).is_constant()
         * 
         * That expression should be true.
         * 
         */ 
        constant_value: function() {
            if (!this.parents[0].is_constant()) {
                // This only gets called when this.is_constant() holds, so
                // it must be that this.parents[1].constant_value() == 
                // this.parents[2].constant_value(); we return either
                return this.parents[1].constant_value();
            } else {
                return (this.parents[0].constant_value() ?
                        this.parents[1].constant_value() :
                        this.parents[2].constant_value());
            }
        },
        evaluate: Shade.memoize_on_guid_dict(function(cache) {
            if (this.parents[1].is_constant() &&
                this.parents[2].is_constant() &&
                this.type.value_equals(this.parents[1].constant_value(),
                                       this.parents[2].constant_value())) {
                // if both sides of the branch have the same value, then
                // this evaluates to the constant, regardless of the condition.
                return this.parents[1].constant_value();
            } else {
                return this.parents[0].evaluate(cache)?
                    this.parents[1].evaluate(cache):
                    this.parents[2].evaluate(cache);
            };
        }),
        is_constant: function() {
            if (!this.parents[0].is_constant()) {
                // if condition is not constant, 
                // then expression is only constant if sides always
                // evaluate to same values.
                if (this.parents[1].is_constant() && 
                    this.parents[2].is_constant()) {
                    var v1 = this.parents[1].constant_value();
                    var v2 = this.parents[2].constant_value();
                    return this.type.value_equals(v1, v2);
                } else {
                    return false;
                }
            } else {
                // if condition is constant, then
                // the expression is constant if the appropriate
                // side of the evaluation is constant.
                return (this.parents[0].constant_value() ?
                        this.parents[1].is_constant() :
                        this.parents[2].is_constant());
            }
        },
        element: function(i) {
            return Shade.ifelse(this.parents[0],
                                   this.parents[1].element(i),
                                   this.parents[2].element(i));
        },
        element_constant_value: function(i) {
            if (!this.parents[0].is_constant()) {
                // This only gets called when this.is_constant() holds, so
                // it must be that this.parents[1].constant_value() == 
                // this.parents[2].constant_value(); we return either
                return this.parents[1].element_constant_value(i);
            } else {
                return (this.parents[0].constant_value() ?
                        this.parents[1].element_constant_value(i) :
                        this.parents[2].element_constant_value(i));
            }
        },
        element_is_constant: function(i) {
            if (!this.parents[0].is_constant()) {
                // if condition is not constant, 
                // then expression is only constant if sides always
                // evaluate to same values.
                if (this.parents[1].element_is_constant(i) && 
                    this.parents[2].element_is_constant(i)) {
                    var v1 = this.parents[1].element_constant_value(i);
                    var v2 = this.parents[2].element_constant_value(i);
                    return this.type.element_type(i).value_equals(v1, v2);
                } else {
                    return false;
                }
            } else {
                // if condition is constant, then
                // the expression is constant if the appropriate
                // side of the evaluation is constant.
                return (this.parents[0].constant_value() ?
                        this.parents[1].element_is_constant(i) :
                        this.parents[2].element_is_constant(i));
            }
        },
        parent_is_unconditional: function(i) {
            return i === 0;
        }
    });
};

Shade.Exp.ifelse = function(if_true, if_false)
{
    return Shade.ifelse(this, if_true, if_false);
};
// FIXME This should be Shade.rotation = Shade.make(function() ...
// but before I do that I have to make sure that at this point
// in the source Shade.make actually exists.

Shade.rotation = Shade(function(angle, axis)
{
    if (axis.type.equals(Shade.Types.vec4))
        axis = axis.swizzle("xyz");
    axis = axis.normalize();

    var s = angle.sin(), c = angle.cos(), t = Shade.sub(1, c);
    var x = axis.at(0), y = axis.at(1), z = axis.at(2);

    return Shade.mat(Shade.vec(x.mul(x).mul(t).add(c),
                               y.mul(x).mul(t).add(z.mul(s)),
                               z.mul(x).mul(t).sub(y.mul(s)),
                               0),
                     Shade.vec(x.mul(y).mul(t).sub(z.mul(s)),
                               y.mul(y).mul(t).add(c),
                               z.mul(y).mul(t).add(x.mul(s)),
                               0),
                     Shade.vec(x.mul(z).mul(t).add(y.mul(s)),
                               y.mul(z).mul(t).sub(x.mul(s)),
                               z.mul(z).mul(t).add(c),
                               0),
                     Shade.vec(0,0,0,1));
});
Shade.translation = Shade(function() {
    function from_vec3(v) {
        return Shade.mat(Shade.vec(1,0,0,0),
                         Shade.vec(0,1,0,0),
                         Shade.vec(0,0,1,0),
                         Shade.vec(v, 1));
    }
    if (arguments.length === 1) {
        var t = arguments[0];
        if (!t.type.equals(Shade.Types.vec3)) {
            throw new Error("expected vec3, got " + t.type.repr() + "instead");
        }
        return from_vec3(t);
    } else if (arguments.length === 2) {
        var x = arguments[0], y = arguments[1];
        if (!x.type.equals(Shade.Types.float_t)) {
            throw new Error("expected float, got " + x.type.repr() + "instead");
        }
        if (!y.type.equals(Shade.Types.float_t)) {
            throw new Error("expected float, got " + y.type.repr() + "instead");
        }
        return from_vec3(Shade.vec(x, y, 0));
    } else if (arguments.length === 3) {
        var x = arguments[0], y = arguments[1], z = arguments[2];
        if (!x.type.equals(Shade.Types.float_t)) {
            throw new Error("expected float, got " + x.type.repr() + "instead");
        }
        if (!y.type.equals(Shade.Types.float_t)) {
            throw new Error("expected float, got " + y.type.repr() + "instead");
        }
        if (!z.type.equals(Shade.Types.float_t)) {
            throw new Error("expected float, got " + z.type.repr() + "instead");
        }
        return from_vec3(Shade.vec(x, y, z));
    } else
        throw new Error("expected either 1, 2 or 3 parameters");
});
Shade.scaling = Shade(function() {
    function build(v1, v2, v3) {
        return Shade.mat(Shade.vec(v1, 0, 0, 0),
                         Shade.vec( 0,v2, 0, 0),
                         Shade.vec( 0, 0,v3, 0),
                         Shade.vec( 0, 0, 0, 1));
    }
    if (arguments.length === 1) {
        var t = arguments[0];
        if (t.type.equals(Shade.Types.float_t))
            return build(t, t, t);
        if (t.type.equals(Shade.Types.vec3))
            return build(t.x(), t.y(), t.z());
        throw new Error("expected float or vec3, got " + t.type.repr() + " instead");
    } else if (arguments.length === 3) {
        return build(arguments[0], arguments[1], arguments[2]);
    } else {
        throw new Error("expected one or three parameters, got " + arguments.length + " instead");
    }
});
Shade.ortho = Shade.make(function(left, right, bottom, top, near, far) {
    var rl = right.sub(left);
    var tb = top.sub(bottom);
    var fn = far.sub(near);
    return Shade.mat(Shade.vec(Shade.div(2, rl), 0, 0, 0),
                     Shade.vec(0, Shade.div(2, tb), 0, 0),
                     Shade.vec(0, 0, Shade.div(-2, fn), 0),
                     Shade.vec(Shade.add(right, left).neg().div(rl),
                               Shade.add(top, bottom).neg().div(tb),
                               Shade.add(far, near).neg().div(fn),
                               1));
});
// FIXME This should be Shade.look_at = Shade.make(function() ...
// but before I do that I have to make sure that at this point
// in the source Shade.make actually exists.

Shade.look_at = function(eye, center, up)
{
    eye = Shade.make(eye);
    center = Shade.make(center);
    up = Shade.make(up);

    var z = eye.sub(center).normalize();
    var x = up.cross(z).normalize();
    var y = up.normalize();
    // var y = z.cross(x).normalize();

    return Shade.mat(Shade.vec(x, 0),
                     Shade.vec(y, 0),
                     Shade.vec(z, 0),
                     Shade.vec(x.dot(eye).neg(),
                               y.dot(eye).neg(),
                               z.dot(eye).neg(),
                               1));
};
/*
 * Shade.discard_if: conditionally discard fragments from the pipeline
 * 

*********************************************************************************
 * 
 * For future reference, this is a copy of the org discussion on the
 * discard statement as I was designing it.
 * 

Discard is a statement; I don't really have statements in the
language.


*** discard is fragment-only.

How do I implement discard in a vertex shader?

**** Possibilities:
***** Disallow it to happen in the vertex shader
Good: Simplest
Bad: Breaks the model in Lux programs where we don't care much about
what happens in vertex expressions vs fragment expressions
Ugly: The error messages would be really opaque, unless I specifically
detect where the discard statement would appear.
***** Send the vertex outside the homogenous cube
Good: Simple
Bad: doesn't discard the whole primitive
Ugly: would make triangles, etc look really weird.
***** Set some special varying which discards every single fragment in the shader
Good: Discards an entire primitive.
Bad: Wastes a varying, which might be a scarce resource.
Ugly: varying cannot be discrete (bool). The solution would be to
discard if varying is greater than zero, set the discarded varying to be greater
than the largest possible distance between two vertices on the screen,
and the non-discarded to zero.

*** Implementation ideas:

**** special key for the program description

like so:

{
  gl_Position: foo
  gl_FragColor: bar
  discard_if: baz
}

The main disadvantage here is that one application of discard is to
save computation time. This means that my current initialization of
variables used in more than one context will be wasteful if none of
these variables are actually used before the discard condition is
verified. What I would need, then, is some dependency analysis that
determines which variables are used for which discard checks, and
computes those in the correct order.

This discard interacts with the initializer code.

**** new expression called discard_if

We add a discard_when(condition, value_if_not) expression, which
issues the discard statement if condition is true. 

But what about discard_when being executed inside conditional
expressions? Worse: discard_when would turn case D above from a
performance problem into an actual bug.

 * 
 */

Shade.discard_if = function(exp, condition)
{
    if (_.isUndefined(exp) ||
        _.isUndefined(condition))
        throw new Error("discard_if expects two parameters");
    exp = Shade.make(exp);
    condition = Shade.make(condition);

    var result = Shade._create_concrete_exp({
        is_constant: Shade.memoize_on_field("_is_constant", function() {
            var cond = _.all(this.parents, function(v) {
                return v.is_constant();
            });
            return (cond && !this.parents[1].constant_value());
        }),
        _must_be_function_call: true,
        type: exp.type,
        expression_type: "discard_if",
        parents: [exp, condition],
        parent_is_unconditional: function(i) {
            return i === 0;
        },
        compile: function(ctx) {
            ctx.strings.push(this.parents[0].type.repr(), this.glsl_name, "(void) {\n",
                             "    if (",this.parents[1].glsl_expression(),") discard;\n",
                             "    return ", this.parents[0].glsl_expression(), ";\n}\n");
        },
        // FIXME How does evaluate interact with fragment discarding?
        // I still need to define the value of a discarded fragment. Currently evaluate
        // on fragment-varying expressions is undefined anyway, so we punt.
        evaluate: function(cache) {
            return exp.evaluate(cache);
        }
    });
    return result;
};
// converts a 32-bit integer into an 8-bit RGBA value.
// this is most useful for picking.

// Ideally we would like this to take shade expressions,
// but WebGL does not support bitwise operators.

Shade.id = function(id_value)
{
    var r = id_value & 255;
    var g = (id_value >> 8) & 255;
    var b = (id_value >> 16) & 255;
    var a = (id_value >> 24) & 255;
    
    return vec4.make([r / 255, g / 255, b / 255, a / 255]);
};

Shade.shade_id = Shade(function(id_value)
{
    return id_value.div(Shade.vec(1, 256, 65536, 16777216)).mod(256).floor().div(255);
});
Shade.frustum = Shade.make(function(left, right, bottom, top, near, far)
{
    var rl = right.sub(left);
    var tb = top.sub(bottom);
    var fn = far.sub(near);
    return Shade.mat(Shade.vec(near.mul(2).div(rl), 0, 0, 0),
                     Shade.vec(0, near.mul(2).div(tb), 0, 0),
                     Shade.vec(right.add(left).div(rl), 
                               top.add(bottom).div(tb), 
                               far.add(near).neg().div(fn),
                               -1),
                     Shade.vec(0, 0, far.mul(near).mul(2).neg().div(fn), 0));
});
Shade.perspective_matrix = Shade.make(function(fovy, aspect, near, far)
{
    var top = near.mul(Shade.tan(fovy.mul(Math.PI / 360)));
    var right = top.mul(aspect);
    return Shade.frustum(right.neg(), right, top.neg(), top, near, far);
});

return Shade;
}());
////////////////////////////////////////////////////////////////////////////////
// The colorspace conversion routines are based on
// Ross Ihaka's colorspace library for R.

Shade.Colors = {};
Shade.Colors.alpha = function(color, alpha)
{
    color = Shade.make(color);
    alpha = Shade.make(alpha);
    if (!alpha.type.equals(Shade.Types.float_t))
        throw new Error("alpha parameter must be float");
    if (color.type.equals(Shade.Types.vec4)) {
        return Shade.vec(color.swizzle("rgb"), alpha);
    }
    if (color.type.equals(Shade.Types.vec3)) {
        return Shade.vec(color, alpha);
    }
    throw new Error("color parameter must be vec3 or vec4");
};

Shade.Exp.alpha = function(alpha)
{
    return Shade.Colors.alpha(this, alpha);
};
Shade.Colors.Brewer = {};

(function() {

var schemes = {
    "qualitative":{"Accent":[[127,201,127],[190,174,212],[253,192,134],[255,255,153],[56,108,176],[240,2,127],[191,91,23],[102,102,102]],
                   "Dark2":[[27,158,119],[217,95,2],[117,112,179],[231,41,138],[102,166,30],[230,171,2],[166,118,29],[102,102,102]],
	           "Paired":[[166,206,227],[31,120,180],[178,223,138],[51,160,44],[251,154,153],[227,26,28],[253,191,111],[255,127,0],[202,178,214],[106,61,154],[255,255,153],[177,89,40]],
	           "Pastel1":[[251,180,174],[179,205,227],[204,235,197],[222,203,228],[254,217,166],[255,255,204],[229,216,189],[253,218,236],[242,242,242]],
                   "Pastel2":[[179,226,205],[253,205,172],[203,213,232],[244,202,228],[230,245,201],[255,242,174],[241,226,204],[204,204,204]],
	           "Set1":[[228,26,28],[55,126,184],[77,175,74],[152,78,163],[255,127,0],[255,255,51],[166,86,40],[247,129,191],[153,153,153]],
	           "Set2":[[102,194,165],[252,141,98],[141,160,203],[231,138,195],[166,216,84],[255,217,47],[229,196,148],[179,179,179]],
	           "Set3":[[141,211,199],[255,255,179],[190,186,218],[251,128,114],[128,177,211],[253,180,98],[179,222,105],[252,205,229],[217,217,217],[188,128,189],[204,235,197],[255,237,111]]},
    "sequential":{"Blues":[[247,251,255],[222,235,247],[198,219,239],[158,202,225],[107,174,214],[66,146,198],[33,113,181],[8,81,156],[8,48,107]],
                  "BuGn":[[247,252,253],[229,245,249],[204,236,230],[153,216,201],[102,194,164],[65,174,118],[35,139,69],[0,109,44],[0,68,27]],
                  "BuPu":[[247,252,253],[224,236,244],[191,211,230],[158,188,218],[140,150,198],[140,107,177],[136,65,157],[129,15,124],[77,0,75]],
                  "GnBu":[[247,252,240],[224,243,219],[204,235,197],[168,221,181],[123,204,196],[78,179,211],[43,140,190],[8,104,172],[8,64,129]],
                  "Greens":[[247,252,245],[229,245,224],[199,233,192],[161,217,155],[116,196,118],[65,171,93],[35,139,69],[0,109,44],[0,68,27]],
                  "Greys":[[255,255,255],[240,240,240],[217,217,217],[189,189,189],[150,150,150],[115,115,115],[82,82,82],[37,37,37],[0,0,0]],
                  "Oranges":[[255,245,235],[254,230,206],[253,208,162],[253,174,107],[253,141,60],[241,105,19],[217,72,1],[166,54,3],[127,39,4]],
                  "OrRd":[[255,247,236],[254,232,200],[253,212,158],[253,187,132],[252,141,89],[239,101,72],[215,48,31],[179,0,0],[127,0,0]],
                  "PuBu":[[255,247,251],[236,231,242],[208,209,230],[166,189,219],[116,169,207],[54,144,192],[5,112,176],[4,90,141],[2,56,88]],
                  "PuBuGn":[[255,247,251],[236,226,240],[208,209,230],[166,189,219],[103,169,207],[54,144,192],[2,129,138],[1,108,89],[1,70,54]],
                  "PuRd":[[247,244,249],[231,225,239],[212,185,218],[201,148,199],[223,101,176],[231,41,138],[206,18,86],[152,0,67],[103,0,31]],
                  "Purples":[[252,251,253],[239,237,245],[218,218,235],[188,189,220],[158,154,200],[128,125,186],[106,81,163],[84,39,143],[63,0,125]],
                  "RdPu":[[255,247,243],[253,224,221],[252,197,192],[250,159,181],[247,104,161],[221,52,151],[174,1,126],[122,1,119],[73,0,106]],
	          "Reds":[[255,245,240],[254,224,210],[252,187,161],[252,146,114],[251,106,74],[239,59,44],[203,24,29],[165,15,21],[103,0,13]],
                  "YlGn":[[255,255,229],[247,252,185],[217,240,163],[173,221,142],[120,198,121],[65,171,93],[35,132,67],[0,104,55],[0,69,41]],
	          "YlGnBu":[[255,255,217],[237,248,177],[199,233,180],[127,205,187],[65,182,196],[29,145,192],[34,94,168],[37,52,148],[8,29,88]],
	          "YlOrBr":[[255,255,229],[255,247,188],[254,227,145],[254,196,79],[254,153,41],[236,112,20],[204,76,2],[153,52,4],[102,37,6]],
	          "YlOrRd":[[255,255,204],[255,237,160],[254,217,118],[254,178,76],[253,141,60],[252,78,42],[227,26,28],[189,0,38],[128,0,38]]},
    "divergent":{"BrBG":[[84,48,5],[140,81,10],[191,129,45],[223,194,125],[246,232,195],[245,245,245],[199,234,229],[128,205,193],[53,151,143],[1,102,94],[0,60,48]],
                 "PiYG":[[142,1,82],[197,27,125],[222,119,174],[241,182,218],[253,224,239],[247,247,247],[230,245,208],[184,225,134],[127,188,65],[77,146,33],[39,100,25]],
	         "PRGn":[[64,0,75],[118,42,131],[153,112,171],[194,165,207],[231,212,232],[247,247,247],[217,240,211],[166,219,160],[90,174,97],[27,120,55],[0,68,27]],
                 "PuOr":[[127,59,8],[179,88,6],[224,130,20],[253,184,99],[254,224,182],[247,247,247],[216,218,235],[178,171,210],[128,115,172],[84,39,136],[45,0,75]],
	         "RdBu":[[103,0,31],[178,24,43],[214,96,77],[244,165,130],[253,219,199],[247,247,247],[209,229,240],[146,197,222],[67,147,195],[33,102,172],[5,48,97]],
	         "RdGy":[[103,0,31],[178,24,43],[214,96,77],[244,165,130],[253,219,199],[255,255,255],[224,224,224],[186,186,186],[135,135,135],[77,77,77],[26,26,26]],
	         "RdYlBu":[[165,0,38],[215,48,39],[244,109,67],[253,174,97],[254,224,144],[255,255,191],[224,243,248],[171,217,233],[116,173,209],[69,117,180],[49,54,149]],
	         "RdYlGn":[[165,0,38],[215,48,39],[244,109,67],[253,174,97],[254,224,139],[255,255,191],[217,239,139],[166,217,106],[102,189,99],[26,152,80],[0,104,55]],
	         "Spectral":[[158,1,66],[213,62,79],[244,109,67],[253,174,97],[254,224,139],[255,255,191],[230,245,152],[171,221,164],[102,194,165],[50,136,189],[94,79,162]]}
};

Shade.Colors.Brewer.sequential = function(opts) {
    opts = _.defaults(opts || {}, {
        alpha: 1,
        min: 0,
        max: 1
    });
    if (_.isUndefined(opts.name))
        throw new Error("'name' is a required option");
    var a = schemes.sequential[opts.name];
    if (_.isUndefined(a))
        throw new Error("Unknown sequential colormap " + opts.name);
    var range = _.map(a, function(lst) {
        return Shade.vec(lst[0] / 255, lst[1]/255, lst[2]/255, opts.alpha);
    });
    var us = _.map(range, function(v, i) {
        return i / (range.length - 1);
    });
    return Shade.Scale.linear({
        domain: _.map(us, function(u) { return Shade.mix(opts.min, opts.max, u); }),
        range: range
    });
};

Shade.Colors.Brewer.qualitative = function(opts) {
    opts = _.defaults(opts || {}, {
        alpha: 1
    });
    if (_.isUndefined(opts.name))
        throw new Error("'name' is a required option");
    var a = schemes.qualitative[opts.name];
    if (_.isUndefined(a))
        throw new Error("Unknown qualitative colormap " + opts.name);
    function lookup(i) {
        if (_.isUndefined(opts.domain)) {
            return a[i];
        }
        return a[opts.domain[i]];
    }
    var range = _.map(a, function(unused, i) {
        lst = lookup(i);
        return Shade.vec(lst[0] / 255, lst[1]/255, lst[2]/255, opts.alpha);
    });
    return Shade.Scale.ordinal({range: range});
};

Shade.Colors.Brewer.divergent = function(opts) {
    opts = _.defaults(opts || {}, {
        alpha: 1,
        low: -1,
        zero: 0,
        high: 1
    });
    if (_.isUndefined(opts.name))
        throw new Error("'name' is a required option");
    var a = schemes.divergent[opts.name];
    if (_.isUndefined(a))
        throw new Error("Unknown divergent colormap " + opts.name);
    var range = _.map(a, function(lst) {
        return Shade.vec(lst[0] / 255, lst[1]/255, lst[2]/255, opts.alpha);
    });
    
    var map1 = Shade.Scale.linear({
        domain: [opts.low, opts.zero, opts.high],
        range: [0, (range.length - 1) / 2, range.length - 1]
    });

    var map2 = Shade.Scale.linear({domain: _.range(range.length),
                                   range: range});
    return Shade(_.compose(map2, map1));
};

})();
(function() {

function compose(g, f)
{
    if (_.isUndefined(f) || _.isUndefined(g))
        throw new Error("Undefined!");
    return function(x) {
        return g(f(x));
    };
}

var table = {};
var colorspaces = ["rgb", "srgb", "luv", "hcl", "hls", "hsv", "xyz"];
_.each(colorspaces, function(space) {
    table[space] = {};
    table[space][space] = function(x) { return x; };
    table[space].create = function(v0, v1, v2) {
        // this function is carefully designed to work for the above
        // color space names. if those change, this probably changes
        // too.
        var l = space.length;
        var field_0 = space[l-3],
            field_1 = space[l-2],
            field_2 = space[l-1];
        var result = {
            space: space,
            values: function() {
                return [this[field_0], this[field_1], this[field_2]];
            },
            as_shade: function(alpha) {
                if (_.isUndefined(alpha))
                    alpha = 1;
                var srgb = table[space].rgb(this);
                return Shade.vec(srgb.r, srgb.g, srgb.b, alpha);
            }
        };
        
        result[field_0] = v0;
        result[field_1] = v1;
        result[field_2] = v2;
        _.each(colorspaces, function(other_space) {
            result[other_space] = function() { return table[space][other_space](result); };
        });
        return result;
    };
});

function xyz_to_uv(xyz)
{
    var t, x, y;
    t = xyz.x + xyz.y + xyz.z;
    x = xyz.x / t;
    y = xyz.y / t;
    return [2 * x / (6 * y - x + 1.5),
            4.5 * y / (6 * y - x + 1.5)];
};

// qtrans takes hue varying from 0 to 1!
function qtrans(q1, q2, hue)
{
    if (hue > 1) hue -= 1;
    if (hue < 0) hue += 1;
    if (hue < 1/6) 
        return q1 + (q2 - q1) * (hue * 6);
    else if (hue < 1/2)
        return q2;
    else if (hue < 2/3)
        return q1 + (q2 - q1) * (2/3 - hue) * 6;
    else
        return q1;
};

function gtrans(u, gamma)
{
    if (u > 0.00304)
        return 1.055 * Math.pow(u, 1 / gamma) - 0.055;
    else
        return 12.92 * u;
    // if (u < 0) return u;
    // return Math.pow(u, 1.0 / gamma);
}

function ftrans(u, gamma)
{
    if (u > 0.03928)
        return Math.pow((u + 0.055) / 1.055, gamma);
    else
        return u / 12.92;
    // if (u < 0) return u;
    // return Math.pow(u, gamma);
}

//////////////////////////////////////////////////////////////////////////////
// table.rgb.*

table.rgb.hsv = function(rgb)
{
    var x = Math.min(rgb.r, rgb.g, rgb.b);
    var y = Math.max(rgb.r, rgb.g, rgb.b);
    if (y !== x) {
        var f = ((rgb.r === x) ? rgb.g - rgb.b : 
                 (rgb.g === x) ? rgb.b - rgb.r :
                                 rgb.r - rgb.g);
        var i = ((rgb.r === x) ? 3 :
                 (rgb.g === x) ? 5 : 1);
        return table.hsv.create((Math.PI/3) * (i - f / (y - x)),
                                (y - x) / y,
                                y);
    } else {
        return table.hsv.create(0, 0, y);
    }
};

table.rgb.hls = function(rgb)
{
    var min = Math.min(rgb.r, rgb.g, rgb.b);
    var max = Math.max(rgb.r, rgb.g, rgb.b);

    var l = (max + min) / 2, s, h;
    if (max !== min) {
        if (l < 0.5)
            s = (max - min) / (max + min);
        else
            s = (max - min) / (2.0 - max - min);
        if (rgb.r === max) {
            h = (rgb.g - rgb.b) / (max - min);
        } else if (rgb.g === max) {
            h = 2.0 + (rgb.b - rgb.r) / (max - min);
        } else {
            h = 4.0 + (rgb.r - rgb.g) / (max - min);
        }
        h = h * Math.PI / 3;
        if (h < 0)           h += Math.PI * 2;
        if (h > Math.PI * 2) h -= Math.PI * 2;
    } else {
        s = 0;
        h = 0;
    }
    return table.hls.create(h, l, s);
};

table.rgb.xyz = function(rgb)
{
    var yn = white_point.y;
    return table.xyz.create(
        yn * (0.412453 * rgb.r + 0.357580 * rgb.g + 0.180423 * rgb.b),
        yn * (0.212671 * rgb.r + 0.715160 * rgb.g + 0.072169 * rgb.b),
        yn * (0.019334 * rgb.r + 0.119193 * rgb.g + 0.950227 * rgb.b));
};

table.rgb.srgb = function(rgb)
{
    return table.srgb.create(gtrans(rgb.r, 2.4),
                             gtrans(rgb.g, 2.4),
                             gtrans(rgb.b, 2.4));
};

// table.rgb.luv = compose(table.xyz.luv, table.rgb.xyz);
// table.rgb.hcl = compose(table.luv.hcl, table.rgb.luv);

//////////////////////////////////////////////////////////////////////////////
// table.srgb.*

table.srgb.xyz = function(srgb)
{
    var yn = white_point.y;
    var r = ftrans(srgb.r, 2.4),
        g = ftrans(srgb.g, 2.4),
        b = ftrans(srgb.b, 2.4);
    return table.xyz.create(
        yn * (0.412453 * r + 0.357580 * g + 0.180423 * b),
        yn * (0.212671 * r + 0.715160 * g + 0.072169 * b),
        yn * (0.019334 * r + 0.119193 * g + 0.950227 * b));
};

table.srgb.rgb = function(srgb)
{
    var result = table.rgb.create(ftrans(srgb.r, 2.4),
                                  ftrans(srgb.g, 2.4),
                                  ftrans(srgb.b, 2.4));
    return result;
};

table.srgb.hls = compose(table.rgb.hls, table.srgb.rgb);
table.srgb.hsv = compose(table.rgb.hsv, table.srgb.rgb);
// table.srgb.luv = compose(table.rgb.luv, table.srgb.rgb);
// table.srgb.hcl = compose(table.rgb.hcl, table.srgb.rgb);

//////////////////////////////////////////////////////////////////////////////
// table.xyz.*

table.xyz.luv = function(xyz)
{
    var y;
    var t1 = xyz_to_uv(xyz);
    y = xyz.y / white_point.y;
    var l = (y > 0.008856 ? 
             116 * Math.pow(y, 1.0/3.0) - 16 :
             903.3 * y);
    return table.luv.create(l, 
                            13 * l * (t1[0] - white_point_uv[0]),
                            13 * l * (t1[1] - white_point_uv[1]));
};
// now I can define these
table.rgb.luv = compose(table.xyz.luv, table.rgb.xyz);
table.srgb.luv = compose(table.rgb.luv, table.srgb.rgb);

table.xyz.rgb = function(xyz)
{
    var yn = white_point.y;
    return table.rgb.create(
        ( 3.240479 * xyz.x - 1.537150 * xyz.y - 0.498535 * xyz.z) / yn,
        (-0.969256 * xyz.x + 1.875992 * xyz.y + 0.041556 * xyz.z) / yn,
        ( 0.055648 * xyz.x - 0.204043 * xyz.y + 1.057311 * xyz.z) / yn
    );
};
table.xyz.hls = compose(table.rgb.hls, table.xyz.rgb);
table.xyz.hsv = compose(table.rgb.hsv, table.xyz.rgb);

table.xyz.srgb = function(xyz)
{
    var yn = white_point.y;
    return table.srgb.create(
        gtrans(( 3.240479 * xyz.x - 1.537150 * xyz.y - 0.498535 * xyz.z) / yn, 2.4),
        gtrans((-0.969256 * xyz.x + 1.875992 * xyz.y + 0.041556 * xyz.z) / yn, 2.4),
        gtrans(( 0.055648 * xyz.x - 0.204043 * xyz.y + 1.057311 * xyz.z) / yn, 2.4)
    );
};

// table.xyz.hcl = compose(table.rgb.hcl, table.xyz.rgb);

//////////////////////////////////////////////////////////////////////////////
// table.luv.*

table.luv.hcl = function(luv)
{
    var c = Math.sqrt(luv.u * luv.u + luv.v * luv.v);    
    var h = Math.atan2(luv.v, luv.u);
    while (h > Math.PI * 2) { h -= Math.PI * 2; }
    while (h < 0) { h += Math.PI * 2; }
    return table.hcl.create(h, c, luv.l);
};
table.rgb.hcl  = compose(table.luv.hcl,  table.rgb.luv);
table.srgb.hcl = compose(table.luv.hcl,  table.srgb.luv);
table.xyz.hcl  = compose(table.rgb.hcl, table.xyz.rgb);

table.luv.xyz = function(luv)
{
    var x = 0, y = 0, z = 0;
    if (!(luv.l <= 0 && luv.u == 0 && luv.v == 0)) {
        y = white_point.y * ((luv.l > 7.999592) ? 
                             Math.pow((luv.l + 16)/116, 3) : 
                             luv.l / 903.3);
        // var t = xyz_to_uv(xn, yn, zn);
        // var un = t[0], vn = t[1];
        var result_u = luv.u / (13 * luv.l) + white_point_uv[0];
        var result_v = luv.v / (13 * luv.l) + white_point_uv[1];
        x = 9 * y * result_u / (4 * result_v);
        z = -x / 3 - 5 * y + 3 * y / result_v;
    }
    return table.xyz.create(x, y, z);
};
table.luv.rgb  = compose(table.xyz.rgb,  table.luv.xyz);
table.luv.hls  = compose(table.rgb.hls,  table.luv.rgb);
table.luv.hsv  = compose(table.rgb.hsv,  table.luv.rgb);
table.luv.srgb = compose(table.rgb.srgb, table.luv.rgb);


//////////////////////////////////////////////////////////////////////////////
// table.hcl.*

table.hcl.luv = function(hcl)
{
    return table.luv.create(
        hcl.l, hcl.c * Math.cos(hcl.h), hcl.c * Math.sin(hcl.h));
};

table.hcl.rgb  = compose(table.luv.rgb,  table.hcl.luv);
table.hcl.srgb = compose(table.luv.srgb, table.hcl.luv);
table.hcl.hsv  = compose(table.luv.hsv,  table.hcl.luv);
table.hcl.hls  = compose(table.luv.hls,  table.hcl.luv);
table.hcl.xyz  = compose(table.luv.xyz,  table.hcl.luv);

//////////////////////////////////////////////////////////////////////////////
// table.hls.*

table.hls.rgb = function(hls)
{
    var p1, p2;
    if (hls.l <= 0.5)
        p2 = hls.l * (1 + hls.s);
    else
        p2 = hls.l + hls.s - (hls.l * hls.s);
    p1 = 2 * hls.l - p2;
    if (hls.s === 0) {
        return table.rgb.create(hls.l, hls.l, hls.l);
    } else {
        return table.rgb.create(
            qtrans(p1, p2, (hls.h + Math.PI * 2/3) / (Math.PI * 2)),
            qtrans(p1, p2, hls.h / (Math.PI * 2)),
            qtrans(p1, p2, (hls.h - Math.PI * 2/3) / (Math.PI * 2)));
    }
};

table.hls.srgb = compose(table.rgb.srgb, table.hls.rgb);
table.hls.hsv  = compose(table.rgb.hsv,  table.hls.rgb);
table.hls.xyz  = compose(table.rgb.xyz,  table.hls.rgb);
table.hls.luv  = compose(table.rgb.luv,  table.hls.rgb);
table.hls.hcl  = compose(table.rgb.hcl,  table.hls.rgb);

//////////////////////////////////////////////////////////////////////////////
// table.hsv.*

table.hsv.rgb = function(hsv)
{
    if (isNaN(hsv.h)) {
        return table.rgb.create(hsv.v, hsv.v, hsv.v);
    } else {
        var v = hsv.v;
        var h = hsv.h / Math.PI * 3; // from [0,2Pi] to [0,6];
        var i = Math.floor(h);
        var f = h - i;
        if (!(i & 1)) // if index is even
            f = 1 - f;
        var m = v * (1 - hsv.s);
        var n = v * (1 - hsv.s * f);
        switch (i) {
        case 6:
        case 0: return table.rgb.create(v, n, m);
        case 1: return table.rgb.create(n, v, m);
        case 2: return table.rgb.create(m, v, n);
        case 3: return table.rgb.create(m, n, v);
        case 4: return table.rgb.create(n, m, v);
        case 5: return table.rgb.create(v, m, n);
        default:
            throw new Error("internal error");
        };
    }
};

table.hsv.srgb = compose(table.rgb.srgb, table.hsv.rgb);
table.hsv.hls  = compose(table.rgb.hls,  table.hsv.rgb);
table.hsv.xyz  = compose(table.rgb.xyz,  table.hsv.rgb);
table.hsv.luv  = compose(table.rgb.luv,  table.hsv.rgb);
table.hsv.hcl  = compose(table.rgb.hcl,  table.hsv.rgb);

// currently we assume a D65 white point, but this could be configurable
var white_point = table.xyz.create(95.047, 100.000, 108.883);
var white_point_uv = xyz_to_uv(white_point);

Shade.Colors.jstable = table;

})();
/*
 * FIXME The API in Shade.Colors is a disgusting mess. My apologies.
 * 
 */

(function() {

function compose(g, f)
{
    if (_.isUndefined(f) || _.isUndefined(g))
        throw new Error("Undefined!");
    return function(x) {
        return g(f(x));
    };
}

var _if = Shade.ifelse;

var table = {};
var colorspaces = ["rgb", "srgb", "luv", "hcl", "hls", "hsv", "xyz"];
_.each(colorspaces, function(space) {
    Shade.Colors[space] = function(v1, v2, v3, alpha) {
        if (_.isUndefined(alpha))
            alpha = 1;
        return Shade.Colors.shadetable[space].create(v1, v2, v3).as_shade(alpha);
    };
    table[space] = {};
    table[space][space] = function(x) { return x; };
    table[space].create = function() {
        var vec;
        if (arguments.length === 1) {
            vec = arguments[0];
            if (!vec.type.equals(Shade.Types.vec3))
                throw new Error("create with 1 parameter requires a vec3");
        } else if (arguments.length === 3) {
            vec = Shade.vec(arguments[0], arguments[1], arguments[2]);
            if (!vec.type.equals(Shade.Types.vec3))
                throw new Error("create with 3 parameter requires 3 floats");
        } else
            throw new Error("create requires either 1 vec3 or 3 floats");
        // this function is carefully designed to work for the above
        // color space names. if those change, this probably changes
        // too.
        var l = space.length;
        var field_0 = space[l-3],
            field_1 = space[l-2],
            field_2 = space[l-1];
        var result = {
            space: space,
            vec: vec,
            values: function() {
                return [this[field_0].constant_value(), 
                        this[field_1].constant_value(), 
                        this[field_2].constant_value()];
            },
            as_shade: function(alpha) {
                if (_.isUndefined(alpha))
                    alpha = Shade.make(1);
                var result = this.rgb().vec;
                return Shade.vec(this.rgb().vec, alpha);
            }
        };
        result[field_0] = vec.swizzle("r");
        result[field_1] = vec.swizzle("g");
        result[field_2] = vec.swizzle("b");
        _.each(colorspaces, function(other_space) {
            result[other_space] = function() { return table[space][other_space](result); };
        });
        return result;
    };
});

function xyz_to_uv(xyz)
{
    var t, x, y;
    t = xyz.x.add(xyz.y).add(xyz.z);
    x = xyz.x.div(t);
    y = xyz.y.div(t);
    return Shade.vec(x.mul(2).div(y.mul(6).sub(x).add(1.5)),
                     y.mul(4.5).div(y.mul(6).sub(x).add(1.5)));
};

// qtrans takes hue varying from 0 to 1!
function qtrans(q1, q2, hue)
{
    hue = _if(hue.gt(1), hue.sub(1), hue);
    hue = _if(hue.lt(0), hue.add(1), hue);
    return _if(hue.lt(1/6), q1.add(q2.sub(q1).mul(hue.mul(6))),
           _if(hue.lt(1/2), q2,
           _if(hue.lt(2/3), q1.add(q2.sub(q1).mul(Shade.make(2/3)
                                                  .sub(hue).mul(6))),
               q1)));
};

function gtrans(u, gamma)
{
    return _if(u.gt(0.00304),
               Shade.mul(1.055, Shade.pow(u, Shade.div(1, gamma))).sub(0.055),
               u.mul(12.92));
}

function ftrans(u, gamma)
{
    return _if(u.gt(0.03928),
               Shade.pow(u.add(0.055).div(1.055), gamma),
               u.div(12.92));
}

//////////////////////////////////////////////////////////////////////////////
// table.rgb.*

function min3(v)
{
    return Shade.min(v.r, Shade.min(v.g, v.b));
}

function max3(v)
{
    return Shade.max(v.r, Shade.max(v.g, v.b));
}

table.rgb.hsv = function(rgb)
{
    var x = min3(rgb);
    var y = max3(rgb);
    
    var f = _if(rgb.r.eq(x), rgb.g.sub(rgb.b),
            _if(rgb.g.eq(x), rgb.b.sub(rgb.r),
                             rgb.r.sub(rgb.g)));
    var i = _if(rgb.r.eq(x), 3, _if(rgb.g.eq(x), 5, 1));
    return table.hsv.create(_if(
        y.eq(x), 
        Shade.vec(0,0,y),
        Shade.vec(Shade.mul(Math.PI/3, i.sub(f.div(y.sub(x)))),
                  y.sub(x).div(y),
                  y)));
};

table.rgb.hls = function(rgb)
{
    var min = min3(rgb);
    var max = max3(rgb);
    var l = max.add(min).div(2), s, h;
    var mx_ne_mn = max.ne(min);
    
    s = _if(mx_ne_mn,
            _if(l.lt(0.5), 
                max.sub(min).div(max.add(min)),
                max.sub(min).div(Shade.sub(2.0, max).sub(min))),
            0);
    h = _if(mx_ne_mn,
            _if(rgb.r.eq(max),                rgb.g.sub(rgb.b).div(max.sub(min)),
            _if(rgb.g.eq(max), Shade.add(2.0, rgb.b.sub(rgb.r).div(max.sub(min))),
                               Shade.add(4.0, rgb.r.sub(rgb.g).div(max.sub(min))))),
            0);
    h = h.mul(Math.PI / 3);
    h = _if(h.lt(0),           h.add(Math.PI * 2),
        _if(h.gt(Math.PI * 2), h.sub(Math.PI * 2), 
                               h));
    return table.hls.create(h, l, s);
};

table.rgb.xyz = function(rgb)
{
    var yn = white_point.y;
    return table.xyz.create(
        yn.mul(rgb.r.mul(0.412453).add(rgb.g.mul(0.357580)).add(rgb.b.mul(0.180423))),
        yn.mul(rgb.r.mul(0.212671).add(rgb.g.mul(0.715160)).add(rgb.b.mul(0.072169))),
        yn.mul(rgb.r.mul(0.019334).add(rgb.g.mul(0.119193)).add(rgb.b.mul(0.950227))));
};

table.rgb.srgb = function(rgb)
{
    return table.srgb.create(gtrans(rgb.r, 2.4),
                             gtrans(rgb.g, 2.4),
                             gtrans(rgb.b, 2.4));
};

// table.rgb.luv = compose(table.xyz.luv, table.rgb.xyz);
// table.rgb.hcl = compose(table.luv.hcl, table.rgb.luv);

//////////////////////////////////////////////////////////////////////////////
// table.srgb.*

table.srgb.xyz = function(srgb)
{
    var yn = white_point.y;
    var r = ftrans(srgb.r, 2.4),
        g = ftrans(srgb.g, 2.4),
        b = ftrans(srgb.b, 2.4);
    return table.xyz.create(
        yn.mul(r.mul(0.412453).add(g.mul(0.357580)).add(b.mul(0.180423))),
        yn.mul(r.mul(0.212671).add(g.mul(0.715160)).add(b.mul(0.072169))),
        yn.mul(r.mul(0.019334).add(g.mul(0.119193)).add(b.mul(0.950227))));
};

table.srgb.rgb = function(srgb)
{
    var result = table.rgb.create(ftrans(srgb.r, 2.4),
                                  ftrans(srgb.g, 2.4),
                                  ftrans(srgb.b, 2.4));
    
    return result;
};

table.srgb.hls = compose(table.rgb.hls, table.srgb.rgb);
table.srgb.hsv = compose(table.rgb.hsv, table.srgb.rgb);
// table.srgb.luv = compose(table.rgb.luv, table.srgb.rgb);
// table.srgb.hcl = compose(table.rgb.hcl, table.srgb.rgb);

//////////////////////////////////////////////////////////////////////////////
// table.xyz.*

table.xyz.luv = function(xyz)
{
    var y;
    var t1 = xyz_to_uv(xyz);
    y = xyz.y.div(white_point.y);
    var l = _if(y.gt(0.008856), 
                Shade.mul(116, Shade.pow(y, 1.0/3.0)).sub(16),
                Shade.mul(903.3, y));
    return table.luv.create(Shade.vec(l, l.mul(t1.sub(white_point_uv)).mul(13)));
};
// now I can define these
table.rgb.luv = compose(table.xyz.luv, table.rgb.xyz);
table.srgb.luv = compose(table.rgb.luv, table.srgb.rgb);

table.xyz.rgb = function(xyz)
{
    var yn = white_point.y;
    return table.rgb.create(
        (xyz.x.mul( 3.240479).sub(xyz.y.mul(1.537150)).sub(xyz.z.mul(0.498535))).div(yn),
        (xyz.x.mul(-0.969256).add(xyz.y.mul(1.875992)).add(xyz.z.mul(0.041556))).div(yn),
        (xyz.x.mul( 0.055648).sub(xyz.y.mul(0.204043)).add(xyz.z.mul(1.057311))).div(yn)
    );
};
table.xyz.hls = compose(table.rgb.hls, table.xyz.rgb);
table.xyz.hsv = compose(table.rgb.hsv, table.xyz.rgb);

table.xyz.srgb = function(xyz)
{
    var yn = white_point.y;
    return table.srgb.create(
        gtrans((xyz.x.mul( 3.240479).sub(xyz.y.mul(1.537150)).sub(xyz.z.mul(0.498535))).div(yn), 2.4),
        gtrans((xyz.x.mul(-0.969256).add(xyz.y.mul(1.875992)).add(xyz.z.mul(0.041556))).div(yn), 2.4),
        gtrans((xyz.x.mul( 0.055648).sub(xyz.y.mul(0.204043)).add(xyz.z.mul(1.057311))).div(yn), 2.4)
    );
};

// table.xyz.hcl = compose(table.rgb.hcl, table.xyz.rgb);

//////////////////////////////////////////////////////////////////////////////
// table.luv.*

table.luv.hcl = function(luv)
{
    var c = Shade.norm(luv.vec.swizzle("gb"));
    var h = Shade.atan(luv.v, luv.u);
    h = _if(h.gt(Math.PI*2), h.sub(Math.PI*2),
        _if(h.lt(0), h.add(Math.PI*2), h));
    while (h > Math.PI * 2) { h -= Math.PI * 2; }
    while (h < 0) { h += Math.PI * 2; }
    return table.hcl.create(h, c, luv.l);
};
table.rgb.hcl  = compose(table.luv.hcl,  table.rgb.luv);
table.srgb.hcl = compose(table.luv.hcl,  table.srgb.luv);
table.xyz.hcl  = compose(table.rgb.hcl, table.xyz.rgb);

table.luv.xyz = function(luv)
{
    var uv = luv.vec.swizzle("gb").div(luv.l.mul(13)).add(white_point_uv);
    var u = uv.swizzle("r"), v = uv.swizzle("g");
    var y = white_point.y.mul(_if(luv.l.gt(7.999592),
                                  Shade.pow(luv.l.add(16).div(116), 3),
                                  luv.l.div(903.3)));
    var x = y.mul(9).mul(u).div(v.mul(4));
    var z = x.div(-3).sub(y.mul(5)).add(y.mul(3).div(v));
    return table.xyz.create(_if(luv.l.le(0).and(luv.u.eq(0).and(luv.v.eq(0))),
                                Shade.vec(0,0,0),
                                Shade.vec(x,y,z)));
};
table.luv.rgb  = compose(table.xyz.rgb,  table.luv.xyz);
table.luv.hls  = compose(table.rgb.hls,  table.luv.rgb);
table.luv.hsv  = compose(table.rgb.hsv,  table.luv.rgb);
table.luv.srgb = compose(table.rgb.srgb, table.luv.rgb);

//////////////////////////////////////////////////////////////////////////////
// table.hcl.*

table.hcl.luv = function(hcl)
{
    return table.luv.create(
        hcl.l, hcl.c.mul(hcl.h.cos()), hcl.c.mul(hcl.h.sin()));
};

table.hcl.rgb  = compose(table.luv.rgb,  table.hcl.luv);
table.hcl.srgb = compose(table.luv.srgb, table.hcl.luv);
table.hcl.hsv  = compose(table.luv.hsv,  table.hcl.luv);
table.hcl.hls  = compose(table.luv.hls,  table.hcl.luv);
table.hcl.xyz  = compose(table.luv.xyz,  table.hcl.luv);

//////////////////////////////////////////////////////////////////////////////
// table.hls.*

table.hls.rgb = function(hls)
{
    var p2 = _if(hls.l.le(0.5),
                 hls.l.mul(hls.s.add(1)),
                 hls.l.add(hls.s).sub(hls.l.mul(hls.s)));
    var p1 = hls.l.mul(2).sub(p2);
    return table.rgb.create(
        _if(hls.s.eq(0),
            Shade.vec(hls.vec.swizzle("ggg")),
            Shade.vec(qtrans(p1, p2, hls.h.add(Math.PI * 2/3).div(Math.PI * 2)),
                      qtrans(p1, p2, hls.h.div(Math.PI * 2)),
                      qtrans(p1, p2, hls.h.sub(Math.PI * 2/3).div(Math.PI * 2)))));
};

table.hls.srgb = compose(table.rgb.srgb, table.hls.rgb);
table.hls.hsv  = compose(table.rgb.hsv,  table.hls.rgb);
table.hls.xyz  = compose(table.rgb.xyz,  table.hls.rgb);
table.hls.luv  = compose(table.rgb.luv,  table.hls.rgb);
table.hls.hcl  = compose(table.rgb.hcl,  table.hls.rgb);

//////////////////////////////////////////////////////////////////////////////
// table.hsv.*

table.hsv.rgb = function(hsv)
{
    var v = hsv.v;
    var h = hsv.h.div(Math.PI).mul(3);
    var i = h.floor();
    var f = h.sub(i);
    f = _if(i.div(2).floor().eq(i.div(2)),
            Shade.sub(1, f),
            f);
    var m = v.mul(Shade.sub(1, hsv.s));
    var n = v.mul(Shade.sub(1, hsv.s.mul(f)));
    return table.rgb.create(_if(i.eq(0), Shade.vec(v, n, m),
                            _if(i.eq(1), Shade.vec(n, v, m),
                            _if(i.eq(2), Shade.vec(m, v, n),
                            _if(i.eq(3), Shade.vec(m, n, v),
                            _if(i.eq(4), Shade.vec(n, m, v),
                            _if(i.eq(5), Shade.vec(v, m, n),
                                         Shade.vec(v, n, m))))))));
};

table.hsv.srgb = compose(table.rgb.srgb, table.hsv.rgb);
table.hsv.hls  = compose(table.rgb.hls,  table.hsv.rgb);
table.hsv.xyz  = compose(table.rgb.xyz,  table.hsv.rgb);
table.hsv.luv  = compose(table.rgb.luv,  table.hsv.rgb);
table.hsv.hcl  = compose(table.rgb.hcl,  table.hsv.rgb);

// currently we assume a D65 white point, but this could be configurable
var white_point = table.xyz.create(95.047, 100.000, 108.883);
var white_point_uv = xyz_to_uv(white_point);

Shade.Colors.shadetable = table;

//////////////////////////////////////////////////////////////////////////////
// Color utility functions

// FIXME Ideally, I would like these to not depend on the 'table' variable,
// which is a gigantic mess. But for now, they do.

function flip(v) { return Shade(1).sub(v); }

Shade.Colors.desaturate = Shade(function(amount) {
    return function(color) {
        var rgb = table.rgb.create(color.r(), color.g(), color.b());
        var hsv = table.rgb.hsv(rgb);
        return table.hsv.create(hsv.h, hsv.s.mul(flip(amount)), hsv.v).as_shade(color.a());
    };
});

Shade.Colors.brighten = Shade(function(amount) {
    return function(color) {
        var rgb = table.rgb.create(color.r(), color.g(), color.b());
        var hls = table.rgb.hls(rgb);
        var darkness = flip(hls.l);
        amount = flip(amount);
        var resulting_darkness = darkness.mul(amount);
        return table.hls.create(hls.h, flip(resulting_darkness), hls.s).as_shade(color.a());
    };
});

Shade.Colors.darken = Shade(function(amount) {
    return function(color) {
        var rgb = table.rgb.create(color.r(), color.g(), color.b());
        var hls = table.rgb.hls(rgb);
        var darkness = flip(hls.l);
        amount = flip(amount);
        var resulting_darkness = darkness.mul(amount);
        return table.hls.create(hls.h, resulting_darkness, hls.s).as_shade(color.a());
    };
});

Shade.Colors.invert = Shade(function(c) {
    var rgb = table.rgb.create(c.r(), c.g(), c.b());
    var hls = table.rgb.hls(rgb);
    return table.hls.create(hls.h, flip(hls.l), hls.s).as_shade(c.a()); 
});

})();
/* These are all pretty sketchily dependent on the underlying
 precision of the FP units.

 It is likely that the only correct and portable implementations are
 through the use of texture lookup tables.

 */
Shade.Bits = {};
/* Shade.Bits.encode_float encodes a single 32-bit IEEE 754
   floating-point number as a 32-bit RGBA value, so that when rendered
   to a non-floating-point render buffer and read with readPixels, the
   resulting ArrayBufferView can be cast directly as a Float32Array,
   which will encode the correct value.

   These gymnastics are necessary because, shockingly, readPixels does
   not support reading off floating-point values of an FBO bound to a
   floating-point texture (!):

   https://www.khronos.org/webgl/public-mailing-list/archives/1108/threads.html#00020

   WebGL does not support bitwise operators. As a result, much of what
   is happening here is less efficient than it should be, and incurs
   precision losses. That is unfortunate, but currently unavoidable as
   well.

*/

// This function is currently only defined for "well-behaved" IEEE 754
// numbers. No denormals, NaN, infinities, etc.
Shade.Bits.encode_float = Shade.make(function(val) {

    var byte1, byte2, byte3, byte4;

    var is_zero = val.eq(0);

    var sign = val.gt(0).ifelse(0, 1);
    val = val.abs();

    var exponent = val.log2().floor();
    var biased_exponent = exponent.add(127);
    var fraction = val.div(exponent.exp2()).sub(1).mul(8388608); // 2^23

    var t = biased_exponent.div(2);
    var last_bit_of_biased_exponent = t.fract().mul(2);
    var remaining_bits_of_biased_exponent = t.floor();

    byte4 = Shade.Bits.extract_bits(fraction, 0, 8).div(255);
    byte3 = Shade.Bits.extract_bits(fraction, 8, 16).div(255);
    byte2 = last_bit_of_biased_exponent.mul(128)
        .add(Shade.Bits.extract_bits(fraction, 16, 23)).div(255);
    byte1 = sign.mul(128).add(remaining_bits_of_biased_exponent).div(255);

    return is_zero.ifelse(Shade.vec(0, 0, 0, 0),
                          Shade.vec(byte4, byte3, byte2, byte1));
});
/* Shade.Bits.extract_bits returns a certain bit substring of the
   original number using no bitwise operations, which are not available in WebGL.

   if they were, then the definition of extract_bits would be:

     extract_bits(num, from, to) = (num >> from) & ((1 << (to - from)) - 1)

   Shade.Bits.extract_bits assumes:

     num > 0
     from < to
*/

Shade.Bits.extract_bits = Shade.make(function(num, from, to) {
    from = from.add(0.5).floor();
    to = to.add(0.5).floor();
    return Shade.Bits.mask_last(Shade.Bits.shift_right(num, from), to.sub(from));
});
/* If webgl supported bitwise operations,
   mask_last(v, bits) = v & ((1 << bits) - 1)

   We use the slower version via mod():

   v & ((1 << k) - 1) = v % (1 << k)
*/
Shade.Bits.mask_last = Shade.make(function(v, bits) {
    return v.mod(Shade.Bits.shift_left(1, bits));
});
Shade.Bits.shift_left = Shade.make(function(v, amt) {
    return v.mul(amt.exp2()).round();
});
Shade.Bits.shift_right = Shade.make(function(v, amt) {
    // NB: this is *not* equivalent to any sequence of operations
    // involving round()

    // The extra gymnastics are necessary because
    //
    // 1. we cannot round the result, since some of the fractional values
    // might be larger than 0.5
    //
    // 2. shifting right by a large number (>22 in my tests) creates
    // a large enough float that precision is an issue (2^22 / exp2(22) < 1, for example). 
    // So we divide an ever so slightly larger number so that flooring
    // does the right thing.
    //
    // THIS REMAINS TO BE THOROUGHLY TESTED.
    //
    // There's possibly a better alternative involving integer arithmetic,
    // but GLSL ES allows implementations to use floating-point in place of integers.
    // 
    // It's likely that the only portably correct implementation of this
    // uses look-up tables. I won't fix this for now.

    v = v.floor().add(0.5);
    return v.div(amt.exp2()).floor();
});
Shade.Scale = {};

/*
 * nearest-neighbor interpolation
 */

Shade.Scale.ordinal = function(opts)
{
    function all_same(set) {
        return _.all(set, function(v) { return v.equals(set[0]); });
    }
    if (!(opts.range.length >= 2)) { 
        throw new Error("Shade.Scale.ordinal requires arrays of length at least 2");
    }
    var range = _.map(opts.range, Shade.make);
    var range_types = _.map(range,  function(v) { return v.type; });
    if (!all_same(range_types))
        throw new Error("Shade.Scale.linear requires range elements to have the same type");

    var choose = Shade.Utils.choose(range);

    return Shade(function(v) {
        return choose(v.as_float().add(0.5));
    });
};
Shade.Scale.linear = function(opts)
{
    var allowable_types = [
        Shade.Types.float_t,
        Shade.Types.vec2,
        Shade.Types.vec3,
        Shade.Types.vec4
    ];
    var vec_types = [
        Shade.Types.vec2,
        Shade.Types.vec3,
        Shade.Types.vec4
    ];
    function is_any(set) {
        return function(t) {
            return _.any(set, function(v) { return v.equals(t); });
        };
    }
    function all_same(set) {
        return _.all(set, function(v) { return v.equals(set[0]); });
    }

    opts = _.defaults(opts || {}, {
        domain: [0, 1],
        range: [0, 1]
    });

    //////////////////////////////////////////////////////////////////////////
    // typechecking

    // that condition is written awkwardly so it catches
    // opts.domain === undefined as well.
    if (!(opts.domain.length >= 2)) { 
        throw new Error("Shade.Scale.linear requires arrays of length at least 2");
    }
    if (opts.domain.length !== opts.range.length) {
        throw new Error("Shade.Scale.linear requires domain and range to be arrays of the same length");
    }

    opts.domain = _.map(opts.domain, Shade.make);
    opts.range = _.map(opts.range, Shade.make);

    var domain_types = _.map(opts.domain, function(v) { return v.type; });
    var range_types =  _.map(opts.range,  function(v) { return v.type; });

    if (!is_any(allowable_types)(domain_types[0]))
        throw new Error("Shade.Scale.linear requires domain type to be one of {float, vec2, vec3, vec4}");
    if (!all_same(domain_types))
        throw new Error("Shade.Scale.linear requires domain elements to have the same type");
    if (!is_any(allowable_types)(range_types[0]))
        throw new Error("Shade.Scale.linear requires range type to be one of {float, vec2, vec3, vec4}");
    if (!all_same(range_types))
        throw new Error("Shade.Scale.linear requires range elements to have the same type");
    if (is_any(vec_types)(domain_types[0]) && (!domain_types[0].equals(range_types[0])))
        throw new Error("Shade.Scale.linear for vec types require equal domain and range types");
    if (opts.domain.length < 2 || opts.range.length < 2)
        throw new Error("Shade.Scale.linear requires domain and range to have at least two elements");

    // Special-case the two-element scale for performance
    if (opts.domain.length === 2) {
        var f1 = opts.domain[0];
        var f2 = opts.domain[1];
        var t1 = opts.range[0];
        var t2 = opts.range[1];
        var df = Shade.sub(f2, f1);
        var dt = Shade.sub(t2, t1);

        return Shade(function(x) {
            return x.sub(f1).mul(dt.div(df)).add(t1);
        });
    } else {
        var domain_array = Shade.array(opts.domain);
        var range_array = Shade.array(opts.range);
        var dt = domain_array.array_element_type;

        return Shade(function(x) {
            function create_shade(i) {
                var segment_at_x = Shade.Scale.linear({
                    domain: [ opts.domain[i], opts.domain[i+1] ],
                    range:  [ opts.range[i],  opts.range[i+1] ] })(x);
                if (i === opts.domain.length-2) {
                    return segment_at_x;
                } else {
                    return Shade.ifelse(x.lt(opts.domain[i+1]),
                                        segment_at_x,
                                        create_shade(i+1));
                }
            }
            return create_shade(0);
        });
    }

/*

 The previous version of the code uses Shade.Array.locate to binary-search the array.
 However, it turns out that we're not allowed to read from arbitrary locations in an
 array (even if we could prove their safety) in WebGL's version of GLSL, which means
 I believe, in principle, that binary search is not implementable inside a for loop 
 in WebGL GLSL. (?!)

 I have temporarily replaced it with a dumb loop over the array.

        var result;

        if (dt.equals(Shade.Types.float_t))
            result = Shade(function(v) {
                var bs = domain_array.locate(v);
                var u = v.sub(bs("vl")).div(bs("vr").sub(bs("vl")));
                var output = Shade.mix(range_array.at(bs("l")), range_array.at(bs("r")), u);
                return output;
            });
        else if (_.any(["vec2", "vec3", "vec4"], function(t) 
                       {
                           return dt.equals(Shade.Types[t]);
                       })) {
            result = Shade(function(v) {
                var result = _.range(dt.vec_dimension()).map(function(i) {
                    var bs = domain_array.locate(v.at(i), function(array_value) {
                        return array_value.at(i);
                    });
                    var u = v.sub(bs("vl")).div(bs("vr").sub(bs("vl")));
                    var output = Shade.mix(range_array.at(bs("l")).at(i), 
                                           range_array.at(bs("r")).at(i), u);
                    return output;
                });
                return Shade.vec.apply(this, result);
            });
        } else {
            throw new Error("internal error on Shade.Scale.linear");
        }
        return result;
*/
};
Shade.Scale.transformed = function(opts)
{
    if (_.isUndefined(opts.transform)) {
        throw new Error("Shade.Scale.transform expects a domain transformation function");
    };
    var linear_scale = Shade.Scale.linear(opts);
    return Shade(function(x) {
        return linear_scale(opts.transform(x));
    });
};
Shade.Scale.log = function(opts)
{
    var new_opts = _.extend({
        transform: function(x) { return Shade.log(x); }
    }, opts);
    return Shade.Scale.transformed(new_opts);
};
Shade.Scale.log10 = function(opts)
{
    var new_opts = _.extend({
        transform: function(x) { return Shade.log(x).div(Math.log(10)); }
    }, opts);
    return Shade.Scale.transformed(new_opts);
};
Shade.Scale.log2 = function(opts)
{
    var new_opts = _.extend({
        transform: function(x) { return Shade.log(x).div(Math.log(2)); }
    }, opts);
    return Shade.Scale.transformed(new_opts);
};
Shade.Scale.Geo = {};
Shade.Scale.Geo.latlong_to_hammer = Shade(function(lat, lon, B)
{
    if (_.isUndefined(B))
        B = Shade(2);
    else if (!B.type.equals(Shade.Types.float_t))
        throw new Error("B should have type float");
    var phi = lat,
        lambda = lon;
    var eta = phi.cos().mul(lambda.div(B).cos()).add(1).sqrt();
    var x = B.mul(Math.sqrt(2)).mul(phi.cos()).mul(lambda.div(B).sin()).div(eta);
    var y = phi.sin().mul(Math.sqrt(2)).div(eta);
    var out = Shade.vec(x, y);
    return out;
});
Shade.Scale.Geo.latlong_to_mercator = Shade(function(lat, lon)
{
    lat = lat.div(2).add(Math.PI/4).tan().log();
    return Shade.vec(lon, lat);
});
Shade.Scale.Geo.latlong_to_spherical = Shade(function(lat, lon)
{
    var stretch = lat.cos();
    return Shade.vec(lon.sin().mul(stretch),
                     lat.sin(),
                     lon.cos().mul(stretch), 1);
});
Shade.Scale.Geo.mercator_to_latlong = Shade(function(x, y)
{
    // http://stackoverflow.com/a/1166095
    return Shade.vec(y.sinh().atan(), x);
});
Shade.Scale.Geo.mercator_to_spherical = Shade(function(x, y)
{
    var lat = y.sinh().atan();
    var lon = x;
    return Shade.Scale.Geo.latlong_to_spherical(lat, lon);
});
// replicates something like an opengl light. 
// Fairly bare-bones for now (only diffuse, no attenuation)
// gl_light is deprecated, functionality is being moved to Shade.Light
Shade.gl_light = function(opts)
{
    console.log("DEPRECATED: use Shade.Light functionality");
    opts = _.defaults(opts || {}, {
        light_ambient: Shade.vec(0,0,0,1),
        light_diffuse: Shade.vec(1,1,1,1),
        two_sided: false,
        per_vertex: false
    });
    function vec3(v) {
        return v.type.equals(Shade.Types.vec4) ? v.swizzle("xyz").div(v.at(3)) : v;
    }
    var light_pos = vec3(opts.light_position);
    var vertex_pos = vec3(opts.vertex);
    var material_color = opts.material_color;
    var light_ambient = opts.light_ambient;
    var light_diffuse = opts.light_diffuse;
    var per_vertex = opts.per_vertex;
    var vertex_normal = (opts.normal.type.equals(Shade.Types.vec4) ? 
                         opts.normal.swizzle("xyz") : 
                         opts.normal).normalize();

    // this must be appropriately transformed
    var N = vertex_normal;
    var L = light_pos.sub(vertex_pos).normalize();
    var v = Shade.max(Shade.ifelse(opts.two_sided,
                                   L.dot(N).abs(),
                                   L.dot(N)), 0);
    if (per_vertex)
        v = Shade.per_vertex(v);

    return Shade.add(light_ambient.mul(material_color),
                     v.mul(light_diffuse).mul(material_color));
};
Shade.Light = {};
// The most basic lighting component, ambient lighting simply multiplies
// the light color by the material color.
Shade.Light.ambient = function(light_opts)
{
    var color;
    if (light_opts.color.type.equals(Shade.Types.vec4)) {
        color = light_opts.color.swizzle("rgb");
    } else if (light_opts.color.type.equals(Shade.Types.vec3)) {
        color = light_opts.color;
    } else throw new Error("expected color of type vec3 or vec4, got " +
                           light_opts.color.type.repr() + " instead");
    return function(material_opts) {
        if (material_opts.color.type.equals(Shade.Types.vec4)) {
            return Shade.vec(
                material_opts.color.swizzle("xyz").mul(color),
                material_opts.color.swizzle("a")
            );
        } else {
            return material_opts.color.mul(color);
        }
    };
};
Shade.Light.diffuse = function(light_opts)
{
    light_opts = _.defaults(light_opts || {}, {
        color: Shade.vec(1,1,1,1)
    });

    function vec3(v) {
        return v.type.equals(Shade.Types.vec4) ? v.swizzle("xyz").div(v.at(3)) : v;
    }
    var light_diffuse = light_opts.color;
    if (light_diffuse.type.equals(Shade.Types.vec4))
        light_diffuse = light_diffuse.swizzle("xyz");
    var light_pos = vec3(light_opts.position);

    return function(material_opts) {
        material_opts = _.defaults(material_opts || {}, {
            two_sided: false
        });
        var vertex_pos = vec3(material_opts.position);
        var material_color = material_opts.color;
        if (material_color.type.equals(Shade.Types.vec4))
            material_color = material_color.swizzle("xyz");

        var vertex_normal;
        if (_.isUndefined(material_opts.normal)) {
            vertex_normal = Shade.ThreeD.normal(vertex_pos);
        } else {
            vertex_normal = vec3(material_opts.normal).normalize();
        }

        var L = light_pos.sub(vertex_pos).normalize();
        var v = Shade.max(Shade.ifelse(material_opts.two_sided,
                                       L.dot(vertex_normal).abs(),
                                       L.dot(vertex_normal)), 0);

        var c = Shade.add(v.mul(light_diffuse).mul(material_color));

        return material_opts.color.type.equals(Shade.Types.vec4) ?
            Shade.vec(c, material_opts.color.a()) : c;
    };
};
// functions to help with 3D rendering.
Shade.ThreeD = {};
// Shade.ThreeD.bump returns a normal perturbed by bump mapping.

Shade.ThreeD.bump = function(opts) {
    // Via Three.JS, and
    // http://mmikkelsen3d.blogspot.sk/2011/07/derivative-maps.html
    var uv         = opts.uv;
    var bump_map   = opts.map;
    var bump_scale = opts.scale;
    var surf_pos   = opts.position;
    var surf_norm  = opts.normal;

    var dSTdx      = Shade.dFdx(uv);
    var dSTdy      = Shade.dFdy(uv);
    var Hll        = Shade.texture2D(bump_map, uv).x();
    var dBx        = Shade.texture2D(bump_map, uv.add(dSTdx)).x().sub(Hll);
    var dBy        = Shade.texture2D(bump_map, uv.add(dSTdy)).x().sub(Hll);
    var dHdxy      = Shade.vec(dBx, dBy).mul(bump_scale);
    var sigmaX     = Shade.dFdx(surf_pos);
    var sigmaY     = Shade.dFdy(surf_pos);
    var R1         = Shade.cross(sigmaY, surf_norm);
    var R2         = Shade.cross(surf_norm, sigmaX);
    var det        = sigmaX.dot(R1);
    var vGrad      = det.sign().mul(dHdxy.x().mul(R1).add(dHdxy.y().mul(R2)));
    return det.abs().mul(surf_norm).sub(vGrad).normalize();
};
/*
 * Given a position expression, computes screen-space normals using
 * pixel derivatives
 */
Shade.ThreeD.normal = function(position)
{
    if (position.type.equals(Shade.Types.vec4))
        position = position.swizzle("xyz").div(position.w());
    var dPos_dpixelx = Shade.dFdx(position);
    var dPos_dpixely = Shade.dFdy(position);
    return Shade.normalize(Shade.cross(dPos_dpixelx, dPos_dpixely));
};
Shade.ThreeD.cull_backface = Shade(function(position, ccw)
{
    if (_.isUndefined(ccw)) ccw = Shade(true);
    ccw = ccw.ifelse(1, -1);
    var n = Shade.ThreeD.normal(position);
    return position.discard_if(n.cross(Shade.vec(0,0,ccw)).z().gt(0));
});
Lux.Geometry = {};
Lux.Geometry.triangulate = function(opts) {
    var poly = _.map(opts.contour, function(contour) {
        var p = [];
        for (var i=0; i<contour.length; ++i) {
            p.push(contour[i][0], contour[i][1]);
        }
        return p;
    });
    return Lux.Lib.tessellate(poly);
};
Lux.Geometry.PLY = {};

Lux.Geometry.PLY.load = function(url, k) {

    function property_size(prop) {
        // char       character                 1
        // uchar      unsigned character        1
        // short      short integer             2
        // ushort     unsigned short integer    2
        // int        integer                   4
        // uint       unsigned integer          4
        // float      single-precision float    4
        // double     double-precision float    8
        return {'char': 1,
                'uchar': 1,
                'short': 2,
                'ushort': 2,
                'int': 4,
                'uint': 4,
                'float': 4,
                'double': 8}[prop.type];
    }
    function property_dataview_setter(prop) {
        return {'char': 'setInt8',
                'uchar': 'setUint8',
                'short': 'setInt16',
                'ushort': 'setUint16',
                'int': 'setInt32',
                'uint': 'setUint32',
                'float': 'setFloat32',
                'double': 'setFloat64'}[prop.type];
    }

    Lux.Net.ajax(url, function(result) {
        var lines = result.split('\n');
        var current_line = 0;

        var header_res = [
                /^element.*/,
                /^comment.*/,
                /^format.*/
        ];

        function parse_header() {
            var header = { 
                elements: [],
                comments: []
            };
            function fail() {
                throw new Error("parse error on line " + (current_line+1) + ": '" + lines[current_line] + "'");
            }
            if (lines[current_line] !== 'ply') {
                fail();
            }
            ++current_line;
            function parse_element_header() {
                var line = lines[current_line].trim().split(' ');
                ++current_line;
                var result = { name: line[1], count: Number(line[2]), 
                               properties: [] };
                line = lines[current_line].trim().split(' ');
                while (line[0] === 'property') {
                    if (line[1] === 'list') {
                        result.properties.push({ type: line[1], 
                                                 name: line[4],
                                                 element_type: line[3] });
                    } else {
                        result.properties.push({ type: line[1], name: line[2] });
                    }
                    ++current_line;
                    line = lines[current_line].trim().split(' ');
                }
                return result;
            }
            while (lines[current_line] !== 'end_header') {
                if (lines[current_line].match(/^element.*/)) {
                    header.elements.push(parse_element_header());
                } else if (lines[current_line].match(/^comment.*/)) {
                    header.comments.push(lines[current_line].trim().split(' ').slice(1).join(" "));
                    ++current_line;
                } else if (lines[current_line].match(/^format.*/)) {
                    header.format = lines[current_line].trim().split(' ').slice(1);
                    ++current_line;
                } else
                    fail();
            }
            current_line++;
            return header;
        };

        // element list parsing is currently very very primitive, and
        // limited to polygonal faces one typically sees in PLY files.

        function parse_element_list(element_header) {
            if (element_header.name !== 'face' ||
                element_header.properties.length !== 1 ||
                element_header.properties[0].element_type !== 'int') {
                throw new Error("element lists are only currently supported for 'face' element and a single property if type 'int'");
            }
            var result = [];
            var max_v = 0;
            for (var i=0; i<element_header.count; ++i) {
                var row = _.map(lines[current_line].trim().split(' '), Number);
                current_line++;
                if (row.length < 4)
                    continue;
                var vertex1 = row[1];
                max_v = Math.max(max_v, row[1], row[2]);
                for (var j=2; j<row.length-1; ++j) {
                    result.push(vertex1, row[j], row[j+1]);
                    max_v = Math.max(max_v, row[j+1]);
                }
            }
            if (max_v > 65535)
                return new Uint32Array(result);
            else
                return new Uint16Array(result);
        }

        function parse_element(element_header) {
            // are we parsing list properties?
            if (_.any(element_header.properties, function(prop) { return prop.type === 'list'; })) {
                if (_.any(element_header.properties, function(prop) { return prop.type !== 'list'; })) {
                    throw new Error("this PLY parser does not currently support mixed property types");
                }
                return parse_element_list(element_header);
            }
            // no, this is a plain property array
            // 
            // we always use a single arraybuffer and stride into it for performance.
            var row_size = _.reduce(_.map(element_header.properties, property_size),
                                    function(a,b) { return a+b; }, 0);
            var result_buffer = new ArrayBuffer(element_header.count * row_size);
            var view = new DataView(result_buffer);
            var row_offset = 0;
            var row_offsets = [];
            var property_setters = _.map(element_header.properties, function(prop) {
                return view[property_dataview_setter(prop)];
            });
            _.each(element_header.properties, function(prop) {
                row_offsets.push(row_offset);
                row_offset += property_size(prop);
            });
            var n_props = row_offsets.length;
            var endian = Lux._globals.ctx._lux_globals.little_endian;
            for (var i=0; i<element_header.count; ++i) {
                var row = _.map(lines[current_line].trim().split(' '), Number);
                current_line++;
                for (var j=0; j<row_offsets.length; ++j) {
                    property_setters[j].call(view, i * row_size + row_offsets[j], row[j], endian);
                };
            }
            return result_buffer;
        }

        function parse_content() {
            if (header.format[0] !== 'ascii' ||
                header.format[1] !== '1.0')
                throw new Error("format is unsupported: " + header.format.join(' '));
            return _.object(_.map(header.elements, function(element) {
                return [element.name, parse_element(element)];
            }));
        }

        var header = parse_header();
        var content = parse_content();
        k({ header: header, content: content });
    });
};
Lux.Text = {};

/*
 * We include here a shim for typeface.js (http://typeface.neocracy.org)
 * 
 * In case typeface.js is already loaded,
 * this is a no-op. Otherwise, we substitute the minimal API we need for loading
 * typeface data as returned by typefaceJS.pm (typefaceJS.pm is *different* from typeface.js.
 * it is a Perl module to convert TTF to the format required by typeface.js)
 * 
 */

if (_.isUndefined(window._typeface_js)) {
    /* we only need one basic function from typeface_js, and we include it from
     * the original file, which is MIT licensed and copyright 2008-2009 David Chester
     * 
     * http://typeface.neocracy.org/typeface-0.15.js
     */
    window._typeface_js = {
        faces: {},
	loadFace: function(typefaceData) {
	    var familyName = typefaceData.familyName.toLowerCase();
	    if (!this.faces[familyName]) {
		this.faces[familyName] = {};
	    }
	    if (!this.faces[familyName][typefaceData.cssFontWeight]) {
		this.faces[familyName][typefaceData.cssFontWeight] = {};
	    }
	    var face = this.faces[familyName][typefaceData.cssFontWeight][typefaceData.cssFontStyle] = typefaceData;
	    face.loaded = true;
	}
    };
}
(function() {

function parse_typeface_instructions(glyph)
{
    // convert the string of typeface instructions coming from a typeface.js glyph
    // representation to a list of "paths", each path being a list of points
    // which are the glyph "internal polygon", and a list of "ears", the quadratic
    // splines that are to be rendered using Loop-Blinn.

    // this function mutates the passed glyph to memoize the result for increased
    // performance.

    if (_.isUndefined(glyph.o))
        return [];

    var x, y, cpx, cpy;
    var ops = _.map(glyph.o.split(" "), function(e) {
        var n = Number(e);
        return isNaN(n) ? e : n;
    });
    ops = ops.slice(0, ops.length-1);

    var paths = [];
    var points = [];
    var quadratic_ears = [];
    var current_point = undefined, control_point;
    var next_point = undefined;
    var pc = 0;
    var quadratic_sign, opcode;
    while (pc < ops.length) {
        switch (opcode = ops[pc++]) {
        case "m":
            if (points.length || quadratic_ears.length) {
                paths.push({points: points,
                            ears: quadratic_ears});
                points = [];
                quadratic_ears = [];
            }
            x = ops[pc++];
            y = ops[pc++];
            current_point = vec.make([x, y]);
            break;
        case "q":
            x = ops[pc++];
            y = ops[pc++];
            cpx = ops[pc++];
            cpy = ops[pc++];
            next_point = vec.make([x, y]);
            control_point = vec.make([cpx, cpy]);
            quadratic_sign = vec.cross(vec.minus(control_point, current_point),
                                       vec.minus(next_point, control_point));
            quadratic_sign = quadratic_sign / Math.abs(quadratic_sign);
            quadratic_ears.push([current_point, control_point, next_point, quadratic_sign]);

            if (quadratic_sign < 0) {
                if (current_point)
                    points.push(current_point);
                current_point = next_point;
            } else {
                if (current_point)
                    points.push(current_point);
                points.push(control_point);
                current_point = next_point;
            }
            break;
        case "l":
            if (current_point)
                points.push(current_point);
            x = ops[pc++];
            y = ops[pc++];
            current_point = vec.make([x, y]);
            break;
        default:
            throw new Error("Unsupported opcode '" + opcode + "'");
        };
    }
    if (points.length || quadratic_ears.length)
        paths.push({points: points,
                    ears: quadratic_ears});

    return paths;
}

var loop_blinn_actor = function(opts) {
    var position_function = opts.position;
    var color_function = opts.color;
    
    function quadratic_discriminator(model) {
        var u = model.uv.x(), v = model.uv.y(), 
        winding = model.winding.sign();
        return u.mul(u).sub(v).mul(winding);
    }
    
    function quadratic_discard(exp, model) {
        return exp.discard_if(quadratic_discriminator(model).gt(0));
    };

    var model = {};
    var uv = Lux.attribute_buffer({vertex_array: [0,0], item_size: 2});
    var winding = Lux.attribute_buffer({vertex_array: [0], item_size: 1});
    var position = Lux.attribute_buffer({vertex_array: [0,0], item_size: 2});
    var internal_position_attribute = Lux.attribute_buffer({vertex_array: [0,0], item_size: 2});
    var elements = Lux.element_buffer([0]); // {vertex_array: []});
    
    var ears_model = Lux.model({
        uv: uv,
        position: position,
        winding: winding,
        elements: 1,
        type: "triangles"
    });
    var x_offset = Shade.parameter("float", 0);
    var y_offset = Shade.parameter("float", 0);
    var offset = Shade.vec(x_offset, y_offset);
    var ears_position = Shade.add(ears_model.position, offset);
    var ears_actor = Lux.actor({
        model: ears_model, 
        appearance: {
            position: position_function(ears_position.div(1000).mul(opts.size)),
            color: quadratic_discard(color_function(ears_position), ears_model)}});
    var internal_model = Lux.model({
        vertex: internal_position_attribute,
        elements: elements
    });
    var internal_position = Shade.add(internal_model.vertex, offset);
    var internal_actor = Lux.actor({
        model: internal_model, 
        appearance: {
            position: position_function(internal_position.div(1000).mul(opts.size)),
            elements: internal_model.elements,
            color: color_function(internal_position)}});
    return {
        ears_actor: ears_actor,
        ears_model: ears_model,
        internal_actor: internal_actor,
        internal_model: internal_model,
        x_offset: x_offset,
        y_offset: y_offset
    };
};

function glyph_to_model(glyph)
{
    if (_.isUndefined(glyph._model)) {
        var paths = parse_typeface_instructions(glyph);
        if (paths.length === 0)
            return undefined;

        var elements = [], vertex = [], uv = [], position = [], winding = [];
        _.each(paths, function(path) {
            _.each(path.ears, function(ear) {
                winding.push.apply(winding, [-ear[3], -ear[3], -ear[3]]);
                position.push.apply(position, ear[0]);
                position.push.apply(position, ear[1]);
                position.push.apply(position, ear[2]);
                uv.push.apply(uv, [0,0, 0.5,0, 1,1]);
            });
        });

        var contour = _.map(paths, function(path) {
            return path.points.slice().reverse();
        });
        var triangulation = Lux.Geometry.triangulate({ contour: contour });
        var internal_model = Lux.model({
            type: "triangles",
            vertex: Lux.attribute_buffer({vertex_array: new Float32Array(triangulation.vertices), item_size: 2, keep_array: true}),
            elements: _.toArray(triangulation.triangles)
        });

        var ears_model = Lux.model({
            uv: Lux.attribute_buffer({vertex_array: uv, item_size: 2, keep_array: true}),
            position: Lux.attribute_buffer({vertex_array: position, item_size: 2, keep_array: true}),
            winding: Lux.attribute_buffer({vertex_array: winding, item_size: 1, keep_array: true}),
            elements: uv.length/2,
            type: "triangles"
        });

        glyph._model = {
            ears_model: ears_model, 
            internal_model: internal_model
        };
    };
    return glyph._model;
}

Lux.Text.outline = function(opts) {
    opts = _.defaults(opts, {
        string: "",
        size: 10,
        align: "left",
        position: function(pos) { return Shade.vec(pos, 0, 1); },
        color: function(pos) { return Shade.color("white"); }
    });
    if (_.isUndefined(opts.font)) {
        throw new Error("outline requires font parameter");
    }
    var actor = loop_blinn_actor(opts);

    var result = {
        set: function(new_string) {
            opts.string = new_string;
        },
        advance: function(char_offset) {
            var result = 0;
            while (char_offset < opts.string.length &&
                   "\n\r".indexOf(opts.string[char_offset])) {
                result += opts.font.glyphs[opts.string[char_offset++]].ha;
            }
            return result;
        },
        alignment_offset: function(char_offset) {
            var advance = this.advance(char_offset);
            switch (opts.align) {
            case "left": return 0;
            case "right": return -advance;
            case "center": return -advance/2;
            default:
                throw new Error("align must be one of 'left', 'center' or 'right'");
            }
        },
        dress: function(scene) {
            actor.internal_batch = actor.internal_actor.dress(scene);
            actor.ears_batch = actor.ears_actor.dress(scene);
            return {
                draw: function() {
                    actor.x_offset.set(result.alignment_offset(0));
                    actor.y_offset.set(0);
                    for (var i=0; i<opts.string.length; ++i) {
                        var c = opts.string[i];
                        if ("\n\r".indexOf(c) !== -1) {
                            actor.x_offset.set(0);                    
                            actor.y_offset.set(actor.y_offset.get() - opts.font.lineHeight);
                            continue;
                        }
                        var glyph = opts.font.glyphs[c];
                        if (_.isUndefined(glyph))
                            glyph = opts.font.glyphs['?'];
                        var model = glyph_to_model(glyph);
                        if (model) {
                            actor.ears_model.elements = model.ears_model.elements;
                            actor.ears_model.uv.set(model.ears_model.uv.get());
                            actor.ears_model.winding.set(model.ears_model.winding.get());
                            actor.ears_model.position.set(model.ears_model.position.get());
                            actor.internal_model.vertex.set(model.internal_model.vertex.get());
                            actor.internal_model.elements.set(model.internal_model.elements.array);
                            actor.ears_batch.draw();
                            actor.internal_batch.draw();
                        }
                        actor.x_offset.set(actor.x_offset.get() + glyph.ha);
                    }
                }
            };
        },
        on: function() { return true; }
    };
    return result;
};

})();
(function() {

function internal_actor(opts) {
    var texture = opts.font.texture;
    var texture_width = opts.font.texture_width;
    var texture_height = opts.font.texture_height;
    
    var position_function = opts.position;
    var color_function = opts.color;

    var uv = Lux.attribute_buffer({vertex_array: [0,0, 1, 0, 1, 1], item_size: 2});
    var position = Lux.attribute_buffer({vertex_array: [0,0, 1, 0, 1, 1], item_size: 2});
    var elements = Lux.element_buffer([0, 1, 2]);

    var x_offset = Shade.parameter("float", 0);
    var y_offset = Shade.parameter("float", 0);
    var offset = Shade.vec(x_offset, y_offset);
    var model = Lux.model({
        uv: uv,
        position: position,
        elements: elements,
        type: "triangles"
    });
    var world_position = Shade.add(model.position, offset).div(opts.font.ascender).mul(opts.size);
    var opacity = Shade.texture2D(texture, model.uv).r();
    var uv_gradmag = model.uv.x().mul(texture_width).dFdx().pow(2).add(model.uv.y().mul(texture_height).dFdy().pow(2)).sqrt();

    var blur_compensation = Shade.Scale.linear(
        {domain: [Shade.max(Shade(0.5).sub(uv_gradmag), 0), Shade.min(Shade(0.5).add(uv_gradmag), 1)],
         range: [0, 1]})(opacity).clamp(0, 1);

    var final_opacity = Shade.ifelse(opts.compensate_blur, blur_compensation, opacity);

    var final_color = color_function(world_position).mul(Shade.vec(1,1,1, final_opacity));
    var actor = Lux.actor({
        model: model, 
        appearance: {
            position: position_function(world_position),
            color: final_color,
            elements: model.elements,
            mode: Lux.DrawingMode.over_with_depth }});
    return {
        actor: actor,
        model: model,
        x_offset: x_offset,
        y_offset: y_offset
    };
}

function glyph_to_model(glyph, font)
{
    if (_.isUndefined(glyph._model)) {
        var elements = [0, 1, 2, 0, 2, 3];
        var position = [glyph.left, glyph.top - glyph.glyph_height,
                        glyph.left + glyph.glyph_width, glyph.top - glyph.glyph_height,
                        glyph.left + glyph.glyph_width, glyph.top,
                        glyph.left, glyph.top];
        var uv = [glyph.xoff / font.texture_width, 1 - (glyph.yoff + glyph.glyph_height) / font.texture_height,
                  (glyph.xoff + glyph.glyph_width) / font.texture_width, 1 - (glyph.yoff + glyph.glyph_height) / font.texture_height,
                  (glyph.xoff + glyph.glyph_width) / font.texture_width, 1 - glyph.yoff/font.texture_height,
                  glyph.xoff / font.texture_width, 1 - glyph.yoff/font.texture_height];
        glyph._model = Lux.model({
            type: "triangles",
            uv: Lux.attribute_buffer({vertex_array: uv, item_size: 2, keep_array: true}),
            position: Lux.attribute_buffer({vertex_array: position, item_size: 2, keep_array: true}),
            elements: Lux.element_buffer(elements)
        });
    }
    return glyph._model;
}

Lux.Text.texture = function(opts) {
    opts = _.defaults(opts, {
        string: "",
        size: 10,
        align: "left",
        position: function(pos) { return Shade.vec(pos, 0, 1); },
        color: function(pos) { return Shade.color("white"); },
        onload: function() { Lux.Scene.invalidate(); },
        compensate_blur: true
    });

    if (_.isUndefined(opts.font)) {
        throw new Error("Lux.Text.texture requires font parameter");
    }

    var actor = {};

    if (!opts.font.texture) {
        opts.font.texture = Lux.texture({
            src: opts.font.image_url,
            mipmaps: false,
            onload: function() {
                return opts.onload();
            }
        });
    }
    actor = internal_actor(opts);

    var result = {
        set: function(new_string) {
            opts.string = new_string;
        },
        advance: function(char_offset) {
            var result = 0;
            while (char_offset < opts.string.length &&
                   "\n\r".indexOf(opts.string[char_offset])) {
                // oh god i need to fix this mess
                var ix = String(opts.string[char_offset++].charCodeAt(0));
                result += opts.font.glyphs[ix].ha;
            }
            return result;
        },
        alignment_offset: function(char_offset) {
            var advance = this.advance(char_offset);
            switch (opts.align) {
            case "left": return 0;
            case "right": return -advance;
            case "center": return -advance/2;
            default:
                throw new Error("align must be one of 'left', 'center' or 'right'");
            }
        },
        dress: function(scene) {
            actor.batch = actor.actor.dress(scene);
            return {
                draw: function() {
                    actor.x_offset.set(result.alignment_offset(0));
                    actor.y_offset.set(0);
                    for (var i=0; i<opts.string.length; ++i) {
                        var c = opts.string[i];
                        if ("\n\r".indexOf(c) !== -1) {
                            actor.x_offset.set(0);
                            actor.y_offset.set(actor.y_offset.get() - opts.font.lineheight);
                            continue;
                        }
                        var glyph = opts.font.glyphs[String(c.charCodeAt(0))];
                        if (_.isUndefined(glyph))
                            glyph = opts.font.glyphs[String('?'.charCodeAt(0))];
                        var model = glyph_to_model(glyph, opts.font);
                        if (model) {
                            actor.model.elements = model.elements;
                            actor.model.uv.set(model.uv.get());
                            actor.model.position.set(model.position.get());
                            actor.batch.draw();
                        }
                        actor.x_offset.set(actor.x_offset.get() + glyph.ha);
                    }
                }
            };
        },
        on: function() { return true; }
    };
    return result;
};

})();
Lux.Debug = {};
Lux.Debug.init = function(div)
{
    if (Lux._globals.debug_table)
        return;
    if (_.isUndefined(div)) {
        div = $('<div style="position:absolute;left:1em;top:1em"></div>');
        $('body').append(div);
    }
    var table = $("<table></table>");
    div.append(table);
    Lux._globals.debug_table = table;
    Lux._globals.debug_dict = {};
};
Lux.Debug.post = function(key, value)
{
    Lux.Debug.init();
    var str = '<td>' + key + '</td><td>' + value + '</td>';
    if (Lux._globals.debug_dict[key]) {
        Lux._globals.debug_dict[key].html(str);
    } else {
        Lux._globals.debug_dict[key] = $('<tr>' + str + '</tr>');
        Lux._globals.debug_table.append(Lux._globals.debug_dict[key]);
    }
};
Lux.Marks = {};
//////////////////////////////////////////////////////////////////////////
// This is like a poor man's instancing/geometry shader. I need a
// general API for it.

Lux.Marks.aligned_rects = function(opts)
{
    opts = _.defaults(opts || {}, {
        mode: Lux.DrawingMode.standard,
        z: function() { return 0; }
    });
    if (!opts.elements) throw new Error("elements is a required field");
    if (!opts.left)     throw new Error("left is a required field");
    if (!opts.right)    throw new Error("right is a required field");
    if (!opts.top)      throw new Error("top is a required field");
    if (!opts.bottom)   throw new Error("bottom is a required field");
    if (!opts.color)    throw new Error("color is a required field");

    var index = _.range(opts.elements * 6);
    var vertex_index = Lux.attribute_buffer({ 
        vertex_array: index, 
        item_size: 1
    });
    var primitive_index = Shade.div(vertex_index, 6).floor();
    var vertex_in_primitive = Shade.mod(vertex_index, 6).floor();

    // aif == apply_if_function
    var aif = function(f, params) {
        if (lux_typeOf(f) === 'function')
            return f.apply(this, params);
        else
            return f;
    };

    var left   = aif(opts.left,   [primitive_index]),
        right  = aif(opts.right,  [primitive_index]),
        bottom = aif(opts.bottom, [primitive_index]),
        top    = aif(opts.top,    [primitive_index]),
        color  = aif(opts.color,  [primitive_index, index_in_vertex_primitive]),
        z      = aif(opts.z,      [primitive_index]);

    var lower_left  = Shade.vec(left,  bottom);
    var lower_right = Shade.vec(right, bottom);
    var upper_left  = Shade.vec(left,  top);
    var upper_right = Shade.vec(right, top);
    var vertex_map  = Shade.array([lower_left, upper_right, upper_left,
                                   lower_left, lower_right, upper_right]);
    var index_array = Shade.array([0, 2, 3, 0, 1, 2]);
    var index_in_vertex_primitive = index_array.at(vertex_in_primitive);

    var model = Lux.model({
        type: "triangles",
        elements: index
    });

    var appearance = {
        position: Shade.vec(vertex_map.at(vertex_in_primitive), z),
        color: color,
        pick_id: opts.pick_id,
        mode: opts.mode
    };

    return Lux.actor({ model: model, appearance: appearance });
};
Lux.Marks.lines = function(opts)
{
    opts = _.defaults(opts || {}, {
        mode: Lux.DrawingMode.standard,
        z: function() { return 0; }
    });

    if (_.isUndefined(opts.elements)) throw new Error("elements is a required field");
    if (_.isUndefined(opts.color))    throw new Error("color is a required field");
    if (_.isUndefined(opts.position) && 
        (_.isUndefined(opts.x) || _.isUndefined(opts.y))) {
        throw new Error("either position or x and y are required fields");
    }

    var vertex_index        = Lux.attribute_buffer({
        vertex_array: _.range(opts.elements * 2), 
        item_size: 1
    });
    var primitive_index     = Shade.div(vertex_index, 2).floor();
    var vertex_in_primitive = Shade.mod(vertex_index, 2).floor();

    var position = opts.position 
        ? opts.position(primitive_index, vertex_in_primitive)
        : Shade.vec(opts.x(primitive_index, vertex_in_primitive),
                    opts.y(primitive_index, vertex_in_primitive),
                    opts.z(primitive_index, vertex_in_primitive));

    var appearance = {
        mode: opts.mode,
        position: position,
        color: opts.color(primitive_index, vertex_in_primitive)
    };
    if (opts.line_width) {
        appearance.line_width = opts.line_width;
    }
    var model = {
        type: "lines",
        elements: vertex_index
    };
    return Lux.actor({ model: model, appearance: appearance });
};
Lux.Marks.dots = function(opts)
{
    opts = _.defaults(opts, {
        fill_color: Shade.vec(0,0,0,1),
        stroke_color: Shade.vec(0,0,0,1),
        point_diameter: 5,
        stroke_width: 2,
        mode: Lux.DrawingMode.over_with_depth,
        alpha: true,
        plain: false
    });

    if (!opts.position)
        throw new Error("missing required parameter 'position'");
    if (!opts.elements)
        throw new Error("missing required parameter 'elements'");

    var S = Shade;
    var ctx = Lux._globals.ctx;

    var fill_color     = Shade(opts.fill_color);
    var stroke_color   = Shade(opts.stroke_color);
    var point_diameter = Shade(opts.point_diameter).mul(ctx._lux_globals.devicePixelRatio);
    var stroke_width   = Shade(opts.stroke_width).add(1);
    var use_alpha      = Shade(opts.alpha);
    opts.plain = Shade(opts.plain);
    
    var model_opts = {
        type: "points",
        vertex: opts.position,
        elements: opts.elements
    };

    var model = Lux.model(model_opts);

    var distance_to_center_in_pixels = S.pointCoord().sub(S.vec(0.5, 0.5))
        .norm().mul(point_diameter);
    var point_radius = point_diameter.div(2);
    var distance_to_border = point_radius.sub(distance_to_center_in_pixels);
    var gl_Position = model.vertex;

    var no_alpha = S.mix(fill_color, stroke_color,
                         S.clamp(stroke_width.sub(distance_to_border), 0, 1));
    
    var plain_fill_color = fill_color;
    var alpha_fill_color = 
        S.ifelse(use_alpha,
                    no_alpha.mul(S.vec(1,1,1,S.clamp(distance_to_border, 0, 1))),
                    no_alpha)
        .discard_if(distance_to_center_in_pixels.gt(point_radius));

    var result = Lux.actor({
        model: model, 
        appearance: {
            position: gl_Position,
            point_size: point_diameter,
            color: opts.plain.ifelse(plain_fill_color, alpha_fill_color),
            mode: opts.mode,
            pick_id: opts.pick_id }});

    /* We pass the gl_Position attribute explicitly because some other
     call might want to explicitly use the same position of the dots marks.

     This is the exact use case of dot-and-line graph drawing.
     */
    result.gl_Position = gl_Position;
    return result;
};
Lux.Marks.scatterplot = function(opts)
{
    opts = _.defaults(opts, {
        x_scale: function (x) { return x; },
        y_scale: function (x) { return x; },
        xy_scale: function (x) { return x; }
    });

    function to_opengl(x) { return x.mul(2).sub(1); }
    var S = Shade;
    
    var x_scale = opts.x_scale;
    var y_scale = opts.y_scale;

    var position, elements;

    if (!_.isUndefined(opts.x)) {
        position = S.vec(to_opengl(opts.x_scale(opts.x)), 
                         to_opengl(opts.y_scale(opts.y)));
    } else if (!_.isUndefined(opts.xy)) {
        position = opts.xy_scale(opts.xy).mul(2).sub(S.vec(1,1));
    }

    if (opts.model) {
        elements = opts.model.elements;
    } else if (opts.elements) {
        elements = opts.elements;
    }
    return Lux.Marks.dots({
        position: position,
        elements: elements,
        fill_color: opts.fill_color,
        stroke_color: opts.stroke_color,
        point_diameter: opts.point_diameter,
        stroke_width: opts.stroke_width,
        mode: opts.mode,
        alpha: opts.alpha,
        plain: opts.plain,
        pick_id: opts.pick_id
    });
};
Lux.Marks.rectangle_brush = function(opts)
{
    opts = _.defaults(opts || {}, {
        color: Shade.vec(1,1,1,0.5),
        mode: Lux.DrawingMode.over_no_depth,
        on: {}
    });
    var gl = Lux._globals.ctx;
    var brush_is_active = false;
    var unproject;
    var selection_pt1 = Shade.parameter("vec2", vec.make([0,0]));
    var selection_pt2 = Shade.parameter("vec2", vec.make([0,0]));
    var b1;

    var handlers = {
        mousedown: function(event) {
            if (opts.accept_event(event)) {
                var xy_v = unproject(vec.make([event.luxX / gl._lux_globals.devicePixelRatio, event.luxY / gl._lux_globals.devicePixelRatio]));
                b1 = xy_v;
                selection_pt1.set(xy_v);
                brush_is_active = true;
                opts.on.brush_started && opts.on.brush_started(b1);
                return false;
            }
            return true;
        },
        mousemove: function(event) { 
            if (!brush_is_active)
                return true;
            if (opts.accept_event(event)) {
                var xy_v = unproject(vec.make([event.luxX / gl._lux_globals.devicePixelRatio, event.luxY / gl._lux_globals.devicePixelRatio]));
                selection_pt2.set(xy_v);
                var b2 = xy_v;
                opts.on.brush_changed && opts.on.brush_changed(b1, b2);
                Lux.Scene.invalidate();
                return false;
            }
            return true;
        },
        mouseup: function(event) {
            if (!brush_is_active)
                return true;
            brush_is_active = false;
            if (opts.accept_event(event)) {
                var xy_v = unproject(vec.make([event.luxX / gl._lux_globals.devicePixelRatio, event.luxY / gl._lux_globals.devicePixelRatio]));
                selection_pt2.set(xy_v);
                var b2 = xy_v;
                if (opts.on.brush_changed) {
                    opts.on.brush_changed(b1, b2);
                } else if (opts.on.brush_ended) {
                    opts.on.brush_ended(b1, b2);
                }
                Lux.Scene.invalidate();
                return false;
            }
            return true;
        }
    };

    var brush_actor = Lux.Marks.aligned_rects({
        elements: 1,
        left: selection_pt1.x(),
        right: selection_pt2.x(),
        top: selection_pt1.y(),
        bottom: selection_pt2.y(),
        color: opts.color,
        mode: opts.mode
    });

    return {
        dress: function(scene) {
            var ctx = Lux._globals.ctx;
            var xform = scene.get_transform();
            var half_screen_size = Shade.vec(ctx.parameters.width, ctx.parameters.height).div(2);
            unproject = Shade(function(p) {
                return xform.inverse({position: p.div(half_screen_size).sub(Shade.vec(1,1))}).position;
            }).js_evaluate;
            return brush_actor.dress(scene);
        }, on: function(ename, event) {
            var handler = handlers[ename];
            if (_.isUndefined(handler))
                return true;
            return handler(event);
        }
    };
};
function spherical_mercator_patch(tess)
{
    var uv = [];
    var elements = [];
    var i, j;

    for (i=0; i<=tess; ++i)
        for (j=0; j<=tess; ++j)
            uv.push(i/tess, j/tess);

    for (i=0; i<tess; ++i)
        for (j=0; j<tess; ++j) {
            var ix = (tess + 1) * i + j;
            elements.push(ix, ix+1, ix+tess+2, ix, ix+tess+2, ix+tess+1);
        }

    return Lux.model({
        type: "triangles",
        uv: [uv, 2],
        elements: elements,
        vertex: function(min, max) {
            var xf = this.uv.mul(max.sub(min)).add(min);
            return Shade.Scale.Geo.mercator_to_spherical(xf.at(0), xf.at(1));
        },
        transformed_uv: function(min, max) {
            return Shade.mix(min, max, this.uv).div(Math.PI * 2).add(Shade.vec(0, 0.5));
        }
    });
}

function latlong_to_mercator(lat, lon)
{
    lat = lat / (180 / Math.PI);
    lon = lon / (180 / Math.PI);
    return [lon, Math.log(1.0/Math.cos(lat) + Math.tan(lat))];
}

Lux.Marks.globe = function(opts)
{
    opts = _.defaults(opts || {}, {
        longitude_center: -98,
        latitude_center: 38,
        zoom: 3,
        resolution_bias: 0,
        patch_size: 10,
        tile_pattern: function(zoom, x, y) {
            return "http://tile.openstreetmap.org/"+zoom+"/"+x+"/"+y+".png";
        }
    });
    var model = Shade.parameter("mat4");
    var patch = spherical_mercator_patch(opts.patch_size);
    var cache_size = 64; // cache size must be (2^n)^2
    var tile_size = 256;
    var tiles_per_line  = 1 << (~~Math.round(Math.log(Math.sqrt(cache_size))/Math.log(2)));
    var super_tile_size = tile_size * tiles_per_line;

    var ctx = Lux._globals.ctx;
    var texture = Lux.texture({
        width: super_tile_size,
        height: super_tile_size
    });

    function new_tile(i) {
        var x = i % tiles_per_line;
        var y = ~~(i / tiles_per_line);
        return {
            texture: texture,
            offset_x: x,
            offset_y: y,
            // 0: inactive,
            // 1: mid-request,
            // 2: ready to draw.
            active: 0,
            x: -1,
            y: -1,
            zoom: -1,
            last_touched: 0
        };
    };

    var tiles = [];
    for (var i=0; i<cache_size; ++i) {
        tiles.push(new_tile(i));
    };

    var zooming = false, panning = false;
    var prev = [0,0];
    var inertia = 1;
    var move_vec = [0,0];

    // FIXME for some reason, sometimes mouseup is preceded by a quick mousemove,
    // even when apparently no mouse movement was detected. This extra tick
    // throws my inertial browsing off. We work around by keeping the
    // second-to-last tick.

    var last_moves = [0,0];
    function log_move() {
        last_moves[1] = last_moves[0];
        last_moves[0] = new Date().getTime();
    }

    var min_x = Shade.parameter("float");
    var max_x = Shade.parameter("float");
    var min_y = Shade.parameter("float");
    var max_y = Shade.parameter("float");
    var offset_x = Shade.parameter("float");
    var offset_y = Shade.parameter("float");
    var texture_scale = 1.0 / tiles_per_line;
    var sampler = Shade.parameter("sampler2D");

    var v = patch.vertex(Shade.vec(min_x, min_y), 
                         Shade.vec(max_x, max_y));
    var mvp = opts.view_proj(model);

    var xformed_patch = patch.uv 
    // These two lines work around the texture seams on the texture atlas
        .mul((tile_size-1.0)/tile_size)
        .add(0.5/tile_size)
    //
        .add(Shade.vec(offset_x, offset_y))
        .mul(texture_scale)
    ;

    var sphere_actor = Lux.actor({
        model: patch, 
        appearance: {
            gl_Position: mvp(v), // Shade.ThreeD.cull_backface(mvp(v)),
            gl_FragColor: Shade.texture2D(sampler, xformed_patch).discard_if(model.mul(v).z().lt(0)),
            mode: Lux.DrawingMode.pass
        }});

    function inertia_tick() {
        var f = function() {
            Lux.Scene.invalidate();
            result.longitude_center += move_vec[0] * inertia;
            result.latitude_center  += move_vec[1] * inertia;
            result.latitude_center  = Math.max(Math.min(80, result.latitude_center), -80);
            result.update_model_matrix();
            if (inertia > 0.01)
                window.requestAnimFrame(f, result.canvas);
            inertia *= 0.95;
        };
        f();
    }

    if (lux_typeOf(opts.zoom) === "number") {
        opts.zoom = Shade.parameter("float", opts.zoom);
    } else if (Lux.is_shade_expression(opts.zoom) !== "parameter") {
        throw new Error("zoom must be either a number or a parameter");
    }

    var result = {
        tiles: tiles,
        queue: [],
        current_osm_zoom: 3,
        longitude_center: opts.longitude_center,
        latitude_center: opts.latitude_center,
        zoom: opts.zoom,
        model_matrix: model,
        mvp: mvp,
        lat_lon_position: function(lat, lon) {
            return mvp(Shade.Scale.Geo.latlong_to_spherical(lat, lon));
        },
        resolution_bias: opts.resolution_bias,
        update_model_matrix: function() {
            while (this.longitude_center < 0)
                this.longitude_center += 360;
            while (this.longitude_center > 360)
                this.longitude_center -= 360;
            var r1 = Lux.rotation(this.latitude_center * (Math.PI/180), [ 1, 0, 0]);
            var r2 = Lux.rotation(this.longitude_center * (Math.PI/180), [ 0,-1, 0]);
            this.model_matrix.set(mat4.product(r1, r2));
        },
        mousedown: function(event) {
            prev[0] = event.offsetX;
            prev[1] = event.offsetY;
            inertia = 0;
            Lux.Scene.invalidate();
        },
        mousemove: function(event) {
            var w = ctx.viewportWidth;
            var h = ctx.viewportHeight;
            var w_divider = 218.18;
            var h_divider = 109.09;
            var zoom = this.zoom.get();

            if ((event.which & 1) && !event.shiftKey) {
                panning = true;
                move_vec[0] = -(event.offsetX - prev[0]) / (w * zoom / w_divider);
                move_vec[1] =  (event.offsetY - prev[1]) / (h * zoom / h_divider);
                prev[0] = event.offsetX;
                prev[1] = event.offsetY;
                log_move();
                this.longitude_center += move_vec[0];
                this.latitude_center += move_vec[1];
                this.latitude_center = Math.max(Math.min(80, this.latitude_center), -80);
                this.update_model_matrix();
                Lux.Scene.invalidate();
            }
            if (event.which & 1 && event.shiftKey) {
                zooming = true;
                var new_zoom = this.zoom.get() * (1.0 + (event.offsetY - prev[1]) / 240);
                this.zoom.set(Math.max(new_zoom, 0.5));
                Lux.Scene.invalidate();
            }
            this.new_center(this.latitude_center, this.longitude_center, this.zoom.get());
            prev[0] = event.offsetX;
            prev[1] = event.offsetY;
        },
        mouseup: function(event) {
            var w = ctx.viewportWidth;
            var h = ctx.viewportHeight;
            var w_divider = 218.18;
            var h_divider = 109.09;
            var now = Date.now();
            // assume 16.66 ms per tick,
            inertia = Math.pow(0.95, (now - last_moves[1]) / 16.666);
            if (panning)
                inertia_tick();
            panning = zooming = false;
        },
        new_center: function(center_lat, center_lon, center_zoom) {
            var w = ctx.viewportWidth;
            var zoom_divider = 63.6396;
            var base_zoom = Math.log(w / zoom_divider) / Math.log(2);

            var zoom = this.resolution_bias + base_zoom + (Math.log(center_zoom / 2.6) / Math.log(2));
            zoom = ~~zoom;
            this.current_osm_zoom = zoom;
            var lst = latlong_to_mercator(center_lat, center_lon);
            var y = (lst[1] / (Math.PI * 2) + 0.5) * (1 << zoom);
            var x = lst[0] / (Math.PI * 2) * (1 << zoom);
            // var y = (center_lat + 90) / 180 * (1 << zoom);
            // var x = center_lon / 360 * (1 << zoom);
            y = (1 << zoom) - y - 1;
            x = (x + (1 << (zoom - 1))) & ((1 << zoom) - 1);

            for (var i=-2; i<=2; ++i) {
                for (var j=-2; j<=2; ++j) {
                    var rx = ~~x + i;
                    var ry = ~~y + j;
                    if (ry < 0 || ry >= (1 << zoom))
                        continue;
                    if (rx < 0)
                        rx += 1 << zoom;
                    if (rx >= (1 << zoom))
                        rx -= 1 << zoom;
                    this.request(rx, ry, ~~zoom);
                }
            }
        },
        get_available_id: function(x, y, zoom) {
            // easy cases first: return available tile or a cache hit
            var now = Date.now();
            for (var i=0; i<cache_size; ++i) {
                if (this.tiles[i].x == x &&
                    this.tiles[i].y == y &&
                    this.tiles[i].zoom == zoom &&
                    this.tiles[i].active != 0) {
                    this.tiles[i].last_touched = now;
                    return i;
                }
            }
            for (i=0; i<cache_size; ++i) {
                if (!this.tiles[i].active) {
                    this.tiles[i].last_touched = now;
                    return i;
                }
            }
            // now we need to bump someone out. who?
            var worst_index = -1;
            var worst_time = 1e30;
            for (i=0; i<cache_size; ++i) {
                if (this.tiles[i].active == 1)
                    // don't use this one, it's getting bumped out
                    continue;
                var score = this.tiles[i].last_touched;
                if (score < worst_time) {
                    worst_time = score;
                    worst_index = i;
                }
            }
            return worst_index;
        },
        init: function() {
            for (var z=0; z<3; ++z)
                for (var i=0; i<(1 << z); ++i)
                    for (var j=0; j<(1 << z); ++j)
                        this.request(i, j, z);
            this.new_center(this.latitude_center, this.longitude_center, this.zoom.get());
            this.update_model_matrix();
        },
        sanity_check: function() {
            var d = {};
            for (var i=0; i<cache_size; ++i) {
                $("#x" + i).text(this.tiles[i].x);
                $("#y" + i).text(this.tiles[i].y);
                $("#z" + i).text(this.tiles[i].zoom);
                if (this.tiles[i].active !== 2)
                    continue;
                var k = this.tiles[i].x + "-" +
                    this.tiles[i].y + "-" +
                    this.tiles[i].zoom;
                if (d[k] !== undefined) {
                    console.log("BAD STATE!", 
                                this.tiles[i].x, this.tiles[i].y, this.tiles[i].zoom, 
                                this.tiles[i].active,
                                k);                    
                    throw new Error("Internal Error in globe");
                }
                d[k] = true;
            }
        },
        request: function(x, y, zoom) {
            var that = this;
            var id = this.get_available_id(x, y, zoom);
            if (id === -1) {
                alert("Could not fulfill request " + x + " " + y + " " + zoom);
                return;
            }
            if (this.tiles[id].x == x && 
                this.tiles[id].y == y && 
                this.tiles[id].zoom == zoom) {
                return;
            }

            that.tiles[id].x = x;
            that.tiles[id].y = y;
            that.tiles[id].zoom = zoom;
            this.tiles[id].active = 1;
            var f = function(x, y, zoom, id) {
                return function() {
                    that.tiles[id].active = 2;
                    that.tiles[id].last_touched = new Date().getTime();
                    // uncomment this during debugging
                    // that.sanity_check();
                    Lux.Scene.invalidate();
                };
            };
            texture: tiles[id].texture.load({
                src: opts.tile_pattern(zoom, x, y),
                crossOrigin: "anonymous",
                x_offset: tiles[id].offset_x * tile_size,
                y_offset: tiles[id].offset_y * tile_size,
                onload: f(x, y, zoom, id)
            });
        },
        dress: function(scene) {
            var sphere_batch = sphere_actor.dress(scene);
            return {
                draw: function() {
                    var lst = _.range(cache_size);
                    var that = this;
                    lst.sort(function(id1, id2) { 
                        var g1 = Math.abs(tiles[id1].zoom - that.current_osm_zoom);
                        var g2 = Math.abs(tiles[id2].zoom - that.current_osm_zoom);
                        return g2 - g1;
                    });

                    sampler.set(texture);
                    for (var i=0; i<cache_size; ++i) {
                        var t = tiles[lst[i]];
                        if (t.active !== 2)
                            continue;
                        min_x.set((t.x / (1 << t.zoom))           * Math.PI*2 + Math.PI);
                        min_y.set((1 - (t.y + 1) / (1 << t.zoom)) * Math.PI*2 - Math.PI);
                        max_x.set(((t.x + 1) / (1 << t.zoom))     * Math.PI*2 + Math.PI);
                        max_y.set((1 - t.y / (1 << t.zoom))       * Math.PI*2 - Math.PI);
                        offset_x.set(t.offset_x);
                        offset_y.set(t.offset_y);
                        sphere_batch.draw();
                    }
                }
            };
        },
        on: function() { return true; }
    };
    result.init();

    return result;
};
Lux.Marks.globe_2d = function(opts)
{
    opts = _.defaults(opts || {}, {
        zoom: 3,
        resolution_bias: -1,
        patch_size: 10,
        cache_size: 3, // 3: 64 images; 4: 256 images.
        tile_pattern: function(zoom, x, y) {
            return "http://tile.openstreetmap.org/"+zoom+"/"+x+"/"+y+".png";
        },
        camera: function(v) { return v; },
        debug: false, // if true, add outline and x-y-zoom marker to every tile
        no_network: false, // if true, tile is always blank white and does no HTTP requests.
        post_process: function(c) { return c; }
    });

    var has_interactor = false, get_center_zoom;
    if (opts.interactor) {
        has_interactor = true;
        get_center_zoom = function() {
            return [opts.interactor.center.get()[0], 
                    opts.interactor.center.get()[1], 
                    opts.interactor.zoom.get()];
        };
    }
    if (opts.no_network) {
        opts.debug = true; // no_network implies debug;
    }

    var patch = Lux.model({
        type: "triangles",
        uv: [[0,0,1,0,1,1,0,0,1,1,0,1], 2],
        vertex: function(min, max) {
            return this.uv.mul(max.sub(min)).add(min);
        }
    });
    var cache_size = 1 << (2 * opts.cache_size);
    var tile_size = 256;
    var tiles_per_line  = 1 << (~~Math.round(Math.log(Math.sqrt(cache_size))/Math.log(2)));
    var super_tile_size = tile_size * tiles_per_line;

    var ctx = Lux._globals.ctx;
    var texture = Lux.texture({
        mipmaps: false,
        width: super_tile_size,
        height: super_tile_size
    });

    function new_tile(i) {
        var x = i % tiles_per_line;
        var y = ~~(i / tiles_per_line);
        return {
            texture: texture,
            offset_x: x,
            offset_y: y,
            // 0: inactive,
            // 1: mid-request,
            // 2: ready to draw.
            active: 0,
            x: -1,
            y: -1,
            zoom: -1,
            last_touched: 0
        };
    };

    var tiles = [];
    for (var i=0; i<cache_size; ++i) {
        tiles.push(new_tile(i));
    };

    var min_x = Shade.parameter("float");
    var max_x = Shade.parameter("float");
    var min_y = Shade.parameter("float");
    var max_y = Shade.parameter("float");
    var offset_x = Shade.parameter("float");
    var offset_y = Shade.parameter("float");
    var texture_scale = 1.0 / tiles_per_line;
    var sampler = Shade.parameter("sampler2D");

    var v = patch.vertex(Shade.vec(min_x, min_y), Shade.vec(max_x, max_y));

    var xformed_patch = patch.uv 
    // These two lines work around the texture seams on the texture atlas
        .mul((tile_size-1.0)/tile_size)
        .add(0.5/tile_size)
    //
        .add(Shade.vec(offset_x, offset_y))
        .mul(texture_scale)
    ;

    var tile_actor = Lux.actor({
        model: patch, 
        appearance: {
            position: opts.camera(v),
            color: opts.post_process(Shade.texture2D(sampler, xformed_patch)),
            mode: Lux.DrawingMode.pass }});

    if (lux_typeOf(opts.zoom) === "number") {
        opts.zoom = Shade.parameter("float", opts.zoom);
    } else if (Lux.is_shade_expression(opts.zoom) !== "parameter") {
        throw new Error("zoom must be either a number or a parameter");
    }

    var unproject;

    var result = {
        tiles: tiles,
        queue: [],
        current_osm_zoom: 0,
        lat_lon_position: Lux.Marks.globe_2d.lat_lon_to_tile_mercator,
        resolution_bias: opts.resolution_bias,
        new_center: function() {
            // ctx.viewport* here is bad...
            // var mn = unproject(vec.make([0, 0]));
            // var mx = unproject(vec.make([ctx.viewportWidth, ctx.viewportHeight]));
            var t = get_center_zoom();
            var center_x = t[0];
            var center_y = t[1];
            var center_zoom = t[2]; // opts.zoom.get();

            var screen_resolution_bias = Math.log(ctx.viewportHeight / 256) / Math.log(2);
            var zoom = this.resolution_bias + screen_resolution_bias + (Math.log(center_zoom) / Math.log(2));
            zoom = ~~zoom;
            this.current_osm_zoom = zoom;
            var y = center_y * (1 << zoom);
            var x = center_x * (1 << zoom);
            y = (1 << zoom) - y - 1;

            for (var i=-2; i<=2; ++i) {
                for (var j=-2; j<=2; ++j) {
                    var rx = ~~x + i;
                    var ry = ~~y + j;
                    if (ry < 0 || ry >= (1 << zoom))
                        continue;
                    if (rx < 0)
                        continue;
                    if (rx >= (1 << zoom))
                        continue;
                    this.request(rx, ry, ~~zoom);
                }
            }
        },
        get_available_id: function(x, y, zoom) {
            // easy cases first: return available tile or a cache hit
            var now = Date.now();
            for (var i=0; i<cache_size; ++i) {
                if (this.tiles[i].x == x &&
                    this.tiles[i].y == y &&
                    this.tiles[i].zoom == zoom &&
                    this.tiles[i].active != 0) {
                    this.tiles[i].last_touched = now;
                    return i;
                }
            }
            for (i=0; i<cache_size; ++i) {
                if (!this.tiles[i].active) {
                    this.tiles[i].last_touched = now;
                    return i;
                }
            }
            // now we need to bump someone out. who?
            var worst_index = -1;
            var worst_time = 1e30;
            for (i=0; i<cache_size; ++i) {
                if (this.tiles[i].active == 1)
                    // don't use this one, it's getting bumped out
                    continue;
                var score = this.tiles[i].last_touched;
                if (score < worst_time) {
                    worst_time = score;
                    worst_index = i;
                }
            }
            return worst_index;
        },
        init: function() {
            for (var z=0; z<3; ++z)
                for (var i=0; i<(1 << z); ++i)
                    for (var j=0; j<(1 << z); ++j)
                        this.request(i, j, z);
        },
        sanity_check: function() {
            var d = {};
            for (var i=0; i<cache_size; ++i) {
                $("#x" + i).text(this.tiles[i].x);
                $("#y" + i).text(this.tiles[i].y);
                $("#z" + i).text(this.tiles[i].zoom);
                if (this.tiles[i].active !== 2)
                    continue;
                var k = this.tiles[i].x + "-" +
                    this.tiles[i].y + "-" +
                    this.tiles[i].zoom;
                if (d[k] !== undefined) {
                    console.log("BAD STATE!", 
                                this.tiles[i].x, this.tiles[i].y, this.tiles[i].zoom, 
                                this.tiles[i].active,
                                k);                    
                    throw new Error("Internal Error in globe_2d");
                }
                d[k] = true;
            }
        },
        request: function(x, y, zoom) {
            var that = this;
            var id = this.get_available_id(x, y, zoom);
            if (id === -1) {
                console.log("Could not fulfill request " + x + " " + y + " " + zoom);
                return;
            }
            if (this.tiles[id].x == x && 
                this.tiles[id].y == y && 
                this.tiles[id].zoom == zoom) {
                return;
            }

            that.tiles[id].x = x;
            that.tiles[id].y = y;
            that.tiles[id].zoom = zoom;
            this.tiles[id].active = 1;
            var f = function(x, y, zoom, id) {
                return function() {
                    that.tiles[id].active = 2;
                    that.tiles[id].last_touched = Date.now();
                    // uncomment this during debugging
                    // that.sanity_check();
                    Lux.Scene.invalidate();
                };
            };
            var xform = opts.debug ? function(image) {
                var c = document.createElement("canvas");
                c.setAttribute("width", image.width);
                c.setAttribute("height", image.height);
                var ctx = c.getContext('2d');
                ctx.drawImage(image, 0, 0);
                ctx.font = "12pt Helvetica Neue";
                ctx.fillStyle = "black";
                ctx.fillText(zoom + " " + x + " " + y + " ", 10, 250);
                ctx.lineWidth = 3;
                ctx.strokeStyle = "black";
                ctx.strokeRect(0, 0, 256, 256);
                return c;
            } : function(image) { return image; };
            var obj = {
                transform_image: xform,
                crossOrigin: "anonymous",
                x_offset: tiles[id].offset_x * tile_size,
                y_offset: tiles[id].offset_y * tile_size,
                onload: f(x, y, zoom, id)
            };
            if (opts.no_network) {
                if (!Lux._globals.blank_globe_2d_image) {
                    var c = document.createElement("canvas");
                    c.setAttribute("width", 256);
                    c.setAttribute("height", 256);
                    var ctx = c.getContext('2d');
                    ctx.fillStyle = "white";
                    ctx.fillRect(0, 0, 256, 256);
                    Lux._globals.blank_globe_2d_image = c;
                }
                obj.canvas = Lux._globals.blank_globe_2d_image;
            } else {
                obj.src = opts.tile_pattern(zoom, x, y);
            }
            tiles[id].texture.load(obj);
        },
        dress: function(scene) {
            var tile_batch = tile_actor.dress(scene);
            var xf = scene.get_transform().inverse;
            if (!has_interactor) {
                get_center_zoom = function() {
                    var p1 = unproject(vec.make([0, 0]));
                    var p2 = unproject(vec.make([1, 1]));
                    var zoom = 1.0/(p2[1] - p1[1]);
                    return [p1[0], p1[1], zoom];
                };
                unproject = Shade(function(p) {
                    return xf({position: p}).position;
                }).js_evaluate;
            }
            var that = this;
            return {
                draw: function() {
                    that.new_center();
                    var lst = _.range(cache_size);
                    lst.sort(function(id1, id2) { 
                        var g1 = Math.abs(tiles[id1].zoom - that.current_osm_zoom);
                        var g2 = Math.abs(tiles[id2].zoom - that.current_osm_zoom);
                        return g2 - g1;
                    });

                    sampler.set(texture);
                    for (var i=0; i<cache_size; ++i) {
                        var t = tiles[lst[i]];
                        if (t.active !== 2)
                            continue;
                        var z = (1 << t.zoom);
                        min_x.set(t.x / z);
                        min_y.set(1 - (t.y + 1) / z);
                        max_x.set((t.x + 1) / z);
                        max_y.set(1 - t.y / z);
                        offset_x.set(t.offset_x);
                        offset_y.set(t.offset_y);
                        tile_batch.draw();
                    }
                }
            };
        },
        on: function() { return true; }
    };
    result.init();

    return result;
};

Lux.Marks.globe_2d.lat_lon_to_tile_mercator = Shade(function(lat, lon) {
    return Shade.Scale.Geo.latlong_to_mercator(lat, lon).div(Math.PI * 2).add(Shade.vec(0.5,0.5));
});

Lux.Marks.globe_2d.transform = function(appearance) {
    var new_appearance = _.clone(appearance);
    new_appearance.position = Shade.vec(Lux.Marks.globe_2d.lat_lon_to_tile_mercator(
        appearance.position.x(),
        appearance.position.y()), appearance.position.swizzle("xw"));
    return new_appearance;
};

Lux.Marks.globe_2d.transform.inverse = function() { throw new Error("unimplemented"); };
Lux.Models = {};
Lux.Models.flat_cube = function() {
    return Lux.model({
        type: "triangles",
        elements: [0,  1,  2,  0,  2,  3,
                   4,  5,  6,  4,  6,  7,
                   8,  9,  10, 8,  10, 11,
                   12, 13, 14, 12, 14, 15,
                   16, 17, 18, 16, 18, 19,
                   20, 21, 22, 20, 22, 23],
        vertex: [[ 1, 1,-1, -1, 1,-1, -1, 1, 1,  1, 1, 1,
                   1,-1, 1, -1,-1, 1, -1,-1,-1,  1,-1,-1,
                   1, 1, 1, -1, 1, 1, -1,-1, 1,  1,-1, 1,
                   1,-1,-1, -1,-1,-1, -1, 1,-1,  1, 1,-1,
                   -1, 1, 1, -1, 1,-1, -1,-1,-1, -1,-1, 1,
                   1, 1,-1,  1, 1, 1,  1,-1, 1,  1,-1,-1], 3],
        normal: [[ 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0,
                   0,-1, 0, 0,-1, 0, 0,-1, 0, 0,-1, 0,
                   0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1,
                   0, 0,-1, 0, 0,-1, 0, 0,-1, 0, 0,-1,
                   -1, 0, 0,-1, 0, 0,-1, 0, 0,-1, 0, 0,
                   1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0], 3],
        tex_coord: [[0,0, 1,0, 1,1, 0,1,
                     0,0, 1,0, 1,1, 0,1,
                     0,0, 1,0, 1,1, 0,1,
                     0,0, 1,0, 1,1, 0,1,
                     0,0, 1,0, 1,1, 0,1,
                     0,0, 1,0, 1,1, 0,1], 2]
    });
};
Lux.Models.mesh = function(u_secs, v_secs) {
    var verts = [];
    var elements = [];
    if (_.isUndefined(v_secs)) v_secs = u_secs;
    if (v_secs <= 0) throw new Error("v_secs must be positive");
    if (u_secs <= 0) throw new Error("u_secs must be positive");
    v_secs = Math.floor(v_secs);
    u_secs = Math.floor(u_secs);
    var i, j;    
    for (i=0; i<=v_secs; ++i) {
        var v = (i / v_secs);
        for (j=0; j<=u_secs; ++j) {
            var u = (j / u_secs);
            verts.push(u, v);
        }
    }
    for (i=0; i<v_secs; ++i) {
        for (j=0; j<=u_secs; ++j) {
            elements.push(i * (u_secs + 1) + j,
                          (i + 1) * (u_secs + 1) + j);
        }
        // set up a non-rasterizing triangle in the middle of the strip
        // to transition between strips.
        if (i < v_secs-1) {
            elements.push((i + 1) * (u_secs + 1) + u_secs,
                          (i + 2) * (u_secs + 1),
                          (i + 2) * (u_secs + 1)
                         );
        }
    }

    var uv_attr = Shade(Lux.attribute_buffer({
        vertex_array: verts, 
        item_size: 2
    }));
    return Lux.model({
        type: "triangle_strip",
        tex_coord: uv_attr,
        vertex: uv_attr.mul(2).sub(1),
        elements: Lux.element_buffer(elements)
    });
};
Lux.Models.sphere = function(lat_secs, long_secs) {
    if (_.isUndefined(lat_secs)) {
        lat_secs = 5;
        long_secs = 5;
    }
    var verts = [];
    var elements = [];
    if (_.isUndefined(long_secs)) long_secs = lat_secs;
    if (lat_secs <= 0) throw new Error("lat_secs must be positive");
    if (long_secs <= 0) throw new Error("long_secs must be positive");
    lat_secs = Math.floor(lat_secs);
    long_secs = Math.floor(long_secs);
    var i, j, phi, theta;    
    for (i=0; i<=lat_secs; ++i) {
        phi = (i / lat_secs);
        for (j=0; j<long_secs; ++j) {
            theta = (j / long_secs);
            verts.push(theta, phi);
        }
    }
    for (i=0; i<lat_secs; ++i) {
        for (j=0; j<long_secs; ++j) {
            elements.push(i * long_secs + j,
                          i * long_secs + ((j + 1) % long_secs),
                          (i + 1) * long_secs + j,
                          i * long_secs + ((j + 1) % long_secs),
                          (i + 1) * long_secs + ((j + 1) % long_secs),
                          (i + 1) * long_secs + j);
        }
    }

    var S = Shade;
    var uv_attr = Lux.attribute_buffer({ vertex_array: verts, item_size: 2});
    phi = S.sub(S.mul(Math.PI, S.swizzle(uv_attr, "r")), Math.PI/2);
    theta = S.mul(2 * Math.PI, S.swizzle(uv_attr, "g"));
    var cosphi = S.cos(phi);
    var position = S.vec(S.sin(theta).mul(cosphi),
                         S.sin(phi),
                         S.cos(theta).mul(cosphi), 1);
    return Lux.model({
        type: "triangles",
        elements: Lux.element_buffer(elements),
        vertex: position,
        tex_coord: uv_attr,
        normal: position
    });
};
Lux.Models.square = function() {
    var uv = Shade(Lux.attribute_buffer({
        vertex_array: [0, 0, 1, 0, 0, 1, 1, 1], 
        item_size: 2
    }));
    return Lux.model({
        type: "triangles",
        elements: Lux.element_buffer([0, 1, 2, 1, 3, 2]),
        vertex: uv,
        tex_coord: uv
    });
};
Lux.Models.teapot = function()
{
    // Teapot data from Daniel Wagner (daniel@ims.tuwien.ac.at), via freeglut
    var teapot_coords = [
        2.1, 3.6, 0.0, 
        2.071, 3.711, 0.0, 
        2.105, 3.748, 0.0, 
        2.174, 3.711, 0.0, 
        2.25, 3.6, 0.0, 
        1.937, 3.6, 0.8242, 
        1.91, 3.711, 0.8128, 
        1.942, 3.748, 0.8261, 
        2.005, 3.711, 0.8532, 
        2.076, 3.6, 0.8831, 
        1.491, 3.6, 1.491, 
        1.47, 3.711, 1.47, 
        1.494, 3.748, 1.494, 
        1.543, 3.711, 1.543, 
        1.597, 3.6, 1.597, 
        0.8242, 3.6, 1.937, 
        0.8128, 3.711, 1.91, 
        0.8261, 3.748, 1.942, 
        0.8532, 3.711, 2.005, 
        0.8831, 3.6, 2.076, 
        0.0, 3.6, 2.1, 
        0.0, 3.711, 2.071, 
        0.0, 3.748, 2.105, 
        0.0, 3.711, 2.174, 
        0.0, 3.6, 2.25, 
            -0.8812, 3.6, 1.937,
            -0.8368, 3.711, 1.91, 
            -0.8332, 3.748, 1.942, 
            -0.8541, 3.711, 2.005, 
            -0.8831, 3.6, 2.076, 
            -1.542, 3.6, 1.491, 
            -1.492, 3.711, 1.47, 
            -1.501, 3.748, 1.494, 
            -1.544, 3.711, 1.543, 
            -1.597, 3.6, 1.597, 
            -1.956, 3.6, 0.8242, 
            -1.918, 3.711, 0.8128, 
            -1.944, 3.748, 0.8261, 
            -2.006, 3.711, 0.8532, 
            -2.076, 3.6, 0.8831, 
            -2.1, 3.6, 0.0, 
            -2.071, 3.711, 0.0, 
            -2.105, 3.748, 0.0, 
            -2.174, 3.711, 0.0, 
            -2.25, 3.6, 0.0, 
            -1.937, 3.6, -0.8242, 
            -1.91, 3.711, -0.8128, 
            -1.942, 3.748, -0.8261, 
            -2.005, 3.711, -0.8532, 
            -2.076, 3.6, -0.8831, 
            -1.491, 3.6, -1.491, 
            -1.47, 3.711, -1.47, 
            -1.494, 3.748, -1.494, 
            -1.543, 3.711, -1.543, 
            -1.597, 3.6, -1.597, 
            -0.8242, 3.6, -1.937, 
            -0.8128, 3.711, -1.91, 
            -0.8261, 3.748, -1.942, 
            -0.8532, 3.711, -2.005, 
            -0.8831, 3.6, -2.076, 
        0.0, 3.6, -2.1, 
        0.0, 3.711, -2.071, 
        0.0, 3.748, -2.105, 
        0.0, 3.711, -2.174, 
        0.0, 3.6, -2.25, 
        0.8242, 3.6, -1.937, 
        0.8128, 3.711, -1.91, 
        0.8261, 3.748, -1.942, 
        0.8532, 3.711, -2.005, 
        0.8831, 3.6, -2.076, 
        1.491, 3.6, -1.491, 
        1.47, 3.711, -1.47, 
        1.494, 3.748, -1.494, 
        1.543, 3.711, -1.543, 
        1.597, 3.6, -1.597, 
        1.937, 3.6, -0.8242, 
        1.91, 3.711, -0.8128, 
        1.942, 3.748, -0.8261, 
        2.005, 3.711, -0.8532, 
        2.076, 3.6, -0.8831, 
        2.525, 3.011, 0.0, 
        2.766, 2.433, 0.0, 
        2.936, 1.876, 0.0, 
        3.0, 1.35, 0.0, 
        2.33, 3.011, 0.9912, 
        2.551, 2.433, 1.086, 
        2.708, 1.876, 1.152, 
        2.767, 1.35, 1.178, 
        1.793, 3.011, 1.793, 
        1.964, 2.433, 1.964, 
        2.084, 1.876, 2.084, 
        2.13, 1.35, 2.13, 
        0.9912, 3.011, 2.33, 
        1.086, 2.433, 2.551, 
        1.152, 1.876, 2.708, 
        1.178, 1.35, 2.767, 
        0.0, 3.011, 2.525, 
        0.0, 2.433, 2.766, 
        0.0, 1.876, 2.936, 
        0.0, 1.35, 3.0, 
            -0.9912, 3.011, 2.33, 
            -1.086, 2.433, 2.551, 
            -1.152, 1.876, 2.708, 
            -1.178, 1.35, 2.767, 
            -1.793, 3.011, 1.793, 
            -1.964, 2.433, 1.964, 
            -2.084, 1.876, 2.084, 
            -2.13, 1.35, 2.13, 
            -2.33, 3.011, 0.9912, 
            -2.551, 2.433, 1.086, 
            -2.708, 1.876, 1.152, 
            -2.767, 1.35, 1.178, 
            -2.525, 3.011, 0.0, 
            -2.766, 2.433, 0.0, 
            -2.936, 1.876, 0.0, 
            -3.0, 1.35, 0.0, 
            -2.33, 3.011, -0.9912, 
            -2.551, 2.433, -1.086, 
            -2.708, 1.876, -1.152, 
            -2.767, 1.35, -1.178, 
            -1.793, 3.011, -1.793, 
            -1.964, 2.433, -1.964, 
            -2.084, 1.876, -2.084, 
            -2.13, 1.35, -2.13, 
            -0.9912, 3.011, -2.33, 
            -1.086, 2.433, -2.551, 
            -1.152, 1.876, -2.708, 
            -1.178, 1.35, -2.767, 
        0.0, 3.011, -2.525, 
        0.0, 2.433, -2.766, 
        0.0, 1.876, -2.936, 
        0.0, 1.35, -3.0, 
        0.9912, 3.011, -2.33, 
        1.086, 2.433, -2.551, 
        1.152, 1.876, -2.708, 
        1.178, 1.35, -2.767, 
        1.793, 3.011, -1.793, 
        1.964, 2.433, -1.964, 
        2.084, 1.876, -2.084, 
        2.13, 1.35, -2.13, 
        2.33, 3.011, -0.9912, 
        2.551, 2.433, -1.086, 
        2.708, 1.876, -1.152, 
        2.767, 1.35, -1.178, 
        2.883, 0.9053, 0.0, 
        2.625, 0.5766, 0.0, 
        2.367, 0.3533, 0.0, 
        2.25, 0.225, 0.0, 
        2.659, 0.9053, 1.132, 
        2.422, 0.5766, 1.03, 
        2.184, 0.3533, 0.9291, 
        2.076, 0.225, 0.8831, 
        2.047, 0.9053, 2.047, 
        1.864, 0.5766, 1.864, 
        1.681, 0.3533, 1.681, 
        1.597, 0.225, 1.597, 
        1.132, 0.9053, 2.659, 
        1.03, 0.5766, 2.422, 
        0.9291, 0.3533, 2.184, 
        0.8831, 0.225, 2.076, 
        0.0, 0.9053, 2.883, 
        0.0, 0.5766, 2.625, 
        0.0, 0.3533, 2.367, 
        0.0, 0.225, 2.25, 
            -1.132, 0.9053, 2.659, 
            -1.03, 0.5766, 2.422, 
            -0.9291, 0.3533, 2.184, 
            -0.8831, 0.225, 2.076, 
            -2.047, 0.9053, 2.047, 
            -1.864, 0.5766, 1.864, 
            -1.681, 0.3533, 1.681, 
            -1.597, 0.225, 1.597, 
            -2.659, 0.9053, 1.132, 
            -2.422, 0.5766, 1.03, 
            -2.184, 0.3533, 0.9291, 
            -2.076, 0.225, 0.8831, 
            -2.883, 0.9053, 0.0, 
            -2.625, 0.5766, 0.0, 
            -2.367, 0.3533, 0.0, 
            -2.25, 0.225, 0.0, 
            -2.659, 0.9053, -1.132, 
            -2.422, 0.5766, -1.03, 
            -2.184, 0.3533, -0.9291, 
            -2.076, 0.225, -0.8831, 
            -2.047, 0.9053, -2.047, 
            -1.864, 0.5766, -1.864, 
            -1.681, 0.3533, -1.681, 
            -1.597, 0.225, -1.597, 
            -1.132, 0.9053, -2.659, 
            -1.03, 0.5766, -2.422, 
            -0.9291, 0.3533, -2.184, 
            -0.8831, 0.225, -2.076, 
        0.0, 0.9053, -2.883, 
        0.0, 0.5766, -2.625, 
        0.0, 0.3533, -2.367, 
        0.0, 0.225, -2.25, 
        1.132, 0.9053, -2.659, 
        1.03, 0.5766, -2.422, 
        0.9291, 0.3533, -2.184, 
        0.8831, 0.225, -2.076, 
        2.047, 0.9053, -2.047, 
        1.864, 0.5766, -1.864, 
        1.681, 0.3533, -1.681, 
        1.597, 0.225, -1.597, 
        2.659, 0.9053, -1.132, 
        2.422, 0.5766, -1.03, 
        2.184, 0.3533, -0.9291, 
        2.076, 0.225, -0.8831, 
        2.199, 0.1424, 0.0, 
        1.927, 0.07031, 0.0, 
        1.253, 0.01934, 0.0, 
        0.0, 0.0, 0.0, 
        2.029, 0.1424, 0.8631, 
        1.777, 0.07031, 0.7562, 
        1.156, 0.01934, 0.4919, 
        1.561, 0.1424, 1.561, 
        1.368, 0.07031, 1.368, 
        0.8899, 0.01934, 0.8899, 
        0.8631, 0.1424, 2.029, 
        0.7562, 0.07031, 1.777, 
        0.4919, 0.01934, 1.156, 
        0.0, 0.1424, 2.199, 
        0.0, 0.07031, 1.927, 
        0.0, 0.01934, 1.253, 
            -0.8631, 0.1424, 2.029, 
            -0.7562, 0.07031, 1.777, 
            -0.4919, 0.01934, 1.156, 
            -1.561, 0.1424, 1.561, 
            -1.368, 0.07031, 1.368, 
            -0.8899, 0.01934, 0.8899, 
            -2.029, 0.1424, 0.8631, 
            -1.777, 0.07031, 0.7562, 
            -1.156, 0.01934, 0.4919, 
            -2.199, 0.1424, 0.0, 
            -1.927, 0.07031, 0.0, 
            -1.253, 0.01934, 0.0, 
            -2.029, 0.1424, -0.8631, 
            -1.777, 0.07031, -0.7562, 
            -1.156, 0.01934, -0.4919, 
            -1.561, 0.1424, -1.561, 
            -1.368, 0.07031, -1.368, 
            -0.8899, 0.01934, -0.8899, 
            -0.8631, 0.1424, -2.029, 
            -0.7562, 0.07031, -1.777, 
            -0.4919, 0.01934, -1.156, 
        0.0, 0.1424, -2.199, 
        0.0, 0.07031, -1.927, 
        0.0, 0.01934, -1.253, 
        0.8631, 0.1424, -2.029, 
        0.7562, 0.07031, -1.777, 
        0.4919, 0.01934, -1.156, 
        1.561, 0.1424, -1.561, 
        1.368, 0.07031, -1.368, 
        0.8899, 0.01934, -0.8899, 
        2.029, 0.1424, -0.8631, 
        1.777, 0.07031, -0.7562, 
        1.156, 0.01934, -0.4919, 
            -2.4, 3.038, 0.0, 
            -3.101, 3.032, 0.0, 
            -3.619, 2.995, 0.0, 
            -3.94, 2.895, 0.0, 
            -4.05, 2.7, 0.0, 
            -2.377, 3.09, 0.2531, 
            -3.122, 3.084, 0.2531, 
            -3.669, 3.041, 0.2531, 
            -4.005, 2.926, 0.2531, 
            -4.12, 2.7, 0.2531, 
            -2.325, 3.206, 0.3375, 
            -3.168, 3.198, 0.3375, 
            -3.778, 3.143, 0.3375, 
            -4.15, 2.993, 0.3375, 
            -4.275, 2.7, 0.3375, 
            -2.273, 3.322, 0.2531, 
            -3.214, 3.313, 0.2531, 
            -3.888, 3.244, 0.2531, 
            -4.294, 3.06, 0.2531, 
            -4.43, 2.7, 0.2531, 
            -2.25, 3.375, 0.0, 
            -3.234, 3.364, 0.0, 
            -3.938, 3.291, 0.0, 
            -4.359, 3.09, 0.0, 
            -4.5, 2.7, 0.0, 
            -2.273, 3.322, -0.2531, 
            -3.214, 3.313, -0.2531, 
            -3.888, 3.244, -0.2531, 
            -4.294, 3.06, -0.2531, 
            -4.43, 2.7, -0.2531, 
            -2.325, 3.206, -0.3375, 
            -3.168, 3.198, -0.3375, 
            -3.778, 3.143, -0.3375, 
            -4.15, 2.993, -0.3375, 
            -4.275, 2.7, -0.3375, 
            -2.377, 3.09, -0.2531, 
            -3.122, 3.084, -0.2531, 
            -3.669, 3.041, -0.2531, 
            -4.005, 2.926, -0.2531, 
            -4.12, 2.7, -0.2531, 
            -3.991, 2.394, 0.0, 
            -3.806, 2.025, 0.0, 
            -3.48, 1.656, 0.0, 
            -3.0, 1.35, 0.0, 
            -4.055, 2.365, 0.2531, 
            -3.852, 1.98, 0.2531, 
            -3.496, 1.6, 0.2531, 
            -2.977, 1.28, 0.2531, 
            -4.196, 2.3, 0.3375, 
            -3.952, 1.881, 0.3375, 
            -3.531, 1.478, 0.3375, 
            -2.925, 1.125, 0.3375, 
            -4.336, 2.235, 0.2531, 
            -4.051, 1.782, 0.2531, 
            -3.566, 1.356, 0.2531, 
            -2.873, 0.9703, 0.2531, 
            -4.4, 2.205, 0.0, 
            -4.097, 1.737, 0.0, 
            -3.582, 1.3, 0.0, 
            -2.85, 0.9, 0.0, 
            -4.336, 2.235, -0.2531, 
            -4.051, 1.782, -0.2531, 
            -3.566, 1.356, -0.2531, 
            -2.873, 0.9703, -0.2531, 
            -4.196, 2.3, -0.3375, 
            -3.952, 1.881, -0.3375, 
            -3.531, 1.478, -0.3375, 
            -2.925, 1.125, -0.3375, 
            -4.055, 2.365, -0.2531, 
            -3.852, 1.98, -0.2531, 
            -3.496, 1.6, -0.2531, 
            -2.977, 1.28, -0.2531, 
        2.55, 2.137, 0.0, 
        3.27, 2.303, 0.0, 
        3.581, 2.7, 0.0, 
        3.752, 3.182, 0.0, 
        4.05, 3.6, 0.0, 
        2.55, 1.944, 0.5569, 
        3.324, 2.159, 0.5028, 
        3.652, 2.617, 0.3839, 
        3.838, 3.151, 0.265, 
        4.191, 3.6, 0.2109, 
        2.55, 1.519, 0.7425, 
        3.445, 1.844, 0.6704, 
        3.806, 2.433, 0.5119, 
        4.027, 3.085, 0.3533, 
        4.5, 3.6, 0.2813, 
        2.55, 1.093, 0.5569, 
        3.566, 1.529, 0.5028, 
        3.961, 2.249, 0.3839, 
        4.215, 3.018, 0.265, 
        4.809, 3.6, 0.2109, 
        2.55, 0.9, 0.0, 
        3.621, 1.385, 0.0, 
        4.031, 2.166, 0.0, 
        4.301, 2.988, 0.0, 
        4.95, 3.6, 0.0, 
        2.55, 1.093, -0.5569, 
        3.566, 1.529, -0.5028, 
        3.961, 2.249, -0.3839, 
        4.215, 3.018, -0.265, 
        4.809, 3.6, -0.2109, 
        2.55, 1.519, -0.7425, 
        3.445, 1.844, -0.6704, 
        3.806, 2.433, -0.5119, 
        4.027, 3.085, -0.3533, 
        4.5, 3.6, -0.2813, 
        2.55, 1.944, -0.5569, 
        3.324, 2.159, -0.5028, 
        3.652, 2.617, -0.3839, 
        3.838, 3.151, -0.265, 
        4.191, 3.6, -0.2109, 
        4.158, 3.663, 0.0, 
        4.238, 3.684, 0.0, 
        4.261, 3.663, 0.0, 
        4.2, 3.6, 0.0, 
        4.308, 3.666, 0.1978, 
        4.379, 3.689, 0.1687, 
        4.381, 3.668, 0.1397, 
        4.294, 3.6, 0.1266, 
        4.64, 3.673, 0.2637, 
        4.69, 3.7, 0.225, 
        4.645, 3.677, 0.1863, 
        4.5, 3.6, 0.1688, 
        4.971, 3.68, 0.1978, 
        5.001, 3.711, 0.1687, 
        4.909, 3.687, 0.1397, 
        4.706, 3.6, 0.1266, 
        5.122, 3.683, 0.0, 
        5.142, 3.716, 0.0, 
        5.029, 3.691, 0.0, 
        4.8, 3.6, 0.0, 
        4.971, 3.68, -0.1978, 
        5.001, 3.711, -0.1687, 
        4.909, 3.687, -0.1397, 
        4.706, 3.6, -0.1266, 
        4.64, 3.673, -0.2637, 
        4.69, 3.7, -0.225, 
        4.645, 3.677, -0.1863, 
        4.5, 3.6, -0.1688, 
        4.308, 3.666, -0.1978, 
        4.379, 3.689, -0.1687, 
        4.381, 3.668, -0.1397, 
        4.294, 3.6, -0.1266, 
        0.0, 4.725, 0.0, 
        0.5109, 4.651, 0.0, 
        0.4875, 4.472, 0.0, 
        0.2953, 4.25, 0.0, 
        0.3, 4.05, 0.0, 
        0.4715, 4.651, 0.2011, 
        0.4499, 4.472, 0.1918, 
        0.2725, 4.25, 0.1161, 
        0.2768, 4.05, 0.1178, 
        0.3632, 4.651, 0.3632, 
        0.3465, 4.472, 0.3465, 
        0.2098, 4.25, 0.2098, 
        0.213, 4.05, 0.213, 
        0.2011, 4.651, 0.4715, 
        0.1918, 4.472, 0.4499, 
        0.1161, 4.25, 0.2725, 
        0.1178, 4.05, 0.2768, 
        0.0, 4.651, 0.5109, 
        0.0, 4.472, 0.4875, 
        0.0, 4.25, 0.2953, 
        0.0, 4.05, 0.3, 
            -0.2011, 4.651, 0.4715, 
            -0.1918, 4.472, 0.4499, 
            -0.1161, 4.25, 0.2725, 
            -0.1178, 4.05, 0.2768, 
            -0.3632, 4.651, 0.3632, 
            -0.3465, 4.472, 0.3465, 
            -0.2098, 4.25, 0.2098, 
            -0.213, 4.05, 0.213, 
            -0.4715, 4.651, 0.2011, 
            -0.4499, 4.472, 0.1918, 
            -0.2725, 4.25, 0.1161, 
            -0.2768, 4.05, 0.1178, 
            -0.5109, 4.651, 0.0, 
            -0.4875, 4.472, 0.0, 
            -0.2953, 4.25, 0.0, 
            -0.3, 4.05, 0.0, 
            -0.4715, 4.651, -0.2011, 
            -0.4499, 4.472, -0.1918, 
            -0.2725, 4.25, -0.1161, 
            -0.2768, 4.05, -0.1178, 
            -0.3632, 4.651, -0.3632, 
            -0.3465, 4.472, -0.3465, 
            -0.2098, 4.25, -0.2098, 
            -0.213, 4.05, -0.213, 
            -0.2011, 4.651, -0.4715, 
            -0.1918, 4.472, -0.4499, 
            -0.1161, 4.25, -0.2725, 
            -0.1178, 4.05, -0.2768, 
        0.0, 4.651, -0.5109, 
        0.0, 4.472, -0.4875, 
        0.0, 4.25, -0.2953, 
        0.0, 4.05, -0.3, 
        0.2011, 4.651, -0.4715, 
        0.1918, 4.472, -0.4499, 
        0.1161, 4.25, -0.2725, 
        0.1178, 4.05, -0.2768, 
        0.3632, 4.651, -0.3632, 
        0.3465, 4.472, -0.3465, 
        0.2098, 4.25, -0.2098, 
        0.213, 4.05, -0.213, 
        0.4715, 4.651, -0.2011, 
        0.4499, 4.472, -0.1918, 
        0.2725, 4.25, -0.1161, 
        0.2768, 4.05, -0.1178, 
        0.6844, 3.916, 0.0, 
        1.237, 3.825, 0.0, 
        1.734, 3.734, 0.0, 
        1.95, 3.6, 0.0, 
        0.6313, 3.916, 0.2686, 
        1.142, 3.825, 0.4857, 
        1.6, 3.734, 0.6807, 
        1.799, 3.6, 0.7654, 
        0.4859, 3.916, 0.4859, 
        0.8786, 3.825, 0.8786, 
        1.231, 3.734, 1.231, 
        1.385, 3.6, 1.385, 
        0.2686, 3.916, 0.6313, 
        0.4857, 3.825, 1.142, 
        0.6807, 3.734, 1.6, 
        0.7654, 3.6, 1.799, 
        0.0, 3.916, 0.6844, 
        0.0, 3.825, 1.237, 
        0.0, 3.734, 1.734, 
        0.0, 3.6, 1.95, 
            -0.2686, 3.916, 0.6313, 
            -0.4857, 3.825, 1.142, 
            -0.6807, 3.734, 1.6, 
            -0.7654, 3.6, 1.799, 
            -0.4859, 3.916, 0.4859, 
            -0.8786, 3.825, 0.8786, 
            -1.231, 3.734, 1.231, 
            -1.385, 3.6, 1.385, 
            -0.6313, 3.916, 0.2686, 
            -1.142, 3.825, 0.4857, 
            -1.6, 3.734, 0.6807, 
            -1.799, 3.6, 0.7654, 
            -0.6844, 3.916, 0.0, 
            -1.237, 3.825, 0.0, 
            -1.734, 3.734, 0.0, 
            -1.95, 3.6, 0.0, 
            -0.6313, 3.916, -0.2686, 
            -1.142, 3.825, -0.4857, 
            -1.6, 3.734, -0.6807, 
            -1.799, 3.6, -0.7654, 
            -0.4859, 3.916, -0.4859, 
            -0.8786, 3.825, -0.8786, 
            -1.231, 3.734, -1.231, 
            -1.385, 3.6, -1.385, 
            -0.2686, 3.916, -0.6313, 
            -0.4857, 3.825, -1.142, 
            -0.6807, 3.734, -1.6, 
            -0.7654, 3.6, -1.799, 
        0.0, 3.916, -0.6844, 
        0.0, 3.825, -1.237, 
        0.0, 3.734, -1.734, 
        0.0, 3.6, -1.95, 
        0.2686, 3.916, -0.6313, 
        0.4857, 3.825, -1.142, 
        0.6807, 3.734, -1.6, 
        0.7654, 3.6, -1.799, 
        0.4859, 3.916, -0.4859, 
        0.8786, 3.825, -0.8786, 
        1.231, 3.734, -1.231, 
        1.385, 3.6, -1.385, 
        0.6313, 3.916, -0.2686, 
        1.142, 3.825, -0.4857, 
        1.6, 3.734, -0.6807, 
        1.799, 3.6, -0.7654
    ];

    var teapot_elements = [
        0, 5, 6, 
        6, 1, 0,
        1, 6, 7,
        7, 2, 1,
        2, 7, 8,
        8, 3, 2,
        3, 8, 9,
        9, 4, 3,
        5, 10, 11,
        11, 6, 5,
        6, 11, 12,
        12, 7, 6,
        7, 12, 13,
        13, 8, 7,
        8, 13, 14,
        14, 9, 8,
        10, 15, 16,
        16, 11, 10,
        11, 16, 17,
        17, 12, 11,
        12, 17, 18,
        18, 13, 12,
        13, 18, 19,
        19, 14, 13,
        15, 20, 21,
        21, 16, 15,
        16, 21, 22,
        22, 17, 16,
        17, 22, 23,
        23, 18, 17,
        18, 23, 24,
        24, 19, 18,
        20, 25, 26,
        26, 21, 20,
        21, 26, 27,
        27, 22, 21,
        22, 27, 28,
        28, 23, 22,
        23, 28, 29,
        29, 24, 23,
        25, 30, 31,
        31, 26, 25,
        26, 31, 32,
        32, 27, 26,
        27, 32, 33,
        33, 28, 27,
        28, 33, 34,
        34, 29, 28,
        30, 35, 36,
        36, 31, 30,
        31, 36, 37,
        37, 32, 31,
        32, 37, 38,
        38, 33, 32,
        33, 38, 39,
        39, 34, 33,
        35, 40, 41,
        41, 36, 35,
        36, 41, 42,
        42, 37, 36,
        37, 42, 43,
        43, 38, 37,
        38, 43, 44,
        44, 39, 38,
        40, 45, 46,
        46, 41, 40,
        41, 46, 47,
        47, 42, 41,
        42, 47, 48,
        48, 43, 42,
        43, 48, 49,
        49, 44, 43,
        45, 50, 51,
        51, 46, 45,
        46, 51, 52,
        52, 47, 46,
        47, 52, 53,
        53, 48, 47,
        48, 53, 54,
        54, 49, 48,
        50, 55, 56,
        56, 51, 50,
        51, 56, 57,
        57, 52, 51,
        52, 57, 58,
        58, 53, 52,
        53, 58, 59,
        59, 54, 53,
        55, 60, 61,
        61, 56, 55,
        56, 61, 62,
        62, 57, 56,
        57, 62, 63,
        63, 58, 57,
        58, 63, 64,
        64, 59, 58,
        60, 65, 66,
        66, 61, 60,
        61, 66, 67,
        67, 62, 61,
        62, 67, 68,
        68, 63, 62,
        63, 68, 69,
        69, 64, 63,
        65, 70, 71,
        71, 66, 65,
        66, 71, 72,
        72, 67, 66,
        67, 72, 73,
        73, 68, 67,
        68, 73, 74,
        74, 69, 68,
        70, 75, 76,
        76, 71, 70,
        71, 76, 77,
        77, 72, 71,
        72, 77, 78,
        78, 73, 72,
        73, 78, 79,
        79, 74, 73,
        75, 0, 1,
        1, 76, 75,
        76, 1, 2,
        2, 77, 76,
        77, 2, 3,
        3, 78, 77,
        78, 3, 4,
        4, 79, 78,
        4, 9, 84,
        84, 80, 4,
        80, 84, 85,
        85, 81, 80,
        81, 85, 86,
        86, 82, 81,
        82, 86, 87,
        87, 83, 82,
        9, 14, 88,
        88, 84, 9,
        84, 88, 89,
        89, 85, 84,
        85, 89, 90,
        90, 86, 85,
        86, 90, 91,
        91, 87, 86,
        14, 19, 92,
        92, 88, 14,
        88, 92, 93,
        93, 89, 88,
        89, 93, 94,
        94, 90, 89,
        90, 94, 95,
        95, 91, 90,
        19, 24, 96,
        96, 92, 19,
        92, 96, 97,
        97, 93, 92,
        93, 97, 98,
        98, 94, 93,
        94, 98, 99,
        99, 95, 94,
        24, 29, 100,
        100, 96, 24,
        96, 100, 101,
        101, 97, 96,
        97, 101, 102,
        102, 98, 97,
        98, 102, 103,
        103, 99, 98,
        29, 34, 104,
        104, 100, 29,
        100, 104, 105,
        105, 101, 100,
        101, 105, 106,
        106, 102, 101,
        102, 106, 107,
        107, 103, 102,
        34, 39, 108,
        108, 104, 34,
        104, 108, 109,
        109, 105, 104,
        105, 109, 110,
        110, 106, 105,
        106, 110, 111,
        111, 107, 106,
        39, 44, 112,
        112, 108, 39,
        108, 112, 113,
        113, 109, 108,
        109, 113, 114,
        114, 110, 109,
        110, 114, 115,
        115, 111, 110,
        44, 49, 116,
        116, 112, 44,
        112, 116, 117,
        117, 113, 112,
        113, 117, 118,
        118, 114, 113,
        114, 118, 119,
        119, 115, 114,
        49, 54, 120,
        120, 116, 49,
        116, 120, 121,
        121, 117, 116,
        117, 121, 122,
        122, 118, 117,
        118, 122, 123,
        123, 119, 118,
        54, 59, 124,
        124, 120, 54,
        120, 124, 125,
        125, 121, 120,
        121, 125, 126,
        126, 122, 121,
        122, 126, 127,
        127, 123, 122,
        59, 64, 128,
        128, 124, 59,
        124, 128, 129,
        129, 125, 124,
        125, 129, 130,
        130, 126, 125,
        126, 130, 131,
        131, 127, 126,
        64, 69, 132,
        132, 128, 64,
        128, 132, 133,
        133, 129, 128,
        129, 133, 134,
        134, 130, 129,
        130, 134, 135,
        135, 131, 130,
        69, 74, 136,
        136, 132, 69,
        132, 136, 137,
        137, 133, 132,
        133, 137, 138,
        138, 134, 133,
        134, 138, 139,
        139, 135, 134,
        74, 79, 140,
        140, 136, 74,
        136, 140, 141,
        141, 137, 136,
        137, 141, 142,
        142, 138, 137,
        138, 142, 143,
        143, 139, 138,
        79, 4, 80,
        80, 140, 79,
        140, 80, 81,
        81, 141, 140,
        141, 81, 82,
        82, 142, 141,
        142, 82, 83,
        83, 143, 142,
        83, 87, 148,
        148, 144, 83,
        144, 148, 149,
        149, 145, 144,
        145, 149, 150,
        150, 146, 145,
        146, 150, 151,
        151, 147, 146,
        87, 91, 152,
        152, 148, 87,
        148, 152, 153,
        153, 149, 148,
        149, 153, 154,
        154, 150, 149,
        150, 154, 155,
        155, 151, 150,
        91, 95, 156,
        156, 152, 91,
        152, 156, 157,
        157, 153, 152,
        153, 157, 158,
        158, 154, 153,
        154, 158, 159,
        159, 155, 154,
        95, 99, 160,
        160, 156, 95,
        156, 160, 161,
        161, 157, 156,
        157, 161, 162,
        162, 158, 157,
        158, 162, 163,
        163, 159, 158,
        99, 103, 164,
        164, 160, 99,
        160, 164, 165,
        165, 161, 160,
        161, 165, 166,
        166, 162, 161,
        162, 166, 167,
        167, 163, 162,
        103, 107, 168,
        168, 164, 103,
        164, 168, 169,
        169, 165, 164,
        165, 169, 170,
        170, 166, 165,
        166, 170, 171,
        171, 167, 166,
        107, 111, 172,
        172, 168, 107,
        168, 172, 173,
        173, 169, 168,
        169, 173, 174,
        174, 170, 169,
        170, 174, 175,
        175, 171, 170,
        111, 115, 176,
        176, 172, 111,
        172, 176, 177,
        177, 173, 172,
        173, 177, 178,
        178, 174, 173,
        174, 178, 179,
        179, 175, 174,
        115, 119, 180,
        180, 176, 115,
        176, 180, 181,
        181, 177, 176,
        177, 181, 182,
        182, 178, 177,
        178, 182, 183,
        183, 179, 178,
        119, 123, 184,
        184, 180, 119,
        180, 184, 185,
        185, 181, 180,
        181, 185, 186,
        186, 182, 181,
        182, 186, 187,
        187, 183, 182,
        123, 127, 188,
        188, 184, 123,
        184, 188, 189,
        189, 185, 184,
        185, 189, 190,
        190, 186, 185,
        186, 190, 191,
        191, 187, 186,
        127, 131, 192,
        192, 188, 127,
        188, 192, 193,
        193, 189, 188,
        189, 193, 194,
        194, 190, 189,
        190, 194, 195,
        195, 191, 190,
        131, 135, 196,
        196, 192, 131,
        192, 196, 197,
        197, 193, 192,
        193, 197, 198,
        198, 194, 193,
        194, 198, 199,
        199, 195, 194,
        135, 139, 200,
        200, 196, 135,
        196, 200, 201,
        201, 197, 196,
        197, 201, 202,
        202, 198, 197,
        198, 202, 203,
        203, 199, 198,
        139, 143, 204,
        204, 200, 139,
        200, 204, 205,
        205, 201, 200,
        201, 205, 206,
        206, 202, 201,
        202, 206, 207,
        207, 203, 202,
        143, 83, 144,
        144, 204, 143,
        204, 144, 145,
        145, 205, 204,
        205, 145, 146,
        146, 206, 205,
        206, 146, 147,
        147, 207, 206,
        147, 151, 212,
        212, 208, 147,
        208, 212, 213,
        213, 209, 208,
        209, 213, 214,
        214, 210, 209,
        210, 214, 211,
        211, 211, 210,
        151, 155, 215,
        215, 212, 151,
        212, 215, 216,
        216, 213, 212,
        213, 216, 217,
        217, 214, 213,
        214, 217, 211,
        211, 211, 214,
        155, 159, 218,
        218, 215, 155,
        215, 218, 219,
        219, 216, 215,
        216, 219, 220,
        220, 217, 216,
        217, 220, 211,
        211, 211, 217,
        159, 163, 221,
        221, 218, 159,
        218, 221, 222,
        222, 219, 218,
        219, 222, 223,
        223, 220, 219,
        220, 223, 211,
        211, 211, 220,
        163, 167, 224,
        224, 221, 163,
        221, 224, 225,
        225, 222, 221,
        222, 225, 226,
        226, 223, 222,
        223, 226, 211,
        211, 211, 223,
        167, 171, 227,
        227, 224, 167,
        224, 227, 228,
        228, 225, 224,
        225, 228, 229,
        229, 226, 225,
        226, 229, 211,
        211, 211, 226,
        171, 175, 230,
        230, 227, 171,
        227, 230, 231,
        231, 228, 227,
        228, 231, 232,
        232, 229, 228,
        229, 232, 211,
        211, 211, 229,
        175, 179, 233,
        233, 230, 175,
        230, 233, 234,
        234, 231, 230,
        231, 234, 235,
        235, 232, 231,
        232, 235, 211,
        211, 211, 232,
        179, 183, 236,
        236, 233, 179,
        233, 236, 237,
        237, 234, 233,
        234, 237, 238,
        238, 235, 234,
        235, 238, 211,
        211, 211, 235,
        183, 187, 239,
        239, 236, 183,
        236, 239, 240,
        240, 237, 236,
        237, 240, 241,
        241, 238, 237,
        238, 241, 211,
        211, 211, 238,
        187, 191, 242,
        242, 239, 187,
        239, 242, 243,
        243, 240, 239,
        240, 243, 244,
        244, 241, 240,
        241, 244, 211,
        211, 211, 241,
        191, 195, 245,
        245, 242, 191,
        242, 245, 246,
        246, 243, 242,
        243, 246, 247,
        247, 244, 243,
        244, 247, 211,
        211, 211, 244,
        195, 199, 248,
        248, 245, 195,
        245, 248, 249,
        249, 246, 245,
        246, 249, 250,
        250, 247, 246,
        247, 250, 211,
        211, 211, 247,
        199, 203, 251,
        251, 248, 199,
        248, 251, 252,
        252, 249, 248,
        249, 252, 253,
        253, 250, 249,
        250, 253, 211,
        211, 211, 250,
        203, 207, 254,
        254, 251, 203,
        251, 254, 255,
        255, 252, 251,
        252, 255, 256,
        256, 253, 252,
        253, 256, 211,
        211, 211, 253,
        207, 147, 208,
        208, 254, 207,
        254, 208, 209,
        209, 255, 254,
        255, 209, 210,
        210, 256, 255,
        256, 210, 211,
        211, 211, 256,
        257, 262, 263,
        263, 258, 257,
        258, 263, 264,
        264, 259, 258,
        259, 264, 265,
        265, 260, 259,
        260, 265, 266,
        266, 261, 260,
        262, 267, 268,
        268, 263, 262,
        263, 268, 269,
        269, 264, 263,
        264, 269, 270,
        270, 265, 264,
        265, 270, 271,
        271, 266, 265,
        267, 272, 273,
        273, 268, 267,
        268, 273, 274,
        274, 269, 268,
        269, 274, 275,
        275, 270, 269,
        270, 275, 276,
        276, 271, 270,
        272, 277, 278,
        278, 273, 272,
        273, 278, 279,
        279, 274, 273,
        274, 279, 280,
        280, 275, 274,
        275, 280, 281,
        281, 276, 275,
        277, 282, 283,
        283, 278, 277,
        278, 283, 284,
        284, 279, 278,
        279, 284, 285,
        285, 280, 279,
        280, 285, 286,
        286, 281, 280,
        282, 287, 288,
        288, 283, 282,
        283, 288, 289,
        289, 284, 283,
        284, 289, 290,
        290, 285, 284,
        285, 290, 291,
        291, 286, 285,
        287, 292, 293,
        293, 288, 287,
        288, 293, 294,
        294, 289, 288,
        289, 294, 295,
        295, 290, 289,
        290, 295, 296,
        296, 291, 290,
        292, 257, 258,
        258, 293, 292,
        293, 258, 259,
        259, 294, 293,
        294, 259, 260,
        260, 295, 294,
        295, 260, 261,
        261, 296, 295,
        261, 266, 301,
        301, 297, 261,
        297, 301, 302,
        302, 298, 297,
        298, 302, 303,
        303, 299, 298,
        299, 303, 304,
        304, 300, 299,
        266, 271, 305,
        305, 301, 266,
        301, 305, 306,
        306, 302, 301,
        302, 306, 307,
        307, 303, 302,
        303, 307, 308,
        308, 304, 303,
        271, 276, 309,
        309, 305, 271,
        305, 309, 310,
        310, 306, 305,
        306, 310, 311,
        311, 307, 306,
        307, 311, 312,
        312, 308, 307,
        276, 281, 313,
        313, 309, 276,
        309, 313, 314,
        314, 310, 309,
        310, 314, 315,
        315, 311, 310,
        311, 315, 316,
        316, 312, 311,
        281, 286, 317,
        317, 313, 281,
        313, 317, 318,
        318, 314, 313,
        314, 318, 319,
        319, 315, 314,
        315, 319, 320,
        320, 316, 315,
        286, 291, 321,
        321, 317, 286,
        317, 321, 322,
        322, 318, 317,
        318, 322, 323,
        323, 319, 318,
        319, 323, 324,
        324, 320, 319,
        291, 296, 325,
        325, 321, 291,
        321, 325, 326,
        326, 322, 321,
        322, 326, 327,
        327, 323, 322,
        323, 327, 328,
        328, 324, 323,
        296, 261, 297,
        297, 325, 296,
        325, 297, 298,
        298, 326, 325,
        326, 298, 299,
        299, 327, 326,
        327, 299, 300,
        300, 328, 327,
        329, 334, 335,
        335, 330, 329,
        330, 335, 336,
        336, 331, 330,
        331, 336, 337,
        337, 332, 331,
        332, 337, 338,
        338, 333, 332,
        334, 339, 340,
        340, 335, 334,
        335, 340, 341,
        341, 336, 335,
        336, 341, 342,
        342, 337, 336,
        337, 342, 343,
        343, 338, 337,
        339, 344, 345,
        345, 340, 339,
        340, 345, 346,
        346, 341, 340,
        341, 346, 347,
        347, 342, 341,
        342, 347, 348,
        348, 343, 342,
        344, 349, 350,
        350, 345, 344,
        345, 350, 351,
        351, 346, 345,
        346, 351, 352,
        352, 347, 346,
        347, 352, 353,
        353, 348, 347,
        349, 354, 355,
        355, 350, 349,
        350, 355, 356,
        356, 351, 350,
        351, 356, 357,
        357, 352, 351,
        352, 357, 358,
        358, 353, 352,
        354, 359, 360,
        360, 355, 354,
        355, 360, 361,
        361, 356, 355,
        356, 361, 362,
        362, 357, 356,
        357, 362, 363,
        363, 358, 357,
        359, 364, 365,
        365, 360, 359,
        360, 365, 366,
        366, 361, 360,
        361, 366, 367,
        367, 362, 361,
        362, 367, 368,
        368, 363, 362,
        364, 329, 330,
        330, 365, 364,
        365, 330, 331,
        331, 366, 365,
        366, 331, 332,
        332, 367, 366,
        367, 332, 333,
        333, 368, 367,
        333, 338, 373,
        373, 369, 333,
        369, 373, 374,
        374, 370, 369,
        370, 374, 375,
        375, 371, 370,
        371, 375, 376,
        376, 372, 371,
        338, 343, 377,
        377, 373, 338,
        373, 377, 378,
        378, 374, 373,
        374, 378, 379,
        379, 375, 374,
        375, 379, 380,
        380, 376, 375,
        343, 348, 381,
        381, 377, 343,
        377, 381, 382,
        382, 378, 377,
        378, 382, 383,
        383, 379, 378,
        379, 383, 384,
        384, 380, 379,
        348, 353, 385,
        385, 381, 348,
        381, 385, 386,
        386, 382, 381,
        382, 386, 387,
        387, 383, 382,
        383, 387, 388,
        388, 384, 383,
        353, 358, 389,
        389, 385, 353,
        385, 389, 390,
        390, 386, 385,
        386, 390, 391,
        391, 387, 386,
        387, 391, 392,
        392, 388, 387,
        358, 363, 393,
        393, 389, 358,
        389, 393, 394,
        394, 390, 389,
        390, 394, 395,
        395, 391, 390,
        391, 395, 396,
        396, 392, 391,
        363, 368, 397,
        397, 393, 363,
        393, 397, 398,
        398, 394, 393,
        394, 398, 399,
        399, 395, 394,
        395, 399, 400,
        400, 396, 395,
        368, 333, 369,
        369, 397, 368,
        397, 369, 370,
        370, 398, 397,
        398, 370, 371,
        371, 399, 398,
        399, 371, 372,
        372, 400, 399,
        401, 401, 406,
        406, 402, 401,
        402, 406, 407,
        407, 403, 402,
        403, 407, 408,
        408, 404, 403,
        404, 408, 409,
        409, 405, 404,
        401, 401, 410,
        410, 406, 401,
        406, 410, 411,
        411, 407, 406,
        407, 411, 412,
        412, 408, 407,
        408, 412, 413,
        413, 409, 408,
        401, 401, 414,
        414, 410, 401,
        410, 414, 415,
        415, 411, 410,
        411, 415, 416,
        416, 412, 411,
        412, 416, 417,
        417, 413, 412,
        401, 401, 418,
        418, 414, 401,
        414, 418, 419,
        419, 415, 414,
        415, 419, 420,
        420, 416, 415,
        416, 420, 421,
        421, 417, 416,
        401, 401, 422,
        422, 418, 401,
        418, 422, 423,
        423, 419, 418,
        419, 423, 424,
        424, 420, 419,
        420, 424, 425,
        425, 421, 420,
        401, 401, 426,
        426, 422, 401,
        422, 426, 427,
        427, 423, 422,
        423, 427, 428,
        428, 424, 423,
        424, 428, 429,
        429, 425, 424,
        401, 401, 430,
        430, 426, 401,
        426, 430, 431,
        431, 427, 426,
        427, 431, 432,
        432, 428, 427,
        428, 432, 433,
        433, 429, 428,
        401, 401, 434,
        434, 430, 401,
        430, 434, 435,
        435, 431, 430,
        431, 435, 436,
        436, 432, 431,
        432, 436, 437,
        437, 433, 432,
        401, 401, 438,
        438, 434, 401,
        434, 438, 439,
        439, 435, 434,
        435, 439, 440,
        440, 436, 435,
        436, 440, 441,
        441, 437, 436,
        401, 401, 442,
        442, 438, 401,
        438, 442, 443,
        443, 439, 438,
        439, 443, 444,
        444, 440, 439,
        440, 444, 445,
        445, 441, 440,
        401, 401, 446,
        446, 442, 401,
        442, 446, 447,
        447, 443, 442,
        443, 447, 448,
        448, 444, 443,
        444, 448, 449,
        449, 445, 444,
        401, 401, 450,
        450, 446, 401,
        446, 450, 451,
        451, 447, 446,
        447, 451, 452,
        452, 448, 447,
        448, 452, 453,
        453, 449, 448,
        401, 401, 454,
        454, 450, 401,
        450, 454, 455,
        455, 451, 450,
        451, 455, 456,
        456, 452, 451,
        452, 456, 457,
        457, 453, 452,
        401, 401, 458,
        458, 454, 401,
        454, 458, 459,
        459, 455, 454,
        455, 459, 460,
        460, 456, 455,
        456, 460, 461,
        461, 457, 456,
        401, 401, 462,
        462, 458, 401,
        458, 462, 463,
        463, 459, 458,
        459, 463, 464,
        464, 460, 459,
        460, 464, 465,
        465, 461, 460,
        401, 401, 402,
        402, 462, 401,
        462, 402, 403,
        403, 463, 462,
        463, 403, 404,
        404, 464, 463,
        464, 404, 405,
        405, 465, 464,
        405, 409, 470,
        470, 466, 405,
        466, 470, 471,
        471, 467, 466,
        467, 471, 472,
        472, 468, 467,
        468, 472, 473,
        473, 469, 468,
        409, 413, 474,
        474, 470, 409,
        470, 474, 475,
        475, 471, 470,
        471, 475, 476,
        476, 472, 471,
        472, 476, 477,
        477, 473, 472,
        413, 417, 478,
        478, 474, 413,
        474, 478, 479,
        479, 475, 474,
        475, 479, 480,
        480, 476, 475,
        476, 480, 481,
        481, 477, 476,
        417, 421, 482,
        482, 478, 417,
        478, 482, 483,
        483, 479, 478,
        479, 483, 484,
        484, 480, 479,
        480, 484, 485,
        485, 481, 480,
        421, 425, 486,
        486, 482, 421,
        482, 486, 487,
        487, 483, 482,
        483, 487, 488,
        488, 484, 483,
        484, 488, 489,
        489, 485, 484,
        425, 429, 490,
        490, 486, 425,
        486, 490, 491,
        491, 487, 486,
        487, 491, 492,
        492, 488, 487,
        488, 492, 493,
        493, 489, 488,
        429, 433, 494,
        494, 490, 429,
        490, 494, 495,
        495, 491, 490,
        491, 495, 496,
        496, 492, 491,
        492, 496, 497,
        497, 493, 492,
        433, 437, 498,
        498, 494, 433,
        494, 498, 499,
        499, 495, 494,
        495, 499, 500,
        500, 496, 495,
        496, 500, 501,
        501, 497, 496,
        437, 441, 502,
        502, 498, 437,
        498, 502, 503,
        503, 499, 498,
        499, 503, 504,
        504, 500, 499,
        500, 504, 505,
        505, 501, 500,
        441, 445, 506,
        506, 502, 441,
        502, 506, 507,
        507, 503, 502,
        503, 507, 508,
        508, 504, 503,
        504, 508, 509,
        509, 505, 504,
        445, 449, 510,
        510, 506, 445,
        506, 510, 511,
        511, 507, 506,
        507, 511, 512,
        512, 508, 507,
        508, 512, 513,
        513, 509, 508,
        449, 453, 514,
        514, 510, 449,
        510, 514, 515,
        515, 511, 510,
        511, 515, 516,
        516, 512, 511,
        512, 516, 517,
        517, 513, 512,
        453, 457, 518,
        518, 514, 453,
        514, 518, 519,
        519, 515, 514,
        515, 519, 520,
        520, 516, 515,
        516, 520, 521,
        521, 517, 516,
        457, 461, 522,
        522, 518, 457,
        518, 522, 523,
        523, 519, 518,
        519, 523, 524,
        524, 520, 519,
        520, 524, 525,
        525, 521, 520,
        461, 465, 526,
        526, 522, 461,
        522, 526, 527,
        527, 523, 522,
        523, 527, 528,
        528, 524, 523,
        524, 528, 529,
        529, 525, 524,
        465, 405, 466,
        466, 526, 465,
        526, 466, 467,
        467, 527, 526,
        527, 467, 468,
        468, 528, 527,
        528, 468, 469,
        469, 529, 528
    ];
    
    var elements = teapot_elements;
    var coords = teapot_coords;

    var mesh = Lux.Mesh.indexed(coords, elements);
    mesh.make_normals();
    return mesh.model;
};
Lux.Mesh = {};
Lux.Mesh.indexed = function(vertices, elements)
{
    vertices = vertices.slice();
    elements = elements.slice();
    
    var model = Lux.model({
        type: "triangles",
        elements: elements,
        vertex: [vertices, 3]
    });

    var normals;

    function create_normals() {
        var normal = new Float32Array(vertices.length);
        var areas = new Float32Array(vertices.length / 3);

        for (var i=0; i<elements.length; i+=3) {
            var i1 = elements[i], i2 = elements[i+1], i3 = elements[i+2];
            var v1 = vec3.copy(vertices.slice(3 * i1, 3 * i1 + 3));
            var v2 = vec3.copy(vertices.slice(3 * i2, 3 * i2 + 3));
            var v3 = vec3.copy(vertices.slice(3 * i3, 3 * i3 + 3));
            var cp = vec3.cross(vec3.minus(v2, v1), vec3.minus(v3, v1));
            var area2 = vec3.length(cp);
            areas[i1] += area2;
            areas[i2] += area2;
            areas[i3] += area2;
            
            normal[3*i1]   += cp[0];
            normal[3*i1+1] += cp[1];
            normal[3*i1+2] += cp[2];
            normal[3*i2]   += cp[0];
            normal[3*i3+1] += cp[1];
            normal[3*i1+2] += cp[2];
            normal[3*i1]   += cp[0];
            normal[3*i2+1] += cp[1];
            normal[3*i3+2] += cp[2];
        }

        for (i=0; i<areas.length; ++i) {
            normal[3*i] /= areas[i];
            normal[3*i+1] /= areas[i];
            normal[3*i+2] /= areas[i];
        }
        return normal;
    }

    return {
        model: model,
        make_normals: function() {
            if (!normals) {
                normals = create_normals();
                this.model.add("normal", [normals, 3]);
            }
            return normals;
        }
    };
};
/*
 * An actor must conform to the following interface:

 * - actors respond to a "dress" method. This method takes as a
 * parameter an object conforming to the scene interface and returns
 * an object conforming to the batch interface.

 * - actors respond to an "on" method. This method takes as a
 * parameter a string and an object. The string is the name of the
 * canvas event that was triggered, and the object is the
 * corresponding event. The method should return false if the event
 * handling chain is to be terminated. If true, the event handling
 * loop will keep traversing the scene graph and calling event
 * handlers.

 */

Lux.actor = function(opts)
{
    opts = _.defaults(opts, {
        on: function() { return true; },
        bake: Lux.bake
    });
    var appearance = opts.appearance;
    var model = opts.model;
    var on = opts.on;
    var bake = opts.bake;
    var batch;
    return {
        dress: function(scene) {
            var xform = scene.get_transform();
            var this_appearance = xform(appearance);
            return bake(model, this_appearance);
        },
        on: function(event_name, event) {
            return opts.on(event_name, event);
        }
    };
};

Lux.actor_list = function(actors_list)
{
    return {
        dress: function(scene) {
            var batch_list = _.map(actors_list, function(actor) {
                return actor.dress(scene);
            });
            return {
                draw: function() {
                    _.each(batch_list, function(batch) {
                        return batch.draw();
                    });
                }
            };
        },
        on: function(event_name, event) {
            for (var i=0; i<actors_list.length; ++i) {
                if (!actors_list[i].on(event_name, event))
                    return false;
            }
            return true;
        }
    };
};
/*
 * Scenes conform to the actor interface. Scenes can then
   contain other scenes, and have hierarchical structure. Currently,
   "sub-scenes" cannot have more than one parent. (If you're thinking
   about scene graphs and sharing, this means that, to you, Lux scenes
   are actually "scene trees".)

 */
Lux.scene = function(opts)
{
    opts = _.defaults(opts || {}, {
        context: Lux._globals.ctx,
        transform: function(i) { return i; },
        pre_draw: function() {},
        post_draw: function() {}
    });
    var ctx = opts.context;
    var transform = opts.transform;

    var dirty = false;
    var pre_display_list = [];
    var post_display_list = [];
    function draw_it() {
        Lux.set_context(ctx);
        var pre = pre_display_list;
        pre_display_list = [];
        var post = post_display_list;
        post_display_list = [];
        for (var i=0; i<pre.length; ++i)
            pre[i]();
        scene.draw();
        dirty = false;
        for (i=0; i<post.length; ++i)
            post[i]();
    }

    var batch_list = [];
    var actor_list = [];
    var parent_scene = undefined;
    var scene = {
        context: ctx,
        get_transform: function() { return transform; },

        add: function(actor) {
            actor_list.push(actor);
            var result = actor.dress(this);
            batch_list.push(result);
            this.invalidate(undefined, undefined, ctx);
            return result;
        }, 

        //////////////////////////////////////////////////////////////////////
        /*
         * animate starts a continuous stream of animation
         * refresh triggers. It returns an object with a single field
         * "stop", which is a function that when called will stop the
         * refresh triggers.
         */

        animate: function(tick_function) {
            if (parent_scene)
                return parent_scene.animate(tick_function);
            if (_.isUndefined(tick_function)) {
                tick_function = _.identity;
            }
            var done = false;
            var that = this;
            function f() {
                that.invalidate(
                    function() {
                        tick_function();
                    }, function() { 
                        if (!done) f();
                    }, ctx);
            };
            f();
            return {
                stop: function() {
                    done = true;
                }
            };
        },

        /*
         * scene.invalidate triggers a scene redraw using
         * requestAnimFrame.  It takes two callbacks to be called respectively
         * before the scene is drawn and after. 
         * 
         * The function allows many different callbacks to be
         * invoked by a single requestAnimFrame handler. This guarantees that
         * every callback passed to scene.invalidate during the rendering
         * of a single frame will be called before the invocation of the next scene 
         * redraw.
         * 
         * If every call to invalidate issues a new requestAnimFrame, the following situation might happen:
         * 
         * - during scene.render:
         * 
         *    - object 1 calls scene.invalidate(f1, f2) (requestAnimFrame #1)
         * 
         *    - object 2 calls scene.invalidate(f3, f4) (requestAnimFrame #2)
         * 
         *    - scene.render ends
         * 
         * - requestAnimFrame #1 is triggered:
         * 
         *    - f1 is called
         * 
         *    - scene.render is called
         * 
         *    ...
         * 
         * So scene.render is being called again before f3 has a chance to run.
         * 
         */
        invalidate: function(pre_display, post_display) {
            if (parent_scene) {
                parent_scene.invalidate(pre_display, post_display);
                return;
            }
            if (!dirty) {
                dirty = true;
                window.requestAnimFrame(function() { return draw_it(); });
            }
            if (pre_display) {
                pre_display_list.push(pre_display);
            }
            if (post_display) {
                post_display_list.push(post_display);
            }
        },


        //////////////////////////////////////////////////////////////////////
        // actor interface

        on: function(event_name, event) {
            for (var i=0; i<actor_list.length; ++i) {
                if (!actor_list[i].on(event_name, event))
                    return false;
            }
            return true;
        },

        dress: function(scene) {
            parent_scene = scene;
            var that = this;
            // reset transform, then re-add things to batch list.
            transform = function(appearance) {
                appearance = opts.transform(appearance);
                appearance = parent_scene.get_transform()(appearance);
                return appearance;
            };
            transform.inverse = function(appearance) {
                appearance = parent_scene.get_transform().inverse(appearance);
                appearance = opts.transform.inverse(appearance);
                return appearance;
            };
            // FIXME ideally we'd have a well-defined cleanup of batches; I
            // think the current implementation below might leak.
            batch_list = _.map(actor_list, function(actor) {
                return actor.dress(that);                
            });
            return this;
        },

        //////////////////////////////////////////////////////////////////////
        // batch interface

        draw: function() {
            opts.pre_draw();
            for (var i=0; i<batch_list.length; ++i) {
                batch_list[i].draw();
            }
            opts.post_draw();
        }

    };
    return scene;
};

Lux.default_scene = function(opts)
{
    opts = _.clone(opts);
    opts.transform = function(appearance) {
        appearance = _.clone(appearance);
        if (!_.isUndefined(appearance.screen_position))
            appearance.position = appearance.screen_position;
        // return Shade.canonicalize_program_object(appearance);
        return appearance;
    };
    opts.transform.inverse = function(i) { return i; };
    var scene = Lux.scene(opts);
    var ctx = scene.context;

    var clearColor, clearDepth;

    if (Lux.is_shade_expression(opts.clearColor)) {
        if (!opts.clearColor.is_constant())
            throw new Error("clearColor must be constant expression");
        if (!opts.clearColor.type.equals(Shade.Types.vec4))
            throw new Error("clearColor must be vec4");
        clearColor = _.toArray(opts.clearColor.constant_value());
    } else
        clearColor = opts.clearColor;

    if (Lux.is_shade_expression(opts.clearDepth)) {
        if (!opts.clearDepth.is_constant())
            throw new Error("clearDepth must be constant expression");
        if (!opts.clearDepth.type.equals(Shade.Types.float_t))
            throw new Error("clearDepth must be float");
        clearDepth = opts.clearDepth.constant_value();
    } else
        clearDepth = opts.clearDepth;

    // FIXME this is kind of ugly, but would otherwise requiring changing the picker infrastructure
    // quite a bit. Since the picker infrastructure should be overhauled anyway,
    // we stick with this hack until we fix everything.
    function clear() {
        switch (ctx._lux_globals.batch_render_mode) {
        case 1:
        case 2:
            ctx.clearDepth(clearDepth);
            ctx.clearColor(0,0,0,0);
            break;
        case 0:
            ctx.clearDepth(clearDepth);
            ctx.clearColor.apply(ctx, clearColor);
            break;
        default:
            throw new Error("Unknown batch rendering mode");
        }
        ctx.clear(ctx.COLOR_BUFFER_BIT | ctx.DEPTH_BUFFER_BIT);
    }
    scene.add({
        dress: function(scene) { return { draw: clear }; },
        on: function() { return true; }
    });
    return scene;
};
Lux.Scene = {};
Lux.Scene.add = function(obj, ctx)
{
    if (_.isUndefined(ctx)) {
        ctx = Lux._globals.ctx;
    }
    var scene = ctx._lux_globals.scene;

    return scene.add(obj);
};
Lux.Scene.remove = function(obj, ctx)
{
    if (_.isUndefined(ctx)) {
        ctx = Lux._globals.ctx;
    }
    var scene = ctx._lux_globals.scene;

    var i = scene.indexOf(obj);

    if (i === -1) {
        return undefined;
    } else {
        return scene.splice(i, 1)[0];
    }
    Lux.Scene.invalidate(undefined, undefined, ctx);
};
Lux.Scene.render = function()
{
    var scene = Lux._globals.ctx._lux_globals.scene;
    for (var i=0; i<scene.length; ++i) {
        scene[i].draw();
    }
};
Lux.Scene.animate = function(tick_function, ctx)
{
    if (_.isUndefined(ctx)) {
        ctx = Lux._globals.ctx;
    }
    var scene = ctx._lux_globals.scene;

    return scene.animate(tick_function);
};
Lux.Scene.on = function(ename, event, ctx) 
{
    if (_.isUndefined(ctx)) {
        ctx = Lux._globals.ctx;
    }
    var scene = ctx._lux_globals.scene;

    return scene.on(ename, event);
};
Lux.Scene.invalidate = function(pre_display, post_display, ctx)
{
    if (_.isUndefined(ctx)) {
        ctx = Lux._globals.ctx;
    }
    var scene = ctx._lux_globals.scene;

    return scene.invalidate(pre_display, post_display);
};
Lux.Scene.Transform = {};
Lux.Scene.Transform.Geo = {};

(function() {

var two_d_position_xform = function(xform, inverse_xform) {
    function make_it(xf) {
        return function(appearance) {
            if (_.isUndefined(appearance.position))
                return appearance;
            appearance = _.clone(appearance);
            var pos = appearance.position;
            var out = xf(appearance.position.x(), appearance.position.y());
            if (pos.type.equals(Shade.Types.vec2))
                appearance.position = out;
            else if (pos.type.equals(Shade.Types.vec3))
                appearance.position = Shade.vec(out, pos.at(2));
            else if (pos.type.equals(Shade.Types.vec4))
                appearance.position = Shade.vec(out, pos.swizzle("zw"));
            return appearance;
        };
    };
    return function(opts) {
        opts = _.clone(opts || {});
        opts.transform = make_it(xform);
        if (!_.isUndefined(inverse_xform)) {
            opts.transform.inverse = make_it(inverse_xform);
            opts.transform.inverse.inverse = opts.transform;
        }
        return Lux.scene(opts);
    };
};
Lux.Scene.Transform.Geo.latlong_to_hammer = function(opts) {
    opts = _.clone(opts || {});
    opts.transform = function(appearance) {
        if (_.isUndefined(appearance.position))
            return appearance;
        appearance = _.clone(appearance);
        var pos = appearance.position;
        var out = Shade.Scale.Geo.latlong_to_hammer(appearance.position.x(), appearance.position.y(), opts.B);
        if (pos.type.equals(Shade.Types.vec2))
            appearance.position = out;
        else if (pos.type.equals(Shade.Types.vec3))
            appearance.position = Shade.vec(out, pos.at(2));
        else if (pos.type.equals(Shade.Types.vec4))
            appearance.position = Shade.vec(out, pos.swizzle("zw"));
        return appearance;
    };
    return Lux.scene(opts);
};
Lux.Scene.Transform.Geo.latlong_to_mercator = 
    two_d_position_xform(Shade.Scale.Geo.latlong_to_mercator,
                         Shade.Scale.Geo.mercator_to_latlong);
Lux.Scene.Transform.Geo.latlong_to_spherical = function(opts) {
    opts = _.clone(opts || {});
    opts.transform = function(appearance) {
        if (_.isUndefined(appearance.position))
            return appearance;
        appearance = _.clone(appearance);
        var pos = appearance.position;
        var lat = appearance.position.x();
        var lon = appearance.position.y();
        var out = Shade.Scale.Geo.latlong_to_spherical(lat, lon);
        if (pos.type.equals(Shade.Types.vec3))
            appearance.position = out;
        else if (pos.type.equals(Shade.Types.vec4))
            appearance.position = Shade.vec(out, pos.w());
        return appearance;
    };
    return Lux.scene(opts);
};
Lux.Scene.Transform.Geo.mercator_to_latlong = 
    two_d_position_xform(Shade.Scale.Geo.mercator_to_latlong,
                         Shade.Scale.Geo.latlong_to_mercator);
})();
Lux.Scene.Transform.Camera = {};
Lux.Scene.Transform.Camera.perspective = function(opts)
{
    opts = _.clone(opts || {});
    var camera = Shade.Camera.perspective(opts);
    opts.transform = function(appearance) {
        if (_.isUndefined(appearance.position))
            return appearance;
        appearance = _.clone(appearance);
        appearance.position = camera(appearance.position);
        return appearance;
    };
    var scene = Lux.scene(opts);
    scene.camera = camera;
    return scene;
};
