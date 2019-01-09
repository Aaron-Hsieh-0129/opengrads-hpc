*
* Simple interactuve test of shapefile functions
*
function main(args)
'open ../../pytests/data/model'
'set lon -180 180'
'draw map'
'clear'
*
* shp_lines
*
'shp_lines  gshhs_land'
'draw title shp_lines gshhs_land'
cr()
*
* shp_polyf
*
'shp_polyf  gshhs_land'
'draw title shp_polyf gshhs_land'
cr()
*
* shp_print
*
'set gxout shaded'
'set parea 0 11 0 8.5'
'set mpdraw off'
'set frame off'
'set xlab off'
'set ylab off'
'set mproj scaled'
'display ps'
'shp_print ps.shp'

return

*.....................................

function cr()
   say 'Hit RETURN to continue'
   pull ans	
   'clear'
return
