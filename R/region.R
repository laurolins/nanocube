setwd("/home/llins/code/compressed_nanocube/R")
require(polycover)
require(nanocube)
require(RColorBrewer)
library(ggplot2)

# thanksgiving dates
# 2015-11-26
# 2014-11-27
# 2013-11-28
# 2012-11-22
# 2011-11-24
# 2010-11-25
# 2009-11-26
#
years  <- 9:15
tgdays <- c(26, 25, 24, 22, 28, 27, 26)

years  <- tail(years,6)
tgdays <- tail(tgdays,6)


map <- function(pattern, values, repetition)
{
	sapply(values, function(v) { do.call("sprintf",c(pattern, as.list(rep(v,repetition)))) })
}

# "nov09=/home/llins/data/taxi/november-2015.nanocube"))
# "nov15=/home/llins/data/taxi/november-2015.nanocube"))
engine <- nanocube::nanocube.load(map("nov%02d=/home/llins/data/taxi/november-20%02d.nanocube",years,2))
nanocube::nanocube.messages(engine)

# some spatial selections as tiles
regions.names <- c("jfk", "empirestate", "any", "jfk-tile")
regions <- sapply(regions.names, function (base) {
			fname <- sprintf(sprintf("%s.tiles",base))
			fcontent <-readChar(fname, file.info(fname)$size)
			#  print(fcontent)
			strsplit(fcontent, "\n")[1]})
names(regions) <- regions.names

# draw
draw <- function(src, dst, unit.in.hours)
{

	unit          <-   unit.in.hours * 60 * 60
	units.per.day <- (24 * 60 * 60) / unit
	num.units     <-  31 * units.per.day

	# the interval for each day will be day * [-0.5, units.per.day-0.5)
	# values will be ploted at 0 to units.per.day-1
	days <- seq(-20,3,1)
	days.bounds  <- -0.5 + units.per.day * c(days, max(days) + 1)
	days.centers <- -0.5 + units.per.day * days + units.per.day/2
	weekdays <- c("Thursday", "Friday", "Saturday", "Sunday", "Monday", "Tuesday", "Wednesday")
	day.lbl <- function(d)
	{
		if (d == 0) {
			"Thanksgiving\nThursday"
		} else if (d < 0) {
			sprintf("D-%02d\n%s",abs(d),weekdays[ 1 + (d %% length(weekdays))])
		} else {
			sprintf("D+%02d\n%s",d,weekdays[ 1 + (d %% length(weekdays))])
		}
	}

	tseries <- lapply(1:length(years), function(i) {
				  y <- years[i]
				  src.constraint  <- sprintf(".b(\"pickup_location\",mask(\"%s\"))",regions[src])
				  dst.constraint  <- sprintf(".b(\"dropoff_location\",mask(\"%s\"))",regions[dst])
				  time.constraint <- sprintf(".b(\"pickup_time\",timeseries(\"20%02d-11-01T00:00-05:00\",%d,%d,%d))",y,unit,num.units,unit)
				  query <- paste(sprintf("nov%02d",y),src.constraint,dst.constraint,time.constraint,";",sep="")
				  print(query)
				  # print(query)
				  t <- nanocube::nanocube.query(engine, query)
				  t <- data.frame(t,
						  off=t$pickup_time - units.per.day * (tgdays[i] - 1),
						  pace=t$duration/t$count)
				  t <- t[order(t$off),]
				  sel <- (t$off >= min(days.bounds)) & (t$off <= max(days.bounds))
				  t <- t[sel,]
	})
	names(tseries) = sprintf("y%02d", years)

	# On November 7, 2014 the speed limit in NYC was lowered from 30mph to 25mph

	# tg14 <- 27
	# tg15 <- 26
	# # align dates
	# t14 <- data.frame(t14,off=t14$pickup_time - 24 * (tg14 - 1))
	# t15 <- data.frame(t15,off=t15$pickup_time - 24 * (tg15 - 1))

	sel <- which(years %in% years)

	add.alpha <- function(col, alpha=1)
	{
		if(missing(col))
			stop("Please provide a vector of colours.")
		apply(sapply(col, col2rgb)/255, 2,
		      function(x)
			      rgb(x[1], x[2], x[3], alpha=alpha))
	}
	colors <- rev(brewer.pal(length(sel),"Set1"))
	colors[1] <- rgb(0.3,0.3,0.3)
	colors <- add.alpha(colors, 0.4)

	ranges <- sapply(1:length(sel), function(i) {
				 t <- tseries[[sel[i]]]
				 c(range(t$off),range(t$count), range(t$pace))
			      })


	xrng <- c(min(ranges[1,]),max(ranges[2,]))
	crng <- c(min(ranges[3,]),max(ranges[4,]))
	yrng <- c(min(ranges[5,]),max(ranges[6,]))

	pace       <- pretty(c(0,yrng),n=5)
	pace.pos   <- pace/yrng[2]

	volume     <- pretty(c(0,crng),n=5)
	volume.pos <- volume/crng[2]

	# line widths
	lwd.min     <- 0.25
	lwd.max     <- 4
	# lwd.coef    <- (lwd.max - lwd.min) ^ (1/length(sel))
	lwd         <-  lwd.min + lwd.max*(1:length(sel))/length(sel)
	line.type   <- "l"
	legend.pos  <- "top"
	# lwd=rep(1,length(sel))

	#
	# Hourly Pace
	#
	pdf(sprintf("pace-%s-%s-%dh.pdf",src,dst,unit.in.hours),width=16,height=5,pointsize=8)
	plot(0,type="n",xlim=range(days.bounds),ylim=c(0,1.1),
	     main=sprintf("%d-Hour(s) Average Trip Duration Yellow Cab Rides Around Thanksgiving Day From %s to %s",unit.in.hours,src,dst),
	     xlab="",ylab="",axes=F, xaxs="i")
	mtext(sapply(days, day.lbl),1,at=days.centers,tck=0,line=1.5,cex=0.9)
	mtext(sprintf("%dm",pace),2,at=pace.pos,las=2,line=0.7,cex=0.9)
	rect(-0.5,-1.0,units.per.day-0.5,2.0,col=gray(0.9))
	abline(h=pace.pos,col=gray(0.5))
	abline(v=days.bounds,col=gray(0.5))
	# abline(v=c(-0.5,units.per.day-0.5),col=gray(0.2),lwd=3)
	# rect(-0.5,0.0,units.per.day-0.5,1.0,col=gray(0.2))
	box()
	for (j in 1:length(sel)) {
		i <- sel[j]
		t <- tseries[[i]]
		x <- t$off
		y <- t$pace / yrng[2]
		# c <- (t$count) / crng[2]
		# lines(x, y, type="l",col=colors[j],lwd=lwd[j])
		points(x, y, type=line.type, col=colors[j], lwd=lwd[j])
		# points(x, y, col=colors[j], pch=16, cex=1.2)
	}
	legend(legend.pos,
	       sprintf("%d",2000 + years[sel]),
	       col=colors,
	       lwd=lwd,
	       xjust=-1,yjust=-1,horiz=T,bg=rgb(1,1,1,0.75),box.lwd=0)
	dev.off()

	for (j in 1:length(sel)) {

		year <- 2000 + years[sel[j]]
		pdf(sprintf("pace-%s-%s-%dh-y%d.pdf",src,dst,unit.in.hours,year),width=16,height=5,pointsize=8)
		plot(0,type="n",xlim=range(days.bounds),ylim=c(0,1.1),
		     main=sprintf("%d-Hour(s) Average Trip Duration Yellow Cab Rides Around Thanksgiving Day From %s to %s",
				  unit.in.hours,src,dst),
		     xlab="",ylab="",axes=F, xaxs="i")
		mtext(sapply(days, day.lbl),1,at=days.centers,tck=0,line=1.5,cex=0.9)
		mtext(sprintf("%dm",pace),2,at=pace.pos,las=2,line=0.7,cex=0.9)
		rect(-0.5,-1.0,units.per.day-0.5,2.0,col=gray(0.9))
		abline(h=pace.pos,col=gray(0.5))
		abline(v=days.bounds,col=gray(0.5))
		# abline(v=c(-0.5,units.per.day-0.5),col=gray(0.2),lwd=3)
		# rect(-0.5,0.0,units.per.day-0.5,1.0,col=gray(0.2))
		box()

		# draw
		{
			i <- sel[j]
			t <- tseries[[i]]
			x <- t$off
			y <- t$pace / yrng[2]
			# c <- (t$count) / crng[2]
			# lines(x, y, type="l",col=colors[j],lwd=lwd[j])
			points(x, y, type=line.type, col=colors[j], lwd=lwd.max)
			# points(x, y, col=colors[j], pch=16, cex=1.2)
		}

		legend(legend.pos,
		       sprintf("%d",year),
		       col=colors[j],
		       lwd=lwd,
		       xjust=-1,yjust=-1,horiz=T,bg=rgb(1,1,1,0.75),box.lwd=0)
		dev.off()
	}

	#
	# Hourly Volume
	#
	pdf(sprintf("volume-%s-%s-%dh.pdf",src,dst,unit.in.hours),width=16,height=5,pointsize=8)
	plot(0,type="n",xlim=range(days.bounds),ylim=c(0,1.1),
	     main=sprintf("%d-Hour(s) Volume of Yellow Cab Rides Around Thanksgiving Day From %s to %s",unit.in.hours,src,dst),
	     xlab="",ylab="",axes=F, xaxs="i")
	mtext(sapply(days, day.lbl),1,at=days.centers,tck=0,line=1.5,cex=0.9)
	mtext(sprintf("%d",volume),2,at=volume.pos,las=2,line=0.7,cex=0.9)
	rect(-0.5,-1.0,units.per.day-0.5,2.0,col=gray(0.9))
	abline(h=volume.pos,col=gray(0.5))
	abline(v=days.bounds,col=gray(0.5))
	# abline(v=c(-0.5,units.per.day-0.5),col=gray(0.2),lwd=3)
	box()
	for (j in 1:length(sel)) {
		i <- sel[j]
		t <- tseries[[i]]
		x <- t$off
		y <- t$count / crng[2]
		# c <- (t$count) / crng[2]
		#lines(x, y, type="l",col=colors[j],lwd=lwd[j])
		points(x, y, type=line.type, col=colors[j], lwd=lwd[j])
		# points(x, y, col=colors[j], pch=16, cex=1.2)
	}
	legend(legend.pos,
	       sprintf("%d",2000 + years[sel]),
	       col=colors,
	       lwd=lwd,
	       xjust=-1,yjust=-1,horiz=T,bg=rgb(1,1,1,0.75),box.lwd=0)
	dev.off()

	for (j in 1:length(sel)) {
		year <- 2000 + years[sel[j]]
		pdf(sprintf("volume-%s-%s-%dh-y%d.pdf",src,dst,unit.in.hours,year),width=16,height=5,pointsize=8)
		plot(0,type="n",xlim=range(days.bounds),ylim=c(0,1.1),
		     main=sprintf("%d-Hour(s) Volume of Yellow Cab Rides Around Thanksgiving Day From %s to %s",unit.in.hours,src,dst),
		     xlab="",ylab="",axes=F, xaxs="i")
		mtext(sapply(days, day.lbl),1,at=days.centers,tck=0,line=1.5,cex=0.9)
		mtext(sprintf("%d",volume),2,at=volume.pos,las=2,line=0.7,cex=0.9)
		rect(-0.5,-1.0,units.per.day-0.5,2.0,col=gray(0.9))
		abline(h=volume.pos,col=gray(0.5))
		abline(v=days.bounds,col=gray(0.5))
		# abline(v=c(-0.5,units.per.day-0.5),col=gray(0.2),lwd=3)
		box()

		{
			i <- sel[j]
			t <- tseries[[i]]
			x <- t$off
			y <- t$count / crng[2]
			# c <- (t$count) / crng[2]
			#lines(x, y, type="l",col=colors[j],lwd=lwd[j])
			points(x, y, type=line.type, col=colors[j], lwd=lwd.max)
			# points(x, y, col=colors[j], pch=16, cex=1.2)
		}

		legend(legend.pos,
		       sprintf("%d",year),
		       col=colors[j],
		       lwd=lwd,
		       xjust=-1,yjust=-1,horiz=T,bg=rgb(1,1,1,0.75),box.lwd=0)
		dev.off()
	}
}


# src <- "any"
# dst <- "jfk"
# unit.in.hours <-   1

draw("any","jfk",1)
draw("any","jfk",4)
draw("any","jfk",6)
draw("any","jfk",24)
draw("jfk","any",1)
draw("jfk","any",4)
draw("jfk","any",6)
draw("jfk","any",24)
draw("any","any",1)
draw("any","any",4)
draw("any","any",6)
draw("any","any",24)


cells.jfk <- polycover::pc.cells(regions$jfk)
pdf("jfk.pdf",width=5,height=5)
polycover::pc.plot(cells.jfk)
dev.off()



# prepare path binary representation into tile coordinates
tx_tiles <- function(location.bin, inputres, tileres)
{
	n          <- length(location.bin)
	lower.30b  <- as.integer(location.bin %% 2^30)
	higher.30b <- as.integer(location.bin  / 2^30)
	M          <- cbind(
		matrix(sapply(1:15,function(i){ bitwAnd(3,bitwShiftR(higher.30b,28 - (2 * (i-1))))}),byrow=F,nrow=n),
		matrix(sapply(1:15,function(i){ bitwAnd(3,bitwShiftR(lower.30b, 28 - (2 * (i-1))))}),byrow=F,nrow=n)
	)
	p          <- rev(2^(0:29))
	x          <- sapply(1:nrow(M), function(i) { (bitwAnd(1,M[i,]) != 0) %*% p })
	y          <- sapply(1:nrow(M), function(i) { (bitwAnd(2,M[i,]) != 0) %*% p })
	x.tile     <- bitwShiftR(x, tileres)
	y.tile     <- bitwShiftR(y, tileres)
	side       <- 2^tileres
	x.pixel    <- bitwAnd(side-1,x)
	y.pixel    <- bitwAnd(side-1,y)
	x.tile0    <- min(x.tile)
	y.tile0    <- min(y.tile)
	x.img      <- (x.tile - x.tile0) * side + x.pixel
	y.img      <- (y.tile - y.tile0) * side + y.pixel
        width      <- max(x.tile) - min(x.tile) + 1
        height     <- max(y.tile) - min(y.tile) + 1

	list(
		inputres=inputres,
		tilelevel=tilelevel,
		tileres=tileres,
		tileside=side,
		imgwidth=width * side,
		imgheight=height * side,
		records= data.frame(
			xtile=x.tile,   ytile=y.tile,
			xpixel=x.pixel, ypixel=y.pixel,
			ximg=x.img,     yimg=y.img))

}

tx_img <- function(tx_tiles_data, values)
{
	m <- matrix(0,tx_tile_data$imgwidth,tx_tiles_data$height)
	for (i in 1:length(x)) {
	    m[1+x[i], 1+y[i]] = values[i]
	}
	m
}

tx_query <- function(engine, dataset, dimension, lat, lon, tillevel, tileres, tilexrange, tileyrange)
{
	base            <-  polycover::pc.d2c(cbind(lon,lat),tilelevel)
	xx              <-  base[,1] + tilexrange
	yy              <-  base[,2] + tileyrange
	tiles           <-  cbind(expand.grid(xx,yy),tilelevel)

	query.result    <- lapply(1:nrow(tiles),
	function(i) {
	nanocube::nanocube.query(engine,
			sprintf("%s
			        .b(\"%s\",dive(tile2d(%d,%d,%d),%d));",
			        # .b(\"dropoff_location\",dive(tile2d(%d,%d,%d),%d));",
			        #       			.b(\"pickup_location\",tile2d(16,19295,40896))
			        dataset, dimension, tiles[i,3],tiles[i,1],tiles[i,2],tileres))
	})

	# merge all query results into a signle data frame
	query.result     <- Reduce(function(a,b) { rbind(a,b)}, query.result)

	location.bin     <- query.result[,1]
	values           <- query.result[,2:ncol(query.result)]

	augmented.result <- tx_tiles(location.bin, tilelevel + tileres, tileres)
	augmented.result$records <- cbind(augmented.result$records, values)

	# sort records by
	pi = order(augmented.result$records$ximg,augmented.result$records$yimg)
	augmented.result$records <- augmented.result$records[pi,]

	augmented.result
}


# http://a.tile.openstreetmap.org/12/1205/1540.png
# lat             <-  40.716594
# lon             <- -74.006081
lat             <-  40.645475
lon             <- -73.784658
tilelevel       <-  15
tilexrange      <- -1:1
tileyrange      <- -1:1
tileres         <-  6

p <- tx_query(engine, "nov15", "pickup_location", lat, lon, tilelevel, tileres, tilexrange, tileyrange)

records         <- p$records

#rbind(data.frame(p1$records,source=1,end=1),
#                         data.frame(p2$records,source=2,end=1),
#                         data.frame(p3$records,source=3,end=1),
#                         data.frame(p4$records,source=4,end=1),
#                         data.frame(d1$records,source=1,end=2),
#                         data.frame(d2$records,source=2,end=2),
#                         data.frame(d3$records,source=3,end=2),
#                         data.frame(d4$records,source=4,end=2))

min.xtile        <- min(records$xtile)
min.ytile        <- min(records$ytile)
records$ximg = (records$xtile - min.xtile) * 2^tileres + records$xpixel
records$yimg = (records$ytile - min.ytile) * 2^tileres + records$ypixel

selection       <- records$count > 0
records         <- records[selection,]
values          <- as.integer(records$count > 10)

#fare/records$count
pdf("airport.pdf",width=6,height=6)
v <- ggplot(records, aes(ximg, yimg))
v + geom_point(aes(color=factor(values)),size=1) + coord_fixed(ratio = 1) + scale_colour_brewer(palette = "Set1")
dev.off()

# holes
holes <- function(x)
{
	r               <- range(x)
	flags           <- rep(0,r[2]-r[1]+1)
	flags[x-r[1]+1] <- 1
	r[1] + which(flags==0) - 1
}


jfk.tile.center     <-  polycover::pc.d2c(cbind(lon,lat),15)
lst <- list()
for (i in -1:1) {
	for (j in -1:1) {
		lst[[sprintf("c%d%d",i,j)]] <- as.integer(c(jfk.tile.center[,1]+i, jfk.tile.center[,2]+j, 15))
	}
}
m <- Reduce(function (a,b) rbind(a,b), lst)


paths <- polycover::pc.c2path(m)

jfk.tile.left       <-  polycover::pc.d2c(cbind(lon,lat),15)
jfk.tile.left       <-  polycover::pc.d2c(cbind(lon,lat),15)
jfk.tile.right      <-  polycover::pc.d2c(cbind(lon,lat),15)
jfk.tile.left       <-  polycover::pc.d2c(cbind(lon,lat),15)
jfk.tile.right      <-  polycover::pc.d2c(cbind(lon,lat),15)
jfk.tile.left       <-  polycover::pc.d2c(cbind(lon,lat),15)
jfk.tile.right      <-  polycover::pc.d2c(cbind(lon,lat),15)
jfk.tile.left       <-  polycover::pc.d2c(cbind(lon,lat),15)
jfk.tile.right      <-  polycover::pc.d2c(cbind(lon,lat),15)
jfk.tile.right[,1]  <-  as.integer(jfk.tile.right[,1] + 1)

polycover::pc.c2path(jfk.tile.left)
polycover::pc.c2path(jfk.tile.right)

base[1]  <- base[1] + 1
base2[1] <- base2[1]
query <- sprintf("nov15.b(\"pickup_location\",tile2d(%d,%d,%d)).b(\"pickup_time\",timeseries(\"2015-11-01T00:00:00-05:00\",86400,31,86400));",base[3],base[1],base[2])
t <- nanocube::nanocube.query(engine, query)
print(t)
2^base[3] - 1 - base[2]


polycover::pc.d2path(cbind(lon,lat),15)
