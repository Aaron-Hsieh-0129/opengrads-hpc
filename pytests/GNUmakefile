#
# Tests an opengrads bundle.
#

HERE := $(shell pwd)

PYTHONPATH := $(HERE)/lib
GADSET  := $(shell cd $(HERE)/../pytests/data; pwd)# where test datsets are
GADDIR  := $(shell cd $(HERE)/../cola/data; pwd)
GABDIR  := $(shell cd $(HERE)/../bin/; pwd)# where binaries for testing are
GABnDIR  := $(shell cd $(HERE)/../opengrads/Contents/; pwd)# where binaries for testing are
GA2UDXT := $(GABDIR)/gex/udxt
GAUDPT  := $(GABDIR)/gex/udpt
GASCRP  := $(GABDIR)/gex/
LD_LIBRARY_PATH := $(shell cd $(HERE)/../bin/gex; pwd):$(SUPPLIBS)/lib:$(LD_LIBRARY_PATH)
DYLD_LIBRARY_PATH := $(SUPPLIBS)/lib:$(shell cd $(HERE)/../bin/gex; pwd):$(SUPPLIBS)/lib:$(DYLD_LIBRARY_PATH)

check:
	/usr/bin/env PYTHONPATH=$(PYTHONPATH) \
	             GADSET=$(GADSET) \
	             GADDIR=$(GADDIR) \
	             GABDIR=$(GABDIR) \
	             GA2UDXT=$(GA2UDXT) \
	             GAUDPT=$(GAUDPT) \
	             GASCRP=$(GASCRP) \
	             LD_LIBRARY_PATH=$(LD_LIBRARY_PATH) \
	             DYLD_LIBRARY_PATH=$(DYLD_LIBRARY_PATH) \
                    ./grads_tests.py -d $(GADSET)/ -b $(GABDIR)/

bundle-check:
	/usr/bin/env PYTHONPATH=$(PYTHONPATH) \
	             GASCRP=$(GASCRP) \
                    ./grads_tests.py -d $(GADSET)/ -b $(GABnDIR)/

