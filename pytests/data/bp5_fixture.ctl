* Added in 2026 as a GPLv2 BP5 regression descriptor; see ../../COPYING.
*
* Written the way a descriptor for a Cartesian model is: the fixture stores X
* and Y in metres, and they appear here as degree offsets on GrADS's 6370 km
* sphere, centred on 0, with T taken from the dataset's CF time coordinate.
* That is also what a descriptor-free bpopen of the same file produces, which
* the regression checks by comparing the two opens' axes.
dset ^bp5_fixture.bp
dtype bp5
title ADIOS2 BP5 reader fixture
undef -9999
xdef 4 linear -1.3491941800568836e-05 8.9946278670458903e-06
ydef 3 linear -8.9946278670458903e-06 8.9946278670458903e-06
zdef 2 levels 1000 500
tdef 2 linear 04:05z03feb2001 10mn
vars 2
temperature=>temp 2 z,y,x Temperature
surface_pressure=>ps 0 y,x Surface pressure
endvars
