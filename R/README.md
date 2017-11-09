# Example

```
require(nanocube)

# load nanocube with two data
engine <- nanocube::nanocube.load("crimes=chicago-crimes-10k-sample.nanocube"))
nanocube::nanocube.messages(engine)

# query crimes by count
crimes <- nanocube::nanocube.query(engine,'crimes.b("type",dive(1));')

#
print(crimes)

```

# Install

## Linux

```
cd $NANOCUBE/R
sudo make linux
# linux: clean all
# 	rsync -av ../src nanocube/src/.
# 	R CMD build nanocube
# 	R CMD INSTALL --no-multiarch nanocube_0.0.1.tar.gz
```

# Here are the steps I followed:

1. Installed RTools
2. Right-click on cmder.exe to Run As Administrator
3. `R CMD build nanocube_0.0.1.tar.gz`
4. `R --arch=x86_64 CMD check --no-multiarch nanocube_0.0.1.tar.gz`
5. `R CMD INSTALL --build nanocube_0.0.1.tar.gz`
6. `R CMD CHECK nanocube_0.0.1.tar.gz`
