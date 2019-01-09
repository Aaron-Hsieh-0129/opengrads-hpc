function test(args)

'reinit'
'load udxt shfilt.udxt'
'open ../../pytests/data/model'
'set lev 200'
'set gxout shaded'
'd sh_filt(ta,1,5)'
say 'Enter RETURN'
pull answer
'clear'
'p = sh_power(ta)'
'set x 1'
'd log(p)'
'draw title Power Spectra'

return
