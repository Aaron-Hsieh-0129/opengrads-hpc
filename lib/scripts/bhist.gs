***************************************************************************************
* $Id: hist.gs,v 1.42 2023/02/06 23:35:32 bguan Exp $
*
* opengrads-hpc: shipped as bhist.gs because the distribution already carries a
* different hist.gs (a histogram plotter by Arlindo da Silva). Only the file name
* and the command name in the usage and error messages were changed.
*
* Copyright (c) 2008-2023, Bin Guan
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
function hist(arg)
*
* Calculate histogram.
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

dimension=subwrd(arg,1)
input=subwrd(arg,2)
output=subwrd(arg,3)
edge_s=subwrd(arg,4)
edge_e=subwrd(arg,5)
bin_size=subwrd(arg,6)
if(bin_size='')
  usage()
  return
endif

if(!valnum(edge_s))
  say '[bhist ERROR] <left_edge> must be numeric.'
  return
endif
if(!valnum(edge_e))
  say '[bhist ERROR] <right_edge> must be numeric.'
  return
endif
if(!valnum(bin_size) | bin_size<=0)
  say '[bhist ERROR] <bin_size> must be numeric >0.'
  return
endif

qdims(1,'mydim')

'hsttmp=maskout('input','edge_e'-('input'))'

'set gxout fwrite'
'set fwrite 'mytmpdir'/hist.dat.'randnum

'hstII=hsttmp/hsttmp'
*
* First to next-to-last bin
*
left=edge_s
right=left+bin_size;if(right>edge_e);right=edge_e;endif
nz=0
zdef=left+bin_size/2
while(left<edge_e)
  if(dimension='t')
    'set t '_.mydim.ts' '_.mydim.te
  endif
  if(dimension='xy')
    'set x '_.mydim.xs' '_.mydim.xe
    'set y '_.mydim.ys' '_.mydim.ye
  endif
  if(dimension='xyt')
    'set x '_.mydim.xs' '_.mydim.xe
    'set y '_.mydim.ys' '_.mydim.ye
    'set t '_.mydim.ts' '_.mydim.te
  endif
  'hstIIa=const(maskout(hstII,hsttmp-'left'),0,-u)'
  'hstIIb=const(maskout(hstII,'right'-hsttmp),0,-u)'
  'hstIIc=const(maskout(hstII,hsttmp-'right'),0,-u)'
  if(right<edge_e)
    'hstIIz=hstIIa*hstIIb*(1-hstIIb*hstIIc)'
  else
    'hstIIz=hstIIa*hstIIb'
  endif
* all bins except last one include left edge but not right edge; last bin includes both edges
  if(dimension='t')
    'set t '_.mydim.ts
    'hstouttmp=sum(hstIIz,t='_.mydim.ts',t='_.mydim.te')'
  endif
  if(dimension='xy')
    'set x '_.mydim.xs
    'set y '_.mydim.ys
    'hstouttmp=sum(sum(hstIIz,x='_.mydim.xs',x='_.mydim.xe'),y='_.mydim.ys',y='_.mydim.ye')'
  endif
  if(dimension='xyt')
    'set x '_.mydim.xs
    'set y '_.mydim.ys
    'set t '_.mydim.ts
    'hstouttmp=sum(sum(sum(hstIIz,x='_.mydim.xs',x='_.mydim.xe'),y='_.mydim.ys',y='_.mydim.ye'),t='_.mydim.ts',t='_.mydim.te')'
  endif
  'display const(hstouttmp,-9.99e8,-u)'
  left=left+bin_size;if(left>edge_e);left=edge_e;endif
  right=right+bin_size;if(right>edge_e);right=edge_e;endif
* begin trick to get a new line character
  'nonexistentvar=1'
  'query defval nonexistentvar 1 1'
  newlinechar=substr(result,12,1)
  'undefine nonexistentvar'
* end trick
  if(math_mod(nz,10)=0 & nz>0)
    zdef=zdef' 'newlinechar
  endif
  zdef=zdef' 'left+bin_size/2
  nz=nz+1
endwhile
zdef='ZDEF 'nz' LEVELS 'zdef

if(dimension='t')
  writectl4t(mytmpdir'/hist.ctl.'randnum,'^hist.dat.'randnum,nz,zdef,output)
endif
if(dimension='xy')
  writectl4xy(mytmpdir'/hist.ctl.'randnum,'^hist.dat.'randnum,nz,zdef,output)
endif
if(dimension='xyt')
  writectl4xyt(mytmpdir'/hist.ctl.'randnum,'^hist.dat.'randnum,nz,zdef,output)
endif

'disable fwrite'
'undefine hsttmp'
'undefine hstII'
'undefine hstIIa'
'undefine hstIIb'
'undefine hstIIc'
'undefine hstIIz'
'undefine hstouttmp'
'set gxout contour'

*
* Output to variable(s).
*
dfile_old=dfile()
'open 'mytmpdir'/hist.ctl.'randnum
file_num=file_number()
_.mydim.resetx
_.mydim.resety
'set lev 'edge_s' 'edge_e
_.mydim.resett
'set dfile 'file_num
output'='output'.'file_num
'set dfile 'dfile_old
'!rm 'mytmpdir'/hist.dat.'randnum
_.mydim.resetz

return
***************************************************************************************
function dfile()
*
* Get the default file number.
*
'query file'

line1=sublin(result,1)
dfile=subwrd(line1,2)

return dfile
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
function writectl4t(ctlfile,datfile,nz,zdef,var)
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
line.5=_.mydim.xdef
line.6=_.mydim.ydef
line.7=zdef
line.8='tdef 1 linear '_.mydim.tims' '_.mydim.dtim
line.9='vars 1'
line.10=var' 'nz' 99 'var
line.11='endvars'
cnt=1
while(cnt<=lines)
  status=write(ctlfile,line.cnt)
  cnt=cnt+1
endwhile
status=close(ctlfile)

return
***************************************************************************************
function writectl4xy(ctlfile,datfile,nz,zdef,var)
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
line.5='xdef 1 levels '_.mydim.lons
line.6='ydef 1 levels '_.mydim.lats
line.7=zdef
line.8=_.mydim.tdef
line.9='vars 1'
line.10=var' 'nz' 99 'var
line.11='endvars'
cnt=1
while(cnt<=lines)
  status=write(ctlfile,line.cnt)
  cnt=cnt+1
endwhile
status=close(ctlfile)

return
***************************************************************************************
function writectl4xyt(ctlfile,datfile,nz,zdef,var)
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
line.5='xdef 1 levels '_.mydim.lons
line.6='ydef 1 levels '_.mydim.lats
line.7=zdef
line.8='tdef 1 linear '_.mydim.tims' '_.mydim.dtim
line.9='vars 1'
line.10=var' 'nz' 99 'var
line.11='endvars'
cnt=1
while(cnt<=lines)
  status=write(ctlfile,line.cnt)
  cnt=cnt+1
endwhile
status=close(ctlfile)

return
***************************************************************************************
function usage()
*
* Print usage information.
*
say '  Calculate histogram.'
say ''
say '  USAGE: bhist t|xy|xyt <input> <output> <left_edge> <right_edge> <bin_size>'
say '    t|xy|xyt: statistics are calculated over selected dimension(s).'
say '    <input>: input field (can have horizontal dimensions; NO vertical dimension).'
say '    <output>: histogram.'
say '    <left_edge>: left edge.'
say '    <right_edge>: right edge.'
say '    <bin_size>: bin size.'
say ''
say '  EXAMPLE 1: histogram over time.'
say '    set time Jan1901 Dec2000'
say '    bhist t precip preciphist -2 2 0.25'
say '    set time Jan1901'
say '    set lev -2 2' 
say '    set xyrev on'
say '    display preciphist'
say ''
say '  EXAMPLE 2: histogram over space.'
say '    set lon 0 360'
say '    set lat -90 90'
say '    bhist xy precip preciphist -2 2 0.25'
say '    set lon 0'
say '    set lat 0'
say '    set lev -2 2' 
say '    set xyrev on'
say '    display preciphist'
say ''
say '  DEPENDENCIES: qdims.gsf'
say ''
say '  Copyright (c) 2008-2023, Bin Guan.'
return
