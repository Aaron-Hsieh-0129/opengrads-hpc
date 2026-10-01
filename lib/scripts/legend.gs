***************************************************************************************
* $Id: legend.gs,v 1.84 2022/05/01 22:16:16 bguan Exp $
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
function legend(arg)
*
* Draw legend for current plot.
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

*
* Print usage.
*
'query gxinfo'
line1=sublin(result,1)
word4=subwrd(line1,4)
if(arg='-h' | word4='Clear')
  usage()
  return
endif

*
* Initialize options.
*
_.orientation.1='v'
_.xoffset.1=0
_.yoffset.1=0
_.scalefactor.1=1
_.unit.1=''
_.categorical.1=0
_.reverse.1=0
_.rotate.1=0

*
* Parse options.
*
rc=parseopt(arg,'-','orient','orientation')
rc=parseopt(arg,'-','xo','xoffset')
rc=parseopt(arg,'-','yo','yoffset')
rc=parseopt(arg,'-','scale','scalefactor')
if(!valnum(_.scalefactor.1) | _.scalefactor.1<=0)
  say '[legend ERROR] <scalefactor> must be numeric >0.'
  return
endif
rc=parseopt(arg,'-','u','unit')
rc=parseopt(arg,'-','cat','categorical')
num_text=parseopt(arg,'-','t','text')
rc=parseopt(arg,'-','rev','reverse')
rc=parseopt(arg,'-','rot','rotate')

*
* Get plot area.
*
  'query gxinfo'
  line3=sublin(result,3)
  line4=sublin(result,4)
  x1=subwrd(line3,4)
  x2=subwrd(line3,6)
  y1=subwrd(line4,4)
  y2=subwrd(line4,6)

*
* Get rvratio.
*
  'query pp2xy 0 0'
  tmpxa=subwrd(result,3)
  'query pp2xy 1 1'
  tmpxb=subwrd(result,3)
  rvratio=tmpxb-tmpxa

*
* Read header.
*
result=read(mytmpdir'/legend.txt.'randnum)
header=sublin(result,2)

***************************************************************************************
* For shading plot, place legend in right or bottom.
***************************************************************************************
if(subwrd(arg,1)='colorbar')
*
* Query legend data.
*
  'query shades'
  line1=sublin(result,1)
  ncol=subwrd(line1,5)
  if(_.categorical.1)
    nlev=ncol
  else
    nlev=ncol-1
  endif
  cnt=1
  while(cnt<=ncol)
    line=sublin(result,cnt+1)
    ccol.cnt=subwrd(line,1)
    cnt=cnt+1
  endwhile
  cnt=1
  while(cnt<=nlev)
    line=sublin(result,cnt+2)
    if(line!='')
      clev.cnt=subwrd(line,2)
    else
      cntm1=cnt-1
      clev.cnt='>'clev.cntm1
    endif
    cnt=cnt+1
  endwhile
  if(_.reverse.1)
    cnt=1
    while(cnt<=ncol/2)
      cup=ccol.cnt
      cnt2=ncol+1-cnt
      ccol.cnt=ccol.cnt2
      ccol.cnt2=cup
      cnt=cnt+1
    endwhile
    cnt=1
    while(cnt<=nlev/2)
      cup=clev.cnt
      cnt2=nlev+1-cnt
      clev.cnt=clev.cnt2
      clev.cnt2=cup
      cnt=cnt+1
    endwhile
    cnt=1
    while(cnt<=num_text/2)
      cup=_.text.cnt
      cnt2=num_text+1-cnt
      _.text.cnt=_.text.cnt2
      _.text.cnt2=cup
      cnt=cnt+1
    endwhile
  endif

*
* Define sizes and spacing.
*
  small_spacing=0.1
  small_spacing=small_spacing*rvratio
  wid=0.15*rvratio
  if(_.orientation.1='v')
    len=(y2-y1)*_.scalefactor.1
  else
    len=(x2-x1)*_.scalefactor.1
  endif
  seg=len/ncol
*
* Draw legend.
*
  if(_.orientation.1='v')
    x=x2+small_spacing*2.75+_.xoffset.1
    y=y1+(y2-y1)/2-len/2+_.yoffset.1
*   (x,y) is lower-left corner of colorbar
    cnt=1
    while(cnt<=ncol)
      'set line 'ccol.cnt
      'draw recf 'x' 'y+(cnt-1)*seg' 'x+wid' 'y+cnt*seg
      cnt=cnt+1
    endwhile
    cnt=1
    while(cnt<=ncol)
      'set line 1 1'
      'draw rec 'x' 'y+(cnt-1)*seg' 'x+wid' 'y+cnt*seg
      cnt=cnt+1
    endwhile
    cnt=1
    while(cnt<=nlev)
      _.rotate.1=math_mod(_.rotate.1,360)
      if(_.rotate.1<-90 | _.rotate.1>90)
        _.rotate.1=0
      endif
      if(_.rotate.1=0)
        'set string 1 l'
      endif
      if(_.rotate.1<0 & _.rotate.1>=-45)
        'set string 1 bl -1 '_.rotate.1
      endif
      if(_.rotate.1<-45)
        'set string 1 bc -1 '_.rotate.1
      endif
      if(_.rotate.1>0 & _.rotate.1<=45)
        'set string 1 tl -1 '_.rotate.1
      endif
      if(_.rotate.1>45)
        'set string 1 tc -1 '_.rotate.1
      endif
      if(_.categorical.1)
        if(num_text=0)
          'draw string 'x+wid+small_spacing/2' 'y+cnt*seg-seg/2' 'clev.cnt
        else
          'draw string 'x+wid+small_spacing/2' 'y+cnt*seg-seg/2' '_.text.cnt
        endif
      else
        'draw string 'x+wid+small_spacing/2' 'y+cnt*seg' 'clev.cnt
      endif
      cnt=cnt+1
    endwhile
    if(_.unit.1!='')
      'set string 1 c -1 90'
      'query string M'
      text_height=subwrd(result,4)
      'draw string 'x-text_height/2-small_spacing*0.75' 'y+len/2' '_.unit.1
    endif
  else
    x=x1+(x2-x1)/2-len/2+_.xoffset.1
    y=y1-small_spacing*4.00+_.yoffset.1
*   (x,y) is upper-left corner of colorbar
    cnt=1
    while(cnt<=ncol)
      'set line 'ccol.cnt
      'draw recf 'x+(cnt-1)*seg' 'y-wid' 'x+cnt*seg' 'y
      cnt=cnt+1
    endwhile
    cnt=1
    while(cnt<=ncol)
      'set line 1 1'
      'draw rec 'x+(cnt-1)*seg' 'y-wid' 'x+cnt*seg' 'y
      cnt=cnt+1
    endwhile
    cnt=1
    while(cnt<=nlev)
      _.rotate.1=math_mod(_.rotate.1,360)
      if(_.rotate.1<-90 | _.rotate.1>90)
        _.rotate.1=0
      endif
      if(_.rotate.1=0)
        'set string 1 tc'
      endif
      if(_.rotate.1<0 & _.rotate.1>=-45)
        'set string 1 tl -1 '_.rotate.1
      endif
      if(_.rotate.1<-45)
        'set string 1 l -1 '_.rotate.1
      endif
      if(_.rotate.1>0 & _.rotate.1<=45)
        'set string 1 tr -1 '_.rotate.1
      endif
      if(_.rotate.1>45)
        'set string 1 r -1 '_.rotate.1
      endif
      if(_.categorical.1)
        if(num_text=0)
          'draw string 'x+cnt*seg-seg/2' 'y-wid-small_spacing/2' 'clev.cnt
        else
          'draw string 'x+cnt*seg-seg/2' 'y-wid-small_spacing/2' '_.text.cnt
        endif
      else
        'draw string 'x+cnt*seg' 'y-wid-small_spacing/2' 'clev.cnt
      endif
      cnt=cnt+1
    endwhile
    if(_.unit.1!='')
      'set string 1 bc -1 0'
      'draw string 'x+len/2' 'y+small_spacing*0.75' '_.unit.1
    endif
  endif
  'set string 1 bl 3 0'
  return
endif

***************************************************************************************
* For mark plot, place legend in bottom-left.
***************************************************************************************
if(header='mark')
*
* Read legend data.
*
  flag=1
  cnt=0
  while(flag)
    result=read(mytmpdir'/legend.txt.'randnum)
    status=sublin(result,1)
    if(status!=0)
      flag=0
    else
      cnt=cnt+1
      line.cnt=sublin(result,2)
    endif
  endwhile
  num_var=cnt
*
* Define sizes and spacing.
*
  small_spacing=0.022
  small_spacing=small_spacing*rvratio*_.scalefactor.1
*
* Draw legend.
*
  max_size=0
  cnt=1
  while(cnt<=num_var)
    size=subwrd(line.cnt,2)
    if(size>max_size)
      max_size=size
    endif
    cnt=cnt+1
  endwhile
  'query string M'
  text_height=subwrd(result,4)
  if(text_height>max_size)
    text_mark_height=text_height
  else
    text_mark_height=max_size
  endif
  cnt=1
  while(cnt<=num_var)
    mag=subwrd(line.cnt,1)
    size=subwrd(line.cnt,2)
    mark=subwrd(line.cnt,3)
    color=subwrd(line.cnt,4)
    style=-1
    thick=-1
    wrdcnt=5
    text=subwrd(line.cnt,wrdcnt)
    while(subwrd(line.cnt,wrdcnt)!='')
      wrdcnt=wrdcnt+1
      text=text' 'subwrd(line.cnt,wrdcnt)
    endwhile
    if(mag=0)
      if(cnt=1)
        x=x1+small_spacing+0.5*size+_.xoffset.1
        y=y1+small_spacing+0.5*size+_.yoffset.1
      endif
      'set line 'color' 'style' 'thick
      'draw mark 'mark' 'x' 'y' 'size
      'set string 1 l'
      'draw string 'x+text_mark_height/2+small_spacing' 'y' 'text
      if(_.orientation.1='v')
        y=y-text_mark_height-small_spacing
      endif
      if(_.orientation.1='h')
        'query string 'text
        string_width=subwrd(result,4)
        x=x+text_mark_height+string_width+3*small_spacing
      endif
    endif
    if(mag!=0)
      x0=x1+_.xoffset.1
      y0=y1+_.yoffset.1
      mag_bigger=mag*2
      mag_smaller=mag/2
      size_bigger=size*math_sqrt(mag_bigger/mag)
      size_smaller=size*math_sqrt(mag_smaller/mag)
      'set line 'color
*     Bigger mark
      x=x0+small_spacing+0.5*size_bigger
      y=y0+small_spacing+0.5*size_bigger
      'draw mark 'mark' 'x' 'y' 'size_bigger
      'set string 1 l'
      x=x+0.5*size_bigger+small_spacing
      'draw string 'x' 'y' 'mag_bigger
      'query string 'mag_bigger
      strwid_bigger=subwrd(result,4)
*     Mark
      x=x+strwid_bigger+2*small_spacing+0.5*size
      'draw mark 'mark' 'x' 'y' 'size
      'set string 1 l'
      x=x+0.5*size+small_spacing
      'draw string 'x' 'y' 'mag
      'query string 'mag
      strwid=subwrd(result,4)
*     Smaller mark
      x=x+strwid+2*small_spacing+0.5*size_smaller
      'draw mark 'mark' 'x' 'y' 'size_smaller
      'set string 1 l'
      x=x+0.5*size_smaller+small_spacing
      'draw string 'x' 'y' 'mag_smaller
      'query string 'mag_smaller
      strwid_smaller=subwrd(result,4)
      'set line 1'
      'draw rec 'x0' 'y0' 'x+strwid_smaller+small_spacing' 'y0+2*small_spacing+size_bigger
      if(_.unit.1!='')
        'set string 1 l'
        'draw string 'x+strwid_smaller+2*small_spacing' 'y0+small_spacing+size_bigger/2' '_.unit.1
      endif
    endif
    cnt=cnt+1
  endwhile
  'set line 1 1 3'
  'set string 1 bl'
  return
endif

***************************************************************************************
* For line plot, place legend in upper-left.
***************************************************************************************
if(header='line')
*
* Read legend data.
*
  flag=1
  cnt=0
  while(flag)
    result=read(mytmpdir'/legend.txt.'randnum)
    status=sublin(result,1)
    if(status!=0)
      flag=0
    else
      cnt=cnt+1
      line.cnt=sublin(result,2)
    endif
  endwhile
  num_var=cnt
*
* Define sizes and spacing.
*
  line_length=0.55
  small_spacing=0.022
  if(_.scalefactor.1<1)
    line_length=line_length*_.scalefactor.1
  endif
  small_spacing=small_spacing*rvratio*_.scalefactor.1
*
* Draw legend.
*
  cnt=1
  while(cnt<=num_var)
    size=subwrd(line.cnt,2)
    if(cnt=1)
      x=x1+small_spacing+0.5*size+_.xoffset.1
      y=y2-small_spacing-0.5*size+_.yoffset.1
    endif
    mark=subwrd(line.cnt,1)
    style=subwrd(line.cnt,3)
    color=subwrd(line.cnt,4)
    thick=subwrd(line.cnt,5)
    wrdcnt=6
    text=subwrd(line.cnt,wrdcnt)
    while(subwrd(line.cnt,wrdcnt)!='')
      wrdcnt=wrdcnt+1
      text=text' 'subwrd(line.cnt,wrdcnt)
    endwhile
    if(substr(style,1,3)!='bar')
      'set line 'color' 'style' 'thick
      'draw mark 'mark' 'x' 'y' 'size
    endif
    if(substr(style,1,3)!='bar' & style!=0)
      if(mark=2|mark=4|mark=10|mark=11)
        'draw line 'x+0.5*size' 'y' 'x+line_length-0.5*size' 'y
      endif
      if(mark=7)
        'draw line 'x+0.34*size' 'y' 'x+line_length-0.34*size' 'y
      endif
      if(mark=8)
        'draw line 'x+0.27*size' 'y' 'x+line_length-0.27*size' 'y
      endif
      if(mark!=2&mark!=4&mark!=10&mark!=11&mark!=7&mark!=8)
        'draw line 'x' 'y' 'x+line_length' 'y
      endif
    endif
    if(substr(style,1,3)!='bar')
      'draw mark 'mark' 'x+line_length' 'y' 'size
    endif
    if(style=0)
      'draw mark 'mark' 'x+0.5*line_length' 'y' 'size
*     Above: If no line then draw a third mark in the middle
    endif
    if(substr(style,1,3)='bar')
      rec.bar='rec'; rec.barf='recf'
      'query gr2xy 0 0'
      tmpxa=subwrd(result,3)
      'query gr2xy 1 0'
      tmpxb=subwrd(result,3)
      bar_width=size
      cola=split(color,';','head')
      colb=split(color,';','tail')
      if(colb='')
        'set line 'cola' 1 'thick
        'draw 'rec.style' 'x' 'y-bar_width/2' 'x+line_length' 'y+bar_width/2
      else
        'set line 'cola' 1 'thick
        'draw 'rec.style' 'x' 'y-bar_width/2' 'x+0.5*line_length-small_spacing' 'y+bar_width/2
        'set line 'colb' 1 'thick
        'draw 'rec.style' 'x+0.5*line_length+small_spacing' 'y-bar_width/2' 'x+line_length' 'y+bar_width/2
      endif
    endif
    'set string 1 l'
    'draw string 'x+size/2+line_length+small_spacing' 'y' 'text
    if(_.orientation.1='v')
      'query string M'
      text_height=subwrd(result,4)
      if(text_height>size)
        text_mark_height=text_height
      else
        text_mark_height=size
      endif
      y=y-text_mark_height-small_spacing
    endif
    if(_.orientation.1='h')
      'query string 'text
      string_width=subwrd(result,4)
      x=x+size+line_length+string_width+3*small_spacing
    endif
    cnt=cnt+1
  endwhile
  'set line 1 1 3'
  'set string 1 bl'
  return
endif

***************************************************************************************
* For box-whisker plot, place legend in upper-left.
***************************************************************************************
if(header='boxplot')
*
* Read legend data.
*
  flag=1
  cnt=0
  while(flag)
    result=read(mytmpdir'/legend.txt.'randnum)
    status=sublin(result,1)
    if(status!=0)
      flag=0
    else
      cnt=cnt+1
      line.cnt=sublin(result,2)
    endif
  endwhile
  num_var=cnt
*
* Define sizes and spacing.
*
  small_spacing=0.022
  small_spacing=small_spacing*rvratio*_.scalefactor.1
*
* Draw legend.
*
  cnt=1
  while(cnt<=num_var)
    size=subwrd(line.cnt,1)
    if(cnt=1)
      x=x1+small_spacing+0.5*size+_.xoffset.1
      y=y2-small_spacing-0.5*size+_.yoffset.1
    endif
    color=subwrd(line.cnt,2)
    wrdcnt=3
    text=subwrd(line.cnt,wrdcnt)
    while(subwrd(line.cnt,wrdcnt)!='')
      wrdcnt=wrdcnt+1
      text=text' 'subwrd(line.cnt,wrdcnt)
    endwhile
    'set string 'color' l'
    'draw string 'x' 'y' 'text
    if(_.orientation.1='v')
      'query string M'
      text_height=subwrd(result,4)
      if(text_height>size)
        text_mark_height=text_height
      else
        text_mark_height=size
      endif
      y=y-text_mark_height-small_spacing
    endif
    if(_.orientation.1='h')
      'query string 'text
      string_width=subwrd(result,4)
      x=x+size+string_width+3*small_spacing
    endif
    cnt=cnt+1
  endwhile
  'set line 1 1 3'
  'set string 1 bl'
  return
endif

***************************************************************************************
* For Taylor plot, place legend in upper-left.
***************************************************************************************
if(header='taylor')
*
* Read legend data.
*
  flag=1
  cnt=0
  while(flag)
    result=read(mytmpdir'/legend.txt.'randnum)
    status=sublin(result,1)
    if(status!=0)
      flag=0
    else
      cnt=cnt+1
      line.cnt=sublin(result,2)
    endif
  endwhile
  num_var=cnt
*
* Determine if all symbols are identical
*
  AllSymbolSame=1
  _.mark.1=subwrd(line.1,1)
  _.size.1=subwrd(line.1,2)
  _.color.1=subwrd(line.1,3)
  cnt=2
  while(cnt<=num_var)
    _.mark.cnt=subwrd(line.cnt,1)
    _.size.cnt=subwrd(line.cnt,2)
    _.color.cnt=subwrd(line.cnt,3)
    AllSymbolSame=AllSymbolSame&(_.mark.cnt=_.mark.1)&(_.size.cnt=_.size.1)&(_.color.cnt=_.color.1)
    cnt=cnt+1
  endwhile
*
* Determine if no symbols are identical
*
  NoSymbolSame=1
  cnt2=1
  while(cnt2<=num_var-1)
    _.mark.cnt2=subwrd(line.cnt2,1)
    _.size.cnt2=subwrd(line.cnt2,2)
    _.color.cnt2=subwrd(line.cnt2,3)
    cnt=cnt2+1
    while(cnt<=num_var)
      _.mark.cnt=subwrd(line.cnt,1)
      _.size.cnt=subwrd(line.cnt,2)
      _.color.cnt=subwrd(line.cnt,3)
      NoSymbolSame=NoSymbolSame&((_.mark.cnt!=_.mark.cnt2)|(_.size.cnt!=_.size.cnt2)|(_.color.cnt!=_.color.cnt2))
      cnt=cnt+1
    endwhile
    cnt2=cnt2+1
  endwhile
*
* Define sizes and spacing.
*
  small_spacing=0.022
  small_spacing=small_spacing*rvratio*_.scalefactor.1
*
* Draw legend.
*
  cnt=1
  while(cnt<=num_var)
    size=subwrd(line.cnt,2)
    if(cnt=1)
      x=x1+small_spacing+0.5*size+_.xoffset.1
      y=y2-small_spacing-0.5*size+_.yoffset.1
      if((x2-x1)/(y2-y1)<1.5)
        y=y-0.25*rvratio
      else
        x=x-0.50*rvratio
        y=y+0.50*rvratio
      endif
    endif
    mark=subwrd(line.cnt,1)
    color=subwrd(line.cnt,3)
    wrdcnt=5
    text=subwrd(line.cnt,wrdcnt)
    if(text!='|')
      while(subwrd(line.cnt,wrdcnt+1)!='|')
        wrdcnt=wrdcnt+1
        text=text' 'subwrd(line.cnt,wrdcnt)
      endwhile
      'query string 'text
      text_width=subwrd(result,4)
      wrdcnt=wrdcnt+2
    else
      text=''
      text_width=0
      wrdcnt=6
    endif
    TEXT=subwrd(line.cnt,wrdcnt)
    while(subwrd(line.cnt,wrdcnt+1)!='')
      wrdcnt=wrdcnt+1
      TEXT=TEXT' 'subwrd(line.cnt,wrdcnt)
    endwhile
    'query string 'TEXT
    TEXT_width=subwrd(result,4)
    if(!AllSymbolSame)
      'set line 'color
      'draw mark 'mark' 'x' 'y' 'size
    endif
    'set string 1 l'
    if(!NoSymbolSame)
      'draw string 'x+size/2+small_spacing' 'y' 'text': 'TEXT
    else
      'draw string 'x+size/2+small_spacing' 'y' 'TEXT
    endif
    'query string M'
    text_height=subwrd(result,4)
    if(text_height>size)
      text_mark_height=text_height
    else
      text_mark_height=size
    endif
    if(_.orientation.1='v')
      y=y-text_mark_height-small_spacing
    endif
    if(_.orientation.1='h'&(!NoSymbolSame))
      x=x+size+text_width+TEXT_width+4*small_spacing
    endif
    if(_.orientation.1='h'&NoSymbolSame)
      x=x+size+TEXT_width+3*small_spacing
    endif
    cnt=cnt+1
  endwhile
  'set line 1'
  'set string 1 bl'
  return
endif

***************************************************************************************
* For vector plot, place legend in bottom-right.
***************************************************************************************
if(header='vector')
*
* Define spacing.
*
  small_spacing=0.022
  big_spacing=0.25
  small_spacing=small_spacing*rvratio
  big_spacing=big_spacing*rvratio
*
* Set X/Y.
*
  x=x2-small_spacing+_.xoffset.1
  y=y1-big_spacing+_.yoffset.1
*
* Draw legend.
*
  result=read(mytmpdir'/legend.txt.'randnum)
  line=sublin(result,2)
  arr_length=subwrd(line,1)
  arr_mag=subwrd(line,2)
  arr_color=subwrd(line,3)
  arr_thick=subwrd(line,4)
  if(_.unit.1!='')
    arr_mag=arr_mag' '_.unit.1
  endif
  'query string 'arr_mag
  arr_mag_strwid=subwrd(result,4)
  'set line 'arr_color' 1 'arr_thick
  'draw line 'x-arr_mag_strwid-arr_length' 'y' 'x-arr_mag_strwid' 'y
  'draw line 'x-arr_mag_strwid-0.0866' 'y+0.05' 'x-arr_mag_strwid' 'y
  'draw line 'x-arr_mag_strwid-0.0866' 'y-0.05' 'x-arr_mag_strwid' 'y
  'set string 'arr_color' l'
  'draw string 'x-arr_mag_strwid' 'y' 'arr_mag
  'set line 1 1 3'
  'set string 1 bl'
  return
endif

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
say '  Draw legend for current plot.'
say ''
say '  USAGE 1: legend [-orient v|h] [-xo <xoffset>] [-yo <yoffset>] [-scale <scalefactor>] [-u <unit>]'
say '  USAGE 2: legend colorbar [-cat 1] [-t <text>] [-rev 1] [-rot <degree>] ...'
say '    v|h: v=vertically oriented (default), h=horizontally oriented.'
say '    <xoffset>: horizontal offset to default position. Default=0.'
say '    <yoffset>: vertical offset to default position. Default=0.'
say '    <scalefactor>: scale factor for line length and space. Default=1.'
say '    <unit>: unit label placed near legend for mark, shading, or vector plot.'
say '    colorbar: for shading plot of regular data (each color represents a range of values).'
say '    colorbar -cat 1: for shading plot of categorical data (each color represents a unique value/category).'
say '    <text>: label for each category. Text beginning with a minus sign or containing spaces must be double quoted.'
say '    -rev 1: reverse direction of legend (both colors and labels).'
say '    <degree>: number of degrees to rotate direction of labels for horizontally oriented colorbar. Default=0.'
say ''
say '  DEPENDENCIES: parsestr.gsf'
say ''
say '  SEE ALSO: shadcon.gs drawmark.gs plot.gs taylor.gs vector.gs'
say ''
say '  Copyright (c) 2012-2020, Bin Guan.'
return
