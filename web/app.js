var nc = new Nanocube({
    url: 'http://localhost:29510',
    ready: function(nc){ 
	var q = nc.query();
	q = q.dim('location')
	q.rectQuery({x:1049,y:2571,z:12},{x:1050,y:2572,z:12});
	console.log(q, q.toString('count'))
    }
});
