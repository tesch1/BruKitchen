# BruKitchen
utilities for dealing with bruker nmr/mri systems and data

## Matlab
To use these utilities, run the script brukitchen_setup.m to compile the
mex programs from within Matlab, then add this directory to your path:

```
>> brukitchen_setup
>> addpath([getenv('HOME') '/src/BruKitchen/matlab']);
% then read an experiment directory:
>> ex = read_bru_experiment('/opt/data/.../myexperiment')
ex = 
  struct with fields:

      acqu: [1×1 struct]
     acqus: [1×1 struct]
      acqp: [1×1 struct]
    method: [1×1 struct]
     pdata: [1×1 containers.Map]
       fid: [2048×1 double]
>> ex.acqus.SW_h
ans =
        5000
```

|Files                 |                                              |
|----------------------|----------------------------------------------|
|read_bru_experiment.m |Read all the files and data from an experiment|
|mexldr.cpp            |Read a single jcamp-dx parameter file         |

## Python
|File                  |                                                  |
|----------------------|--------------------------------------------------|
|BruKitchen.py         |utility functions for controlling the spectrometer|
|PvCmd.py              |classes for wrapping control of ParaVision        |
|pvshell.py            |a command-line shell for controlling ParaVision   |
|procall.py            |example of how to process a series of PV experiments|
