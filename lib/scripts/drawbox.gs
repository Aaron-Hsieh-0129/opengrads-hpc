***************************************************************************************
* $Id: drawbox.gs,v 1.13 2016/05/11 21:09:19 bguan Exp $
*
* Copyright (c) 2010, Bin Guan
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are
* permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice, this list
*    of conditions and the following disclaimer.
*
* 2. Redistributions in binary form must reproduce the above copyright notice, this
*    list of conditions and the following disclaimer in the documentation and/or other
*    materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
* OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
* SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
* TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
* BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
* ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
* DAMAGE.
***************************************************************************************
function drawbox(arg)
*
* Draw an unfilled "box" under any map projection.
*
lon1=subwrd(arg,1)
lon2=subwrd(arg,2)
lat1=subwrd(arg,3)
lat2=subwrd(arg,4)
if(lat2='')
  usage()
  return
endif
if(!valnum(lon1))
  say '[drawbox ERROR] <lon1> must be numeric.'
  return
endif
if(!valnum(lon2))
  say '[drawbox ERROR] <lon2> must be numeric.'
  return
endif
if(!valnum(lat1))
  say '[drawbox ERROR] <lat1> must be numeric.'
  return
endif
if(!valnum(lat2))
  say '[drawbox ERROR] <lat2> must be numeric.'
  return
endif
if(lon2<lon1)
  say '[drawbox ERROR] <lon2> must be >= <lon1>.'
  return
endif
if(lat2<lat1)
  say '[drawbox ERROR] <lat2> must be >= <lat1>.'
  return
endif

*
* Divide zonal lines into N segments, each of which is 0.01 inch long (or about so when the lines are not straight).
*
'query w2xy 'lon1' 'lat1
x1=subwrd(result,3)
y1=subwrd(result,6)
'query w2xy 'lon2' 'lat1
x2=subwrd(result,3)
y2=subwrd(result,6)
'query w2xy 'lon2' 'lat2
x3=subwrd(result,3)
y3=subwrd(result,6)
'query w2xy 'lon1' 'lat2
x4=subwrd(result,3)
y4=subwrd(result,6)
if(!valnum(x1))
  say '[drawbox ERROR] No scaling environment. Make a plot first.'
  return
endif
if(math_abs(x2-x1)>=math_abs(x3-x4))
  xdis=math_abs(x2-x1)
else
  xdis=math_abs(x3-x4)
endif
N=math_nint(xdis/0.01)

*
* In some cases, the straight-line length is much shorter than the length of the latitudinal circle (e.g., in polar stereographic projections
* where we have lon1=0 and lon2=359), and N could be too small to draw a smooth line. Force N to be 100 if it is smaller than that.
*
if(N>0 & N<100)
  N=100
endif

if(N!=0)
  dlon=(lon2-lon1)/N
endif

*
* Draw lower line.
*
i=0
lon=lon1
while(i<=N-1)
  'query w2xy 'lon' 'lat1
  x1=subwrd(result,3)
  y1=subwrd(result,6)
  'query w2xy 'lon+dlon' 'lat1
  x2=subwrd(result,3)
  y2=subwrd(result,6)
  'draw line 'x1' 'y1' 'x2' 'y2
  i=i+1
  lon=lon+dlon
endwhile

*
* Draw upper line.
*
i=N
lon=lon2
while(i>=1)
  'query w2xy 'lon' 'lat2
  x3=subwrd(result,3)
  y3=subwrd(result,6)
  'query w2xy 'lon-dlon' 'lat2
  x4=subwrd(result,3)
  y4=subwrd(result,6)
  'draw line 'x3' 'y3' 'x4' 'y4
  i=i-1
  lon=lon-dlon
endwhile

*
* Divide meridional lines into N segments, each of which is 0.01 inch long (or about so when the lines are not straight).
*
'query w2xy 'lon1' 'lat1
x1=subwrd(result,3)
y1=subwrd(result,6)
'query w2xy 'lon1' 'lat2
x2=subwrd(result,3)
y2=subwrd(result,6)
'query w2xy 'lon2' 'lat1
x3=subwrd(result,3)
y3=subwrd(result,6)
'query w2xy 'lon2' 'lat2
x4=subwrd(result,3)
y4=subwrd(result,6)
if(!valnum(y1))
  say '[drawbox ERROR] No scaling environment. Make a plot first.'
  return
endif
if(math_abs(y2-y1)>=math_abs(y3-y4))
  ydis=math_abs(y2-y1)
else
  ydis=math_abs(y3-y4)
endif
N=math_nint(ydis/0.01)

*
* In some cases, the straight-line length is much shorter than the length of the latitudinal circle (e.g., in polar stereographic projections
* where we have lon1=0 and lon2=359), and N could be too small to draw a smooth line. Force N to be 100 if it is smaller than that.
*
if(N>0 & N<100)
  N=100
endif

if(N!=0)
  dlat=(lat2-lat1)/N
endif

*
* Draw left line.
*
j=0
lat=lat1
while(j<=N-1)
  'query w2xy 'lon1' 'lat
  x1=subwrd(result,3)
  y1=subwrd(result,6)
  'query w2xy 'lon1' 'lat+dlat
  x2=subwrd(result,3)
  y2=subwrd(result,6)
  'draw line 'x1' 'y1' 'x2' 'y2
  j=j+1
  lat=lat+dlat
endwhile

*
* Draw right line.
*
j=N
lat=lat2
while(j>=1)
  'query w2xy 'lon2' 'lat
  x3=subwrd(result,3)
  y3=subwrd(result,6)
  'query w2xy 'lon2' 'lat-dlat
  x4=subwrd(result,3)
  y4=subwrd(result,6)
  'draw line 'x3' 'y3' 'x4' 'y4
  j=j-1
  lat=lat-dlat
endwhile

return
***************************************************************************************
function usage()
*
* Print usage information.
*
say '  Draw an unfilled "box" under any map projection.'
say ''
say '  Usage: drawbox <lon1> <lon2> <lat1> <lat2>'
say '    <lon1>: Beginning longitude.'
say '    <lon2>: Ending longitude. Must be >= <lon1>.'
say '    <lat1>: Beginning latitude.'
say '    <lat2>: Ending latitude. Must be >= <lat1>.'
say ''
say '  Copyright (c) 2010, Bin Guan.'
return
