require(polycover)
require(nanocube)
require(RColorBrewer)
library(ggplot2)

# some spatial selections as tiles
regions.names <- c("jfk", "lga", "empirestate", "any")
regions <- sapply(regions.names, function (base) {
			fname <- sprintf(sprintf("%s.tiles",base))
			fcontent <-readChar(fname, file.info(fname)$size)
			print(fcontent)
			strsplit(fcontent, "\n")[1]})
names(regions) <- regions.names

#
folder  <- "/home/llins/data/taxi/run-2/2015"
command <- sprintf("ls -1 %s | grep nanocube$ | grep 004 | tr '\n' ','",folder)
files   <- system(command,intern=TRUE)
files   <- unlist(strsplit(files,","))
files   <- sapply(files,function(s) sprintf("@%s/%s", folder, s))
src     <- paste0(files,sep="",collapse=",")
t <- proc.time()
engine <- nanocube::nanocube.load(sprintf("taxi=%s",src))
print(proc.time() - t)
nanocube::nanocube.messages(engine)
data <- nanocube::nanocube.query(engine,"taxi;")
#  user  system elapsed
# 0.046  47.749 205.119

# prepare path binary representation into tile coordinates
tx_tiles <- function(location.bin, tilelevel, tileres)
{
	inputres   <- tilelevel + tileres
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

tx_query <- function(engine, dataset, dimension, lat, lon, tilelevel, tileres, tilexrange, tileyrange)
{
	print(tilelevel)
	base            <- polycover::pc.d2c(cbind(lon,lat),tilelevel)
	xx              <- base[,1] + tilexrange
	yy              <- base[,2] + tileyrange
	tiles           <- cbind(expand.grid(xx,yy),tilelevel)
	dst <- "jfk"
	dst.constraint  <- sprintf('.b("dropoff_location",mask("%s"))',regions[dst])
	time.constraint <- sprintf('.b("pickup_time", timeseriesagg("2015-12-23T16:00-05:00",3600,1,3600))')
	base.query      <- sprintf("%s%s%s", dataset, dst.constraint, time.constraint)
	# print(base.query)
	query.result    <- lapply(1:nrow(tiles),
	function(i) {
		src.constraint <- sprintf('.b("%s",dive(tile2d(%d,%d,%d),%d))',dimension,
					  tiles[i,3],tiles[i,1],tiles[i,2],tileres)
		# print(src.constraint)
		query <- sprintf("%s%s;", base.query, src.constraint)
		# print(query)
		nanocube::nanocube.query(engine,query)
	})
	# merge all query results into a signle data frame
	query.result     <- Reduce(function(a,b) { rbind(a,b)}, query.result)
	location.bin     <- query.result[,1]
	values           <- query.result[,2:ncol(query.result)]
	augmented.result <- tx_tiles(location.bin, tilelevel, tileres)
	augmented.result$records <- cbind(augmented.result$records, values)
	# sort records by
	pi = order(augmented.result$records$ximg,augmented.result$records$yimg)
	augmented.result$records <- augmented.result$records[pi,]
	augmented.result
}

place <- function(name, lat, lon, tilexrange, tileyrange, tilelevel, tileres)
{
	list(name=name, lat=lat, lon=lon,
	     tilexrange=tilexrange, tileyrange=tileyrange,
	     tilelevel=tilelevel, tileres=tileres)
}
# http://a.tile.openstreetmap.org/12/1205/1540.png
# lat             <-  40.716594
# lon             <- -74.006081
# tilelevel       <-  14
# tilexrange      <- -1:1
# tileyrange      <- -1:1
# tileres         <-  6
jfk <- place("jfk", 40.635966, -73.778760, -3:0,  0:4, 12, 5)
t33 <- place("t33", 40.716594, -74.006081, -1:1, -1:1, 14, 6)

query.place <- function(place)
{
	tx_query(engine, "taxi", "pickup_location",
		 place$lat, place$lon, place$tilelevel,
		 place$tileres, place$tilexrange, place$tileyrange)
}

target <- jfk

t <- proc.time()
result <- query.place(target)
print(proc.time() - t)

records <- result$records
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
records$ximg = (records$xtile - min.xtile) * 2^target$tileres + records$xpixel
records$yimg = (records$ytile - min.ytile) * 2^target$tileres + records$ypixel

selection       <- records$count > 0
records         <- records[selection,]
# values          <- as.integer(records$count > 10)
ex     <- records$duration/records$count
ex2    <- records$duration2/records$count + 1e-5
stddev <- sqrt(ex2 - ex^2)
values <- stddev

#fare/records$count
col=gray(0.3)
pdf("/tmp/avg-duration.pdf",width=6,height=6)
v <- ggplot(records, aes(ximg, yimg, colour=ex))
v + geom_point(size=0.5) + scale_colour_distiller(palette="Spectral") + coord_fixed(ratio = 1) + theme(panel.background=element_rect(fill=col,colour=col),panel.grid.major=element_line(colour=col),panel.grid.minor=element_line(colour=col))
dev.off()

+ scale_colour_brewer(palette = "Set1")
v +

# Count src-dst count
tx_tile_src_dst <- function(engine, dataset, tile.src, tile.dst)
{
	query <- sprintf("%s
			 .b(\"pickup_location\",tile2d(%d,%d,%d))
			 .b(\"dropoff_location\",tile2d(%d,%d,%d));",
			 dataset,
			 tile.src[1], tile.src[2], tile.src[3],
			 tile.dst[1], tile.dst[2], tile.dst[3])
	# print(query)
	nanocube::nanocube.query(engine, query)
}

#
tiles        <- unique(cbind(xtile=records$xtile, ytile=records$ytile))
m            <- nrow(tiles)
combinations <- expand.grid(1:m,1:m)
aux <- lapply(1:nrow(combinations),
       function(row) {
	i        <- combinations[row,1]
	j        <- combinations[row,2]
	src.tile <- c(tilelevel, tiles[i,1], tiles[i,2])
	dst.tile <- c(tilelevel, tiles[j,1], tiles[j,2])
	count    <- tx_tile_src_dst(engine, "taxi", src.tile, dst.tile)$count
	# print(src.tile)
	# print(dst.tile)
	# print(count)
	if (length(count) > 0)
		c(src.tile, dst.tile, count)
	else
		NA
})
T <- Reduce(function(a,b) { rbind(a,b)}, aux[!is.na(aux)])
o <- order(-T[,7])
T <- T[o,]
colnames(T) <- c("slev","sx","sy","dlev","dx","dy","count")
T <- cbind(T, per=100*T[,"count"]/sum(T[,"count"]))
T <- cbind(T, cum=cumsum(T[,"per"]))


head(T,100)
T <- matrix(aux, ncol=7, byrow=T)



T <- matrix(sapply(expand.grid(1:m,1:m), function(x) {
       	src.tile <- cbind(tiles[x[1],], tilelevel)
       	dst.tile <- cbind(tiles[x[2],], tilelevel)
	count    <- tx_tile_src_dst(engine, "taxi", src.tile, dst.tile)$count
	cbind(src.tile, dst.tile, count)
}), ncol=6)
T <- matrix(aux, ncol=7)

# penn station
# 40.749114, -73.992062
polycover::pc.d2c(cbind(-73.992062, 40.749114), 6)

print(tx_tile_src_dst(engine, "taxi", c(14,4824,10226), c(14,4824,10226)))


#
# for each pair of tiles in level 14 compute the number of trips
# lets get the most complex pairs and isolate them.
#





+ scale_colour_manual(values = c("red","blue", "green"))

scale_colour_brewer(palette = "Set1")


+ scale_colour_manual(values=c("#CC6666", "#9999CC", "#66CC99", "red"))
v + geom_raster(aes(fill=records$source)) + scale_fill_manual(values=c("#CC6666", "#9999CC", "#66CC99"))


# fare     <- img.it(data, depth, "fare")
# count    <- img.it(data, depth, "count")
# image(values, col = grey(seq(0, 1, length = 256)), axes = FALSE, asp=1)
# image(log(1+(fare/(1+count))), col = grey(seq(0, 1, length = 256)), axes = FALSE, asp=1)
# filled.contour(values, asp=1, frame.plot=F,  nlevels=5, col=brewer.pal(7,"YlOrRd"),
# col=c(1,2,3,4,5,6),
#               plot.axes = contour(values,add=T,nlevels=7))
# v + geom_contour()
# v + geom_density_2d()



















# inputres        <-  tilelevel + tileres
# base            <-  polycover::pc.d2c(cbind(lon,lat),tilelevel)
# xx              <-  base[,1] + xrange
# yy              <-  base[,2] + yrange
# tiles           <-  cbind(expand.grid(xx,yy),tilelevel)
# colnames(tiles) <-  c("x","y","level")

#
# results.hi  <- lapply(1:nrow(tiles),
# 	function(i) {
# 	nanocube::query(engine,
# 			sprintf("taxi
#       .b(\"pickup_location\",tile2d(16,19295,40896))
# 			.b(\"dropoff_location\",dive(tile2d(%d,%d,%d),8));",tiles[i,3],tiles[i,1],tiles[i,2]))
# 	})



# now create a matrix with the correct values
# data.hi   <- run(8)
# fare.hi   <- img.it(data.lo, "count")
# count.hi  <- img.it(data.lo, "distance")

