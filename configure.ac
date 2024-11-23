#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
AC_PREREQ([2.69])

m4_define([ver_major], [1])
m4_define([ver_minor], [0])
m4_define([ver_release], [2])
# Set build number to commit count only if ".git" folder is present
m4_define([ver_build], m4_if(m4_esyscmd_s([ls -1 -d ".git" 2>&1]),.git,[m4_esyscmd_s([git rev-list --count HEAD])],[0]))

m4_define([pngpal2raw_version],
      [ver_major.ver_minor.ver_release.ver_build])

AC_INIT([PNG and PAL to RAW files converter], [pngpal2raw_version], [mefistotelis@gmail.com], [pngpal2raw])

AC_CONFIG_SRCDIR([src/pngpal2raw.cpp])
AC_CONFIG_HEADERS([src/config.h])

AC_CANONICAL_HOST
AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE([foreign subdir-objects])
ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_DEFINE([VER_MAJOR], [ver_major], [Major version number])
AC_DEFINE([VER_MINOR], [ver_minor], [Minor version number])
AC_DEFINE([VER_RELEASE], [ver_release], [Release number])
AC_DEFINE([VER_BUILD], [ver_build], [Build number])

# Default install dir
#AC_PREFIX_DEFAULT(/usr/local)

# Set the proper prefix
AS_IF([test "$prefix" = "NONE"], [
  prefix="$ac_default_prefix"
])
AS_IF([test "$exec_prefix" = "NONE"], [
  exec_prefix="$prefix"
])

# Checks for programs.

AC_PROG_CC
AC_PROG_CXX
AC_CHECK_PROGS(WINDRES, $WINDRES "${target_alias}-windres" "${target}-windres", "windres", :)
AC_PROG_SED # `sed` is used in Makefile for packaging shared libraries on Windows

have_posix=yes
have_windows=no
have_darwin=no

case "$host" in
  *-*-mingw*)
    have_windows=yes
    have_posix=no
    ;;
  *-*-darwin*)
    have_darwin=yes
    ;;
esac

AM_CONDITIONAL([TARGET_WINDOWS], [test "$have_windows" = "yes"])

# On Windows, we can optionally include resources in the executable.
AM_CONDITIONAL([HAS_WINDRES], [test -n "${WINDRES}" -a "$have_windows" = "yes"])

# Checks for libraries.

PKG_CHECK_MODULES([PNG], [libpng], [], [
  AC_MSG_ERROR([libpng not found])
])

# Flags

CFLAGS="$CFLAGS -Wall -Wextra -Wno-unused-parameter"
CFLAGS="$CFLAGS -include \"\$(top_builddir)/src/config.h\""

# Add configuration of libraries to flags; the order of libraries is important

CFLAGS="$CFLAGS $PNG_CFLAGS"
LIBS="$LIBS $PNG_LIBS"


AC_SUBST([WINDRES])

# Prepare makefiles from `.in` templates
AC_CONFIG_FILES([Makefile])

# Prepare files for Windows installer
AC_CONFIG_LINKS([
  res/pngpal2raw.ico:res/pngpal2raw.ico
])

# Done - generate and execute `config.status`
AC_OUTPUT
