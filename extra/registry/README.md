# Nanocube Registry

This process allows nanocube engine processes to register their URLs
and make it easier for nanocube clients to connect to their engines of
interest.


# Message format

An UTF-8 line based format of key-value pairs



<service>|<params>




Grammar:



    DOCUMENT  = [KEYVALUE (NEWLINE KEYVALUE)*]
    SEPARATOR = '\n'
    CONTENT   = 

Requests

    subscribe|<alias>|[<key>:<value>(;<key>:<value>)*]
    unsubscribe|<alias>|[<key>:<value>(;<key>:<value>)*]
    info|<alias>|<key>
    
Response

    
    





In json that is what we would have

{
    "service": "online",
    "user": "llins",
    "url": "http://nano1:854",
    "alias": "latency-august",
    "version": "0.1.4",
    "description":"This is the August dataset of nanocube bla bla bla\nThis is very interesting..."
    }









## API

### online '|' <hostname> '|' <port> '|' <alias> '|' <version> ['|' <description>]

Registers a new server.

### lookup/<alias>

Retrieve from the alias the host and port of the nanocube server