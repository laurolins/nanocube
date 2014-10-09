var nc = new Nanocube({
    url: 'http://localhost:29512',
    ready: function(nc){ 
	var q = nc.query();
	q = q.dim('location').rectQuery(new Tile(1049,2571,12), new Tile(1050,2572,12));
	q = nc.query().dim('location').tile(new Tile(1,2,2),8);
	console.log(q.toString('count'));
	q.run_query().done(
	    function(json){
	    }
	});
    }
});
