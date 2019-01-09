#! /bin/sh

if [ "z$srcdir" = "z" ]; then
        srcdir=.
fi

export PYTHONPATH="$srcdir/lib"
export GASCRP=$srcdir
export GADDIR=`pwd`"/$srcdir/../data"
"$srcdir/grads_tests.py" -d "$srcdir/data/" -b ../src/
