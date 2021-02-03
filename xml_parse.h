#pragma once

// xml_parse.h
// Xml parser.
// dbien
// 10JAN2021

#include "xml_ns.h"
#include "xml_types.h"
#include "xml_traits.h"
#include "_xmlplex_utf32.h"
#include "_xmlplex_utf16.h"
#include "_xmlplex_utf8.h"

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
  typedef typename _TyXmlTraits::_TyChar _TyChar;
  typedef typename _TyXmlTraits::_TyLexTraits _TyLexTraits;
  typedef _lexical_analyzer< _TyLexTraits > _TyLexicalAnalyzer;
  typedef _l_stream< _TyLexTraits > _TyStream;
  typedef typename _TyLexTraits::_TyUserObj _TyUserObj;
  typedef typename _TyLexTraits::_TyPtrUserObj _TyPtrUserObj;
  typedef typename _TyLexTraits::_TyTransport _TyTransport;
  typedef typename _xml_namespace_map_traits< _TyChar >::_TyUriAndPrefixMap _TyUriAndPrefixMap;
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
  // This method will create and attach a read cursor.
  _TyReadCursor GetReadCursor()
  {
    _TyReadCursor xrc;
    AttachReadCursor( xrc );
    return xrc;
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
    pair< typename _TyUriAndPrefixMap::iterator, bool > pib = m_mapPrefixes.insert( _TyStdStr( _rsv ) );
    Assert( pib.second );
    return *pib.first;
  }
  typename _TyUriAndPrefixMap::value_type const & _RStrAddUri( _TyStrView const & _rsv )
  {
    typename _TyUriAndPrefixMap::const_iterator cit = m_mapUris.find( _rsv );
    if ( m_mapUris.end() != cit )
      return *cit;
    pair< typename _TyUriAndPrefixMap::iterator, bool > pib = m_mapUris.insert( _TyStdStr( _rsv ) );
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

template < class t_TyChar >
struct _xml_var_get_var_transport
{
  typedef _l_transport_file< t_TyChar, false > _TyTransportFile;
  typedef _l_transport_file< t_TyChar, true > _TyTransportFileSwitchEndian;
  typedef _l_transport_mapped< t_TyChar, false > _TyTransportMapped;
  typedef _l_transport_mapped< t_TyChar, true > _TyTransportMappedSwitchEndian;
  typedef _l_transport_fixed< t_TyChar, false > _TyTransportFixed;
  typedef _l_transport_fixed< t_TyChar, true > _TyTransportFixedSwitchEndian;
  // For char8_t we don't include the switch endian types...
  using _TyTransportVar = typename std::conditional< is_same_v< t_TyChar, char8_t >, _l_transport_var< _TyTransportFile, _TyTransportMapped, _TyTransportFixed >,
    _l_transport_var< _TyTransportFile, _TyTransportFileSwitchEndian, _TyTransportMapped, _TyTransportMappedSwitchEndian, _TyTransportFixed, _TyTransportFixedSwitchEndian > >::type;
};
template < class t_TyChar >
using xml_var_get_var_transport_t = typename _xml_var_get_var_transport< t_TyChar >::_TyTransportVar;

// Adaptors to allow access to single switch endian types. Yes, this is annoying but mostly the _xml_var_get_var_transport would be used to access this functionality one would think.
template < class t_TyChar >
using xml_get_switch_endian_transport_file_t = typename _l_transport_file< t_TyChar, true >;
template < class t_TyChar >
using xml_get_switch_endian_transport_mapped_t = typename _l_transport_mapped< t_TyChar, true >;
template < class t_TyChar >
using xml_get_switch_endian_transport_fixed_t = typename _l_transport_fixed< t_TyChar, true >;

// xml_parser_var:
// Templatized by transport template and set of characters types to support. To create a _l_transport_var use xml_var_get_var_transport_t<> or something like it.
template < template < class t_TyChar > t_TempTyTransport, class t_TyTpCharPack = tuple< char32_t, char16_t, char8_t > >
class xml_parser_var
{
  typedef xml_parser_var _TyThis;
public:
  typedef t_TyTpCharPack _TyTpCharPack;
  typedef MultiplexTuplePack_t< t_TempTyTransport, _TyTpCharPack > _TyTpTransports;
  typedef MultiplexTuplePack_t< TGetXmlTraitsDefault, _TyTpTransports > _TyTpXmlTraits;
  // Now get the variant type - include a monostate so that we can default initialize:
  typedef MultiplexMonostateTuplePack_t< xml_parser, _TyTpXmlTraits, variant > _TyParserVariant;
  // For the cursor we don't need a monostate because we will deliver the fully created type from a local method.
  typedef xml_cursor_var< _TyTpTransports > _TyXmlCursorVar;

  ~xml_parser_var() = default;
  xml_parser_var() = default;
  xml_parser_var( xml_parser_var const & ) = delete;
  xml_parser_var & operator=( xml_parser_var const & ) = delete;
  xml_parser_var( xml_parser_var && ) = default;
  xml_parser_var & operator=( xml_parser_var && ) = default;
  void swap( _TyThis & _r )
  {
    m_varParser.swap( _r );
  }

  bool FIsNull() const
  {
    return FIsA< monostate >();
  }
  template < class t_TyParser >
  bool FIsA() const
  {
    return holds_alternative< t_TyParser >();
  }
  // Create and return an attached read cursor.
  _TyXmlCursorVar GetReadCursor()
  {
    Assert( !FIsNull() );
    return std::visit( _VisitHelpOverloadFCall {
      [](monostate) 
      {
        THROWNAMEDBADVARIANTACCESSEXCEPTION("Need to create a parser first.");
      },
      [this]( auto _tParser ) -> _TyXmlCursorVar
      {
        return _TyXmlCursorVar( _tParser.GetReadCursor() );
      }
    }, m_varParser );
  }
protected:
  _TyParserVariant m_varParser;
};

__XMLP_END_NAMESPACE