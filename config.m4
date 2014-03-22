PHP_ARG_ENABLE(uopz, whether to enable uopz support,
[  --enable-uopz   Enable uopz support])

if test "$PHP_UOPZ" != "no"; then
  PHP_NEW_EXTENSION(uopz, uopz.c, $ext_shared)
fi
