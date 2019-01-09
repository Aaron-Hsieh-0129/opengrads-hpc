function main(args)

rc=gsfallow(on)

dolin=1
dogaus=0

'load udxt libmf.udxt'
'load udxt ../re/re.udxt'

'open ../../pytests/data/model'

'set lat 0 40'
'set lon 120 180'
'grhist ps'
return

'mfhilo ps cl'
return

'set lev 850'
'd ta'
'd smth2d(ta,3)'

'q pos'
'c'
'set lat 0'
'd ta'
'd linreg(ta)'
print result

'q pos'
'reset'
'set lev 850'
'd ta'
'd esmrf(ta)'

return




'set lev 500'

#
# test default behaviour for single param, use re standard of setting the regrid method
# based on the size of the output grid box v input grid box,  
# if output > input - box average, if output < input use bilinear
#

'r1d=re(ta,2)'
'r2d=re2(ta,2)'

'set gxout stat'
'd r1d-r2d'
print result

'set gxout contour'

'd ta'

if(dolin)
'r1=re(ta,1,1,bs)'
print result
'r2=re2(ta,1,1,bs)'
print result
endif
'c'
'd re(ta,1,1,bs)-re2(ta,1,1,bs)'
'q pos'
'c'
'd re(ta,1,1,ba)-re2(ta,1,1,ba)'
'q pos'
if(dogaus)
'r1g=re(ta,360,linear,0,1.0,182,gaus,1,182,bs)'
print 'r1111111111111111111 '
print result
'r2g=re2(ta,360,182,gg_ba)'
#'r2a=re2(ta,2024,712,gg_ba)'
#'r1a=re(ta,720,linear,0,0.5,314,gaus,1,314,bl)'
print 'r222222222222222222 '
endif
'c'
'set lon 330'
'set lat -90 -85'
'set lat 85 90'

'd ta'
'q pos'
'd r1'
'q pos'
'd r2'
'q pos'

'c'
'set lat 0'
'set lon -10 10'
'set lon 350 370'

'd ta'

'q pos'
'd r1'
'q pos'
'd r2'
'q pos'
return

'd r1-r2'
'q pos'
'c'



return
