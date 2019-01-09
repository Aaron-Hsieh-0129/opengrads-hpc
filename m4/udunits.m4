dnl GA_CHECK_UDUNITS : Check for udunits
dnl args :             action-if-yes, action-if-no

AC_DEFUN([GA_CHECK_UDUNITS],
[
  ga_use_udunits='no'
  AC_CHECK_HEADER([udunits.h],
  [  AC_CHECK_LIB([udunits],[utInit],
     [  ga_use_udunits='yes'
        UDUNITS_LIBS='-ludunits'
     ])
  ])
  
  if test "z$ga_use_udunits" = "zyes" ; then
      m4_if([$1], [], [:], [$1])
  else
      m4_if([$2], [], [:], [$2])
  fi
  AC_SUBST([UDUNITS_LIBS])
])
