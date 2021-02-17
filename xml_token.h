#pragma once

// xml_token.h
// XML token object.
// dbien
// 07JAN2021
// This implements a wrapper on the _l_token<> class that is returned from the lexical analyzer.

#include "xml_ns.h"
#include "xml_types.h"
#include "_l_token.h"

__XMLP_BEGIN_NAMESPACE

// xml_token:
// This is the token base class - for all types of tokens that may be returned.
// Keep this as lightweight as possible.
// Allow attribute perusal through lambda only - this simplifies the interface quite nicely.
// Member functions access the methods of all types of tokens - throw if erroneous method called - ie. asking for the comment text from a non-comment token.

template < class t_TyXmlTraits >
class xml_token
{
  typedef xml_token _TyThis;
public:
  typedef t_TyXmlTraits _TyXmlTraits;
  typedef typename _TyXmlTraits::_TyChar _TyChar;
  typedef typename _TyXmlTraits::_TyLexTraits _TyLexTraits;
  typedef typename _TyLexTraits::_TyTpValueTraits _TyTpValueTraits;
  typedef typename _TyLexTraits::_TyTransportCtxt _TyTransportCtxt;
  typedef typename _TyLexTraits::_TyUserObj _TyUserObj;
  typedef basic_string_view< _TyChar > _TyStrView;
  typedef pair< _TyStrView, _TyStrView > _TyPrSvTagPrefix; // note that the prefix is second, not first.
  typedef _l_token< _TyTransportCtxt, _TyUserObj, _TyTpValueTraits > _TyLexToken;
  typedef _l_data<> _TyData;
  typedef _l_value< _TyChar, _TyTpValueTraits > _TyLexValue;

  ~xml_token() = default;
  xml_token( _TyLexToken const & _rtok )
    : m_tokToken( _rtok )
  {
  }
  xml_token( _TyLexToken && _rrtok )
    : m_tokToken( std::move( _rrtok ) )
  {
  }
  xml_token() = delete;
  xml_token( xml_token const & ) = default;
  xml_token & operator=( xml_token const & ) = default;
  xml_token( xml_token && ) = default;
  xml_token & operator=( xml_token && ) = default;

  vtyTokenIdent GetTokenId() const
  {
    return m_tokToken.GetTokenId();
  }
  const _TyLexToken & GetLexToken() const
  {
    return m_tokToken;
  }
  _TyLexToken & GetLexToken()
  {
    return m_tokToken;
  }
  // Shortcut.
  _TyLexValue & operator [] ( size_t _nEl )
  {
    return m_tokToken[_nEl];
  }
  const _TyLexValue & operator [] ( size_t _nEl ) const
  {
    return m_tokToken[_nEl];
  }
  bool FIsTag() const
  {
    vtyTokenIdent tid = m_tokToken.GetTokenId();
    return ( s_knTokenSTag == tid ) || ( s_knTokenEmptyElemTag == tid );
  }
  bool FIsComment() const
  {
    return s_knTokenComment == m_tokToken.GetTokenId();
  }
  bool FIsProcessingInstruction() const
  {
    return s_knTokenComment == m_tokToken.GetTokenId();
  }
// Tag methods:
#if 0 // later
  _TyStrView SvGetTag( _TyStrView * _svGetNamespacePrefix ) const
  {
    VerifyThrowSz( FIsTag(), "Not a tag." );

  }
  // This returns the tag and the prefix.
  _TyPrSvTagPrefix PrSvGetTag() const
  {
    _TyPrSvTagPrefix prsv;
    prsv.first = SvGetTag( &prsv.second );
    return prsv;
  }
  // This will return the full qualified tag name with URI prefixed onto the front of the tag.
  _TyPrSvTagPrefix PrSvGetFullyQualifiedTag() const

  // This must be a tag-token.
  // This method doesn't validate the uniqueness of attribute names.
  // I figure that if we aren't going to validate everything then there is no
  //  reason to validate attr name uniqueness either.
  template < class t_FObj >
  void PeruseAttributes( t_FObj && _rrf )
  {
    VerifyThrowSz( FIsTag(), "Not a tag." );
    _TyValue & valAttr = _RGetRgAttrs();
    typename _TyValue::_TySegArrayValues & rsaAttrs = valAttr.GetValueArray();
    rsaAttrs.NApplyContiguous( 0, rsaAttrs.NElements(), std::forward<t_FObj>(_rrf) );
  }
#endif //0

protected:
  _TyLexToken m_tokToken;
};

// As with all tokens: No default constructor, which translates to a variant: No monostate.
template < class t_TyTpTransports >
class xml_token_var
{
  typedef xml_token_var _TyThis;
public:
  typedef t_TyTpTransports _TyTpTransports;
  typedef MultiplexTuplePack_t< TGetXmlTraitsDefault, _TyTpTransports > _TyTpXmlTraits;
  // Define our variant type - there is no monostate for this.
  typedef MultiplexTuplePack_t< xml_token, _TyTpXmlTraits, variant > _TyVariant;

  template < class t_TyXmlToken >
  xml_token_var( t_TyXmlToken && _rrtok )
    : m_varXmlToken( std::move( _rrtok ) )
  {
  }
  ~xml_token_var() = default;
  xml_token_var() = delete; // no monostate and variant types aren't default constructible.
  xml_token_var( xml_token_var const & ) = default;
  xml_token_var & operator =( xml_token_var const & ) = default;
  xml_token_var( xml_token_var && ) = default;
  xml_token_var & operator =( xml_token_var && ) = default;
  void swap( xml_token_var & _r )
  {
    m_varXmlToken.swap( _r.m_varXmlToken );
  }

  vtyTokenIdent GetTokenId() const
  {
    return std::visit( _VisitHelpOverloadFCall {
      [this]( auto _tXmlToken ) -> vtyTokenIdent
      {
        return _tXmlToken.GetTokenId();
      }
    }, m_varXmlToken );
  }

protected:
  _TyVariant m_varXmlToken;
};

__XMLP_END_NAMESPACE
