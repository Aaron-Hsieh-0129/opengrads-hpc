***************************************************************************************
* $Id: drawline.gs,v 1.17 2016/05/11 21:08:08 bguan Exp $
*
* Copyright (c) 2005-2015, Bin Guan
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
function drawline(arg)
*
* Draw lines at specified locations.
*
if(subwrd(arg,2)='')
  usage()
  return
endif

'query gxinfo'
line3=sublin(result,3)
line4=sublin(result,4)
x1=subwrd(line3,4)
x2=subwrd(line3,6)
y1=subwrd(line4,4)
y2=subwrd(line4,6)
line5=sublin(result,5)
xaxis=subwrd(line5,3)
yaxis=subwrd(line5,6)

cnt=1
word=subwrd(arg,cnt)
while(word!='')
  axis=word
  cnt=cnt+1
  word=subwrd(arg,cnt)
  while(word!='' & word!='Lon' & word!='Lat' & word!='Lev' & word!='Val' & word!='Time')
    wcoor=word
    if(axis!='Time' & !valnum(wcoor))
      say '[drawline ERROR] <coordinate> for Lon|Lat|Lev|Val must be numeric.'
      return
    endif
    if(xaxis=axis)
      if(yaxis='Time')
        'query w2xy 'wcoor' 0000z1Jan0000'
      else
        'query w2xy 'wcoor' 0'
      endif
      xcoor=subwrd(result,3)
      'draw line 'xcoor' 'y1' 'xcoor' 'y2
    endif
    if(yaxis=axis)
      if(xaxis='Time')
        'query w2xy 0000z1Jan0000 'wcoor
      else
        'query w2xy 0 'wcoor
      endif
      ycoor=subwrd(result,6)
      'draw line 'x1' 'ycoor' 'x2' 'ycoor
    endif
    cnt=cnt+1
    word=subwrd(arg,cnt)
  endwhile
endwhile

return
***************************************************************************************
function usage()
*
* Print usage information.
*
say '  Draw lines at specified locations.'
say ''
say '  USAGE: drawline Lon|Lat|Lev|Val|Time <coordinate1> [<coordinate2>...] [Lon|Lat|Lev|Val|Time <coordinate1> [<coordinate2>...]]...'
say '    <coordinate>: world coordinate.'
say ''
say '  EXAMPLE 1: drawline Lon 180 Lat 0'
say '    Draw international date line and equator on a map.'
say ''
say '  EXAMPLE 2: drawline Val -1 0 1'
say '    Draw lines with constant value -1, 0 and 1 in a line graph.'
say ''
say '  EXAMPLE 3: drawline Time 1Jan2005'
say '    Draw a straight line marking 1 January 2005.'
say ''
say '  NOTE: proper capitalization MUST be used for key words Lon, Lat, Lev, Val and Time.'
say ''
say '  Copyright (c) 2005-2015, Bin Guan.'
return
