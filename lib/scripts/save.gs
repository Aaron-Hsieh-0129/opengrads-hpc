***************************************************************************************
* $Id: save.gs,v 1.77 2017/07/11 22:46:51 bguan Exp $
*
* Copyright (c) 2012-2017, Bin Guan
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
function save(arg)
*
* Save single-precision data in GrADS (.dat and .ctl) or netCDF (.nc) format.
*
rc=gsfallow('on')

*
* Parse -v option (variables to be saved).
*
num_var=parseopt(arg,'-','v','var')
if(num_var=0)
  usage()
  return
endif

*
* Initialize other options.
*
cnt=1
while(cnt<=num_var)
  _.name.cnt=_.var.cnt
  _.desc.cnt=_.var.cnt
  cnt=cnt+1
endwhile
_.format.1='GrADS'
_.ctl.1=0
_.undef.1=-9.99e8
_.path.1='.'

*
* Parse -n option (name to be used in output .ctl).
*
rc=parseopt(arg,'-','n','name')

*
* Parse -d option (description to be used in output .ctl).
*
rc=parseopt(arg,'-','d','desc')

*
* Parse -f option (format of output).
*
rc=parseopt(arg,'-','f','format')

*
* Parse -c option (whether to output .ctl for .nc).
*
rc=parseopt(arg,'-','c','ctl')

*
* Parse -u option (undef value to be used in output).
*
rc=parseopt(arg,'-','u','undef')
if(!valnum(_.undef.1))
  say '[save ERROR] <undef> must be numeric.'
  return
endif

*
* Parse -o option (common name for output).
*
num_file=parseopt(arg,'-','o','file')
if(num_file=0)
  usage()
  return
endif

*
* Parse -p option (path to output).
*
rc=parseopt(arg,'-','p','path')

qdims(1,'mydim')

*
* Write .nc file.
*
if(_.format.1='netCDF')
  if(num_var>1)
    say '[save ERROR] Only one variable is allowed for netCDF format.'
    usage()
    return
  endif
  'set undef '_.undef.1
  'set sdfwrite -nc4 -chunk -zip -5d -rt -flt '_.path.1'/'_.file.1'.nc'
  'set sdfattr '_.var.1' String long_name '_.desc.1
  'sdfwrite '_.var.1
  if(_.ctl.1)
  writectl4nc(_.path.1'/'_.file.1'.ctl','^'_.file.1'.nc')
  endif
endif

*
* Write .dat and .ctl files.
*
if(_.format.1!='netCDF')
  'set gxout fwrite'
  'set fwrite '_.path.1'/'_.file.1'.dat'
  tcnt=_.mydim.ts
  while(tcnt<=_.mydim.te)
    'set t 'tcnt
    vcnt=1
    while(vcnt<=num_var)
      zcnt=_.mydim.zs
      while(zcnt<=_.mydim.ze)
        'set z 'zcnt
        'display const('_.var.vcnt','_.undef.1',-u)'
        zcnt=zcnt+1
      endwhile
      vcnt=vcnt+1
    endwhile
    say 'T='tcnt': written.'
    tcnt=tcnt+1
  endwhile
  'disable fwrite'
  'set gxout contour'
  writectl4dat(_.path.1'/'_.file.1'.ctl','^'_.file.1'.dat',num_var,name,desc)
endif

*
* Restore original dimension environment.
*
_.mydim.resetx
_.mydim.resety
_.mydim.resetz
_.mydim.resett

return
***************************************************************************************
function writectl4nc(ctlfile,ncfile)
*
* Write .ctl file.
*
lines=3
line.1='dset 'ncfile
if(_.mydim.cal='')
  line.2='*options'
else
  line.2='options '_.mydim.cal
endif
line.3='tdef time '_.mydim.nt' linear '_.mydim.tims' '_.mydim.dtim
cnt=1
while(cnt<=lines)
  status=write(ctlfile,line.cnt)
  cnt=cnt+1
endwhile
status=close(ctlfile)

return
***************************************************************************************
function writectl4dat(ctlfile,datfile,nv,var,desc)
*
* Write .ctl file.
*
lines=10
line.1='dset 'datfile
line.2='undef '_.undef.1
if(_.mydim.cal='')
  line.3='*options'
else
  line.3='options '_.mydim.cal
endif
line.4='title intentionally left blank.'
line.5=_.mydim.xdef
line.6=_.mydim.ydef
line.7=_.mydim.zdef
line.8=_.mydim.tdef
line.9='vars 'nv
line.10='endvars'
cnt=1
while(cnt<=lines-1)
  status=write(ctlfile,line.cnt)
  cnt=cnt+1
endwhile
cnt=1
while(cnt<=nv)
  varline=_.var.cnt' '_.mydim.nz0' 99 '_.desc.cnt
  status=write(ctlfile,varline)
  cnt=cnt+1
endwhile
status=write(ctlfile,line.lines)
status=close(ctlfile)

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
function usage()
*
* Print usage information.
*
say '  Save single-precision data in GrADS (.dat and .ctl) or netCDF (.nc) format.'
say ''
say '  USAGE 1: save -v <var1> [<var2>...] [-n <name1> [<name2>...]] [-d <description1> [<description2>]] [-u <undef>] -o <file> [-p <path>]'
say '  USAGE 2: save -v <var> -f netCDF [-c 0|1] [-d <description>] [-u <undef>] -o <file> [-p <path>]'
say '    <var>: variable to be saved. Can be any GrADS expression if [-f netCDF] is not in use; must be a defined variable otherwise.'
say '    -f netCDF: save in netCDF format. Only one DEFINED variable can be saved when this option is in use.'
say '    -c 1: output .ctl for .nc (so that .nc with 365-day calendar can be read by GrADS via xdfopen).'
say '    <name>: name for a variable in saved .ctl file. <var> is used if unset.'
say '    <description>: description (long name) for a variable. <var> is used if unset.'
say '    <undef>: undef value for .dat and .ctl. Default=-9.99e8.'
say '    <file>: output filename. Do NOT include filename extension.'
say '    <path>: path to output files. Do NOT include trailing "/". Current path is used if unset.'
say ''
say '  EXAMPLE 1: save "nino3" and "nino4" to "myfile.dat" and "myfile.ctl".'
say '    save -v nino3 nino4 -o myfile'
say ''
say '  EXAMPLE 2: save "sst" to "myfile.nc".'
say '    save -v sst -o myfile -f netCDF'
say ''
say '  DEPENDENCIES: parsestr.gsf qdims.gsf'
say ''
say '  Copyright (c) 2012-2017, Bin Guan.'
return
