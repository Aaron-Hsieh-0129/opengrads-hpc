***************************************************************************************
* $Id: drawmark.gs,v 1.60 2023/07/20 22:20:16 bguan Exp $
*
* Copyright (c) 2012-2020, Bin Guan
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
function drawmark(arg)
*
* Draw marks at data points of a 2-D field.
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

var=subwrd(arg,1)
mark=subwrd(arg,2)
color=subwrd(arg,3)
size=subwrd(arg,4)
mag=subwrd(arg,5)
append=subwrd(arg,6)
text=parsestr(arg,7)
if(size='')
  usage()
  return
endif
if(mag='')
  mag=0
endif
if(append='')
  append=0
endif
if(text='')
  text='Variable'
endif

if(!valnum(size) | size<0)
  say '[drawmark ERROR] <size> must be numeric >=0.'
  return
endif
if(!valnum(mag) | mag<0)
  say '[drawmark ERROR] <magnitude> must be numeric >=0.'
  return
endif

color1=split(color,'&','head')
color2=split(color,'&','tail')
if(color2='')
  color2=color1
endif

qdims(0,'mydim')

num_varying_dim=(_.mydim.xsO!=_.mydim.xeO)+(_.mydim.ysO!=_.mydim.yeO)+(_.mydim.zsO!=_.mydim.zeO)+(_.mydim.tsO!=_.mydim.teO)
if(num_varying_dim!=2)
  say '[drawmark ERROR] # of varying dimensions must be 2. Current setting is'
  'query dims'
  cnt=2
  while(cnt<=6)
    say ' 'sublin(result,cnt)
    cnt=cnt+1
  endwhile
  return
endif

'display 0/0'
undef=subwrd(result,4)

*
* Run display to get scaling environment for gr2xy.
*
'set gxout contour'
'set clevs -1e9'
'display lat'
'query gxinfo'
line5=sublin(result,5)
xaxis=subwrd(line5,3)
yaxis=subwrd(line5,6)

*
* Evaluate at once for whole domain, rather than pixel by pixel later on-the-fly, for computational efficiency.
*
'drawmarktmp='var

*
* Draw mark.
*
xcnt=math_nint(_.mydim.xs)
while(xcnt<=_.mydim.xe)
  ycnt=math_nint(_.mydim.ys)
  while(ycnt<=_.mydim.ye)
    zcnt=math_nint(_.mydim.zs)
    while(zcnt<=_.mydim.ze)
      tcnt=math_nint(_.mydim.ts)
      while(tcnt<=_.mydim.te)
        'set x 'xcnt
        'set y 'ycnt
        'set z 'zcnt
        'set t 'tcnt
        'query dims'
        linex=sublin(result,2)
        liney=sublin(result,3)
        linez=sublin(result,4)
        linet=sublin(result,5)
        x_rounded=subwrd(linex,9)
        y_rounded=subwrd(liney,9)
        z_rounded=subwrd(linez,9)
        t_rounded=subwrd(linet,9)
        within_parea=(x_rounded>=_.mydim.xs & x_rounded<=_.mydim.xe) & (y_rounded>=_.mydim.ys & y_rounded<=_.mydim.ye) & (z_rounded>=_.mydim.zs & z_rounded<=_.mydim.ze) & (t_rounded>=_.mydim.ts & t_rounded<=_.mydim.te)
        'display drawmarktmp'
        value=subwrd(result,4)
        if(within_parea & value!=undef)
          if(mag!=0);sizescaled=size*math_sqrt(math_abs(value)/mag);else;sizescaled=size;endif
          if(xaxis='Lon' & yaxis='Lat')
            'query gr2xy 'xcnt' 'ycnt
          endif
          if(xaxis='Lon' & yaxis='Lev')
            'query gr2xy 'xcnt' 'zcnt
          endif
          if(xaxis='Lon' & yaxis='Time')
            'query gr2xy 'xcnt' 'tcnt
          endif
          if(xaxis='Lat' & yaxis='Lon')
            'query gr2xy 'ycnt' 'xcnt
          endif
          if(xaxis='Lat' & yaxis='Lev')
            'query gr2xy 'ycnt' 'zcnt
          endif
          if(xaxis='Lat' & yaxis='Time')
            'query gr2xy 'ycnt' 'tcnt
          endif
          if(xaxis='Lev' & yaxis='Lon')
            'query gr2xy 'zcnt' 'xcnt
          endif
          if(xaxis='Lev' & yaxis='Lat')
            'query gr2xy 'zcnt' 'ycnt
          endif
          if(xaxis='Lev' & yaxis='Time')
            'query gr2xy 'zcnt' 'tcnt
          endif
          if(xaxis='Time' & yaxis='Lon')
            'query gr2xy 'tcnt' 'xcnt
          endif
          if(xaxis='Time' & yaxis='Lat')
            'query gr2xy 'tcnt' 'ycnt
          endif
          if(xaxis='Time' & yaxis='Lev')
            'query gr2xy 'tcnt' 'zcnt
          endif
          x=subwrd(result,3)
          y=subwrd(result,6)
          if(value<0)
            'set line 'color1
          else
            'set line 'color2
          endif
          'draw mark 'mark' 'x' 'y' 'sizescaled
        endif
        tcnt=tcnt+1
      endwhile
      zcnt=zcnt+1
    endwhile
    ycnt=ycnt+1
  endwhile
  xcnt=xcnt+1
endwhile

*
* Save mark information for use by legend.gs
*
num_var=1
if(append!=1)
  rc=write(mytmpdir'/legend.txt.'randnum,'mark')
endif
cnt=1
while(cnt<=num_var)
  line=mag' 'size' 'mark' 'color2' 'text
  if(append!=1)
    rc=write(mytmpdir'/legend.txt.'randnum,line)
  else
    rc=write(mytmpdir'/legend.txt.'randnum,line,append)
  endif
  cnt=cnt+1
endwhile
rc=close(mytmpdir'/legend.txt.'randnum)

*
* Restore original dimension environment.
*
_.mydim.resetx
_.mydim.resety
_.mydim.resetz
_.mydim.resett

*
* Undefine temporary variable.
*
'undefine drawmarktmp'

return
***************************************************************************************
function split(instr,char,where)
outstr1=instr
outstr2=''
* note: default output if char is not found
cnt=1
while(substr(instr,cnt,1)!='')
  if(substr(instr,cnt,1)=char)
    outstr1=substr(instr,1,cnt-1)
    outstr2=substr(instr,cnt+1,strlen(instr)-cnt)
    break
  endif
  cnt=cnt+1
endwhile
if(where='head')
  return outstr1
endif
if(where='tail')
  return outstr2
endif
***************************************************************************************
function usage()
*
* Print usage information.
*
say '  Draw marks at data points of a 2-D field.'
say ''
say '  USAGE: drawmark <var> <mark> <color1>[&<color2>] <size> [<magnitude> [<append> [<text>]]]]'
say '    <var>: variable name. Can be any GrADS expression.'
say '    <mark>: mark type.'
say '    <color>: mark color. <color1>=negative, and <color2>=positive if two colors are given.'
say '    <size>: reference mark size (diameter).'
say '    <magnitude>: mark sizes (diameters) are set proportional to square root of variable magnitude if <magnitude> is nonzero, or <size> if zero. Default=0.'
say '    <append>: 0 (default) or 1. Set to 1 if appending to an existing plot. (Run "legend.gs" after all data are plotted.)'
say '    <text>: Text to be shown in legend. Text beginning with a minus sign or containing spaces must be double quoted.'
say ''
say '  NOTE: <var> must be on a grid consistent with default file. If not, use "set dfile" to change default file.'
say ''
say '  EXAMPLE 1:'
say '    drawmark swe 3 3 0.1 0.5'
say '    legend'
say ''
say '  EXAMPLE 2:'
say '    drawmark sat-273.15 3 4&2 0.1 5'
say '    legend'
say ''
say '  DEPENDENCIES: qdims.gsf'
say ''
say '  SEE ALSO: legend.gs'
say ''
say '  Copyright (c) 2012-2020, Bin Guan.'
return
