//Import the CSS
import './nanocubes.css';


//My classes
import Nanocube4 from './src/Nanocube/Nanocube4';
import Nanocube3 from './src/Nanocube/Nanocube3';
import Viewer from './src/Nanocube/Viewer';

let fetch = window.fetch;


//main
let urlargs = getArgFromUrl();
urlargs.config = urlargs.config || './config.json';

(async (urlargs)=>{
    let config = null;
    try{
        let res = await fetch(urlargs.config);
        config = await res.json();
    }
    catch(e){
        console.log('Fail to read '+ urlargs.config);
        return;
    }

    //start the viewer
    await startViewer(config,urlargs);
})(urlargs);


//Helper functions

//https://stackoverflow.com/questions/8486099/how-do-i-parse-a-url-query-parameters-in-javascript
function getArgFromUrl(){
    let query = window.location.search.substr(1);
    let result = {};
    query.split("&").forEach(function(part) {
        let item = part.split("=");
        if(item[0] !== ""){
            result[item[0]] = decodeURIComponent(item[1]);
        }
    });
    
    return result;
};

async function nc3or4(url){
    try{  //v4
        let response = await fetch(url + '/schema()');
        let schema = await response.json();
        let nc = new Nanocube4();
        return nc.init(url);
    }
    catch(e){
        try{ //v3
            let response = await fetch(url + '/schema');
            let schema = await response.json();
            let nc = new Nanocube3();
            return nc.init(url);
        }
        catch(e){
            console.log(url+' is not a Nanocube');
            return null;
        }
    }
};

async function startViewer(config,urlargs){
    let ncnames = Object.keys(config.nanocube);
    let ncpromises = ncnames.map((k)=>nc3or4(config.nanocube[k].url));
    let nanocubes = await Promise.all(ncpromises);
    
    let nchash = {};
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
}
