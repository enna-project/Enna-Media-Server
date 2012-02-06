##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##
##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##
m4_define([v_maj], [0])
m4_define([v_min], [0])
m4_define([v_mic], [1])
m4_define([v_rev], m4_esyscmd([(svnversion "${SVN_REPO_PATH:-.}" | grep -v '\(export\|Unversioned directory\)' || echo 0) | awk -F : '{printf("%s\n", $1);}' | tr -d ' :MSP\n']))
m4_if(v_rev, [0], [m4_define([v_rev], m4_esyscmd([git log 2> /dev/null | (grep -m1 git-svn-id || echo 0) | sed -e 's/.*@\([0-9]*\).*/\1/' | tr -d '\n']))])
##--   When released, remove the dnl on the below line
dnl m4_undefine([v_rev])
##--   When doing snapshots - change soname. remove dnl on below line
dnl m4_define([relname], [ver-pre-svn-07])
dnl m4_define([v_rel], [-release relname])
##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##
m4_ifdef([v_rev], [m4_define([v_ver], [v_maj.v_min.v_mic.v_rev])], [m4_define([v_ver], [v_maj.v_min.v_mic])])
m4_define([lt_cur], m4_eval(v_maj + v_min))
m4_define([lt_rev], v_mic)
m4_define([lt_age], v_min)
##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##
##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##

AC_INIT([enna-media-server], [v_ver], [enlightenment-devel@lists.sourceforge.net])
AC_PREREQ([2.59])
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_MACRO_DIR([m4])

AC_CONFIG_HEADERS([config.h])
AH_TOP([
#ifndef EMS_CONFIG_H__
#define EMS_CONFIG_H__
])
AH_BOTTOM([
#endif /* EMS_CONFIG_H__ */
])

AM_INIT_AUTOMAKE([1.6 dist-bzip2])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_GNU_SOURCE
AC_SYS_LARGEFILE

AC_LIBTOOL_WIN32_DLL
define([AC_LIBTOOL_LANG_CXX_CONFIG], [:])dnl
define([AC_LIBTOOL_LANG_F77_CONFIG], [:])dnl
AC_PROG_LIBTOOL

##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##
##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##
m4_ifdef([v_rev], , [m4_define([v_rev], [0])])
m4_ifdef([v_rel], , [m4_define([v_rel], [])])
AC_DEFINE_UNQUOTED(VMAJ, [v_maj], [Major version])
AC_DEFINE_UNQUOTED(VMIN, [v_min], [Minor version])
AC_DEFINE_UNQUOTED(VMIC, [v_mic], [Micro version])
AC_DEFINE_UNQUOTED(VREV, [v_rev], [Revison])
version_info="lt_cur:lt_rev:lt_age"
release_info="v_rel"
AC_SUBST(version_info)
AC_SUBST(release_info)
##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##
##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##
VMAJ=v_maj
AC_SUBST(VMAJ)


### Needed information

AC_CANONICAL_BUILD
AC_CANONICAL_HOST

requirement_ems=""

case "$host_os" in
   mingw32ce*)
      MODULE_ARCH="$host_os-$host_cpu"
      ;;
   *)
      MODULE_ARCH="$host_os-$host_cpu-v_maj.v_min.v_mic"
      ;;
esac
AC_SUBST(MODULE_ARCH)
AC_DEFINE_UNQUOTED(MODULE_ARCH, "${MODULE_ARCH}", "Module architecture")

case "$host_os" in
   mingw*)
      MODULE_EXT=".dll"
      ;;
   *)
      MODULE_EXT=".so"
      ;;
esac

AC_DEFINE_UNQUOTED(SHARED_LIB_SUFFIX, "${MODULE_EXT}", [Suffix for shared objects])

### Checks for programs
AC_PROG_CC
AC_PROG_CXX

# pkg-config
PKG_PROG_PKG_CONFIG

### Checks for libraries

# Evil library for compilation on Windows CE

EFL_EMS_BUILD=""
case "$host_os" in
   mingw*)
      PKG_CHECK_MODULES([EVIL], [evil >= 1.0.0])
      AC_DEFINE([HAVE_EVIL], [1], [Set to 1 if evil package is installed])
      requirement_ems="${requirement_ems} evil"
      EFL_EMS_BUILD="-DEFL_EMS_BUILD"
   ;;
esac
AC_SUBST([EFL_EMS_BUILD])

### Checks for compiler characteristics
AC_C_CONST
AC_C_BIGENDIAN
AC_C_INLINE
AC_PROG_CC_STDC
EFL_ATTRIBUTE_UNUSED

have_wince="no"
have_win32="no"
EMS_CFLAGS=""
case "$host_os" in
   mingw32ce*)
      EMS_CFLAGS="${EVIL_CFLAGS}"
      have_win32="yes"
      have_wince="yes"
      ;;
   mingw*)
      EMS_CFLAGS="${EVIL_CFLAGS}"
      have_win32="yes"
      ;;
esac
AM_CONDITIONAL([EMS_HAVE_WINCE], [test "x$have_wince" = "xyes"])
AM_CONDITIONAL([EMS_HAVE_WIN32], [test "x$have_win32" = "xyes"])

m4_ifdef([v_mic],
   [
    EFL_COMPILER_FLAG([-Wall])
    EFL_COMPILER_FLAG([-W])
   ])

EFL_COMPILER_FLAG([-Wshadow])

case "$host_vendor" in
  ps3*)
      PKG_CHECK_MODULES([ESCAPE], [escape])
      AC_DEFINE(HAVE_ESCAPE, 1, [Set to 1 if Escape package is installed])
      requirement_ems="${requirement_ems} escape"
      EMS_CFLAGS="${ESCAPE_CFLAGS} ${EMS_CFLAGS}"
    ;;
esac

AC_SUBST([EMS_CFLAGS])


### Checks for linker characteristics
EMS_LIBS=
lt_enable_auto_import=""
case "${host_os}" in
   mingw*)
      EMS_LIBS="-ldl ${EVIL_LIBS} -lm"
      lt_enable_auto_import="-Wl,--enable-auto-import"
   ;;
   dragonfly*|openbsd*)
      EMS_LIBS="-lm"
   ;;
   freebsd*|netbsd*)
      EMS_LIBS="-lrt -lm"
   ;;
   darwin*)
      EMS_LIBS="-lm"
   ;;
   cygwin*)
      EMS_LIBS="-ldl -lm"
   ;;
   *)
      EMS_LIBS="-ldl -lrt -lm"
   ;;
esac
case "$host_vendor" in
  ps3*)
      # Escape had already been checked to exist
      EMS_LIBS="${ESCAPE_LIBS}"
    ;;
esac
AC_SUBST([EMS_LIBS])
AC_SUBST([lt_enable_auto_import])

EFL_LINKER_FLAG([-fno-strict-aliasing])

CFLAGS="${CFLAGS} ${EMS_CFLAGS}"
LDFLAGS="${LDFLAGS} ${EMS_LDFLAGS}"
LIBS="${LIBS} ${EMS_LIBS}"

AC_CONFIG_FILES([
Makefile
src/Makefile
src/bin/Makefile
])

AC_OUTPUT

#####################################################################
## Info

echo
echo
echo
echo "------------------------------------------------------------------------"
echo "$PACKAGE $VERSION"
echo "------------------------------------------------------------------------"
echo
echo
echo

