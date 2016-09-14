#!/usr/bin/env python
#
# utilities to work with bruker NMR/MRI systems
#
# (c)2016 Michael Tesch. tesch1@gmail.com
#
import logging
import math

class Spectrometer(object):
    ''' Spectrometer hardware and setup functionality '''
    log = logging.getLogger('Spectrometer')

    def __init__(self):
        self.tsversion = 2.1

    def SetCalibration(self, pw, dBW = None, refAtt = None, flip = 90):
        ''' set the calibration of the current coil, given :
        pulse width[us]
        pulse power[dBW], or refAtt[dB]
        flip[deg]
        '''
        if dBW == None and refAtt == None:
            raise ValueError('SetCalibration: must provide dBW or refAtt')
        if dBW != None and refAtt != None:
            raise ValueError('SetCalibration: only provide dBW or refAtt')
        if refAtt:
            # convert old-style dB values into new-style dBW values
            dBW = self.W2dBW(self.db2W(refAtt))
        Hz = (flip/360.0) / (pw * 1e-6)
        V = math.sqrt(self.dBW2W(dBW))
        self._cal_pw = pw
        self._cal_dBW = dBW
        self._cal_Hz_per_V = Hz / V
        self.log.info('calibration: {0} us, {1} dBW, {2} deg, := {3} Hz, {4} Hz/V'
                 .format(pw, dBW, flip, Hz, self._cal_Hz_per_V))

    def CalcPwFromFlip(self, flip, dBW):
        ''' calculate the pw for a desired flip angle[deg] at a give power[dBW] '''
        V = math.sqrt(self.dBW2W(dBW))
        Hz = self._cal_Hz_per_V * V
        return 1e6 * (flip / 360.0) / Hz

    def dBW2W(self, dBW):
        ''' convert -dBW to Watts -- used in topspin >= 3.0 '''
        return pow(10, -dBW/10.0)

    def W2dBW(self, W):
        ''' convert Watts to -dBW -- used in topspin >= 3.0 '''
        return -10.0 * math.log10(W)

    def dB2W(self, dB, MaxW = 70):
        ''' convert dB to Watts -- used in topspin < 3.0 '''
        # in old topspin, max amplifier Wattage was system-dependent, just default to 70W here
        return math.pow(MaxW * 10.0, -((dB + 6) / 10.0))

    def W2dB(self, W, MaxW = 70):
        ''' convert Watts to dB -- used in topspin < 3.0 '''
        # in old topspin, max amplifier Wattage was system-dependent, just default to 70W here
        return -(10.0 * math.log10(P / MaxW) + 6.0);


if __name__ == '__main__':
    logging.basicConfig(level=logging.INFO)
