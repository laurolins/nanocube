# Notes

## Chicago Crimes Example

The table `crime50k.csv` in the older version (< v3) had its original field names
'Date' and 'Primary Type' modified to 'time' and 'crime'. Now (version >=v4) we
preserve the original header and keep it in a compressed .gz format.

```shell
#
# to create NC for our standard example 'crime50k.dmp'
# (sample of 50k records extracted from
#      https://data.cityofchicago.org/api/views/ijzp-q8t2/rows.csv?accessType=DOWNLOAD)
#
nanocube <(zcat crime50k.csv.gz) crime50k.map crime50k.nanocube
#
# or equivalently
#
zcat crime50k.csv.gz | nanocube -stdin crime50k.map crime50k.nanocube
```

## Nanocube's Technical Paper Example

To illustrate the internals of a nanocube, we use the example in Figure 2 of the
original [Nanocubes technical paper](http://nanocubes.net/assets/pdf/nanocubes_paper.pdf).

```shell
nanocube paper_example.csv paper-example.map paper-example.nanocube
```






