
bin_PROGRAMS = pngpal2raw
bindir = $(prefix)/$(PACKAGE)

pngpal2raw_SOURCES = \
	bflibrary/src/gflicply.c \
	bflibrary/src/gflicrec.c \
	bflibrary/include/bffile.h \
	bflibrary/include/bfflic.h \
	bflibrary/include/bfmemut.h \
	bflibrary/include/bftypes.h \
	bflibrary/include/privbflog.h \
	src/ci_string.hpp \
	src/imagedata.cpp \
	src/imagedata.hpp \
	src/pngpal2raw.cpp \
	src/pngpal2raw_ver.h \
	src/prog_options.hpp \
	config.h

if HAS_WINDRES
pngpal2raw_RESRCS = pngpal2raw_stdres.res
else
pngpal2raw_RESRCS =
endif

if HAS_WINDRES
%.res: $(top_srcdir)/res/%.rc
	$(WINDRES) $(pngpal2raw_RCFLAGS) -i $< -J rc -o $@ -O coff
endif

doc_DATA = README.md
docdir = $(prefix)/$(PACKAGE)

EXTRA_DIST = GPL3 doc res

# Include dynamic libraries in the package
if TARGET_WINDOWS
install-exec-hook:
	mkdir -p "$(DESTDIR)${prefix}/$(PACKAGE)"
	## Find main executable dependencies
	$(eval lib_SHARED_INSTALL := $(shell objdump -p pngpal2raw$(EXEEXT) | \
		sed -n 's/\s*\(DLL Name:\|NEEDED\)\s*\(.*\)$$/\2/p' | \
		xargs -I {} find $(shell dirname $(shell which ${CXX})) -name '{}'))
	## Find sub-dependencies for the previous dependencies
	$(eval lib_SHARED_INSTALL += $(shell echo -n '$(lib_SHARED_INSTALL)' | \
        xargs -d ' ' -I {} objdump -p '{}' | \
		sed -n 's/\s*\(DLL Name:\|NEEDED\)\s*\(.*\)$$/\2/p' | \
		xargs -I {} find $(shell dirname $(shell which ${CXX})) -name '{}'))
	## Remove duplicates
	$(eval lib_SHARED_INSTALL := $(sort $(lib_SHARED_INSTALL)))
	cp $(lib_SHARED_INSTALL) $(DESTDIR)${prefix}/$(PACKAGE)
endif

install-data-hook:
	cd "$(DESTDIR)${prefix}/$(PACKAGE)"; \
	mv README.md pngpal2raw_readme.md
