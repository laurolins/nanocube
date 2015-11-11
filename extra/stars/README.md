# Github project stars and timestamps

How to get the list of users that starred a project and the timestamp
of when they did it.

```
curl -o x1 "https://api.github.com/repos/laurolins/nanocube/stargazers?page=1&per_page=300" -H "Accept: application/vnd.github.v3.star+json"
curl -o x2 "https://api.github.com/repos/laurolins/nanocube/stargazers?page=2&per_page=300" -H "Accept: application/vnd.github.v3.star+json"
curl -o x3 "https://api.github.com/repos/laurolins/nanocube/stargazers?page=3&per_page=300" -H "Accept: application/vnd.github.v3.star+json"
curl -o x4 "https://api.github.com/repos/laurolins/nanocube/stargazers?page=4&per_page=300" -H "Accept: application/vnd.github.v3.star+json"
curl -o x5 "https://api.github.com/repos/laurolins/nanocube/stargazers?page=5&per_page=300" -H "Accept: application/vnd.github.v3.star+json"
curl -o x6 "https://api.github.com/repos/laurolins/nanocube/stargazers?page=6&per_page=300" -H "Accept: application/vnd.github.v3.star+json"
cat x1 x2 x3 x4 x5 x6 > all
# filter all to prepare a table like this one
# timestamp|user
# 2013-08-28T19:57:44Z|login1
# 2013-08-28T21:31:16Z|login2
# 2013-08-29T12:42:31Z|login3
# 2013-08-29T12:57:53Z|login4
# 2013-08-29T13:06:45Z|login5
# ...
#
# use a script like the plot.R to create a plot
#
```

Here is a plot up to 2015-11-11:

![image](./nanocube-stars-evolution-2015-11-11.png?raw=true)

