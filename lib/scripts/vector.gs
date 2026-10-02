***************************************************************************************
* $Id: vector.gs,v 1.18 2020/06/04 21:18:43 bguan Exp $
*
* Copyright (c) 2010-2020, Bin Guan
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
function vector(arg)
*
* Plot vectors.
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

expr=subwrd(arg,1)
length=subwrd(arg,2)
mag=subwrd(arg,3)
color=subwrd(arg,4)
thick=subwrd(arg,5)
if(mag='')
  usage()
  return
endif
if(color='')
  color=1
endif
if(thick='')
  thick=4
endif

if(!valnum(length) | length<=0)
  say '[vector ERROR] <length> must be numeric >0.'
  return
endif
if(!valnum(mag) | mag<=0)
  say '[vector ERROR] <magnitude> must be numeric >0.'
  return
endif

expr1=split(expr,';','head')
expr2=split(expr,';','tail')

'set arrscl 'length' 'mag
'set ccolor 'color
'set cthick 'thick
'set arrlab off'
'display 'expr1';'expr2
'set arrlab on'

line0='vector'
line1=length' 'mag' 'color' 'thick
rc=write(mytmpdir'/legend.txt.'randnum,line0)
rc=write(mytmpdir'/legend.txt.'randnum,line1)
rc=close(mytmpdir'/legend.txt.'randnum)

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
say '  Plot vectors.'
say ''
say '  USAGE: vector <expression1>;<expression2> <length> <magnitude> [<color> [<thickness>]]'
say '    <expression1>: first component of vector.'
say '    <expression2>: second component of vector.'
say '    <length>: reference length of arrow.'
say '    <magnitude>: reference magnitude of arrow.'
say '    <color>: arrow color. Default=1.'
say '    <thickness>: arrow thickness. Default=4.'
say ''
say '  SEE ALSO: legend.gs'
say ''
say '  Copyright (c) 2010-2020, Bin Guan.'
return
