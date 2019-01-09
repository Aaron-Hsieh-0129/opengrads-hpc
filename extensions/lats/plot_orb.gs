*
* Plot portrait page with 8 time steps
*
function main()
   birds = 'aqua terra aura cloudsat calipso'
   terra = 'modis_terra misr mopitt aster'
   aqua = 'modis_aqua airs amsu_a hsb amsr_e'
   aura = 'hirdls omi tes'
   eos = 'tskin ' birds ' ' terra ' ' aqua ' ' aura

   i = 1
   sat = subwrd(eos,i)
   while ( sat != '' )
     plot8(sat)
     i = i + 1
     sat = subwrd(eos,i)
   endwhile


return



function plot8(sat)

  'reinit'
  'sdfopen output/'sat'.nc4'
  'set gxout grfill'
  'set grads off'

  i = 1
  while ( i<9 )
     'subplot 4 2 ' i
     'set t ' i
     'q time'
      time = subwrd(result,3)
      time = substr(time,1,2)
      'd tskin'
      'draw title ' sat ' ' time 'Z'
      i = i + 1
   endwhile
   'gxyat output/' sat '.png'

