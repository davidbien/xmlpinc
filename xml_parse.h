#pragma once

// xml_parse.h
// Xml parser.
// dbien
// 10JAN2021

#include "xml_ns.h"
#include "xml_types.h"
#include "xml_traits.h"
#include "xml_lex.h"

__XMLP_BEGIN_NAMESPACE
__XMLLEX_USING_NAMESPACE

// 1) Need mode to skip comments that can be turned on and off.
// 2) Ditto for processing instructions.
// 3) May want an FNextTag() as well as an FNextToken(). FNextTag() may skip CharData, whereas FNextToken() would not.


template < class t_TyXmlTraits >
class xml_parser
{
  typedef xml_parser _TyThis;
public:
  typedef t_TyXmlTraits _TyXmlTraits;
  typedef typename t_TyXmlTraits::_TyLexTraits _TyLexTraits;
  typedef _lexical_analyzer< _TyLexTraits > _TyLexicalAnalyzer;


  template < class... t_TysArgs >
  void emplaceTransport( t_TysArgs&&... _args )
  {
    m_lexXml.emplaceTransport( std::forward< t_TysArgs >( _args )... );
  }
  // Open a given transport object. This constructor is for variant transport
  template < class t_TyTransport, class ... t_TysArgs >
  void emplaceVarTransport( t_TysArgs ... _args )
  {
    m_lexXml.template emplaceVarTransport< t_TyTransport >( std::forward< t_TysArgs >( _args )... );
  }

  void AttachReadCursor( _TyReadCursor 

protected:
  _TyLexicalAnalyzer m_lexXml;
};

__XMLP_END_NAMESPACE