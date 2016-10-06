#!/bin/bash

errors=0
good=0

export XWINNMRHOME=/opt/topspin
export XWINNMRHOME=${HOME}/src/pv51

jdxs="$XWINNMRHOME/exp/stan/nmr/lists/wave/* $XWINNMRHOME/exp/stan/nmr/lists/gp/* $XWINNMRHOME/exp/stan/nmr/parx/template/general/S019/*/*"
#jdxs=$XWINNMRHOME/exp/stan/nmr/parx/template/general/S019/*/*
#jdxs=$XWINNMRHOME/exp/stan/nmr/par/*/*
#jdxs=$(find $HOME/src/od1n-code/trunk/odin -name *.smp)

export DEBUG=1
unset DEBUG

if [ ! -x jcampdx ]; then
    g++ -std=c++11 jcampdx.cpp jcamp_parse.cpp jcamp_scan.cpp FileLoc.cpp -DMAIN -o jcampdx
fi

for jdx in $jdxs ; do
    #echo test $jdx
    if ! ( grep -qs JCAMP-DX $jdx || grep -qs JCAMPDX $jdx ); then
        # || grep -qs MRI.include $jdx
        # ignore ppgs that arent base-level pulse jdxuences
        #echo ignore $jdx
        continue
    fi
    echo $jdx
    #continue
    if ! ./jcampdx $jdx ; then
        echo $jdx bad
        #cpp -w $INCLUDES $jdx > fail.out
        DEBUG=1 ./jcampdx $jdx
        errors=$(( $errors + 1 ));
        #break
    else
        good=$(( $good + 1 ));
    fi
done

echo 'errors: ' $errors
echo 'good: ' $good
