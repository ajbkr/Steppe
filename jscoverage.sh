#!/bin/bash
JSCOVERAGE_SERVER=/usr/local/bin/jscoverage-server

$JSCOVERAGE_SERVER --no-instrument=tests --no-instrument=js/qunit &
