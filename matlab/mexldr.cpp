// -*-  Mode: C++; c-basic-offset: 2 -*-
// 
// JCAMP-DX c++ mex wrapper
// 
// Macos: mex mexldr.cpp jcampdx.cpp jcamp_parse.cpp jcamp_scan.cpp FileLoc.cpp
// Linux: mex mexldr.cpp jcampdx.cpp jcamp_parse.cpp jcamp_scan.cpp FileLoc.cpp CXXFLAGS="-std=c++11 -fPIC"
//
// (c)2016 Michael Tesch, tesch1@gmail.com
//
//#include "premexh.h"
#include "mex.h"
#include "jcampdx.hpp"
#include "debug.hpp"

DebugLevel g_debug_level = LEVEL_INFO2;
void DebugFunc(DebugLevel level, string str, string location)
{
  mexErrMsgTxt((location + ":" + str).c_str());
}

/* paramstruct = mexLoadLDRS(filename) */
void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
  char ldrfile[512];
  const char * errmsg = "usage: paramstruct = mexldr('../path/to/ldrfile')\n";
  Ldrset ldrset;
  int count;
  int ii;
  const char ** keys;
  mwSize dims[2] = {1, 1};

  if (nrhs != 1 || nlhs != 1 || !mxIsChar(prhs[0])) {
    mexWarnMsgTxt(" bad params");
    mexErrMsgTxt(errmsg);
  }

  if (mxGetString(prhs[0], ldrfile, sizeof(ldrfile))) {
    mexWarnMsgTxt(" can't get param file name from argument");
    mexErrMsgTxt(errmsg);
  }

  /* read the procpar */
  try {
    ldrset.loadFile(ldrfile);
  }
  catch (std::exception & exc) {
    mexWarnMsgIdAndTxt("mexldr:loadFile", "%s / %s", ldrfile, exc.what());
  }
  count = ldrset.size();

  /* gather param names into key array */
  keys = (const char **)malloc(sizeof(char *) * count);
  if (!keys)
    mexErrMsgTxt("malloc");

  auto labels = ldrset.getLabels();
  auto label = labels.begin();
  for (ii = 0; ii < count; ii++, label++) {
    // nasty...
    keys[ii] = label->c_str();
    if (keys[ii][0] == '$')
      keys[ii]++;
  }

  /* Construct outdata */
  plhs[0] = mxCreateStructArray(2, dims, count, keys);

  /* set struct values */
  for (ii = 0; ii < count; ii++) {
    mxArray *field_value = NULL; // compiler warning
    int j, fieldcount;

    Ldr & ldr = ldrset.getLdr(keys[ii]);
    std::vector<int> shape = ldr.shape();
    fieldcount = dims[0] = shape[0];
    if (shape.size() > 1) {
      dims[1] = shape[1];
      fieldcount *= shape[1];
    }

    switch (ldr.type()) {
    case RECORD_TEXT:
    case RECORD_STRING:
    case RECORD_QSTRING:
      field_value = mxCreateCellMatrix(fieldcount, 1);
      for (j = 0; j < fieldcount; j++) {
        mxSetCell(field_value, j, mxCreateString(ldr.str(j).c_str()));
      }
      break;
    case RECORD_NUMERIC:
      field_value = mxCreateNumericArray(2, dims, mxDOUBLE_CLASS, mxREAL);
      for (j = 0; j < fieldcount; j++) {
        mxGetPr(field_value)[j] = ldr.num(j);
      }
      break;
    case RECORD_GROUP:
      field_value = mxCreateCellMatrix(0, 1);
      //field_value = mxCreateStructArray(2, dims, count, keys);
      break;
    default:
      //mexErrMsgTxt("mexLoadLDRS internal error");
      
      mexWarnMsgIdAndTxt("mexldr:unkn",
                         "mexLoadLDRS internal error, unhandled ldr type %d from '%s'",
                         ldr.type(), keys[ii]);
    }
    mxSetFieldByNumber(plhs[0], 0, ii, field_value);
  }
  free(keys);
}
