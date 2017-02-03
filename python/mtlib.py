#
# (c)Copyright 2016, Michael Tesch, tesch1@gmail.com
# All rights reserved.
#

# some useful commands for dealing with TopSpin

import sys
import os
import copy
import shutil
import glob
import datetime

from TopCmds import *

# bruker-provided stuff
import de.bruker.nmr as nmr
import de.bruker.nmr.mfw.base as mfw
import de.bruker.nmr.mfw.root as root
import de.bruker.nmr.mfw.dialogs as dialogs
import de.bruker.nmr.prsc.cpr as cpr
import de.bruker.nmr.prsc.toplib as top
import de.bruker.data.Constants as dataconst
import de.bruker.nmr.prsc.util as putil
import de.bruker.nmr.pr.browse as browse
import de.bruker.nmr.pr as pr
import de.bruker.nmr.prsc as prsc
import de.bruker.nmr.sc as sc

def DATAPATH(dataset=None):
    if not dataset:
        dataset = CURDATA()
    return os.path.join(dataset[3], dataset[0], dataset[1])

def PROCPATH(dataset=None):
    if not dataset:
        dataset = CURDATA()
    return os.path.join(dataset[3], dataset[0], dataset[1], 'pdata', dataset[2])

def xPROCPATH(dataset=None):
    if not dataset:
        dataset = CURDATA()
    return os.path.join(dataset[3], dataset[0], dataset[1], 'pdata', dataset[2])

def QCMD(cmd):
    return XCMD('qu ' + cmd)

def CLONEDATASET(newexpname, newexpnum, olddataset = None, replace = False, loadIt = True):
    ''' clone an existing dataset into a new one, delete processed data '''
    if not olddataset:
        olddataset = CURDATA()
    newdataset = olddataset[:]
    newdataset[0] = newexpname
    newdataset[1] = str(newexpnum)
    oldpath = DATAPATH(olddataset)
    newpath = DATAPATH(newdataset)
    if os.path.isdir(newpath):
        if replace:
            shutil.rmtree(newpath)
        else:
            print 'abort CLONEDATASET(); experiment already exists: ' + newpath
            if loadIt:
                RE(newdataset)
            return newdataset
    shutil.copytree(oldpath, newpath)
    # remove processed data files
    for datafile in glob.glob(newpath + '/pdata/1/[1-9]*'):
        os.remove(datafile)
    # load the new dataset into the active "thread"
    if loadIt:
        RE(newdataset)
    return newdataset

def getpar(varname, arrayidx = None, axis = 0):
    ''' better version of GETPAR '''
    if arrayidx != None:
        arrayidxS = ' ' + str(arrayidx)
    else:
        arrayidxS = ''
    value = GETPAR(varname + arrayidxS, axis)
    print 'getpar', varname + arrayidxS, value
    try:
        value = float(value)
    except:
        pass
    return value

def setpar(varname, arrayidx = None, value = '', axis = None):
    ''' better version of PUTPAR '''
    if axis == None:
        axisS = ''
    else:
        axisS = str(axis) + ' '
    if arrayidx != None:
        arrayidxS = ' ' + str(arrayidx)
    else:
        arrayidxS = ''
    print 'setpar', axisS + varname + arrayidxS, value
    PUTPAR(axisS + varname + arrayidxS, str(value))
