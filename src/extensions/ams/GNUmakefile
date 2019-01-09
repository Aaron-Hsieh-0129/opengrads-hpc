#
# GNU makefile for sample Dynamically Linked UDFs.
#

include ../supplibs.mk

XCFLAGS = -I$(SUPPLIBS)/include/pcre
XLDFLAGS = $(SUPPLIBS)/lib/libpcre.a

export XCFLAGS XLDFLAGS 
export PODS=env.pod path.pod

export GSFS=basename.gsf chdir.gsf dirname.gsf exists.gsf fexists.gsf dexists.gsf getatime.gsf \
            getctime.gsf getmtime.gsf glob.gsf isdir.gsf isfile.gsf \
            wordexp.gsf match.gsf sed.gsf \
            setenv.gsf getenv.gsf printenv.gsf abspath.gsf

all %:
	@$(MAKE) -f ../gex.mk $@ \
        CSRC="env.c path.c"\
        EXTRAS="pcrs.o"


