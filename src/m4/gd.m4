dnl GA_CHECK_LIB_GD : check for gd library
dnl args :             action-if-yes, action-if-no

AC_DEFUN([GA_CHECK_LIB_GD],
[
  ga_check_gd="no"
  GD_LIBS=
  GD_CFLAGS=
  GD_LDFLAGS=

  ga_pkgconfig_gd=yes
  PKG_CHECK_MODULES([GD],[gdlib],,[ga_pkgconfig_gd=no])
  ac_save_LIBS="$LIBS"
  ac_save_CPPFLAGS="$CPPFLAGS"
  ac_save_LDFLAGS="$LDFLAGS"

  ga_config_gd=no
  if test $ga_pkgconfig_gd != 'yes'; then
    AC_PATH_PROG([GD_CONFIG],[gdlib-config],[no])
    if test "$GD_CONFIG" != "no"; then
      GD_LIBS=`$GD_CONFIG --libs`
      GD_CFLAGS=`$GD_CONFIG --cflags`
      GD_LDFLAGS=`$GD_CONFIG --ldflags`
      ga_config_gd=yes
    fi
  fi
  
  if test $ga_pkgconfig_gd = 'yes' -o $ga_config_gd = 'yes'; then
     LDFLAGS="$LDFLAGS $GD_LDFLAGS"
     LIBS="$LIBS $GD_LIBS"
     AC_CHECK_HEADER([gd.h],
     [  AC_CHECK_LIB([gd], [gdImageCreate],
        [  ga_check_gd=yes
           GD_LIBS="-lgd $GD_LIBS"
           AC_CHECK_FUNCS([gdCompareInt])
        ],
        [
           GD_LDFLAGS=
           GD_LIBS=
           LIBS="$ac_save_LIBS"
           LDFLAGS="$ac_save_LDFLAGS"
        ])
     ],
     [
       GD_CFLAGS=
       CPPFLAGS="$ac_save_CPPFLAGS"
     ])
  fi
  if test $ga_check_gd = 'no'; then
     AC_CHECK_HEADER([gd.h],
      [ AC_CHECK_LIB([z], [compress],
        [ AC_CHECK_LIB([png], [main],
          [ AC_CHECK_LIB([gd], [gdImageCreate],
            [ ga_check_gd=yes
              AC_CHECK_FUNCS([gdCompareInt])
              GD_LIBS='-lgd -lpng -lz'
            ])
          ])
        ])
     ])
  fi

  LIBS="$ac_save_LIBS"
  CPPFLAGS="$ac_save_CPPFLAGS"
  LDFLAGS="$ac_save_LDFLAGS"

  if test $ga_check_gd = 'yes'; then
     m4_if([$1], [], [:], [$1])
  else
     m4_if([$2], [], [:], [$2])
  fi

  AC_SUBST([GD_LIBS])
  AC_SUBST([GD_LDFLAGS])
  AC_SUBST([GD_CFLAGS])
])
