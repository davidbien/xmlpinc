#pragma once

// _l_user.h
// "User" objects for the lexical analyzer.
// dbien
// 17DEC2020

// These provide a "default translation" that many lexical analyzers can just use as-is.
// Users may want to override the methods in some cases - especially where character translation is required.
// A good example is where a piece of text may represent a hexidecimal number, etc. The translator in the overridden
//  user object would translate and also potentially check for overflow, etc.

#include "xml_ns.h"
#include "xml_types.h"

__XMLP_BEGIN_NAMESPACE

// _xml_user_obj_base_dtd:
// Contains some infrastructure to support the DTD if desired.
template < class t_TyXmlTraits, bool t_kfSupportDTD >
class _xml_user_obj_base_dtd
{
  typedef _xml_user_obj_base_dtd _TyThis;
public:
  static constexpr bool s_kfSupportDTD = true;
  typedef t_TyXmlTraits _TyXmlTraits;
  typedef typename _TyXmlTraits::_TyChar _TyChar;
  typedef typename _TyXmlTraits::_TyStdStr _TyStdStr;
  typedef typename _TyXmlTraits::_TyStrView _TyStrView;
  typedef typename _TyXmlTraits::_TyEntityMap;

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

  template < class t_TyTransportCtxt >
  const _TyStdStr & _RLookupParameterEntity( _l_data_typed_range const & _rdr, t_TyTransportCtxt const & _rcxt )
  {
    _TyStrView sv;
    _rcxt.GetStringView( sv, _rdr );
    _TyEntityMap::const_iterator cit = m_mapParameterEntities.find( sv );
    if ( m_mapParameterEntities.end() == cit )
    {
      _TyStdStr str( sv );
      THROWXMLPARSEEXCEPTION("Can't find parameter entity [%s].", str.c_str() );
    }
    return cit->second;
  }
};
// Non-DTD base.
template < class t_TyXmlTraits >
class _xml_user_obj_base_dtd< t_TyXmlTraits, false >
{
  typedef _xml_user_obj_base_dtd _TyThis;
public:
  typedef t_TyXmlTraits _TyXmlTraits;
  static constexpr bool s_kfSupportDTD = false;
  void swap( _TyThis & _r )
  {
  }
};

template < class t_TyXmlTraits >
class xml_user_obj 
  : public _xml_user_obj_base_dtd< t_TyXmlTraits, t_TyXmlTraits::s_kfSupportDTD >,
    public _xml_user_obj_base_namespace< t_TyXmlTraits, t_TyXmlTraits::t_kfSupportNamespaces >
{
  typedef xml_user_obj _TyThis;
  typedef _xml_user_obj_base_dtd< t_TyXmlTraits, t_TyXmlTraits::s_kfSupportDTD > _TyBaseDtd;
public:
  typedef t_TyXmlTraits _TyXmlTraits;
  using _TyBaseDtd::s_kfSupportDTD;
  typedef typename _TyXmlTraits::_TyChar _TyChar;
  typedef typename _TyXmlTraits::_TyStdStr _TyStdStr;
  typedef typename _TyXmlTraits::_TyStrView _TyStrView;
  typedef typename _TyXmlTraits::_TyEntityMap _TyEntityMap;
  typedef typename _TyXmlTraits::_TyData _TyData;
  typedef typename _TyXmlTraits::_TyValue _TyValue;
  typedef typename _TyXmlTraits::_TyLexTraits _TyLexTraits;
  typedef _l_action_object_value_base< _TyLexTraits, false > _TyAxnObjBase;
  typedef _l_stream< _TyLexTraits > _TyStream;

  // Entity reference lookup:
  // To keep things simple (at least for now) we insert the standard entity references in the constructor.
  _TyEntityMap m_mapEntities{ { str_array_cast<_TyChar>("quot"), str_array_cast<_TyChar>("\"") }, 
                              { str_array_cast<_TyChar>("amp"), str_array_cast<_TyChar>("&") }, 
                              { str_array_cast<_TyChar>("apos"), str_array_cast<_TyChar>("\'") }, 
                              { str_array_cast<_TyChar>("lt"), str_array_cast<_TyChar>("<") }, 
                              { str_array_cast<_TyChar>("gt"), str_array_cast<_TyChar>(">") } };
  
  // Then this is true then when s_knTokenCharData is received it is check for be entirely composed of whitespace
  // and if so it is ignored entirely and not retunred to the xml parser.
  bool m_fFilterWhitespaceCharData{false};

  xml_user_obj() = default;
  xml_user_obj( xml_user_obj const & _r ) = delete;
  xml_user_obj & operator =( xml_user_obj const & ) = delete;
  xml_user_obj( xml_user_obj && _rr ) = default;
  xml_user_obj & operator =( xml_user_obj && ) = default;
  void swap( _TyThis & _r )
  {
    _TyBaseDtd::swap( _r );
    _TyBaseNamespaces::swap( _r );
    m_mapEntities.swap( _r.m_mapEntities );
  }
  void SetFilterWhitespaceCharData( bool _fFilterWhitespaceCharData )
  {
    m_fFilterWhitespaceCharData = _fFilterWhitespaceCharData;
  }
  bool FFilterToken( const _TyAxnObjBase * _paobCurToken, _TyStream const & _rstrm ) const
  {
    if ( !m_fFilterWhitespaceCharData )
      return false;
    // We know the ultimate type of this token so we can cast:
    typedef TyGetTokenCharData< _TyLexTraits, false > _TyTokenCharData;
    const _TyTokenCharData * ptcd = static_cast< _TyTokenCharData * >( _paobCurToken );
    // First of all, if we have more than one data_typed_range_el or the one we have it not Plain text
    //  then we don't filter.
    const _TyData & krdt = ptcd->RDataGet();
    if ( !krdt.FContainsSingleDataRange() || ( s_kdtPlainText != krdt.DataRangeGetSingle().type() ) )
      return false;
    // The data must still be present in the stream when this method is called, but since the engine is calling it we assume that it is:
    // These are the same as the S production from the XML specification. XML 1.1 might actually allow some other space characters but we aren't going to worry about that now.
    return _rstrm.FSpanChars( krdt, str_array_cast<_TyChar>("\n\r\x20\t") );
  }
  template < class t_TyTransportCtxt >
  size_t _NChars( _TyData const & _rd, t_TyTransportCtxt const & _rcxt ) const
  {
    if ( _rd.FContainsSingleDataRange() )
      return _NCharsDr( DataRangeGetSingle() );
    size_t nChars = 0;
    _rd.GetSegArrayDataRanges().ApplyContiguous( 0, _rd.GetSegArrayDataRanges().NElements(),
      [&nChars]( _l_data_typed_range const * _pdrBegin, _l_data_typed_range const * const _pdrEnd )
      {
        for ( ; _pdrEnd != _pdrBegin; ++_pdrBegin )
          nChars += _NCharsDr( *_pdrBegin, _rcxt );
      }
    );
    return nChars;
  }
  template < class t_TyTransportCtxt >
  size_t _NCharsDr( _l_data_typed_range const & _rdr, t_TyTransportCtxt const & _rcxt )
  {
    size_t nLen;
    switch( _rdr.type() )
    {
      default:
        Assert( 0 );
      case s_kdtPlainText:
        nLen = _rdr.length();
      break;
      case s_kdtEntityRef:
      {
        if ( s_kfSupportDTD )
          nLen = _RLookupEntity(_rdr,_rcxt).length();
        else
          nLen = 1; // We don't need to parse here since we can parse later. All standard entity refs are 1 character.
      }
      break;
      case s_kdtCharDecRef:        
      case s_kdtCharHexRef:
        nLen = 1; // Parse later - we know that each results in one character in the result.
      break;
      case s_kdtPEReference:
      {
        if ( s_kfSupportDTD )
          // This isn't entirely true since the parameter entity must be parsed at access time according to the current DTD parse context...
          nLen = _RLookupParameterEntity(_rdr,_rcxt).length();
      }
      break;
    }
    return nLen;
  }
  _TyChar _TchTranslateRef( _TyStrView & _rsv, vtyDataType _rdt )
  {
    Assert( s_kdtCharDecRef == _rdt || s_kdtCharHexRef == _rdt );
    _TyChar tch;
    int iTrans = ( s_kdtCharDecRef == _rdt ) ? 10 : 16, &_rsv[0], _rsv.length(), tch, _l_char_type_map<_TyChar>::ms_kcMax, false );
    if ( !!iTrans )
    {
      Assert( -1 == iTrans );
      String strErrChars = _rsv;
      THROWXMLPARSEEXCEPTIONERRNO( ::GetLastErrNo(), "Error translating character reference[%s].");
    }
    return tch;
  }
  // Return 0 and populate _rsv or return the character to which the markup corresponds.
  template < class t_TyTransportCtxt >
  _TyChar _TchGetStringViewDr( _TyStrView & _rsv, _l_data_typed_range const & _rdr, t_TyTransportCtxt const & _rcxt )
  {
    _TyChar tchRtn = 0;
    switch( _rdr.type() )
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
        tchRtn = _TchTranslateRef( _rdr.type(), _rsv );
      }
      break;
      case s_kdtEntityRef:
      {
        _rsv = _RLookupEntity(_rdr,_rcxt);
      }
      break;
      case s_kdtPEReference:
      {
        // There are no productions that offer a potential PEReference outside of the DTD so no need for error checking here.
        Assert( s_kfSupportDTD ); // just assert.
        if ( s_kfSupportDTD )
          nLen = _RLookupParameterEntity(_rdr,_rcxt).length();
      }
      break;
    }
    return tchRtn;
  }
  template < class t_TyTransportCtxt >
  const _TyStdStr & _RLookupEntity( _l_data_typed_range const & _rdr, t_TyTransportCtxt const & _rcxt )
  {
    _TyStrView sv;
    _rcxt.GetStringView( sv, _rdr );
    _TyEntityMap::const_iterator cit = m_mapEntities.find( sv );
    if ( m_mapEntities.end() == cit )
    {
      _TyStdStr str( sv );
      THROWXMLPARSEEXCEPTION("Can't find entity [%s].", str.c_str() );
    }
    return cit->second;
  }

  template < class t_TyStringView, class t_TyToken, class t_TyTransportCtxt >
  static void GetStringView( t_TyStringView & _rsvDest, t_TyTransportCtxt & _rcxt, t_TyToken & _rtok, _TyValue & _rval )
    requires ( sizeof( typename t_TyStringView::value_type ) != sizeof( _TyChar ) )
  {
    typedef typename _TyValue::template get_string_type< typename t_TyStringView::value_type > _TyStrConvertTo;
    _TyStrConvertTo strConverted;
    GetString( strConverted, _rcxt, _rtok, _rval );
    _rsvDest = _rval.emplaceVal( std::move( strConverted ) );
  }
  template < class t_TyStringView, class t_TyString, class t_TyToken, class t_TyTransportCtxt >
  static bool FGetStringViewOrString( t_TyStringView & _rsvDest, t_TyString & _rstrDest, t_TyTransportCtxt & _rcxt, t_TyToken & _rtok, _TyValue const & _rval )
    requires ( sizeof( typename t_TyStringView::value_type ) != sizeof( _TyChar ) )
  {
    static_assert( sizeof( typename t_TyStringView::value_type ) == sizeof( typename t_TyString::value_type ) );
    Assert( _rsvDest.empty() );
    Assert( _rstrDest.empty() );
    GetString( _rstrDest, _rcxt, _rtok, _rval );
    return false;
  }
// var transport:
  template < class t_TyStringView, class t_TyToken, class t_TyTransportCtxt >
  static void GetStringView( t_TyStringView & _rsvDest, t_TyTransportCtxt & _rcxt, t_TyToken & _rtok, _TyValue & _rval )
    requires ( TFIsTransportVarCtxt_v< t_TyTransportCtxt > )
  {
    return visit(_VisitHelpOverloadFCall {
      [&_rsvDest,&_rtok,&_rval]( auto & _rcxtTransport )
      {
        GetStringView( _rsvDest, _rcxtTransport, _rtok, _rval );
      }
    }, _rcxt.GetVariant() );
  }
  template < class t_TyStringView, class t_TyString, class t_TyToken, class t_TyTransportCtxt >
  static bool FGetStringViewOrString( t_TyStringView & _rsvDest, t_TyString & _rstrDest, t_TyTransportCtxt & _rcxt, t_TyToken & _rtok, const _TyValue & _rval )
    requires ( TFIsTransportVarCtxt_v< t_TyTransportCtxt > )
  {
    return visit(_VisitHelpOverloadFCall {
      [&_rsvDest,&_rstrDest,&_rtok,&_rval]( auto & _rcxtTransport )
      {
        FGetStringViewOrString( _rsvDest, _rstrDest, _rcxtTransport, _rtok, _rval );
      }
    }, _rcxt.GetVariant() );
  }
  template < class t_TyString, class t_TyToken, class t_TyTransportCtxt >
  static void GetString( t_TyString & _rstrDest, t_TyTransportCtxt & _rcxt, t_TyToken & _rtok, const _TyValue & _rval )
    requires ( TFIsTransportVarCtxt_v< t_TyTransportCtxt > )
  {
    return visit(_VisitHelpOverloadFCall {
      [&_rstrDest,&_rtok,&_rval]( auto & _rcxtTransport )
      {
        GetString( _rstrDest, _rcxtTransport, _rtok, _rval );
      }
    }, _rcxt.GetVariant() );
  }
// Non-converting GetString*.
  template < class t_TyStringView, class t_TyToken, class t_TyTransportCtxt >
  static void GetStringView( t_TyStringView & _rsvDest, t_TyTransportCtxt & _rcxt, t_TyToken & _rtok, _TyValue & _rval )
    requires ( ( sizeof( typename t_TyStringView::value_type ) == sizeof( _TyChar ) ) && !TFIsTransportVarCtxt_v< t_TyTransportCtxt > )
  {
    Assert( _rsvDest.empty() );
    Assert( _rval.FHasTypedData() ); // We are converting the _TyData object that is in _rval.
    const _TyData kdtr = _rval.template GetVal< _TyData >();
    _rcxt.AssertValidDataRange( kdtr );
    if ( !kdtr.FContainsSingleDataRange(s_kdtPlainText) ) // Is translation necessary?
    {
      typedef typename _TyValue::template get_string_type< _TyChar > _TyStringNorm;
      _TyStringNorm strBacking;
      GetString( strBacking, _rcxt, _rtok, _rval );
      _TyStringNorm & rstr = _rval.emplaceVal( std::move( strBacking ) );
      _rsvDest = t_TyStringView( (const typename t_TyStringView::value_type*)&rstr[0], rstr.length() );
    }
    else
    {
      // We could set the stringview into the object since it doesn't really hurt:
      _rcxt.GetStringView( _rsvDest, kdtr );
      _rval.SetVal( _rsvDest );
    }
  }
  template < class t_TyStringView, class t_TyToken, class t_TyTransportCtxt >
  static void KGetStringView( t_TyStringView & _rsvDest, t_TyTransportCtxt & _rcxt, t_TyToken & _rtok, _TyValue const & _rval )
    requires ( ( sizeof( typename t_TyStringView::value_type ) == sizeof( _TyChar ) ) && !TFIsTransportVarCtxt_v< t_TyTransportCtxt > )
  {
    Assert( _rsvDest.empty() );
    Assert( _rval.FHasTypedData() ); // We are converting the _TyData object that is in _rval.
    const _TyData kdtr = _rval.template GetVal< _TyData >();
    _rcxt.AssertValidDataRange( kdtr );
    VerifyThrowSz( kdtr.FContainsSingleDataRange(s_kdtPlainText), "KGetStringView() is only valid for single data ranges." );
    _rcxt.GetStringView( _rsvDest, kdtr );
  }
  template < class t_TyStringView, class t_TyString, class t_TyToken, class t_TyTransportCtxt >
  static bool FGetStringViewOrString( t_TyStringView & _rsvDest, t_TyString & _rstrDest, t_TyTransportCtxt & _rcxt, t_TyToken & _rtok, const _TyValue & _rval )
    requires ( ( sizeof( typename t_TyStringView::value_type ) == sizeof( _TyChar ) ) && !TFIsTransportVarCtxt_v< t_TyTransportCtxt > )
  {
    static_assert( sizeof( typename t_TyStringView::value_type ) == sizeof( typename t_TyString::value_type ) );
    Assert( _rsvDest.empty() );
    Assert( _rstrDest.empty() );
    Assert( _rval.FHasTypedData() ); // We are converting the _TyData object that is in _rval.
    const _TyData kdtr = _rval.template GetVal< _TyData >();
    _rcxt.AssertValidDataRange( kdtr );
    if ( !kdtr.FContainsSingleDataRange(s_kdtPlainText) )
    {
      GetString( _rstrDest, _rcxt, _rtok, _rval );
      return false;
    }
    else
    {
      _rcxt.GetStringView( _rsvDest, kdtr );
      return true;
    }
  }
  template < class t_TyString, class t_TyToken, class t_TyTransportCtxt >
  static void GetString( t_TyString & _rstrDest, t_TyTransportCtxt & _rcxt, t_TyToken & _rtok, const _TyValue & _rval )
    requires ( ( sizeof( typename t_TyString::value_type ) == sizeof( _TyChar ) ) && !TFIsTransportVarCtxt_v< t_TyTransportCtxt > )
  {
    Assert( _rstrDest.empty() );
    Assert( _rval.FHasTypedData() ); // We are converting the _TyData object that is in _rval.
    const _TyData kdtr = _rval.template GetVal< _TyData >();
    if ( kdtr.FIsNull() )
      return;
    _rcxt.AssertValidDataRange( kdtr );
    // Then we must back with a string:
    vtyDataPosition nCharsCount = _NChars( kdtr, _rcxt );
    vtyDataPosition nCharsRemaining = nCharsCount;
    t_TyString strBacking( nCharsRemaining, 0 ); // Return the type the caller asked for.
    if ( kdtr.FContainsSingleDataRange(s_kdtPlainText) )
    {
      memcpy( &strBacking[0], _rcxt.GetTokenBuffer().begin() + kdtr.begin() - _rcxt.PosTokenStart(), nCharsRemaining * sizeof( _TyChar ) );
      nCharsRemaining = 0;
    }
    else
    {
      t_TyChar * pcCur = (t_TyChar*)&strBacking[0]; // Current output pointer.
      kdtr.GetSegArrayDataRanges().ApplyContiguous( 0, kdtr.GetSegArrayDataRanges().NElements(), 
        [&pcCur,&nCharsRemaining,&_rcxt]( const _l_data_typed_range * _pdtrBegin, const _l_data_typed_range * _pdtrEnd )
        {
          _TyStrView sv;
          const _l_data_typed_range * pdtrCur = _pdtrBegin;
          for ( ; nCharsRemaining && ( _pdtrEnd != pdtrCur ); ++pdtrCur )
          {
            _TyChar tch = _TchGetStringViewDr( sv, *pdtrCur, _rcxt );
            if ( !tch )
            {
              Assert( nCharsRemaining >= sv.length() );
              vtyDataPosition nCharsCopy = min( nCharsRemaining, sv.length() );
              Assert( nCharsCopy == sv.length() ); // should have reserved enough.
              memcpy( pcCur, &sv[0], nCharsCopy * sizeof( _TyChar ) );
              pcCur += nCharsCopy;
              nCharsRemaining -= nCharsCopy;
            }
            else
            {
              *pcCur++ = tch;
              --nCharsRemaining;
            }
          }
        }
      );
      Assert( !nCharsRemaining ); // Should have eaten everything.
    }
    strBacking.resize( nCharsCount - nCharsRemaining );
    _rstrDest = std::move( strBacking );
  }

// Converting GetString*.
  template < class t_TyString, class t_TyToken, class t_TyTransportCtxt >
  static void GetString( t_TyString & _rstrDest, t_TyTransportCtxt & _rcxt, t_TyToken & _rtok, _TyValue const & _rval )
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
    typename _TyValue::template get_string_type< _TyChar > strTempBuf; // For when we have more than knchMaxAllocaSize.
    vtyDataPosition nCharsCount = _NChars( kdtr, _rcxt );
    vtyDataPosition nCharsRemaining = nCharsCount;
    _TyChar * pcBuf;
    if ( nCharsCount > knchMaxAllocaSize )
    {
      strTempBuf.resize( nCharsCount );
      pcBuf = &strTempBuf[0];
    }
    else
      pcBuf = (_TyChar*)alloca( nCharsCount * sizeof( _TyChar ) );
    if ( kdtr.FContainsSingleDataRange() )
    {
      // REVIEW: Could just use a string view here instead and not copy to the stack.
      memcpy( pcBuf, _rcxt.GetTokenBuffer().begin() + kdtr.DataRangeGetSingle().begin() - _rcxt.PosTokenStart(), nCharsRemaining * sizeof( _TyChar ) );
      nCharsRemaining = 0;
    }
    else
    {
      t_TyChar * pcCur = pcBuf; // Current output pointer.
      kdtr.GetSegArrayDataRanges().ApplyContiguous( 0, kdtr.GetSegArrayDataRanges().NElements(), 
        [&pcCur,&nCharsRemaining,&_rcxt]( const _l_data_typed_range * _pdtrBegin, const _l_data_typed_range * _pdtrEnd )
        {
          _TyStrView sv;
          const _l_data_typed_range * pdtrCur = _pdtrBegin;
          for ( ; nCharsRemaining && ( _pdtrEnd != pdtrCur ); ++pdtrCur )
          {
            _TyChar tch = _TchGetStringViewDr( sv, *pdtrCur, _rcxt );
            if ( !tch )
            {
              Assert( nCharsRemaining >= sv.length() );
              vtyDataPosition nCharsCopy = min( nCharsRemaining, sv.length() );
              Assert( nCharsCopy == sv.length() ); // should have reserved enough.
              memcpy( pcCur, &sv[0], nCharsCopy * sizeof( _TyChar ) );
              pcCur += nCharsCopy;
              nCharsRemaining -= nCharsCopy;
            }
            else
            {
              *pcCur++ = tch;
              --nCharsRemaining;
            }
          }
        }
      );
      Assert( !nCharsRemaining );
    }
    if ( nCharsRemaining )
      nCharsCount -= nCharsRemaining;
    t_TyString strConverted;
    ConvertString( strConverted, pcBuf, nCharsCount );
    _rstrDest = std::move( strConverted );
  }
};

__XMLP_END_NAMESPACE
