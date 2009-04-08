# Tests

AUTOMAKE_OPTIONS = foreign dejagnu

DEJATOOL=fonc

AM_CPPFLAGS = -I$(top_srcdir) $(BES_CPPFLAGS)
AM_LDADD = $(BES_DAP_LIBS)

# These are not used by automake but are often useful for certain types of
# debugging. Set CXXFLAGS to this in the nightly build using export ...
CXXFLAGS_DEBUG = -g3 -O0 -fno-defer-pop -Wall -W -Wcast-align -Werror
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

# This determines what gets built by make check
noinst_PROGRAMS = $(UNIT_TESTS)

nodist_include_HEADERS = test_config.h


DISTCLEANFILES = test_config.h

CLEANFILES = *.dbg *.log *.sum site.exp *.nc

EXTRA_DIST = baselines fonc.tests common_tests.exp test_config.h.in bes.conf

############################################################################
# Unit Tests
#

UNIT_TESTS = simpleT00 simpleT01 simpleT02	\
	arrayT					\
	structT00 structT01 structT02		\
	attrT gridT namesT seqT

SRCS = ../FONcTransform.cc ../FONcTransmitter.cc
simpleT00_SOURCES = simpleT00.cc $(SRCS)
simpleT00_LDADD = $(AM_LDADD)

simpleT01_SOURCES = simpleT01.cc $(SRCS)
simpleT01_LDADD = $(AM_LDADD)

simpleT02_SOURCES = simpleT02.cc $(SRCS)
simpleT02_LDADD = $(AM_LDADD)

arrayT_SOURCES = arrayT.cc $(SRCS)
arrayT_LDADD = $(AM_LDADD)

structT00_SOURCES = structT00.cc $(SRCS)
structT00_LDADD = $(AM_LDADD)

structT01_SOURCES = structT01.cc $(SRCS)
structT01_LDADD = $(AM_LDADD)

structT02_SOURCES = structT02.cc $(SRCS)
structT02_LDADD = $(AM_LDADD)

attrT_SOURCES = attrT.cc $(SRCS)
attrT_LDADD = $(AM_LDADD)

gridT_SOURCES = gridT.cc $(SRCS)
gridT_LDADD = $(AM_LDADD)

namesT_SOURCES = namesT.cc $(SRCS)
namesT_LDADD = $(AM_LDADD)

seqT_SOURCES = seqT.cc $(SRCS)
seqT_LDADD = $(AM_LDADD)

