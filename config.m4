PHP_ARG_ENABLE([uopz],
  [whether to enable uopz support],
  [AS_HELP_STRING([--enable-uopz],
    [Enable uopz support])])

PHP_ARG_ENABLE([uopz-coverage],
  [whether to enable uopz coverage support],
  [AS_HELP_STRING([--enable-uopz-coverage],
    [Enable uopz coverage support])],
  [no],
  [no])

PHP_ARG_WITH([uopz-sanitize],
  [whether to enable AddressSanitizer for uopz],
  [AS_HELP_STRING([--with-uopz-sanitize],
    [Build uopz with AddressSanitizer support])],
  [no],
  [no])

if test "$PHP_UOPZ" != "no"; then
  AS_VAR_IF([PHP_UOPZ_SANITIZE], [no],, [
    EXTRA_LDFLAGS="-lasan"
    EXTRA_CFLAGS="-fsanitize=address -fno-omit-frame-pointer"
    PHP_SUBST([EXTRA_LDFLAGS])
    PHP_SUBST([EXTRA_CFLAGS])
  ])

  PHP_NEW_EXTENSION([uopz], m4_normalize([
      src/class.c
      src/constant.c
      src/executors.c
      src/function.c
      src/handlers.c
      src/hook.c
      src/return.c
      src/util.c
      uopz.c
    ]),
    [$ext_shared],,
    [-DZEND_ENABLE_STATIC_TSRMLS_CACHE=1])
  PHP_ADD_BUILD_DIR([$ext_builddir/src])
  PHP_ADD_INCLUDE([$ext_builddir])

  AS_VAR_IF([PHP_UOPZ_COVERAGE], [no],, [PHP_ADD_MAKEFILE_FRAGMENT])
fi
