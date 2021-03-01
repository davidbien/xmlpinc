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
  bool FHasNamespace() const
  {
    Assert( FIsTag() );
    VerifyThrowSz( FIsTag(), "FHasNamespace() is only applicable to tags." );
    return !GetValue()[0][1].FIsBool();
  }
  _TyXmlNamespaceValueWrap GetNamespaceReference() const
  {
    Assert( FIsTag() && !GetValue()[0][1].FIsBool() );
    VerifyThrowSz( FIsTag(), "GetNamespaceReference() is only applicable to tags." );
    return GetValue()[0][1].GetVal< _TyXmlNamespaceValueWrap >().ShedReference();
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
  // Set the tag them given a namespace reference that is already sussed - i.e. no lookups are necessary to add the namespace.
  template < class t_TyChar >
  void SetTagName( _TyXmlDocumentContext & _rcxtDoc, _TyXmlNamespaceValueWrap const & _rnsvw, const t_TyChar * _pcTagName, size_t _stLenTag = 0 )
  {
    AssertValid();
    VerifyThrowSz( _rcxtDoc.FHasNamespaceMap(), "Namespace are not being used but a _TyXmlNamespaceValueWrap was passed in." );
    _InitTag();
    Assert( _pcTagName );
    if ( !_stLenTag )
      _stLenTag = StrNLen(  _pcTagName );
    VerifyThrow( _stLenTag );
    // Translate everything to our output character type before processing further.
    _TyStdStr strBufName;
    _TyStrView svTagName = StrViewConvertString( _pcTagName, _stLenTag, strBufName );
    _SetTagName( _rcxtDoc, svTagName, nullptr, nullptr, &_rnsvw );
  }
  // Add the namespace _rpuNamespace to this tag.
  // If the URI is the current URI for the given prefix then we don't add a namespace decl attribute.
  // Regardless we return a reference to the (prefix,uri) namespace.
  // Note that if the prefix for this namespace matches the prefix of the tag (which must already refer to an existing)
  //  namespace, then this namespace will overwrite the tag's namespace (as it appropriate).
  // Note as well that we will not check for duplicate namespaces here but only on tag output since that will be faster overall.
  // So, as such, a user may declare multiple tags with the same prefix and different URIs and the tag may change namespaces multiple
  //  tinmes but the error won't be thrown until we actually write the tag.
  template < class t_TyChar >
  _TyXmlNamespaceValueWrap AddNamespace( _TyXmlDocumentContext & _rcxtDoc, TGetPrefixUri< t_TyChar > const & _rpuNamespace )
  {
    Assert( FIsTag() );
    VerifyThrow( FIsTag() && !GetValue().FIsNull() ); // We should have already called SetTagName().
    // First translate to the character set of the token:
    _TyStdStr strBufPrefix, strBufUri;
    _TyStrView svPrefix = StrViewConvertString( _ppuNamespace->first, strBufPrefix );
    _TyStrView svUri = StrViewConvertString( _ppuNamespace->first, strBufUri );
    AddNamespace( _rcxtDoc, svPrefix, svUri );
  }
  _TyXmlNamespaceValueWrap AddNamespace( _TyXmlDocumentContext & _rcxtDoc, _TyStrView const & _rsvPrefix, _TyStrView const & _rsvUri )
  {
    // Take care of the various potential errors all in one go:
    VerifyThrowSz( _rcxtDoc.FHasNamespaceMap() && ( !_rsvUri.empty() || _rsvPrefix.empty() ), "Either namespaces not enabled, or passing an empty URI for a non-empty prefix (which is not allowed)." );
    _TyXmlNamespaceValueWrap xnvw = GetNamespaceValueWrap( _rcxtDoc, _rsvPrefix, _rsvUri )
    // If this is a namespace declaraion then we need to add it to the set of attributes for this tag:
    if ( xnvw.FIsNamespaceDeclaration() )
    {
      _DeclareNamespace( _rcxtDoc, std::move( xnvw ) );
      Assert( xnvw.FIsNamespaceReference() ); // Pass in a declaration, return a reference.
    }
    // Now check to see if the tag has a namespace and if it matches the new prefix and if so update it.
    if ( GetValue()[0][1].FIsBool() )
    {
      // Then if this is a default namespace then it applies to the tag:
      if ( _rsvPrefix.empty() )
        GetValue()[0][1].emplaceVal( std::move( xnvw ) );
    }
    else
    {
      _TyXmlNamespaceValueWrap & rxnvw = GetValue()[0][1].GetVal< _TyXmlNamespaceValueWrap >();
      if ( rxnvw.RStringPrefix() == _rsvPrefix )
        rxnvw.swap( xnvw ); // fastest.
      Assert( !rxnvw.FIsNamespaceDeclaration() ); // should never be the case.
    }
  }
  // Add an attribute to this tag token.
  // To apply a namespace to an attribute get a _TyXmlNamespaceValueWrap by calling AddNamespace() or get the namespace from
  //  a tag. The default (empty prefix) namespace cannot be applied to an attribute and we will throw if it is attempted.
  template < class t_TyChar >
  void AddAttribute(  _TyXmlDocumentContext & _rcxtDoc, const t_TyChar * _pcAttrName, size_t _stLenAttrName = (numeric_limits< size_t >::max)(),
                      const t_TyChar * _pcAttrValue = nullptr, size_t _stLenAttrValue = (numeric_limits< size_t >::max)(),
                      _TyXmlNamespaceValueWrap * _pxnvw = nullptr )
  {
    VerifyThrow( FIsTag() && !GetValue().FIsNull() && !!_pcAttrName );
    if ( (numeric_limits< size_t >::max)() == _stLenAttrName )
      _stLenAttrName = StrNLen( _pcAttrName );
    else
      Assert( StrNLen( _pcAttrName, _stLenAttrName ) == _stLenAttrName ); // no embedded nulls.
    VerifyThrowSz( _stLenAttrName, "Empty attribute name." );
    if ( _pcAttrValue )
    {
      if ( (numeric_limits< size_t >::max)() == _stLenAttrValue )
        _stLenAttrValue = StrNLen( _pcAttrValue );
      else
        Assert( StrNLen( _pcAttrValue, _stLenAttrValue ) == _stLenAttrValue ); // no embedded nulls.
    }

    // Convert encodings if necessary:
    _TyStdStr strBufName, strBufValue;
    _TyStrView svName = StrViewConvertString( _pcAttrName, _stLenAttrName, strBufName );
    _TyStrView svValue = StrViewConvertString( _pcAttrValue, _stLenAttrValue, strBufValue );
    _AddAttribute( _rcxtDoc, svName, svValue, _pxnvw );
  }
  template < class t_TyStrViewOrString >
  void AddAttribute(  _TyXmlDocumentContext & _rcxtDoc, t_TyStrViewOrString const & _strName,
                      t_TyStrViewOrString const & _strValue, _TyXmlNamespaceValueWrap * _pxnvw = nullptr )
  {
    AddAttribute( _rcxtDoc, &_strName[0], _strName.length(), &_strValue[0], _strValue.length(), _pxnvw );
  }
protected:
  void _AddAttribute( _TyXmlDocumentContext & _rcxtDoc, _TyStrView const & _rsvName, _TyStrView const & _rsvValue, _TyXmlNamespaceValueWrap * _pxnvw )
  {
    
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
  void _SetTagName( _TyXmlDocumentContext & _rcxtDoc, _TyStrView & _rsvTagName, _TyStrView * _psvPrefix, _TyStrView * _psvUri, _TyXmlNamespaceValueWrap const * _pnsvw = nullptr )
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
    if ( _rcxtDoc.FHasNamespaceMap() && ( _psvPrefix || _pnsvw || nPosColon || _rcxtDoc.HasDefaultNamespace() ) )
    {
      // If a prefix was present in the name then it must match either a currently active prefix or the prefix passed in in _ppuNamespace.
      _TyStrView svPrefix( _pcTagName, nPosColon );
      if ( nPosColon )
      {
        VerifyThrowSz(  ( !_psvPrefix && !_pnsvw ) || 
                        ( !!_psvPrefix && ( *_psvPrefix == svPrefix ) ) ||
                        ( !!_pnsvw && ( _pnsvw->RStringPrefix() == svPrefix ) ), "Tag prefix(when present) must match passed (prefix,URI)." );
        if ( !_psvPrefix && !_pnsvw )
        { // Use the same codepath below.
          Assert( !_psvUri );
          _psvPrefix = &svPrefix;
        }
      }

      _TyXmlNamespaceValueWrap * pxnvwTagRef; // always initialized below if used.
      if ( !_pnsvw )
      {
        // Get the namespace value wrap. If this represents a reference to an existing current (prefix,uri)
        //  pair then we needn't declare the namespace attribute - with no loss of generality.
        _TyXmlNamespaceValueWrap xnvw = _rcxtDoc.GetNamespaceValueWrap( _psvPrefix, _psvUri );
        if ( xnvw.FIsNamespaceDeclaration() )
        {
          // Then we must declare the namespace as an attribute, this will
          //  add one to the number of namespace decls associated with this element.
          _DeclareNamespace( _rcxtDoc, std::move( xnvw ) );
          Assert( xnvw.FIsNamespaceReference() ); // Pass in a declaration, return a reference.
        }
        rvalTag[1].emplaceVal( std::move( xnvw ) );
      }
      else
      {
        if ( !_pnsvw->FIsNamespaceDeclaration() ) // We know that a namespace declaration is an active namespace.
        { // check to see that the URI is the active namespace for the given prefix.
          VerifyThrowSz( _rcxtDoc.FIsActiveNamespace( *_pnsvw ), "Trying to use an inactive namespace as the current namespace." );
          // Note that we could allow this to switch the namespace as well - it's a matter of design.
        }
        // Update the tag's reference to the namespace:
        rvalTag[1].emplaceVal( _pnsvw->ShedReference() );
      }
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
  void _DeclareNamespace( _TyXmlDocumentContext & _rcxtDoc, _TyXmlNamespaceValueWrap && _rrxnvw )
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
    Assert( _rrxnvw.FIsNull() );
    _rrxnvw = rxnvw.ShedReference(); // return a reference.
    rstrAttrValue[4].emplaceVal( _rcxtDoc.FAttributeValuesDoubleQuote() );
  }

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
