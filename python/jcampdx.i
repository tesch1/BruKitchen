/* File: jcampdx.i */
%module jcampdx

%include "std_set.i"
%include "std_vector.i"
%include "std_string.i"
%apply const std::string& {std::string* foo};

%{
#define SWIG_FILE_WITH_INIT
#include "jcampdx.hpp"
%}

%include "jcampdx.hpp"
