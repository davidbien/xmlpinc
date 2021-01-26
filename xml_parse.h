#pragma once

// xml_parse.h
// Xml parser.
// dbien
// 10JAN2021

#include "xml_ns.h"
#include "xml_types.h"
#include "xml_traits.h"
#include "_xmlplex.h"

__XMLP_BEGIN_NAMESPACE
__XMLPLEX_USING_NAMESPACE


// 1) Need mode to skip comments that can be turned on and off.
// 2) Ditto for processing instructions.
// 3) May want an FNextTag() as well as an FNextToken(). FNextTag() may skip CharData, whereas FNextToken() would not.


template < class t_TyXmlTraits >
class xml_parser
{
  typedef xml_parser _TyThis;
  friend xml_read_cursor< t_TyXmlTraits >;
public:
  typedef t_TyXmlTraits _TyXmlTraits;
  typedef typename _TyXmlTraits::_TyLexTraits _TyLexTraits;
  typedef _lexical_analyzer< _TyLexTraits > _TyLexicalAnalyzer;
  typedef _l_stream< _TyLexTraits > _TyStream;
  typedef typename _TyLexTraits::_TyUserObj _TyUserObj;
  typedef typename _TyLexTraits::_TyPtrUserObj _TyPtrUserObj;
  typedef typename _TyLexTraits::_TyTransport _TyTransport;
  typedef typename _TyXmlTraits::_TyUriAndPrefixMap _TyUriAndPrefixMap;
  typedef typename _TyXmlTraits::_TyStdStr _TyStdStr;
  typedef typename _TyXmlTraits::_TyStrView _TyStrView;
  typedef xml_read_cursor< _TyXmlTraits > _TyReadCursor;

  ~xml_parser() = default;
  xml_parser() = default;
  xml_parser( xml_parser const & ) = delete;
  xml_parser & operator=( xml_parser const & ) = delete;
  xml_parser( xml_parser && ) = default;
  xml_parser & operator=( xml_parser && ) = default;
  void swap( _TyThis & _r )
  {
    m_lexXml.swap( _r.m_lexXml );
    m_mapUris.swap( _r.m_mapUris );
    m_mapPrefixes.swap( _r.m_mapPrefixes );
  }

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

  _TyLexicalAnalyzer & GetLexicalAnalyzer()
  {
    return m_lexXml;
  }
  const _TyLexicalAnalyzer & GetLexicalAnalyzer() const
  {
    return m_lexXml;
  }
  _TyStream & GetStream()
  {
    return m_lexXml.GetStream();
  }
  const _TyStream & GetStream() const
  {
    return m_lexXml.GetStream();
  }
  _TyTransport & GetTransport()
  {
    return m_lexXml.GetTransport();
  }
  const _TyTransport & GetTransport() const
  {
    return m_lexXml.GetTransport();
  }
  _TyUserObj & GetUserObj()
  {
    return m_lexXml.GetUserObj();
  }
  const _TyUserObj & GetUserObj() const
  {
    return m_lexXml.GetUserObj();
  }
  _TyPtrUserObj & GetUserObjPtr()
  {
    return m_lexXml.GetUserObjPtr();
  }
  _TyUriAndPrefixMap & GetUriMap()
  {
    return m_mapUris;
  }
  const _TyUriAndPrefixMap & GetUriMap() const
  {
    return m_mapUris;
  }
  _TyUriAndPrefixMap & GetPrefixMap()
  {
    return m_mapPrefixes;
  }
  const _TyUriAndPrefixMap & GetPrefixMap() const
  {
    return m_mapPrefixes;
  }
  // Attaching a read cursor starts the parsing process - the first token is parsed when a read cursor is attached.
  void AttachReadCursor( _TyReadCursor & _xrc )
  {
    _xrc._AttachXmlParser( this );
  }
protected:
  // Accessed by xml_read_cursor<>:
  void _SetFilterWhitespaceCharData( bool _fFilterWhitespaceCharData )
  {
    // We set this filter into the user object since it can short-circuit token creation and then we
    //  won't create a token for the whitespace.
    m_lexXml.GetUserObj().SetFilterWhitespaceCharData( _fFilterWhitespaceCharData );
  }
  void _SetFilterAllTokenData( bool _fFilterAllTokenData )
  {
    // We set this filter into the user object since it can short-circuit token creation and then we
    //  won't create a token for the whitespace.
    m_lexXml.GetUserObj().SetFilterAllTokenData( _fFilterAllTokenData );
  }
  typename _TyUriAndPrefixMap::value_type const & _RStrAddPrefix( _TyStrView const & _rsv )
  {
    typename _TyUriAndPrefixMap::const_iterator cit = m_mapPrefixes.find( _rsv );
    if ( m_mapPrefixes.end() != cit )
      return *cit;
    pair< typename _TyUriAndPrefixMap::iterator, bool > pib = m_mapPrefixes.insert( _rsv );
    Assert( pib.second );
    return *pib.first;
  }
  typename _TyUriAndPrefixMap::value_type const & _RStrAddUri( _TyStrView const & _rsv )
  {
    typename _TyUriAndPrefixMap::const_iterator cit = m_mapUris.find( _rsv );
    if ( m_mapUris.end() != cit )
      return *cit;
    pair< typename _TyUriAndPrefixMap::iterator, bool > pib = m_mapUris.insert( _rsv );
    Assert( pib.second );
    return *pib.first;
  }  
  void _InitMapsAndUserObj()
  {
    m_mapUris.clear();
    m_mapPrefixes.clear();
    GetUserObj().ClearEntities();
  }
  _TyLexicalAnalyzer m_lexXml;
  _TyUriAndPrefixMap m_mapUris;
  _TyUriAndPrefixMap m_mapPrefixes;
};

__XMLP_END_NAMESPACE