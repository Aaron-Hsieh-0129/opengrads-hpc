***************************************************************************************
* $Id: vcr.gs,v 1.18 2019/02/27 21:45:42 bguan Exp $
*
* Copyright (c) 2014-2015, Bin Guan
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
function vcr(arg)
*
* Create vertical cross-section.
*
rc=gsfallow('on')

* Define system temporary directory.
tmpdir='/tmp'
* Get username and create user-specific temporary directory.
'!echo $USER > .bGASL.txt'
rc=read('.bGASL.txt')
while(sublin(rc,1))
  '!echo $USER > .bGASL.txt'
  rc=read('.bGASL.txt')
endwhile
user=sublin(rc,2)
'!rm .bGASL.txt'
mytmpdir=tmpdir'/bGASL-'user
'!mkdir -p 'mytmpdir
* Get process ID.
pidlock=mytmpdir'/pid.lock'
pidfile=mytmpdir'/pid.txt'
'!while true; do if mkdir 'pidlock'; then break; else echo System busy. Please wait...; sleep 1; fi; done 2> /dev/null'
'!echo $PPID > 'pidfile
rc=read(pidfile)
randnum=sublin(rc,2)
'!rm -r 'pidlock

input=subwrd(arg,1)
output=subwrd(arg,2)
num_point=subwrd(arg,3)
if(output='')
  usage()
  return
endif

qdims(1,'mydim')

*
* Set sampled points (equally spaced).
*
if(num_point='')
  num_point=math_abs(math_nint(_.mydim.xs-_.mydim.xe))+math_abs(math_nint(_.mydim.ys-_.mydim.ye))
else
  if(valnum(num_point)=0)
  num_point_name=num_point
  num_point=math_abs(math_nint(_.mydim.xs-_.mydim.xe))+math_abs(math_nint(_.mydim.ys-_.mydim.ye))
  'define 'num_point_name'='num_point
  endif
endif
cnt=1
while(cnt<=num_point)
  lon.cnt=_.mydim.lonsO+(cnt-1)*(_.mydim.loneO-_.mydim.lonsO)/(num_point-1)
  lat.cnt=_.mydim.latsO+(cnt-1)*(_.mydim.lateO-_.mydim.latsO)/(num_point-1)
  cnt=cnt+1
endwhile

*
* Resample and write .dat file.
*
'set gxout fwrite'
'set fwrite 'mytmpdir'/vcr.dat.'randnum
tcnt=_.mydim.ts
while(tcnt<=_.mydim.te)
  'set t 'tcnt
  zcnt=_.mydim.zs
  while(zcnt<=_.mydim.ze)
    'set z 'zcnt
    cnt=1
    while(cnt<=num_point)
      'set lon 'lon.cnt' 1e30'
      'set lat 'lat.cnt' 1e30'
      'query dims'
      line2=sublin(result,2)
      line3=sublin(result,3)
      xdecimal=subwrd(line2,11)
      ydecimal=subwrd(line3,11)
      x1=math_int(xdecimal);x2=x1+1
      y1=math_int(ydecimal);y2=y1+1
      'set x 'x1' 'x2
      lon1=subwrd(result,4);lon2=subwrd(result,5)
      'set y 'y1' 'y2
      lat1=subwrd(result,4);lat2=subwrd(result,5)
      'set x 'x1
      'set y 'y1
      'vcrtmp11='input
      'set x 'x2
      'set y 'y1
      'vcrtmp21='input
      'set x 'x1
      'set y 'y2
      'vcrtmp12='input
      'set x 'x2
      'set y 'y2
      'vcrtmp22='input
      'display const((vcrtmp11*('lon2'-'lon.cnt')*('lat2'-'lat.cnt')+vcrtmp21*('lon.cnt'-'lon1')*('lat2'-'lat.cnt')+vcrtmp12*('lon2'-'lon.cnt')*('lat.cnt'-'lat1')+vcrtmp22*('lon.cnt'-'lon1')*('lat.cnt'-'lat1'))/(('lon2'-'lon1')*('lat2'-'lat1')),-9.99e8,-u)'
      cnt=cnt+1
    endwhile
    zcnt=zcnt+1
  endwhile
  tcnt=tcnt+1
endwhile
'disable fwrite'
'set gxout contour'

'set x 1'
'set y 1'
qdims(0,'filedim')
lon1_of_file=_.filedim.lons
lat1_of_file=_.filedim.lats

writectl(mytmpdir'/vcr.ctl.'randnum,'^vcr.dat.'randnum,num_point,lon1_of_file,lat1_of_file,output)

*
* Output to variable(s).
*
'open 'mytmpdir'/vcr.ctl.'randnum
file_num=file_number()
'set x 1 'num_point
'set y 1'
_.mydim.resetz
_.mydim.resett
output'='output'.'file_num
'close 'file_num
'!rm 'mytmpdir'/vcr.dat.'randnum
_.mydim.resetx
_.mydim.resety

'undefine vcrtmp11'
'undefine vcrtmp21'
'undefine vcrtmp12'
'undefine vcrtmp22'

return
***************************************************************************************
function writectl(ctlfile,datfile,num_point,lon1_of_file,lat1_of_file,var)
*
* Write .ctl file.
*
lines=11
line.1='dset 'datfile
line.2='undef -9.99e8'
if(_.mydim.cal='')
  line.3='*options'
else
  line.3='options '_.mydim.cal
endif
line.4='title intentionally left blank.'
line.5='xdef 'num_point' linear 'lon1_of_file' '_.mydim.dlon
line.6='ydef 1 levels 'lat1_of_file
line.7=_.mydim.zdef
line.8=_.mydim.tdef
line.9='vars 1'
line.10=var' '_.mydim.nz0' 99 'var
line.11='endvars'
cnt=1
while(cnt<=lines)
  status=write(ctlfile,line.cnt)
  cnt=cnt+1
endwhile
status=close(ctlfile)

return
***************************************************************************************
function file_number()
*
* Get the number of files opened.
*
'query files'
line1=sublin(result,1)
if(line1='No files open')
  return 0
endif

lines=1
while(sublin(result,lines+1)!='')
  lines=lines+1
endwhile

return lines/3
***************************************************************************************
function usage()
*
* Print usage information.
*
say '  Create vertical cross-section.'
say ''
say '  USAGE: vcr <input> <output> [<num_point>]'
say '    <input>: input. Can be any GrADS expression.'
say '    <output>: vertical cross-section.'
say '    <num_point>: set (if numeric) or return (if non-numeric) # of horizontal points sampled along cross section.'
say ''
say '  EXAMPLE:'
say '    set lon 180 200'
say '    set lat 15 0'
say '    set lev 1000 100'
say '    vcr humidity out 10'
say '    set x 1 10'
say '    set y 1'
say '    set xlabs A|B'
say '    display out'
say ''
say '  DEPENDENCIES: qdims.gsf'
say ''
say '  Copyright (c) 2014-2015, Bin Guan.'
return
