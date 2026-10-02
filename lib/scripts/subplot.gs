***************************************************************************************
* $Id: subplot.gs,v 1.104 2021/06/09 05:05:31 bguan Exp $
*
* Copyright (c) 2006-2016, Bin Guan
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
function subplot(arg)
*
* Prepare plotting area for a multi-panel plot.
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
* Read input.
*
ntot=subwrd(arg,1)
idx=subwrd(arg,2)
if(idx='')
  usage()
  return
endif
wrd3=subwrd(arg,3)
if(valnum(wrd3))
  ncol=wrd3
else
  ncol=2
endif
if(valnum(ntot)!=1 | ntot<1)
  say '[subplot ERROR] <ntot> must be integer >=1.'
  return
endif
if(valnum(idx)!=1 | idx<1)
  say '[subplot ERROR] <idx> must be integer >=1.'
  return
endif
nrow=ntot/ncol
if(nrow!=math_int(nrow))
  nrow=math_int(nrow)+1
endif

*
* Initialize.
*
_.isrowmajor.1=0
_.xyratio.1=0
_.istight.1=0
_.isxtight.1=0
_.isytight.1=0
_.morexspace.1=0
_.moreyspace.1=0
_.morexpad.1=0
_.moreypad.1=0
_.pareawid.1=0
_.pareahgt.1=0
_.xappend.1=0
_.yappend.1=0
rc=parseopt(arg,'-','rowmajor','isrowmajor')
rc=parseopt(arg,'-','xy','xyratio')
rc=parseopt(arg,'-','tight','istight')
rc=parseopt(arg,'-','xtight','isxtight')
rc=parseopt(arg,'-','ytight','isytight')
rc=parseopt(arg,'-','xs','morexspace')
rc=parseopt(arg,'-','ys','moreyspace')
rc=parseopt(arg,'-','xp','morexpad')
rc=parseopt(arg,'-','yp','moreypad')
rc=parseopt(arg,'-','x','pareawid')
rc=parseopt(arg,'-','y','pareahgt')
rc=parseopt(arg,'-','xappend','xappend')
rc=parseopt(arg,'-','yappend','yappend')

if(!valnum(_.xyratio.1))
  say '[subplot ERROR] <xyratio> must be numeric.'
  return
endif
if(!valnum(_.morexspace.1))
  say '[subplot ERROR] <xspace> must be numeric.'
  return
endif
if(!valnum(_.moreyspace.1))
  say '[subplot ERROR] <yspace> must be numeric.'
  return
endif
if(!valnum(_.morexpad.1) | _.morexpad.1<0)
  say '[subplot ERROR] <xpad> must be numeric >=0.'
  return
endif
if(!valnum(_.moreypad.1) | _.moreypad.1<0)
  say '[subplot ERROR] <ypad> must be numeric >=0.'
  return
endif
if(!valnum(_.pareawid.1) | _.pareawid.1<0)
  say '[subplot ERROR] <pareawid> must be numeric >=0.'
  return
endif
if(!valnum(_.pareahgt.1) | _.pareahgt.1<0)
  say '[subplot ERROR] <pareahgt> must be numeric >=0.'
  return
endif

*
* Make and open a .ctl file (no .dat file) if no file is open (for running when no file is open, e.g., when plotting a Taylor diagram).
*
'query dims'
file_created=0
if(sublin(result,1)='No files open')
  ctllines=11
  ctlline.1='dset ^%y4.dat'
  ctlline.2='undef -9.99e8'
  ctlline.3='options template'
  ctlline.4='title intentionally left blank.'
  ctlline.5='xdef 2 levels 0 216'
  ctlline.6='ydef 2 levels -90 90'
  ctlline.7='zdef 1 levels 1000'
  ctlline.8='tdef 1 linear 01jan0001 1dy'
  ctlline.9='vars 1'
  ctlline.10='var 0 99 var'
  ctlline.11='endvars'
  cnt=1
  while(cnt<=ctllines)
    status=write(mytmpdir'/subplot.ctl.'randnum,ctlline.cnt)
    cnt=cnt+1
  endwhile
  status=close(mytmpdir'/subplot.ctl.'randnum)
  'open 'mytmpdir'/subplot.ctl.'randnum
  file_created=1
endif

qdims(0,'mydim')

'set vpage off'
'set parea off'

*
* Needed because xlab/ylab could have been turned off earlier for a tight plot.
*
'set xlab on'
'set ylab on'

*
* Get aspect ratio of plot area.
*
num_varying_dim=(_.mydim.xsO!=_.mydim.xeO)+(_.mydim.ysO!=_.mydim.yeO)+(_.mydim.zsO!=_.mydim.zeO)+(_.mydim.tsO!=_.mydim.teO)
if(num_varying_dim<1 | num_varying_dim>2)
  say '[subplot ERROR] # of varying dimensions must be 1 or 2. Current setting is'
  'query dims'
  cnt=2
  while(cnt<=6)
    say ' 'sublin(result,cnt)
    cnt=cnt+1
  endwhile
  return
endif
'query gxinfo'
line6=sublin(result,6)
mproj=subwrd(line6,3)
if(num_varying_dim=1)
*
* For line graph, use fixed aspect ratio.
*
  xy_ratio=1
else
  if(_.mydim.xsO!=_.mydim.xeO & _.mydim.ysO!=_.mydim.yeO & mproj=2)
*
*   For Lat/Lon projection, aspect ratio is calculated.
*
    multi_factor=(5/3)/(360/180)
    lonlat_ratio=(_.mydim.loneO-_.mydim.lonsO)/(_.mydim.lateO-_.mydim.latsO)
    xy_ratio=multi_factor*lonlat_ratio
  else
*
*   For all other 2-D maps, aspect ratio is obtained by test-plotting (using current dimension setting).
*
    'set grid off'
    'set grads off'
    'set frame off'
    'set xlab off'
    'set ylab off'
    'set mpdraw off'
    'set gxout contour'
    'set clevs -1e9'
    'display lat'
    'query gxinfo'
    line3=sublin(result,3)
    line4=sublin(result,4)
    tmpa=subwrd(line3,4)
    tmpb=subwrd(line3,6)
    tmpc=subwrd(line4,4)
    tmpd=subwrd(line4,6)
    xy_ratio=(tmpb-tmpa)/(tmpd-tmpc)
    'set grid on'
    'set grads on'
    'set frame on'
    'set xlab on'
    'set ylab on'
    'set mpdraw on'
    'set gxout contour'
  endif
endif
if(_.xyratio.1)
  xy_ratio=_.xyratio.1
endif

*
* Set up margins/spacing/padding.
*
'query gxinfo'
line2=sublin(result,2)
realpagewid=subwrd(line2,4)
realpagehgt=subwrd(line2,6)
marginleft=0.25
marginright=0.25
margintop=0.5
marginbottom=0.5
*xspace0=0
*yspace0=0
*xpad0=0.33
*ypad0=0.22
xspace0=-1.1
yspace0=-1.1
xpad0=0.88
ypad0=0.77
xspace=xspace0+_.morexspace.1
yspace=yspace0+_.moreyspace.1
xpad=xpad0+_.morexpad.1
ypad=ypad0+_.moreypad.1
if(_.istight.1)
  xspace=-2*xpad
  yspace=-2*ypad
endif
if(_.isxtight.1)
  xspace=-2*xpad
endif
if(_.isytight.1)
  yspace=-2*ypad
endif

*
* Set z- and t-dimension to be fixed because later I use "query defval 1 1" to pass values between sub-plots.
* If z/t is not fixed, "query defval 1 1" may get undefined values if sub-plots are for different z/t's.
*
'set z 1'
'set t 1'

*
* Get virtual page width and height, automatically fitting page width or height.
*
if(_.pareawid.1=0 & _.pareahgt.1=0)
  pareawid_fitwid=(realpagewid-marginleft-marginright-2*xpad*ncol-xspace*(ncol-1))/ncol
  pareahgt_fitwid=pareawid_fitwid/xy_ratio
  pareahgt_fithgt=(realpagehgt-margintop-marginbottom-2*ypad*nrow-yspace*(nrow-1))/nrow
  if(pareahgt_fitwid<pareahgt_fithgt)
    pareawid=pareawid_fitwid
    pareahgt=pareawid/xy_ratio
  else
    pareahgt=pareahgt_fithgt
    pareawid=pareahgt*xy_ratio
  endif
else
  if(_.pareawid.1!=0)
    pareawid=_.pareawid.1
    pareahgt=pareawid/xy_ratio
  else
    pareahgt=_.pareahgt.1
    pareawid=pareahgt*xy_ratio
  endif
endif
vpagewid=pareawid+2*xpad
vpagehgt=pareahgt+2*ypad

*
* Exit if parea is too large (as a result of arbitrary parea width or height).
*
if(pareawid*ncol+marginleft+marginright+2*xpad*ncol+xspace*(ncol-1)>realpagewid | pareahgt*nrow+margintop+marginbottom+2*ypad*nrow+yspace*(nrow-1)>realpagehgt)
  say '[subplot ERROR] cannot fit into page; try reducing <pareawid> or <pareahgt>.'
  return
endif

*
* Exit if plots to be appended cannot fit into rest of page.
*
if(idx=1)
  if(_.xappend.1=1)
    'query defval vpagexbmax 1 1'
    vpage_xb_max=subwrd(result,3)
    if(vpagewid*ncol+xspace*ncol>realpagewid-vpage_xb_max-marginright)
      say '[subplot ERROR] cannot fit into page; try reducing # of columns to be appended.'
      return
    endif
  endif
  if(_.yappend.1=1)
    'query defval vpageybmin 1 1'
    vpage_yb_min=subwrd(result,3)
    if(vpagehgt*nrow+yspace*nrow>vpage_yb_min-marginbottom)
      say '[subplot ERROR] cannot fit into page; try reducing # of rows to be appended.'
      return
    endif
  endif
endif

*
* Get virtual page boundaries.
* (xa,ya)---------
*    |           |
*    |   vpage   |
*    |           |
*    ---------(xb,yb)
*
if(_.isrowmajor.1=0)
  row_coordinate=idx-math_int((idx-1)/nrow)*nrow
  col_coordinate=math_int((idx-1)/nrow)+1
  idx_above=idx-1
  idx_left=idx-nrow
else
  col_coordinate=idx-math_int((idx-1)/ncol)*ncol
  row_coordinate=math_int((idx-1)/ncol)+1
  idx_left=idx-1
  idx_above=idx-ncol
endif
if(idx=1)
  if(_.xappend.1=0)
    'define vpagexa'idx'=0+'marginleft
  else
    'define vpagexa'idx'=vpagexbmax+'xspace
  endif
  if(_.yappend.1=0)
    'define vpageya'idx'='realpagehgt'-'margintop
  else
    'define vpageya'idx'=vpageybmin-'yspace
  endif
endif
if(col_coordinate=1 & idx!=1)
  'define vpagexa'idx'=vpagexa'idx_above
  'define vpageya'idx'=vpageyb'idx_above'-'yspace
endif
if(row_coordinate=1 & idx!=1)
  'define vpagexa'idx'=vpagexb'idx_left'+'xspace
  'define vpageya'idx'=vpageya'idx_left
endif
if(col_coordinate!=1 & row_coordinate!=1)
  'define vpagexa'idx'=vpagexb'idx_left'+'xspace
  'define vpageya'idx'=vpageyb'idx_above'-'yspace
endif
'define vpagexb'idx'=vpagexa'idx'+'vpagewid
'define vpageyb'idx'=vpageya'idx'-'vpagehgt

*
* Record rightmost and lowest virtual page boundaries for later use.
*
if(idx=1)
  'define vpagexbmax=vpagexb'idx
  'define vpageybmin=vpageyb'idx
else
  'query defval vpagexb'idx' 1 1'
  vpage_xb=subwrd(result,3)
  'query defval vpagexbmax 1 1'
  vpage_xb_max=subwrd(result,3)
  if(vpage_xb>vpage_xb_max)
    'define vpagexbmax=vpagexb'idx
  endif
  'query defval vpageyb'idx' 1 1'
  vpage_yb=subwrd(result,3)
  'query defval vpageybmin 1 1'
  vpage_yb_min=subwrd(result,3)
  if(vpage_yb<vpage_yb_min)
    'define vpageybmin=vpageyb'idx
  endif
endif

*
* Set virtual page boundaries.
*
'query defval vpagexa'idx' 1 1'
vpage_xa=subwrd(result,3)
'query defval vpagexb'idx' 1 1'
vpage_xb=subwrd(result,3)
'query defval vpageya'idx' 1 1'
vpage_ya=subwrd(result,3)
'query defval vpageyb'idx' 1 1'
vpage_yb=subwrd(result,3)
'set vpage 'vpage_xa' 'vpage_xb' 'vpage_yb' 'vpage_ya

*
* Set plotting area.
*
'query gxinfo'
line2=sublin(result,2)
psudopagewid=subwrd(line2,4)
psudopagehgt=subwrd(line2,6)
if(psudopagewid=realpagewid)
  rvratio=realpagewid/vpagewid
else
  rvratio=realpagehgt/vpagehgt
endif
pareawid=pareawid*rvratio
pareahgt=pareahgt*rvratio
parea_xa=(psudopagewid-pareawid)/2
parea_xb=psudopagewid-(psudopagewid-pareawid)/2
parea_ya=psudopagehgt-(psudopagehgt-pareahgt)/2
parea_yb=(psudopagehgt-pareahgt)/2
if(parea_xa<0);parea_xa=0;endif
if(parea_xb>=psudopagewid);parea_xb=psudopagewid-5e-5;endif
if(parea_ya>=psudopagehgt);parea_ya=psudopagehgt-5e-5;endif
if(parea_yb<0);parea_yb=0;endif
'set parea 'parea_xa' 'parea_xb' 'parea_yb' 'parea_ya

*
* Set label sizes (optional).
*
'set clopts -1 -1 '0.07*rvratio
'set xlopts 1 -1 '0.09*rvratio
'set ylopts 1 -1 '0.09*rvratio
'set strsiz '0.11*rvratio
say '[subplot info] Current setting is'
say ' aspect ratio = 'math_format('%.3f',xy_ratio)'.'
say ' set clopts -1 -1 'math_format('%.3f',0.07*rvratio)
say ' set xlopts 1 -1 'math_format('%.3f',0.09*rvratio)
say ' set ylopts 1 -1 'math_format('%.3f',0.09*rvratio)
say ' set strsiz 'math_format('%.3f',0.11*rvratio)

*
* Set xlab, ylab (optional).
*
if(_.istight.1=1 | _.isxtight.1=1)
  if(col_coordinate=1)
    'set ylab on'
  else
    'set ylab off'
  endif
endif
if(_.istight.1=1 | _.isytight.1=1)
  if(row_coordinate=nrow)
    'set xlab on'
  else
    'set xlab off'
  endif
endif

*
* Restore original dimension environment.
*
_.mydim.resetx
_.mydim.resety
_.mydim.resetz
_.mydim.resett

if(file_created=1)
  'close 'file_number()
endif

return
***************************************************************************************
function file_number()
*
* Get the number of files opened.
*
'query files'
line1=sublin(result,1)
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
function usage()
*
* Print usage information.
*
say '  Prepare plotting area for a multi-panel plot.'
say ''
say '  USAGE: subplot <ntot> <idx> [<ncol>] [-rowmajor 0|1] [-xy <xyratio>] [-tight 0|1] [-xtight 0|1] [-ytight 0|1]'
say '         [-xs <xspace>] [-ys <yspace>] [-xp <xpad>] [-yp <ypad>] [-x <pareawid>] [-y <pareahgt>] [-xappend 0|1] [-yappend 0|1]'
say '    <ntot>: total number of panels to be plotted. Do NOT have to be # of rows times # of columns; will be rounded up to that value.'
say '    <idx>: index of panel. In any column/row, panels with smaller <idx> MUST be plotted earlier.'
say '    <ncol>: number of columns. Default=2 (even if <ntot> = 1).'
say '    -rowmajor 1: plot 1st row first, then 2nd row, ...'
say '    <xyratio>: aspect ratio of plotting area. Default=1. An optimal value is calculated for map projections.'
say '    -tight 1: leave no spaces between panels.'
say '    -xtight 1: leave no horizontal spaces between panels.'
say '    -ytight 1: leave no vertical spaces between panels.'
say '    <xspace>: horizontal spacing in addition to default value.'
say '    <yspace>: vertical spacing in addition to default value.'
say '    <xpad>: horizontal padding in addition to default value.'
say '    <ypad>: vertical padding in addition to default value.'
say '    <pareawid>: arbitrary parea width.'
say '    <pareahgt>: arbitrary parea height. Ignored if <pareawid> is specified.'
say '    -xappend 1: attach a new page right of existing plots. This is NOT intended for simple multi-column plots.'
say '    -yappend 1: attach a new page below existing plots. This is NOT intended for simple multi-row plots.'
say ''
say '  NOTE:'
say '    1. Spacing refers to blank space between virtual pages; can be any value.'
say '    2. Padding refers to space between virtual page boundaries and plotting area; cannot be negative values.'
say '    3. For best result, set desired dimensions before (instead of after) running this script.'
say ''
say '  EXAMPLE 1: 2 rows by 2 columns.'
say '    set lon 120 300'
say '    set lat -25 25'
say '    set t 1'
say '    subplot 4 1'
say '    display sst'
say '    ...'
say '    set t 4'
say '    subplot 4 4'
say '    display sst'
say ''
say '  EXAMPLE 2: 3 rows by 1 column and no vertical spaces between panels.'
say '    set lon 120 300'
say '    set lat -25 25'
say '    set t 1'
say '    subplot 3 1 1 -ytight 1'
say '    display sst'
say '    ...'
say '    set t 3'
say '    subplot 3 3 1 -ytight 1'
say '    display sst'
say ''
say '  DEPENDENCIES: parsestr.gsf qdims.gsf'
say ''
say '  Copyright (c) 2006-2016, Bin Guan.'
return
