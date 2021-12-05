bin_PROGRAMS = mpconv mpgen

AM_CXXFLAGS = @JSON_CFLAGS@ @TAGLIB_CFLAGS@

mpconv_SOURCES = mpconv.cc \
	MusicCrawler.cc \
	Track.cc \
	Utilities.cc

mpconv_LDADD = @TAGLIB_LIBS@ @JSON_LIBS@

mpgen_SOURCES = mpgen.cc \
	MusicCrawler.cc \
	Track.cc \
	Utilities.cc

mpgen_LDADD = @TAGLIB_LIBS@ @JSON_LIBS@
