***************************************************************************************
* $Id: shadcon.gs,v 1.83 2022/09/22 18:24:37 bguan Exp $
*
* Copyright (c) 2013-2020, Bin Guan
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
function shadcon(arg)
*
* Plot 2-D graph using shading and/or contours with specified color and contour information. 
*
color=subwrd(arg,1)
word2=subwrd(arg,2)
if(valnum(word2))
  offset=subwrd(arg,2)
  var=subwrd(arg,3)
  cint=subwrd(arg,4)
  blackout=subwrd(arg,5)
  shading_style=subwrd(arg,6)
else
  offset=0
  var=subwrd(arg,2)
  cint=subwrd(arg,3)
  blackout=subwrd(arg,4)
  shading_style=subwrd(arg,5)
endif

if(cint='')
  usage()
  return
endif

if(blackout='')
  blackout=0
endif

if(shading_style='')
  shading_style='shaded'
endif

if(!valnum(offset))
  say '[shadcon ERROR] <offset> must be numeric.'
  return
endif
if(!valnum(blackout) | blackout<0)
  say '[shadcon ERROR] <blackout> must be numeric >=0.'
  return
endif

*
* Define color maps
* RGB values are retrieved from http://colorbrewer2.org Copyright (c) Cynthia Brewer, Mark Harrower and The Pennsylvania State University
*
* gray (from light to dark)
'set rgb 36 224 224 224'
'set rgb 37 186 186 186'
'set rgb 38 135 135 135'
'set rgb 39  77  77  77'
'set rgb 40  26  26  26'
col.gray.1=36
col.gray.2=37
col.gray.3=38
col.gray.4=39
col.gray.5=40
col.gray.ncol=5
* green (from light to dark)
'set rgb 41 230 245 208'
'set rgb 42 184 225 134'
'set rgb 43 127 188  65'
'set rgb 44  77 146  33'
'set rgb 45  39 100  25'
col.green.1=41
col.green.2=42
col.green.3=43
col.green.4=44
col.green.5=45
col.green.ncol=5
* fuchsia (from light to dark)
'set rgb 46 253 224 239'
'set rgb 47 241 182 218'
'set rgb 48 222 119 174'
'set rgb 49 197  27 125'
'set rgb 50 142   1  82'
col.fuchsia.1=46
col.fuchsia.2=47
col.fuchsia.3=48
col.fuchsia.4=49
col.fuchsia.5=50
col.fuchsia.ncol=5
* blue (from light to dark)
'set rgb 51 224 243 248'
'set rgb 52 171 217 233'
'set rgb 53 116 173 209'
'set rgb 54  69 117 180'
'set rgb 55  49  54 149'
col.blue.1=51
col.blue.2=52
col.blue.3=53
col.blue.4=54
col.blue.5=55
col.blue.ncol=5
* red (from light to dark)
'set rgb 56 254 224 144'
'set rgb 57 253 174  97'
'set rgb 58 244 109  67'
'set rgb 59 215  48  39'
'set rgb 60 165   0  38'
col.red.1=56
col.red.2=57
col.red.3=58
col.red.4=59
col.red.5=60
col.red.ncol=5
* GREEN (from light to dark)
'set rgb 61 247 252 253'
'set rgb 62 229 245 249'
'set rgb 63 204 236 230'
'set rgb 64 153 216 201'
'set rgb 65 102 194 164'
'set rgb 66  65 174 118'
'set rgb 67  35 139  69'
'set rgb 68   0 109  44'
'set rgb 69   0  68  27'
col.GREEN.1=61
col.GREEN.2=62
col.GREEN.3=63
col.GREEN.4=64
col.GREEN.5=65
col.GREEN.6=66
col.GREEN.7=67
col.GREEN.8=68
col.GREEN.9=69
col.GREEN.ncol=9
* FUCHSIA (from light to dark)
'set rgb 71 247 244 249'
'set rgb 72 231 225 239'
'set rgb 73 212 185 218'
'set rgb 74 201 148 199'
'set rgb 75 223 101 176'
'set rgb 76 231  41 138'
'set rgb 77 206  18  86'
'set rgb 78 152   0  67'
'set rgb 79 103   0  31'
col.FUCHSIA.1=71
col.FUCHSIA.2=72
col.FUCHSIA.3=73
col.FUCHSIA.4=74
col.FUCHSIA.5=75
col.FUCHSIA.6=76
col.FUCHSIA.7=77
col.FUCHSIA.8=78
col.FUCHSIA.9=79
col.FUCHSIA.ncol=9
* BLUE (from light to dark)
'set rgb 81 247 251 255'
'set rgb 82 222 235 247'
'set rgb 83 198 219 239'
'set rgb 84 158 202 225'
'set rgb 85 107 174 214'
'set rgb 86  66 146 198'
'set rgb 87  33 113 181'
'set rgb 88   8  81 156'
'set rgb 89   8  48 107'
col.BLUE.1=81
col.BLUE.2=82
col.BLUE.3=83
col.BLUE.4=84
col.BLUE.5=85
col.BLUE.6=86
col.BLUE.7=87
col.BLUE.8=88
col.BLUE.9=89
col.BLUE.ncol=9
* RED (from light to dark)
'set rgb 91 255 245 240'
'set rgb 92 254 224 210'
'set rgb 93 252 187 161'
'set rgb 94 252 146 114'
'set rgb 95 251 106  74'
'set rgb 96 239  59  44'
'set rgb 97 203  24  29'
'set rgb 98 165  15  21'
'set rgb 99 103   0  13'
col.RED.1=91
col.RED.2=92
col.RED.3=93
col.RED.4=94
col.RED.5=95
col.RED.6=96
col.RED.7=97
col.RED.8=98
col.RED.9=99
col.RED.ncol=9

*
* Extract colormap and ccolor
*
outstr1=split(color,',','head')
outstr2=split(color,',','tail')
if(outstr2!='')
  colormap=outstr1
  ccolor=outstr2
else
  firstchar1=substr(outstr1,1,1)
  if(!valnum(firstchar1))
    colormap=outstr1
    ccolor=''  
  else
    colormap=''
    ccolor=outstr1
  endif
endif

*
* Extract map1 and map2
*
if(colormap!='')
  mapncol1=split(colormap,'&','head')
  mapncol2=split(colormap,'&','tail')
  map1=split(mapncol1,'=','head')
  ncol1_user=split(mapncol1,'=','tail')
  if(valnum(ncol1_user) & ncol1_user<0)
    ncol1_user=-ncol1_user
    order1_user=-1
  endif
  map2=split(mapncol2,'=','head')
  ncol2_user=split(mapncol2,'=','tail')
  if(valnum(ncol2_user) & ncol2_user<0)
    ncol2_user=-ncol2_user
    order2_user=-1
  endif
  ncol1=col.map1.ncol
  ncol2=col.map2.ncol
  order1=1
  order2=1
  if(valnum(ncol1_user) & ncol1_user<ncol1)
    ncol1=ncol1_user
  endif
  if(valnum(ncol2_user) & ncol2_user<ncol2)
    ncol2=ncol2_user
  endif
  if(valnum(order1_user) & order1_user<order1)
    order1=order1_user
  endif
  if(valnum(order2_user) & order2_user<order2)
    order2=order2_user
  endif
endif

*
* Extract ccolor1 and ccolor2
*
if(ccolor!='')
  ccolor1=split(ccolor,'&','head')
  ccolor2=split(ccolor,'&','tail')
  if(ccolor2='')
    ccolor2=ccolor1
  endif
endif

*
* Extract cint1, cint2, ...
*
cint_rest=cint
cnt=1
cint.cnt=split(cint_rest,'&','head')
cint_rest=split(cint_rest,'&','tail')
cint_max=cint.cnt
while(cnt<=100)
cnt=cnt+1
cint.cnt=split(cint_rest,'&','head')
cint_rest=split(cint_rest,'&','tail')
if(cint.cnt!='')
cint_max=cint.cnt
else
cint.cnt=cint_max
endif
endwhile
cint=cint.1

*
* Calculate cumulative sum of cint1, cint2, ...
*
cnt=1
cintcsum.cnt=cint.cnt
while(cnt<=100)
cnt=cnt+1
cnt_pre=cnt-1
cintcsum.cnt=cintcsum.cnt_pre+cint.cnt
endwhile
cnt=1
while(cnt<=100)
cnt_neg=-cnt
cintcsum.cnt_neg=-cintcsum.cnt
cnt=cnt+1
endwhile

if(colormap!='' & ccolor!='')
  if(math_int(blackout)!=blackout)
    say '[shadcon ERROR] Shading and contour levels are incompatible because <blackout> is not integer.'
    return
  endif
  if(math_mod(offset,cint)!=0)
    say '[shadcon ERROR] Shading and contour levels are incompatible because <offset> is not a multiple of <cint>.'
    return
  endif
endif

*
* Plot shading
*
if(colormap!='')
* Set clevs for shading
  setclevs='set clevs'
  if(ncol1!=0)
    cnt=-(ncol1-1)
    while(cnt<=-1)
*     clev=(cnt-blackout)*cint+offset
      clev=cintcsum.cnt-blackout*cint+offset
      setclevs=setclevs' 'clev
      cnt=cnt+1
    endwhile
  endif
  if(blackout!=0)
    if(ncol1!=0)
      clev=-blackout*cint+offset
      setclevs=setclevs' 'clev
    endif
    if(ncol2!=0)
      clev=blackout*cint+offset
      setclevs=setclevs' 'clev
    endif
  else
    if(ncol1!=0 & ncol2!=0)
      clev=offset
      setclevs=setclevs' 'clev
    endif
  endif
  if(ncol2!=0)
    cnt=1
    while(cnt<=ncol2-1)
*     clev=(cnt+blackout)*cint+offset
      clev=cintcsum.cnt+blackout*cint+offset
      setclevs=setclevs' 'clev
      cnt=cnt+1
    endwhile
  endif
* Set ccols for shading
  setccols='set ccols'
  if(ncol1!=0)
    cnt=ncol1
    while(cnt>=1)
      if(order1=1)
        idx=cnt
      else
        idx=ncol1+1-cnt
      endif
      setccols=setccols' 'col.map1.idx
      cnt=cnt-1
    endwhile
  endif
  if(blackout!=0)
    setccols=setccols' '0
  endif
  if(ncol2!=0)
    cnt=1
    while(cnt<=ncol2)
      if(order2=1)
        idx=cnt
      else
        idx=ncol2+1-cnt
      endif
      setccols=setccols' 'col.map2.idx
      cnt=cnt+1
    endwhile
  endif
* Display shading
  'set gxout 'shading_style
  setclevs
  setccols
  'display 'var
endif

*
* Plot contours
*
if(ccolor!='')
  'set gxout contour'
  'set cint 'cint
  'set cstyle 2'
  'set ccolor 'ccolor1
  'set black 'offset-(blackout-0.5)*cint' 1e30'
  'display 'var
  'set cint 'cint
  'set cstyle 1'
  'set ccolor 'ccolor2
  'set black -1e30 'offset+(blackout-0.5)*cint
  'display 'var
  if(blackout=0)
    'set clevs 'offset
    'set cstyle 1'
    'set ccolor 'ccolor2
    'display 'var
  endif
endif

'set gxout contour'

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
say '  Plot 2-D graph using shading and/or contours with specified color and contour information.'
say ''
say '  USAGE 1: shadcon <color1>&<color2> <var> <cint1>[&<cint2>...] [<blackout> [grfill]]'
say '  USAGE 2: shadcon <ccolor1>[&<ccolor2>] ...'
say '  USAGE 3: shadcon <color1>&<color2>,<ccolor1>[&<ccolor2>] ...'
say '  USAGE 4: shadcon ... <offset> <var> <cint1>[&<cint2>...] [<blackout> [grfill]]'
say '    <color1>&<color2>: colormap for shading, e.g., blue&red. <color> can be blue, BLUE, red, RED, green, fuchsia, or gray, and # of color levels can be specified like "blue=5&red=5" (lighter colors toward center; default) or "blue=-5&red=-5" (darker colors toward center). Default (maximum) # of color levels is 9 for BLUE and RED, and 5 for others. No shading if not specified.'
say '    <ccolor1>[&<ccolor2>]: color for contours below and above <offset>, e.g., 4&2. No contours if not specified.'
say '    <offset>: offset to zero for shading/contour levels. Default=0.'
say '    <var>: variable to be plotted.'
say '    <cint1>[&<cint2>...]: shading interval (contour interval is always <cint1>). The last specified value will be repeated for each remaining shading.'
say '    <blackout>: values between -<blackout>*<cint1> and <blackout>*<cint1> will NOT be plotted.'
say '                All values will be plotted if <blackout>=0 (default).'
say '    grfill: use tiles instead of smooth contours for shading.'
say ''
say '  EXAMPLE 1: plot "sst" using a blue-to-red color map with a shading/contour interval of 0.1.'
say '    shadcon blue&red,1 sst 0.1'
say '    legend'
say ''
say '  EXAMPLE 2: as EXAMPLE 1, except without contours.'
say '    shadcon blue&red sst 0.1'
say '    legend'
say ''
say '  EXAMPLE 3: as EXAMPLE 2, except using grfill for shading.'
say '    shadcon blue&red sst 0.1 0 grfill'
say '    legend'
say ''
say '  EXAMPLE 4: as EXAMPLE 1, except without shading.'
say '    shadcon 1 sst 0.1'
say '    legend'
say ''
say '  Copyright (c) 2013-2020, Bin Guan.'
return
