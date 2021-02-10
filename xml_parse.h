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

inline EFileCharacterEncoding _XmlParserOpenFile( const string & _rstrFileName, FileObj & _rfoResult )
{
  // First we must determine the type of the file and if there is a BOM then we have to not send the BOM to the transport.
  // Also if this parser doesn't support the encoding that we find then we have to throw an error.
  // In the case where there is a BOM then we will pass the open file handle which is seek()'d to the correct starting position
  //  past the BOM.
  VerifyThrowSz( FFileExists( _rstrFileName.c_str() ), "File[%s] doesn't exist.", _rstrFileName.c_str() );
  FileObj fo( OpenReadOnlyFile( _rstrFileName.c_str() ) );
  VerifyThrowSz( fo.FIsOpen(), "Couldn't open file [%s]", _rstrFileName.c_str() );
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
  _rfoResult.swap( fo );
  return efceEncoding;
}

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
    if ( this == &_r )
      return;
    m_lexXml.swap( _r.m_lexXml );
    m_mapUris.swap( _r.m_mapUris );
    m_mapPrefixes.swap( _r.m_mapPrefixes );
    m_strFileName.swap( _r.m_strFileName );
  }

  // Open this file using the transport template given.
  // We allow for multiple types of the same transport with different switch endian values because this
  //  is a configuration that might want to be used since it is a bit smaller (and actually we might
  //  want to integrate it into the xml_parser_var to make it's binary footprint smaller).
  template < template < class ... > class t_tempTyTransport >
  _TyReadCursor OpenFileVar( const char * _pszFileName )
  {
    // Produce a hard compiler error if the transport variant doesn't contain at least one possibility of this t_tempTyTransport.
    typedef typename _TyTransport::_TyVariant _TyTransportVariant;
    static_assert( has_type_v< t_tempTyTransport< _TyChar, false_type >, _TyTransportVariant >
                  || has_type_v< t_tempTyTransport< _TyChar, true_type >, _TyTransportVariant > );
    VerifyThrow( !!_pszFileName );
    m_strFileName = _pszFileName;
    FileObj fo;
    EFileCharacterEncoding efceEncoding = _XmlParserOpenFile( m_strFileName, fo );
    // First check if we can't possibly support this encoding.
    size_t bvSupportedEncodings = _TyTransport::GetSupportedEncodingBitVector();
    if ( !( bvSupportedEncodings & ( 1ull << efceEncoding ) ) )
    {
      SetLastErrNo( vkerrInvalidArgument );
      THROWXMLPARSEEXCEPTIONERRNO( vkerrInvalidArgument, "File [%s] is in [%s] character encoding. This parser type doesn't support that encoding.", 
          m_strFileName.c_str(), PszCharacterEncodingShort( efceEncoding ) );
    }
    // As with the var parser we try to match the encoding to a type that is within the variant.
    // If it doesn't match then we throw an error.
    switch( efceEncoding )
    {
      case efceUTF8:
      {
        _OpenParserCheckTransportType< t_tempTyTransport< char8_t, false_type > >( fo );
      }
      break;
      case efceUTF16BE:
      {
        _OpenParserCheckTransportType< t_tempTyTransport< char16_t, integral_constant< bool, vkfIsLittleEndian > > >( fo );
      }
      break;
      case efceUTF16LE:
      {
        _OpenParserCheckTransportType< t_tempTyTransport< char16_t, integral_constant< bool, vkfIsBigEndian > > >( fo );
      }
      break;
      case efceUTF32BE:
      {
        _OpenParserCheckTransportType< t_tempTyTransport< char32_t, integral_constant< bool, vkfIsLittleEndian > > >( fo );
      }
      break;
      case efceUTF32LE:
      {
        _OpenParserCheckTransportType< t_tempTyTransport< char32_t, integral_constant< bool, vkfIsBigEndian > > >( fo );
      }
      break;
    }
    return GetReadCursor();
  }
  template < class t_TyTransport >
  void _OpenParserCheckTransportType( FileObj & _rfo )
  {
    typedef t_TyTransport _TyTransportCheck;
    typedef typename _TyTransport::_TyVariant _TyTransportVariant;
    typedef conditional_t< has_type_v< _TyTransportCheck, _TyTransportVariant >, _TyTransportCheck, false_type > _TyTransportResult;
    _OpenFileVarParser< _TyTransportResult >( _rfo);
  }
  template < class t_TyTransport >
  void _OpenFileVarParser( FileObj & _rfo )
  {
    emplaceVarTransport< t_TyTransport >( _rfo );
  }
  template <>
  void _OpenFileVarParser< false_type >( FileObj & _rfo )
  {
    Assert( 0 ); // We should never get here because we should never be in the case where we would try to instatiate this type...
    // But the code won't compile with out it. Our test cases test all possibilites so we don't even need to throw from here.
  }

  // This will attempt to open the file. If it is in the wrong format it will throw an xml_parse_exception.
  _TyReadCursor OpenFile( const char * _pszFileName )
  {
    VerifyThrow( !!_pszFileName );
    m_strFileName = _pszFileName;
    FileObj fo;
    EFileCharacterEncoding efceEncoding = _XmlParserOpenFile( m_strFileName, fo );
    // If we don't support this type of file then we should set the "invalid argument" error code and throw such an error:
    EFileCharacterEncoding efceSupported = _TyTransport::GetSupportedCharacterEncoding();
    if ( efceEncoding != efceSupported )
    {
      SetLastErrNo( vkerrInvalidArgument );
      THROWXMLPARSEEXCEPTIONERRNO( vkerrInvalidArgument, "File [%s] is in [%s] character encoding. This parser only supports the [%s] character encoding.", 
          m_strFileName.c_str(), PszCharacterEncodingShort( efceEncoding ), PszCharacterEncodingShort( efceSupported )  );
    }
    emplaceTransport( fo );
    return GetReadCursor();
  }
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
  string m_strFileName; // Record this because it doesn't cost much - it may be enpty.
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
  typedef xml_read_cursor_var< _TyTpTransports > _TyReadCursorVar;

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
  _TyReadCursorVar OpenFile( const char * _pszFileName )
  {
    return _OpenFile< t_tempTyTransport >( _pszFileName );
  }
  // This will open the file with the transport passed which must be within the variant t_tempTyTransport.
  // If it isn't we want to fail at compile time - not at runtime.
  template < template < class ... > class t__tempTyTransport >
  _TyReadCursorVar OpenFileVar( const char * _pszFileName )
  {
    return _OpenFile< t__tempTyTransport >( _pszFileName );
  }
  // Create and return an attached read cursor. The parser would have already need to have an open transport stream.
  _TyReadCursorVar GetReadCursor()
  {
    Assert( !FIsNull() );
    return std::visit( _VisitHelpOverloadFCall {
      [](monostate) -> _TyReadCursorVar
      {
        THROWNAMEDBADVARIANTACCESSEXCEPTION("Need to create a parser first.");
        return _TyReadCursorVar();
      },
      [this]( auto & _tParser ) -> _TyReadCursorVar
      {
        return _TyReadCursorVar( _tParser.GetReadCursor() );
      }
    }, m_varParser );
  }
protected:
  // This will open the file with transport t__tempTyTransport. We should be able to share this method between
  //  single and variant transport implementations if we are sneaky (and I am sneaky).
  template < template < class ... > class t__tempTyTransport >
  _TyReadCursorVar _OpenFile( const char * _pszFileName )
  {
    VerifyThrow( !!_pszFileName );
    m_strFileName = _pszFileName;
    FileObj fo;
    EFileCharacterEncoding efceEncoding = _XmlParserOpenFile( m_strFileName, fo );
    // Now, based on BOM and type, we must try to instantiate but fail with a throw if the type of file we want to instantiate isn't in our variant's type... interesting problem.
    switch( efceEncoding )
    {
      case efceUTF8:
      {
        _OpenFileCheckParserType< t__tempTyTransport< char8_t, false_type > >( fo, efceEncoding );
      }
      break;
      case efceUTF16BE:
      {
        _OpenFileCheckParserType< t__tempTyTransport< char16_t, integral_constant< bool, vkfIsLittleEndian > > >( fo, efceEncoding );
      }
      break;
      case efceUTF16LE:
      {
        _OpenFileCheckParserType< t__tempTyTransport< char16_t, integral_constant< bool, vkfIsBigEndian > > >( fo, efceEncoding );
      }
      break;
      case efceUTF32BE:
      {
        _OpenFileCheckParserType< t__tempTyTransport< char32_t, integral_constant< bool, vkfIsLittleEndian > > >( fo, efceEncoding );
      }
      break;
      case efceUTF32LE:
      {
        _OpenFileCheckParserType< t__tempTyTransport< char32_t, integral_constant< bool, vkfIsBigEndian > > >( fo, efceEncoding );
      }
      break;
    }
    return GetReadCursor();
  }
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
    SetLastErrNo( vkerrInvalidArgument );
    THROWXMLPARSEEXCEPTIONERRNO( vkerrInvalidArgument, "File [%s] is in [%s] character encoding." 
        "This variant parser type doesn't support that character encoding. Add types to the parser to support it", 
        m_strFileName.c_str(), PszCharacterEncodingShort( efceEncoding ), PszCharacterEncodingShort( efceSupported )  );
  }
  string m_strFileName; // Record this because it doesn't cost much.
  _TyParserVariant m_varParser;
};

__XMLP_END_NAMESPACE