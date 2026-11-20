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
#include "xml_attr_default.h"
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
      case s_knTokenDoctypeDecl:
        Assert( GetValue().FHasTypedData() || GetValue().FIsString() ); // The text of the declaration.
      break;
      case s_knTokenEntityContentBegin:
      case s_knTokenEntityContentEnd:
        Assert( GetValue().FIsString() ); // The name of the entity.
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
    return s_knTokenProcessingInstruction == m_tokToken.GetTokenId();
  }
  // Where the content of a general entity begins and ends - see s_knTokenEntityContentBegin.
  bool FIsEntityContentBegin() const
  {
    return s_knTokenEntityContentBegin == m_tokToken.GetTokenId();
  }
  bool FIsEntityContentEnd() const
  {
    return s_knTokenEntityContentEnd == m_tokToken.GetTokenId();
  }
  // The document type declaration - see s_knTokenDoctypeDecl.
  bool FIsDoctypeDecl() const
  {
    return s_knTokenDoctypeDecl == m_tokToken.GetTokenId();
  }
  template< class t_TyStrView >
  void KGetStringView( _TyLexValue const & _rval, t_TyStrView & _rsv ) const
  {
    Assert( _rval.FHasTypedData() );
    VerifyThrow( _rval.FHasTypedData() );
    m_tokToken.KGetStringView( _rsv, _rval );
  }
  // Tag methods - for a tag which is made rather than read: see xml_writer<>::StartTag().
  // The name of the tag - and so the namespace which it is in, when namespaces are used. The name is then a qualified
  //  name ( Namespaces in XML [7] ) and:
  //  1) With (_svPrefix,_svUri): the tag is in that namespace - which is declared on the tag unless it is the one in scope
  //     for the prefix. A prefix which the name has is to be _svPrefix; a name without one is given it.
  //  2) With _rxnvw: the tag is in that namespace, which is to be in scope. Its prefix is that of the name likewise.
  //  3) Otherwise the tag is in the namespace in scope for the prefix of the name - it is an error if there is none - or,
  //     for a name without one, in the default namespace if one is in scope ( 6.2 ).
  // Without namespaces the name is a Name [5]: a colon is a character of it as any other.
  // The strings may be of any type of character: they are converted to ours.
  template < class t_TyCharName >
  void SetTagName( _TyXmlDocumentContext & _rcxtDoc, basic_string_view< t_TyCharName > _svName )
  {
    _InitTag();
    TGetConversionBuffer_t< t_TyCharName, _TyChar > cbufName;
    _SetTagName( _rcxtDoc, _SvConvert( _svName, cbufName ), nullptr, nullptr );
  }
  template < class t_TyCharName, class t_TyCharPrefix, class t_TyCharUri >
  void SetTagName( _TyXmlDocumentContext & _rcxtDoc, basic_string_view< t_TyCharName > _svName, basic_string_view< t_TyCharPrefix > _svPrefix, basic_string_view< t_TyCharUri > _svUri )
  {
    VerifyThrowSz( _rcxtDoc.FHasNamespaceMap(), "Namespaces are not used but a namespace was given for the tag." );
    _InitTag();
    TGetConversionBuffer_t< t_TyCharName, _TyChar > cbufName;
    TGetConversionBuffer_t< t_TyCharPrefix, _TyChar > cbufPrefix;
    TGetConversionBuffer_t< t_TyCharUri, _TyChar > cbufUri;
    const _TyStrView svPrefix = _SvConvert( _svPrefix, cbufPrefix );
    const _TyStrView svUri = _SvConvert( _svUri, cbufUri );
    _SetTagName( _rcxtDoc, _SvConvert( _svName, cbufName ), &svPrefix, &svUri );
  }
  template < class t_TyCharName >
  void SetTagName( _TyXmlDocumentContext & _rcxtDoc, _TyXmlNamespaceValueWrap const & _rxnvw, basic_string_view< t_TyCharName > _svName )
  {
    VerifyThrowSz( _rcxtDoc.FHasNamespaceMap(), "Namespaces are not used but a namespace was given for the tag." );
    _InitTag();
    TGetConversionBuffer_t< t_TyCharName, _TyChar > cbufName;
    _SetTagName( _rcxtDoc, _SvConvert( _svName, cbufName ), nullptr, nullptr, &_rxnvw );
  }
  // Add the namespace to the set of attributes.
  void DeclareNamespace( _TyXmlDocumentContext & _rcxtDoc, _TyXmlNamespaceValueWrap && _rrxnvw )
  {
    Assert( _rrxnvw.FIsAttributeNamespaceDeclaration() && _rrxnvw.FIsNamespaceReference() ); // yep.
    _TyLexValue & rvalAttrNew = _DeclareNewAttr( _rcxtDoc );
    {//B - attr name
      // xmlns:prefix - or xmlns for the default namespace.
      _TyStdStr & rstrAttrName = rvalAttrNew[vknNameIdx].template emplaceArgs< _TyStdStr >( _TyMarkupTraits::s_kszXmlnsEtc, StaticStringLen( _TyMarkupTraits::s_kszXmlnsEtc ) - ( _rrxnvw.RStringPrefix().empty() ? 1 : 0 ) );
      rstrAttrName += _rrxnvw.RStringPrefix();
    }//EB
    // attr value: URI: We can use a string view on the URI from the URI map.
    rvalAttrNew[vknAttr_ValueIdx].template emplaceArgs< _TyStrView >( _rrxnvw.RStringUri() );
    _TyXmlNamespaceValueWrap & rxnvw = rvalAttrNew[vknNamespaceIdx].emplaceVal( std::move( _rrxnvw ) ); // Now move the wrapper into place so that when the value is destroyed we remove the namespace.
    Assert( _rrxnvw.FIsNull() );
    ++GetValue()[vknTagNameIdx][vknTagName_NNamespaceDeclsIdx].template GetVal<vtySignedLvalueInt>();
  }

  // Declare the namespace (_svPrefix,_svUri) on this tag - unless it is the one in scope for the prefix, when there is
  //  nothing to declare. The empty prefix is the default namespace - and with the empty URI that undeclares it ( 6.2 ).
  // A declaration applies to the tag which it is on: the tag and its attributes of the prefix are in the namespace.
  // What is returned refers to the namespace: it may be given to attributes and to the tags within this one - while it
  //  is in scope.
  template < class t_TyCharPrefix, class t_TyCharUri >
  _TyXmlNamespaceValueWrap AddNamespace( _TyXmlDocumentContext & _rcxtDoc, basic_string_view< t_TyCharPrefix > _svPrefix, basic_string_view< t_TyCharUri > _svUri )
  {
    VerifyThrow( FIsTag() && !GetValue().FIsNull() ); // The tag has its name.
    VerifyThrowSz( _rcxtDoc.FHasNamespaceMap(), "Namespaces are not used - a namespace may not be declared." );
    TGetConversionBuffer_t< t_TyCharPrefix, _TyChar > cbufPrefix;
    TGetConversionBuffer_t< t_TyCharUri, _TyChar > cbufUri;
    const _TyStrView svPrefix = _SvConvert( _svPrefix, cbufPrefix );
    const _TyStrView svUri = _SvConvert( _svUri, cbufUri );
    VerifyThrowSz( svPrefix.empty() || ( NLenNCName( svPrefix ) == svPrefix.length() ), "The prefix [%s] is not an NCName.", StrConvertString< char >( svPrefix ).c_str() );
    VerifyThrowSz( svPrefix != _SvXmlns(), "The prefix xmlns is not declared ( Namespaces in XML 3 )." );
    VerifyThrowSz( svUri != _TyStrView( _TyMarkupTraits::s_kszXmlnsUri, StaticStringLen( _TyMarkupTraits::s_kszXmlnsUri ) ), "The namespace of xmlns is not declared ( Namespaces in XML 3 )." );
    VerifyThrowSz( ( svUri != _TyStrView( _TyMarkupTraits::s_kszXmlUri, StaticStringLen( _TyMarkupTraits::s_kszXmlUri ) ) ) || ( svPrefix == _TyStrView( _TyMarkupTraits::s_kszXmlPrefix, StaticStringLen( _TyMarkupTraits::s_kszXmlPrefix ) ) ),
      "The namespace of the prefix xml is not bound to another prefix ( Namespaces in XML 3 )." );
    // A tag declares a prefix once.
    _TyLexValue const & rrgvalAttrs = GetValue()[vknAttributesIdx];
    for ( size_t nAttr = 0; nAttr < rrgvalAttrs.GetSize(); ++nAttr )
    {
      _TyLexValue const & rvalNS = rrgvalAttrs[nAttr][vknNamespaceIdx];
      if ( rvalNS.template FIsA< _TyXmlNamespaceValueWrap >() && rvalNS.template GetVal< _TyXmlNamespaceValueWrap >().FIsNamespaceDeclaration() )
        VerifyThrowSz( rvalNS.template GetVal< _TyXmlNamespaceValueWrap >().RStringPrefix() != svPrefix, "The prefix [%s] is declared on this tag already.", StrConvertString< char >( svPrefix ).c_str() );
    }
    _TyXmlNamespaceValueWrap xnvw = _rcxtDoc.GetNamespaceValueWrap( enrtStandaloneReference, &svPrefix, &svUri );
    if ( xnvw.FIsNamespaceDeclaration() )
    {
      _DeclareNamespace( _rcxtDoc, std::move( xnvw ), enrtStandaloneReference ); // Leaves a reference to it in xnvw.
      _RebindNames( xnvw );
    }
    return xnvw;
  }
  // Add an attribute. When namespaces are used its name is a qualified name: an attribute with a prefix is in the
  //  namespace in scope for the prefix - or in that of _pxnvw, whose prefix it is then to be. One without is in no
  //  namespace - unless _pxnvw, or the default namespace for attributes, gives it one: the name is then given its prefix.
  //  ( The default namespace isn't that of an attribute - 6.2: the namespace of an attribute is that of its prefix. )
  // A namespace is declared by AddNamespace() - not as an attribute.
  // The value is the characters of the value: it is checked, and what must be is escaped, when it is written.
  template < class t_TyCharName, class t_TyCharValue >
  void AddAttribute(  _TyXmlDocumentContext & _rcxtDoc, basic_string_view< t_TyCharName > _svName, basic_string_view< t_TyCharValue > _svValue,
                      const _TyXmlNamespaceValueWrap * _pxnvw = nullptr )
  {
    TGetConversionBuffer_t< t_TyCharName, _TyChar > cbufName;
    TGetConversionBuffer_t< t_TyCharValue, _TyChar > cbufValue;
    _AddAttribute( _rcxtDoc, _SvConvert( _svName, cbufName ), _SvConvert( _svValue, cbufValue ), _pxnvw );
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
        // ( The name may be a string of another type of character - see CheckDuplicateTokenAttrs(). )
        _TyStrView svName;
        _TyStdStr strName;
        (void)m_tokToken.FGetStringViewOrString( svName, strName, _rrgval[vknNameIdx] );
        *_pfIsAttrNamespaceDecl = svName.starts_with( str_array_cast< _TyChar >("xmlns") );
      }
      return true;
    }
    return false;
  }
protected:
  // A string of another type of character as ours - in _rbuf if it is converted.
  template < class t_TyCharFrom, class t_TyBuffer >
  static _TyStrView _SvConvert( basic_string_view< t_TyCharFrom > _sv, t_TyBuffer & _rbuf )
  {
    if ( _sv.empty() )
      return _TyStrView();
    return StrViewConvertString( _sv.data(), _sv.length(), _rbuf );
  }
  static _TyStrView _SvXmlns()
  {
    return _TyStrView( _TyMarkupTraits::s_kszXmlnsEtc, StaticStringLen( _TyMarkupTraits::s_kszXmlnsEtc ) - 1 ); // Without its ':'.
  }
  // A tag which is made: (name,namespace,count of namespace declarations) and the array of attributes.
  void _InitTag()
  {
    VerifyThrow( FIsTag() );
    _TyLexValue & rval = GetValue();
    VerifyThrowSz( rval.FIsNull(), "The name of a tag is set once." );
    rval.SetSize( vknTagArrayCount );
    _TyLexValue & rvalTag = rval[vknTagNameIdx];
    rvalTag.SetSize( vknTagName_ArrayCount );
    rvalTag[vknTagName_NNamespaceDeclsIdx].emplaceVal( vtySignedLvalueInt( 0 ) );
    rval[vknAttributesIdx].SetArray();
  }
public:
  // The names of XML - the writer checks names with these as well.
  // The length of the NCName [4] which _sv begins with - zero if it begins with none.
  static size_t NLenNCName( _TyStrView const & _sv )
  {
    if ( _sv.empty() )
      return 0;
    const _TyChar * pcMatch = _l_match< _TyChar >::PszMatch( PspGetNCNameStart< _TyChar >(), _sv.data(), _sv.length() );
    return !pcMatch ? 0 : size_t( pcMatch - _sv.data() );
  }
  // A qualified name [7] - Prefix ':' LocalPart, or LocalPart: the position of its colon - zero if it has none. Throws if
  //  it isn't one.
  static size_t NColonQualifiedName( _TyStrView const & _svName )
  {
    const size_t knLenFirst = NLenNCName( _svName );
    size_t nPosColon = 0;
    bool fValid = !!knLenFirst;
    if ( fValid && ( knLenFirst < _svName.length() ) )
    {
      nPosColon = knLenFirst;
      fValid = ( _TyChar(':') == _svName[ nPosColon ] ) && ( nPosColon + 1 < _svName.length() ) &&
        ( NLenNCName( _svName.substr( nPosColon + 1 ) ) == _svName.length() - nPosColon - 1 );
    }
    VerifyThrowSz( fValid, "[%s] is not a qualified name.", StrConvertString< char >( _svName ).c_str() );
    return nPosColon;
  }
  // A Name [5] - for when namespaces aren't used: NCNames and colons. What follows a colon may begin with any NameChar
  //  [4a] - it is matched as an NCName after a NameStartChar. Throws if it isn't one.
  static void ValidateName( _TyStrView const & _svName )
  {
    bool fValid = !_svName.empty();
    _TyStdStr strPiece;
    for ( size_t nPos = 0; fValid; )
    {
      const size_t knPosColon = _svName.find( _TyChar(':'), nPos );
      const _TyStrView svPiece = _svName.substr( nPos, ( _TyStrView::npos == knPosColon ) ? _TyStrView::npos : ( knPosColon - nPos ) );
      if ( !svPiece.empty() )
      {
        if ( !nPos )
          fValid = ( NLenNCName( svPiece ) == svPiece.length() );
        else
        {
          strPiece.assign( 1, _TyChar('_') );
          strPiece += svPiece;
          fValid = ( NLenNCName( strPiece ) == strPiece.length() );
        }
      }
      if ( _TyStrView::npos == knPosColon )
        break;
      nPos = knPosColon + 1;
    }
    VerifyThrowSz( fValid, "[%s] is not a name.", StrConvertString< char >( _svName ).c_str() );
  }
protected:
  // See SetTagName(): _psvPrefix and _psvUri are given together - or _pxnvw - or neither.
  void _SetTagName( _TyXmlDocumentContext & _rcxtDoc, _TyStrView const & _rsvTagName, _TyStrView const * _psvPrefix, _TyStrView const * _psvUri, _TyXmlNamespaceValueWrap const * _pxnvw = nullptr )
  {
    Assert( !_psvPrefix == !_psvUri );
    _TyLexValue & rvalTag = GetValue()[vknTagNameIdx];
    _TyLexValue & rvalNS = rvalTag[vknNamespaceIdx];
    if ( !_rcxtDoc.FHasNamespaceMap() )
    {
      ValidateName( _rsvTagName );
      rvalNS.emplaceVal( false );
      rvalTag[vknNameIdx].template emplaceArgs< _TyStdStr >( _rsvTagName );
      return;
    }
    const size_t knPosColon = NColonQualifiedName( _rsvTagName );
    const _TyStrView svPrefixOfName( _rsvTagName.data(), knPosColon );
    VerifyThrowSz( svPrefixOfName != _SvXmlns(), "A tag doesn't have the prefix xmlns ( Namespaces in XML 3 )." );
    if ( !!_pxnvw )
    {
      VerifyThrowSz( !knPosColon || ( svPrefixOfName == _pxnvw->RStringPrefix() ), "The prefix of the tag [%s] is not that of the namespace which was given for it.", StrConvertString< char >( _rsvTagName ).c_str() );
      VerifyThrowSz( _rcxtDoc.FIsActiveNamespace( *_pxnvw ), "The namespace which was given for the tag [%s] is not in scope.", StrConvertString< char >( _rsvTagName ).c_str() );
      rvalNS.emplaceVal( _pxnvw->ShedReference( enrtTagNameReference ) );
    }
    else
    if ( !!_psvPrefix )
    {
      VerifyThrowSz( !knPosColon || ( svPrefixOfName == *_psvPrefix ), "The prefix of the tag [%s] is not that of the namespace which was given for it.", StrConvertString< char >( _rsvTagName ).c_str() );
      VerifyThrowSz( _psvPrefix->empty() || ( NLenNCName( *_psvPrefix ) == _psvPrefix->length() ), "The prefix [%s] is not an NCName.", StrConvertString< char >( *_psvPrefix ).c_str() );
      _TyXmlNamespaceValueWrap xnvw = _rcxtDoc.GetNamespaceValueWrap( enrtTagNameReference, _psvPrefix, _psvUri );
      if ( xnvw.FIsNamespaceDeclaration() )
        _DeclareNamespace( _rcxtDoc, std::move( xnvw ), enrtTagNameReference ); // Leaves a reference to it in xnvw.
      rvalNS.emplaceVal( std::move( xnvw ) );
    }
    else
    if ( knPosColon )
      rvalNS.emplaceVal( _rcxtDoc.GetNamespaceValueWrap( enrtTagNameReference, svPrefixOfName ) ); // Throws if the prefix isn't in scope.
    else
    {
      _TyXmlNamespaceValueWrap xnvwDefault;
      if ( _rcxtDoc.FHasDefaultNamespace( &xnvwDefault ) )
        rvalNS.emplaceVal( std::move( xnvwDefault ) );
      else
        rvalNS.emplaceVal( false );
    }
    const _TyStrView svPrefix = rvalNS.FIsBool() ? _TyStrView() : _TyStrView( rvalNS.template GetVal< _TyXmlNamespaceValueWrap >().RStringPrefix() );
    _SetName( _rcxtDoc.FIncludePrefixesInAttrNames(), _rsvTagName, knPosColon, svPrefix, rvalTag[vknNameIdx] );
  }
  // The namespace which _rxnvw refers to was just declared on this tag: the tag and its attributes of its prefix are in it.
  void _RebindNames( _TyXmlNamespaceValueWrap const & _rxnvw )
  {
    _TyStrView svPrefix( _rxnvw.RStringPrefix() );
    _TyLexValue & rvalNSTag = GetValue()[vknTagNameIdx][vknNamespaceIdx];
    // A tag in no namespace has no prefix: the default namespace is then its namespace. ( One whose URI is empty is no
    //  namespace: xmlns="" - 6.2. )
    if ( rvalNSTag.FIsBool() ? svPrefix.empty() : ( rvalNSTag.template GetVal< _TyXmlNamespaceValueWrap >().RStringPrefix() == svPrefix ) )
    {
      if ( _rxnvw.RStringUri().empty() )
        rvalNSTag.emplaceVal( false );
      else
        rvalNSTag.emplaceVal( _rxnvw.ShedReference( enrtTagNameReference ) );
    }
    if ( svPrefix.empty() )
      return; // Not that of an attribute.
    _TyLexValue & rrgvalAttrs = GetValue()[vknAttributesIdx];
    for ( size_t nAttr = 0; nAttr < rrgvalAttrs.GetSize(); ++nAttr )
    {
      _TyLexValue & rvalNS = rrgvalAttrs[nAttr][vknNamespaceIdx];
      if ( !rvalNS.template FIsA< _TyXmlNamespaceValueWrap >() || rvalNS.template GetVal< _TyXmlNamespaceValueWrap >().FIsAttributeNamespaceDeclaration() )
        continue;
      if ( rvalNS.template GetVal< _TyXmlNamespaceValueWrap >().RStringPrefix() == svPrefix )
        rvalNS.emplaceVal( _rxnvw.ShedReference( enrtAttrNameReference ) );
    }
  }
  const _TyXmlNamespaceValueWrap * _PGetDefaultAttributeNamespace( _TyXmlDocumentContext & _rcxtDoc, const _TyXmlNamespaceValueWrap * _pxnvw ) const
  {
    return !_pxnvw ? ( _rcxtDoc.FHasDefaultAttributeNamespace() ? &_rcxtDoc.GetDefaultAttributeNamespace() : nullptr ) : _pxnvw;
  }
  // See AddAttribute(). Everything is checked before the attribute is added: if this throws the tag is as it was.
  void _AddAttribute( _TyXmlDocumentContext & _rcxtDoc, _TyStrView const & _rsvName, _TyStrView const & _rsvValue, const _TyXmlNamespaceValueWrap * _pxnvw )
  {
    VerifyThrow( FIsTag() && !GetValue().FIsNull() ); // The tag has its name.
    if ( !_rcxtDoc.FHasNamespaceMap() )
    {
      VerifyThrowSz( !_pxnvw, "Namespaces are not used but a namespace was given for the attribute." );
      ValidateName( _rsvName );
      _TyLexValue & rvalAttrNew = _DeclareNewAttr( _rcxtDoc );
      rvalAttrNew[vknNamespaceIdx].emplaceVal( false );
      rvalAttrNew[vknNameIdx].template emplaceArgs< _TyStdStr >( _rsvName );
      rvalAttrNew[vknAttr_ValueIdx].template emplaceArgs< _TyStdStr >( _rsvValue );
      return;
    }
    const size_t knPosColon = NColonQualifiedName( _rsvName );
    const _TyStrView svPrefixOfName( _rsvName.data(), knPosColon );
    VerifyThrowSz( ( knPosColon ? svPrefixOfName : _rsvName ) != _SvXmlns(), "A namespace is declared by AddNamespace() - [%s] is not added as an attribute.", StrConvertString< char >( _rsvName ).c_str() );
    const _TyXmlNamespaceValueWrap * pxnvw = ( !_pxnvw && !knPosColon ) ? _PGetDefaultAttributeNamespace( _rcxtDoc, nullptr ) : _pxnvw;
    _TyXmlNamespaceValueWrap xnvw; // Null: the attribute is in no namespace.
    if ( !!pxnvw )
    {
      VerifyThrowSz( !knPosColon || ( svPrefixOfName == pxnvw->RStringPrefix() ), "The prefix of the attribute [%s] is not that of the namespace which was given for it.", StrConvertString< char >( _rsvName ).c_str() );
      VerifyThrowSz( !pxnvw->RStringPrefix().empty(), "The attribute [%s] is not in the default namespace - the namespace of an attribute is that of its prefix.", StrConvertString< char >( _rsvName ).c_str() );
      VerifyThrowSz( _rcxtDoc.FIsActiveNamespace( *pxnvw ), "The namespace which was given for the attribute [%s] is not in scope.", StrConvertString< char >( _rsvName ).c_str() );
      xnvw = pxnvw->ShedReference( enrtAttrNameReference );
    }
    else
    if ( knPosColon )
      xnvw = _rcxtDoc.GetNamespaceValueWrap( enrtAttrNameReference, svPrefixOfName ); // Throws if the prefix isn't in scope.
    const _TyStrView svPrefix = xnvw.FIsNull() ? _TyStrView() : _TyStrView( xnvw.RStringPrefix() );
    _TyLexValue & rvalAttrNew = _DeclareNewAttr( _rcxtDoc );
    if ( xnvw.FIsNull() )
      rvalAttrNew[vknNamespaceIdx].emplaceVal( false );
    else
      rvalAttrNew[vknNamespaceIdx].emplaceVal( std::move( xnvw ) );
    _SetName( _rcxtDoc.FIncludePrefixesInAttrNames(), _rsvName, knPosColon, svPrefix, rvalAttrNew[vknNameIdx] );
    rvalAttrNew[vknAttr_ValueIdx].template emplaceArgs< _TyStdStr >( _rsvValue );
  }
  // The name as it is kept: with its prefix - or without it, when the namespace has it - see FIncludePrefixesInAttrNames().
  // _rsvPrefix: the prefix of the namespace of the name.
  static void _SetName( bool _fIncludePrefixInName, _TyStrView const & _rsvName, size_t _nPosColon, _TyStrView const & _rsvPrefix, _TyLexValue & _rvalName )
  {
    if ( _fIncludePrefixInName == !_nPosColon )
    {
      if ( _nPosColon )
        _rvalName.template emplaceArgs< _TyStdStr >( _rsvName.data() + _nPosColon + 1, _rsvName.length() - _nPosColon - 1 );
      else
      if ( !_rsvPrefix.empty() )
      {
        _TyStdStr & rstrName = _rvalName.template emplaceArgs< _TyStdStr >( _rsvPrefix );
        rstrName += _TyChar( ':' );
        rstrName += _rsvName;
      }
      else
        _rvalName.template emplaceArgs< _TyStdStr >( _rsvName );
    }
    else
      _rvalName.template emplaceArgs< _TyStdStr >( _rsvName ); // As it is to be.
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
      // xmlns:prefix - or xmlns for the default namespace.
      _TyStdStr & rstrAttrName = rvalAttrNew[vknNameIdx].template emplaceArgs< _TyStdStr >( _TyMarkupTraits::s_kszXmlnsEtc, StaticStringLen( _TyMarkupTraits::s_kszXmlnsEtc ) - ( _rrxnvw.RStringPrefix().empty() ? 1 : 0 ) );
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
                  Assert( rvValue.FHasTypedData() || rvValue.FIsString() || FIsAttrDefaultValue( rvValue ) ); // might be empty.
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
                                              xml_value_traits_pack< typename t_TyTransport::_TyChar > >;
// The same for the tokens of XML traits which support the DTD or don't - the type of the user object is other.
template < bool t_kfSupportDTD >
struct TGetXmlTokenFromTransportOf
{
  template < class t_TyTransport >
  using _TyGet = xml_token< typename t_TyTransport::_TyTransportCtxt,
                            xml_user_obj< typename t_TyTransport::_TyChar, t_kfSupportDTD >,
                            xml_value_traits_pack< typename t_TyTransport::_TyChar > >;
};

// As with all tokens: No default constructor, which translates to a variant: No monostate.
template < class t_TyTpTransports, bool t_kfSupportDTD >
class xml_token_var
{
  typedef xml_token_var _TyThis;
public:
  typedef t_TyTpTransports _TyTpTransports;
  // Define our variant type - there is no monostate for this. We have to filter duplicates because some transports will share the same context.
  typedef unique_variant_t< MultiplexTuplePack_t< TGetXmlTokenFromTransportOf< t_kfSupportDTD >::template _TyGet, _TyTpTransports, variant > > _TyVariant;

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
template < class t_TyTpTransports, bool t_kfSupportDTD >
  void swap(xml_token_var< t_TyTpTransports, t_kfSupportDTD >& _rl, xml_token_var< t_TyTpTransports, t_kfSupportDTD >& _rr)
  {
    _rl.swap(_rr);
  }
}
