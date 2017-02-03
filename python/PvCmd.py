#!/usr/bin/env python
'''
python wrapper to make using PV5 more tolerable

There are three classes here: PvCmd, PvScan and PvObj:

PvCmd is a wrapper for the pvcmd binary, which passes commands to/from the 
currently running ParaVision suite of programs.  It basically wraps the functionality
of the pvcmd binary.

PvScan is a wrapper for the PvSC ("Scan Control") application of ParaVision.
It has wrappers for managing the creation and execution of scans in the scan
list.

PvObj is a wrapper for a single dataset/experiment - it is created by providing 
a path to the dataset or by requesing a new dataset from PvCmd or clonding an 
existing PvObj.  A dataset can be not-yet-executed, meaning that the
experiment has been setup but not executed.  The object is like an entry in the
PvSC window, and can do basically the same things, like 'Go' 'Gsp' 'Gop' 'Clone'
etc.  Attributes to the object are the method's parameters.

(c) Copyright 2016, Michael Tesch, tesch1@gmail.com

'''

import os
import time
import logging
import subprocess

def is_exe(fpath):
    ''' helper function '''
    return os.path.isfile(fpath) and os.access(fpath, os.X_OK)

def is_int(s):
    try:
        int(s)
        return True
    except:
        return False


class PvObj(object):
    ''' class that wraps a single PV dataset & its recons
    
    Attributes are proxys for the method's variables, and can be set and read:

    print o.RG
    o.RG = 110

    Other things can be done with it too:
    o2 = o.Clone()
    o.Gsp()
    o.Gop()
    o.Go()

    '''

    def __init__(self, objpath, pvScan):
        # make sure it's a pv
        if not isinstance(pvScan, PvScan):
            raise ValueError("PvObj(): pvScan must be a PvScan object")
        # verify that the objpath is reasonable
        if not os.path.isdir(objpath):
            raise AttributeError("invalid object path: '%s'" % (objpath))
        #if not os.path.isfile(objpath + '/../../acqp'):
        #    print os.path.isdir(objpath),os.path.isfile(objpath + '/../../acqp')
        #    raise AttributeError("invalid object path no acqp: '%s'".format(objpath))
        self.__dict__['_objpath'] = objpath
        self.__dict__['_pvScan'] = pvScan
        self.__dict__['log'] = logging.getLogger('PvObj[%s]' % objpath)

    def __str__(self):
        return self._objpath

    def __repr__(self):
        return '{' + self._objpath + '}'

    def __setattr__(self, name, value):
        if name[0:2] == '__':
            raise NotImplementedError
        self.SetParam(name, value)

    def __getattr__(self, name):
        if name[0:2] == '__':
            raise NotImplementedError
        return self.GetParam(name)

    def __eq__(self, other):
        if isinstance(other, PvObj):
            return self._objpath == other._objpath
        return NotImplemented

    def __ne__(self, other):
        result = self.__eq__(other)
        if result is NotImplemented:
            return result
        return not result

    def GetParam(self, name):
        self._pvScan.SetObj(self)
        return self._pvScan.GetParam(name)

    def SetParam(self, name, value):
        self._pvScan.SetObj(self)
        self._pvScan.SetParam(name, value)

    def ProcPath(self):
        ''' the path to the object's reconstruction '''
        return self._objpath

    def StudyPath(self):
        ''' the path to the object's study '''
        return '/'.join(self._objpath.split('/')[0:-3])

    def ExpPath(self):
        ''' the path to the object's experiment '''
        return '/'.join(self._objpath.split('/')[0:-2])

    def Clone(self):
        ''' clone the object described by pvobj, return the new PvObj '''
        self.log.info('Clone')
        self._pvScan.SetObj(self)
        # maybe... ?   -procno <path> : prono path to clone. -- untested
        self._pvScan.Command('pvDsetClone', 'Scan', 'Current')
        return self._pvScan.GetObj()

    def CloneReco(self):
        ''' clone the object described by pvobj, return the new PvObj '''
        self.log.info('CloneReco')
        self._pvScan.SetObj(self)
        self._pvScan.Command('pvDsetClone', 'Reco', 'Current')
        #pvScan pvDsetCloneProcno -procno path 0
        return self._pvScan.GetObj()

    def RemoveFromList(self):
        self._pvScan.Command('pvDsetObjListRemove', self._objpath)
        # other options:
        # pvDsetObjListRemove All
        # pvDsetObjListRemove Study
        # pvDsetObjListRemove Subject
        # pvDsetObjListRemove OtherStudies
        # pvDsetObjListRemove OtherSubjects

    def ExportToTopspin(self):
        self.log.info('ExportToTopspin')
        self._pvScan.SetObj(self)
        self._pvScan.Command('pvDsetExport')

    def Delete(self):
        self.log.info('Delete')
        self._pvScan.SetObj(self)
        self._pvScan.Command('pvDsetDel', 'Scan', 'Current', '-Control', '-Alt')

    def DeleteReco(self):
        self.log.info('DeleteReco')
        self._pvScan.SetObj(self)
        self._pvScan.Command('pvDsetDel', 'Reco', 'Current', '-Control', '-Alt')

    # todo:
    # pvcmd -a pvScan pvStartGsp
    # pvcmd -a pvScan pvStartReco -Control -Alt
    # pvcmd -a pvScan CprWait xau CsiSplitMSlices
    # pvcmd -a pvScan pvStartGsauto -cmd Auto_Shim_XYZ
    # pvcmd -a pvScan pvStartGsauto -cmd Auto_SF
    # pvcmd -a pvScan pvStopPipe
    # pvcmd -a pvScan pvStopScan -quiet
    # pvcmd -a pvScan -r pvStartGopTst -dset $DATASET -goptstargs "-o $simulationDir
    # pvcmd -a pvScan -r pvStartGopTst -dset $DATASET

    # Open Geometry editor: pvcmd -a pvScan pvScanGed
    # pvcmd -a pvScan pvScanGedGeometry $DataSetProcno
    # pvcmd -a pvScan pvScanGedSet1x1
    # pvcmd -a pvScan pvScanGedReference $DataSetProcno
    # pvcmd -a pvScan pvScanGedAccept
    # pvcmd -a pvScan pvScanGedRefresh

    def Adjustment(self, adjustment, category='Standard'):
        '''
        run an adjustment (calibration), one of:
        'RCVR'   - receiver gain
        'FREQ'   - center frequency
        'SHIM'   - x,y,z shim
        'TRANSM' - reference transmitter gain
        from either category 'Standard' or 'Current'
        '''
        self.log.info('Adjustment(%s,%s)' % (adjustment, category))
        if adjustment not in ['RCVR', 'FREQ', 'SHIM', 'TRANSM']:
            raise ValueError('Adjustment must be "RCVR", "FREQ", or "SHIM"')
        if category not in ['Standard', 'Current']:
            raise ValueError('Adjustment Category must be "Standard" or "Current"')
        self._pvScan.SetObj(self)
        self._pvScan.Command('pvStartGsauto', '-cmd', adjustment+'_'+category)
        time.sleep(10)
        self._pvScan.Sync(self._objpath)

    def Start(self):
        '''
        like clicking the Traffic Light button in the Scan Control window (i think?)
        '''
        self.log.info('Start')
        self._pvScan.SetObj(self)
        self._pvScan.Command('pvStartScan', '-Control', '-Alt')
        self._pvScan.Sync(self._objpath)

    def Stop(self):
        '''
        stop pipeline and currently running scan - 

        might cause a popup dialog which then needs to be closed
        by the mouse click :{
        '''
        self.log.info('Stop')
        #self._pvScan.Command('pvStopScan','-quiet')
        self._pvScan.Command('pvStopMultiPipe')
        self._pvScan.Command('pvStopPipe', self._objpath)
        #self._pvScan.Command('pvStopScan')
        self._pvScan.Sync(self._objpath)

    def Gop(self):
        '''
        like clicking the GOP button in the Spectrometer Control Tool
        '''
        self._pvScan.Command('pvStartGop', self._objpath, '-Control', '-Alt')
        self._pvScan.Sync(self._objpath)

    def Gsp(self):
        '''
        like clicking the GSP button in the Spectrometer Control Tool
        '''
        self._pvScan.Command('pvStartGsp', self._objpath)
        self._pvScan.Sync(self._objpath)

    def Undo(self, what='Scan'):
        '''
        Undo a Scan or a Recon
        '''
        if what != 'Scan' and what != 'Reco':
            raise ValueError('Undo: what must be Scan or Reco')
        self._pvScan.SetObj(self)
        self._pvScan.Command('pvDsetUndo', what, 'Current', '-Control', '-Alt')

# side note:
'''
how to get a list of all commands supported by all apps:

for s in `pvcmd -l` ; do echo "-- $s --"; pvcmd -a $s -r CmdList ; echo '' ;  done

'''

def floatify(thing):
    ''' take a pvcmd string 'thing' and turn it into a representitive python object '''
    try:
        s = thing
        if int(float(s)) == float(s):
            return int(float(s))
        else:
            return float(s)
    except:
        return thing

def pythify(val):
    ''' take a pvcmd string 'val' and turn it into a representitive python object '''
    # if the variable is a list, convert the string into a python list
    if len(val) and val[0] == '{' and val[-1] == '}':
        val = val[1:-1]
        val = pythify(val.strip())
        val = eval('[' + str(val) + ']')
        # try to convert val to float(s)
        val = [floatify(x) for x in val]
    else:
        val = floatify(val)
    return val

class PvApp(object):
    '''
    wrapper around commands that need to have an app
    '''
    def __init__(self, appname, pv=None):
        if pv != None and not isinstance(pv, PvCmd):
            raise ValueError("PvApp(): pv must be a PvCmd object")
        if pv:
            self.pv = pv
        else:
            self.pv = PvCmd()
        self.app = appname
        self.log = logging.getLogger('[%s]' % appname)
        #self.commands = self.Command('CmdList').split(' ')

    def GetParam(self, param):
        ''' '''
        val = self.pv._run_pvcmd('-get', self.app, param)
        val = pythify(val)
        return val

    def SetParam(self, param, value):
        ''' '''
        self.pv._run_pvcmd('-set', self.app, param, str(value))
        #self.Sync()

    def ParamList(self):
        ''' get a list of available parameters (maybe move to PvObj?) '''
        pass

    def Sync(self, path=None):
        ''' '''
        self.log.info('Sync(%s)' % path)
        self.pv._run_pvcmd('-s', self.app)
        if path:
            self.pv._run_pvcmd('-s', path)

    def Command(self, *cmd):
        ''' send command to app, get results '''
        res = self.pv._run_pvcmd('-a', self.app, '-r', *cmd)
        self.log.info('Command %s->%s' % (str(cmd), res))
        self.Sync()
        return res

    def CommandQuiet(self, *cmd):
        ''' send command to app, dont get results '''
        self.log.info('CommandQuiet(%s)' % str(cmd))
        res = self.pv._run_pvcmd('-a', self.app, *cmd)
        self.Sync()
        return res

    def CommandList(self):
        return self.Command('CmdList')

class PvScan(PvApp):
    def __init__(self, pv):
        super (PvScan, self).__init__('pvScan', pv)

#    def ExpPath(self):
#        ''' get full current experiment path '''
#        return (self.GetParam('DU') + '/' +
#                self.GetParam('USER') + '/' +
#                self.GetParam('NAME') + '/' +
#                self.GetParam('EXPNO'))

#    def ProcPath(self):
#        return self.ExpPath() + '/' + self.GetParam('PROCNO')

    def StudyPath(self):
        return self.Command('pvDsetPath', '-path', 'STUDY')

    def ExpPath(self):
        return self.Command('pvDsetPath', '-path', 'EXPNO')

    def ProcPath(self):
        return self.Command('pvDsetPath', '-path', 'PROCNO')

    def Popup(self, message):
        return self.Command('pvErrorAlert', 'Python', message)
        #self.pv._run_pvcmd('-s', 'gui', app, message)

    def DisableSomeErrors(self):
        '''
        Man kann einige Fehlermeldungen mit
        pvcmd -a pvScan CprNoWait setdef ackn ok

        abschalten (d.h. die Fehler werden automatisch bestaetigt). Das funktioniert 
        aber nicht fuer alle Fehlermeldungen und ich weiss nicht, ob das hier eine 
        der Fehlermeldungen ist, die man automatisch bestaetigen kann.
        '''
        self.Command('CprNoWait','setdef','ackn','ok')

    def SetObj(self, pvobj):
        ''' set the currently selected object to pvobj '''
        self.log.debug('SetObj(%s)' % pvobj)
        if is_int(pvobj):
            # just change the EXPNO
            newdir = self.GetParam('DU') + '/data/' + \
                     self.GetParam('USER') + '/nmr/' + \
                     self.GetParam('NAME') + '/' + \
                     str(pvobj) + '/pdata/1'
            if os.path.isdir(newdir):
                pvobj = newdir
                # TODO: this doesn't work  like this - actually
                #       it goes to a 0-based index in the scan list(!)
                #
                #self.CommandQuiet('pvDsetSsel', str(pvobj))
            else:
                raise ValueError('PvCmd::SetObj: invalid (empty) expno:'+str(pvobj))
        #self.CommandQuiet('pvDsetSsel', str(pvobj))
        self.CommandQuiet('pvDsetObjSel', str(pvobj))

    # pvDsetSsel New
    # pvDsetSsel New PROTOCOL LOCATION
    # pvDsetSsel Ok
    # pvDsetSsel Cancel
    # pvDsetSsel default
    # pvDsetSsel <full path to processed data>
    # pvDsetSsel <0-based index into exp list in scan control window>
    # pvDsetSed -> open method editor
    # pvDsetPed -> open patient editor
    # pvDsetYed -> open study editor

    def GetObj(self, index=None, restore=True):
        ''' get the currently selected object, or the object at the numerical 'index' '''
        self.log.info('GetObj(%s,%s)' % (index, restore))
        if index:
            if restore:
                oldobj = self.ProcPath()
            self.SetObj(index)
        try:
            pvobj = PvObj(self.ProcPath(), self)
        except:
            self.log.warn('GetObj: unable to get obj at (%s)' % (self.ProcPath()))
            pvobj = None
        if index and restore:
            self.SetObj(oldobj)
        return pvobj

    def GetObjList(self):
        ''' return a list of all objecs in the object list '''
        ''' doens't really work '''
        self.log.info('GetObjList')
        pvobjlist = []
        index = 0
        seen = []
        selection = self.GetObj()
        for index in range(0,100):
            try:
                pvo = self.GetObj(index, restore=False)
                # GetObj() succeeded, add object
                if pvo in seen:
                    break
                #print 'GetObjList adding', pvo
                seen += [pvo]
                try:
                    method = pvo.Method
                except:
                    method = ''
                pvobjlist += [(index, pvo, method)]
            except Exception, ex:
                print 'GetObjList error', ex
                break
        self.SetObj(selection)
        return pvobjlist
    #...?
    # pvDsetListSubjects
    # pvDsetListStudies
    # pvDsetListScans [ByName ById ByPath]

    def CreateStudy(self, **kwargs):
        ''' create a new study, return path '''
        self.log.info('CreateStudy:' + str(kwargs))
        if not len(kwargs['subjectid']):
            raise ValueError('PvCmd::CreateStudy: invalid (empty) subjectid')
        # -studyname <name> -subjectid <id> -name <name> -subjectname <name> [ -birthdate YYYYMMDD ]
        # [ -type (animal | human | other) ] [ -gender (f | m | u) ]
        # [ -remarks <remarks> ] ] [ -studyloc <location> ]
        # [ -coil <coil> ] [ -entry (FeetFirst | HeadFirst) ]
        # [ -position (Supine | Prone | Left | Right) ] [ -weight <weight> ]
        # [ -referral <referral> ] [ -purpose <purpose> ]
        validkeys = ['studyname', 'subjectid', 'subjectname', 'birthdate', 'type', 'gender',
                     'remarks', 'name', 'studyloc', 'coil', 'entry', 'position',
                     'weight', 'referral', 'purpose']
        args = []
        for k, v in kwargs.items():
            if k not in validkeys:
                print 'CreateStudy: bad arg',k
                continue
            args += ['-'+k]
            args += [str(v)]
        #print 'CreateStudy',args
        path = self.Command('pvDsetCreateStudy', *args)
        return path

    #def DeleteStudy(self, studyid):
    #    self.pv.pvDatMan.Command('DMDelete', studyid)

    def NewScan(self, protocolLoc, protocolName):
        '''
        create a new scan, using current patient&study and the
        protocol specified by protocolLoc / protocolName
        pvDsetListLocations
        '''
        self.log.info('NewScan')
        self.Command('pvDsetSsel', 'New', protocolLoc, protocolName)
        return self.GetObj()

    # Reset commands
    # pvcmd -a pvScan pvResetInstrument
    # pvcmd -a pvScan pvResetParameterValues $pv::procnoDir

class PvCmd(object):
    '''
    class for interacting with the whole ParaVision suite via the "pvcmd" utility

    attributes are the names of the applications that "pvcmd" can talk to
    '''

    def __init__(self):
        self._pvcmd = ''
        self._pvapps = dict()
        self.verbose = False
        self.log = logging.getLogger('PvCmd')

        # find the pvcmd binary
        if 'XWINNMRHOME' in os.environ:
            self.XWINNMRHOME = os.environ['XWINNMRHOME']
        else:
            print "warning, XWINNMRHOME not set, defaulting to /opt/PV5.1/"
            self.XWINNMRHOME = "/opt/PV5.1"
        self.log.info('XWINNMRHOME:%s' % self.XWINNMRHOME)
        self._pvcmd = self.XWINNMRHOME + "/prog/bin/scripts/pvcmd"
        if not is_exe(self._pvcmd):
            self._pvcmd = "./pvcmd.tester"
            self.log.warning('PvCmd using test harness')
            #raise EnvironmentError("no pvcmd or XWINNMRHOME not set")
        self.runningApps()
        self.log.info('Apps:%s' % self._pvapps.keys())
        if 'pvScan' in self._pvapps.keys():
            self._pvapps.pop('pvScan', None)
            self._pvapps['pvScan'] = PvScan(self)
        else:
            print 'pvScan not in running apps:',self._pvapps.keys()
            self.log.error('pvScan not in running apps')
        #print "running apps:", self._pvapps.keys()

    def __setattr__(self, name, value):
        if (name not in ['_pvcmd', '_pvapps', 'XWINNMRHOME', 'verbose', 'log']
            and not hasattr(self, name)): # would this create a new attribute?
            raise AttributeError("Creating new attribute '%s' is not allowed!" % name)
        super (PvCmd, self).__setattr__(name, value)

    def __getattr__(self, name):
        return self._pvapps[name]

    def _run_pvcmd(self, *args):
        try:
            cmd = [self._pvcmd]
            cmd += args
            self.log.debug('#%s' % str(cmd))
            p = subprocess.Popen(cmd, shell=False,
                                 stdin=subprocess.PIPE,
                                 stdout=subprocess.PIPE,
                                 stderr=subprocess.PIPE)
            res, err = p.communicate(input='')
            if self.verbose:
                print 'pvcmd: ', str(cmd[1:]), '/', res, '/', err, '/', p.returncode
            self.log.debug(' =%s/%s/%s.' % (res, err, p.returncode))
            if p.returncode or len(err):
                #print cmd, p.returncode
                #print("OS error running pvcmd: %s".format(err))
                raise ValueError("Error from pvcmd: %s" % (err))
            #time.sleep(0.1)
        except Exception, ex:
            print 'exception:', ex
            print cmd
            raise ex
        except ValueError, ex:
            raise ex
        #print res
        return res.strip()

    def runningApps(self):
        appnames = self._run_pvcmd('-l').split()
        for name in appnames:
            self._pvapps[name] = PvApp(name, self)
        return self._pvapps.keys()

    def PVExit(self):
        ''' tell pv to exit '''
        self.pvCmd.Command('pvCmdExit')

