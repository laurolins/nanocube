# 107897 locates r=100m k=5     one thread
llins@nano4:~/code/compressed_nanocube/tests/roadsnap$ time ./roadsnap-test.py
time to prepare 0.54s num locates 107916
time to query 4.13s

real    0m4.730s
user    0m0.679s
sys     0m0.179s
llins@nano4:~/code/compressed_nanocube/tests/roadsnap$ less /tmp/result.txt

# 107897 locates r=10km k=5     one thread
llins@nano4:~/code/compressed_nanocube/tests/roadsnap$ time ./roadsnap-test.py
time to prepare 0.51s num locates 107897
time to query 210.38s

real    3m30.948s
user    0m0.724s
sys     0m0.184s
llins@nano4:~/code/compressed_nanocube/tests/roadsnap$

