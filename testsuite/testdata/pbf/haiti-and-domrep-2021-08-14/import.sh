#!/bin/bash
# Import Haiti and Dom. Rep. PBF for testing

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

# To regenerate the DB:
dropdb --encoding=UTF8 pgsql_pbf_import_test


createdb --encoding=UTF8 pgsql_pbf_import_test

psql pgsql_pbf_import_test --command='CREATE SCHEMA osm2pgsql_pgsql;'
psql pgsql_pbf_import_test --command='CREATE SCHEMA osm2pgsql_pgsql_middle;'

psql pgsql_pbf_import_test --command='CREATE EXTENSION postgis;'
psql pgsql_pbf_import_test --command='CREATE EXTENSION hstore;'


# --slim will also create indexes on the OSM id (id) column

osm2pgsql \
    --slim \
    -d pgsql_pbf_import_test \
    --output=pgsql \
    -l \
    --hstore-all \
    --output-pgsql-schema=osm2pgsql_pgsql \
    --middle-schema=osm2pgsql_pgsql_middle \
    --extra-attributes \
    --create \
    -r pbf \
    ${SCRIPT_DIR}/haiti-and-domrep-latest.osm.pbf
    
    
