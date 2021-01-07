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

template < class t_TyChar >
class xml_user_obj
{
  typedef xml_user_obj _TyThis;
public:
  typedef t_TyChar _TyChar;
  typedef basic_string< _TyChar > _TyStdStr;
  typedef _l_data< _TyChar > _TyData;
  typedef _l_value< _TyChar > _TyValue;
  typedef _l_transport_backed_ctxt< _TyChar > _TyTransportCtxtBacked;
  typedef _l_transport_fixedmem_ctxt< t_TyChar > _TyTransportCtxtFixedMem;

  // Entity reference lookup:
  // To keep things simple (at least for now) we insert the standard entity references in the constructor.
  typedef StringTransparentHash< _TyChar > _TyStringTransparentHash; // Allow lookup by string_view without creating a string.
  typedef unordered_map< _TyStdStr, _TyStdStr, _TyStringTransparentHash, std::equal_to<void> > _TyEntityMap;
  _TyEntityMap m_mapEntities{ { "quot", "\"" }, { "amp", "&" }, { "apos", "\'" }, { "lt", "<" }, { "gt", ">" } };

  xml_user_obj() = default;
  xml_user_obj( xml_user_obj const & _r ) = delete;
  xml_user_obj & operator =( xml_user_obj const & ) = delete;
  xml_user_obj( xml_user_obj && _rr ) = default;
  xml_user_obj & operator =( xml_user_obj && ) = default;

  size_t _NChars( _TyData const & _rd ) const
  {
    if ( _rd.FContainsSingleDataRange() )
      return _NCharsDr( DataRangeGetSingle() );
    size_t nChars = 0;
    _rd.GetSegArrayDataRanges().ApplyContiguous( 0, _rd.GetSegArrayDataRanges().NElements(),
      [&nChars]( _l_data_typed_range const * _pdrBegin, _l_data_typed_range const * const _pdrEnd )
      {
        for ( ; _pdrEnd != _pdrBegin; ++_pdrBegin )
          nChars += _NCharsDr( *_pdrBegin );
      }
    );
    return nChars;
  }
  size_t _NCharsDr( _l_data_typed_range const & _rdr )
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
        {
          // typename _TyEntityMap::const_iterator cit = m_mapEntities.find() - todo...
        }
        else
          nLen = 1; // We don't need to parse here since we can parse later. All standard entity refs are 1 byte.
      }
      break;
      case s_kdtCharDecRef:        
      case s_kdtCharHexRef:
        nLen = 1; // Parse later - we know that each results in one character in the result.
      break;
      case s_kdtPEReference:
      {
        if ( s_kfSupportDTD )
        {
          //...
        }
      }
      break;
    }
    return nLen;
  }

  template < class t_TyStringView, class t_TyToken, class t_TyTransportCtxt >
  static void GetStringView( t_TyStringView & _rsvDest, t_TyTransportCtxt & _rcxt, t_TyToken & _rtok, _TyValue & _rval )
    requires ( !is_same_v< typename t_TyStringView::value_type, _TyChar > )
  {
    typedef typename t_TyStringView::value_type _TyCharConvertTo;
    typedef typename _TyValue::template get_string_type< _TyCharConvertTo > _TyStrConvertTo;
    _TyStrConvertTo strConverted;
    GetString( strConverted, _rcxt, _rtok, _rval );
    _rsvDest = _rval.emplaceVal( std::move( strConverted ) );
  }
  template < class t_TyStringView, class t_TyString, class t_TyToken, class t_TyTransportCtxt >
  static bool FGetStringViewOrString( t_TyStringView & _rsvDest, t_TyString & _rstrDest, t_TyTransportCtxt & _rcxt, t_TyToken & _rtok, _TyValue const & _rval )
    requires ( !is_same_v< typename t_TyStringView::value_type, _TyChar > )
  {
    Assert( _rsvDest.empty() );
    Assert( _rstrDest.empty() );
    t_TyString strConverted;
    GetString( strConverted, _rcxt, _rtok, _rval );
    _rstrDest = std::move( strConverted );
    return true;
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
    requires ( is_same_v< typename t_TyStringView::value_type, _TyChar > && !TFIsTransportVarCtxt_v< t_TyTransportCtxt > )
  {
    Assert( _rsvDest.empty() );
    Assert( _rval.FHasTypedData() ); // We are converting the _TyData object that is in _rval.
    const _TyData kdtr = _rval.template GetVal< _TyData >();
    _rcxt.AssertValidDataRange( kdtr );
    if ( !kdtr.FContainsSingleDataRange(s_kdtPlainText) ) // Is translation necessary?
    {
      typename _TyValue::template get_string_type< _TyChar > strBacking;
      GetString( strBacking, _rcxt, _rtok, _rval );
      _rsvDest = _rval.emplaceVal( std::move( strBacking ) );
    }
    else
    {
      // We could set the stringview into the object since it doesn't really hurt:
      _rcxt.GetTokenBuffer().GetStringView( _rsvDest, kdtr.begin() - _rcxt.PosTokenStart(), kdtr.end() );
      _rval.SetVal( _rsvDest );
    }
  }
  template < class t_TyStringView, class t_TyString, class t_TyToken, class t_TyTransportCtxt >
  static bool FGetStringViewOrString( t_TyStringView & _rsvDest, t_TyString & _rstrDest, t_TyTransportCtxt & _rcxt, t_TyToken & _rtok, const _TyValue & _rval )
    requires ( is_same_v< typename t_TyStringView::value_type, _TyChar > && !TFIsTransportVarCtxt_v< t_TyTransportCtxt > )
  {
    Assert( _rsvDest.empty() );
    Assert( _rstrDest.empty() );
    Assert( _rval.FHasTypedData() ); // We are converting the _TyData object that is in _rval.
    const _TyData kdtr = _rval.template GetVal< _TyData >();
    _rcxt.AssertValidDataRange( kdtr );
    if ( !kdtr.FContainsSingleDataRange(s_kdtPlainText) )
    {
      typename _TyValue::template get_string_type< _TyChar > strBacking;
      GetString( _rstrDest, _rcxt, _rtok, _rval );
      return false;
    }
    else
    {
      _rcxt.GetTokenBuffer().GetStringView( _rsvDest, kdtr.begin() - _rcxt.PosTokenStart(), kdtr.end() );
      return true;
    }
  }
  template < class t_TyString, class t_TyToken, class t_TyTransportCtxt >
  static void GetString( t_TyString & _rstrDest, t_TyTransportCtxt & _rcxt, t_TyToken & _rtok, const _TyValue & _rval )
    requires ( is_same_v< typename t_TyString::value_type, _TyChar > && !TFIsTransportVarCtxt_v< t_TyTransportCtxt > )
  {
    Assert( _rstrDest.empty() );
    Assert( _rval.FHasTypedData() ); // We are converting the _TyData object that is in _rval.
    const _TyData kdtr = _rval.template GetVal< _TyData >();
    if ( kdtr.FIsNull() )
      return;
    _rcxt.AssertValidDataRange( kdtr );
    // Then we must back with a string:
    vtyDataPosition nCharsCount = kdtr.CountChars();
    vtyDataPosition nCharsRemaining = nCharsCount;
    t_TyString strBacking( nCharsRemaining, 0 ); // Return the type the caller asked for.
    if ( kdtr.FContainsSingleDataRange(s_kdtPlainText) )
    {
      memcpy( &strBacking[0], _rcxt.GetTokenBuffer().begin() + kdtr.begin() - _rcxt.PosTokenStart(), nCharsRemaining * sizeof( _TyChar ) );
      nCharsRemaining = 0;
    }
    else
    {
      t_TyChar * pcCur = &strBacking[0]; // Current output pointer.
      kdtr.GetSegArrayDataRanges().ApplyContiguous( 0, kdtr.GetSegArrayDataRanges().NElements(), 
        [&pcCur,&nCharsRemaining,&_rcxt]( const _l_data_typed_range * _pdtrBegin, const _l_data_typed_range * _pdtrEnd )
        {
          const _l_data_typed_range * pdtrCur = _pdtrBegin;
          for ( ; nCharsRemaining && ( _pdtrEnd != pdtrCur ); ++pdtrCur )
          {
            Assert( nCharsRemaining >= pdtrCur->length() );
            vtyDataPosition nCharsCopy = min( nCharsRemaining, pdtrCur->length() );
            Assert( nCharsCopy == pdtrCur->length() ); // should have reserved enough.
            memcpy( pcCur, _rcxt.GetTokenBuffer().begin() + pdtrCur->begin() - _rcxt.PosTokenStart(), nCharsCopy * sizeof( _TyChar ) );
            pcCur += nCharsCopy;
            nCharsRemaining -= nCharsCopy;
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
    requires ( !is_same_v< typename t_TyString::value_type, _TyChar > && !TFIsTransportVarCtxt_v< t_TyTransportCtxt > )
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
    vtyDataPosition nCharsCount = kdtr.CountChars();
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
      memcpy( pcBuf, _rcxt.GetTokenBuffer().begin() + kdtr.DataRangeGetSingle().begin() - _rcxt.PosTokenStart(), nCharsRemaining * sizeof( _TyChar ) );
      nCharsRemaining = 0;
    }
    else
    {
      t_TyChar * pcCur = pcBuf; // Current output pointer.
      kdtr.GetSegArrayDataRanges().ApplyContiguous( 0, kdtr.GetSegArrayDataRanges().NElements(), 
        [&pcCur,&nCharsRemaining,&_rcxt]( const _l_data_typed_range * _pdtrBegin, const _l_data_typed_range * _pdtrEnd )
        {
          const _l_data_typed_range * pdtrCur = _pdtrBegin;
          for ( ; nCharsRemaining && ( _pdtrEnd != pdtrCur ); ++pdtrCur )
          {
            Assert( nCharsRemaining >= pdtrCur->length() );
            vtyDataPosition nCharsCopy = min( nCharsRemaining, pdtrCur->length() );
            Assert( nCharsCopy == pdtrCur->length() ); // should have reserved enough.
            memcpy( pcCur, _rcxt.GetTokenBuffer().begin() + pdtrCur->begin() - _rcxt.PosTokenStart(), nCharsCopy * sizeof( _TyChar ) );
            pcCur += nCharsCopy;
            nCharsRemaining -= nCharsCopy;
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
