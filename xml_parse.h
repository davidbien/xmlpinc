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
  typedef typename _TyXmlTraits::_TyLexTraits _TyLexTraits;
  typedef _lexical_analyzer< _TyLexTraits > _TyLexicalAnalyzer;
  typedef typename _TyXmlTraits::_TyUriMap _TyUriMap;
  typedef typename _TyXmlTraits::_TyStdStr _TyStdStr;
  typedef typename _TyXmlTraits::_TyStrView _TyStrView;

  // Emplacing the transport will read the byte order mark from the source file.
  // If that byte order mark isn't compatible with the character type we are using
  //  to read that file then it will throw a parse exception.
  template < class... t_TysArgs >
  void emplaceTransport( t_TysArgs&&... _args )
  {
    m_lexXml.emplaceTransport( std::forward< t_TysArgs >( _args )... );
  }
  // Open a given transport object. This constructor is for variant transport
  template < class t_TyTransport, class ... t_TysArgs >
  void emplaceVarTransport( t_TysArgs&& ... _args )
  {
    m_lexXml.template emplaceVarTransport< t_TyTransport >( std::forward< t_TysArgs >( _args )... );
  }

  // Attaching a read cursor starts the parsing process - the first token is parsed when a read cursor is attached.
  // The read cursor will then be pointing at that first-parsed token.
  void AttachReadCursor( _TyReadCursor & _xrc )
  {

  }

  _TyUriMap::value_type const & RStrAddUri( _tyStrView const & _rsv )
  {
    _TyUriMap::const_iterator cit = m_mapUris.find( _rsv );
    if ( m_mapUris.end() != cit )
      return *cit;
    pair< _TyUriMap::iterator, bool > pib = m_mapUris.insert( _rsv );
    Assert( pib.second );
    return *pib.first;
  }

protected:
  // Accessed by xml_read_cursor<>:
  void _SetFilterWhitespaceCharData( bool _fFilterWhitespaceCharData )
  {
    // We set this filter into the user object since it can short-circuit token creation and then we
    //  won't create a token for the whitespace.
    m_lexXml.RGetUserObj().SetFilterWhitespaceCharData( _fFilterWhitespaceCharData );
  }
  void _SetFilterAllTokenData( bool _fFilterAllTokenData )
  {
    // We set this filter into the user object since it can short-circuit token creation and then we
    //  won't create a token for the whitespace.
    m_lexXml.RGetUserObj().SetFilterAllTokenData( _fFilterAllTokenData );
  }
  
  _TyLexicalAnalyzer m_lexXml;
  _TyUriMap m_mapUris;
};

__XMLP_END_NAMESPACE