ACLOCAL_AMFLAGS = -I m4 

##############################################################
#
# Subdirectories with Makefiles
#

SUBDIRS=src pytests

BINDISTFILES=COPYING INSTALL NEWS README

DATADISTFILES=data

DOCDISTFILES=doc

EXTRA_DIST= $(BINDISTFILES) $(DATADISTFILES) $(DOCDISTFILES)

CVS = @CVS@

CVSTAG = @CVSTAG@

# Indicate that these targets are not the names of files
.PHONY: nobin-dists all-dists snapshot-dist bin-dist snapshot-bin-dist \
	data-dist doc-dist

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

gex-check: binstall
	cd extensions; $(MAKE) check

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

snapshot: distdir
	mkdir -p $(distdir); \
        cp -pR pytests $(distdir);\
	rm -rf `find $(distdir) -name CVS`;\
	rm -rf `find $(distdir) -name '*.pyc'`;\
	date_str=`date '+%Y%m%d-%H%M'`; \
	$(AMTAR) chof - $(distdir) | GZIP=$(GZIP_ENV) gzip -c >$(distdir)-snapshot-$${date_str}.tar.gz; \
	$(am__remove_distdir)

bin-snapshot: all-am
	$(MAKE) $(AM_MAKE_FLAGS) prefix=$(prefix)/$(distdir) install-exec; \
	for file in $(BINDISTFILES) ; do \
	  cp -pR $$file $(distdir)/ ; \
	done; \
	rm -rf `find $(distdir) -name CVS`; \
	date_str=`date '+%Y%m%d-%H%M'`; \
	$(AMTAR) chof - $(distdir) | GZIP=$(GZIP_ENV) gzip -c >$(distdir)-bin-snapshot-$${date_str}.tar.gz; \
	$(am__remove_distdir)

#
# Bundle, gex installation targets
#

Documentation.html: Documentation.php
	php Documentation.php > Documentation.html

#
# Bundle installation under local dir
#
binstall: 
	@bundle/bundle_create.sh 

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
