#
# Top Makefile for building COLA GrADS, OpenGrADS extensions and assembling 
# an OpenGrADS Bundle.
#

prefix=/opt # where to install binaries

#
# Build core COLA binaries
#
cola-build: cola/Makefile
	@(cd cola; $(MAKE) )
	@cp etc/udpt bin/gex

cola-install cola-clean cola-distclean:
	(cd cola; $(MAKE) $(subst cola-,,$@))
	@cp etc/udpt bin/gex

cola/Makefile:
	@oga_configure

#
# OpenGRADS Extensions
#

gex-build:
	@(cd extensions; $(MAKE) --silent all)

gex-install: gex-build
	@(cd extensions; $(MAKE) --silent $(subst gex-,,$@))

gex-clean gex-distclean: 
	@(cd extensions; $(MAKE) --silent $(subst gex-,,$@))
#
# Bundle, gex installation targets
#

Documentation.html: Documentation.php
	php Documentation.php > Documentation.html

#
# Bundle installation under local dir
#
bundle-create:
	@bundle/bundle_create.sh 

#
# Bundle unit testing
#

bundle-check: bundle-create
	@(cd pytests; make check)

#
# Bundle installation under prefix
#
bundle-install: 
	@bundle/bundle_create.sh 
	@mkdir -p $(prefix)
	@rm -rf .opengrads
	@mv opengrads .opengrads
	@mv .opengrads/Contents $(prefix)/OpenGrADS
	@rm -rf .opengrads
	@echo "Make sure to put $(prefix)/OpenGrADS in your path"

clean:
	$(MAKE) cola-clean
	$(MAKE) gex-clean

distclean:
	$(MAKE) cola-distclean
	$(MAKE) gex-distclean
	/bin/rm -rf bin opengrads

#
# Just binaries and gex
#
full-install: install
	@mkdir -p $(prefix)/bin/gex
	@cd extensions; $(MAKE) $(AM_MAKE_FLAGS) bindir=$(prefix)/bin install

#
#                                J A V A
#                                -------

java-dist: all-am
	$(MAKE)
	(cd Java; $(MAKE) grads.jar)
	cp -p Java/grads.jar $(distdir).jar

java-clean:
	(cd Java; make clean)

java-distclean:
	(cd Java; make distclean)

reallyclean:
	$(MAKE) distclean
	$(MAKE) java-distclean




# ------------------------------- TO DO ---------------------------------------

dist-all: dist data-dist doc-dist

src-dist:
	@echo 
	@echo "Creating tarball with sources"
	@echo "-----------------------------"
	$(CVS) export -r $(CVSTAG) -kk -d $(distdir) Grads
	$(AMTAR) chof - $(distdir) | GZIP=$(GZIP_ENV) gzip -c >$(distdir)-bundle.tar.gz; \
	$(am__remove_distdir)


bin-dist: all-am
	$(MAKE) $(AM_MAKE_FLAGS) prefix=$(prefix)/$(distdir) install-exec; \
	for file in $(BINDISTFILES) ; do \
	  cp -pR $$file $(distdir)/ ; \
	done; \
	rm -rf `find $(distdir) -name CVS`; \
	$(AMTAR) chof - $(distdir) | GZIP=$(GZIP_ENV) gzip -c >$(distdir)-bin-$(host_triplet).tar.gz; \
	$(am__remove_distdir)

gex-dist: 
	mkdir -p $(distdir)/bin/gex
	cd extensions; $(MAKE) $(AM_MAKE_FLAGS) bindir=$(prefix)/$(distdir)/bin install
	for file in $(BINDISTFILES) ; do \
	  cp -pR extensions/$$file $(distdir)/$$file-gex ; \
	done; \
	$(AMTAR) chof - $(distdir) | GZIP=$(GZIP_ENV) gzip -c >$(distdir)-gex-$(host_triplet).tar.gz; \
	$(am__remove_distdir)

bundle-dist: 
	@bundle/bundle_create.sh $(distdir)
	$(AMTAR) cvfz $(distdir)-bundle-$(host_triplet).tar.gz $(distdir) 
	$(am__remove_distdir)

data-dist:
	mkdir -p $(distdir); \
	for file in $(DATADISTFILES) ; do \
	  cp -pR $$file $(distdir)/ ; \
	done; \
	rm -rf `find $(distdir) -name CVS`; \
	$(AMTAR) chof - $(distdir) | GZIP=$(GZIP_ENV) gzip -c >$(distdir)-data.tar.gz; \
	$(am__remove_distdir)

doc-dist:
	mkdir -p $(distdir); \
	for file in $(DOCDISTFILES) ; do \
	  cp -pR $$file $(distdir)/ ; \
	done; \
	rm -rf `find $(distdir) -name CVS`
	$(AMTAR) chof - $(distdir) | GZIP=$(GZIP_ENV) gzip -c >$(distdir)-doc.tar.gz; \
	$(am__remove_distdir)



