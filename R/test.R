# print(nanocube::messages(engine))
# print(nanocube::query(engine, "taxi;"))
# print(nanocube::messages(engine))
require(nanocube)
engine <- nanocube::load(c(
"nypd=w:/compressed_nanocube/data/nypd_20161013.nanocube",
"taxi=w:/compressed_nanocube/data/taxi_1M.nanocube"))
x <- nanocube::query(engine, "nypd.b(\"time\",dive(10));")
# print(nanocube::messages(engine))
# print(attributes(x));
# print(dim(x));
print(x);
y <- nanocube::query(engine, "taxi.b(\"pickup_location\",dive(10));")
print(y);

#
# 33T location
# 40.716594, -74.006081
#
# The nanocube tile for this lat/lon is
#
