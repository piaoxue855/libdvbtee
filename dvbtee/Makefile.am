AM_CXXFLAGS = -I../libdvbtee -I../libdvbtee_server -D__STDC_FORMAT_MACROS
AM_CXXFLAGS += -I../libdvbtee/decode -I../libdvbtee/decode/table -I../libdvbtee/decode/descriptor

bin_PROGRAMS = dvbtee
dvbtee_SOURCES = dvbtee.cpp
dvbtee_LDADD = -L../libdvbtee -L../libdvbtee_server -ldvbtee -ldvbtee_server
#MacOSX needs libiconv
#dvbtee_LDADD += -L/opt/local/lib -liconv

if DVBPSIBUILD
AM_CPPFLAGS = -I../usr/include/
endif DVBPSIBUILD
