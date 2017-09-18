// -*-  Mode: C++; c-basic-offset: 2 -*-
// 
// JCAMP-DX c++ class and json conversion
//
// (c)2016 Michael Tesch, tesch1@gmail.com
//
// http://www.jcamp-dx.org/protocols.html
//  JCAMP-DX: A Standard Form for the Exchange of Infrared Spectra in Computer Readable Form
//  ROBERT S. McDONALD and PAUL A. WILKS, JR.
//  Appl. Spectrosc. 42(1), pp151-162, 1988
//

#include <limits>
#include <algorithm>

#include "jcampdx.hpp"
#include "jcamp_scan.hpp"
#include "jcamp_parse.hpp"

#ifndef NDEBUG
#define DEBUG (getenv("DEBUG") ? 2 : 0)
#else
#define DEBUG 0
#endif

#ifndef jcamp_yyparse
int jcamp_yyparse (Ldrset & jdx, yyscan_t scanner);
#endif

#ifdef _WIN32
#include <malloc.h>

char *
strndup(const char *s, size_t n)
{
  char *result;
  size_t len = strlen(s);

  if (n < len)
    len = n;

  result = (char *)malloc(len + 1);
  if (!result)
    return 0;

  result[len] = '\0';
  return (char *)memcpy(result, s, len);
}
#endif // _WIN32

ostream & operator <<(ostream & out, Ldr::Record const & rr)
{
  switch (rr._type) {
  case RECORD_TEXT:
  case RECORD_STRING:
    out << rr._str;
    break;
  case RECORD_QSTRING:
    out << "<" << rr._str << ">";
    break;
  case RECORD_NUMERIC:
    out << rr._num;
    break;
  case RECORD_UNSET:
    out << "?";
    break;
  case RECORD_GROUP:
    out << "(" << rr.group() << ")";
    break;
  }
  return out;
}

ostream & operator <<(ostream & out, Ldr const & l)
{
  try {
    if (l._label.size())
      out << "##" << l._label << "=";
    if (l._shape_type != Ldr::SHAPE_1D)
      out << "(" << l._data.size() << ")\n";
    for (auto it = l._data.begin(); it != l._data.end(); it++)
      out << *it << " ";
  }
  catch (const std::exception & ex) {
    ERROR("error in Ldr '" << l._label << ex.what() << "'\n");
    throw;
  }
  return out;
}

ostream & operator <<(ostream & out, Ldrset const & l)
{
  for (auto li = l._ldrs.begin(); li != l._ldrs.end(); li++)
    if (li->first == "TITLE")
      out << li->second << "\n";
  for (auto li = l._ldrs.begin(); li != l._ldrs.end(); li++)
    if (li->first[0] != '$' && li->first != "TITLE")
      out << li->second << "\n";
  for (auto li = l._ldrs.begin(); li != l._ldrs.end(); li++)
    if (li->first[0] == '$')
      out << li->second << "\n";
  for (auto & li : l._blocks)
    out << *li;
  out << "##END=\n";
  return out;
}

// //////////////////////////////////////////////////////////
// Record
Ldr::Record::Record()
  : _type(RECORD_UNSET), _num(0), _ldr(NULL)
{
}

Ldr::Record::Record(const string & str, bool quoted)
  : _type(quoted ? RECORD_QSTRING : RECORD_STRING), _str(str), _num(0), _ldr(NULL)
{
  try {
    _num = std::stod(str);
  }
  catch (...) {
#ifdef __EMSCRIPTEN__
    _num = 0;
#else
    _num = std::numeric_limits<real_t>::signaling_NaN();
#endif
  }
}

Ldr::Record::Record(real_t val)
  : _type(RECORD_NUMERIC), _num(val), _ldr(NULL)
{
  _str = std::to_string(val);
}

Ldr::Record::Record(Ldr * ldr)
  : _type(RECORD_GROUP), _ldr(ldr)
{
}

const real_t &
Ldr::Record::num() const
{
  return _num;
}

const string &
Ldr::Record::str() const
{
  return _str;
}

record_type
Ldr::Record::type() const
{
  return _type;
}

void
Ldr::Record::setNum(real_t val)
{
  _num = val;
  _type = RECORD_NUMERIC;
}

void
Ldr::Record::setStr(const string & str, bool quoted)
{
  _str = str;
  _type = quoted ? RECORD_QSTRING : RECORD_STRING;
}

const Ldr &
Ldr::Record::group() const
{
  if (!_ldr) {
    ERROR("group() fail" << _type << " " << _str << "\n");
    throw std::out_of_range("NULL ldr group");
  }
  return *_ldr;
}

void
Ldr::Record::setType(record_type type)
{
  _type = type;
}


// //////////////////////////////////////////////////////////
// Label
static void
removeCharsFromString(string & str, const char * charsToRemove)
{
  for (unsigned int ii = 0; ii < strlen(charsToRemove); ++ii) {
    str.erase(std::remove(str.begin(), str.end(), charsToRemove[ii]), str.end());
  }
}

Label::Label(string name)
{
  for (auto & c: name) c = toupper(c);
  removeCharsFromString(name, " -/_$");
  string::assign(name);
}

// //////////////////////////////////////////////////////////
// Ldr

Ldr::Ldr()
  : _shape_type(SHAPE_1D)
{
}

Ldr::Ldr(const string & label)
  : _label(label), _shape_type(SHAPE_1D)
{
}

Ldr::Ldr(record_type type, const string & str, const string & label)
  : _label(label), _shape_type(SHAPE_1D)
{
  appendStr(str);
  _data.back().setType(type);
}

Ldr::Ldr(record_type type, real_t val, const string & label)
  : _label(label), _shape_type(SHAPE_1D)
{
  appendNum(val);
  _data.back().setType(type);
}

size_t
Ldr::size() const
{
  return _data.size();
}

std::vector<int>
Ldr::shape() const
{
  std::vector<int> d = { (int)_data.size() };
  return d;
}

string
Ldr::label() const
{
  return _label;
}

const string &
Ldr::str(size_t idx) const
{
  if (idx >= _data.size()) {
    stringstream str;
    str << "Ldr::str " << _label << " index error: '" << idx << "/" << _data.size() << "'";
    throw std::out_of_range(str.str());
  }
  return _data.at(idx).str();
}

const real_t &
Ldr::num(size_t idx) const
{
  if (idx >= _data.size()) {
    stringstream str;
    str << "Ldr::num " << _label << " index error: '" << idx << "/" << _data.size() << "'";
    throw std::out_of_range(str.str());
  }
  return _data.at(idx).num();
}

record_type
Ldr::type(size_t idx) const
{
  if (idx >= _data.size()) {
    stringstream str;
    str << "Ldr::type " << _label << " index error: '" << idx << "/" << _data.size() << "'";
    throw std::out_of_range(str.str());
  }
  return _data.at(idx).type();
}

void
Ldr::setStr(const string & str, size_t idx)
{
  if (idx >= _data.size())
    _data.resize(idx+1);
  _data[idx].setStr(str);
}

void
Ldr::setNum(real_t val, size_t idx)
{
  if (idx >= _data.size())
    _data.resize(idx+1);
  _data[idx].setNum(val);
}

void
Ldr::setLabel(const string & label)
{
  _label = label;
}

void
Ldr::setShape(const string & shape)
{
  int a,b;
  if (shape == "(X++(Y..Y))") {
    _shape_type = SHAPE_XYY;
  }
  else if (shape == "(XY..XY)") {
    _shape_type = SHAPE_XYXY;
  }
  else if (2 == sscanf(shape.c_str(), "(%d..%d)", &a, &b)) {
    _shape.clear();
    _shape.push_back(b-a+1);
    _shape_type = SHAPE_XYY;
  }
  else {
    ERROR("unknown shape type: " << shape << "\n");
  }
}

void
Ldr::setShape(const Ldr & ldr)
{
  for (size_t ii = 0; ii < ldr.size(); ii++)
    _shape.push_back((int)ldr.num());
  _shape_type = SHAPE_XYXY;
}

void
Ldr::appendStr(const string & str, bool quoted)
{
  _data.emplace_back(str, quoted);
}

void
Ldr::appendNum(real_t val)
{
  _data.emplace_back(val);
}

void
Ldr::appendGroup(Ldr * group)
{
  _data.emplace_back(group);
}

// //////////////////////////////////////////////////////////
// Ldrset

Ldrset::Ldrset()
{
}

Ldrset::Ldrset(const string & filename)
{
  loadFile(filename);
}

Ldrset::~Ldrset()
{
}

size_t
Ldrset::size() const
{
  return _ldrs.size();
}

void
Ldrset::loadFile(const string & filename)
{
#if !defined(__EMSCRIPTEN__) && 0
  extern int jcamp_yydebug;
#endif

  FILE *fp = fopen(filename.c_str(), "r");

  if (!fp) {
    stringstream str;
    str << "invalid input file: " << filename << ": " << strerror(errno) << "\n";
    throw std::invalid_argument(str.str());
  }

  // set lex to read from it instead of defaulting to STDIN:
  _curfilename = filename;
  jcamp_topnode = NULL;

  yyscan_t scanner;
  jcamp_yylex_init(&scanner);
#if !defined(__EMSCRIPTEN__) && 0
  jcamp_yydebug = DEBUG;
#endif
  if (DEBUG)
    jcamp_yyset_debug(2, scanner);
  jcamp_yyset_in(fp, scanner);
  //jcamp_yyset_lineno(0, scanner);
  //jcamp_yyset_column(0, scanner);
  try {
    if (jcamp_yyparse(*this, scanner))
      ERROR("parse of '" << filename << "' failed\n");
  }
  catch (const std::exception & ex) {
    jcamp_yylex_destroy(scanner);
    throw;
  }

  if (DEBUG) INFO("parse done " << jcamp_topnode << "\n");

  fclose(fp);

  // newly loaded ldrs override existing ldrs
  jcamp_topnode->_ldrs.insert(_ldrs.begin(), _ldrs.end());
  std::swap(jcamp_topnode->_ldrs, _ldrs);
  // these are just pointers, so nothing will be overridden, just combined. maybe should
  // someday fix (todo) so that blocks with same TITLE get combined.
  _blocks.insert(_blocks.end(), jcamp_topnode->_blocks.begin(), jcamp_topnode->_blocks.end());
  delete jcamp_topnode;
  jcamp_topnode = NULL;
  validate();
}

void
Ldrset::loadString(const string & jdxstring, const string & nametag)
{
#if !defined(__EMSCRIPTEN__) && 0
  extern int jcamp_yydebug;
#endif

  // set lex to read from it instead of defaulting to STDIN:
  _curfilename = nametag;
  jcamp_topnode = NULL;

  yyscan_t scanner;
  jcamp_yylex_init(&scanner);
#if !defined(__EMSCRIPTEN__) && 0
  jcamp_yydebug = DEBUG;
#endif
  if (DEBUG)
    jcamp_yyset_debug(2, scanner);
  //jcamp_yyset_in(fp, scanner);
  jcamp_yy_scan_string(jdxstring.c_str(), scanner);
  jcamp_yyset_lineno(1, scanner);
  jcamp_yyset_column(0, scanner);

  try {
    if (jcamp_yyparse(*this, scanner))
      ERROR("parse of string failed\n");
  }
  catch (const std::exception & ex) {
    jcamp_yylex_destroy(scanner);
    throw;
  }

  jcamp_yylex_destroy(scanner);

  if (DEBUG)
    INFO("parse done " << jcamp_topnode << "\n");

  // newly loaded ldrs override existing ldrs
  jcamp_topnode->_ldrs.insert(_ldrs.begin(), _ldrs.end());
  std::swap(jcamp_topnode->_ldrs, _ldrs);
  // these are just pointers, so nothing will be overridden, just combined. maybe should
  // someday fix (todo) so that blocks with same TITLE get combined.
  _blocks.insert(_blocks.end(), jcamp_topnode->_blocks.begin(), jcamp_topnode->_blocks.end());
  delete jcamp_topnode;
  jcamp_topnode = NULL;
  validate();
}

void
Ldrset::clear()
{
  _ldrs.clear();
  _blocks.clear();
}

void
Ldrset::addLdr(const string & label, const Ldr & ldr)
{
  if (DEBUG) INFO("addingLdr '" << label << "' " << ldr << "\n");
  _ldrs.emplace(label, ldr).first->second.setLabel(label);
}

void
Ldrset::addLdr(const Ldr & ldr)
{
  if (DEBUG) INFO("addingLdr '" << ldr.label() << "' " << ldr << "\n");
  _ldrs.emplace(ldr.label(), ldr);
}

void
Ldrset::addBlock(Ldrset * ldrset)
{
  if (DEBUG) INFO("addingBlock\n");
  ldrset->validate();
  _blocks.push_back(std::shared_ptr<Ldrset>(ldrset));
}

void
Ldrset::deleteLdr(const string & label)
{
  _ldrs.erase(label);
}

Ldr &
Ldrset::getLdr(const string & label)
{
  if (!_ldrs.count(label))
    throw std::out_of_range("getLdr no such label:" + label);
  return _ldrs.at(label);
}

const Ldr &
Ldrset::getLdr(const string & label) const
{
  if (!_ldrs.count(label))
    throw std::out_of_range("getLdr no such label:" + label);
  return _ldrs.at(label);
}

void
Ldrset::validate() const
{
  if (DEBUG) INFO("validate" << "" << "\n");
}

// data retreival functions
bool
Ldrset::labelExists(const string & label) const
{
  return _ldrs.count(label) > 0;
}

string
Ldrset::getString(const string & label, size_t idx) const
{
  if (!_ldrs.count(label)) {
    throw std::out_of_range("Ldrset::getString no such label: '" + label + "'");
  }
  return _ldrs.at(label).str(idx);
}

real_t
Ldrset::getDouble(const string & label, size_t idx) const
{
  if (!_ldrs.count(label)) {
    throw std::out_of_range("Ldrset::getDouble no such label: '" + label + "'");
  }
  return _ldrs.at(label).num(idx);
}

void
Ldrset::newEmpty(const string & label)
{
  if (_ldrs.count(label)) {
    return; //! \todo comment this out -- be less tolerant
    throw std::out_of_range("Ldrset::newEmpty label exitst: '" + label + "'");
  }
  addLdr(Ldr(label));
}

void
Ldrset::setString(const string & label, const string & str, size_t idx, bool create)
{
  if (!_ldrs.count(label)) {
    if (create)
      newEmpty(label);
    else {
      throw std::out_of_range("Ldrset::setString no such label: '" + label + "'");
    }
  }
  _ldrs.at(label).setStr(str, idx);
}

void
Ldrset::setDouble(const string & label, real_t val, size_t idx, bool create)
{
  //INFO("setDouble(" << label << "[" << idx << "] = " << val << ")\n");
  if (!_ldrs.count(label)) {
    if (create)
      newEmpty(label);
    else {
      throw std::out_of_range("Ldrset::setDouble no such label: '" + label + "'");
    }
  }
  _ldrs.at(label).setNum(val, idx);
}

// retrieve a sub-set of LDRs in the particular block
std::shared_ptr<Ldrset>
Ldrset::getBlock(int blocknum)
{
  return _blocks.at(blocknum);
}

size_t Ldrset::getBlockCount() const
{
  return _blocks.size();
}

//
std::set<string>
Ldrset::getLabels() const
{
  std::set<string> labels;
  for (auto & it : _ldrs)
    labels.insert(it.second.label());
  return labels;
}

#ifdef JCAMP_TO_JSON
//
// Ldr::Record
//
json Ldr::Record::to_json() const
{
  json jj;
  switch (_type) {
  case RECORD_TEXT:
  case RECORD_STRING:
  case RECORD_QSTRING:
    jj = _str;
    break;
  case RECORD_NUMERIC:
    jj = _num;
    break;
  case RECORD_GROUP:
    jj = _ldr->to_json();
    break;
  case RECORD_UNSET:
    jj = {};
  }
  return jj;
}

void from_json(const json & jj, Ldr::Record & record)
{
  if (jj.is_string())
    record = Ldr::Record(jj.get<string>(), true);
  else if (jj.is_number())
    record = Ldr::Record(jj.get<real_t>());
  else if (jj.is_array()) {
    Ldr * ldr = new Ldr();
    from_json(jj, *ldr);
    record = Ldr::Record(ldr);
  }
  else
    ERROR("Unable to convert JSON to Record: " << jj.dump() << "\n");
}

//
// Ldr - convert an Ldr/Ldrset to json
//
json Ldr::to_json() const
{
  json jj = json::object();
  jj["label"] = label();
  json data;
  for (auto & record : _data) {
    json rec = record.to_json();
    data.push_back(rec);
  }
  jj["data"] = data;
  if (_shape.size()) {
    jj["shape"] = _shape;
    jj["shape_type"] = _shape_type;
  }
  return jj;
}

void from_json(const json & jj, Ldr & ldr)
{
  ldr._data.clear();
  for (auto & elem : jj["data"]) {
    Ldr::Record r = elem;
    ldr._data.push_back(r);
  }
  ldr._label = jj["label"];
  if (jj.count("shape"))
    ldr._shape = jj["shape"].get<std::vector<int> >();
  if (jj.count("shape_type"))
    ldr._shape_type = (Ldr::shape_type)(int)jj["shape_type"];
}

//
// Ldrset - convert an Ldr/Ldrset to json
//
json Ldrset::to_json() const
{
  json jj = json::array();
  for (auto & ldr : _ldrs) {
    json jldr = ldr.second.to_json();
    jj.push_back(jldr);
  }
  if (getBlockCount()) {
    // add a level of json array
    jj = {jj};
    // add the blocks as additional arrays
    for (auto & blockp : _blocks) {
      json jn = blockp->to_json();
      jj.push_back(jn);
    }
  }
  return jj;
}

void from_json(const json & jj, Ldrset & ldrset)
{
  ldrset.clear();
  // it's an array of arrays, each sub-arrays is an Ldrset,
  // the first block goes into ldrset, the rest added using addBlock
  if (jj.is_array()) {
    if (jj.size() && jj[0].is_array()) {
      // first one is top-level
      from_json(jj[0], ldrset);
      for (size_t ii = 1; ii < jj.size(); ii++) {
        // the rest are blocks
        Ldrset * block = new Ldrset();
        from_json(jj[ii], *block);
        ldrset.addBlock(block);
      }
    }
    else {
      // it's a single array, each element should be an Ldr in json form
      for (auto & el : jj) {
        Ldr ldr;
        from_json(el, ldr);
        ldrset.addLdr(ldr);
      }
    }
  }
  else {
    // something's wrong with the format of jj
    ERROR("Unable to convert JSON to Ldrset (JCAMP-DX)\n");
  }
}

#endif // JCAMP_TO_JSON

#ifdef JCAMPDX_MAIN
#define CXXOPTS_NO_RTTI
#include "cxxopts.hpp"
#include "cio.hpp"
#ifdef CIO
using cio::cout;
#else
using std::cout;
#endif

int main(int argc, char *argv[])
{
  cxxopts::Options options(argv[0], "jcampdx data file utility");
  options.add_options()
    ("f,file",          "jcamp file to read",
     cxxopts::value<std::vector<string> >()->default_value({"/dev/stdin"}))
    ("J,jfile",         "JSON file to read",
     cxxopts::value<std::vector<string> >()->default_value({"/dev/stdin"}))
    ("s,se",            "something else",
     cxxopts::value<std::vector<string> >()->default_value({"/dev/stdin"}))
    ("h,help",          "print help message")
    ("j,json",          "print as JSON")
    ("v,verbose",       "be verbose")
    ;
  options.parse(argc, argv);
  options.parse_positional("file");

  if (options.count("help")) {
    cout << options.help();
    exit(0);
  }

  for (auto & filename : options["file"].as<std::vector<string> >() ) {
    try {
      Ldrset jc(filename);
      if (DEBUG) {
        cout << "done:\n";
        cout << jc;
      }
      if (options.count("json")) {
        json jj = jc.to_json();
        cout << jj.dump(2) << "\n";
      }
      return 0;
    }
    catch (const std::exception & ex) {
      cout << "failed: " << ex.what() << "\n";
      return -1;
    }
  }
}
#endif
