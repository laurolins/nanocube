#
# path length
#
path_length     = 0:25
bits_per_label  = 1:2
children_length = 0:4
bits_per_child  = 64 # pointer

t = expand.grid(labelbits    = bits_per_label,
                pathlen      = path_length,
                childrenbits = bits_per_child,
                childrenlen  = children_length)
t = data.frame(t, pathbytes = as.integer((t$labelbits * t$pathlen + 7)/8))
t = data.frame(t, pathcap  =as.integer((t$pathbytes * 8) / t$labelbits))
t = data.frame(t, childrenbytes=as.integer((t$childrenbits * t$childrenlen + 7)/8))
t = data.frame(t, childrencap=as.integer((t$childrenbytes * 8) / t$childrenbits))
t = t[,c(1,3,2,5,6,4,7,8)]

# this gives us 24 classes of length -> capacity
# length_to_capacity = cumsum(2^c(rep(0,4), rep(1,4), rep(2,5), rep(3,4), rep(4,4), rep(5,4)))
# cut(1:256,c(0,length_to_capacity),labels=FALSE)
# length_to_capacity
# normalize_length = function(lengths) {
#  x =  length_to_capacity[cut(lengths,c(0,length_to_capacity),labels=FALSE)]
#  x[lengths==0] = 0
#  return(x)
#  # return(length_to_capacity[cut(lengths,c(0,length_to_capacity),labels=FALSE)])
# }

# 50% overhead
normalize_bytesize_two_x = function(lengths) {
  x = lengths
  x [lengths > 0] = 2^as.integer(ceiling(log(lengths[lengths>0],2)))
  return(x)
}

makedenser = function(x, levels = 0) {
  while (levels > 0) {
    x = sort(unique(c(x, as.integer( (tail(x,-1) + head(x,-1))/2 ) ) ) )
    levels = levels-1
  }
  x
}
byte_classes   = makedenser(2^(0:12),2)
length_classes = makedenser(2^(0:12),1)
normalize = function(x, classes) {
  x[x > 0] = classes[cut(x[x>0],c(0,classes),labels=FALSE)]
  return(x)
}


# extended path and children (to avoid moving on every insertion)
t = data.frame(t, pathextcap=normalize(t$pathlen, length_classes), childrenextcap=normalize(t$childrenlen, length_classes))
t = data.frame(t, 
               pathextbytes=as.integer((t$pathextcap*t$labelbits + 7)/8),
               childrenextbytes=as.integer((t$childrenextcap*t$childrenbits + 7)/8))
t = data.frame(t, headerbytes=rep(2,nrow(t)))
# headerbytes 
t = data.frame(t,totalbytes=t$pathextbytes + t$childrenextbytes + t$headerbytes)

# total bytes
t = data.frame(t,totalextbytes=normalize(t$totalbytes, byte_classes))

# overhead
t = data.frame(t, overhead=t$totalextbytes/t$totalbytes)

# deepoverhead
t = data.frame(t, deepoverhead=t$totalextbytes/(t$pathbytes + t$childrenbytes + t$headerbytes))

o = order(-t$deepoverhead)
head(t[o,],20)


quantile(t$overhead,probs=seq(0,1,0.1))


# binary representations
binarys<-function(i) {
  a<-2^(31:0)
  b<-2*a
  sapply(i,function(x) paste(as.integer((x %% b)>=a),collapse=""))
}

# 
# 
# 2^(1:10)
# sort(c(2^(0:14), 2^(1:14)+2^(0:13))) #, 2^(2:14) + 2^(1:13) + 2^(0:12)))
# sort(c(2^(0:14), 2^(1:14)+2^(0:13))) #, 2^(2:14) + 2^(1:13) + 2^(0:12)))
# 
# 2^(0:14)
# makedenser = function(x, levels = 0) {
#   r = x
#   while (levels > 0) {
#     r = sort(unique(c(r, as.integer( (tail(r,-1) + head(r,-1))/2 ) ) ) )
#     levels = levels-1
#   }
#   r
# }
# x = makedenser(2^(0:14),2)
# x
# length(x)
# 
# tail(x,-1) - head(x,-1)
# 
# 