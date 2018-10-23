isOctave = exist('OCTAVE_VERSION', 'builtin') ~= 0;

if isOctave
    oldf = mkoctfile('-p', 'CXXFLAGS');
    setenv('CXXFLAGS', [' -std=c++11 ' strtrim(oldf) ]);
    mex -v mexldr.cpp jcampdx.cpp jcamp_parse.cpp jcamp_scan.cpp FileLoc.cpp
else
    mex -v mexldr.cpp jcampdx.cpp jcamp_parse.cpp jcamp_scan.cpp FileLoc.cpp CXXFLAGS="-std=c++11 -fPIC"

end