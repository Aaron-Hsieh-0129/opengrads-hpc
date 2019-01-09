#########################################
#  Pedro T.H. Tsai
#  Feb 15, 2009
#
#  special macros:
#  $@
#    Full name of the current target. 
#  $?
#    A list of files for current dependency which are out-of-date. 
#  $<
#    The source file of the current (single) dependency. 
#########################################

#
# compiler options
#
JAVA = java
JC = javac
JAR = jar

JCFLAGS = -g -deprecation

#
# List of targets to build.
#

BINS += DebugInfo.class   ConfigInfo.class 

BINS += MsgQ.class MsgQDataBufIntface.class MsgQIntDataBuf.class \
          MsgQStrBufDataBuf.class MsgQStringDataBuf.class 

BINS += SockIPC.class

BINS += Timer.class TimerNfyIntface.class 

BINS += JGradsCmd.class JGradsUserInput.class 

BINS += JGrads.class

BINS += GVMclient.class JGfxRender.class 

BINS += gxJ.class 

BINS += IntBuf.class CrcBufDataObjIntface.class CrcBufList.class Lock.class 

BINS += GVMServer.class GVMGfxDispatch.class GVM.class  

SRCS = $(BINS:.class=.java)

###########################################################
 
all : $(BINS)

#$(BINS) : %.class : %.java
#	$(JC) $(JCFLAGS) $<

#################################################
info:
	@echo list of source files: $(JAVA_SRC)

run :
	@echo "Start JGrads client...."
	java -cp ./grads.jar GVMclient

svc : grads.jar
	@echo "Start GradsVM Server ...."
	java -cp ./grads.jar GVMServer

clean :
	/bin/rm -rf *~ *.bak *.class 

distclean :
	/bin/rm -rf *~ *.bak *.class *.jar

tar :
	@echo "tar up files ...."
	tar cvf ./gvmcode.tar $(JAVA_SRC) GNUmakefile

#                  --------------------------------
#                        NestedVM/Java Targets
#                  --------------------------------

NESTEDVM = grads.class bufrscan.class gribscan.class gribmap.class gxeps.class \
        gxps.class stnmap.class wgrib.class grib2scan.class

#NESTEDVM = grads.class 

%.class : %.java
	$(JC) $(JCFLAGS) $*.java

%.class : ../src/%
	$(JAVA) -Xms256m -Xmx512m org.ibex.nestedvm.Compiler \
              -o unixRuntime -outfile $*.class $* $<

grads.jar: $(BINS) $(NESTEDVM)
	cp $(NESTEDVMDIR)/java/nestedvm.jar grads.jar
	$(JAR) -J-Dmode=console uvmf mainClass grads.jar \
               *.class $(SRCS)
