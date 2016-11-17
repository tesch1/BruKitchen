#!/usr/bin/env python
'''
command line interface for paravision, or actually the PvCmd and PvObj
classes.

(c) Copyright 2016, Michael Tesch, tesch1@gmail.com

'''

import cmd
import sys
import traceback
import logging

from PvCmd import PvCmd,PvObj
from BruKitchen import Spectrometer

class pvshell(cmd.Cmd):
    geompars = ['PVM_VoxArrSize', 'PVM_VoxArrPosition', 'PVM_VoxExcOrder', 'PVM_VoxArrCSDisplacement']
    """ Simple ParaVision command line interface."""
    def __init__(self):
        cmd.Cmd.__init__(self)
        self.pv = PvCmd()
        self.do_system('')
        self.pv.verbose = False
        self.update_prompt()
        self.geom = {}

    def postcmd(self, stop, line):
        self.update_prompt()
        return stop

    def update_prompt(self):
        self.prompt = str(self.pv.pvScan.GetObj()) + ' > '

    def do_system(self, line):
        ''' print info about the current system '''
        print "Institution:   ", self.pv.pvScan.GetParam('ACQ_institution')
        print "System:        ", self.pv.pvScan.GetParam('ACQ_station')
        print "PV version:    ", self.pv.pvScan.GetParam('ACQ_sw_version')
        print "Status:        ", self.pv.pvScan.GetParam('ACQ_status')
        print "Config Status: ", self.pv.pvScan.GetParam('CONFIG_status_string')
        print "Shim Status:   ", self.pv.pvScan.GetParam('CONFIG_shim_status')
        print "Instrument:    ", self.pv.pvScan.GetParam('CONFIG_instrument_type')
        print "Max gradient:  ", self.pv.pvScan.GetParam('PVM_GradCalConst'), "Hz/mm"

    def do_ls(self, line):
        ''' list available scans ? or something '''
        objs = self.pv.pvScan.GetObjList()
        for iobj in objs:
            print " ".join(str(x) for x in iobj)

    def do_man(self, line):
        ''' get info about available commands '''
        if not len(line):
            # list all commands
            print self.pv.pvScan.Command('CmdList')
        else:
            pass

    def do_info(self, line):
        ''' print some info about the current scan '''
        obj = self.pv.pvScan.GetObj()
        print "Scan Method:   ", obj.Method
        print "Scan Name:     ", obj.ACQ_scan_name
        print "Scan Completed:", obj.ACQ_completed
        print "Scan Duration: ", obj.PVM_ScanTimeStr
        print "Reco Image:    ", obj.RECO_image_type
        print "BF1:           ", obj.BF1
        print "RG:            ", obj.RG
        refAtt = obj.PVM_RefAttCh1
        sp = Spectrometer()
        sp.SetCalibration(1000, refAtt)
        print "RefAtt         ", sp._cal_dBW, ', Hz/V=',sp._cal_Hz_per_V

    def do_pwd(self, line):
        ''' print path of current Scan/Reco '''
        print self.pv.pvScan.ProcPath()

    def do_verbose(self, line):
        ''' print path of current Scan/Reco '''
        self.pv.verbose = not self.pv.verbose
        print 'verbose=',self.pv.verbose

    def do_rm(self, line):
        ''' remove a Scan/Reco '''
        obj = self.pv.pvScan.GetObj()
        print 'deleting ', obj
        obj.Delete()
    
    def do_study(self, line):
        ''' print current or create a new study

        field: subjectid, studyname, studyloc, name, weight=10.0
        '''
        if len(line):
            self.pv.pvScan.CreateStudy(*line.split(' '))
        else:
            print self.pv.pvScan.StudyPath()
    
    def do_clone(self, line):
        ''' clone the current object/scan '''
        self.pv.pvScan.GetObj().Clone()
    
    def do_export(self, line):
        ''' export the current object/scan to Topspin '''
        self.pv.pvScan.GetObj().ExportToTopspin()
    
    def do_sel(self, line):
        ''' change currently selected object/scan '''
        self.pv.pvScan.SetObj(line)
    
    def do_cd(self, line):
        ''' change currently selected object/scan '''
        try:
            num = int(line)
            # this is short hand for changing to a different experiment in the current study
            self.pv.pvScan.SetObj(num)
        except Exception as x:
            pass

    def do_p(self, line):
        ''' print out the value of a parameter in the current obj/scan '''
        try:
            print line, '=', self.pv.pvScan.GetObj().__getattr__(line)
        except ValueError as ex:
            print "'", line, "' not set."

    def do_set(self, line):
        ''' set the value of a parameter in the current obj/scan '''
        try:
            lines = line.split(" ")
            self.pv.pvScan.GetObj().__setattr__(lines[0], " ".join(lines[1:]))
        except ValueError as ex:
            print "'", line, "' not set:", ex

    def do_start(self, line):
        ''' traffic light (?) '''
        self.pv.pvScan.GetObj().Start()

    def do_stop(self, line):
        ''' traffic light stop (?) '''
        self.pv.pvScan.GetObj().Stop()

    def do_undo(self, line):
        ''' undo a "Scan" or a "Reco" '''
        if len(line):
            self.pv.pvScan.GetObj().Undo(line)
        else:
            self.pv.pvScan.GetObj().Undo()

    def do_gop(self, line):
        ''' GOP '''
        self.pv.pvScan.GetObj().Gop()

    def do_gsp(self, line):
        ''' GSP '''
        self.pv.pvScan.GetObj().Gsp()

    def do_getgeom(self, line):
        ''' get the current geometry, store in geom clipboard '''
        obj = self.pv.pvScan.GetObj()
        for pname in self.geompars:
            self.geom[pname] = obj.GetParam(pname)
            print pname,'=',self.geom[pname]

    def do_setgeom(self, line):
        ''' set the geometry of the current scan to that stored in the geom clipboard '''
        if not self.geom:
            print 'must run "getgeom" before "setgeom"'
            return
        obj = self.pv.pvScan.GetObj()
        for pname in self.geom.keys():
            print 'setting',pname,'=',self.geom[pname]
            obj.SetParam(pname, self.geom[pname])
    
    def do_EOF(self, line):
        return True

    def do_quit(self, arg):
        ''' quit '''
        sys.exit(1)

    def do_kill(self, arg):
        ''' quit ParaVision '''
        self.pv.PVExit()

if __name__ == '__main__':
    # setup logging to console and file
    logging.basicConfig(filename='pvshell.log', level=logging.DEBUG)
    ch = logging.StreamHandler()
    ch.setLevel(logging.WARN)
    logging.getLogger('').addHandler(ch)
    done = False
    pvs = pvshell()
    while not done:
        try:
            pvs.cmdloop()
        except (SystemExit, KeyboardInterrupt):
            print
            done = True
        except:
            print traceback.print_exc()
