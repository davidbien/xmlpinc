#pragma once

// xml_context.h
// Various context-related objects for the XML parser/writer impl.
// dbien
// 26FEB2021

#include "xml_types.h"

__XMLP_BEGIN_NAMESPACE

template < class t_TyChar >
class XMLDeclProperties
{
  typedef XMLDeclProperties _TyThis;
public:
  typedef t_TyChar _TyChar;
  typedef basic_string< _TyChar > _TyStdStr;
  XMLDeclProperties() = default;
  XMLDeclProperties( XMLDeclProperties const & ) = default;
  XMLDeclProperties & operator=( XMLDeclProperties const & ) = default;
  XMLDeclProperties( XMLDeclProperties && ) = default;
  XMLDeclProperties & operator=( XMLDeclProperties && ) = default;
  XMLDeclProperties( bool _fStandalone, EFileCharacterEncoding _efce )
    : m_fStandalone( _fStandalone ),
      m_strEncoding( SvCharacterEncodingName< _TyChar >( _efce ) )
  {
  }
  void Init( bool _fStandalone, EFileCharacterEncoding _efce )
  {
    m_fStandalone = _fStandalone;
    m_strEncoding = SvCharacterEncodingName< _TyChar >( _efce );
  }
  void swap( _TyThis & _r )
  {
    std::swap( m_fStandalone, _r.m_fStandalone );
    m_strEncoding.swap( _r.m_strEncoding );
    std::swap( m_nVersionMinorNumber, _r.m_nVersionMinorNumber );
  }
  void clear()
  {
    m_strEncoding.clear();
    m_fStandalone = false;
    m_nVersionMinorNumber = 0;
  }
  template < class t_TyLexToken >
  void FromXMLDeclToken( t_TyLexToken const & _rltok )
  {
    typedef typename t_TyLexToken::_TyValue _TyLexValue;
    const _TyLexValue & rrgVals = _rltok.GetValue();
    if ( !rrgVals.FIsNull() )
    {
      Assert( rrgVals.FIsArray() );
      m_fStandalone = rrgVals[vknXMLDecl_StandaloneIdx].template GetVal<bool>();
      rrgVals[vknXMLDecl_EncodingIdx].GetString( _rltok, m_strEncoding );
      _TyStdStr strMinorVNum;
      rrgVals[vknXMLDecl_VersionMinorNumberIdx].GetString( _rltok, strMinorVNum );
      m_nVersionMinorNumber = uint8_t( strMinorVNum[0] - _TyChar('0') );
    }
  }
  _TyStdStr m_strEncoding;
  bool m_fStandalone{false};
  uint8_t m_nVersionMinorNumber{0};
};

// _xml_output_format:
// Static formatting options for XML output.
class _xml_output_format
{
  typedef _xml_output_format _TyThis;
public:
// If this is true then we won't add any spaces or end-of-lines when writing the output.
  bool m_fNoFormatting{true};
// Whether to use double quotes for attribute values by default.
  bool m_fAttributeValuesDoubleQuote{true};
// If this is negative one then a tab is used per indent. If it is zero then no indentation is added.
  int8_t m_byNSpacesPerIndent{2};
// This is the maximum indentation level for which spaces/tabs are added. After this the maximum is used.
  size_t m_nMaximumIndentLevelForWhitespace{32};
// This isn't really an "output" format but it determines what the tags look like while we have them in there.
// The xml_read_cursor has this same option and thus it is easy to keep things in sync when we are just copying
//  an entire XML node, etc.
// Note that it is faster to write the tag to the file since we have to write any prefix.
  bool m_fIncludePrefixesInAttrNames{true};

  ~_xml_output_format() = default;
  _xml_output_format() = default;
  _xml_output_format( _xml_output_format const & ) = default;
  _xml_output_format & operator =( _xml_output_format const & ) = default;
  void swap( _TyThis & _r )
  {
    if ( this == & _r )
      return;
    std::swap( m_fNoFormatting, _r.m_fNoFormatting );
    std::swap( m_fAttributeValuesDoubleQuote, _r.m_fAttributeValuesDoubleQuote );
    std::swap( m_byNSpacesPerIndent, _r.m_byNSpacesPerIndent );
    std::swap( m_nMaximumIndentLevelForWhitespace, _r.m_nMaximumIndentLevelForWhitespace );
    std::swap( m_fIncludePrefixesInAttrNames, _r.m_fIncludePrefixesInAttrNames );
  }
  bool FUseTabsForIndent() const
  {
    return -1 == m_byNSpacesPerIndent;
  }
  size_t NSpacesPerIndent() const
  {
    return -1 == m_byNSpacesPerIndent ? 0 : m_byNSpacesPerIndent;
  }
  bool FIncludePrefixesInAttrNames() const
  {
    return m_fIncludePrefixesInAttrNames;
  }
  bool FAttributeValuesDoubleQuote() const
  {
    return m_fAttributeValuesDoubleQuote;
  }
};
// _xml_output_context:
// Dynamic output context.
class _xml_output_context
{
  typedef _xml_output_context _TyThis;
public:
  ~_xml_output_context() = default;
  _xml_output_context() = default;
  _xml_output_context( const _xml_output_context & ) = default;
  _xml_output_context & operator =( const _xml_output_context & ) = default;

  size_t m_nCurIndentLevel{0};
};

// This is used with tags. We will encounter a local-declared namespace first as a reference in the tag.
// This will cause a namespace declaration in the copied tag's namespace field. Then when we encounter
//  the actual namespace declaraion (the attribute) we will notice that this namespace is already declared.
// This object will exchange the declaration with the reference if the declaration is encountered. If it isn't
//  encountered then we will declare the namespace by adding an attribute (after the copy is finished).
template < class t_TyChar >
class _xml_token_copy_context
{
  typedef _xml_token_copy_context _TyThis;
public:
  typedef t_TyChar _TyChar;
  typedef xml_namespace_value_wrap< _TyChar > _TyXmlNamespaceValueWrap;
  typedef tuple< _TyXmlNamespaceValueWrap > _TyTpValueTraitsPack;
  typedef _l_value< _TyChar, _TyTpValueTraitsPack > _TyLexValue;

  ~_xml_token_copy_context() = default;
  _xml_token_copy_context() = default;
  _xml_token_copy_context( _xml_token_copy_context const & ) = default;
  _xml_token_copy_context & operator=( _xml_token_copy_context const &) = default;

  _TyLexValue * m_plvalContainerCur{nullptr}; // This is set the the container for the current value - it may be null. 
  
  // We maintain two lists:
  // 1) An array of values containing namespace declaration values.
  // 2) An array of values containing namespace reference values.
  // Then we will appropriately match declarations with actual attribute namespace declarations.
  typedef vector< _TyLexValue * > _TyRgNamespaces;
  _TyRgNamespaces m_rgDeclarations;
  _TyRgNamespaces m_rgReferences;
};

// _xml_document_context:
// This organizes all the info from parsing that we need to keep to maintain a valid standalone xml_document. It contains backing memory
//  for string views on prefixes and URIs, the XMLDecl properties (or synthesized ones), and the transport for those transport types
//  using a non-backing transport context.
template < class t_TyLexUserObj >
class _xml_document_context
{
  typedef _xml_document_context _TyThis;
public:
  typedef t_TyLexUserObj _TyLexUserObj;
  typedef typename _TyLexUserObj::_TyChar _TyChar;
  typedef typename _xml_namespace_map_traits< _TyChar >::_TyUriAndPrefixMap _TyUriAndPrefixMap;
  typedef basic_string< _TyChar > _TyStdStr;
  typedef xml_namespace_map< _TyChar > _TyXmlNamespaceMap;
  typedef XMLDeclProperties< _TyChar > _TyXMLDeclProperties;
  typedef _xml_output_format _TyXMLOutputFormat;
  typedef _xml_output_context _TyXMLOutputContext;
  typedef pair< _TyXMLOutputFormat, _TyXMLOutputContext > _TyPrFormatContext;
  typedef xml_namespace_value_wrap< _TyChar > _TyXmlNamespaceValueWrap;
  typedef _xml_token_copy_context< _TyChar > _TyTokenCopyContext;
  typedef typename _TyTokenCopyContext::_TyLexValue _TyLexValue;
  typedef typename _TyLexUserObj::_TyEntityMap _TyEntityMap;

  void Init( bool _fStandalone, EFileCharacterEncoding _efce, bool _fUseNamespaces, const _TyPrFormatContext * _pprFormatContext = nullptr )
  {
    // Create the user object - it will create all the default entities in its entity map.
    m_upUserObj = make_unique< _TyLexUserObj >();
    m_XMLDeclProperties.Init( _fStandalone, _efce );
    m_mapUris.clear();
    m_mapPrefixes.clear();
    if ( _fUseNamespaces )
      m_optMapNamespaces.emplace( *this );
    else
      m_optMapNamespaces.reset();
    if ( _pprFormatContext )
      m_optprFormatContext.emplace( *_pprFormatContext );
    else
      m_optprFormatContext.reset();
  }
  ~_xml_document_context() = default;
  _xml_document_context() = default;
  _xml_document_context( _xml_document_context const & ) = delete;
  _xml_document_context & operator =( _xml_document_context const & ) = delete;
  _xml_document_context( _xml_document_context && ) = default;
  _xml_document_context & operator =( _xml_document_context && ) = default;
  void swap( _xml_document_context & _r )
  {
    if ( &_r == this )
      return;
    m_upUserObj.swap( _r.m_upUserObj );
    m_mapUris.swap( _r.m_mapUris );
    m_mapPrefixes.swap( _r.m_mapPrefixes );
    m_XMLDeclProperties.swap( _r.m_XMLDeclProperties );
    m_optMapNamespaces.swap( _r.m_optMapNamespaces );
    m_optprFormatContext.swap( _r.m_optprFormatContext );
    m_xnvwDefaultAttributeNamespace.swap( _r.m_xnvwDefaultAttributeNamespace );
  }
  bool FAttributeValuesDoubleQuote() const
  {
    Assert( !!m_optprFormatContext );
    return !m_optprFormatContext ? true : m_optprFormatContext->first.FAttributeValuesDoubleQuote();
  }
  bool FIncludePrefixesInAttrNames() const
  {
    Assert( !!m_optprFormatContext );
    return !m_optprFormatContext ? false : m_optprFormatContext->first.FIncludePrefixesInAttrNames();
  }
  _TyLexUserObj & GetUserObj()
  {
    Assert( !!m_upUserObj );
    return *m_upUserObj;
  }
  const _TyLexUserObj & GetUserObj() const
  {
    Assert( !!m_upUserObj );
    return *m_upUserObj;
  }
  template < class t_TyStrViewOrString >
  typename _TyUriAndPrefixMap::value_type const & RStrAddPrefix( t_TyStrViewOrString && _rrs )
  {
    typename _TyUriAndPrefixMap::const_iterator cit = m_mapPrefixes.find( _rrs );
    if ( m_mapPrefixes.end() != cit )
      return *cit;
    pair< typename _TyUriAndPrefixMap::iterator, bool > pib = m_mapPrefixes.insert( std::forward< t_TyStrViewOrString >( _rrs ) );
    Assert( pib.second );
    return *pib.first;
  }
  template < class t_TyStrViewOrString >
  typename _TyUriAndPrefixMap::value_type const & RStrAddUri( t_TyStrViewOrString && _rrs )
  {
    typename _TyUriAndPrefixMap::const_iterator cit = m_mapUris.find( _rrs );
    if ( m_mapUris.end() != cit )
      return *cit;
    pair< typename _TyUriAndPrefixMap::iterator, bool > pib = m_mapUris.insert( std::forward< t_TyStrViewOrString >( _rrs ) );
    Assert( pib.second );
    return *pib.first;
  }
  bool FHasNamespaceMap() const
  {
    return m_optMapNamespaces.has_value();
  }
  // Return if there is currently an active default namespace and return a reference to it if _pxnvw.
  bool FHasDefaultNamespace( _TyXmlNamespaceValueWrap * _pxnvw = nullptr ) const
  {
    Assert( FHasNamespaceMap() );
    return MapNamespaces().FHasDefaultNamespace( _pxnvw );
  }
  bool FIsActiveNamespace( _TyXmlNamespaceValueWrap const & _rxnvw ) const
  {
    return MapNamespaces().FIsActiveNamespace( _rxnvw );
  }
  const _TyEntityMap & GetEntityMap() const
  {
    Assert( !!m_upUserObj );
    return m_upUserObj->GetEntityMap();
  }
  _TyXmlNamespaceMap & MapNamespaces()
  {
    Assert( FHasNamespaceMap() );
    return *m_optMapNamespaces;
  }
  const _TyXmlNamespaceMap & MapNamespaces() const
  {
    Assert( FHasNamespaceMap() );
    return *m_optMapNamespaces;
  }
  template < class t_TyStrViewOrString >
  _TyXmlNamespaceValueWrap GetNamespaceValueWrap( const t_TyStrViewOrString * _psvPrefix, const t_TyStrViewOrString * _psvUri )
    requires TAreSameSizeTypes_v< typename t_TyStrViewOrString::value_type, _TyChar >
  {
    return MapNamespaces().GetNamespaceValueWrap( *this, _psvPrefix, _psvUri );
  }
  template < class t_TyStrViewOrString >
  _TyXmlNamespaceValueWrap GetNamespaceValueWrap( t_TyStrViewOrString && _rrsvPrefix, t_TyStrViewOrString && _rrsvUri )
    requires TAreSameSizeTypes_v< typename t_TyStrViewOrString::value_type, _TyChar >
  {
    return MapNamespaces().GetNamespaceValueWrap( *this, std::forward< t_TyStrViewOrString >( _rrsvPrefix ), std::forward< t_TyStrViewOrString >( _rrsvUri ) );
  }
  template < class t_TyStrViewOrString >
  _TyXmlNamespaceValueWrap GetNamespaceValueWrap( const t_TyStrViewOrString & _rsvPrefix )
    requires TAreSameSizeTypes_v< typename t_TyStrViewOrString::value_type, _TyChar >
  {
    return MapNamespaces().GetNamespaceValueWrap( _rsvPrefix );
  }
// Default attribute namespace support.
  bool FHasDefaultAttributeNamespace() const
  {
    return !m_xnvwDefaultAttributeNamespace.FIsNull();
  }
  bool ClearDefaultAttributeNamespace()
  {
    m_xnvwDefaultAttributeNamespace.Clear();
  }
  void SetDefaultAttributeNamespace( _TyXmlNamespaceValueWrap const & _rxnvw )
  {
    VerifyThrowSz( !m_xnvwDefaultAttributeNamespace.FIsDefaultNamespace(), "Attributes cannot use the default namespace." );
    m_xnvwDefaultAttributeNamespace = _rxnvw.ShedReference();
  }
  _TyXmlNamespaceValueWrap const & GetDefaultAttributeNamespace() const
  {
    return m_xnvwDefaultAttributeNamespace;
  }
  // _l_value<>::_MoveFrom() calls an r-value-reference version of this method but we don't need it - dont't want it.
  template < class t_TyXmlNamespaceValueWrapOther >
  void TranslateUserType( _TyTokenCopyContext * _ptccCopyCtxt, t_TyXmlNamespaceValueWrapOther const & _rxnvwOther, _TyLexValue & _rvalNSVW  )
  {
    Assert( _rvalNSVW.FIsNull() );
    Assert( !!_ptccCopyCtxt && !!_ptccCopyCtxt->m_plvalContainerCur ); // We always pass this in - we expect to be within a container.
    // First, regardless, we must get a (prefix,uri) from the other nsvw:
    typedef pair< _TyStdStr, _TyStdStr > _TyPrStrPrefixUri;
    _TyPrStrPrefixUri prstrPrefixUri;
    _rxnvwOther.GetPrStrPrefixUri( prstrPrefixUri );

    // Record the containers for all declarations and references - this lets us fix things up pretty easily.    
    _TyXmlNamespaceValueWrap xnvwThis = GetNamespaceValueWrap( std::move( prstrPrefixUri.first ), std::move( prstrPrefixUri.second ) );
    _TyXmlNamespaceValueWrap & rxnvwInValue = _rvalNSVW.emplaceVal< _TyXmlNamespaceValueWrap >( std::move( xnvwThis ) );
    if ( rxnvwInValue.FIsNamespaceDeclaration() )
      _ptccCopyCtxt->m_rgDeclarations.push_back( _ptccCopyCtxt->m_plvalContainerCur );
    else
    {
      Assert( rxnvwInValue.FIsNamespaceReference() );
      _ptccCopyCtxt->m_rgReferences.push_back( _ptccCopyCtxt->m_plvalContainerCur );
    }
  }

  unique_ptr< _TyLexUserObj > m_upUserObj; // The user object. Contains all entity references.
  _TyUriAndPrefixMap m_mapUris; // set of unqiue URIs.
  _TyUriAndPrefixMap m_mapPrefixes; // set of unique prefixes.
  _TyXMLDeclProperties m_XMLDeclProperties;
  // This isn't needed in all scenarios but essential to have it here when it is used.
  typedef optional< _TyXmlNamespaceMap > _TyOptXmlNamespaceMap;
  _TyOptXmlNamespaceMap m_optMapNamespaces;
  // This would only be populated in outputting situations.
  typedef optional< _TyPrFormatContext > _TyOptPrFormatContext;
  _TyOptPrFormatContext m_optprFormatContext;
  // This can be set and it will be used as the default attribute namespace for all declared attributes.
  _TyXmlNamespaceValueWrap m_xnvwDefaultAttributeNamespace;
};

// _xml_document_context_transport: This is a document context that includes the tranport type, etc. 
//  This isn't necessary nor desired for some instances of _xml_document_context so we will it out.
template < class t_TyXmlTraits >
class _xml_document_context_transport : protected _xml_document_context< typename t_TyXmlTraits::_TyLexUserObj >
{
  typedef _xml_document_context_transport _TyThis;
  typedef _xml_document_context< typename t_TyXmlTraits::_TyLexUserObj > _TyBase;
public:
  typedef t_TyXmlTraits _TyXmlTraits;
  typedef typename _TyXmlTraits::_TyTransport _TyTransport;
  using _TyBase::_TyBase; // This should work.
  _xml_document_context_transport( _xml_document_context_transport && ) = default;
  _xml_document_context_transport & operator =( _xml_document_context_transport && ) = default;
  void swap( _xml_document_context_transport & _r )
  {
    if ( &_r == this )
      return;
    _TyBase::swap( _r );
    m_opttpImpl.swap( _r.m_opttpImpl );
  }

  using _TyBase::m_upUserObj;
  using _TyBase::m_mapUris;
  using _TyBase::m_mapPrefixes;
  using _TyBase::m_XMLDeclProperties;
  // For some transports where the backing is mapped memory it is convenient to store the transport here because it
  //  allows the parser object to go away entirely.
  typedef optional< _TyTransport > _TyOptTransport;
  _TyOptTransport m_opttpImpl;
};

// _xml_document_context_transport_var: Variant version of the _xml_document_context_transport.
// We must allow a monostate since we want a default constructor.
template < class t_TyTpTransports >
class _xml_document_context_transport_var
{
  typedef _xml_document_context_transport_var _TyThis;
public:
  typedef t_TyTpTransports _TyTpTransports;
  typedef MultiplexTuplePack_t< TGetXmlTraitsDefault, _TyTpTransports > _TyTpXmlTraits;
  typedef MultiplexMonostateTuplePack_t< _xml_document_context_transport, _TyTpXmlTraits, variant > _TyVariant;

  ~_xml_document_context_transport_var() = default;
  _xml_document_context_transport_var() = default;
  _xml_document_context_transport_var( _xml_document_context_transport_var const & ) = delete;
  _xml_document_context_transport_var & operator =( _xml_document_context_transport_var const & ) = delete;
  _xml_document_context_transport_var( _xml_document_context_transport_var && ) = default;
  _xml_document_context_transport_var & operator =( _xml_document_context_transport_var && ) = default;
  void swap( _xml_document_context_transport_var & _r )
  {
    if ( &_r == this )
      return;
    m_var.swap( _r.m_var );
  }
  template < class t_TyXmlDocumentContext >
  t_TyXmlDocumentContext & emplace( t_TyXmlDocumentContext && _rrxdc )
  {
    using _TyRemoveRef = remove_reference_t< t_TyXmlDocumentContext >;
    return m_var.template emplace<_TyRemoveRef>( std::move( _rrxdc ) );
  }

  bool FIsNull() const
  {
    return FIsA< monostate >();
  }
  template < class t_TyT >
  bool FIsA() const
  {
    return holds_alternative< t_TyT >();
  }

protected:
  _TyVariant m_var;
};

__XMLP_END_NAMESPACE
