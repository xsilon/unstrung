#!/bin/sh

SENDDIO=${SENDDIO-./senddio}

${SENDDIO} --fake -v -d ../INPUTS/basic-dio.txt | tee OUTPUT/senddio-test-02.raw | diff -B -w - senddio-test-02.out
