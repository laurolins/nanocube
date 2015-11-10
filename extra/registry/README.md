# Registry

A simple dictionary registry using nanomsg for communication.

## API

### insert '|' <alias> ( '|' <key> '|' <value> )*

Registers a new dictionary into the registry.

Results in `0|OK` in case of success and in `<error_code>|<error_message>` in case of error.

### get 

Retrieve all aliases and corresponding dictionaries from the registry

Results in `0(|<alias_1>|<key_1_1>|<value_1_1>|...|<key_1_n>|<value_1_n><new_line>)*` in case of success and in `<error_code>|<error_message>` in case of error.

### get '|' <alias>

Retrieve alias dictionary

Results in `0|<alias_1>|<key_1_1>|<value_1_1>|...|<key_1_n>|<value_1_n><new_line>` in case of success and in `<error_code>|<error_message>` in case of error.

### remove '|' <alias>

Removes dictionary from registry

Results in `0|OK` in case of success and in `<error_code>|<error_message>` in case of error.

