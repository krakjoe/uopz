uopz-test-coverage:
	CCACHE_DISABLE=1 EXTRA_CFLAGS="-fprofile-arcs -ftest-coverage" TEST_PHP_ARGS="-q" $(MAKE) clean test

uopz-test-coverage-lcov: uopz-test-coverage
	lcov -c --directory $(top_srcdir)/src/.libs --output-file $(top_srcdir)/coverage.info

uopz-test-coverage-html: uopz-test-coverage-lcov
	genhtml $(top_srcdir)/coverage.info --output-directory=$(top_srcdir)/html

uopz-test-coverage-travis:
	CCACHE_DISABLE=1 EXTRA_CFLAGS="-fprofile-arcs -ftest-coverage" $(MAKE)
