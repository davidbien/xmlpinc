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
      //( name, namespaceWrapper, nNamespaceDecls )
      rvalTag.SetSize( 2 );
      rvalTag.emplace_back( vtyDataPosition(0) ); // Set number of namespace decls.
      rval[1].SetArray(); // make second element an empty array of attributes.
    }
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
    AssertValid();
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
    Assert( !_psvPrefix == !_psvUri );
    size_t nPosColon;
    {//B - test name validity
      const _TyStateProto * pspNCNameStart = PspGetNCNameStart< _TyChar >();
      const _TyChar * pcTagName = &_rsvTagName[0];
      const _TyChar * pcMatch = _l_match< t_TyChar >::PszMatch( pspNCNameStart, pcTagName, _rsvTagName.length() );
      nPosColon = *pcMatch == _TyChar(':') ? ( pcMatch - pcTagName ) : 0;
      if ( nPosColon )
        pcMatch = _l_match< t_TyChar >::PszMatch( pspNCNameStart, pcMatch + 1, _rsvTagName.length() - nPosColon - 1 );
      VerifyThrowSz( ( pcMatch - pcTagName ) == _rsvTagName.length(), "Invalid characters found in tag name[%s]", StrConvertString< char >( _rsvTagName ).c_str() );
    }//EB
    _TyLexValue & rvalTag = GetValue()[0];
    if ( _rcxtDoc.FHasNamespaceMap() && ( _psvPrefix || nPosColon ) )
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
      // Get the namespace value wrap. If this represents a reference to an existing current (prefix,uri)
      //  pair then we needn't declare the namespace attribute - with no loss of generality.
      _TyXmlNamespaceValueWrap xnvw = _rcxtDoc.GetNamespaceValueWrap( _psvPrefix, _psvUri );
      _TyXmlNamespaceValueWrap xnvwRef;
      _TyXmlNamespaceValueWrap * pxnvwTagRef = &xnvw;
      if ( xnvw.FIsNamespaceDeclaration() )
      {
        // Then we must declare the namespace as an attribute, this will
        //  add one to the number of namespace decls associated with this element.
        xnvwRef = _DeclareNamespace( _rcxtDoc, std::move( xnvw ) );
        pxnvwTagRef = &xnvwRef;
      }
      // Update the tag's reference to the namespace:
      rvalTag[1].emplaceVal( std::move( *pxnvwTagRef ) );
    }
    else
    {
      // The tag is in no namespace:
      rvalTag[1].emplaceVal( false );
    }
    // Now set the tag name according to whether we should include prefixes or not:
    if ( _rcxtDoc.FIncludePrefixesInAttrNames() == !nPosColon )
    {
      if ( nPosColon )
        rvalTag[0].emplaceArgs< _TyStdStr >( &_rsvTagName[0] + nPosColon + 1, _rsvTagName.length() - nPosColon - 1 );
      else
      {
        _TyStdStr & rstrTagName = rvalTag[0].emplaceArgs< _TyStdStr >( *_psvPrefix );
        rstrTagName += _TyChar( ': ');
        rstrTagName += _rsvTagName;
      }
    }
    else
    {
      // tag name is already in correct format:
      rvalTag[0].emplaceArgs< _TyStdStr >( _rsvTagName );
    }
    // I think we are done... whew!
  }
  // Add the namespace to the set of attributes.
  _TyXmlNamespaceValueWrap _DeclareNamespace( _TyXmlDocumentContext & _rcxtDoc, _TyXmlNamespaceValueWrap && _rrxnvw )
  {
    Assert( _rrxnvw.FIsNamespaceDeclaration() );
    _TyLexValue & rrgvalAttrs = GetValue()[1];
    Assert( rrgvalAttrs.FIsArray() );
    _TyLexValue & rvalAttrNew = rrgvalAttrs.emplace_back();
    // (name,namespacewrap,value,fusedoublequote)
    rvalAttrNew.SetSize( 4 );
    {//B - attr name
      _TyStdStr & rstrAttrName = rvalAttrNew[0].emplaceArgs< _TyStdStr >( _TyMarkupTraits::s_kszXmlnsEtc, StaticStringLen( _TyMarkupTraits::s_kszXmlnsEtc ) );
      rstrAttrName += _rrxnvw.RStringPrefix();
    }//EB
    // attr value: URI: We can use a string view on the URI from the URI map.
    rstrAttrValue[2].emplaceArgs< _TyStrView >( _rrxnvw.RStringUri() );
    _TyXmlNamespaceValueWrap & rxnvw = rstrAttrValue[1].emplaceVal( std::move( _rrxnvw ) ); // Now move the wrapper into place so that when the value is destroyed we remove the namespace.
    rstrAttrValue[4].emplaceVal( _rcxtDoc.FAttributeValuesDoubleQuote() );
    return rxnvw.ShedReference();
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
