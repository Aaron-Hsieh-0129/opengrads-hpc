#
# Tests an opengrads bundle.
#

HERE := $(shell pwd)

export PYTHONPATH = $(HERE)/lib
export GASCRP = $(HERE)
export GADDIR = $(HERE)/../cola/data
export GAUDPT = $(HERE)/../bin/gex/udpt
export LD_LIBRARY_PATH = $(HERE)/../bin/gex

check:
	@./grads_tests.py -d "$(HERE)/data/" -b "$(HERE)/../bin/"

