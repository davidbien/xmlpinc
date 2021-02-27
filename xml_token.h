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

template < class t_TyTransportCtxt, class t_TyUserObj, class t_TyTpValueTraits >
class xml_token
{
  typedef xml_token _TyThis;
public:
  typedef t_TyTransportCtxt _TyTransportCtxt;
  typedef t_TyUserObj _TyUserObj;
  typedef t_TyTpValueTraits _TyTpValueTraits;
  typedef typename _TyTransportCtxt::_TyChar _TyChar;
  typedef basic_string_view< _TyChar > _TyStrView;
  typedef basic_string< _TyChar > _TyStdStr;
  typedef pair< _TyStrView, _TyStrView > _TyPrSvTagPrefix; // note that the prefix is second, not first.
  typedef _l_token< _TyTransportCtxt, _TyUserObj, _TyTpValueTraits > _TyLexToken;
  typedef _l_data<> _TyData;
  typedef _l_value< _TyChar, _TyTpValueTraits > _TyLexValue;
  typedef _xml_document_context< _TyUserObj > _TyXmlDocumentContext;

  ~xml_token() = default;
  xml_token( _TyLexToken const & _rtok )
    : m_tokToken( _rtok )
  {
  }
  xml_token( _TyLexToken && _rrtok )
    : m_tokToken( std::move( _rrtok ) )
  {
  }
  xml_token( _TyUserObj & _ruoUserObj, , const _TyAxnObjBase * _paobCurToken )
    : m_tokToken( _ruoUserObj, _paobCurToken )
  {
  }
  xml_token() = delete;
  xml_token( xml_token const & ) = default;
  xml_token & operator=( xml_token const & ) = default;
  xml_token( xml_token && ) = default;
  xml_token & operator=( xml_token && ) = default;
  void swap( xml_token & _r )
  {
    m_tokToken.swap( _r.m_tokToken );
  }
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
  _TyLexValue & GetValue()
  {
    return m_tokToken.GetValue();
  }
  const _TyLexValue & GetValue() const
  {
    return m_tokToken.GetValue();
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
  template< class t_TyStrView >
  void KGetStringView( _TyLexValue const & _rval, t_TyStrView & _rsv ) const
  {
    Assert( _rval.FHasTypedData() );
    VerifyThrow( _rval.FHasTypedData() );
    m_tokToken.KGetStringView( _rsv, _rval );
  }
  void _InitTag( _TyXmlDocumentContext & _rcxtDoc )
  {
    VerifyThrow( FIsTag() );
    _TyLexValue & rval = GetValue();
    if ( rval.FIsNull() )
    {
      // Set up tag structure: leave the array of attributes as null for now.
      rval.SetSize( 2 ); // (tag,rgattr)
      _TyValue & rvalTag = rval[0];
      //(name, namespaceWrapper, fUseDoubleQuote )
      rvalTag.SetSize( 2 );
      rvalTag.emplace_back( _rcxtDoc->m_optprFormatContext->first.m_fAttributeValuesDoubleQuote );
    }
#if ASSERTSENABLED
    // Check that the structure is correct for a tag.
    Assert( 2 == _rval.GetSize() );
#endif //ASSERTSENABLED
  }
  // Tag methods:
  // Set the tag name and potentially the namespace.
  // 1) If _pszTagName contains a colon:
  //  a) If we are using namespaces then the _rcxtDoc will contain a namespace map otherwise not.
  //  b) If we are using namespaces then check that the prefix refers to either an active namespace or the namespace passed in _ppuNamespace.
  //     If _xml_output_format<>::m_fIncludePrefixesInAttrNames then we won't excide any prefix and colon off the front of the tag name,
  //     and we'll add the prefix to the front if it isn't present.
  //  c) If we aren't using namespaces then we just leave things like they are. Note that we cannot read names with two colons in them in this impl.
  //  If _ppuNamespace is passed and a prefix is present on _pcTagName then they must match. Use another call to add another namespace separately - the
  //    idea is that _ppuNamespace is the namespace of the tag.
  template < class t_TyChar >
  void SetTagName( _TyXmlDocumentContext & _rcxtDoc, const t_TyChar * _pcTagName, size_t _stLenTag = 0, TGetPrefixUri< t_TyChar > const * _ppuNamespace = nullptr )
  {
    typedef _l_state_proto< t_TyChar > _TyStateProto;
    AssertValid();
    VerifyThrowSz( !_ppuNamespace || _rcxtDoc.FHasNamespaceMap(), "Namespace are not being used but a namespace (prefix,uri) was passed in." );
    _InitTag();
    Assert( _pcTagName );
    if ( !_stLenTag )
      _stLenTag = StrNLen(  _pcTagName );
    VerifyThrow( _stLenTag );
    // Translate everything to our output character type before processing further.
    _TyStdStr strBufName;
    _TyStrView svTagName = StrViewConvertString( _pcTagName, _stLenTag, strBufName );
    _TyStdStr strBufPrefix, strBufUri;
    _TyStrView svPrefix, svUri;
    if ( _ppuNamespace )
    {
      svPrefix = StrViewConvertString( _ppuNamespace->first, strBufPrefix );
      svUri = StrViewConvertString( _ppuNamespace->first, strBufUri );
    }
    _SetTagName( _rcxtDoc, svTagName, _ppuNamespace ? &svPrefix : nullptr, _ppuNamespace ? &svUri : nullptr );
  }
  // _rsvTagName is modifiable if desired.
  void _SetTagName( _TyXmlDocumentContext & _rcxtDoc, _TyStrView & _rsvTagName, _TyStrView * _psvPrefix, _TyStrView * _psvUri )
  {
    typedef typename _TyXmlDocumentContext::_TyUriAndPrefixMap _TyUriAndPrefixMap;
    typedef typename _TyUriAndPrefixMap::value_type _TyStrUriPrefix;
    typedef typename _TyXmlDocumentContext::_TyNamespaceMap _TyNamespaceMap;
    Assert( !_psvPrefix == !_psvUri );
    size_t nPosColon;
    {//B
      const _TyStateProto * pspNCNameStart = PspGetNCNameStart< _TyChar >();
      const _TyChar * pcTagName = &_rsvTagName[0];
      const _TyChar * pcMatch = _l_match< t_TyChar >::PszMatch( pspNCNameStart, pcTagName, _rsvTagName.length() );
      nPosColon = *pcMatch == _TyChar(':') ? ( pcMatch - pcTagName ) : 0;
      if ( nPosColon )
        pcMatch = _l_match< t_TyChar >::PszMatch( pspNCNameStart, pcMatch + 1, _rsvTagName.length() - nPosColon - 1 );
      VerifyThrowSz( ( pcMatch - pcTagName ) == _rsvTagName.length(), "Invalid characters found in tag name[%s]", StrConvertString< char >( _rsvTagName ).c_str() );
    }//EB
    if ( _rcxtDoc.FHasNamespaceMap() && ( _ppuNamespace || nPosColon ) )
    {
      // If a prefix was present in the name then it must match either a currently active prefix or the prefix passed in in _ppuNamespace.
      _TyStrView svPrefix( _pcTagName, nPosColon );
      if ( nPosColon )
      {
        VerifyThrowSz( !_psvPrefix || ( *_psvPrefix == svPrefix ), "Tag prefix(when present) must match passed (prefix,URI)." );
        if ( !_psvPrefix )
        { // Use the same codepath below.
          Assert( !_psvUri );
          _psvPrefix = &svPrefix;
        }
      }
      _TyNamespaceMap::const itNM = _rcxtDoc.MapNamespaces().find( *_psvPrefix );
      // Throw unless we are declaring a new (prefix,uri).
      VerifyThrowSz( ( _rcxtDoc.MapNamespaces().end() != itNM ) || !!_psvUri, "Prefix[%s] not found in namespace map.", StrConvertString< char >( *_psvPrefix ).c_str() );
      typedef xml_namespace_value_wrap< _TyChar > _TyXmlNamespaceValueWrap;
      _TyXmlNamespaceValueWrap xnvw;
      if ( !!_psvUri )
      {
        // We may be declaring a new (prefix,uri) combo or we may be referencing the active URI for the given
        //  prefix. There is no need for reference counting so if we are referencing the existing URI then we just
        //  have to reference the namespace appropriately for the tag but no need to declare it.
        if (  ( _rcxtDoc.MapNamespaces().end() == itNM ) ||
              ( itNM->second.second.front().RStrUri() != *_psvUri ) )
        {
          if ( _rcxtDoc.MapNamespaces().end() == itNM )
          {
            _TyStrUriPrefix const & rstrPrefix = _rcxtDoc.RStrAddPrefix( *_psvPrefix );
            pair< typename _TyNamespaceMap::iterator, bool > pib = _rcxtDoc.MapNamespaces().emplace( std::piecewise_construct, std::forward_as_tuple(rstrPrefix), std::forward_as_tuple() );
            Assert( pib.first );
            pib.first->second.first = &rstrPrefix; // allow single lookup.
          }
          _TyStrUriPrefix const & rstrUri = _rcxtDoc.RStrAddUri( *_psvUri );
          itNM->second.second.push( &rstrUri );
          xnvw.Init( *it, &m_mapNamespaces ); // squirrel this away but make sure to release if we fail along the way.
        }
        else
        {
          // We are the same as the existing namespace decl:
          xnvw.Init( *it, nullptr ); // This doesn't remove the namespace on destruct.
        }
      }
      
      m_pvtUri( &_rvt.second.second.front().RStrUri() ),

      bool fDefaultNS = !_ppuNamespace->first.length();
      VerifyThrowSz( fDefaultNS || _ppuNamespace->second.length(), "A non-default-prefix URI must have non-zero length." );
      _TyNamespaceMap::iterator itNS = _rcxtDoc.MapNamespaces().find( _ppuNamespace->first );
      if ( m_mapNamespaces.end() == itNS )
      {
        // New prefix:
          pair< typename _TyNamespaceMap::iterator, bool > pib = _rcxtDoc.MapNamespaces().emplace( std::piecewise_construct, std::forward_as_tuple(_ppuNamespace->first), std::forward_as_tuple() );
          Assert( pib.second );
          itNS = pib.first;
          it->second.first = &rvtPrefix; // This allows one lookup to process each attribute.
        _AddNamespaceDecl( )

      }
      else
      {

      }
    }

  }

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

// When we actually support DTD and validation (if ever because they aren't that important to me) then we might have to make this more complex.
// This is an adaptor for use with MultiplexTuplePack_t<>.
template < class t_TyTransport >
using TGetXmlTokenFromTransport = xml_token<  typename t_TyTransport::_TyTransportCtxt, 
                                              xml_user_obj< typename t_TyTransport::_TyChar, false >,
                                              tuple< xml_namespace_value_wrap< typename t_TyTransport::_TyChar > > >;

// As with all tokens: No default constructor, which translates to a variant: No monostate.
template < class t_TyTpTransports >
class xml_token_var
{
  typedef xml_token_var _TyThis;
public:
  typedef t_TyTpTransports _TyTpTransports;
  // Define our variant type - there is no monostate for this. We have to filter duplicates because some transports will share the same context.
  typedef unique_variant_t< MultiplexTuplePack_t< TGetXmlTokenFromTransport, _TyTpTransports, variant > > _TyVariant;

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
      []( auto _tXmlToken ) -> vtyTokenIdent
      {
        return _tXmlToken.GetTokenId();
      }
    }, m_varXmlToken );
  }

protected:
  _TyVariant m_varXmlToken;
};

__XMLP_END_NAMESPACE

namespace std
{
__XMLP_USING_NAMESPACE
  // override std::swap so that it is efficient:
  template < class t_TyTransportCtxt, class t_TyUserObj, class t_TyTpValueTraits >
  void swap(xml_token< t_TyTransportCtxt, t_TyUserObj, t_TyTpValueTraits >& _rl, xml_token< t_TyTransportCtxt, t_TyUserObj, t_TyTpValueTraits >& _rr)
  {
    _rl.swap(_rr);
  }
template < class t_TyTpTransports >
  void swap(xml_token_var< t_TyTpTransports >& _rl, xml_token_var< t_TyTpTransports >& _rr)
  {
    _rl.swap(_rr);
  }
}
