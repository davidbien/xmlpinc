#pragma once

//          Copyright David Lawrence Bien 1997 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt).

// xml_exc.h
// XML parsing exceptions.
// dbien
// 07JAN2021

#include "_namdexc.h"
#include "xml_ns.h"
#include "xml_types.h"

__XMLP_BEGIN_NAMESPACE

// This exception will get thrown when there is an issue with the JSON stream.
class xml_parse_exception : public _t__Named_exception_errno<>
{
  typedef _t__Named_exception_errno<> _tyBase;
public:
  xml_parse_exception(const string_type &__s, vtyErrNo _nErrno = 0)
      : _tyBase(__s, _nErrno)
  {
  }
  xml_parse_exception(const char *_pcFmt, va_list args)
  {
    RenderVA(_pcFmt, args);
  }
  xml_parse_exception(int _errno, const char *_pcFmt, va_list args)
      : _tyBase(_errno)
  {
    RenderVA(_pcFmt, args);
  }
  using _tyBase::RenderVA;
};
// By default we will always add the __FILE__, __LINE__ even in retail for debugging purposes.
#define THROWXMLPARSEEXCEPTION(MESG, ...) ExceptionUsage<xml_parse_exception>::ThrowFileLineFunc(__FILE__, __LINE__, FUNCTION_PRETTY_NAME, MESG, ##__VA_ARGS__)
#define THROWXMLPARSEEXCEPTIONERRNO(ERRNO, MESG, ...) ExceptionUsage<xml_parse_exception>::ThrowFileLineFuncErrno(__FILE__, __LINE__, FUNCTION_PRETTY_NAME, ERRNO, MESG,##__VA_ARGS__)

__XMLP_END_NAMESPACE