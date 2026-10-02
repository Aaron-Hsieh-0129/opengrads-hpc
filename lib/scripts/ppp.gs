***************************************************************************************
* $Id: ppp.gs,v 1.44 2023/06/21 04:22:01 bguan Exp $
*
* Copyright (c) 2013-2023, Bin Guan
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
function ppp(arg)
*
* Produce properly-cropped, publication-ready graphic files.
*
outfile=subwrd(arg,1)
fmt1=subwrd(arg,2)
fmt2=subwrd(arg,3)
fmt3=subwrd(arg,4)
fmt4=subwrd(arg,5)
fmt5=subwrd(arg,6)
if(outfile='')
  usage()
  return
endif
if(fmt1='')
  fmt1='eps'
endif

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

*
* Produce .eps file.
*
if(fmt1='eps' | fmt2='eps' | fmt3='eps' | fmt4='eps' | fmt5='eps')
  'gxprint 'outfile'.eps white'
  '!gs -dBATCH -dNOPAUSE -q -sDEVICE=bbox 'outfile'.eps 2>&1 |grep %%BoundingBox >'mytmpdir'/ppp.bbox.'randnum
  bbox=sublin(read(mytmpdir'/ppp.bbox.'randnum),2)
  rc=close(mytmpdir'/ppp.bbox.'randnum)
  '!sed -i s/^%%BoundingBox:.*/"'bbox'"/ 'outfile'.eps'
  '!sed -i /^%%PageBoundingBox:/d 'outfile'.eps'
  say '[ppp info] 'outfile'.eps produced.'
endif

*
* Produce .pdf file.
*
if(fmt1='pdf' | fmt2='pdf' | fmt3='pdf' | fmt4='pdf' | fmt5='pdf')
  'gxprint 'mytmpdir'/ppp.eps.'randnum' white eps'
  '!gs -dBATCH -dNOPAUSE -q -sDEVICE=bbox 'mytmpdir'/ppp.eps.'randnum' 2>&1 |grep %%BoundingBox >'mytmpdir'/ppp.bbox.'randnum
  bbox=sublin(read(mytmpdir'/ppp.bbox.'randnum),2)
  rc=close(mytmpdir'/ppp.bbox.'randnum)
  '!sed -i s/^%%BoundingBox:.*/"'bbox'"/ 'mytmpdir'/ppp.eps.'randnum
  '!sed -i /^%%PageBoundingBox:/d 'mytmpdir'/ppp.eps.'randnum
  '!gs -q -dBATCH -dNOPAUSE -dEPSCrop -sDEVICE=pdfwrite -sOutputFile='outfile'.pdf 'mytmpdir'/ppp.eps.'randnum' 2>/dev/null'
  say '[ppp info] 'outfile'.pdf produced.'
endif

*
* Produce .png file.
*
if(fmt1='png' | fmt2='png' | fmt3='png' | fmt4='png' | fmt5='png')
  'gxprint 'mytmpdir'/ppp.eps.'randnum' white eps'
  '!gs -dBATCH -dNOPAUSE -q -sDEVICE=bbox 'mytmpdir'/ppp.eps.'randnum' 2>&1 |grep %%BoundingBox >'mytmpdir'/ppp.bbox.'randnum
  bbox=sublin(read(mytmpdir'/ppp.bbox.'randnum),2)
  rc=close(mytmpdir'/ppp.bbox.'randnum)
  '!sed -i s/^%%BoundingBox:.*/"'bbox'"/ 'mytmpdir'/ppp.eps.'randnum
  '!sed -i /^%%PageBoundingBox:/d 'mytmpdir'/ppp.eps.'randnum
  '!gs -q -dBATCH -dNOPAUSE -dEPSCrop -sDEVICE=png16m -r150 -dTextAlphaBits=4 -dGraphicsAlphaBits=4 -sOutputFile='outfile'.png 'mytmpdir'/ppp.eps.'randnum' 2>/dev/null'
  say '[ppp info] 'outfile'.png produced.'
endif
***************************************************************************************
function usage()
*
* Print usage information.
*
say '  Produce properly-cropped, publication-ready graphic files.'
say ''
say '  USAGE: ppp <outfile> [<format1>] [<format2>]...'
say '    <outfile>: full path of output file. Do NOT include the filename extension (e.g., use "mypath/myfile", instead of "mypath/myfile.eps").'
say '    <format>: eps (default), pdf, or png.'
say ''
say '  EXAMPLE 1: print to "myfile.eps".'
say '    ppp myfile'
say ''
say '  EXAMPLE 2: print to "myfile.png".'
say '    ppp myfile png'
say ''
say '  Copyright (c) 2013-2023, Bin Guan.'
return
