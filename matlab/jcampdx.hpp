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

//
// some vocabulary from jcampdx paper:
// files are sets of LABELED-DATA-RECORDS
//
//  AFFN   - ascii free form numeric
//  ASDF   - ascii squeezed difference form
//  BLOCK  - set of LABEL:DATA pairs, unique LABELs in the set
//  LABEL  - starts with ##, ends with =, ie ##SOMELABEL=
//  DATA   - the data associated with a LABEL
//  tabular data - starts on line following LABEL
//
// some labels are reserved
// labels which are used-defined start with '$'
//
// just a little commentary: this format sucks.
//
#ifndef JCAMPDX_HPP
#define JCAMPDX_HPP

#ifdef _WIN32
char * strndup(const char *s, size_t n);
#endif

#if defined(CIO)
#include "cio.hpp"
#else
#include <iostream>
#include <sstream>
using std::ostream;
using std::stringstream;
#endif

#if defined(MATLAB_MEX_FILE)
#if defined(ERROR) || defined(INFO)
#error "header issues"
#endif
#define ERROR(putput) do { std::cerr << putput << "\n"; } while (0)
#define INFO(putput) do { std::cout << putput << "\n"; } while (0)
#else
#include "debug.hpp"
#endif

#include <string>
using std::string;
#include <map>
#include <set>
#include <vector>
#include <memory>
#ifndef real_t
#ifdef REAL_FLOAT
typedef float real_t;
#else
typedef double real_t;
#endif
#endif

#ifdef JCAMP_TO_JSON
#include "json/src/json.hpp"
using json = nlohmann::json;
#endif

enum record_type {
  RECORD_TEXT = 1,
  RECORD_STRING,
  RECORD_QSTRING,
  RECORD_NUMERIC,
  RECORD_GROUP,
  RECORD_UNSET,
};

class Ldr;

//! string to label
struct Label : public string {
  Label(string name);
};

class Ldr {
public:
  Ldr();
  Ldr(const string & label);
  Ldr(record_type type, const string & str, const string & label = "");
  Ldr(record_type type, real_t val, const string & label = "");

  size_t size() const;
  std::vector<int> shape() const;
  string label() const;

  // get values
  const string & str(size_t idx = 0) const;
  const real_t & num(size_t idx = 0) const;
  record_type type(size_t idx = 0) const;
  void setStr(const string & str, size_t idx = 0);
  void setNum(real_t val, size_t idx = 0);

  void setLabel(const string & label);
  void setShape(const string & variable_list);
  void setShape(const Ldr & shape);

  void appendStr(const string & str, bool quoted = false);
  void appendNum(real_t val);
  void appendGroup(Ldr * group);

private:
  class Record {
  public:
    Record();
    Record(const string & str, bool quoted = false);
    Record(real_t val);
    Record(Ldr * ldr);

    const real_t & num() const;
    const string & str() const;
    record_type type() const;
    void setNum(real_t val);
    void setStr(const string & str, bool quoted = false);
    const Ldr & group() const;
    void setType(record_type type);

    friend ostream & operator <<(ostream & out, Ldr::Record const & l);
    friend ostream & operator <<(ostream & out, Ldr const & l);
    friend Ldr;
#ifdef JCAMP_TO_JSON
    json to_json() const;
#endif
  private:
    record_type _type;
    string _str;
    real_t _num;
    Ldr * _ldr;
  };

public:
  friend Record;
  friend ostream & operator <<(ostream & out, Ldr const & l);
  friend ostream & operator <<(ostream & out, Ldr::Record const & l);
#ifdef JCAMP_TO_JSON
  json to_json() const;
  friend void from_json(const json & jj, Ldr::Record & ldr);
  friend void from_json(const json & jj, Ldr & ldr);
#endif

private:
  std::vector<Record> _data;
  //std::vector<real_t> _num; // special case -- optimize table data .. someday
  string _label;
  std::vector<int> _shape;
  enum shape_type { SHAPE_1D, SHAPE_2D, SHAPE_XYY, SHAPE_XYXY } _shape_type;
};

//
class Ldrset
{
public:

  Ldrset();
  Ldrset(const string & filename);
  ~Ldrset();

  void loadFile(const string & filename);
  void loadString(const string & jdxstring, const string & nametag="string");
  void clear();
  size_t size() const;

  //
  void addLdr(const Ldr & ldr);
  void addLdr(const string & label, const Ldr & ldr);
  void addBlock(Ldrset * ldrset);
  void deleteLdr(const string & label);
  Ldr & getLdr(const string & label);
  const Ldr & getLdr(const string & label) const;

  // data retreival
  bool labelExists(const string & label) const;
  string getString(const string & label, size_t idx = 0) const;
  real_t getDouble(const string & label, size_t idx = 0) const;

  void newEmpty(const string & label);
  void setString(const string & label, const string & str, size_t idx = 0, bool create = false);
  void setDouble(const string & label, real_t val, size_t idx = 0, bool create = false);

  // retrieve a sub-set of LDRs in the particular block
  std::shared_ptr<Ldrset> getBlock(int blocknum);
  size_t getBlockCount() const;

  friend ostream & operator <<(ostream & out, Ldrset const & l);
#ifdef JCAMP_TO_JSON
  json to_json() const;
#endif

  // stuff for the parser
  Ldrset * jcamp_topnode;
  string _curfilename;
  std::set<string> getLabels() const;

private:
  void validate() const;

  std::map<Label, Ldr> _ldrs;
  std::vector<std::shared_ptr<Ldrset> > _blocks;
};

#ifdef JCAMP_TO_JSON
#include "json/src/json.hpp"
using json = nlohmann::json;

// convert json into an Ldr/Ldrset
void from_json(const json & jj, Ldr::Record & ldr);
void from_json(const json & jj, Ldr & ldr);
void from_json(const json & jj, Ldrset & ldrset);
#endif // JCAMP_TO_JSON

#endif // JCAMPDX_HPP
