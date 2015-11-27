#!/bin/bash -x
#./do_update.sh /jffs2 /jffs2/fallback /jffs2/update
REBOOT="yes"
DESTDIR="$1"
JFFS2TAR="jffs2.tar.gz"
FALLBACKDIR="$2"
UPDATEDIR="$3"

echo Update dir is "$UPDATEDIR"

mkdir -p "$FALLBACKDIR"
rm -r "$FALLBACKDIR"/*


UPDFILES=`tar -tf "$UPDATEDIR"/$JFFS2TAR`

for file in ${UPDFILES[@]}
do
    echo copy "$file"
    if [ -e $DESTDIR/"$file" ] ; then
	if [ -d $DESTDIR/"$file" ] ; then
	    mkdir -p $FALLBACKDIR/"$file"
	else 
	    cp $DESTDIR/$file $FALLBACKDIR/$file
	fi
    fi
done

# other stuff on $name

mkdir -p "$DESTDIR"
tar xzvf  "$UPDATEDIR"/"$JFFS2TAR" -C "$DESTDIR"

echo test test 1

if [ $REBOOT == "yes" ] ; then
    exit 126
else
    exit 0
fi
