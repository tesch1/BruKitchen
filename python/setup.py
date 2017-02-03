#!/usr/bin/env python

"""
setup.py file for SWIG example
"""

from distutils.core import setup, Extension


jcampdx_module = Extension('_jcampdx',
                           sources=['jcampdx.i',
                                    '../matlab/jcampdx.cpp',
                                    '../matlab/FileLoc.cpp',
                                    '../matlab/jcamp_scan.cpp',
                                    '../matlab/jcamp_parse.cpp'],
                           extra_compile_args=['-std=c++11'],
                           swig_opts=['-modern', '-I../matlab', '-c++'],
                           include_dirs=['../matlab/'])

setup (name = 'jcmapdx',
       version = '0.1',
       author      = "Michael Tesch",
       author_email='tesch1@gmail.com',
       url         ='https://www.github.com/tesch1/BruKitchen/',
       description = """Module to read JCAMP-DX parameter files""",
       ext_modules = [jcampdx_module],
       py_modules  = ["jcampdx"],
       )
