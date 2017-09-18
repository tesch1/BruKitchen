/*
 * (c)2016 Michael Tesch. tesch1@gmail.com
 */
/*! \file       FileLoc.cpp
 * \brief       location in file, and associated exceptions
 *
 */
#include <sstream>
#include "FileLoc.hpp"
#include "cxx_utils.hpp"

using ppg::Loc_Error;

std::ostream &
operator <<(std::ostream & out, FileLoc const & l)
{
  //out << cxx_basename(l.filename) << ":" << l.first_line << "," << l.first_column << " '" << l.rawtext << "'";
  out << "|" << l.first_line << ":" << l.first_column << "-" ;
  if (l.first_line == l.last_line)
    out << l.last_column;
  else
    out << l.last_line << ":" << l.last_column;
  out << " '" << l.rawtext << "'";
  return out;
}

namespace ppg {

std::ostream &
operator <<(std::ostream & out, Loc_Error const & l)
{
  out << l._errstr << "\n from " << l.loc();
  return out;
}

const char *
Loc_Error::what() const noexcept
{
  return _errstr.c_str();
}

Loc_Error::Loc_Error(const FileLoc & loc, std::string str)
  : _loc(loc), _errstr(str)
{
  std::stringstream ers;
  ers << loc << " " << str;
  _errstr = ers.str();
}

Loc_Error::Loc_Error(const FileLoc & loc, const std::exception & uplevel)
  : _loc(loc)
{
  std::stringstream ers;
  ers << uplevel.what() << " (";
  ers << ")";
  _errstr = ers.str();
}

} // namespace ppg
