#pragma once

//          Copyright David Lawrence Bien 1997 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt).

// xml_token.h
// XML token object.
// dbien
// 07JAN2021
// This implements a wrapper on the _l_token<> class that is returned from the lexical analyzer.

#include "xml_ns.h"
#include "xml_types.h"
#include "_l_token.h"
#include "_l_match.h"

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
  typedef _l_action_object_base< _TyChar, false > _TyAxnObjBase;
  typedef _xml_document_context< _TyUserObj > _TyXmlDocumentContext;
  typedef xml_namespace_value_wrap< _TyChar > _TyXmlNamespaceValueWrap;
  typedef _l_state_proto< _TyChar > _TyStateProto;
  typedef xml_markup_traits< _TyChar > _TyMarkupTraits;

  ~xml_token() = default;
  xml_token() = delete;
  // Because we might have namespace *declarations* we cannot allow default copying of tokens.
  // We could create a namespace reference when copying a namespace declarations. Have to flesh this out a bit more.
  xml_token( xml_token const & ) = delete;
  xml_token & operator=( xml_token const & ) = delete;
  xml_token( xml_token && ) = default;
  xml_token & operator=( xml_token && ) = default;
  xml_token( _TyLexToken const & _rtok )
    : m_tokToken( _rtok )
  {
  }
  xml_token( _TyLexToken && _rrtok )
    : m_tokToken( std::move( _rrtok ) )
  {
  }
  xml_token( _TyUserObj & _ruoUserObj, const _TyAxnObjBase * _paobCurToken )
    : m_tokToken( _ruoUserObj, _paobCurToken )
  {
  }
  xml_token( _TyUserObj & _ruoUserObj, vtyTokenIdent _tidAccept )
    : m_tokToken( _ruoUserObj, _tidAccept )
  {
  }
  // Copy the passed token - we use the lex token since an xml token is only a wrapper.
  template < class t_TyContainerNew, class t_TyLexToken >
  xml_token( t_TyContainerNew & _rNewContainer, t_TyLexToken const & _rtokCopy, typename t_TyContainerNew::_TyTokenCopyContext * _ptccCopyCtxt = nullptr )
    : m_tokToken( _rNewContainer, _rtokCopy, _ptccCopyCtxt )
  {
  }
  // Move in the passed token - we'll move what we can and copy the rest.
  // We can move in the _l_value<> object and then we will have to modify positions on encoding change.
  template < class t_TyContainerNew, class t_TyLexToken >
  xml_token( t_TyContainerNew & _rNewContainer, t_TyLexToken && _rrtokCopy, typename t_TyContainerNew::_TyTokenCopyContext * _ptccCopyCtxt = nullptr )
    : m_tokToken( _rNewContainer, std::move( _rrtokCopy ), _ptccCopyCtxt )
  {
  }
  void swap( xml_token & _r )
  {
    m_tokToken.swap( _r.m_tokToken );
  }
  void AssertValid( bool _fUseNamespaces = false ) const
  {
#if ASSERTSENABLED
    switch( m_tokToken.GetTokenId() )
    {
      case s_knTokenETag:
        Assert( 0 ); // Don't expect to see a token for an end tag.
      break;
      case s_knTokenSTag:
      case s_knTokenEmptyElemTag:
        _AssertValidTag( _fUseNamespaces );
      break;
      case s_knTokenComment:
        _AssertValidComment();
      break;
      case s_knTokenXMLDecl:
        _AssertValidXMLDecl();
      break;
      case s_knTokenCDataSection:
        _AssertValidCDataSection();
      break;
      case s_knTokenCharData:
        _AssertValidCharData();
      break;
      case s_knTokenProcessingInstruction:
        _AssertValidProcessingInstruction();
      break;
    }
#endif //ASSERTSENABLED
  }
  bool FNullValue() const
  {
    return m_tokToken.GetValue().FIsNull();
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
    return !GetValue()[vknTagNameIdx][vknNamespaceIdx].FIsBool();
  }
  _TyXmlNamespaceValueWrap GetNamespaceReference( ENamespaceReferenceType _enrtReferenceType ) const
  {
    Assert( FIsTag() && !GetValue()[vknTagNameIdx][vknNamespaceIdx].FIsBool() );
    VerifyThrowSz( FIsTag(), "GetNamespaceReference() is only applicable to tags." );
    return GetValue()[vknTagNameIdx][vknNamespaceIdx].template GetVal< _TyXmlNamespaceValueWrap >().ShedReference( _enrtReferenceType );
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
    TGetConversionBuffer_t< t_TyChar, _TyChar > cbufName;
    _TyStrView svTagName = StrViewConvertString( _pcTagName, _stLenTag, cbufName );
    TGetConversionBuffer_t< t_TyChar, _TyChar > cbufPrefix, cbufUri;
    _TyStrView svPrefix, svUri;
    if ( _ppuNamespace )
    {
      svPrefix = StrViewConvertString( _ppuNamespace->first, cbufPrefix );
      svUri = StrViewConvertString( _ppuNamespace->first, cbufUri );
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
    TGetConversionBuffer_t< t_TyChar, _TyChar > cbufName;
    _TyStrView svTagName = StrViewConvertString( _pcTagName, _stLenTag, cbufName );
    _SetTagName( _rcxtDoc, svTagName, nullptr, nullptr, &_rnsvw );
  }
  // Add the namespace to the set of attributes.
  void DeclareNamespace( _TyXmlDocumentContext & _rcxtDoc, _TyXmlNamespaceValueWrap && _rrxnvw )
  {
    Assert( _rrxnvw.FIsAttributeNamespaceDeclaration() && _rrxnvw.FIsNamespaceReference() ); // yep.
    _TyLexValue & rvalAttrNew = _DeclareNewAttr( _rcxtDoc );
    {//B - attr name
      _TyStdStr & rstrAttrName = rvalAttrNew[vknNameIdx].template emplaceArgs< _TyStdStr >( _TyMarkupTraits::s_kszXmlnsEtc, StaticStringLen( _TyMarkupTraits::s_kszXmlnsEtc ) );
      rstrAttrName += _rrxnvw.RStringPrefix();
    }//EB
    // attr value: URI: We can use a string view on the URI from the URI map.
    rvalAttrNew[vknAttr_ValueIdx].template emplaceArgs< _TyStrView >( _rrxnvw.RStringUri() );
    _TyXmlNamespaceValueWrap & rxnvw = rvalAttrNew[vknNamespaceIdx].emplaceVal( std::move( _rrxnvw ) ); // Now move the wrapper into place so that when the value is destroyed we remove the namespace.
    Assert( _rrxnvw.FIsNull() );
    ++GetValue()[vknTagNameIdx][vknTagName_NNamespaceDeclsIdx].template GetVal<vtySignedLvalueInt>();
  }

#if 0 // These are incomplete but will be completed
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
    TGetConversionBuffer_t< t_TyChar, _TyChar > cbufPrefix, cbufUri;
    _TyStrView svPrefix = StrViewConvertString( _rpuNamespace.first, cbufPrefix );
    _TyStrView svUri = StrViewConvertString( _rpuNamespace.first, cbufUri );
    AddNamespace( _rcxtDoc, svPrefix, svUri );
  }
  _TyXmlNamespaceValueWrap AddNamespace( _TyXmlDocumentContext & _rcxtDoc, _TyStrView const & _rsvPrefix, _TyStrView const & _rsvUri )
  {
    // Take care of the various potential errors all in one go:
    VerifyThrowSz( _rcxtDoc.FHasNamespaceMap() && ( !_rsvUri.empty() || _rsvPrefix.empty() ), "Either namespaces not enabled, or passing an empty URI for a non-empty prefix (which is not allowed)." );
    _TyXmlNamespaceValueWrap xnvw = _rcxtDoc.GetNamespaceValueWrap( _rsvPrefix, _rsvUri );
    // If this is a namespace declaraion then we need to add it to the set of attributes for this tag:
    if ( xnvw.FIsNamespaceDeclaration() )
    {
      _DeclareNamespace( _rcxtDoc, std::move( xnvw ), enrtTagNameReference ); // If xnvw ends up modifying anything it will be a tag.
      Assert( xnvw.FIsNamespaceReference() ); // Pass in a declaration, return a reference.
    }
    // Now check to see if the tag has a namespace and if it matches the new prefix and if so update it.
    if ( GetValue()[vknTagNameIdx][vknNamespaceIdx].FIsBool() )
    {
      // Then if this is a default namespace then it applies to the tag:
      if ( _rsvPrefix.empty() )
        GetValue()[vknTagNameIdx][vknNamespaceIdx].emplaceVal( std::move( xnvw ) );
    }
    else
    {
      _TyXmlNamespaceValueWrap & rxnvw = GetValue()[vknTagNameIdx][vknNamespaceIdx].GetVal< _TyXmlNamespaceValueWrap >();
      if ( rxnvw.RStringPrefix() == _rsvPrefix )
        rxnvw.swap( xnvw ); // fastest.
      Assert( !rxnvw.FIsNamespaceDeclaration() ); // should never be the case.
    }
  }
#endif //incomplete
  // Add an attribute to this tag token.
  // To apply a namespace to an attribute get a _TyXmlNamespaceValueWrap by calling AddNamespace() or get the namespace from
  //  a tag. The default (empty prefix) namespace cannot be applied to an attribute and we will throw if it is attempted.
  // The attribute name is validated within this method. The attribute value is not validated until output at which point
  //  it may be segmented into a string of references interleaved with Attribute CharData.
  // So technically we needn't convert the value to the xml_token's character type here but it is cleaner to do so.
  template < class t_TyChar >
  void AddAttribute(  _TyXmlDocumentContext & _rcxtDoc, const t_TyChar * _pcAttrName, size_t _stLenAttrName = (numeric_limits< size_t >::max)(),
                      const t_TyChar * _pcAttrValue = nullptr, size_t _stLenAttrValue = (numeric_limits< size_t >::max)(),
                      const _TyXmlNamespaceValueWrap * _pxnvw = nullptr )
  {
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
    TGetConversionBuffer_t< t_TyChar, _TyChar > cbufName, cbufValue;
    _TyStrView svName = StrViewConvertString( _pcAttrName, _stLenAttrName, cbufName );
    _TyStrView svValue = StrViewConvertString( _pcAttrValue, _stLenAttrValue, cbufValue );
    _AddAttribute( _rcxtDoc, svName, svValue, _pxnvw );
  }
  template < class t_TyStrViewOrString >
  void AddAttribute(  _TyXmlDocumentContext & _rcxtDoc, t_TyStrViewOrString const & _strName,
                      t_TyStrViewOrString const & _strValue, const _TyXmlNamespaceValueWrap * _pxnvw = nullptr )
  {
    AddAttribute( _rcxtDoc, &_strName[0], _strName.length(), &_strValue[0], _strValue.length(), _pxnvw );
  }
  // We can only support printf formatting for the stdc supported character types which are char and wchar_t at this point.
  // Methods are not available for characters not of these sizes.
  template < class t_TyChar >
  void FormatAttribute( _TyXmlDocumentContext & _rcxtDoc, const t_TyChar * _pcAttrName, size_t _stLenAttrName,
                        const t_TyChar * _pcAttrValue, size_t _stLenAttrValue,
                        const _TyXmlNamespaceValueWrap * _pxnvw, ... )
    requires( TAreSameSizeTypes_v< t_TyChar, char > || TAreSameSizeTypes_v< t_TyChar, wchar_t > )
  {
    va_list ap;
    va_start( ap, _pxnvw );
    FormatAttributeVArg( _rcxtDoc, _pcAttrName, _stLenAttrName, _pcAttrValue, _pxnvw, ap );
    va_end( ap );
  }
  template < class t_TyChar >
  void FormatAttributeVArg( _TyXmlDocumentContext & _rcxtDoc, const t_TyChar * _pcAttrName, size_t _stLenAttrName,
                            const t_TyChar * _pcAttrValue, size_t _stLenAttrValue,
                            const _TyXmlNamespaceValueWrap * _pxnvw, va_list _ap )
    requires( TAreSameSizeTypes_v< t_TyChar, char > || TAreSameSizeTypes_v< t_TyChar, wchar_t > )
  {
    typedef conditional_t< TAreSameSizeTypes_v< t_TyChar, char >, char, wchar_t > _TyChar;
    typedef basic_string< _TyChar > _TyString;
    _TyString strValue;
    VPrintfStdStr( strValue, _stLenAttrValue, _pcAttrValue, _ap );
    AddAttribute( _rcxtDoc, _pcAttrName, _stLenAttrName, &strValue[0], strValue.length(), _pxnvw );
  }
  // This method for internal use.
  void _FixupNamespaceDeclarations( _TyXmlDocumentContext & _rcxtDoc, typename _TyXmlDocumentContext::_TyTokenCopyContext & _rctxtTokenCopy )
  {
    Assert( FIsTag() );
    if ( !_rctxtTokenCopy.m_rgDeclarations.size() )
      return; // nada para hacer.
    // The number of declarations is always equal to the count of declarations in the array:
    vtySignedLvalueInt & rnTagNamespaceDecls = ( GetValue()[vknTagNameIdx][vknTagName_NNamespaceDeclsIdx].template GetVal<vtySignedLvalueInt>() = _rctxtTokenCopy.m_rgDeclarations.size() );
    // Declare a lambda to sort the value pointers by the prefix contained in the namespace declaration/reference:
    auto lambdaCompareNamespacePrefix = []( const _TyLexValue * _plvalNameLeft, const _TyLexValue * _plvalNameRight ) -> bool
    {
      const _TyLexValue & rlvalNSLeft = (*_plvalNameLeft)[vknNamespaceIdx];
      const _TyLexValue & rlvalNSRight = (*_plvalNameRight)[vknNamespaceIdx];
      // We should only see namespace value wraps in the namespace position:
      Assert( rlvalNSLeft.template FIsA< _TyXmlNamespaceValueWrap >() && rlvalNSRight.template FIsA< _TyXmlNamespaceValueWrap >() );
      return rlvalNSLeft.template GetVal< _TyXmlNamespaceValueWrap >().RStringPrefix() < rlvalNSRight.template GetVal< _TyXmlNamespaceValueWrap >().RStringPrefix();
    };
    std::sort( _rctxtTokenCopy.m_rgDeclarations.begin(), _rctxtTokenCopy.m_rgDeclarations.end(), lambdaCompareNamespacePrefix );
    std::sort( _rctxtTokenCopy.m_rgReferences.begin(), _rctxtTokenCopy.m_rgReferences.end(), lambdaCompareNamespacePrefix );
    // Now move through finding the any declaration that matches a set of references.
    _TyLexValue * const * pplvalCurDeclaration = &_rctxtTokenCopy.m_rgDeclarations[0];
    _TyLexValue * const * const pplvalEndDeclarations = pplvalCurDeclaration + _rctxtTokenCopy.m_rgDeclarations.size();
    _TyLexValue * const * pplvalCurReference = _rctxtTokenCopy.m_rgReferences.size() ? &_rctxtTokenCopy.m_rgReferences[0] : nullptr;
    _TyLexValue * const * const pplvalEndReferences = !pplvalCurReference ? nullptr : ( pplvalCurReference + _rctxtTokenCopy.m_rgReferences.size() );
    // We must traverse all the declarations but we can ignore references once we are finished with the declarations.
    for ( ; ( pplvalCurDeclaration != pplvalEndDeclarations ); )
    {
      if ( ( pplvalCurReference == pplvalEndReferences ) || lambdaCompareNamespacePrefix( *pplvalCurDeclaration, *pplvalCurReference ) )
      {
        // A declaration that corresponds to no reference. If this isn't a namespace declaration attribute then we need
        //  to add such an attribute.
        bool fIsAttrNamespaceDecl;
        bool fIsAttr;
        if ( !( fIsAttr = _FIsAttribute( **pplvalCurDeclaration, &fIsAttrNamespaceDecl ) ) || !fIsAttrNamespaceDecl )
        {
          // Then a new namespace (prefix,URI) that hasn't been declared yet. Declare it. This has the effect of leaving a
          //  namespace reference in its place which happens to be exactly what we want. This adds one to rnTagNamespaceDecls internally, so we subtract one:
          --rnTagNamespaceDecls;
          _DeclareNamespace( _rcxtDoc, std::move( (**pplvalCurDeclaration)[vknNamespaceIdx].template GetVal< _TyXmlNamespaceValueWrap >() ), fIsAttr ? enrtAttrNameReference : enrtTagNameReference );
        }
        ++pplvalCurDeclaration;
      }
      else
      if ( lambdaCompareNamespacePrefix( *pplvalCurReference, *pplvalCurDeclaration ) )
      {
        // Then a reference without a corresponding declaration. This is normal. This may be a namespace declaration in which case it will be ignored upon output.
        // There's no good reason to delete this declaration. Even if it is written to the file it is merely redundant, but I don't want to write it to the file.
        ++pplvalCurReference;
      }
      else
      {
        // Then a declaration corresponds to some set of references.
        // Move through all matching references and see if one of them is the actual declaration, unless the declaration is the declaration:
        bool fIsAttrNamespaceDecl;
        bool fIsAttr;
        if ( ( fIsAttr = _FIsAttribute( **pplvalCurDeclaration, &fIsAttrNamespaceDecl ) ) && fIsAttrNamespaceDecl )
        {
          // A declaration on the attr declaration, nothing to fixup - must skip all matching references.
          for ( ++pplvalCurReference; ( pplvalCurReference != pplvalEndReferences ) && !lambdaCompareNamespacePrefix( *pplvalCurDeclaration, *pplvalCurReference ); ++pplvalCurReference )
            ;
        }
        else
        {
          bool fFoundDeclaration = false; // must skip all matching references even after finding the declaration.
          do
          {
            if ( !fFoundDeclaration && _FIsAttribute( **pplvalCurReference, &fIsAttrNamespaceDecl ) && fIsAttrNamespaceDecl )
            {
              // We found the actual declaration in the reference - just swap the two:
              _TyXmlNamespaceValueWrap & rxnvwFixup = (**pplvalCurDeclaration)[vknNamespaceIdx].template GetVal< _TyXmlNamespaceValueWrap >();
              rxnvwFixup.swap( (**pplvalCurReference)[vknNamespaceIdx].template GetVal< _TyXmlNamespaceValueWrap >() );
              rxnvwFixup.SetReferenceType( fIsAttr ? enrtAttrNameReference : enrtTagNameReference );
            }
          }
          while( ( ++pplvalCurReference != pplvalEndReferences ) && !lambdaCompareNamespacePrefix( *pplvalCurDeclaration, *pplvalCurReference ) );
          ++pplvalCurDeclaration;
        }
      }
    }
    // I think that's it, but I could be wrong... lol.
  }
  // Return if the array of _l_values indicates an attribute and if _pfIsAttrNamespaceDecl then
  //  find out if it is an attribute namespace declaration.
  bool _FIsAttribute( _TyLexValue const & _rrgval, bool * _pfIsAttrNamespaceDecl = nullptr ) const
  {
    if ( _rrgval.GetSize() == 4 ) // This is currently the way of doing it - could change - this is easy.
    {
      if ( !!_pfIsAttrNamespaceDecl )
      {
        _TyStrView svName;
        _rrgval[vknNameIdx].KGetStringView( m_tokToken, svName );
        *_pfIsAttrNamespaceDecl = svName.starts_with( str_array_cast< _TyChar >("xmlns") );
      }
      return true;
    }
    return false;
  }
protected:
  void _InitTag( _TyXmlDocumentContext & _rcxtDoc )
  {
    VerifyThrow( FIsTag() );
    _TyLexValue & rval = GetValue();
    if ( rval.FIsNull() )
    {
      // Set up tag structure: leave the array of attributes as null for now.
      rval.SetSize( 2 ); // (tag,rgattr)
      _TyLexValue & rvalTag = rval[vknTagNameIdx];
      //( name, namespaceWrapper, nNamespaceDecls )
      rvalTag.SetSize( 2 );
      rvalTag.emplace_back( size_t(0) ); // Set number of namespace decls.
      rval[vknAttributesIdx].SetArray(); // make second element an empty array of attributes.
    }
  }
  // Test for a valid qualified name and return the position of the colon or zero if no colon.
  // Throw if we encounter an error.
  size_t _NColonValidQualifiedName( const _TyChar * _pcNameTest, size_t _nLenName )
  {
    const _TyStateProto * const kpspNCNameStart = PspGetNCNameStart< _TyChar >();
    const _TyChar * pcMatch = _l_match< _TyChar >::PszMatch( kpspNCNameStart, _pcNameTest, _nLenName );
    size_t nPosColon = *pcMatch == _TyChar(':') ? ( pcMatch - _pcNameTest ) : 0;
    if ( nPosColon )
      pcMatch = _l_match< _TyChar >::PszMatch( kpspNCNameStart, pcMatch + 1, _nLenName - nPosColon - 1 );
    VerifyThrowSz( ( pcMatch - _pcNameTest ) == _nLenName, "Invalid characters found in qaulified name[%s]", StrConvertString< char >( _pcNameTest, _nLenName ).c_str() );
    return nPosColon;
  }
  void _SetTagName( _TyXmlDocumentContext & _rcxtDoc, _TyStrView & _rsvTagName, _TyStrView * _psvPrefix, _TyStrView * _psvUri, _TyXmlNamespaceValueWrap const * _pxnvw = nullptr )
  {
    Assert( !_psvPrefix == !_psvUri );
    size_t nPosColon = _NColonValidQualifiedName( &_rsvTagName[0], _rsvTagName.length() );
    _TyLexValue & rvalTag = GetValue()[vknTagNameIdx];
    _TyLexValue & rvalNS = rvalTag[vknNamespaceIdx];
    if ( _rcxtDoc.FHasNamespaceMap() && ( _psvPrefix || _pxnvw || nPosColon || _rcxtDoc.HasDefaultNamespace() ) )
    {
      // If a prefix was present in the name then it must match either a currently active prefix or the prefix passed in in _ppuNamespace.
      _TyStrView svPrefix( &_rsvTagName[0], nPosColon );
      if ( nPosColon )
      {
        VerifyThrowSz(  ( !_psvPrefix && !_pxnvw ) || 
                        ( !!_psvPrefix && ( *_psvPrefix == svPrefix ) ) ||
                        ( !!_pxnvw && ( _pxnvw->RStringPrefix() == svPrefix ) ), "Tag prefix(when present) must match passed (prefix,URI)." );
        if ( !_psvPrefix && !_pxnvw )
        { // Use the same codepath below.
          Assert( !_psvUri );
          _psvPrefix = &svPrefix;
        }
      }

      if ( !_pxnvw )
      {
        // Get the namespace value wrap. If this represents a reference to an existing current (prefix,uri)
        //  pair then we needn't declare the namespace attribute - with no loss of generality.
        _TyXmlNamespaceValueWrap xnvw = _rcxtDoc.GetNamespaceValueWrap( _psvPrefix, _psvUri );
        if ( xnvw.FIsNamespaceDeclaration() )
        {
          // Then we must declare the namespace as an attribute, this will
          //  add one to the number of namespace decls associated with this element.
          _DeclareNamespace( _rcxtDoc, std::move( xnvw ), enrtTagNameReference );
          Assert( xnvw.FIsNamespaceReference() ); // Pass in a declaration, return a reference.
        }
        rvalNS.emplaceVal( std::move( xnvw ) );
      }
      else
      {
        if ( !_pxnvw->FIsNamespaceDeclaration() ) // We know that a namespace declaration is an active namespace.
        { // check to see that the URI is the active namespace for the given prefix.
          VerifyThrowSz( _rcxtDoc.FIsActiveNamespace( *_pxnvw ), "Trying to use an inactive namespace as the current namespace." );
          // Note that we could allow this to switch the namespace as well - it's a matter of design.
        }
        // Update the tag's reference to the namespace:
        rvalNS.emplaceVal( _pxnvw->ShedReference( enrtTagNameReference ) );
      }
    }
    else
    {
      // The tag is in no namespace:
      rvalNS.emplaceVal( false );
    }
    _SetName( _rcxtDoc.FIncludePrefixesInAttrNames(), _rcxtDoc, _rsvTagName, nPosColon, *_psvPrefix, rvalTag[vknNameIdx] );
    // I think we are done... whew!
  }
  const _TyXmlNamespaceValueWrap * _PGetDefaultAttributeNamespace( _TyXmlDocumentContext & _rcxtDoc, const _TyXmlNamespaceValueWrap * _pxnvw ) const
  {
    return !_pxnvw ? ( _rcxtDoc.FHasDefaultAttributeNamespace() ? &_rcxtDoc.GetDefaultAttributeNamespace() : nullptr ) : _pxnvw;
  }
  void _AddAttribute( _TyXmlDocumentContext & _rcxtDoc, _TyStrView const & _rsvName, _TyStrView const & _rsvValue, const _TyXmlNamespaceValueWrap * _pxnvw )
  {
    const _TyXmlNamespaceValueWrap * pxnvwDefaulted = _PGetDefaultAttributeNamespace( _pxnvw );
    VerifyThrow( FIsTag() && !GetValue().FIsNull() && ( !pxnvwDefaulted || _rcxtDoc.FHasNamespaceMap() ) );
    size_t nPosColon = _NColonValidQualifiedName( &_rsvName[0], _rsvName.length() );
    _TyLexValue & rvalAttrNew = _DeclareNewAttr( _rcxtDoc );
    _TyLexValue & rvalNS = rvalAttrNew[vknNamespaceIdx];
    _TyStrView svPrefix( &_rsvName[0], nPosColon );
    if ( _rcxtDoc.FHasNamespaceMap() && ( nPosColon || pxnvwDefaulted ) )
    {
      // As with the tag declaration: If a prefix is present then it should match the 
      if ( !pxnvwDefaulted )
        rvalNS.emplaceVal( _rcxtDoc.GetNamespaceValueWrap( svPrefix ) );
      else
      {
        VerifyThrowSz( !nPosColon || ( svPrefix == pxnvwDefaulted->RStringPrefix() ), "Prefix(when present) must match passed namespace(when present) object's prefix." );
        VerifyThrowSz( pxnvwDefaulted->FIsNamespaceDeclaration() || _rcxtDoc.FIsActiveNamespace( *pxnvwDefaulted ), "Trying to use an inactive namespace as the current namespace." );
        rvalNS.emplaceVal( pxnvwDefaulted->ShedReference( enrtAttrNameReference ) );
      }
      // Now make sure that this isn't a default namespace because there is no way to indicate that on an attribute name...
      VerifyThrowSz( !rvalNS.template GetVal< _TyXmlNamespaceValueWrap >().RStringPrefix().empty(), "Attempt to apply the default namespace to an attribute name. That don't feng shui." );
    }
    else
    {
      rvalNS.emplaceVal( false ); // the attribute is in no namespace.
    }
    // Now we need to set the name appropriately according to the current options.
    _SetName( true, _rcxtDoc, _rsvName, nPosColon, svPrefix, rvalAttrNew[vknNameIdx] );
    // Now just set in the value - we don't validate it until we write it out - since validating it and writing it go hand in hand.
    rvalAttrNew[vknAttr_ValueIdx].template emplaceArgs< _TyStdStr >( _rsvValue );
  }
  void _SetName( bool _fIncludePrefixInName, _TyStrView const & _rsvName, size_t _nPosColon, _TyStrView const & _rsvPrefix, _TyLexValue & _rvalName )
  {
    // Now set the name according to whether we should include prefixes or not:
    if ( _fIncludePrefixInName == !_nPosColon )
    {
      if ( _nPosColon )
        _rvalName.template emplaceArgs< _TyStdStr >( &_rsvName[0] + _nPosColon + 1, _rsvName.length() - _nPosColon - 1 );
      else
      if ( !_rsvPrefix.empty() ) // no prefix for default namespace.
      {
        _TyStdStr & rstrTagName = _rvalName.template emplaceArgs< _TyStdStr >( _rsvPrefix );
        rstrTagName += _TyChar( ':');
        rstrTagName += _rsvName;
      }
      else
      {
        Assert( !_nPosColon );
        _rvalName.template emplaceArgs< _TyStdStr >( _rsvName );
      }
    }
    else
    {
      // name is already in correct format:
      _rvalName.template emplaceArgs< _TyStdStr >( _rsvName );
    }
  }
  // Declare a new attribute and fill it with the appropriate defaults.
  _TyLexValue & _DeclareNewAttr( _TyXmlDocumentContext & _rcxtDoc )
  {
    _TyLexValue & rrgvalAttrs = GetValue()[vknAttributesIdx];
    Assert( rrgvalAttrs.FIsArray() );
    _TyLexValue & rvalAttrNew = rrgvalAttrs.emplace_back();
    // (name,namespacewrap,value,fusedoublequote)
    rvalAttrNew.SetSize( 4 );
    rvalAttrNew[vknAttr_FDoubleQuoteIdx].emplaceVal( _rcxtDoc.FAttributeValuesDoubleQuote() );
    return rvalAttrNew;
  }
  // Add the namespace to the set of attributes.
  void _DeclareNamespace( _TyXmlDocumentContext & _rcxtDoc, _TyXmlNamespaceValueWrap && _rrxnvw, ENamespaceReferenceType _enrtReferenceType )
  {
    Assert( _rrxnvw.FIsNamespaceDeclaration() );
    _TyLexValue & rvalAttrNew = _DeclareNewAttr( _rcxtDoc );
    {//B - attr name
      _TyStdStr & rstrAttrName = rvalAttrNew[vknNameIdx].template emplaceArgs< _TyStdStr >( _TyMarkupTraits::s_kszXmlnsEtc, StaticStringLen( _TyMarkupTraits::s_kszXmlnsEtc ) );
      rstrAttrName += _rrxnvw.RStringPrefix();
    }//EB
    // attr value: URI: We can use a string view on the URI from the URI map.
    rvalAttrNew[vknAttr_ValueIdx].template emplaceArgs< _TyStrView >( _rrxnvw.RStringUri() );
    _TyXmlNamespaceValueWrap & rxnvw = rvalAttrNew[vknNamespaceIdx].emplaceVal( std::move( _rrxnvw ) ); // Now move the wrapper into place so that when the value is destroyed we remove the namespace.
    Assert( _rrxnvw.FIsNull() );
    _rrxnvw = rxnvw.ShedReference( _enrtReferenceType ); // return a reference inside of the passed ref.
    // Add one to the count of attribute namespace declarations in this tag:
    ++GetValue()[vknTagNameIdx][vknTagName_NNamespaceDeclsIdx].template GetVal<vtySignedLvalueInt>();
  }

#if ASSERTSENABLED
  void _AssertValidName( bool _fIsTag, const _TyLexValue & _rrgvName, vtySignedLvalueInt & _rnNamespaceDecls, const _TyXmlNamespaceValueWrap ** _ppxnvw, bool _fIsAttrNamespaceDecl = false ) const
  {
    const _TyLexValue & rvName = _rrgvName[vknNameIdx];
    Assert( ( rvName.FHasTypedData() && !rvName.FEmptyTypedData() ) || rvName.FIsString() );
    const _TyLexValue & rvNS = _rrgvName[vknNamespaceIdx];
    Assert( rvNS.FIsBool() || rvNS.template FIsA< _TyXmlNamespaceValueWrap >() );
    if ( _ppxnvw )
      *_ppxnvw = nullptr;
    if ( rvNS.template FIsA< _TyXmlNamespaceValueWrap >() )
    {
      const _TyXmlNamespaceValueWrap & rxnvw = rvNS.template GetVal< _TyXmlNamespaceValueWrap >();
      if ( _ppxnvw )
        *_ppxnvw = &rxnvw;
      Assert( !rxnvw.FIsNamespaceDeclaration() || !_fIsTag );
      if ( rxnvw.FIsNamespaceDeclaration() )
        ++_rnNamespaceDecls;
      else
      {
        ENamespaceReferenceType enrtReferenceType;
        Assert( rxnvw.FIsNamespaceReference( &enrtReferenceType ) );
        // Make sure the reference is categorized correctly.
        switch( enrtReferenceType )
        {
          default:
          case enrtStandaloneReference:
          case enrtNamespaceReferenceTypeCount:
            Assert( false ); // Should never see these inside of an element.
          break;
          case enrtTagNameReference:
            Assert( _fIsTag );
          break;
          case enrtAttrNameReference:
            Assert( !_fIsTag && !_fIsAttrNamespaceDecl );
          break;
          case enrtAttrNamespaceDeclReference:
            Assert( !_fIsTag && _fIsAttrNamespaceDecl );
          break;
        };
      }
    }
    else
    {
      Assert( !_fIsAttrNamespaceDecl ); // We shouldn't be indicating that it is a namespace declaration in the method.
    }
  }
  void _AssertValidTag( bool _fUseNamespaces ) const
  {
    _TyLexValue const & rvRoot = GetValue();
    Assert( vknTagArrayCount == rvRoot.GetSize() );
    if ( vknTagArrayCount == rvRoot.GetSize() )
    {
      _TyLexValue const & rvTag = rvRoot[vknTagNameIdx];
      // When we are skipping tags we don't parse namespace decls to speed up processing.
      Assert( ( vknTagName_ArrayCount == rvTag.GetSize() ) || ( vknTagName_ArrayCount-1 == rvTag.GetSize() ) );
      if ( vknTagName_ArrayCount == rvTag.GetSize() )
      {
        vtySignedLvalueInt nNamespaceDecls = 0;
        const _TyXmlNamespaceValueWrap * pxnvw;
        _AssertValidName( true, rvTag, nNamespaceDecls, &pxnvw );
        Assert( !pxnvw || pxnvw->FIsNamespaceReference() );
        Assert( rvTag[vknTagName_NNamespaceDeclsIdx].template FIsA<vtySignedLvalueInt>() );
        _TyLexValue const & rrgvAttrs = rvRoot[vknAttributesIdx];
        Assert( rrgvAttrs.FIsArray() );
        if ( rrgvAttrs.FIsArray() )
        {
          const typename _TyLexValue::_TySegArrayValues & rsaAttrs = rrgvAttrs.GetValueArray();
          rsaAttrs.ApplyContiguous( 0, rsaAttrs.NElements(),
            [this,&nNamespaceDecls,&_fUseNamespaces]( const _TyLexValue * _pvBegin, const _TyLexValue * _pvEnd )
            {
              for ( const _TyLexValue * pvCur = _pvBegin; _pvEnd != pvCur; ++pvCur )
              {
                Assert( vknAttr_ArrayCount == pvCur->GetSize() );
                if ( vknAttr_ArrayCount == pvCur->GetSize() )
                {
                  const _TyXmlNamespaceValueWrap * pxnvw;
                  bool fIsAttrNamespaceDecl;
                  bool fIsAttr = _FIsAttribute( *pvCur, &fIsAttrNamespaceDecl );
                  _AssertValidName( false, *pvCur, nNamespaceDecls, &pxnvw, fIsAttrNamespaceDecl );
                  Assert( fIsAttr );
                  Assert( !pxnvw || !pxnvw->FIsNull() );
                  Assert( !( !!pxnvw && pxnvw->FIsNamespaceDeclaration() ) || fIsAttrNamespaceDecl );
                  Assert( !_fUseNamespaces || !fIsAttrNamespaceDecl || ( !!pxnvw && !pxnvw->FIsNull() ) );
                  const _TyLexValue & rvValue = (*pvCur)[vknAttr_ValueIdx];
                  Assert( rvValue.FHasTypedData() || rvValue.FIsString() ); // might be empty.
                  Assert( (*pvCur)[vknAttr_FDoubleQuoteIdx].FIsBool() );
                }
              }
            }
          );
        }
        Assert( !_fUseNamespaces || ( nNamespaceDecls == rvTag[vknTagName_NNamespaceDeclsIdx].template GetVal<vtySignedLvalueInt>() ) );
      }
      else
      if ( vknTagName_ArrayCount-1 == rvTag.GetSize() )
      {
        // Then we are in "skip" mode (or we have a bug). In this case we aren't processing namespace declarations, etc. so
        //  we aren't as strict about our assertions. In fact there isn't too much to do here... lol.
      }
    }
  }
  void _AssertValidComment() const
  {
    _TyLexValue const & rvRoot = GetValue();
    // An empty comment is not valid - but we might allow it here and just fix up to a single space on output.
    Assert( ( rvRoot.FHasTypedData() && !rvRoot.FEmptyTypedData() ) || rvRoot.FIsString() );
  }
  void _AssertValidXMLDecl() const
  {
    _TyLexValue const & rvRoot = GetValue();
    Assert( rvRoot.FIsNull() || rvRoot.FIsArray() ); // null for a pseudo token here.
    if ( rvRoot.FIsArray() )
    {
      Assert( vknXMLDecl_ArrayCount == rvRoot.GetSize() );
      if ( vknXMLDecl_ArrayCount == rvRoot.GetSize() )
      {
        Assert( rvRoot[vknXMLDecl_StandaloneYesIdx].FIsBool() );
        Assert( rvRoot[vknXMLDecl_StandaloneNoIdx].FIsBool() );
        Assert( rvRoot[vknXMLDecl_StandaloneDoubleQuoteIdx].FIsBool() );
        Assert( rvRoot[vknXMLDecl_EncodingIdx].FHasTypedData() || rvRoot[vknXMLDecl_EncodingIdx].FIsString() );
        Assert( rvRoot[vknXMLDecl_EncodingDoubleQuoteIdx].FIsBool() );
        Assert( rvRoot[vknXMLDecl_EncodingSingleQuoteIdx].FIsBool() );
        Assert( rvRoot[vknXMLDecl_VersionMinorNumberIdx].FHasTypedData() || rvRoot[vknXMLDecl_VersionMinorNumberIdx].FIsString() );
        Assert( rvRoot[vknXMLDecl_VersionMinorNumberDoubleQuoteIdx].FIsBool() );
      }
    }
  }
  void _AssertValidCDataSection() const
  {
    // An empty CDataSection is valid - why you would want one is questionable.
    _TyLexValue const & rvRoot = GetValue();
    Assert( rvRoot.FHasTypedData() || rvRoot.FIsString() );
  }
  void _AssertValidCharData() const
  {
    // Empty CharData is not valid - production-wise - but we would just not write anything at all.
    _TyLexValue const & rvRoot = GetValue();
    Assert( ( rvRoot.FHasTypedData() && !rvRoot.FEmptyTypedData() ) || rvRoot.FIsString() );
  }
  void _AssertValidProcessingInstruction() const
  {
    _TyLexValue const & rvRoot = GetValue();
    Assert( rvRoot.FIsArray() );
    if ( rvRoot.FIsArray() )
    {
      // We expect a potentially null element in the "meat" of the PI.
      Assert( 2 == rvRoot.GetSize() );
      _TyLexValue const & rvTarget = rvRoot[vknProcessingInstruction_PITargetIdx];
      Assert( ( rvTarget.FHasTypedData() && !rvTarget.FEmptyTypedData() ) || rvTarget.FIsString() );
      _TyLexValue const & rvMeat = rvRoot[vknProcessingInstruction_MeatIdx];
      Assert( rvMeat.FHasTypedData() || rvMeat.FIsString() );
    }
  }
#endif //ASSERTSENABLED

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
  void AssertValid( bool _fUseNamespaces = false ) const
  {
#if ASSERTSENABLED
    std::visit( _VisitHelpOverloadFCall {
      [_fUseNamespaces]( const auto & _tXmlToken ) -> void
      {
        _tXmlToken.AssertValid( _fUseNamespaces );
      }
    }, m_varXmlToken );
#endif //ASSERTSENABLED
  }
  bool FNullValue() const
  {
    return std::visit( _VisitHelpOverloadFCall {
      []( const auto & _tXmlToken ) -> bool
      {
        return _tXmlToken.FNullValue();
      }
    }, m_varXmlToken );
  }
  vtyTokenIdent GetTokenId() const
  {
    return std::visit( _VisitHelpOverloadFCall {
      []( const auto & _tXmlToken ) -> vtyTokenIdent
      {
        return _tXmlToken.GetTokenId();
      }
    }, m_varXmlToken );
  }
  _TyVariant & GetVariant()
  {
    return m_varXmlToken;
  }
  const _TyVariant & GetVariant() const
  {
    return m_varXmlToken;
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
