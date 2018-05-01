/*global $ jsep  */

var Expression = function(expr){
    this.parsetree = jsep(expr);
};

Expression.prototype = {
    getData: function(q,qfunc){
        return this._process(this.parsetree,q,qfunc);
    },

    _process: function(expr,q,qfunc){
        var p;
        switch(expr.type) {
        case 'CallExpression':
            p =  this._binExp(expr,q,qfunc);
            break;
        case 'BinaryExpression':
            p =  this._binExp(expr,q,qfunc);
            break;
        case 'LogicalExpression':
            p =  this._binExp(expr,q,qfunc);
            break;
        case 'MemberExpression':
            p = this._memExp(expr,q,qfunc);
            break;
        case 'Literal':
            var dfd = new $.Deferred();
            p = dfd.promise();
            dfd.resolve(expr.value);
            break;
        case 'Identifier':
            p = qfunc(q[expr.name]);
            break;
        default:
            throw "Cannot parse expression";
        }
        return p;
    },

    _memExp: function(memexp, q, qfunc){

        //function for recursive processing
        function memExpQuery(memexp, q){
            //process the type
            var newq = null ;
            if (memexp.object.type == 'MemberExpression'){
                newq = memExpQuery(memexp.object, q);
            }
            else if (memexp.object.type == 'Identifier'){
                //select the base query
                newq = $.extend(true,{}, q[memexp.object.name]);
            }

            //process the properties
            var prop = memexp.property;
            if (prop.type=='BinaryExpression'&& prop.operator==  '==' ){
                var catvar = prop.left.name;
                var catval;

                if(prop.right.type == 'Identifier'){
                    catval = [prop.right.name];
                }

                if(prop.right.type == 'Literal'){
                    catval = [prop.right.value];
                }

                if(prop.right.type == 'ArrayExpression'){
                    catval = prop.right.elements.map(function(d){
                        if (d.name){
                            return d.name;
                        }
                        else{
                            return d.value;
                        }
                    });
                }
                
                catval = catval.map(function(d){ return {cat: d , id: null };});
                
                newq.setCatConst(catvar,catval);
            }
            return newq;
        }

        //process the query
        var resq = memExpQuery(memexp,q);

        //exec the spatial query
        return qfunc(resq);
    },

    _binExp: function(binexp, q, qfunc){
        var dfd = new $.Deferred();

        //process left and right
        var left = this._process(binexp.left,q,qfunc);
        var right = this._process(binexp.right,q,qfunc);

        var expr = this;
        $.when(left,right).done(function(){
            var results = arguments;
            var resleft = results[0];
            var resright = results[1];

            function getOpFunc(operator){
                switch (operator){
                case '+':
                    return function(a,b) {return a+b;};
                case '-':
                    return function(a,b) {return a-b;};
                case '*':
                    return function(a,b) {return a*b;};
                case '/':
                    return function(a,b) {
                        if(isNaN(a/b)){
                            return 0;
                        }
                        else{
                            return a/b;
                        }
                    };
                case '||':
                    return function(a,b) { return Math.max(a,b); };
                case '&&':
                    return function(a,b) { return Math.min(a,b); };

                default:
                    throw "Unsupported Operation";
                }
            }

            var opfunc = getOpFunc(binexp.operator);
            if (!opfunc){
                dfd.resolve(null);
            }

            var res = null;
            if (opfunc){
                res = expr._op(opfunc,resleft,resright);
            }
            dfd.resolve(res);
        });
        return dfd.promise();
    },

    _callExp: function(callexp, q, qfunc){
        var dfd = new $.Deferred();

        //process the arguments
        var args = callexp.arguments.forEach(function(d){
            return this._process(d,q,qfunc);
        });

        var expr = this;
        $.when.apply($,args).done(function(){
            var results = arguments;

            function getOpFunc(operator){
                switch (operator){
                case '+':
                    return function(a,b) {return a+b;};
                case '-':
                    return function(a,b) {return a-b;};
                case '*':
                    return function(a,b) {return a*b;};
                case '/':
                    return function(a,b) {return (a+1e-4)/(b+1e-4);};
                default:
                    throw "Unsupported Operation";
                }
            }

            var opfunc = getOpFunc(binexp.operator);
            if (!opfunc){
                dfd.resolve(null);
            }

            var res = null;
            if (opfunc){
                res = expr._op(opfunc,resleft,resright);
            }
            dfd.resolve(res);
        });
        return dfd.promise();
    },

    _opTemporal: function(opfunc,left,right){
        var lefthash = {};
        if (typeof left === 'number'){
            right.data.forEach(function(d,i){
                lefthash[d.time] = left;
            });
        }
        else{
            left.data.forEach(function(d,i){
                lefthash[d.time] = d.val;
            });
        }
       var righthash = {};
        if (typeof right == 'number'){
            left.data.forEach(function(d,i){
                righthash[d.time] = right;
            });
        }
        else{
            right.data.forEach(function(d,i){
                righthash[d.time] = d.val;
            });
        }


        var allkeys = {};
        Object.keys(righthash).forEach(function(d){ allkeys[d]=1; });
        Object.keys(lefthash).forEach(function(d){ allkeys[d]=1; });


        var res = {};
        res.data = Object.keys(allkeys).map(function(k){
            var l = lefthash[k] || 0 ;
            var r = righthash[k] || 0;
            var val =  opfunc(l,r);

            return {time: new Date(k),val: val};
        });
        res.data = res.data.filter(function(d){return isFinite(d.val);});
        res.data = res.data.filter(function(d){return d.val !== 0;});
        res.type = left.type || right.type;
        //res.data = res.data.sort(function(a,b){return a.time - b.time;});

        return res;
    },

    _opCategorical: function(opfunc,left,right){
        if (typeof left === 'number'){
            var leftval = left;
            left = $.extend(true, {}, right);
            left.data = left.data.map(function(d) {
                d.val = leftval;
                return d;
            });
        }
        
        if (typeof right == 'number'){
            var rightval = right;
            right = $.extend(true, {}, left);
            right.data = right.data.map(function(d) {
                d.val = rightval;
                return d;
            });

        }
        var lefthash = {};
        left.data.forEach(function(d) {
            lefthash[d.id]=d.val;
        });
        var righthash = {};
        right.data.forEach(function(d) {
            righthash[d.id]=d.val;
        });
        
        var allkeys = {};
        left.data.forEach(function(d){
            allkeys[d.id] = d.cat;
        });

        right.data.forEach(function(d){
            allkeys[d.id] = d.cat;
        });

        var res = {};
        res.data = Object.keys(allkeys).map(function(k){
            var l = lefthash[k] || 0 ;
            var r = righthash[k] || 0;
            var val = opfunc(l,r);

            return {id:k, cat:allkeys[k],val:val};
        });
        res.data = res.data.filter(function(d){return isFinite(d.val);});
        res.data = res.data.filter(function(d){return d.val !== 0;});
        res.type = left.type || right.type;
        return res;
    },

    _opSpatial: function(opfunc,left,right){
        var lefthash = {};
        if (typeof left === 'number'){
            right.data.forEach(function(d,i){
                lefthash[[d.x,d.y]] = left;
            });
        }
        else{
            left.data.forEach(function(d,i){
                lefthash[[d.x,d.y]] = d.val;
            });
        }

        var righthash = {};
        if (typeof right == 'number'){
            left.data.forEach(function(d,i){
                righthash[[d.x,d.y]] = right;
            });
        }
        else{
            right.data.forEach(function(d,i){
                righthash[[d.x,d.y]] = d.val;
            });
        }


        var allkeys = {};
        Object.keys(righthash).forEach(function(d){ allkeys[d]=1; });
        Object.keys(lefthash).forEach(function(d){ allkeys[d]=1; });



        var res = {opts: left.opts || right.opts};
        res.data = Object.keys(allkeys).map(function(k){
            var l = lefthash[k] || 0 ;
            var r = righthash[k] || 0;
            var val =  opfunc(l,r);

            var coord = k.split(',');
            return {x: +coord[0],y: +coord[1],val: val};
        });
        res.data = res.data.filter(function(d){return isFinite(d.val);});
        res.data = res.data.filter(function(d){return d.val !== 0;});
        res.type = left.type || right.type;
        return res;
    },

    _op: function(opfunc,left,right){
        var type = left.type || right.type;

        switch(type){
        case 'spatial':
            return this._opSpatial(opfunc,left,right);
        case 'temporal':
            return this._opTemporal(opfunc,left,right);
        case 'cat':
            return this._opCategorical(opfunc,left,right);

        default:
            return null;
        }
    }
};
