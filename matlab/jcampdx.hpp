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

#include <string>
#include <deque>
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

enum record_type {
  RECORD_TEXT,
  RECORD_STRING,
  RECORD_QSTRING,
  RECORD_NUMERIC,
  RECORD_GROUP,
  RECORD_UNSET,
};

class Ldr;

class Record {
public:
  Record();
  Record(std::string str, bool quoted = false);
  Record(real_t val);
  Record(Ldr * ldr);

  const real_t & num() const;
  const std::string & str() const;
  record_type type() const;
  void setNum(real_t val);
  void setStr(std::string str, bool quoted = false);
  const Ldr & group() const;
  void setType(record_type type);

  friend std::ostream & operator <<(std::ostream & out, Record const & l);

private:
  record_type _type;
  std::string _str;
  real_t _num;
  Ldr * _ldr;
};

//! string to label
struct Label : public std::string {
  Label(std::string name);
};

class Ldr {
public:
  Ldr();
  Ldr(std::string label);
  Ldr(record_type type, std::string str, std::string label = "");
  Ldr(record_type type, real_t val, std::string label = "");

  size_t size() const;
  std::vector<int> shape() const;
  std::string label() const;

  // get values
  const std::string & str(size_t idx = 0) const;
  const real_t & num(size_t idx = 0) const;
  record_type type(size_t idx = 0) const;
  void setStr(std::string str, size_t idx = 0);
  void setNum(real_t val, size_t idx = 0);

  void setLabel(std::string label);
  void setShape(std::string variable_list);
  void setShape(const Ldr & shape);

  void appendStr(std::string str, bool quoted = false);
  void appendNum(real_t val);
  void appendGroup(Ldr * group);

  friend std::ostream & operator <<(std::ostream & out, Ldr const & l);

private:
  std::vector<Record> _data;
  //std::vector<real_t> _num; // special case -- optimize table data .. someday
  std::string _label;
  std::vector<int> _shape;
  enum shape_type { SHAPE_1D, SHAPE_2D, SHAPE_XYY, SHAPE_XYXY } _shape_type;
};

//
class Ldrset
{
public:

  Ldrset();
  Ldrset(const std::string filename);
  ~Ldrset();

  void loadFile(const std::string filename);
  void loadString(const std::string jdxstring, std::string nametag="string");
  void clear();
  size_t size() const;

  //
  void addLdr(const Ldr & ldr);
  void addLdr(const std::string label, const Ldr & ldr);
  void addBlock(Ldrset * ldrset);
  void deleteLdr(const std::string label);
  Ldr & getLdr(const std::string label);

  // data retreival
  bool labelExists(const std::string label) const;
  std::string getString(const std::string label, size_t idx = 0) const;
  real_t getDouble(const std::string label, size_t idx = 0) const;

  void newEmpty(const std::string label);
  void setString(const std::string label, std::string str, size_t idx = 0, bool create = false);
  void setDouble(const std::string label, real_t val, size_t idx = 0, bool create = false);

  // retrieve a sub-set of LDRs in the particular block
  Ldrset & getBlock(int blocknum);

  friend std::ostream & operator <<(std::ostream & out, Ldrset const & l);

  // stuff for the parser
  Ldrset * jcamp_topnode;
  std::string _curfilename;
  std::set<std::string> getLabels();

private:
  void validate() const;

  std::map<Label, Ldr> _ldrs;
  std::deque<std::shared_ptr<Ldrset> > _blocks;
};

#endif // JCAMPDX_HPP
