#!/bin/sh

# must be aligned with the installation directory chosen in
# clients/examples/python
testpath="$TSTSRCBASE/clients/examples/python"
# ignore PYTHONPATH from Mtest, it is hardcoded to the dirs for the
# Python that runs Mtest (currently always Python 2)
PYTHONPATH=${PYTHON2PATH}
export PYTHONPATH

Mlog -x "${PYTHON2} ${testpath}/sqlsample.py $MAPIPORT $TSTDB $MAPIHOST"
