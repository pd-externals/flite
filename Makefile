# Makefile to build class 'helloworld' for Pure Data.
# Needs Makefile.pdlibbuilder as helper makefile for platform-dependent build
# settings and rules.

# library name
lib.name = flite

# input source file (class name == source file basename)
class.sources = flite.c

# all extra files to be included in binary distribution of the library
datafiles = \
	flite-help.pd \
	flite-numbers.pd flite-test2.pd flite-test.pd \
	README.md

# VOICE can be one of {kal16, kal, awb, rms, slt}
VOICE=kal16

#alldebug: CPPFLAGS += -DFLITE_DEBUG=1

cflags = -DVERSION='"$(lib.version)"' -DVOICE=$(VOICE)
ldlibs = -lflite_cmu_us_$(VOICE) -lflite_cmulex -lflite -lm

# include Makefile.pdlibbuilder from submodule directory 'pd-lib-builder'
PDLIBBUILDER_DIR=pd-lib-builder/
include $(PDLIBBUILDER_DIR)/Makefile.pdlibbuilder
