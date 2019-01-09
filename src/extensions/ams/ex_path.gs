function main()

*  Enable library functions
*  ------------------------
   rc = gsfallow('on')

*  Check if a file
*  ---------------
   if ( isfile('/usr/bin') )
         say '/usr/bin is a file'
   else
         say '/usr/bin is NOT a file'
   endif 

*  Check if a directory
*  --------------------
   if ( isdir('/usr/bin') )
         say '/usr/bin is a directory'
   else
         say '/usr/bin a NOT file'
   endif 

*  Get list of header files
*  ------------------------
   headers = wordexp('/opt/X11/include/png*.h')
   say 'PNG header files are: ' headers

*  Search and replace
*  ------------------
   new = sed('s/grads/GrADS/ig', 'Opengrads provides GRADS extensions')
   say new

