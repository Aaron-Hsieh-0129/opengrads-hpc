***************************************************************************************
* $Id: drawstr.gs,v 1.66 2021/06/09 21:22:22 bguan Exp $
*
* Copyright (c) 2005-2020, Bin Guan
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
function drawstr(arg)
*
* Annotate current plot.
*
rc=gsfallow('on')

*
* Parse -T option (label for an individual panel).
*
num_TXT=parseopt(arg,'-','T','TEXT')

*
* Parse -t option (label for a multi-panel plot).
*
num_txt=parseopt(arg,'-','t','text')
if(num_TXT+num_txt=0 | num_TXT*num_txt>0)
  usage()
  return
endif

*
* Initialize other options.
*
if(num_TXT>0)
  cnt=1
  while(cnt<=num_TXT)
    _.position.cnt=cnt
    _.color.cnt=1
    _.size.cnt=0.18
    _.thickness.cnt=-1
    _.xoffset.cnt=0
    _.yoffset.cnt=0
    cnt=cnt+1
  endwhile
endif
if(num_txt>0)
  x=0
  y=0
  cnt=1
  while(cnt<=num_txt)
    _.position.cnt=cnt
    _.color.cnt=1
    _.thickness.cnt=-1
    _.xoffset.cnt=0
    _.yoffset.cnt=0
    cnt=cnt+1
  endwhile
endif

*
* Parse -p option (position).
*
ps=13
p.1='tl'
p.2='tc'
p.3='tr'
p.4='bl'
p.5='br'
p.6='bc'
p.7='b25'
p.8='b75'
p.9='l'
p.10='r'
p.11='tl2'
p.12='tr2'
p.13='corr'
rc=parseopt(arg,'-','p','position')
cnt=1
while(cnt<=num_txt)
  if(!valnum(_.position.cnt))
    p_cnt=1
    flag=0
    while(p_cnt<=ps)
      flag=flag | (_.position.cnt=p.p_cnt)
      p_cnt=p_cnt+1
    endwhile
    if(!flag)
      say '[drawstr ERROR] Invalid <position>.'
      say ''
      return
    endif
  else
    if(valnum(_.position.cnt)=2 | _.position.cnt<1 | _.position.cnt>13)
      say '[drawstr ERROR] <position> must be integer within 1-13.'
      say ''
      return
    endif
  endif
  cnt=cnt+1
endwhile

*
* Parse -c option (color).
*
colorrc=parseopt(arg,'-','c','color')

*
* Parse -z option (size).
*
sizerc=parseopt(arg,'-','z','size')

*
* Parse -k option (thickness).
*
thicknessrc=parseopt(arg,'-','k','thickness')

*
* Parse -b option (background color).
*
backgroundrc=parseopt(arg,'-','b','background')

*
* Parse -xo option (x offset).
*
xoffsetrc=parseopt(arg,'-','xo','xoffset')

*
* Parse -yo option (y offset).
*
yoffsetrc=parseopt(arg,'-','yo','yoffset')

*
* Draw <TEXT>.
*
if(num_TXT>0)
  'set vpage off'
  'set parea off'
  cnt=1
  while(cnt<=num_TXT)
    while(_.TEXT.cnt='')
      cnt=cnt+1
    endwhile
    if(cnt>num_TXT)
      return
    endif
    lastchar=substr(_.position.cnt,strlen(_.position.cnt),1)
    if(valnum(lastchar))
      anchoridx=_.position.cnt
      direction='t'
    else
      anchoridx=substr(_.position.cnt,1,strlen(_.position.cnt)-1)
      direction=substr(_.position.cnt,strlen(_.position.cnt),1)
    endif
    anchoridx1=split(anchoridx,'&','head')
    anchoridx2=split(anchoridx,'&','tail')
    if(anchoridx2='')
      'query defval vpagexa'anchoridx' 1 1'
      vpagexa=subwrd(result,3)
      'query defval vpagexb'anchoridx' 1 1'
      vpagexb=subwrd(result,3)
      'query defval vpageya'anchoridx' 1 1'
      vpageya=subwrd(result,3)
      'query defval vpageyb'anchoridx' 1 1'
      vpageyb=subwrd(result,3)
      if(direction='t')
        x=vpagexa+(vpagexb-vpagexa)/2
        y=vpageya-0.44
        'set string '_.color.cnt' bc '_.thickness.cnt' 0'
      endif
      if(direction='b')
        x=vpagexa+(vpagexb-vpagexa)/2
        y=vpageyb+0.44
        'set string '_.color.cnt' tc '_.thickness.cnt' 0'
      endif
      if(direction='l')
        x=vpagexa+0.44
        y=vpageya-(vpageya-vpageyb)/2
        'set string '_.color.cnt' c '_.thickness.cnt' 90'
      endif
      if(direction='r')
        x=vpagexb-0.44
        y=vpageya-(vpageya-vpageyb)/2
        'set string '_.color.cnt' c '_.thickness.cnt' 270'
      endif
    else
      'query defval vpagexa'anchoridx1' 1 1'
      vpagexa1=subwrd(result,3)
      'query defval vpagexb'anchoridx1' 1 1'
      vpagexb1=subwrd(result,3)
      'query defval vpageya'anchoridx1' 1 1'
      vpageya1=subwrd(result,3)
      'query defval vpageyb'anchoridx1' 1 1'
      vpageyb1=subwrd(result,3)
      'query defval vpagexa'anchoridx2' 1 1'
      vpagexa2=subwrd(result,3)
      'query defval vpagexb'anchoridx2' 1 1'
      vpagexb2=subwrd(result,3)
      'query defval vpageya'anchoridx2' 1 1'
      vpageya2=subwrd(result,3)
      'query defval vpageyb'anchoridx2' 1 1'
      vpageyb2=subwrd(result,3)
      if(direction='t')
        x=(vpagexb1+vpagexa2)/2
        y=vpageya1-0.44
        'set string '_.color.cnt' bc '_.thickness.cnt' 0'
      endif
      if(direction='b')
        x=(vpagexb1+vpagexa2)/2
        y=vpageyb1+0.44
        'set string '_.color.cnt' tc '_.thickness.cnt' 0'
      endif
      if(direction='l')
        x=vpagexa1+0.44
        y=(vpageyb1+vpageya2)/2
        'set string '_.color.cnt' c '_.thickness.cnt' 90'
      endif
      if(direction='r')
        x=vpagexb1-0.44
        y=(vpageyb1+vpageya2)/2
        'set string '_.color.cnt' c '_.thickness.cnt' 270'
      endif
    endif
    x=x+_.xoffset.cnt
    y=y+_.yoffset.cnt
    'set strsiz '_.size.cnt
    'draw string 'x' 'y' '_.TEXT.cnt
    cnt=cnt+1
  endwhile
  'set string 1 bl 3 0'
  return
endif

*
* Get plot area
*
'query gxinfo'
line3=sublin(result,3)
line4=sublin(result,4)
x1=subwrd(line3,4)
x2=subwrd(line3,6)
y1=subwrd(line4,4)
y2=subwrd(line4,6)
x25=x1+(x2-x1)/4
x50=x1+(x2-x1)/2
x75=x1+(x2-x1)/4*3
y50=y1+(y2-y1)/2

*
* Define spacing.
*
small_spacing=0.05
big_spacing=0.2
'query pp2xy 0 0'
tmpxa=subwrd(result,3)
'query pp2xy 1 1'
tmpxb=subwrd(result,3)
rvratio=tmpxb-tmpxa
small_spacing=small_spacing*rvratio
big_spacing=big_spacing*rvratio

*
* Draw <text>.
*
cnt=1
while(cnt<=num_txt)
  while(_.text.cnt='')
    cnt=cnt+1
  endwhile
  if(cnt>num_txt)
    return
  endif
  if(_.position.cnt=1 | _.position.cnt=p.1)
    x=x1
    y=y2+small_spacing
    'set string '_.color.cnt' bl '_.thickness.cnt' 0'
  endif
  if(_.position.cnt=2 | _.position.cnt=p.2)
    x=x50
    y=y2+small_spacing
    'set string '_.color.cnt' bc '_.thickness.cnt' 0'
  endif
  if(_.position.cnt=3 | _.position.cnt=p.3)
    x=x2
    y=y2+small_spacing
    'set string '_.color.cnt' br '_.thickness.cnt' 0'
  endif
  if(_.position.cnt=4 | _.position.cnt=p.4)
    x=x1+small_spacing
    y=y1+small_spacing
    'set string '_.color.cnt' bl '_.thickness.cnt' 0'
  endif
  if(_.position.cnt=5 | _.position.cnt=p.5)
    x=x2-small_spacing
    y=y1+small_spacing
    'set string '_.color.cnt' br '_.thickness.cnt' 0'
  endif
  if(_.position.cnt=6 | _.position.cnt=p.6)
    x=x50
    y=y1-big_spacing
    'set string '_.color.cnt' tc '_.thickness.cnt' 0'
  endif
  if(_.position.cnt=7 | _.position.cnt=p.7)
    x=x25
    y=y1-big_spacing
    'set string '_.color.cnt' tc '_.thickness.cnt' 0'
  endif
  if(_.position.cnt=8 | _.position.cnt=p.8)
    x=x75
    y=y1-big_spacing
    'set string '_.color.cnt' tc '_.thickness.cnt' 0'
  endif
  if(_.position.cnt=9 | _.position.cnt=p.9)
    x=x1-2.2*big_spacing
    y=y50
    'set string '_.color.cnt' c '_.thickness.cnt' 90'
  endif
  if(_.position.cnt=10 | _.position.cnt=p.10)
    x=x2+2.2*big_spacing
    y=y50
    'set string '_.color.cnt' c '_.thickness.cnt' 270'
  endif
  if(_.position.cnt=11 | _.position.cnt=p.11)
    x=x1+small_spacing
    y=y2-small_spacing
    'set string '_.color.cnt' tl '_.thickness.cnt' 0'
  endif
  if(_.position.cnt=12 | _.position.cnt=p.12)
    x=x2-small_spacing
    y=y2-small_spacing
    'set string '_.color.cnt' tr '_.thickness.cnt' 0'
  endif
  if(_.position.cnt=13 | _.position.cnt=p.13)
    if((x2-x1)/(y2-y1)<1.5)
      x=x1+(x2-x1)*1.414/2+1.55*big_spacing
      y=y1+(y2-y1)*1.414/2+1.55*big_spacing
      'set string '_.color.cnt' bc '_.thickness.cnt' -45'
    else
      x=x50
      y=y2+2.2*big_spacing
      'set string '_.color.cnt' bc '_.thickness.cnt' 0'
    endif
  endif
  x=x+_.xoffset.cnt
  y=y+_.yoffset.cnt
  if(sizerc=num_txt)
*   Need the above if to prevent possible errors since _.size.cnt has no initial value defined for "-t".
    'set strsiz '_.size.cnt*rvratio
  endif
  if(backgroundrc=num_txt)
    'query string '_.text.cnt
    string_width=subwrd(result,4)
    'query string M'
    string_height=subwrd(result,4)
    if(_.position.cnt=4 | _.position.cnt=p.4)
      recf_left=x1
      recf_lower=y1
    endif
    if(_.position.cnt=5 | _.position.cnt=p.5)
      recf_left=x2-string_width-2*small_spacing
      recf_lower=y1
    endif
    if(_.position.cnt=11 | _.position.cnt=p.11)
      recf_left=x1
      recf_lower=y2-string_height-2*small_spacing
    endif
    if(_.position.cnt=12 | _.position.cnt=p.12)
      recf_left=x2-string_width-2*small_spacing
      recf_lower=y2-string_height-2*small_spacing
    endif
    recf_left=recf_left+_.xoffset.cnt
    recf_lower=recf_lower+_.yoffset.cnt
    recf_right=recf_left+string_width+2*small_spacing
    recf_upper=recf_lower+string_height+2*small_spacing
    'set line '_.background.cnt
    'draw recf 'recf_left' 'recf_lower' 'recf_right' 'recf_upper
    'set line 1'
    'draw rec 'recf_left' 'recf_lower' 'recf_right' 'recf_upper
  endif
  'draw string 'x' 'y' '_.text.cnt
  cnt=cnt+1
endwhile
'set string 1 bl -1 0'
return

return
***************************************************************************************
function parseopt(instr,optprefix,optname,outname)
*
* Parse an option, store argument(s) in a global variable array.
*
rc=gsfallow('on')
cnt=1
cnt2=0
while(subwrd(instr,cnt)!='')
  if(subwrd(instr,cnt)=optprefix''optname)
    cnt=cnt+1
    word=subwrd(instr,cnt)
    while(word!='' & (valnum(word)!=0 | substr(word,1,1)''999!=optprefix''999))
      cnt2=cnt2+1
      _.outname.cnt2=parsestr(instr,cnt)
      cnt=_end_wrd_idx+1
      word=subwrd(instr,cnt)
    endwhile
  endif
  cnt=cnt+1
endwhile
return cnt2
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
say '  Annotate current plot.'
say ''
say '  USAGE 1: drawstr -t <text1> [<text2>...] [-p <position1> [<position2>...]] [-c <color1> [<color2>...]] [-z <size1> [<size2>...]]'
say '           [-k <thickness1> [<thickness2>...]] [-b <background1> [<background2>...]] [-xo <xoffset1> [<xoffset2>...]] [-yo <yoffset1> [<yoffset2>...]]'
say '  USAGE 2: drawstr -T <TEXT1> [<TEXT2>...] [-p <position1> [<position2>...]] [-c <color1> [<color2>...]] [-z <size1> [<size2>...]]'
say '           [-k <thickness1> [<thickness2>...]] [-xo <xoffset1> [<xoffset2>...]] [-yo <yoffset1> [<yoffset2>...]]'
say '    <text>: label for an individual panel. Text beginning with a minus sign or containing spaces must be double quoted.'
say '    <TEXT>: label for a multi-panel plot (e.g., main title, column title, etc.). Text beginning with a minus sign or containing spaces must be double quoted.'
say '    <position>: position of <text> or <TEXT>.'
say '                For <text>, refer to schematic below. For <TEXT>, use <idx>t|b|l|r, where <idx> is from subplot.gs and t|b|l|r refers to top|bottom|left|right.'
say '                Default="1 2 3..." for <text>, and "1t 2t 3t..." for <TEXT>.'
say '    <color>: text color. Default=1.'
say '    <size>: text size. Current setting is used for <text> if unset. Default=0.18 for <TEXT>.'
say '    <thickness>: text thickness.'
say '    <background>: background color of text. Applicable to text inside plotting area only.'
say '    <xoffset>: horizontal offset to default position. Default=0.'
say '    <yoffset>: vertical offset to default position. Default=0.'
say ''
say '                 <TEXT>'
say ''
say '    1               2               3'
say '    +-------------------------------+'
say '    |11                           12|'
say '    |                               |'
say '    |                               |'
say '   9|           Plot Area           |10'
say '    |                               |'
say '    |                               |'
say '    |4                             5|'
say '    +-------------------------------+'
say '            7       6       8        '
say ''
say '  NOTE: "-T" and "-t" options cannot be used together.'
say ''
say '  EXAMPLE 1: add axis labels.'
say '    drawstr -p 6 9 -t Longitude Latitude'
say ''
say '  EXAMPLE 2: add column titles for a 3 rows by 2 columns plot.'
say '    subplot 6 1'
say '    ...'
say '    subplot 6 6'
say '    ...'
say '    drawstr -p 1t 4t -T "Column A" "Column B"'
say ''
say '  DEPENDENCIES: parsestr.gsf'
say ''
say '  Copyright (c) 2005-2020, Bin Guan.'
return
