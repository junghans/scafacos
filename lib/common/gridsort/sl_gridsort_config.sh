#!/bin/sh
#set -xv

SL=../sl

SL_CONFIG=${SL}/support/sl_config.sh

${SL_CONFIG} --src-sl="${SL}" --dst-sl=. --source=single --source-link --am-libname=libfcs_gridsort.a --am-libprefix=fcs_gridsort_ --extra-prefix=fcs_gridsort_

sed -i -e 's!^noinst_LTLIBRARIES\(.*libfcs_gridsort.la\)$!if ENABLE_SINGLE_LIB\nnoinst_LTLIBRARIES\1\nelse\nlib_LTLIBRARIES\1\n\endif!' \
       -e 's!\(-I\$(srcdir)/include\)!-I$(top_srcdir)/lib \1!' \
       Makefile.am
