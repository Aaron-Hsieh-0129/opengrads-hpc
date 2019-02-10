#!/bin/bash
#
# Post install, creates symlinks in /opt/local/bin
#

binFiles="bufrscan geos grads grib2scan gribmap gribscan lats4d.sh ncep opengrads stnmap"

/bin/mkdir -p /opt/local/bin
cd /opt/local/bin

for file in $binFiles; do
    /bin/rm -rf /opt/local/bin/$file
    /bin/ln -s /Applications/OpenGrADS/$file .
done

exit 0



