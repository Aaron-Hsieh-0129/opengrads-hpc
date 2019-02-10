#!/bin/bash
#
# Post install, creates symlinks in /usr/local/bin
#

binFiles="bufrscan geos grads grib2scan gribmap gribscan lats4d.sh ncep opengrads stnmap"

/bin/mkdir -p /usr/local/bin
cd /usr/local/bin

for file in $binFiles; do
    /bin/rm -rf /usr/local/bin/$file
    /bin/ln -s /Applications/OpenGrADS/$file .
done

exit 0



