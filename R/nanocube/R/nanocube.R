nanocube.load <- function(sFiles)
{
	.Call(ncr_load, sFiles)
}

nanocube.messages <- function(sEngine)
{
	.Call(ncr_messages, sEngine)
}

nanocube.query <- function(sEngine, sQuery)
{
	.Call(ncr_query, sEngine, sQuery)
}


# function degrees, mercator, tile

nanocube.d2m <- function(x) 
{
	if (class(x)=="numeric") { # single point
		c(x=x[1]/180, y=log(tan((x[2] * pi/180.0)/2 + pi/4))/pi)
	}
	else if (class(x) == "matrix") { # matrix with two columns x==1 and y==2
		cbind(x=x[,1]/180, y=log(tan((x[,2] * pi/180.0)/2 + pi/4))/pi)
	}
	else {
		return("Invalid operation");
	}
}

nanocube.m2d <- function(x) 
{
	if (class(x)=="numeric") {
		c(x=x[1]*180, y=atan(sinh(x[2] * pi)) / (pi/180))
	}
	else if (class(x) == "matrix") {
		cbind(x=x[,1]*180, y=atan(sinh(x[,2] * pi)) / (pi/180))
	}
	else {
		return("Invalid operation");
	}
}

nanocube.d2c <- function(points,level) 
{
	x = points[,1]/180
	y = log(tan((points[,2] * pi/180.0)/2 + pi/4))/pi
	result = cbind(x = as.integer((x + 1.0)/2.0 * 2^level),
		       y = as.integer((y + 1.0)/2.0 * 2^level),
		       z = level)
	storage.mode(result) = "integer"
	return(result)
}

nanocube.m2c <- function(points,level) 
{
	x = points[,1]
	y = points[,2]
	result =cbind( x = as.integer((x + 1.0)/2.0 * 2^level ),
		      y = as.integer((y + 1.0)/2.0 * 2^level ),
		      z = level )
	storage.mode(result) = "integer"
	return(result)
}

nanocube.cellname_bounds <- function(cellnames) 
{
	result = matrix(unlist(lapply(1:dim(cellnames)[1],
				      function(i) {
					      x     = cellnames[i,1]
					      y     = cellnames[i,2]
					      level = cellnames[i,3]
					      n = 2^level
					      c(  2*x/n-1,
						2*y/n-1,
						2*(x+1)/n-1,
						2*(y+1)/n-1)
				      })), ncol=4, byrow=T)
	colnames(result) = c("x1","y1","x2","y2")
	result
}


