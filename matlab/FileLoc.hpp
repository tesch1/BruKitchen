/*
 * (c)2016 Michael Tesch. tesch1@gmail.com
 */
/*! \file       fileloc.hpp
 * \brief       location in a file & associated exception class
 *
 */
#ifndef FILELOC_HPP
#define FILELOC_HPP

#include <string>
#include <typeinfo>

/** \brief      file location for the source code producing a node
 *
 * [todo] Detailed description
 */
class FileLoc
{
public:
  FileLoc() :
    first_line(1), first_column(0), last_line(1), last_column(0),
    filename("?"), rawtext("*") {}
  unsigned int first_line;
  unsigned int first_column;
  unsigned int last_line;
  unsigned int last_column;
  std::string filename;
  std::string rawtext;
  friend std::ostream & operator <<(std::ostream & out, FileLoc const & l);
};

typedef class FileLoc _YYLTYPE;
#define BSEQ_YYLTYPE _YYLTYPE
#define JCAMP_YYLTYPE _YYLTYPE
#define YYLTYPE _YYLTYPE

// for flex
#define FILELOC_YYLLOC_DEFAULT(prefix, Current, Rhs, NN)                \
  do {                                                                  \
    if (NN) {                                                           \
      (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;            \
      (Current).first_column = YYRHSLOC (Rhs, 1).first_column;          \
      (Current).last_line    = YYRHSLOC (Rhs, NN).last_line;            \
      (Current).last_column  = YYRHSLOC (Rhs, NN).last_column;          \
      (Current).filename     = YYRHSLOC (Rhs, 1).filename;              \
      (Current).rawtext      = "";                                      \
    }                                                                   \
    else {                                                              \
      (Current).first_line   = (Current).last_line   =                  \
        YYRHSLOC(Rhs, 0).last_line;                                     \
      (Current).first_column = (Current).last_column =                  \
        YYRHSLOC(Rhs, 0).last_column;                                   \
      (Current).filename  = prefix._curfilename; /* new */              \
      (Current).rawtext  = "?";                          /* new */      \
    }                                                                   \
  } while (0)

namespace ppg {

class PPGNode;

/** \brief      [todo] Brief description
 *
 * [todo] Detailed description
 */
/*! file location associated errors */
class Loc_Error : public std::exception {
public:
  Loc_Error(const FileLoc & loc, std::string str);
  Loc_Error(const FileLoc & loc, const std::exception & uplevel);

  template <class HASLOC>
  Loc_Error(const HASLOC * node, const char *str)
    : _loc(), _errstr(str)
  {
    if (node) {
      _errstr = typeid(*node).name() + std::string(" ") + _errstr;
      _loc = node->loc();
    }
  }

  template <class HASLOC>
  Loc_Error(const HASLOC * node, std::string str)
    : _loc(), _errstr(str)
  {
    if (node) {
      _loc = node->loc();
      _errstr = typeid(*node).name() + std::string(" ") + _errstr;
    }
  }


  template <class HASLOC>
  Loc_Error(const HASLOC * node, const std::exception & uplevel)
    : _loc()
  {
    std::string ers;
    ers = uplevel.what() + std::string(" (");
    if (node) {
      ers += typeid(*node).name();
      _loc = node->loc();
    }
    ers += ")";
    _errstr = ers;
  }

  const FileLoc & loc() const { return _loc; };
  //virtual const char* what() const throw();
  virtual const char* what() const noexcept;
  friend std::ostream & operator <<(std::ostream & out, Loc_Error const & l);

private:
  FileLoc _loc;
  std::string _errstr;
};

} // namespace ppg

#endif // FILELOC_HPP
