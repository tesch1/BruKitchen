#
# calculate eddy current correction terms for microimaging
#
# (c)Copyright 2016, Michael Tesch, tesch1@gmail.com
# All rights reserved.
#
import sys
import os
import time
import copy
import shutil
import glob
import datetime

mypath='/home/guest/tesch/BruKitchen/python'
if mypath not in sys.path:
    sys.path.append(mypath)
from mtlib import *

print 'GETCWD=',os.getcwd()
print 'PATH=',sys.path
print 'CURDATA=',CURDATA()
print 'PROCPATH=',PROCPATH()

if not CURDATA():
    EXIT()

#
# create a new experiment
#

pp_name = 'mt_ecc.ppg'
pp_text = '''
;mt_ecc
;avance-version (06/11/09)
;1D sequence
;
;$CLASS=HighRes
;$DIM=1D
;$TYPE=
;$SUBTYPE=
;$COMMENT=
;$OWNER=nmrsu
#include <Avance.incl>
#include <Grad.incl>

;$EXTERN

"acqt0=-p1*2/3.1416"
"p17=80u"

  10m UNBLKGRAMP
1 ze
2 30m
  10m
  d1
  ;p17:gp1      ; ramp up
  ;p12:gp2      ; gradient on
  ;p17:gp3      ; ramp down
  p17 grad{step(cnst1,10)|step(cnst2,10)|step(cnst3,10)}
  p12 grad{(cnst1)|(cnst2)|(cnst3)}
  p17 grad{(cnst1)-step(cnst1,10)|(cnst2)-step(cnst2,10)|(cnst3)-step(cnst3,10)}
  vd            ; gradient off  allow eddy currents to decay
  p1
  go=2
  d11 wr #0 if #0 ivd
  lo to 2 times td1
  10m
  200u BLKGRAMP
  10m
exit

ph1=0 2 2 0 1 3 3 1
ph31=0 2 2 0 1 3 3 1

;pl1 : f1 channel - power level for pulse (default)
;p1 : f1 channel -  high power pulse
;p17 : ramp up/down time
;p12 : gradient on time
;d1 : relaxation delay; 1-5 * T1
;NS: 1 * n, total number of scans: NS * TD0
;cnst:x grad val[%]
;cnst2:y grad val[%]
;cnst3:z grad val[%]
'''

def write_vdlist(vdname, delays):
    fd = open('/opt/topspin3.2/exp/stan/nmr/lists/vd/'+vdname, 'w')
    for d in delays:
        fd.write('%6.6f\n' % d)
    fd.close()

print 'c',CURDATA()
print 'PROCPATH=',xPROCPATH()
pp = DEF_PULSPROG(pp_text)
write_vdlist('mt_ecc', [0.0001, 0.0003, 0.001, 0.003, 
                        0.020, 0.100, 0.400, 1.000])
pp.SAVE_AS(pp_name)
print 'd',CURDATA()
print 'PROCPATH=',xPROCPATH()

# where to put our new experiments
todaytag = 'amt_' + datetime.datetime.now().strftime("%Y%m%d") + "_ecc"
ex1 = [todaytag, '1', '1', None]
ex1 = CURDATA()
ex1[0] = todaytag
print ex1

NEWDATASET(ex1)
RE(ex1)
PUTPAR("PULPROG", pp_name)
PUTPAR("VDLIST", 'mt_ecc')
ZG()
