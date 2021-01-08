#pragma once

// xml_token.h
// XML token object.
// dbien
// 07JAN2021
// This implements a wrapper on the _l_token<> class that is returned from the lexical analyzer.

#include "xml_ns.h"
#include "xml_types.h"
#include "_l_token.h"

__XMLP_BEGIN_NAMESPACE

// xml_token:
// This is the token base class - for all types of tokens that may be returned.
// Keep this as lightweight as possible.
// Allow attribute perusal through lambda only - this simplifies the interface quite nicely.
// Member functions access the methods of all types of tokens - throw if erroneous method called - ie. asking for the comment text from a non-comment token.

template < class t_TyTransport >
class xml_token
{
  typedef xml_token _TyThis;
public:
  typedef t_TyTransport _TyTransport;
  typedef typename _TyTransport::_TyChar _TyChar;
  typedef xml_user_obj< _TyChar > _TyUserObj;
  typedef _l_user_context< _TyTransport, _TyUserObj > _TyUserContext;
  typedef _l_token< _TyUserContext > _TyToken;

  ~xml_token() = default;
  xml_token( _TyToken const & _rtok )
    : m_tokToken( _rtok )
  {
  }
  xml_token( _TyToken && _rrtok )
    : m_tokToken( std::move( _rrtok ) )
  {
  }
  xml_token() = delete;
  xml_token( xml_token const & ) = default;
  xml_token & operator=( xml_token const & ) = default;
  xml_token( xml_token && ) = default;
  xml_token & operator=( xml_token && ) = default;


protected:
  _TyToken m_tokToken;
};

__XMLP_END_NAMESPACE
