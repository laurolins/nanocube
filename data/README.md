# Notes

## Examples

The table `crime50k.csv` in the older version (< v3) had its original field names
'Date' and 'Primary Type' modified to 'time' and 'crime'. Now (version >=v4) we
preserve the original header and keep it in a compressed .gz format.

```
#
# to create NC for our standard example `crime50k.dmp` extracted from
#
#     https://data.cityofchicago.org/api/views/ijzp-q8t2/rows.csv?accessType=DOWNLOAD
#
nanocube <(crime50k.csv.gz) crime50k.map crime50k.nanocube
#
# or equivalently
#
zcat crime50k.csv.gz | nanocube -stdin crime50k.map crime50k.nanocube
```
