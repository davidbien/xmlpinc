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
  typedef _l_transport_file< t_TyChar, false_type > _TyTransportFile;
  typedef _l_transport_file< t_TyChar, true_type > _TyTransportFileSwitchEndian;
  typedef _l_transport_mapped< t_TyChar, false_type > _TyTransportMapped;
  typedef _l_transport_mapped< t_TyChar, true_type > _TyTransportMappedSwitchEndian;
  typedef _l_transport_fixedmem< t_TyChar, false_type > _TyTransportFixed;
  typedef _l_transport_fixedmem< t_TyChar, true_type > _TyTransportFixedSwitchEndian;
  // For char8_t we don't include the switch endian types...
  using _TyTransportVar = typename std::conditional< is_same_v< t_TyChar, char8_t >, _l_transport_var< _TyTransportFile, _TyTransportMapped, _TyTransportFixed >,
    _l_transport_var< _TyTransportFile, _TyTransportFileSwitchEndian, _TyTransportMapped, _TyTransportMappedSwitchEndian, _TyTransportFixed, _TyTransportFixedSwitchEndian > >::type;
};
template < class t_TyChar >
using xml_var_get_var_transport_t = typename _xml_var_get_var_transport< t_TyChar >::_TyTransportVar;

// xml_parser_var:
// Templatized by transport template and set of characters types to support. To create a _l_transport_var use xml_var_get_var_transport_t<> or something like it.
template <  template < class ... > class t_tempTyTransport, class t_TyTp2DCharPack >
class xml_parser_var
{
  typedef xml_parser_var _TyThis;
public:
  typedef t_TyTp2DCharPack _TyTp2DCharPack;
  typedef MultiplexTuplePack2D_t< t_tempTyTransport, _TyTp2DCharPack > _TyTpTransports;
  typedef MultiplexTuplePack_t< TGetXmlTraitsDefault, _TyTpTransports > _TyTpXmlTraits;
  // Now get the variant type - include a monostate so that we can default initialize:
  typedef MultiplexMonostateTuplePack_t< xml_parser, _TyTpXmlTraits, variant > _TyParserVariant;
  // For the cursor we don't need a monostate because we will deliver the fully created type from a local method.
  typedef xml_read_cursor_var< _TyTpTransports > _TyXmlCursorVar;

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
    return holds_alternative< t_TyParser >( m_varParser );
  }
  // This will open the file with transport t_tempTyTransport<>
  _TyXmlCursorVar OpenFile( const char * _pszFileName )
  {
    VerifyThrow( !!_pszFileName );
    m_strFileName = _pszFileName;
    // First we must determine the type of the file and if there is a BOM then we have to not send the BOM to the transport.
    // In the case where there is a BOM then we will pass the open file handle which is seek()'d to the correct starting position
    //  past the BOM.
    VerifyThrowSz( FFileExists( m_strFileName.c_str() ), "File[%s] doesn't exist.", m_strFileName.c_str() );
    FileObj fo( OpenReadOnlyFile( m_strFileName.c_str() ) );
    VerifyThrowSz( fo.FIsOpen(), "Couldn't open file [%s]", m_strFileName.c_str() );
    uint8_t rgbyBOM[vknBytesBOM];
    size_t nbyRead;
    int iResult = FileRead( fo.HFileGet(), rgbyBOM, vknBytesBOM, &nbyRead );
    Assert( !iResult );
    Assert( nbyRead == vknBytesBOM );
    EFileCharacterEncoding efceEncoding = efceFileCharacterEncodingCount;
    if ( !iResult && ( nbyRead == vknBytesBOM ) )
      efceEncoding = GetCharacterEncodingFromBOM( rgbyBOM, nbyRead ); // sus the encoding and return its length in nbyRead.
    if ( efceFileCharacterEncodingCount == efceEncoding )
    {
      // Since we know we are opening XML we can use an easy heuristic to determine the encoding:
      efceEncoding = DetectEncodingXmlFile( rgbyBOM, vknBytesBOM );
      Assert( efceFileCharacterEncodingCount != efceEncoding ); // Unless the source isn't XML the above should succeed.
      VerifyThrowSz( efceFileCharacterEncodingCount != efceEncoding, "Unclear the encoding of the file - it doesn't begin with a \'<\' in any encoding apparently. That's a bad sign for it being an XML file." );
      nbyRead = 0; // Start at the beginning of the file.
    }
    // We read the file - which seeked the read pointer - reset the read point to after any BOM, if any.
    (void)NFileSeekAndThrow( fo.HFileGet(), nbyRead, vkSeekBegin );
    
    // Now, based on BOM and type, we must try to instantiate but fail with a throw if the type of file we want to instantiate isn't in our variant's type... interesting problem.
    switch( efceEncoding )
    {
      case efceUTF8:
      {
        _OpenFileCheckParserType< t_tempTyTransport< char8_t, false_type > >( fo, efceEncoding );
      }
      break;
      case efceUTF16BE:
      {
        _OpenFileCheckParserType< t_tempTyTransport< char16_t, integral_constant< bool, vkfIsLittleEndian > > >( fo, efceEncoding );
      }
      break;
      case efceUTF16LE:
      {
        _OpenFileCheckParserType< t_tempTyTransport< char16_t, integral_constant< bool, vkfIsBigEndian > > >( fo, efceEncoding );
      }
      break;
      case efceUTF32BE:
      {
        _OpenFileCheckParserType< t_tempTyTransport< char32_t, integral_constant< bool, vkfIsLittleEndian > > >( fo, efceEncoding );
      }
      break;
      case efceUTF32LE:
      {
        _OpenFileCheckParserType< t_tempTyTransport< char32_t, integral_constant< bool, vkfIsBigEndian > > >( fo, efceEncoding );
      }
      break;
    }
    return GetReadCursor();
  }
  // Create and return an attached read cursor. The parser would have already need to have an open transport stream.
  _TyXmlCursorVar GetReadCursor()
  {
    Assert( !FIsNull() );
    return std::visit( _VisitHelpOverloadFCall {
      [](monostate) -> _TyXmlCursorVar
      {
        THROWNAMEDBADVARIANTACCESSEXCEPTION("Need to create a parser first.");
        return _TyXmlCursorVar();
      },
      [this]( auto & _tParser ) -> _TyXmlCursorVar
      {
        return _TyXmlCursorVar( _tParser.GetReadCursor() );
      }
    }, m_varParser );
  }
protected:
  template < class t_TyTransport >
  void _OpenFileCheckParserType( FileObj & _rfo, EFileCharacterEncoding _efceEncoding )
  {
    typedef t_TyTransport _TyTransport;
    typedef TGetXmlTraitsDefault< _TyTransport > _TyXmlTraits;
    typedef xml_parser< _TyXmlTraits > _TyXmlParser;
    typedef conditional_t< has_type_v< _TyXmlParser, _TyParserVariant >, _TyXmlParser, false_type > _TyXmlParserInVariant;
    _OpenFileParser< _TyXmlParserInVariant >( _rfo, _efceEncoding );
  }
  template < class t_TyParser >
  void _OpenFileParser( FileObj & _rfo, EFileCharacterEncoding )
  {
    typedef t_TyParser _TyParser;
    // We have a type, now, that is present in the variant...
    _TyParser & rp = m_varParser.emplace< t_TyParser >();
    rp.emplaceTransport( _rfo );

  }
  template <>
  void _OpenFileParser< false_type >( FileObj & _rfo, EFileCharacterEncoding _efceEncoding )
  {
    VerifyThrowSz( false, "Encoding [%s] not supported by the variant object in this xml_parser_var<>.", PszCharacterEncodingShort( _efceEncoding ) );
  }
  string m_strFileName; // Record this because it doesn't cost much.
  _TyParserVariant m_varParser;
};

__XMLP_END_NAMESPACE