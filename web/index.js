import jquery from 'jquery';
let $ = window.$ = jquery;

//Import the CSS
import './nanocubes.css';


//My classes
import Nanocube4 from './src/Nanocube/Nanocube4';
import Nanocube3 from './src/Nanocube/Nanocube3';
import Viewer from './src/Nanocube/Viewer';


//https://stackoverflow.com/questions/8486099/how-do-i-parse-a-url-query-parameters-in-javascript
function getArgFromUrl() {
    let query = window.location.search.substr(1);
    let result = {};
    query.split("&").forEach(function(part) {
        var item = part.split("=");
        if(item[0] !== ""){
            result[item[0]] = decodeURIComponent(item[1]);
        }
    });
        
    return result;
}


let fetch = window.fetch;
let nc3or4 = async(url)=>{
    try{  //v4
        let response = await fetch(url + '/schema()');
        let schema = await response.json();
        return Nanocube4.initNanocube(url);
    }
    catch(e){
        try{ //v3
            let response = await fetch(url + '/schema');
            let schema = await response.json();
            return Nanocube3.initNanocube(url);
        }
        catch(e){
            console.log(url+' is not a Nanocube');
        }
    }
};

let startViewer=async(config,urlargs)=>{
    let nanocubes= await Promise.all(Object.keys(config.nanocube)
                                     .map((k)=>nc3or4(config.nanocube[k].url)));
    
    let nchash = {};
    let ncnames = Object.keys(config.nanocube);
    nanocubes.forEach(function(d,i){
        nchash[ncnames[i]] = d;
    });
    let viewer = new Viewer({
        nanocubes: nchash,
        div_id:'#nc',
        config: config,
        urlargs: urlargs
    });
    viewer.update();
};

//main
let urlargs = getArgFromUrl();
urlargs.config = urlargs.config || './config.json';


(async (urlargs)=>{
    try{
        let res = await fetch(urlargs.config);
        let config = await res.json();
        //start the viewer
        startViewer(config,urlargs);
    }
    catch(e){
        console.log('Fail to read '+ urlargs.config);
    }
})(urlargs);
