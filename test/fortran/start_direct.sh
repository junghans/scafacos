#! /bin/sh

. ../defs || exit 1

np=2

[ -n "$NP" ] && np=$NP

start_mpi_job -np $np ./interface_test direct ../inp_data/fortran_interface_test_inp_data.dat 1 
