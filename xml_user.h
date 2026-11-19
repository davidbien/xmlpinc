#pragma once

//          Copyright David Lawrence Bien 1997 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt).

// xml_user.h
// "User" objects for the XML parser.
// dbien
// 17DEC2020

// These provide a "default translation" that many lexical analyzers can just use as-is.
// Users may want to override the methods in some cases - especially where character translation is required.
// A good example is where a piece of text may represent a hexidecimal number, etc. The translator in the overridden
//  user object would translate and also potentially check for overflow, etc.

#include "_util.h"
#include "_l_chrtr.h"
#include "xml_ns.h"
#include "xml_types.h"
#include "xml_exc.h"

__XMLP_BEGIN_NAMESPACE

// _xml_user_obj_base_dtd:
// Contains some infrastructure to support the DTD if desired.
template < class t_TyChar, bool t_kfSupportDTD >
class _xml_user_obj_base_dtd
{
  typedef _xml_user_obj_base_dtd _TyThis;
public:
  typedef t_TyChar _TyChar;
  static constexpr bool s_kfSupportDTD = true;
  typedef basic_string< _TyChar > _TyStdStr;
  typedef basic_string_view< _TyChar > _TyStrView;
#ifdef _MSC_VER
  typedef StringTransparentHash< _TyChar > _TyStringTransparentHash; // Allow lookup by string_view without creating a string.
  typedef unordered_map< _TyStdStr, _TyStdStr, _TyStringTransparentHash, std::equal_to<void> > _TyEntityMap;
#else
  typedef map< _TyStdStr, _TyStdStr, std::less<> > _TyEntityMap;
#endif

  _TyEntityMap m_mapParameterEntities{};

  ~_xml_user_obj_base_dtd() = default;
  _xml_user_obj_base_dtd() = default;
  _xml_user_obj_base_dtd( _xml_user_obj_base_dtd const & ) = delete;
  _xml_user_obj_base_dtd & operator =( _xml_user_obj_base_dtd const & ) = delete;
  _xml_user_obj_base_dtd( _xml_user_obj_base_dtd && ) = default;
  _xml_user_obj_base_dtd & operator =( _xml_user_obj_base_dtd && ) = default;
  void swap( _TyThis & _r )
  {
    m_mapParameterEntities.swap( _r.m_mapParameterEntities );
  }
  void ClearParamEntities()
  {
     m_mapParameterEntities.clear();
  }
  template < class t_TyTransportCtxt >
  const _TyStdStr & _RLookupParameterEntity( _l_data_typed_range const & _rdr, t_TyTransportCtxt const & _rcxt ) const
  {
    _TyStrView sv;
    _rcxt.GetStringView( sv, _rdr );
    typename _TyEntityMap::const_iterator cit = m_mapParameterEntities.find( sv );
    if ( m_mapParameterEntities.end() == cit )
      THROWXMLPARSEEXCEPTION("Can't find parameter entity [%s].", StrConvertString< char >( sv ).c_str() );
    return cit->second;
  }
};
// Non-DTD base.
template < class t_TyChar >
class _xml_user_obj_base_dtd< t_TyChar, false >
{
  typedef _xml_user_obj_base_dtd _TyThis;
public:
  typedef t_TyChar _TyChar;
  static constexpr bool s_kfSupportDTD = false;
  typedef basic_string< _TyChar > _TyStdStr;
  typedef basic_string_view< _TyChar > _TyStrView;
#ifdef _MSC_VER
  typedef StringTransparentHash< _TyChar > _TyStringTransparentHash; // Allow lookup by string_view without creating a string.
  typedef unordered_map< _TyStdStr, _TyStdStr, _TyStringTransparentHash, std::equal_to<void> > _TyEntityMap;
#else
  typedef map< _TyStdStr, _TyStdStr, std::less<> > _TyEntityMap;
#endif

  void swap( _TyThis & _r )
  {
  }
  void ClearParamEntities()
  {
  }
  template < class t_TyTransportCtxt >
  const _TyStdStr& _RLookupParameterEntity(_l_data_typed_range const& _rdr, t_TyTransportCtxt const& _rcxt) const
  {
    // no-op - just here so that things compile.
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnull-dereference"
#endif //__GNUC__
    return *(_TyStdStr*)0;
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif //__GNUC__
  }
};

// char32_t:
template < class t_TyChar >
struct _TGetCharRefConvertBuffer
{
  typedef t_TyChar type;
};
template <>
struct _TGetCharRefConvertBuffer<char>
{
  typedef char type[4];
};
template <>
struct _TGetCharRefConvertBuffer<char8_t>
{
  typedef char8_t type[4];
};
template <>
struct _TGetCharRefConvertBuffer<char16_t>
{
  typedef char16_t type[2];
};
template <>
struct _TGetCharRefConvertBuffer<wchar_t>
{
#ifdef BIEN_WCHAR_16BIT
  typedef wchar_t type[2];
#else 
  typedef wchar_t type;
#endif
};
template < class t_TyChar >
using TGetCharRefConvertBuffer = typename _TGetCharRefConvertBuffer< t_TyChar >::type;

template < class t_TyChar, bool t_kfSupportDTD >
class xml_user_obj 
  : public _xml_user_obj_base_dtd< t_TyChar, t_kfSupportDTD >
{
  typedef xml_user_obj _TyThis;
  typedef _xml_user_obj_base_dtd< t_TyChar, t_kfSupportDTD > _TyBase;
public:
  using _TyBase::s_kfSupportDTD;
  using typename _TyBase::_TyChar;
  using typename _TyBase::_TyStdStr;
  using typename _TyBase::_TyStrView;
  using typename _TyBase::_TyEntityMap;
  typedef _l_data<> _TyData;
  typedef _l_action_object_base< _TyChar, false > _TyAxnObjBase;
  typedef TGetCharRefConvertBuffer< _TyChar > _TyCharRefConvertBuffer;

  // Entity reference lookup:
  _TyEntityMap m_mapEntities;
  
  // Then this is true then when s_knTokenCharData is received it is check for be entirely composed of whitespace
  // and if so it is ignored entirely and not retunred to the xml parser.
  bool m_fFilterWhitespaceCharData{false};
  bool m_fFilterAllTokenData{false};

  xml_user_obj()
  {
    ClearEntities(); // initialize the standard entities.
  }
  xml_user_obj( xml_user_obj const & _r ) = delete;
  xml_user_obj & operator =( xml_user_obj const & ) = delete;
  xml_user_obj( xml_user_obj && _rr ) = default;
  xml_user_obj & operator =( xml_user_obj && ) = default;
  void swap( _TyThis & _r )
  {
    _TyBase::swap( _r );
    m_mapEntities.swap( _r.m_mapEntities );
  }
  _TyEntityMap & GetEntityMap()
  {
    return m_mapEntities;
  }
  const _TyEntityMap & GetEntityMap() const
  {
    return m_mapEntities;
  }
  void SetFilterWhitespaceCharData( bool _fFilterWhitespaceCharData )
  {
    m_fFilterWhitespaceCharData = _fFilterWhitespaceCharData;
  }
  void SetFilterAllTokenData( bool _fFilterAllTokenData )
  {
    m_fFilterAllTokenData = _fFilterAllTokenData;
  }
  using _TyBase::_RLookupParameterEntity;
  void ClearEntities()
  {
    if ( m_mapEntities.size() != 5 )
    {
      m_mapEntities.clear();
      m_mapEntities = { { _TyStdStr( str_array_cast<_TyChar>("quot") ), _TyStdStr( str_array_cast<_TyChar>("\"") ) }, 
                        { _TyStdStr( str_array_cast<_TyChar>("amp") ), _TyStdStr( str_array_cast<_TyChar>("&") ) }, 
                        { _TyStdStr( str_array_cast<_TyChar>("apos") ), _TyStdStr( str_array_cast<_TyChar>("\'") ) }, 
                        { _TyStdStr( str_array_cast<_TyChar>("lt") ), _TyStdStr( str_array_cast<_TyChar>("<") ) }, 
                        { _TyStdStr( str_array_cast<_TyChar>("gt") ), _TyStdStr( str_array_cast<_TyChar>(">") ) } };
    }
    _TyBase::ClearParamEntities();
  }
  template < class t_TyStream >
  bool FProcessAndFilterToken( _TyAxnObjBase * _paobCurToken, t_TyStream const & _rstrm, const vtyDataPosition _kposEndToken ) const
  {
    typedef typename t_TyStream::_TyTraits _TyLexTraits;
    if ( m_fFilterAllTokenData )
    {
      vtyTokenIdent tid = _paobCurToken->VGetTokenId();
      Assert( ( tid == s_knTokenSTag ) || ( tid == s_knTokenETag ) );
      if ( tid == s_knTokenSTag )
      {
        // Then rid all token attribute data.
        typedef TyGetTokenSTag< _TyLexTraits, false > _TyTokenSTag;
        _TyTokenSTag * ptst = static_cast< _TyTokenSTag * >( _paobCurToken );
        typedef TyGetTriggerSaveAttributes< _TyLexTraits, false > _TyTriggerSaveAttributes;
        _TyTriggerSaveAttributes & rtgSaveAttributes = ptst->template GetConstituentTriggerObj< _TyTriggerSaveAttributes >();
        rtgSaveAttributes.Clear(); // Clear all accumulated attributes.
      }
      return false;
    }
    else
    if ( _paobCurToken->VGetTokenId() != s_knTokenCharData )
      return false;
    // Move through and fix up the CharData endpoints since we had to do things this way.
    // We know the ultimate type of this token so we can cast:
    typedef TyGetTokenCharData< _TyLexTraits, false > _TyTokenCharData;
    typedef TyGetTriggerCharDataEnd< _TyLexTraits, false > _TyTriggerCharDataEnd;
    _TyTokenCharData * ptcd = static_cast< _TyTokenCharData * >( _paobCurToken );
    _TyTriggerCharDataEnd & rtgCharDataEnd = ptcd->template GetConstituentTriggerObj< _TyTriggerCharDataEnd >();
    // First of all, if we have more than one data_typed_range_el or the one we have it not Plain text
    //  then we don't filter.
    _TyData & rdt = rtgCharDataEnd.RDataGet();
    vtyDataPosition posCur = _rstrm.PosTokenStart();
    auto lambdaProcessCharData = [_kposEndToken,&posCur]( _l_data_typed_range * _pdtrBegin, _l_data_typed_range * _pdtrEnd ) -> size_t
    {
      _l_data_typed_range * pdtrCur = _pdtrBegin;
      for ( ; pdtrCur != _pdtrEnd; ++pdtrCur )
      {
        if ( ( pdtrCur->m_posBegin - ( vkdpNullDataPosition == pdtrCur->m_posEnd ) ) >= _kposEndToken ) // yes! -"=="! sneaky but correct.
        {
          // Then we received some triggers after the end of the current token - likely we will fail but not until the next token is processed.
          // Don't process any further:
          return pdtrCur - _pdtrBegin;
        }
        if ( vkdpNullDataPosition == pdtrCur->m_posEnd )
        {
          Assert( s_kdtPlainText == pdtrCur->type() );
          Assert( vkdpNullDataPosition != pdtrCur->m_posBegin );
          pdtrCur->m_posEnd = pdtrCur->m_posBegin;
          pdtrCur->m_posBegin = posCur;
          posCur = pdtrCur->m_posEnd;
        }
        else
        {
          Assert( s_kdtPlainText != pdtrCur->type() ); // This must be a reference of some sort.
          posCur = pdtrCur->m_posEnd + 1; // We know that all references end with a ";".
        }
      }
      return _pdtrEnd - _pdtrBegin;
    };
    if ( rdt.FContainsSingleDataRange() )
      Verify( 1 == lambdaProcessCharData( &rdt.DataRangeGetSingle(), &rdt.DataRangeGetSingle() + 1 ) );
    else
    {
      size_t nApplied = rdt.GetSegArrayDataRanges().NApplyContiguous( 0, rdt.NPositions(), lambdaProcessCharData );
      if ( nApplied < rdt.NPositions() )
          rdt.GetSegArrayDataRanges().SetSizeSmaller( nApplied );
    }
    
    if ( !m_fFilterWhitespaceCharData )
      return false;
    if ( !rdt.FContainsSingleDataRange() || ( s_kdtPlainText != rdt.DataRangeGetSingle().type() ) )
      return false;
    // The data must still be present in the stream when this method is called, but since the engine is calling it we assume that it is:
    // These are the same as the S production from the XML specification. XML 1.1 might actually allow some other space characters but we aren't going to worry about that now.
    return _rstrm.FSpanChars( rdt.DataRangeGetSingle(), str_array_cast<_TyChar>( STR_XML_WHITESPACE_TOKEN ) );
  }
  template < class t_TyTransportCtxt >
  size_t _NChars( _TyData const & _rd, t_TyTransportCtxt const & _rcxt ) const
  {
    if ( _rd.FContainsSingleDataRange() )
      return _NCharsDr( _rd.DataRangeGetSingle(), _rcxt );
    size_t nChars = 0;
    _rd.GetSegArrayDataRanges().ApplyContiguous( 0, _rd.GetSegArrayDataRanges().NElements(),
      [this,&nChars,&_rcxt]( _l_data_typed_range const * _pdrBegin, _l_data_typed_range const * const _pdrEnd )
      {
        for ( ; _pdrEnd != _pdrBegin; ++_pdrBegin )
          nChars += _NCharsDr( *_pdrBegin, _rcxt );
      }
    );
    return nChars;
  }
  template < class t_TyTransportCtxt >
  size_t _NCharsDr( _l_data_typed_range const & _rdr, t_TyTransportCtxt const & _rcxt ) const
  {
    size_t nLen;
    switch( DtGetType( _rdr.type() ) )
    {
      default:
        Assert( 0 );
        [[fallthrough]];
      case s_kdtPlainText:
        nLen = _rdr.length();
      break;
      case s_kdtAttrWhitespace:
      {
        // A space for each of the characters - but an end of line written as #xD #xA is one ( 2.11 ).
        _TyStrView sv;
        _rcxt.GetStringView( sv, _rdr );
        nLen = sv.length();
        for ( size_t n = 1; n < sv.length(); ++n )
          nLen -= ( ( _TyChar( 0xD ) == sv[n-1] ) && ( _TyChar( 0xA ) == sv[n] ) );
      }
      break;
      case s_kdtEntityRef:
      {
        // The entity map is user-extensible so we must look up the actual replacement length.
        nLen = RStrLookupEntity(_rdr,_rcxt).length();
      }
      break;
      case s_kdtCharDecRef:        
      case s_kdtCharHexRef:
      {
        // A character reference may expand to multiple code units in UTF-8 (up to 4) and UTF-16 (up to 2).
        // Translate exactly as GetString() will so that the count always matches what is copied.
        _TyStrView sv;
        _TyCharRefConvertBuffer crcb;
        _rcxt.GetStringView( sv, _rdr );
        nLen = !_TchTranslateRef( DtGetType( _rdr.type() ), sv, crcb ) ? sv.length() : 1;
      }
      break;
      case s_kdtPEReference:
      {
        nLen = 0;
        if ( s_kfSupportDTD )
          // This isn't entirely true since the parameter entity must be parsed at access time according to the current DTD parse context...
          nLen = _RLookupParameterEntity(_rdr,_rcxt).length();
      }
      break;
    }
    return nLen;
  }
  // Smae character type char-ref:
  static _TyChar _TchTranslateRef( vtyDataType _rdt, _TyStrView & _rsv, _TyCharRefConvertBuffer & )
    requires( TAreSameSizeTypes_v< _TyChar, char32_t > )
  {
    Assert( s_kdtCharDecRef == _rdt || s_kdtCharHexRef == _rdt );
    char32_t tch;
    int iTrans = IReadPositiveNum( s_kdtCharDecRef == _rdt ? 10 : 16, &_rsv[0], _rsv.length(), tch, _l_char_type_map<char32_t>::ms_kcMax, false );
    if ( !!iTrans )
    {
      Assert( -1 == iTrans );
      THROWXMLPARSEEXCEPTIONERRNO( GetLastErrNo(), "Error translating character reference[%s].", StrConvertString<char>( _rsv ).c_str() );
    }
    // Only characters matching the XML Char production may be referenced. This also rejects 0, which callers use to mean "no single character".
    VerifyThrowSz( ( 0x9 == tch ) || ( 0xA == tch ) || ( 0xD == tch ) || ( ( tch >= 0x20 ) && ( tch <= 0xD7FF ) ) || ( ( tch >= 0xE000 ) && ( tch <= 0xFFFD ) ) || ( ( tch >= 0x10000 ) && ( tch <= 0x10FFFF ) ),
      "Character reference[%s] is not a legal XML character.", StrConvertString<char>( _rsv ).c_str() );
    return (_TyChar)tch;
  }
  // Different character type char-ref - the reference is always in UTF32.
  static _TyChar _TchTranslateRef( vtyDataType _rdt, _TyStrView & _rsv, _TyCharRefConvertBuffer & _rgcbBuffer )
    requires( !TAreSameSizeTypes_v< _TyChar, char32_t > )
  {
    Assert( s_kdtCharDecRef == _rdt || s_kdtCharHexRef == _rdt );
    char32_t tch;
    int iTrans = IReadPositiveNum( s_kdtCharDecRef == _rdt ? 10 : 16, &_rsv[0], _rsv.length(), tch, _l_char_type_map<char32_t>::ms_kcMax, false );
    if ( !!iTrans )
    {
      Assert( -1 == iTrans );
      THROWXMLPARSEEXCEPTIONERRNO( GetLastErrNo(), "Error translating character reference[%s].", StrConvertString<char>( _rsv ).c_str() );
    }
    // Only characters matching the XML Char production may be referenced. This also rejects 0, which callers use to mean "no single character".
    VerifyThrowSz( ( 0x9 == tch ) || ( 0xA == tch ) || ( 0xD == tch ) || ( ( tch >= 0x20 ) && ( tch <= 0xD7FF ) ) || ( ( tch >= 0xE000 ) && ( tch <= 0xFFFD ) ) || ( ( tch >= 0x10000 ) && ( tch <= 0x10FFFF ) ),
      "Character reference[%s] is not a legal XML character.", StrConvertString<char>( _rsv ).c_str() );
    // Now we must convert the character into potentially a string:
    _TyStdStr strConverted;
    ConvertString( strConverted, &tch, 1 );
    Assert( strConverted.length() <= sizeof( _rgcbBuffer ) ); // max UTF16 = 2, UTF8 = 4.
    size_t nLen = strConverted.length();
    if ( 1 == nLen )
      return strConverted[0];
    else
    {
      memcpy( _rgcbBuffer, &strConverted[0], nLen * sizeof(_TyChar) );
      _rsv = _TyStrView( _rgcbBuffer, nLen );
    }
    return 0;
  }
  // Return 0 and populate _rsv or return the character to which the markup corresponds.
  // _rgcbBuffer contains a buffer that allows for translation from a UTF32 character reference into the current character type which may be UTF16 or UTF8.
  template < class t_TyTransportCtxt >
  _TyChar _TchGetStringViewDr( _TyStrView & _rsv, _l_data_typed_range const & _rdr, t_TyTransportCtxt const & _rcxt, _TyCharRefConvertBuffer & _rgcbBuffer ) const
  {
    _TyChar tchRtn = 0;
    switch( DtGetType( _rdr.type() ) )
    {
      default:
        Assert( 0 );
      case s_kdtPlainText:
        _rcxt.GetStringView( _rsv , _rdr );
      break;
      case s_kdtCharDecRef:        
      case s_kdtCharHexRef:
      {
        _rcxt.GetStringView( _rsv , _rdr );
        tchRtn = _TchTranslateRef( DtGetType( _rdr.type() ), _rsv, _rgcbBuffer );
      }
      break;
      case s_kdtEntityRef:
      {
        _rsv = RStrLookupEntity(_rdr,_rcxt);
      }
      break;
      case s_kdtPEReference:
      {
        // There are no productions that offer a potential PEReference outside of the DTD so no need for error checking here.
        Assert( s_kfSupportDTD ); // just assert.
        if ( s_kfSupportDTD )
          _rsv = _RLookupParameterEntity(_rdr,_rcxt);
      }
      break;
    }
    return tchRtn;
  }
  // Copy a piece of a value as it is in the value - a reference is what it stands for, the white space of the value of
  //  an attribute is spaces - to _rpcCur and move that along.
  template < class t_TyTransportCtxt >
  void _CopyDr( _TyChar *& _rpcCur, size_t & _rnCharsRemaining, _l_data_typed_range const & _rdtr, t_TyTransportCtxt const & _rcxt ) const
  {
    if ( s_kdtAttrWhitespace == DtGetType( _rdtr.type() ) )
    {
      size_t nSpaces = _NCharsDr( _rdtr, _rcxt );
      Assert( _rnCharsRemaining >= nSpaces ); // should have reserved enough.
      nSpaces = min( _rnCharsRemaining, nSpaces );
      std::fill_n( _rpcCur, nSpaces, _TyChar( ' ' ) );
      _rpcCur += nSpaces;
      _rnCharsRemaining -= nSpaces;
      return;
    }
    _TyStrView sv;
    _TyCharRefConvertBuffer crcb;
    _TyChar tch = _TchGetStringViewDr( sv, _rdtr, _rcxt, crcb );
    if ( !tch )
    {
      Assert( _rnCharsRemaining >= sv.length() );
      size_t nCharsCopy = min( _rnCharsRemaining, sv.length() );
      Assert( nCharsCopy == sv.length() ); // should have reserved enough.
      memcpy( _rpcCur, sv.data(), nCharsCopy * sizeof( _TyChar ) );
      _rpcCur += nCharsCopy;
      _rnCharsRemaining -= nCharsCopy;
    }
    else
    if ( _rnCharsRemaining )
    {
      *_rpcCur++ = tch;
      --_rnCharsRemaining;
    }
  }
// The value of an attribute whose declared type is not CDATA - 3.3.3: see s_kdtfAttrTokenized.
  // Whether the pieces are flagged so. All of the pieces of a value are flagged or none is.
  static bool _FIsTokenized( _TyData const & _rdt )
  {
    if ( _rdt.FContainsSingleDataRange() )
      return !!( s_kdtfAttrTokenized & _rdt.DataRangeGetSingle().type() );
    return !!_rdt.GetSegArrayDataRanges().NElements() && !!( s_kdtfAttrTokenized & _rdt.GetSegArrayDataRanges().ElGet( 0 ).type() );
  }
  // Discard the spaces at the ends and leave one space of each sequence of spaces - in place. Returns the count of characters left.
  static size_t _NNormalizeTokenized( _TyChar * _pc, size_t _nch )
  {
    _TyChar * pcOut = _pc;
    bool fPendingSpace = false;
    for ( const _TyChar * pcCur = _pc, * const pcEnd = _pc + _nch; pcEnd != pcCur; ++pcCur )
    {
      if ( _TyChar( ' ' ) == *pcCur )
        fPendingSpace = ( pcOut != _pc ); // Those at the beginning are discarded.
      else
      {
        if ( fPendingSpace )
        {
          *pcOut++ = _TyChar( ' ' );
          fPendingSpace = false;
        }
        *pcOut++ = *pcCur;
      }
    }
    return size_t( pcOut - _pc ); // Those at the end were still pending.
  }
  // The view of a value which is all in one piece of the document as it is in the document - if there is one:
  //  1) The value is one piece of plain text.
  //  2) The same for the value of a tokenized attribute: then the view is the piece without the spaces at its ends - and
  //     there is none if there is a sequence of spaces within it.
  template < class t_TyStringView, class t_TyTransportCtxt >
  static bool _FGetViewOfData( t_TyStringView & _rsvDest, _TyData const & _rdt, t_TyTransportCtxt const & _rcxt )
  {
    if ( !_rdt.FContainsSingleDataRange() )
      return false;
    const vtyDataType kdt = _rdt.DataRangeGetSingle().type();
    if ( s_kdtPlainText == kdt )
    {
      _rcxt.GetStringView( _rsvDest, _rdt.DataRangeGetSingle() );
      return true;
    }
    if ( ( s_kdtPlainText | s_kdtfAttrTokenized ) != kdt )
      return false;
    typedef typename t_TyStringView::value_type _TyCharView;
    t_TyStringView sv;
    _rcxt.GetStringView( sv, _rdt.DataRangeGetSingle() );
    while ( !sv.empty() && ( _TyCharView( ' ' ) == sv.front() ) )
      sv.remove_prefix( 1 );
    while ( !sv.empty() && ( _TyCharView( ' ' ) == sv.back() ) )
      sv.remove_suffix( 1 );
    for ( size_t n = 1; n < sv.length(); ++n )
    {
      if ( ( _TyCharView( ' ' ) == sv[n-1] ) && ( _TyCharView( ' ' ) == sv[n] ) )
        return false;
    }
    _rsvDest = sv;
    return true;
  }
  template < class t_TyTransportCtxt >
  const _TyStdStr & RStrLookupEntity( _l_data_typed_range const & _rdr, t_TyTransportCtxt const & _rcxt ) const
  {
    _TyStrView sv;
    _rcxt.GetStringView( sv, _rdr );
    typename _TyEntityMap::const_iterator cit = m_mapEntities.find( sv );
    if ( m_mapEntities.end() == cit )
      THROWXMLPARSEEXCEPTION( "Can't find entity [%s].", StrConvertString<char>( sv ).c_str() );
    return cit->second;
  }
  _TyStrView _SvLookupEntity( _TyStrView const & _rsv ) const
  {
    typename _TyEntityMap::const_iterator cit = m_mapEntities.find( _rsv );
    if ( m_mapEntities.end() == cit )
      return _TyStrView();
    return cit->second;
  }
  // Lookup the given entity in the entity map. May require encoding conversion on to and from.
  // Use TGetConversionBuffer_t<> to obtain the appropriate conversion buffer.
  template < class t_TyStrViewOrString >
  _TyStrView SvLookupEntity( t_TyStrViewOrString const & _rsv )
    requires( TAreSameSizeTypes_v< _TyChar, typename t_TyStrViewOrString::value_type > ) // non-converting version.
  {
    return _SvLookupEntity( _rsv );
  }
  template < class t_TyStrViewOrString >
  _TyStrView SvLookupEntity( t_TyStrViewOrString const & _rsv )
    requires( !TAreSameSizeTypes_v< _TyChar, typename t_TyStrViewOrString::value_type > )
  {
    _TyStdStr strConvertBuf;
    return _SvLookupEntity( StrViewConvertString( _rsv, strConvertBuf ) );
  }

  template < class t_TyStringView, class t_TyToken, class t_TyTransportCtxt >
  void GetStringView( t_TyStringView & _rsvDest, t_TyTransportCtxt & _rcxt, t_TyToken & _rtok, typename t_TyToken::_TyValue& _rval ) const
    requires ( sizeof( typename t_TyStringView::value_type ) != sizeof( _TyChar ) )
  {
    typedef typename t_TyToken::_TyValue::template get_string_type< typename t_TyStringView::value_type > _TyStrConvertTo;
    _TyStrConvertTo strConverted;
    GetString( strConverted, _rcxt, _rtok, _rval );
    _TyStrConvertTo & rstr = _rval.emplaceVal( std::move( strConverted ) );
    _rsvDest = t_TyStringView( (const typename t_TyStringView::value_type *)&rstr[0], rstr.length() );
  }
  template < class t_TyStringView, class t_TyString, class t_TyToken, class t_TyTransportCtxt >
  bool FGetStringViewOrString( t_TyStringView & _rsvDest, t_TyString & _rstrDest, t_TyTransportCtxt & _rcxt, t_TyToken & _rtok, typename t_TyToken::_TyValue const & _rval ) const
    requires ( sizeof( typename t_TyStringView::value_type ) != sizeof( _TyChar ) )
  {
    static_assert( sizeof( typename t_TyStringView::value_type ) == sizeof( typename t_TyString::value_type ) );
    Assert( _rsvDest.empty() );
    Assert( _rstrDest.empty() );
    GetString( _rstrDest, _rcxt, _rtok, _rval );
    _rsvDest = t_TyStringView( (const typename t_TyStringView::value_type*)&_rstrDest[0], _rstrDest.length() );;
    return false;
  }
  // A view in another character type without changing the value: there is nowhere to keep the converted string.
  // This is here so that KGetStringView() compiles for every type of view - the value may hold a string of that
  //  character type instead of positions, and then there is a view of it. It throws if it is reached.
  template < class t_TyStringView, class t_TyToken, class t_TyTransportCtxt >
  void KGetStringView( t_TyStringView & /* _rsvDest */, t_TyTransportCtxt & /* _rcxt */, t_TyToken const & /* _rtok */, typename t_TyToken::_TyValue const & /* _rval */ ) const
    requires ( ( sizeof( typename t_TyStringView::value_type ) != sizeof( _TyChar ) ) && !TFIsTransportVarCtxt_v< t_TyTransportCtxt > )
  {
    THROWNAMEDBADVARIANTACCESSEXCEPTION("Can't get a constant string view of a character type not matching the character type of the parser - use GetStringView() or FGetStringViewOrString().");
  }
// var transport:
  template < class t_TyStringView, class t_TyToken, class t_TyTransportCtxt >
  void GetStringView( t_TyStringView & _rsvDest, t_TyTransportCtxt & _rcxt, t_TyToken & _rtok, typename t_TyToken::_TyValue & _rval ) const
    requires ( TFIsTransportVarCtxt_v< t_TyTransportCtxt > )
  {
    return visit(_VisitHelpOverloadFCall {
      [this,&_rsvDest,&_rtok,&_rval]( auto & _rcxtTransport )
      {
        GetStringView( _rsvDest, _rcxtTransport, _rtok, _rval );
      }
    }, _rcxt.GetVariant() );
  }
  template < class t_TyStringView, class t_TyToken, class t_TyTransportCtxt >
  void KGetStringView( t_TyStringView & _rsvDest, t_TyTransportCtxt & _rcxt, t_TyToken const & _rtok, typename t_TyToken::_TyValue const & _rval ) const
    requires ( TFIsTransportVarCtxt_v< t_TyTransportCtxt > )
  {
    return visit(_VisitHelpOverloadFCall {
      [this,&_rsvDest,&_rtok,&_rval]( auto & _rcxtTransport )
      {
        KGetStringView( _rsvDest, _rcxtTransport, _rtok, _rval );
      }
    }, _rcxt.GetVariant() );
  }
  template < class t_TyStringView, class t_TyToken, class t_TyTransportCtxt >
  void KGetStringView( t_TyStringView & _rsvDest, t_TyTransportCtxt & _rcxt, t_TyToken const & _rtok, _l_data_range const & _rdr ) const
    requires ( TFIsTransportVarCtxt_v< t_TyTransportCtxt > )
  {
    return visit(_VisitHelpOverloadFCall {
      [this,&_rsvDest,&_rtok,&_rdr]( auto & _rcxtTransport )
      {
        KGetStringView( _rsvDest, _rcxtTransport, _rtok, _rdr );
      }
    }, _rcxt.GetVariant() );
  }
  template < class t_TyStringView, class t_TyString, class t_TyToken, class t_TyTransportCtxt >
  bool FGetStringViewOrString( t_TyStringView & _rsvDest, t_TyString & _rstrDest, t_TyTransportCtxt & _rcxt, t_TyToken & _rtok, const typename t_TyToken::_TyValue & _rval ) const
    requires ( TFIsTransportVarCtxt_v< t_TyTransportCtxt > )
  {
    return visit(_VisitHelpOverloadFCall {
      [this,&_rsvDest,&_rstrDest,&_rtok,&_rval]( auto & _rcxtTransport )
      {
        return FGetStringViewOrString( _rsvDest, _rstrDest, _rcxtTransport, _rtok, _rval );
      }
    }, _rcxt.GetVariant() );
  }
  template < class t_TyString, class t_TyToken, class t_TyTransportCtxt >
  void GetString( t_TyString & _rstrDest, t_TyTransportCtxt & _rcxt, t_TyToken & _rtok, const typename t_TyToken::_TyValue & _rval )
    requires ( TFIsTransportVarCtxt_v< t_TyTransportCtxt > )
  {
    return visit(_VisitHelpOverloadFCall {
      [this,&_rstrDest,&_rtok,&_rval]( auto & _rcxtTransport )
      {
        GetString( _rstrDest, _rcxtTransport, _rtok, _rval );
      }
    }, _rcxt.GetVariant() );
  }
// Non-converting GetString*.
  template < class t_TyStringView, class t_TyToken, class t_TyTransportCtxt >
  void GetStringView( t_TyStringView & _rsvDest, t_TyTransportCtxt & _rcxt, t_TyToken & _rtok, typename t_TyToken::_TyValue & _rval ) const
    requires ( ( sizeof( typename t_TyStringView::value_type ) == sizeof( _TyChar ) ) && !TFIsTransportVarCtxt_v< t_TyTransportCtxt > )
  {
    Assert( _rsvDest.empty() );
    Assert( _rval.FHasTypedData() ); // We are converting the _TyData object that is in _rval.
    const _TyData kdtr = _rval.template GetVal< _TyData >();
    _rcxt.AssertValidDataRange( kdtr );
    if ( !_FGetViewOfData( _rsvDest, kdtr, _rcxt ) ) // Is translation necessary?
    {
      typedef typename t_TyToken::_TyValue::template get_string_type< _TyChar > _TyStringNorm;
      _TyStringNorm strBacking;
      GetString( strBacking, _rcxt, _rtok, _rval );
      _TyStringNorm & rstr = _rval.emplaceVal( std::move( strBacking ) );
      _rsvDest = t_TyStringView( (const typename t_TyStringView::value_type*)rstr.data(), rstr.length() );
    }
    else
    {
      // We could set the stringview into the object since it doesn't really hurt:
      _rval.SetVal( std::as_const( _rsvDest ) ); // const so that the same-size converting SetVal() overloads are chosen (e.g. char->char8_t).
    }
  }
  template < class t_TyStringView, class t_TyToken, class t_TyTransportCtxt >
  void KGetStringView( t_TyStringView & _rsvDest, t_TyTransportCtxt & _rcxt, t_TyToken const & _rtok, typename t_TyToken::_TyValue const & _rval ) const
    requires ( ( sizeof( typename t_TyStringView::value_type ) == sizeof( _TyChar ) ) && !TFIsTransportVarCtxt_v< t_TyTransportCtxt > )
  {
    Assert( _rsvDest.empty() );
    Assert( _rval.FHasTypedData() ); // We are converting the _TyData object that is in _rval.
    const _TyData kdtr = _rval.template GetVal< _TyData >();
    _rcxt.AssertValidDataRange( kdtr );
    VerifyThrowSz( _FGetViewOfData( _rsvDest, kdtr, _rcxt ), "KGetStringView() is only valid for single data ranges." );
  }
  template < class t_TyStringView, class t_TyToken, class t_TyTransportCtxt >
  void KGetStringView( t_TyStringView & _rsvDest, t_TyTransportCtxt & _rcxt, t_TyToken const & _rtok, _l_data_range const & _rdr ) const
    requires ( ( sizeof( typename t_TyStringView::value_type ) == sizeof( _TyChar ) ) && !TFIsTransportVarCtxt_v< t_TyTransportCtxt > )
  {
    Assert( _rsvDest.empty() );
    _rcxt.AssertValidDataRange( _rdr );
    _rcxt.GetStringView( _rsvDest, _rdr );
  }
  template < class t_TyStringView, class t_TyString, class t_TyToken, class t_TyTransportCtxt >
  bool FGetStringViewOrString( t_TyStringView & _rsvDest, t_TyString & _rstrDest, t_TyTransportCtxt & _rcxt, t_TyToken const & _rtok, const typename t_TyToken::_TyValue & _rval ) const
    requires ( ( sizeof( typename t_TyStringView::value_type ) == sizeof( _TyChar ) ) && !TFIsTransportVarCtxt_v< t_TyTransportCtxt > )
  {
    static_assert( sizeof( typename t_TyStringView::value_type ) == sizeof( typename t_TyString::value_type ) );
    Assert( _rsvDest.empty() );
    Assert( _rstrDest.empty() );
    Assert( _rval.FHasTypedData() ); // We are converting the _TyData object that is in _rval.
    const _TyData kdtr = _rval.template GetVal< _TyData >();
    _rcxt.AssertValidDataRange( kdtr );
    if ( !_FGetViewOfData( _rsvDest, kdtr, _rcxt ) )
    {
      GetString( _rstrDest, _rcxt, _rtok, _rval );
      _rsvDest = t_TyStringView( (const typename t_TyStringView::value_type*)_rstrDest.data(), _rstrDest.length() );
      return false;
    }
    else
      return true;
  }
  template < class t_TyString, class t_TyToken, class t_TyTransportCtxt >
  void GetString( t_TyString & _rstrDest, t_TyTransportCtxt & _rcxt, t_TyToken const & _rtok, const typename t_TyToken::_TyValue & _rval ) const
    requires ( ( sizeof( typename t_TyString::value_type ) == sizeof( _TyChar ) ) && !TFIsTransportVarCtxt_v< t_TyTransportCtxt > )
  {
    Assert( _rstrDest.empty() );
    Assert( _rval.FHasTypedData() ); // We are converting the _TyData object that is in _rval.
    const _TyData kdtr = _rval.template GetVal< _TyData >();
    if ( kdtr.FIsNull() )
      return;
    _rcxt.AssertValidDataRange( kdtr );
    // Then we must back with a string:
    size_t nCharsCount = _NChars( kdtr, _rcxt );
    size_t nCharsRemaining = nCharsCount;
    t_TyString strBacking( nCharsRemaining, 0 ); // Return the type the caller asked for.
    if ( kdtr.FContainsSingleDataRange(s_kdtPlainText) )
    {
      memcpy( &strBacking[0], _rcxt.GetTokenBuffer().begin() + kdtr.DataRangeGetSingle().begin() - _rcxt.PosTokenStart(), nCharsRemaining * sizeof( _TyChar ) );
      nCharsRemaining = 0;
    }
    else
    {
      _TyChar * pcCur = (_TyChar*)&strBacking[0]; // Current output pointer.
      auto lambdaTranslate = 
        [this,&pcCur,&nCharsRemaining,&_rcxt]( const _l_data_typed_range * _pdtrBegin, const _l_data_typed_range * _pdtrEnd )
        {
          const _l_data_typed_range * pdtrCur = _pdtrBegin;
          for ( ; nCharsRemaining && ( _pdtrEnd != pdtrCur ); ++pdtrCur )
            _CopyDr( pcCur, nCharsRemaining, *pdtrCur, _rcxt );
        };
      // A single data range that isn't plain text is a lone reference - it is stored as a single range, not in the segmented array.
      // _l_data::Apply() handles both the single range and the segmented array cases.
      kdtr.Apply( lambdaTranslate );
      Assert( !nCharsRemaining ); // Should have eaten everything.
    }
    nCharsCount -= nCharsRemaining;
    if ( _FIsTokenized( kdtr ) )
      nCharsCount = _NNormalizeTokenized( (_TyChar*)&strBacking[0], nCharsCount );
    strBacking.resize( nCharsCount );
    _rstrDest = std::move( strBacking );
  }

// Converting GetString*.
  template < class t_TyString, class t_TyToken, class t_TyTransportCtxt >
  void GetString( t_TyString & _rstrDest, t_TyTransportCtxt & _rcxt, t_TyToken const & _rtok, typename t_TyToken::_TyValue const & _rval ) const
    requires ( ( sizeof( typename t_TyString::value_type ) != sizeof( _TyChar ) ) && !TFIsTransportVarCtxt_v< t_TyTransportCtxt > )
  {
    Assert( _rstrDest.empty() );
    typedef typename t_TyString::value_type _TyCharConvertTo;
    Assert( _rval.FHasTypedData() ); // We are converting the _TyData object that is in _rval.
    const _TyData kdtr = _rval.template GetVal< _TyData >();
    if ( kdtr.FIsNull() )
      return;
    _rcxt.AssertValidDataRange( kdtr );
    // Then we must back with a converted string, attempt to use an alloca() buffer:
    static size_t knchMaxAllocaSize = vknbyMaxAllocaSize / sizeof( _TyChar );
    typename t_TyToken::_TyValue::template get_string_type< _TyChar > strTempBuf; // For when we have more than knchMaxAllocaSize.
    size_t nCharsCount = _NChars( kdtr, _rcxt );
    size_t nCharsRemaining = nCharsCount;
    _TyChar * pcBuf;
    if ( nCharsCount > knchMaxAllocaSize )
    {
      strTempBuf.resize( nCharsCount );
      pcBuf = &strTempBuf[0];
    }
    else
      pcBuf = (_TyChar*)alloca( nCharsCount * sizeof( _TyChar ) );
    if ( kdtr.FContainsSingleDataRange(s_kdtPlainText) )
    {
      // REVIEW: Could just use a string view here instead and not copy to the stack.
      memcpy( pcBuf, _rcxt.GetTokenBuffer().begin() + kdtr.DataRangeGetSingle().begin() - _rcxt.PosTokenStart(), nCharsRemaining * sizeof( _TyChar ) );
      nCharsRemaining = 0;
    }
    else
    {
      _TyChar * pcCur = pcBuf; // Current output pointer.
      auto lambdaTranslate = 
        [this,&pcCur,&nCharsRemaining,&_rcxt]( const _l_data_typed_range * _pdtrBegin, const _l_data_typed_range * _pdtrEnd )
        {
          const _l_data_typed_range * pdtrCur = _pdtrBegin;
          for ( ; nCharsRemaining && ( _pdtrEnd != pdtrCur ); ++pdtrCur )
            _CopyDr( pcCur, nCharsRemaining, *pdtrCur, _rcxt );
        };
      // A single data range that isn't plain text is a lone reference - it is stored as a single range, not in the segmented array.
      // _l_data::Apply() handles both the single range and the segmented array cases.
      kdtr.Apply( lambdaTranslate );
      Assert( !nCharsRemaining );
    }
    if ( nCharsRemaining )
      nCharsCount -= nCharsRemaining;
    if ( _FIsTokenized( kdtr ) )
      nCharsCount = _NNormalizeTokenized( pcBuf, nCharsCount );
    t_TyString strConverted;
    ConvertString( strConverted, pcBuf, nCharsCount );
    _rstrDest = std::move( strConverted );
  }
};

__XMLP_END_NAMESPACE
