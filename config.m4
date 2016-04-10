PHP_ARG_ENABLE(uopz, whether to enable uopz support,
[  --enable-uopz   Enable uopz support])

PHP_ARG_WITH(uopz-sanitize, whether to enable AddressSanitizer for uopz,
[  --with-uopz-sanitize Build uopz with AddressSanitizer support], no, no)

if test "$PHP_UOPZ" != "no"; then
  if test "$PHP_UOPZ_SANITIZE" != "no"; then
    EXTRA_LDFLAGS="-lasan"
	EXTRA_CFLAGS="-fsanitize=address -fno-omit-frame-pointer"
	PHP_SUBST(EXTRA_LDFLAGS)
    PHP_SUBST(EXTRA_CFLAGS)
  fi

  PHP_NEW_EXTENSION(uopz, uopz.c src/util.c src/return.c src/hook.c src/constant.c src/function.c src/class.c src/handlers.c src/executors.c, $ext_shared,, -DZEND_ENABLE_STATIC_TSRMLS_CACHE=1)
  PHP_ADD_BUILD_DIR($ext_builddir/src, 1)
  PHP_ADD_INCLUDE($ext_builddir)
fi
