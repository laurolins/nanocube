# Nanocube Registry

This process allows nanocube engine processes to register their URLs
and make it easier for nanocube clients to connect to their engine of
interest.


## API

### online/<hostname>/<port>/<alias>/<version>/<code>

Registers a new server.

### lookup/<alias>

Retrieve from the alias the host and port of the nanocube server