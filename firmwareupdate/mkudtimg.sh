#!/bin/bash -x

echo "Make firmware file"

THISPWD=`/bin/pwd`
INPUTDIR="$1"

if (git describe --tags 2> /dev/null) ; then 
	VERSION=`git describe --tags`
elif (git log -1) ; then
	VERSION=`git log -1 --pretty=format:'%h' --abbrev-commit`
else
    VERSION=`svnversion -n`;
fi 



OUTPUTNAME="allprogs"

if [ $2 ] ; then 
    OUTPUTNAME=$2
fi

TMPDIR="/tmp/.fupdate"
JFFS2TAR="jffs2.tar.gz"

echo this dir is $THISPWD

mkdir -p $TMPDIR/

#copy do_update script
cp do_update.sh $TMPDIR

#copy jffs2 files 
cd $INPUTDIR
tar  czvf $TMPDIR/$JFFS2TAR *
cd -

#pack image 
cd $TMPDIR
tar czvf "$THISPWD"/$OUTPUTNAME"$VERSION".tar.gz *
cd -

rm -r $TMPDIR

        
