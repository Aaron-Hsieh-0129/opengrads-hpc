* Added in 2026 as a GPLv2 BP5 regression descriptor; see ../../COPYING.
dset ^bp5_fixture.bp
dtype bp5
title ADIOS2 BP5 reader fixture
undef -9999
xdef 4 linear 0 1
ydef 3 linear -1 1
zdef 2 levels 1000 500
tdef 2 linear 00z01jan2000 1hr
vars 2
temperature=>temp 2 z,y,x Temperature
surface_pressure=>ps 0 y,x Surface pressure
endvars
