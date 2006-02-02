# File: ./Makefile.am
# Package: flite
# Description:
#   + top-level automake file
#
# Process this file with Automake to create Makefile.in.
#-----------------------------------------------------------------------

#-----------------------------------------------------------------------
# Options & Subdirectories
#-----------------------------------------------------------------------

## --- automake options
#AUTOMAKE_OPTIONS = foreign dist-bzip2 dist-zip
AUTOMAKE_OPTIONS = foreign

## --- recursion subdirectories
SUBDIRS = config

## --- pseudo-deps for '.SUFFIXES'
SUFFIXES = .pod .txt

#-----------------------------------------------------------------------
# Flags and variables
#-----------------------------------------------------------------------
PD_DIR = @PD_DIR@
PD_INC = @PD_INC@
PD_DOCDIR = @PD_DOC@
PD_EXTDIR = @PD_EXTDIR@
EXT    = @EXT@

#-----------------------------------------------------------------------
# pd externals (hacked 'libexec_*')
#-----------------------------------------------------------------------
EXEEXT=.@EXT@
libexecdir = @PD_EXTDIR@

## --- externals
libexec_PROGRAMS = flite

## --- documentation
datadir = @PD_DOCDIR@
data_DATA = flite-help.pd

#-----------------------------------------------------------------------
# sources
#-----------------------------------------------------------------------
flite_SOURCES = flite.c

#-----------------------------------------------------------------------
# external compilation : flags
#-----------------------------------------------------------------------
DEFS   = @DEFS@
AFLAGS = @AFLAGS@
DFLAGS = @DFLAGS@
LFLAGS = @LFLAGS@
WFLAGS = @WFLAGS@
OFLAGS = @OFLAGS@

AM_CPPFLAGS = $(DEFS) $(IFLAGS) $(DFLAGS)
AM_CFLAGS   = $(OFLAGS) $(WFLAGS) $(AFLAGS)

flite_LDFLAGS = $(LFLAGS)
flite_LDADD   = $(FLITE_VOICELIBS) $(FLITE_LANGLIBS) $(FLITE_LEXLIBS) $(FLITE_LIBS)

#-----------------------------------------------------------------------
# Variables: cleanup
#-----------------------------------------------------------------------
## --- mostlyclean: built by 'make' & commonly rebuilt
#MOSTLYCLEANFILES =

## --- clean: built by 'make'
#CLEANFILES =

## --- distclean: built by 'configure'
DISTCLEANFILES = \
	config.log	\
	config.cache	\
	config.status

## -- maintainerclean: built by maintainer / by hand
MAINTAINERCLEANFILES = *~ \
	$(PODS:.pod=.txt) \
	Makefile Makefile.in \
	aclocal.m4 \
	configure \
	install-sh \
	stamp-h.in \
	config.h.in

maintainer-clean-local:
	rm -rf autom4te.cache

#CVSCLEAN_SUBDIRS = $(SUBDIRS)

#CVSCLEANFILES = Makefile.in Makefile


#-----------------------------------------------------------------------
# Additional Variables & Rules: PODS
#-----------------------------------------------------------------------
PODS = README.pod

.pod.txt:
	pod2text $< $@


#-----------------------------------------------------------------------
# Variables: distribution
#-----------------------------------------------------------------------

## --- extra distribution files
EXTRA_DIST = $(PODS:.pod=.txt) $(data_DATA) autogen.sh configure \
	COPYING flite-1.1-noaudio.patch

## --- recursion subdirectories for 'make dist'
DIST_SUBDIRS = $(SUBDIRS)

## --- dist-hook: when another 'Makefile.am' is overkill
#DISTHOOK_DIRS = foo
#DISTHOOK_FILES = foo/bar.txt foo/baz.txt
#dist-hook:
#	for d in $(DISTHOOK_DIRS); do\
#	  mkdir -p $(distdir)/$$d ;\
#	done
#	for f in $(DISTHOOK_FILES); do\
#	  cp -p $(srcdir)/$$f $(distdir)/$$f ;\
#	done

#dist-bz2: dist-bzip2 ;


#-----------------------------------------------------------------------
# Rules: cleanup
#-----------------------------------------------------------------------
.PHONY: cvsclean cvsclean-hook

cvsclean: maintainer-clean ;

