// -*-  Mode: C++; c-basic-offset: 2 -*-
// 
// JCAMP-DX c++ class
//
// (c)2016 Michael Tesch, tesch1@gmail.com
//
// http://www.jcamp-dx.org/protocols.html
//  JCAMP-DX: A Standard Form for the Exchange of Infrared Spectra in Computer Readable Form
//  ROBERT S. McDONALD and PAUL A. WILKS, JR.
//  Appl. Spectrosc. 42(1), pp151-162, 1988
//

#include <iostream>
#include <sstream>
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

std::ostream &
operator <<(std::ostream & out, Record const & rr)
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
  case RECORD_GROUP:
    out << "(" << rr.group() << ")";
    break;
  }
  return out;
}

std::ostream &
operator <<(std::ostream & out, Ldr const & l)
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
    std::cerr << "error in Ldr '" << l._label << ex.what() << "'\n";
    throw;
  }
  return out;
}

std::ostream &
operator <<(std::ostream & out, Ldrset const & l)
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

Record::Record()
  : _type(RECORD_UNSET), _num(0), _ldr(NULL)
{
}

Record::Record(std::string str, bool quoted)
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

Record::Record(real_t val)
  : _type(RECORD_NUMERIC), _num(val), _ldr(NULL)
{
  std::stringstream str;
  str << val;
  _str = str.str();
}

Record::Record(Ldr * ldr)
  : _type(RECORD_GROUP), _ldr(ldr)
{
}

const real_t &
Record::num() const
{
  return _num;
}

const std::string &
Record::str() const
{
  return _str;
}

record_type
Record::type() const
{
  return _type;
}

void
Record::setNum(real_t val)
{
  _num = val;
  _type = RECORD_NUMERIC;
}

void
Record::setStr(std::string str, bool quoted)
{
  _str = str;
  _type = quoted ? RECORD_QSTRING : RECORD_STRING;
}

const Ldr &
Record::group() const
{
  if (!_ldr) {
    std::cerr << "group() fail" << _type << " " << _str << "\n";
    throw std::out_of_range("NULL ldr group");
  }
  return *_ldr;
}

void
Record::setType(record_type type)
{
  _type = type;
}


// //////////////////////////////////////////////////////////
// Label
static void
removeCharsFromString(std::string & str, const char * charsToRemove)
{
  for (unsigned int ii = 0; ii < strlen(charsToRemove); ++ii) {
    str.erase(std::remove(str.begin(), str.end(), charsToRemove[ii]), str.end());
  }
}

Label::Label(std::string name)
{
  for (auto & c: name) c = toupper(c);
  removeCharsFromString(name, " -/_$");
  std::string::assign(name);
}

// //////////////////////////////////////////////////////////
// Ldr

Ldr::Ldr()
  : _shape_type(SHAPE_1D)
{
}

Ldr::Ldr(std::string label)
  : _label(label), _shape_type(SHAPE_1D)
{
}

Ldr::Ldr(record_type type, std::string str, std::string label)
  : _label(label), _shape_type(SHAPE_1D)
{
  appendStr(str);
  _data.back().setType(type);
}

Ldr::Ldr(record_type type, real_t val, std::string label)
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

std::string
Ldr::label() const
{
  return _label;
}

const std::string &
Ldr::str(size_t idx) const
{
  if (idx >= _data.size()) {
    std::stringstream str;
    str << "Ldr::str " << _label << " index error: '" << idx << "/" << _data.size() << "'";
    throw std::out_of_range(str.str());
  }
  return _data.at(idx).str();
}

const real_t &
Ldr::num(size_t idx) const
{
  if (idx >= _data.size()) {
    std::stringstream str;
    str << "Ldr::num " << _label << " index error: '" << idx << "/" << _data.size() << "'";
    throw std::out_of_range(str.str());
  }
  return _data.at(idx).num();
}

record_type
Ldr::type(size_t idx) const
{
  if (idx >= _data.size()) {
    std::stringstream str;
    str << "Ldr::type " << _label << " index error: '" << idx << "/" << _data.size() << "'";
    throw std::out_of_range(str.str());
  }
  return _data.at(idx).type();
}

void
Ldr::setStr(std::string str, size_t idx)
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
Ldr::setLabel(std::string label)
{
  _label = label;
}

void
Ldr::setShape(std::string shape)
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
    std::cerr << "unknown shape type: " << shape << "\n";
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
Ldr::appendStr(std::string str, bool quoted)
{
  _data.emplace_back(Record(str, quoted));
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

Ldrset::Ldrset(const std::string filename)
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
Ldrset::loadFile(const std::string filename)
{
#if !defined(__EMSCRIPTEN__) && 0
  extern int jcamp_yydebug;
#endif

  FILE *fp = fopen(filename.c_str(), "r");

  if (!fp) {
    std::stringstream str;
    str << "invalid input file: " << filename << ": " << strerror(errno) << std::endl;
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
      std::cerr << "parse of '" << filename << "' failed\n";
  }
  catch (const std::exception & ex) {
    jcamp_yylex_destroy(scanner);
    throw;
  }

  if (DEBUG) std::cerr << "parse done " << jcamp_topnode << "\n";

  fclose(fp);
  _ldrs = jcamp_topnode->_ldrs;
  _blocks = jcamp_topnode->_blocks;
  delete jcamp_topnode;
  validate();
}

void
Ldrset::loadString(const std::string jdxstring, std::string nametag)
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
      std::cerr << "parse of string failed\n";
  }
  catch (const std::exception & ex) {
    jcamp_yylex_destroy(scanner);
    throw;
  }

  jcamp_yylex_destroy(scanner);

  if (DEBUG) std::cerr << "parse done " << jcamp_topnode << "\n";

  _ldrs = jcamp_topnode->_ldrs;
  _blocks = jcamp_topnode->_blocks;
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
Ldrset::addLdr(const std::string label, const Ldr & ldr)
{
  if (DEBUG) std::cerr << "addingLdr '" << label << "' " << ldr << "\n";
  _ldrs.emplace(label, ldr).first->second.setLabel(label);
}

void
Ldrset::addLdr(const Ldr & ldr)
{
  if (DEBUG) std::cerr << "addingLdr '" << ldr.label() << "' " << ldr << "\n";
  _ldrs.emplace(ldr.label(), ldr);
}

void
Ldrset::addBlock(Ldrset * ldrset)
{
  if (DEBUG) std::cerr << "addingBlock" << "" << "\n";
  ldrset->validate();
  _blocks.push_back(std::shared_ptr<Ldrset>(ldrset));
}

void
Ldrset::deleteLdr(const std::string label)
{
  _ldrs.erase(label);
}

Ldr &
Ldrset::getLdr(const std::string label)
{
  if (!_ldrs.count(label))
    throw std::out_of_range("getLdr no such label:" + label);
  return _ldrs.at(label);
}

void
Ldrset::validate() const
{
  if (DEBUG) std::cerr << "validate" << "" << "\n";
}

// data retreival functions
bool
Ldrset::labelExists(const std::string label) const
{
  return _ldrs.count(label) > 0;
}

std::string
Ldrset::getString(const std::string label, size_t idx) const
{
  if (!_ldrs.count(label)) {
    std::stringstream str;
    str << "Ldrset::getString no such label: '" << label << "'";
    throw std::out_of_range(str.str());
  }
  return _ldrs.at(label).str(idx);
}

real_t
Ldrset::getDouble(const std::string label, size_t idx) const
{
  if (!_ldrs.count(label)) {
    std::stringstream str;
    str << "Ldrset::getDouble no such label: '" << label << "'";
    throw std::out_of_range(str.str());
  }
  return _ldrs.at(label).num(idx);
}

void
Ldrset::newEmpty(const std::string label)
{
  if (_ldrs.count(label)) {
    return;
    std::stringstream str;
    str << "Ldrset::newEmpty label exitst: '" << label << "'";
    throw std::out_of_range(str.str());
  }
  addLdr(Ldr(label));
}

void
Ldrset::setString(const std::string label, std::string str, size_t idx, bool create)
{
  if (_ldrs.count(label))
    _ldrs.at(label).setStr(str, idx);
  else if (create)
    addLdr(Ldr(RECORD_STRING, str, label));
  else {
    std::stringstream str;
    str << "Ldrset::setString no such label: '" << label << "'";
    throw std::out_of_range(str.str());
  }
}

void
Ldrset::setDouble(const std::string label, real_t val, size_t idx, bool create)
{
  //std::cerr << "setDouble(" << label << ", " << val << ")\n";
  if (_ldrs.count(label))
    _ldrs.at(label).setNum(val, idx);
  else if (create)
    addLdr(Ldr(RECORD_NUMERIC, val, label));
  else {
    std::stringstream str;
    str << "Ldrset::setDouble no such label: '" << label << "'";
    throw std::out_of_range(str.str());
  }
}

// retrieve a sub-set of LDRs in the particular block
Ldrset &
Ldrset::getBlock(int blocknum)
{
  return *_blocks[blocknum];
}

//
std::set<std::string>
Ldrset::getLabels()
{
  std::set<std::string> labels;
  for (auto it : _ldrs)
    labels.insert(it.second.label());
  return labels;
}
 
#ifdef MAIN
#include "cxxopts.hpp"
int main(int argc, char *argv[])
{
  cxxopts::Options options(argv[0], "jcampdx data file utility");
  options.add_options()
    ("f,file",          "jcamp file to read",
     cxxopts::value<std::vector<std::string> >()->default_value({"/dev/stdin"}))
    ("s,se",            "something else",
     cxxopts::value<std::vector<std::string> >()->default_value({"/dev/stdin"}))
    ("h,help",          "print help message")
    ("v,verbose",       "be verbose")
    ;
  options.parse(argc, argv);
  options.parse_positional("file");

  if (options.count("help")) {
    std::cout << options.help();
    exit(0);
  }

  for (auto & filename : options["file"].as<std::vector<std::string> >() ) {
    try {
      Ldrset jc(filename);
      if (DEBUG) {
        std::cout << "done:\n";
        std::cout << jc;
      }
      return 0;
    }
    catch (const std::exception & ex) {
      std::cerr << "failed: " << ex.what() << "\n";
      return -1;
    }
  }
}
#endif
