# OBD II YAML format
In order to describe OBD II data, we use the YAML format documented here. The
purpose of the format is to easily describe (in human-readable format) the
meaning and interpretation of OBD II PIDs while retaining the ability for a
computer to efficiently process the data. For instance, exclusively using JSON
would be human-readable but inefficient for a computer. Even a self-describing
format like msgpack is much more verbose than a packed format with accompanying
schema. Given the YAML description here, one can both parse OBD II data at
runtime -- translating bitpacked CAN frames into values -- and interpret the
packed data after being parsed (unit, string description of PID, etc.)

## Schema
The exact schema is described in `data/schema.yml` and is evaluated by the
`schema-check` tool in `bin`. By making the schema be another YAML file, the
schema documentation and the actual data files will always stay in-sync, and
schemas can be checked at compile-time (via the `schema-check` tool) rather
than at runtime.

There is also a ninja target `schema-check` that will check all files in
`data` besides the schema file itself.

## Examples
Example OBD II files can be found in the `data` directory.
