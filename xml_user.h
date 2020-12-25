#pragma once

// _l_user.h
// "User" objects for the lexical analyzer.
// dbien
// 17DEC2020

// These provide a "default translation" that many lexical analyzers can just use as-is.
// Users may want to override the methods in some cases - especially where character translation is required.
// A good example is where a piece of text may represent a hexidecimal number, etc. The translator in the overridden
//  user object would translate and also potentially check for overflow, etc.

#include "_l_ns.h"
#include "_l_types.h"

__LEXOBJ_BEGIN_NAMESPACE

template < class t_TyChar >
class _l_default_user_obj
{
  typedef _l_default_user_obj _TyThis;
public:
  typedef t_TyChar _TyChar;
  typedef _l_data< _TyChar > _TyData;
  typedef _l_value< _TyChar > _TyValue;
  typedef _l_transport_fd_ctxt< _TyChar > _TyTransportCtxtFd;
  typedef _l_transport_fixedmem_ctxt< t_TyChar > _TyTransportCtxtFixedMem;

  // These are the default GetString*() impls. They just concatenates segmented strings regardless of the m_nType value.
  // _rval is a constituent value of _rtok.m_value or may be _rtok.m_value itself. We expect _rval's _TyData object to be
  //  occupied and we will convert it to either a string or a string_view depending on various things...
  // 1) If the character type of the returned string matches _TyChar:
    // a) If _rval<_TyData> contains only a single value and it doesn't cross a segmented memory boundary then we can return a
    //    stringview and we will also update the value with a stringview as it is inexpensive.
    // b) If _rval<_TyData> contains only a single value but it cross a segmented memory boundary then we will create a string
    //    of the appropriate length and then stream the segarray data into the string.
    // c) If _rval<_TyData> contains multiple values then we have to create a string of length adding all the sub-lengths together
    //    and then stream each piece.
  // 2) If the character type doesn't match then need to first create the string - hopefully on the stack using alloca() - and then
  //    pass it to the string conversion.
  // In all cases where we produce a new string we store that string in _rval - we must because we are returning a stringview to it.
  // This means that we may store data in a character representation that isn't _TyChar in _rval and that's totally fine (at least for me).
// Generic transport methods:
// For all transport types these converting methods are exactly the same.
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
// fd transport:
// Non-converting GetString* for fd transport.
  template < class t_TyStringView, class t_TyToken, class t_TyTransportCtxt >
  static void GetStringView( t_TyStringView & _rsvDest, t_TyTransportCtxt & _rcxt, t_TyToken & _rtok, _TyValue & _rval )
    requires ( is_same_v< typename t_TyStringView::value_type, _TyChar > && is_base_of_v< _TyTransportCtxtFd, t_TyTransportCtxt > ) // we act specially for fd transport.
  {
    Assert( _rsvDest.empty() );
    Assert( _rval.FHasTypedData() ); // We are converting the _TyData object that is in _rval.
    const _TyData kdtr = _rval.GetVal< _TyData >();
    _rcxt.AssertValidDataRange( kdtr );
    if ( !kdtr.FContainsSingleDataRange() || !_rcxt.GetTokenBuffer().FGetStringView( _rsvDest, kdtr.begin(), kdtr.end() ) )
    {
      typename _TyValue::get_string_type< _TyChar > strBacking;
      GetString( strBacking, _rcxt, _rtok, _rval );
      _rsvDest = _rval.emplaceVal( std::move( strBacking ) );
    }
    else
    {
      // We could set the stringview into the object since it doesn't really hurt:
      _rval.SetVal( _rsvDest );
    }
  }
  template < class t_TyStringView, class t_TyString, class t_TyToken, class t_TyTransportCtxt >
  static bool FGetStringViewOrString( t_TyStringView & _rsvDest, t_TyString & _rstrDest, t_TyTransportCtxt & _rcxt, t_TyToken & _rtok, const _TyValue & _rval )
    requires ( is_same_v< typename t_TyStringView::value_type, _TyChar > && is_base_of_v< _TyTransportCtxtFd, t_TyTransportCtxt > ) // we act specially for fd transport.
  {
    Assert( _rsvDest.empty() );
    Assert( _rstrDest.empty() );
    Assert( _rval.FHasTypedData() ); // We are converting the _TyData object that is in _rval.
    const _TyData kdtr = _rval.GetVal< _TyData >();
    _rcxt.AssertValidDataRange( kdtr );
    if ( !kdtr.FContainsSingleDataRange() || !_rcxt.GetTokenBuffer().FGetStringView( _rsvDest, kdtr.begin(), kdtr.end() ) )
    {
      typename _TyValue::get_string_type< _TyChar > strBacking;
      GetString( _rstrDest, _rcxt, _rtok, _rval );
      return false;
    }
    else
      return true;
  }
  template < class t_TyString, class t_TyToken, class t_TyTransportCtxt >
  static void GetString( t_TyString & _rstrDest, t_TyTransportCtxt & _rcxt, t_TyToken & _rtok, const _TyValue & _rval )
    requires ( is_same_v< typename t_TyString::value_type, _TyChar > && is_base_of_v< _TyTransportCtxtFd, t_TyTransportCtxt > ) // we act specially for fd transport.
  {
    Assert( _rstrDest.empty() );
    Assert( _rval.FHasTypedData() ); // We are converting the _TyData object that is in _rval.
    const _TyData kdtr = _rval.GetVal< _TyData >();
    if ( kdtr.FIsNull() )
      return;
    _rcxt.AssertValidDataRange( kdtr );
    // Then we must back with a string:
    vtyDataPosition nCharsCount = kdtr.CountChars();
    vtyDataPosition nCharsRemaining = nCharsCount;
    t_TyString strBacking( nCharsRemaining, 0 ); // Return the type the caller asked for.
    if ( kdtr.FContainsSingleDataRange() )
    {
      _tySizeType nCharsRead = _rcxt.GetTokenBuffer().Read( kdtr.begin(), &strBacking[0], nCharsRemaining );
      Assert( nCharsRemaining == nCharsRead );
      nCharsRemaining -= nCharsRead;
    }
    else
    {
      t_TyChar * pcCur = &strBacking[0]; // Current output pointer.
      kdtr.GetSegArrayDataRanges().ApplyContiguous( 0, kdtr.GetSegArrayDataRanges().NElements(), 
        [&pcCur,&nCharsRemaining,&_rcxt]( const _l_data_typed_range * _pdtrBegin, const _l_data_typed_range * _pdtrEnd )
        {
          Assert( nCharsRemaining ); // Should have reserved enough.
          if ( nCharsRemaining )
          {
            vtyDataPosition nRead = _rcxt.GetTokenBuffer().ReadSegmented( _pdtrBegin, _pdtrEnd, pcCur, nCharsRemaining );
            pcCur += nRead;
            nCharsRemaining -= nRead;
          }
        }
      );
      Assert( !nCharsRemaining ); // Should have eaten everything.
    }
    strBacking.resize( nCharsCount - nCharsRemaining );
    _rstrDest = std::move( strBacking );
  }

// Converting GetString* for fd transport.
  template < class t_TyString, class t_TyToken, class t_TyTransportCtxt >
  static void GetString( t_TyString & _rstrDest, t_TyTransportCtxt & _rcxt, t_TyToken & _rtok, _TyValue const & _rval )
    requires ( !is_same_v< typename t_TyString::value_type, _TyChar > && is_base_of_v< _TyTransportCtxtFd, t_TyTransportCtxt > ) // we act specially for fd transport.
  {
    Assert( _rstrDest.empty() );
    typedef typename t_TyString::value_type _TyCharConvertTo;
    Assert( _rval.FHasTypedData() ); // We are converting the _TyData object that is in _rval.
    const _TyData kdtr = _rval.template GetVal< _TyData >();
    if ( kdtr.FIsNull() )
      return;
    _rcxt.AssertValidDataRange( kdtr );
    // Then we must back with a converted string, attempt to use an alloca() buffer:
    static const size_t knchMaxAllocaSize = ( 1 << 19 ) / sizeof( _TyChar ); // Allow 512KB on the stack. After that we go to a string.
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
      size_t nCharsRead = _rcxt.GetTokenBuffer().Read( kdtr.begin(), pcBuf, nCharsRemaining );
      Assert( nCharsRead == nCharsRemaining );
      nCharsRemaining -= nCharsRead;
    }
    else
    {
      t_TyChar * pcCur = pcBuf; // Current output pointer.
      kdtr.GetSegArrayDataRanges().ApplyContiguous( 0, kdtr.GetSegArrayDataRanges().NElements(), 
        [&pcCur,&nCharsRemaining,&_rcxt]( const _l_data_typed_range * _pdtrBegin, const _l_data_typed_range * _pdtrEnd )
        {
          vtyDataPosition nRead = _rcxt.GetTokenBuffer().ReadSegmented( _pdtrBegin, _pdtrEnd, pcCur, nCharsRemaining );
          pcCur += nRead;
          nCharsRemaining -= nRead;
        }
      );
      Assert( !nCharsRemaining );
    }
    if ( nCharsRemaining )
    {
      if ( nCharsCount > knchMaxAllocaSize )
      {
        strTempBuf.resize( nCharsCount - nCharsRemaining );
        pcBuf = &strTempBuf[0];
      }
      nCharsCount -= nCharsRemaining;
    }
    t_TyString strConverted;
    ConvertString( strConverted, pcBuf, nCharsCount );
    _rstrDest = std::move( strConverted );
  }

// Fixed-memory transport GetString* methods:
// Non-converting GetString* for fixed-memory transport.
  template < class t_TyStringView, class t_TyToken, class t_TyTransportCtxt >
  static void GetStringView( t_TyStringView & _rsvDest, t_TyTransportCtxt & _rcxt, t_TyToken & _rtok, _TyValue & _rval )
    requires ( is_same_v< typename t_TyStringView::value_type, _TyChar > && is_base_of_v< _TyTransportCtxtFixedMem, t_TyTransportCtxt > ) // all fixed mem context is handled the same - easy.
  {
    Assert( _rstrDest.empty() );
    Assert( _rval.FHasTypedData() ); // We are converting the _TyData object that is in _rval.
    const _TyPrMemView prmvFull = _rcxt.RPrmvFull();
    const _TyData kdtr = _rval.GetVal< _TyData >();
    _rcxt.AssertValidDataRange( kdtr );
    if ( !kdtr.FContainsSingleDataRange() )
    {
      // Then we must back with a string:
      typename _TyValue::get_string_type< _TyChar > strBacking;
      GetString( strBacking, _rcxt, _rtok, _rval );
      _rsvDest = _rval.emplaceVal( std::move( strBacking ) );
    }
    else
    {
      // We could set the stringview into the object since it doesn't really hurt:
      _rsvDest = _rval.emplaceArgs< typename _TyValue::get_string_view_type< _TyChar > >( kdtr.FIsNull() ? 0 : ( prmvFull.first + kdtr.begin() ), kdtr.length() );
    }
  }
  template < class t_TyStringView, class t_TyString, class t_TyToken, class t_TyTransportCtxt >
  static bool FGetStringViewOrString( t_TyStringView & _rsvDest, t_TyString & _rstrDest, t_TyTransportCtxt & _rcxt, t_TyToken & _rtok, _TyValue const & _rval )
    requires ( is_same_v< typename t_TyStringView::value_type, _TyChar > && is_base_of_v< _TyTransportCtxtFixedMem, t_TyTransportCtxt > ) // all fixed mem context is handled the same - easy.
  {
    Assert( _rsvDest.empty() );
    Assert( _rstrDest.empty() );
    Assert( _rval.FHasTypedData() ); // We are converting the _TyData object that is in _rval.
    const _TyPrMemView prmvFull = _rcxt.RPrmvFull();
    const _TyData kdtr = _rval.GetVal< _TyData >();
    if ( kdtr.FIsNull() )
      return true;
    _rcxt.AssertValidDataRange( kdtr );
    if ( !kdtr.FContainsSingleDataRange() )
    {
      // Then we must back with a string:
      t_TyString strBacking;
      GetString( strBacking, _rcxt, _rtok, _rval );
      _rstrDest = std::move( strBacking );
      return false;
    }
    else
    {
      _rsvDest = t_TyStringView( prmvFull.first + kdtr.begin(), kdtr.length() );
      return true;
    }
  }
  template < class t_TyString, class t_TyToken, class t_TyTransportCtxt >
  static void GetString( t_TyString & _rstrDest, t_TyTransportCtxt & _rcxt, t_TyToken & _rtok, _TyValue const & _rval )
    requires ( is_same_v< typename t_TyString::value_type, _TyChar > && is_base_of_v< _TyTransportCtxtFixedMem, t_TyTransportCtxt > ) // all fixed mem context is handled the same - easy.
  {
    Assert( _rstrDest.empty() );
    Assert( _rval.FHasTypedData() ); // We are converting the _TyData object that is in _rval.
    const _TyPrMemView prmvFull = _rcxt.RPrmvFull();
    const _TyData kdtr = _rval.GetVal< _TyData >();
    if ( kdtr.FIsNull() )
      return;
    _rcxt.AssertValidDataRange( kdtr );
    vtyDataPosition nCharsCount = kdtr.CountChars();
    vtyDataPosition nCharsRemaining = nCharsCount;
    t_TyString strBacking( nCharsRemaining, 0 );
    if ( kdtr.FContainsSingleDataRange() )
    {
      memcpy( &strBacking[0], prmvFull.first + kdtr.begin(), kdtr.length() * sizeof( _TyChar ) );
    }
    else
    {
      t_TyChar * pcCur = &strBacking[0]; // Current output pointer.
      kdtr.GetSegArrayDataRanges().ApplyContiguous( 0, kdtr.GetSegArrayDataRanges().NElements(), 
        [&pcCur,&nCharsRemaining,&_rcxt]( const _l_data_typed_range * _pdtrBegin, const _l_data_typed_range * const _pdtrEnd )
        {
          for ( const _l_data_typed_range * pdtrCur = _pdtrBegin; nCharsRemaining && ( _pdtrEnd != pdtrCur ); ++pdtrCur )
          {
            vtyDataPosition nToRead = pdtrCur->length();
            Assert( nCharsRemaining >= nToRead );
            if ( nToRead > nCharsRemaining )
              nToRead = nCharsRemaining;
            Assert( nToRead + pdtrCur->begin() )
            memcpy( pcCur, prmvFull.first + pdtrCur->begin(), nToRead * sizeof( _TyChar ) );
            nCharsRemaining -= nToRead;
            pcCur += nToRead;
          }
        }
      );
    }
    Assert( !nCharsRemaining ); // Should have eaten everything.
    strBacking.resize( nCharsCount - nCharsRemaining );
    _rstrDest = std::move( strBacking );
  }

// Converting GetString* for fixed-memory transport.
  template < class t_TyString, class t_TyToken, class t_TyTransportCtxt >
  static void GetString( t_TyString & _rstrDest, t_TyTransportCtxt & _rcxt, t_TyToken & _rtok, _TyValue const & _rval )
    requires ( !is_same_v< typename t_TyString::value_type, _TyChar > && is_base_of_v< _TyTransportCtxtFixedMem, t_TyTransportCtxt > )
  {
    Assert( _rstrDest.empty() );
    typedef typename t_TyString::value_type _TyCharConvertTo;
    typedef typename _TyTransportCtxtFixedMem::_TyPrMemView _TyPrMemView;
    Assert( _rval.FHasTypedData() ); // We are converting the _TyData object that is in _rval.
    const _TyPrMemView prmvFull = _rcxt.RPrmvFull();
    const _TyData kdtr = _rval.template GetVal< _TyData >();
    if ( kdtr.FIsNull() )
      return; // empty string.
    _rcxt.AssertValidDataRange( kdtr );
    // Then we must back with a converted string, attempt to use an alloca() buffer:
    static const size_t knchMaxAllocaSize = ( 1 << 19 ) / sizeof( _TyChar ); // Allow 512KB on the stack. After that we go to a string.
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
      memcpy( pcBuf, prmvFull.first + kdtr.DataRangeGetSingle().begin(), kdtr.DataRangeGetSingle().length() * sizeof( _TyChar ) );
      nCharsRemaining -= kdtr.DataRangeGetSingle().length();
    }
    else
    {
      t_TyChar * pcCur = pcBuf; // Current output pointer.
      kdtr.GetSegArrayDataRanges().NApplyContiguous( 0, kdtr.GetSegArrayDataRanges().NElements(), 
        [&pcCur,&nCharsRemaining,&_rcxt]( const _l_data_typed_range * _pdtrBegin, const _l_data_typed_range * _pdtrEnd )
        {
          const _TyPrMemView prmvFull = _rcxt.RPrmvFull();
          const _l_data_typed_range * pdtrCur;
          for ( pdtrCur = _pdtrBegin; nCharsRemaining && ( _pdtrEnd != pdtrCur ); ++pdtrCur )
          {
            vtyDataPosition nToRead = pdtrCur->length();
            Assert( nCharsRemaining >= nToRead );
            if ( nToRead > nCharsRemaining )
              nToRead = nCharsRemaining;
            memcpy( pcCur, prmvFull.first + pdtrCur->begin(), nToRead * sizeof( _TyChar ) );
            nCharsRemaining -= nToRead;
            pcCur += nToRead;
          }
          Assert( _pdtrEnd == pdtrCur );
          return pdtrCur - _pdtrBegin;
        }
      );
    }
    Assert( !nCharsRemaining );
    if ( nCharsRemaining )
    {
      if ( nCharsCount > knchMaxAllocaSize )
      {
        strTempBuf.resize( nCharsCount - nCharsRemaining );
        pcBuf = &strTempBuf[0];
      }
      nCharsCount -= nCharsRemaining;
    }
    t_TyString strConverted;
    ConvertString( strConverted, pcBuf, nCharsCount );
    _rstrDest = std::move( strConverted );
  }

};

__LEXOBJ_END_NAMESPACE
