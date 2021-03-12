#pragma once

// xml_writer.h
// XML writer. Uses the defined productions to match supplied strings to ensure correctness.
// dbien
// 13FEB2021

// Design goals:
// 1) As with the xml_read_cursor we want to only store information on tags we *currently* know about - i.e. the ones
//    in the write(read) context list.
// 2) We will move the tag and content tags into this object and write them and then just copy the tag name and any 
//    namespace management objects found and leave copies in xml_read_cursor's copy so that normal operation occurs.
// 3) My thought is that this can pretty much be just modelled on how the xml_tag stuff peruses the xml_read_cursor.
//    But we aren't constructing a tree and just need to hold onto the current context list to the root.
// 4) The tags should be able to come either:
//    a) come straight from the xml_read_cursor(_var). In this case there is no need for validation of the data.
//       We assume that if the data is in the form of _l_data positions (or vtyDataPosition) in the _l_value then
//       it came from the parser and thus there is no need for output validation.
//    b) If the data isn't in _l_data or vtyDataPosition form in the _l_value - i.e. it has been written over with
//       some type of string then this data would need to be validated before converting to the output format. Note that
//       this data may need to be converted to be validated by the character's state machine.
//    b) come from the BeginTag() interface which returns an object that can be used to add attributes, etc, from
//       various sources, etc. as well as close the tag.
//    c) we should be able to "splice" in data from an xml_read_cursor(_var) and we want to be able to splice in
//       data from different character encodings from the character encoding we are outputing.
// 5) We want to output any encoding from any other encoding: The output encoding of the writer shouldn't depend
//    at all on the encoding of the source.
//    a) This will involve translating the source tokens in-place to a different character type which, given how we have done
//       things should be pretty easy. The positions within the sub-objects will be exactly the same they are just indexing
//       a different character type. That's not true at all, actually due to the presence of variable length characters.
//       We'll have to think about how to do this.         

__XMLP_BEGIN_NAMESPACE

// xml_write_context:
// There is a stack of write contexts corresponding to the current set of unclosed tags from root to current leaf.
template < class t_TyXmlTransportOut >
class xml_write_context
{
  typedef xml_write_context _TyThis;
public:
  typedef typename t_TyXmlTransportOut::_TyChar _TyChar;
  typedef xml_writer< t_TyXmlTransportOut > _TyXmlWriter;
  // We always used backed context transport contexts for our tokens in the writer. When translating tokens
  //  any transport context can be converted into a backed context.
  typedef _l_transport_backed_ctxt< _TyChar > _TyTransportCtxt;
  typedef xml_user_obj< _TyChar, vkfSupportDTD > _TyLexUserObj;
  typedef tuple< xml_namespace_value_wrap< _TyChar > > _TyTpValueTraitsPack;
  typedef xml_token< _TyTransportCtxt, _TyLexUserObj, _TyTpValueTraitsPack > _TyXmlToken;

  xml_write_context( _TyLexUserObj & _ruoUserObj, const _TyAxnObjBase * _paobCurToken )
    : m_xtkToken( _ruoUserObj, _paobCurToken )
  {
  }
  void SetDetectReferences( EDetectReferences _edrDetectReferences )
  {
    m_edrDetectReferences = _edrDetectReferences;
  }
  EDetectReferences GetDetectReferences() const
  {
    return m_edrDetectReferences;
  }
  const _TyXmlToken & GetToken() const
  {
    return m_xtkToken;
  }
  _TyXmlToken & GetToken()
  {
    return m_xtkToken;
  }
  // The data inside of m_xtkToken is complete - write it out to the output stream.
  // We could then delete the data - except for namespace declarations, but I don't see the
  //  need to preemptively delete since not too much memory is being taken up.
  // If _fHasContent is true then it is known that the tag has content and thus the tag end characters can be written.
  void CommitTagData( ECommitTagDisp ectdDisp = ectdContentUnknown )
  {
    if ( m_fCommitted )
      return; // no-op - valid to call more than once.
    m_pxwWriter->_CommitTag( m_xtkToken, m_edrDetectReferences, ectdDisp );
    m_fCommitted = true; // If we throw whilte committing we are not committed - even though it may not be fixable.
  }
  void EndTag()
  {
    CommitTagData( ectdNoContent );
    m_pxwWriter->EndTag( this );
  }
protected:
  _TyXmlWriter * m_pxwWriter;
  _TyXmlToken m_xtkToken; // This is always a tag token. It really doesn't matter if the id is s_knTokenSTag or s_knTokenEmptyElemTag.
  bool m_fCommitted{false}; // Committed yet?
  EDetectReferences m_edrDetectReferences{ edrDetectReferencesCount }; // User can override this inside of xml_write_tag.
};

// xml_write_tag:
// This is a wrapper which stores a references to a tag that has been started in the associated xml_writer<> object.
// When the lifetime of this object ends, the tag is ended, so act accordingly. The nice thing is that even in retail
//  we will throw if you do something wrong - like end a tag early.
// The Commit() method must be called on this tag before adding another tag. We will throw an error if Commit() hasn't
//  been called on the top tag on the stack when a new tag as been added.
// If Commit() hasn't been called when the xml_write_tag's lifetime is ended then Commit() is called automatically.
template < class t_TyXmlTransportOut >
class xml_write_tag
{
  typedef xml_write_tag _TyThis;
public:
  typedef t_TyXmlTransportOut _TyXmlTransportOut;
  typedef xml_writer< _TyXmlTransportOut > _TyWriter;
  typedef xml_write_context< _TyXmlTransportOut > _TyWriteContext;

  // This is our attribute value/name interface object:
  // 1) Namespaces might be declared for this tag - default or prefixed.
  // 2) Attribute/value pairs might be added to this tag.
  // 3) This tag may already be populated with data that came from an XML stream but now the user wants to modify the values.

  xml_write_tag( _TyWriteContext & _rwcxt )
    : m_pwcxt( &_rwcxt )
  {
  }
  ~xml_write_tag()
  {
    // We shouldn't throw out of this destructor if we are within an unwinding:
    if ( !!m_pwcxt )
    {
      bool fInUnwinding = !!std::uncaught_exceptions();
      try
      {
        EndTag();
      }
      catch ( std::exception const & )
      {
        if ( !fInUnwinding )
          throw; // let someone else deal with this.
        // we could log here - not much else to be done besides aborting and that's draconian.
      }
    }
  }

  // This causes the data contained within this tag to be written to the transport.
  // This method may only be called once.
  // Note that starting another tag under this one will also cause an implicit Commit() to occur ( as a shortcut ).
  // Note that writing a token to the when there is a non-committed tag causes an auto-commit.
  // Calling commit on a committed tag is not an error - it is merely ignored - this takes care of cases where
  //  the addition of a sub-tag or sub-token causes a commit.
  void Commit()
  {
    Assert( m_pwcxt );
    m_pwcxt->CommitTagData();
  }
  // End this tag. This is called from the destructor but may also be called directly.
  void EndTag()
  {
    if ( !!m_pwcxt )
    {
      _TyWriteContext * pwcxt = m_pwcxt;
      pwcxt->EndTag();
    }
  }
  // This determines how references that may be present in attribute handles are detected or not.
  void SetDetectReferences( EDetectReferences _edrDetectReferences )
  {
    m_pwcxt->SetDetectReferences( _edrDetectReferences );
  }
  EDetectReferences GetDetectReferences() const
  {
    return m_pwcxt->GetDetectReferences();
  }
  // Return if the tag has a namespace or not.
  bool FHasNamespace() const
  {
    Assert( m_pwcxt );
    return m_pwcxt->GetToken().FHasNamespace();
  }
  // Get a namespace reference from this tag - the one that is on the tag itself.
  _TyXmlNamespaceValueWrap GetNamespaceReference() const
  {
    Assert( FHasNamespace() ); // We will throw in the variant accessing the value wrap if it isn't there.
    return m_pwcxt->GetToken().GetNamespaceReference();
  }
  // Add the namespace and return a reference to it that can be used to apply it to attributes.
  // The namespace will only be declared as an attribute if it has to be. If the (uri)
  //  is the current namespace for (prefix) then there is no need to declare the namespace.
  template < class t_TyChar >
  _TyXmlNamespaceValueWrap AddNamespace( TGetPrefixUri< t_TyChar > const & _rpuNamespace )
  {
    return m_pwcxt->GetToken().AddNamespace( m_pwcxt->GetDocumentContext(), _rpuNamespace );
  }
  // Use this to set the default namespace for any added attributes.
  void SetDefaultAttributeNamespace( _TyXmlNamespaceValueWrap const & _rxnvw )
  {
    m_xnvwAttributes = _rxnvw.ShedReference();
  }
  void ClearDefaultAttributeNamespace()
  {
    m_xnvwAttributes.Clear();
  }
  template < class t_TyChar >
  void AddAttribute(  const t_TyChar * _pcAttrName, size_t _stLenAttrName = (numeric_limits< size_t >::max)(),
                      const t_TyChar * _pcAttrValue = nullptr, size_t _stLenAttrValue = (numeric_limits< size_t >::max)(),
                      const _TyXmlNamespaceValueWrap * _pxnvw = nullptr )
  {
    _CheckCommitted();
     m_pwcxt->GetToken().AddAttribute(  m_pwcxt->GetDocumentContext(), _pcAttrName, _stLenAttrName, _pcAttrValue, _stLenAttrValue, 
                                        _PGetDefaultAttributeNamespace( _pxnvw ) );
  }
  template < class t_TyStrViewOrString >
  void AddAttribute(  t_TyStrViewOrString const & _strName, t_TyStrViewOrString const & _strValue, const _TyXmlNamespaceValueWrap * _pxnvw = nullptr )
  {
    _CheckCommitted();
    m_pwcxt->GetToken().AddAttribute( m_pwcxt->GetDocumentContext(), _strName, _strValue, _PGetDefaultAttributeNamespace( _pxnvw ) );
  }
  // We support adding formatted attributes but only for char and wchar_t types. They will be interpreted as UTF-8 and UTF-16/32 depending on platform.
  template < class t_TyChar >
  void FormatAttribute( const t_TyChar * _pcAttrName, size_t _stLenAttrName = (numeric_limits< size_t >::max)(),
                        const t_TyChar * _pcAttrValueFmt = nullptr, size_t _stLenAttrValue = (numeric_limits< size_t >::max)(),
                        const _TyXmlNamespaceValueWrap * _pxnvw = nullptr, ... )
    requires( TAreSameSizeTypes_v< t_TyChar, char > || TAreSameSizeTypes_v< t_TyChar, wchar_t > )
  {
    _CheckCommitted();
    va_list ap;
    va_start( ap, _pxnvw );
    m_pwcxt->GetToken().FormatAttributeVArg( m_pwcxt->GetDocumentContext(), _pcAttrName, _stLenAttrName, 
      _pcAttrValueFmt, _stLenAttrValue, _PGetDefaultAttributeNamespace( _pxnvw ), ap );
    va_end( ap );
  }
  // A more succinct format that doesn't allow as many options.
  template < class t_TyChar >
  void FormatAttribute( const t_TyChar * _pszAttrName, const t_TyChar * _pszAttrValueFmt, ... )
    requires( TAreSameSizeTypes_v< t_TyChar, char > || TAreSameSizeTypes_v< t_TyChar, wchar_t > )
  {
    _CheckCommitted();
    va_list ap;
    va_start( ap, _pszAttrValueFmt );
    m_pwcxt->GetToken().FormatAttributeVArg( m_pwcxt->GetDocumentContext(), _pszAttrName, (numeric_limits< size_t >::max)(), 
      _pszAttrValueFmt, (numeric_limits< size_t >::max)(), _PGetDefaultAttributeNamespace( nullptr ), ap );
    va_end( ap );
  }
  template < class t_TyChar >
  void FormatAttribute( _TyXmlNamespaceValueWrap const & _rxnvw, const t_TyChar * _pszAttrName, const t_TyChar * _pszAttrValueFmt, ... )
    requires( TAreSameSizeTypes_v< t_TyChar, char > || TAreSameSizeTypes_v< t_TyChar, wchar_t > )
  {
    _CheckCommitted();
    Assert( !!&_rxnvw ); // shouldn't pass null ref - though we will not crash - but we will not honor any default attribute namespace that have been decalred.
    va_list ap;
    va_start( ap, _pszAttrValueFmt );
    m_pwcxt->GetToken().FormatAttributeVArg( m_pwcxt->GetDocumentContext(), _pszAttrName, (numeric_limits< size_t >::max)(), 
      _pszAttrValueFmt, (numeric_limits< size_t >::max)(), &_rxnvw, ap );
    va_end( ap );
  }
protected:
  void _CheckCommitted() const
  {
    VerifyThrowSz( !m_pwcxt->FCommited(), "Trying to edit a tag that has already been committed." );
  }
  const _TyXmlNamespaceValueWrap * _PGetDefaultAttributeNamespace( const _TyXmlNamespaceValueWrap * _pxnvw ) const
  {
    // The document-level default atttribute namespace is handled by xml_token.
    return !_pxnvw ? &m_xnvwAttributes : _pxnvw;
  }
  _TyWriteContext * m_pwcxt{nullptr}; // The context in the context stack to which this xml_write_tag corresponds.
  _TyXmlNamespaceValueWrap m_xnvwAttributes; // When this is set then it is used for marking any added attributes.
};

// xml_writer:
// Writes an XML file stream through an XML write transport.
// The encoding type of t_TyXmlTransportOut is independent of t_TyXmlTraits but we must specify it here to allow splicing, etc.,
//  into a file because we can't mix encoding within a single file so no reason to try to be more flexible than that.
template < class t_TyXmlTransportOut >
class xml_writer
{
  typedef xml_writer _TyThis;
public:
  typedef t_TyXmlTransportOut _TyXmlTransportOut;
  typedef typename _TyXmlTransportOut::_TyChar _TyChar;
  typedef xml_markup_traits< _TyChar > _TyMarkupTraits;
  typedef xml_write_tag< _TyChar > _TyXmlWriteTag;
  // We need a user object in order to be able to create tokens.
  typedef xml_user_obj< _TyChar, false > _TyLexUserObj;
  // We need to have our own local namespace map which means we need the full on document context:
  typedef _xml_document_context< _TyLexUserObj > _TyXmlDocumentContext;
  typedef typename _TyXmlDocumentContext::_TyPrFormatContext _TyPrFormatContext;
  typedef typename _xml_namespace_map_traits< _TyChar >::_TyNamespaceMap _TyNamespaceMap;
  typedef xml_write_context< _TyXmlTransportOut > _TyWriteContext;
  typedef typename _TyWriteContext::_TyXmlToken _TyXmlToken;
  typedef list< _TyWriteContext > _TyListWriteContexts;

  xml_writer()
  {
  }
  ~xml_writer() = default;

  // Open the given file for write in the given encoding.
  // This will also write the XMLDecl if we are to do so, and regardless it will create a
  //  write context stack containing a single element which is the XMLDecl element regardless
  //  if we are to write the element.
  void OpenFile( const char * _pszFileName, bool _fStandalone = true, const _xml_output_format * _pxofFormat = nullptr )
  {
    _TyPrFormatContext prFormatContext;
    if ( _pxofFormat )
      prFormatContext.first = *_pxofFormat;
    m_xdcxtDocumentContext.Init( _fStandalone, _TyXmlTransportOut::GetCharacterEncoding(), m_fUseNamespaces, &prFormatContext );
    m_strFileName = _pszFileName;
    FileObj foFile( CreateWriteOnlyFile( _pszFileName ) );
    VerifyThrowSz( foFile.FIsOpen(), "Unable to open file[%s]", _pszFileName );
    // emplace the transport - it will write the BOM or not:
    emplaceTransport( foFile, m_fWriteBOM );
    _Init(); // This might write the XMLDecl tag to the file, but definitely will init the context stack.
  }
  // Initialize the xml_writer. Write the XMLDecl root "tag" if that is desired. Regardless create a XMLDecl pseudo
  //  tag as the root of the context list.
  void _Init()
  {
    m_lContexts.clear();
    m_fWroteFirstTag = false;
    m_fHaveUnendedTag = false;
    if ( m_fWriteXMLDecl )
    {
      // Just write the XML declaration brute force - no reason to get fancy here. Note that this also includes
      //  ' version="1.0" encoding="'.
      _WriteTransportRaw( _TyMarkupTraits::s_kszXMLDeclBeginEtc, StaticStringLen( _TyMarkupTraits::s_kszXMLDeclBeginEtc ) );
      const _TyChar * pszEncoding = PszCharacterEncodingName< _TyChar >( _TyXmlTransportOut::GetCharacterEncoding() );
      _WriteTransportRaw( pszEncoding, StrNLen( pszEncoding ) );
      _WriteTransportRaw( _TyMarkupTraits::s_kszStandaloneEtc, StaticStringLen( _TyMarkupTraits::s_kszStandaloneEtc ) );
      if ( m_fStandalone )
        _WriteTransportRaw( _TyMarkupTraits::s_kszYes, StaticStringLen( _TyMarkupTraits::s_kszYes ) );
      else
        _WriteTransportRaw( _TyMarkupTraits::s_kszNo, StaticStringLen( _TyMarkupTraits::s_kszNo ) );
      _WriteTransportRaw( _TyMarkupTraits::s_kszXMLDeclEndEtc, StaticStringLen( _TyMarkupTraits::s_kszXMLDeclEndEtc ) );
    }
    m_lContexts.emplace_back( m_xdcxtDocumentContext.GetUserObj(), s_knTokenXMLDecl  ); // create the XMLDecl pseudo-tag.
  }
  template < template < class ... > class t_tempTyXmlTransportOut >
  void OpenFileVar( const char * _pszFileName, bool _fStandalone = true )
  {
  }

// options:
  void SetWriteBOM( bool _fWriteBOM )
  {
    VerifyThrowSz( ( m_fWriteBOM == _fWriteBOM ) || !FHasTransport(), "Mustn't change the use of namespaces after opening a transport." );
    m_fWriteBOM = _fWriteBOM;
  }
  void FGetWriteBOM()) const
  {
    return m_fWriteBOM;
  }
  void SetWriteXMLDecl( bool _fWriteXMLDecl )
  {
    VerifyThrowSz( ( m_fWriteXMLDecl == _fWriteXMLDecl ) || !FHasTransport(), "Mustn't change the use of namespaces after opening a transport." );
    m_fWriteXMLDecl = _fWriteXMLDecl;
  }
  void FGetWriteXMLDecl()) const
  {
    return m_fWriteXMLDecl;
  }
  void SetUseNamespaces( bool _fUseNamespaces )
  {
    VerifyThrowSz( ( m_fUseNamespaces == _fUseNamespaces ) || !FHasTransport(), "Mustn't change the use of namespaces after opening a transport." );
    m_fUseNamespaces = _fUseNamespaces;
  }
  void FGetUseNamespaces() const
  {
    return m_fUseNamespaces;
  }
// status:
  bool FIsOpen() const
  {
    return FHasTransport();
  }
  bool FHasTransport() const
  {
    return m_optTransportOut.has_value();
  }
  bool FInsideDocumentTag() const
  {
    return m_lContexts.size() > 1; // the claim is constant time since C++17.
  }
  bool FInProlog() const
  {
    return !m_fWroteFirstTag && ( 1 == m_lContexts.size() );
  }
  bool FInEpilog() const
  {
    return m_fWroteFirstTag && ( 1 == m_lContexts.size() );
  }
  // This will:
  // 1) Call FMoveDown on _xrc
  // 2) We must check and declare any namespace that we may encounter along the way as
  //    these may have been declared above the current position of _rxrc, but we will
  //    only know when we finally encounter a reference to them.
  template < class t_TyReadCursor >
  void WriteFromReadCursor( t_TyReadCursor & _rxrc )
  {
  }

  // Start the tag. If _ppvNamespace is non-null:
  // 1) Check to see if the (prefix,uri) happens to be current. If it isn't:
  //    a) If prefix isn't the empty string then it will be added to the front of _pszTagName. 
  //    b) The appropriate xmlns= will be added as an attribute in this case.
  // 2) If the prefix is current then we will still add it to the front of _pszTagName but we won't
  //    add the xmlns declaration.
  // 3) We don't validate the contents of the name or any prefix now we do that on output.  
  // 4) For convenience's sake the caller may add a prefix onto the beginning of _pszTagName and that
  //    prefix will be used if it is a current namespace prefix. If it isn't a current namespace prefix
  //    then an exception will be thrown.
  template < class t_TyChar >
  _TyXmlWriteTag StartTag( const t_TyChar * _pszTagName, size_t _stLenTag = 0, TGetPrefixUri< t_TyChar > const * _ppuNamespace = nullptr )
  {
    _CheckCommitCur();
    // Add a new tag as the top of the context.
    _TyWriteContext & rwcxNew = m_lContexts.emplace_back( m_xdcxtDocumentContext.GetUserObj(), s_knTokenSTag );
    rwcxNew.GetToken().SetTagName( m_xdcxtDocumentContext, _pszTagName, _stLenTag, _ppuNamespace );
    return _TyXmlWriteTag( rwcxNew );
  }
  // Once a namespace has been added you can then obtain its _TyXmlNamespaceValueWrap and use that to mark namespace on tags
  //  and attributes. This is much faster since no namespace lookup needs to be made.
  template < class t_TyChar >
  _TyXmlWriteTag StartTag( _TyXmlNamespaceValueWrap const & _rxnvw, const t_TyChar * _pszTagName, size_t _stLenTag = 0 )
  {
    _CheckCommitCur();
    // Add a new tag as the top of the context.
    _TyWriteContext & rwcxNew = m_lContexts.emplace_back( m_xdcxtDocumentContext.GetUserObj(), s_knTokenSTag );
    rwcxNew.GetToken().SetTagName( m_xdcxtDocumentContext, _rxnvw, _pszTagName, _stLenTag );
    return _TyXmlWriteTag( rwcxNew );
  }
  // Start a tag by copying the passed token.
  // If we namespace of the tag isn't current then the namespace is declared as an additional attribute.
  // We also copy current namespace declarations even if they are not referred to by the tag's namespace prefix.
  template < class t_TyXmlToken >
  _TyXmlWriteTag StartTag( t_TyXmlToken const & _rtok )
  {

  }
  // Start a tag by moving its contents into this object.
  // We leave the tag name intact in the old object and we must leave any active namespace declarations there and just copy them.
  _TyXmlWriteTag StartTag( _TyXmlToken && _rrtok )
  {
    
  }
  // We are writing a token - we are not writing the end tag for a token.
  void _CheckCommitCur()
  {
    _CheckWriteTagEnd( true );
    if ( !FInsideDocumentTag() )
      return;
    _TyWriteContext & rwcxCur = m_lContexts.back();
    // We know, if we are committing here, that we have content and thus can can write the tag end right away.
    rwcxCur.CommitTagData( ectdWithContent );
  }
  // Check to see if we have started a tag and if so end it allowing for an end tag.
  void _CheckWriteTagEnd( bool _fWithContent )
  {
    if ( m_fHaveUnendedTag )
    {
      m_fHaveUnendedTag = false;
      if ( _fWithContent )
        _WriteTransportRaw( _TyMarkupTraits::s_kszTagEnd, StaticStringLen( _TyMarkupTraits::s_kszTagEnd ) );
      else
        _WriteTransportRaw( _TyMarkupTraits::s_kszEmptyElemTagEnd, StaticStringLen( _TyMarkupTraits::s_kszEmptyElemTagEnd ) );
    }
  }

#if 0
  // Ends the current tag. If _ptokEnd is passed it needs to match the top of the write stack. If not an exception will be thrown.
  // The safest manner of operation is to pass _ptokEnd bask to check - but I don't see a reason to *require* it.
  void EndTag( _TyXmlWriteTag * _ptokEnd = nullptr )
  {

  }
#endif //0

  // Write a non-tag token to the output transport at the current position.
  // Validation is performed - e.g. only whitespace chardata is allowed before the first tag.
  // The token is passed the production to which it correspond's state machine to ensure that it matches.
  // If a non-match is found that isn't correctable with a CharRef, etc. then we will throw an error.
  template < class t_TyXmlToken >
  void WriteToken( t_TyXmlToken const & _rtok )
  {
    switch( _rtok.GetTokenId() )
    {
      case s_knTokenSTag:
      case s_knTokenETag:
      case s_knTokenEmptyElemTag:
      case s_knTokenXMLDecl: // This included as it is written automagically and shouldn't be written here.
        VerifyThrowSz( false, "WriteToken() is to be used to write any token except tag-related tokens." );
      break;
      case s_knTokenComment:
        _WriteComment( _rtok );
      break;
      case s_knTokenCDataSection:
        _WriteCDataSection( _rtok );
      break;
      case s_knTokenCharData:
      {
        // Must be a single value here:
        _CheckCommitCur();
        _WriteCharAndAttrData< TGetCharDataStart >( _rtok, _rtok.GetValue() );
      }
      break;
      case s_knTokenProcessingInstruction:
        _WriteProcessingInstruction( _rtok );
      break;
    }
  }
  template < class... t_TysArgs >
  _TyXmlTransportOut emplaceTransport( t_TysArgs&&... _args )
  {
    return m_optTransportOut.emplace( std::forward< t_TysArgs >(_args) ... );
  }
protected:
  template < class t_TyChar >
  void _WriteTransportRaw( t_TyChar const * _pcBegin, t_TyChar const * _pcEnd )
  {
    Assert( !!m_optTransportOut );
    Assert( _pcEnd >=_pcBegin );
    VerifyThrow( !!m_optTransportOut && ( _pcEnd >=_pcBegin ) );
    if ( _pcEnd == _pcBegin )
      return; // no-op.
    m_optTransportOut->Write( _pcBegin, _pcEnd );
  }
  template < class t_TyChar >
  void _WriteTransportRaw( t_TyChar const * _pcBegin, size_t _nchLen )
  {
    _WriteTransportRaw( _pcBegin, _pcBegin + _nchLen );
  }
  template < class t_TyToken >
  void _WriteTypedData( t_TyToken const & _rtok, typename t_TyToken::_TyValue const & _rval )
  {
    typedef basic_string_view< typename t_TyXmlToken::_TyChar > _TyStrView;
    if ( _rval.FEmptyTypedData() )
      return; // no-op.
    _TyStrView sv;
    _rtok.KGetStringView( rval, sv );
    _WriteTransportRaw( &sv[0], sv.length() );
  }
  // This just commits the start of the tag. And we can't write the last ">" or "/>" until
  //  we know if there will be sub-tag data at all. This is because one invariant is that we
  //  never seek the output file which allows us to write to stdout, etc.
  // We will set the variable m_fHaveUnendedTag to true to indicate that either a ">" or "/>"
  //  needs writing.
  // Validation:
  // 1) Tag and attribute names have already been validated as they were accumulated.
  // 2) Naemspaces were validated for activity at a given usage point, and tag
  //  declarations which already had a prefix are re-namespaced when that same prefix
  //  is redeclared on that tag. 
  // 3) Need to validate that there are no duplicate attribute declarations. This include unique prefixes using the same system.
  template < class t_TyXmlToken >
  void _CommitTag( t_TyXmlToken const & _rtok, EDetectReferences _edrDetectReferences, ECommitTagDisp ectdDisp )
  {
    _CheckWriteTagEnd( true ); // We might have an unended tag and since we are a sub tag then that tag has content.
    // We shouldn't need to validate the attributes if this token came from an xml_read_cursor - as it would have been validated on the way in.
    // We must check all attribute (name,value) pairs and if they are all typed data (i.e. not user added strings) then they all came from
    //  an xml_read_cursor and they needn't be validated.
    CheckDuplicateTokenAttrs( true, _rtok.GetLexToken(), true );
    _WriteTransportRaw( _TyMarkupTraits::s_kszTagBegin, StaticStringLen( _TyMarkupTraits::s_kszTagBegin ) );
    m_fHaveUnendedTag = ( ectdDisp == ectdContentUnknown );
    const _TyLexValue & rvalRoot = _rtok.GetValue();
    _WriteName( _rtok, rvalRoot[0] );
    // Move through all attributes writing each.
    const _TyLexValue & rrgAttrs = rvalRoot[1];
    size_t nAttrs = rrgAttrs.GetSize();
    rrgAttrs.GetValueArray().ApplyContiguous( 0, nAttrs,
      [this]( const _TyLexValue * _pvBegin, const _TyLexValue * _pvEnd )
      {
        for ( const _TyLexValue * pvCur = _pvBegin; _pvEnd != pvCur; ++pvCur )
        {
          _WriteTransportRaw( _TyMarkupTraits::s_kszSpace, StaticStringLen( _TyMarkupTraits::s_kszSpace ) );
          const _TyLexValue & rvCur = *pvCur;
          _WriteName( _rtok, rvCur );
          _WriteTransportRaw( _TyMarkupTraits::s_kszEqualSign, StaticStringLen( _TyMarkupTraits::s_kszEqualSign ) );
          // The tag itself specifies whether it wants double or single quotes.
          bool fUseDoubleQuote = rvCur[3].GetVal< bool >();
          _WriteTransportRaw( fUseDoubleQuote ? _TyMarkupTraits::s_kszDoubleQuote : _TyMarkupTraits::s_kszSingleQuote, StaticStringLen( _TyMarkupTraits::s_kszSingleQuote ) );
          // Now write the value using the appropriate start token as the validator.
          if ( fUseDoubleQuote )
            _WriteCharAndAttrData< TGetAttCharDataNoSingleQuoteStart >( _rtok, rvCur[2], _edrDetectReferences );
          else
            _WriteCharAndAttrData< TGetAttCharDataNoDoubleQuoteStart >( _rtok, rvCur[2], _edrDetectReferences );
          _WriteTransportRaw( fUseDoubleQuote ? _TyMarkupTraits::s_kszDoubleQuote : _TyMarkupTraits::s_kszSingleQuote, StaticStringLen( _TyMarkupTraits::s_kszSingleQuote ) );
        }
      }
    );
    // Write the ending if we know we have content.
    if ( !m_fHaveUnendedTag )
    {
      Assert( ( ectdDisp == ectdWithContent ) || ( ectdDisp == ectdNoContent ) );
      if ( ectdDisp == ectdWithContent )
        _WriteTransportRaw( _TyMarkupTraits::s_kszTagEnd, StaticStringLen( _TyMarkupTraits::s_kszTagEnd ) );
      else
        _WriteTransportRaw( _TyMarkupTraits::s_kszEmptyElemTagEnd, StaticStringLen( _TyMarkupTraits::s_kszEmptyElemTagEnd ) );
    }
    // otherwise we don't write any ending since we don't know yet which ending we should write.
  }
  // Write the name - it will have already been validated.
  // _rvalRgName is a value array containing at least (name,namespacewrap). 
  // It may contains any number of things beyond that and they don't figure into this method.
  template < class t_TyXmlToken >
  void _WriteName( t_TyXmlToken const & _rtok, typename t_TyXmlToken::_TyLexValue const & _rvalRgName )
  {
    typedef t_TyXmlToken _TyXmlToken;
    typedef typename _TyXmlToken::_TyLexValue _TyLexValue;
    typedef typename _TyXmlToken::_TyChar _TyChar;
    typedef xml_namespace_value_wrap< _TyChar > _TyXmlNamespaceValueWrap;
    typedef basic_string_view< _TyChar > _TyStrView;
    const _TyLexValue & rvalName = _rvalRgName[0];
    const _TyLexValue & rvalNS = _rvalRgName[1];
    if ( !rvalNS.FIsBool() && !m_xdcxtDocumentContext.FIncludePrefixesInAttrNames() )
    {
      _TyXmlNamespaceValueWrap & rxnvw = rvalNS.GetVal< _TyXmlNamespaceValueWrap >();
      _TyStrView svPrefix = rxnvw.RStringPrefix();
      _WriteTransportRaw( &svPrefix[0], svPrefix.length() );
      _WriteTransportRaw( _TyMarkupTraits::s_kszTagColon, StaticStringLen( _TyMarkupTraits::s_kszTagColon ) );
    }
    // Now write the name.
    if ( !rvalName.FHasTypedData() )
    {
      rvalName.ApplyString( 
        [this]< typename t_TyChar >( const t_TyChar * _pcBegin, const t_TyChar * const _pcEnd )
        {
          _WriteTransportRaw( _pcBegin, _pcEnd );
        }
      );
    }
    else
      _WriteTypedData( _rtok, rvalName );
  }

  template < class t_TyXmlToken >
  void _WriteComment( t_TyXmlToken const & _rtok )
  {
    _CheckCommitCur();
    typedef typename t_TyXmlToken::_TyLexValue _TyLexValue;
    _WriteTransportRaw( _TyMarkupTraits::s_kszCommentBegin, StaticStringLen( _TyMarkupTraits::s_kszCommentBegin ) );
    // Scenarios:
    // 1) We do everything in the character type of t_TyXmlToken. There is no reason to convert because we can just convert on output and that doesn't require a buffer.
    // 2) Validate if necessary and then write.
    // 3) If we came from an xml_read_cursor: We will have positions and not a string in the _l_value object. In this case validation is not necessary as it was validated on input.
    // For a comment we should see a single, non-empty, _l_value element. If it is empty we will throw because that is not a valid comment.
    const _TyLexValue & rval = rtok.GetValue();
    // We should see either a single data range here or some type of string.
    VerifyThrow( rval.FHasTypedData() || rval.FIsString() );
    if ( !rval.FHasTypedData() ) // We assume that if we have data positions then it came from an xml_read_cursor and thereby doesn't need validation.
    {
      // Then we need to validate the string in whatever character type it is in - don't matter none.
      rval.ApplyString( 
        [this]< typename t_TyChar >( const t_TyChar * _pcBegin, const t_TyChar * const _pcEnd )
        {
          // No anti-accepting states in the the CommentChar production - no need to obtain the accept state. No fancy processing for comments.
          VerifyThrowSz( _pcEnd == _l_match< t_TyChar >::PszMatch( PspGetCommentCharsStart< t_TyChar >(), _pcBegin, ( _pcEnd - _pcBegin ) ), 
            "Invalid comment - did you put two dashes in a row?" );
          // While we are here we might as well write the data to the output transport.
          _WriteTransportRaw( _pcBegin, _pcEnd );
        }
      );
    }
    else
    {
      _WriteTypedData( _rtok, rval );
    }
    _WriteTransportRaw( _TyMarkupTraits::s_kszCommentEnd, StaticStringLen( _TyMarkupTraits::s_kszCommentEnd ) );
  }
  template < template < class t_TyChar > t_tempGetStartToken, class t_TyXmlToken >
  void _WritePIPortion( t_TyXmlToken const & _rtok, typename t_TyToken::_TyValue const & _rval )
  {
    VerifyThrow( _rval.FHasTypedData() || _rval.FIsString() );
    if ( _rval.FHasTypedData() )
      _WriteTypedData( _rtok, _rval );
    else
    {      
      _rval.ApplyString( 
        [this]< typename t_TyChar >( const t_TyChar * _pcBegin, const t_TyChar * const _pcEnd )
        {
          const _TyStateProto * const kpspValidateTokenStart = t_tempGetStartToken< t_TyChar >::s_kpspStart;
          const bool kfIsPITargetMeat = ( kpspValidateTokenStart == PspGetPITargetMeatStart< t_TyChar >() );
          // In this case there is no remedy to any encountered anti-accept state, but we want to know if we hit one
          //  so we can give a more informative error message.
          const _TyStateProto * pspAccept;
          const t_TyChar * pcMatch = _l_match< t_TyChar >::PszMatch( kpspValidateTokenStart, _pcBegin, _pcEnd - _pcBegin, &pspAccept );
          VerifyThrowSz( !( !pspAccept || pspAccept->IsAntiAcceptingState() || ( pcMatch != _pcEnd ) ), 
            kfIsPITargetMeat ? "Invalid characters found in PITargetMeat." : "Invalid characters found in PITarget." );
          _WriteTransportRaw( _pcBegin, _pcEnd );
        }
    }
  }
  template < class t_TyXmlToken >
  void _WriteProcessingInstruction( t_TyXmlToken const & _rtok )
  {
    _CheckCommitCur();
    typedef typename t_TyXmlToken::_TyLexValue _TyLexValue;
    _WriteTransportRaw( _TyMarkupTraits::s_kszProcessingInstructionBegin, StaticStringLen( _TyMarkupTraits::s_kszProcessingInstructionBegin ) );
    const _TyLexValue & rval = rtok.GetValue();
    VerifyThrow( rval.FIsArray() );
    // We may have just a PITarget or a PITarget + PITargetMeat.
    _WritePIPortion< TGetPITargetStart >( _rtok, rval[0] );
    if ( ( rval.GetSize() > 1 ) && !rval[1].FEmptyTypedData() )
    {
      _WriteTransportRaw( _TyMarkupTraits::s_kszSpace, StaticStringLen( _TyMarkupTraits::s_kszSpace ) );
      _WritePIPortion< TGetPITargetMeatStart >( _rtok, rval[0] );
    }
    _WriteTransportRaw( _TyMarkupTraits::s_kszProcessingInstructionEnd, StaticStringLen( _TyMarkupTraits::s_kszProcessingInstructionEnd ) );
  }
  template < class t_TyXmlToken >
  void _WriteCDataSection( t_TyXmlToken const & _rtok )
  {
    _CheckCommitCur();
    typedef typename t_TyXmlToken::_TyLexValue _TyLexValue;
    _WriteTransportRaw( _TyMarkupTraits::s_kszCDataSectionBegin, StaticStringLen( _TyMarkupTraits::s_kszCDataSectionBegin ) );
    // If we find illegal characters within a CDATA section then there is no remedy. (Well we could end the CDataSection and put in a CharRef, but we aren't doing that now.)
    // If we find a "]]>" embedded in the CDataSection then we will appropriately escape it with an overlapping CDataSection. I.e.: "<![CDATA[]]]]><![CDATA[>]]>".
    const _TyLexValue & rval = rtok.GetValue();
    // We should see either a single data range here or some type of string.
    VerifyThrow( rval.FHasTypedData() || rval.FIsString() );
    if ( !rval.FHasTypedData() ) // We assume that if we have data positions then it came from an xml_read_cursor and thereby doesn't need validation.
    {
      // Then we need to validate the string in whatever character type it is in - don't matter none.
      rval.ApplyString( 
        [this]< typename t_TyChar >( const t_TyChar * _pcBegin, const t_TyChar * const _pcEnd )
        {
          // Move through and appropriately replace each found "]]>" with "]]]]><![CDATA[>", writing as we go.
          // Since we are using an anti-accepting state in the CDCharsOutputValidate production we need to examine the returned state for anti-accepting.
          const _TyStateProto * pspCDCharsOutputValidate = PspGetCDCharsOutputValidateStart< t_TyChar >();
          for ( const t_TyChar * pcCur = _pcBegin; _pcEnd != pcCur;  )
          {
            const _TyStateProto * pspAccept;
            const t_TyChar * pcMatch = _l_match< t_TyChar >::PszMatch( pspCDCharsOutputValidate, pcCur, _pcEnd - pcCur, &pspAccept );
            // If we find an accepting state that doesn't complete the entire remainder of characters then we are at a failure point:
            //  there are no transitions from this final accepting state.
            // However if we find an anti-accepting state then that means we have encountered an embedded "]]>" and we can appropriately escape.
            VerifyThrowSz( !( !pcMatch || ( !pspAccept->FIsAntiAcceptingState() && ( pcMatch != _pcEnd ) ) ),
              "Invalid characters found inside of a CDataSection." );
            // Write what we can. We should be pointing just after the "]]>" so we should be able to write all the way to pcMatch-1:
            if ( pcMatch == _pcEnd ) // most likely occurence
            {
              Assert( !pspAccept->FIsAntiAcceptingState() );
              _WriteTransportRaw( pcCur, _pcEnd );
              return; // all done.
            }
            Assert( ( pcMatch[-3] == t_TyChar(']') ) && ( pcMatch[-2] == t_TyChar(']') ) && ( pcMatch[-1] == t_TyChar('>') ) );
            _WriteTransportRaw( pcCur, pcMatch-1 );
            pcCur = pcMatch - 1;
            _WriteTransportRaw( _TyMarkupTraits::s_kszCDataSectionReplaceEnd, StaticStringLen( _TyMarkupTraits::s_kszCDataSectionReplaceEnd ) );
          }
        }
      );
    }
    else
    {
      _WriteTypedData( _rtok, rval );
    }
    _WriteTransportRaw( _TyMarkupTraits::s_kszCDataSectionEnd, StaticStringLen( _TyMarkupTraits::s_kszCDataSectionEnd ) );
  }
  // Three possibilities:
  // 1) _rtok contains typed data. In this case we won't validate at all and _edrDetectReferences DOES NOT come into play.
  // 2) _rtok contains a single string. In this _edrDetectReferences is used to validate and segment the string, etc.
  //    During validation, in addition to the the functionality of _edrDetectReferences below, we will substitute:
  //    a) '<' is escaped with "&lt;".
  //    b) '"' is escapted with "&quot;".
  //    c) ''' is escapted with "&apos;".
  //    d) Also if an anti-accepting state if found then it indicates that we found "]]>" within CharData
  //       and we will escape the '>' with "&gt;".
  // 3) If we haven't written the first tag then the only possibility is that this is a CharData tag.
  //    In this case we must validate that the tag contains only spaces.
  // _edrDetectReferences:
  // edrNoReferences: then any embedded '&' is escaped with "&amp;"
  // edrAlwaysReference: then any embedded '&' *must* correspond to a valid Reference of some sort.
  // edrAutoReference/edrAutoReferenceNoError: try to detect if we should replace with "&amp;" or not:
  //  1) If we see what looks like a CharRef (which is pretty obvious) then we will validate it.
  //  2) If we see what looks like just an EntityRef then we check if it is in the entity ref map:
  //     a) If it isn't in the entity map and edrAutoReferenceNoError then we will replace the leading '&' with "&amp;"
  //     b) If it isn't in the enity map and edrAutoReference then we will throw an error for a missing reference.
  // _rval: This is the value that we are currently writing. This may be one of the attribute values of a tag or chardata.
  // t_tempGetStartToken: This is one of TGetCharDataStart<>, TGetAttCharDataNoSingleQuoteStart<>, TGetAttCharDataNoDoubleQuoteStart<>.
  template < template < class t_TyChar > class t_tempGetStartToken, class t_TyXmlToken >
  void _WriteCharAndAttrData( t_TyXmlToken const & _rtok, typename t_TyXmlToken::_TyLexValue const & _rval, EDetectReferences _edrDetectReferences = edrDetectReferencesCount )
  {
    typedef typename t_TyXmlToken::_TyLexValue _TyLexValue;
    if ( _edrDetectReferences == edrDetectReferencesCount )
      _edrDetectReferences = m_edrDefaultDetectReferences; // use default which the user can set.
    // We should see either a single data range here or some type of string.
    VerifyThrow( _rval.FHasTypedData() || _rval.FIsString() );
    if ( !_rval.FHasTypedData() ) // We assume that if we have data positions then it came from an xml_read_cursor and thereby doesn't need validation.
    {
      // Then we need to validate the string in whatever character type it is in - don't matter none.
      _rval.ApplyString( 
        [this]< typename t_TyChar >( const t_TyChar * _pcBegin, const t_TyChar * const _pcEnd )
        {
          const _TyStateProto * const kpspValidateTokenStart = t_tempGetStartToken< t_TyChar >::s_kpspStart;
          if ( !m_fWroteFirstTag && ( kpspValidateTokenStart == PspGetCharDataStart< t_TyChar >() ) )
          {
            // CharData before the first tag should match the S production.
            const t_TyChar * pcMatchSpace = _l_match< t_TyChar >::PszMatch( PspGetSpacesStart< t_TyChar >(), pcCur, _pcEnd - pcCur );
            VerifyThrowSz( pcMatchSpace == _pcEnd, "CharData before the first tag much be composed entirely of spaces." );
            _WriteTransportRaw( _pcBegin, _pcEnd );
            return;
          }
          const _TyStateProto * const kpspAllReferences = PspGetAllReferencesStart< t_TyChar >();
          // Move through and validate as we have described above.
          // Since we are using an anti-accepting state in the CharData production we need to examine the returned state for anti-accepting.
          for ( const t_TyChar * pcCur = _pcBegin; _pcEnd != pcCur;  )
          {
#if ASSERTSENABLED
            const t_TyChar * dbg_pcAtBlockStart = pcCur;
#endif //ASSERTSENABLED
            const _TyStateProto * pspAccept;
            const t_TyChar * pcMatch = _l_match< t_TyChar >::PszMatch( kpspValidateTokenStart, pcCur, _pcEnd - pcCur, &pspAccept );
            // If we find an accepting state that doesn't complete the entire remainder of characters then we:
            // 1) Might be at the start of a reference.
            // 2) Might be at a single or double quote.
            // However if we find an anti-accepting state then that means we have encountered an embedded "]]>" and we can appropriately escape.
            if ( !pspAccept->FIsAntiAcceptingState() ) // Most common thing to happen is this.
            {
              Assert( ( pcMatch[-3] == t_TyChar(']') ) && ( pcMatch[-2] == t_TyChar(']') ) && ( pcMatch[-1] == t_TyChar('>') ) );
              _WriteTransportRaw( pcCur, pcMatch-1 );
              pcCur = pcMatch - 1;
              _WriteTransportRaw( _TyMarkupTraits::s_kszEntityGreaterThan, StaticStringLen( _TyMarkupTraits::s_kszEntityGreaterThan ) );
            }
            else
            {
              // In this case we should be pointing at an invalid character:
              Assert( ( *pcMatch == t_TyChar('\'') ) || ( *pcMatch == t_TyChar('\"') ) || ( *pcMatch == t_TyChar('<') ) );
              _WriteTransportRaw( pcCur, pcMatch );
              pcCur = pcMatch;
              switch( *pcCur++ )
              {
                default:
                  Assert( false ); // no reason to throw because if things are correct we cannot get here.
                break;
                case t_TyChar('\''):
                  _WriteTransportRaw( _TyMarkupTraits::s_kszEntityApostrophe, StaticStringLen( _TyMarkupTraits::s_kszEntityApostrophe ) );
                break;
                case t_TyChar('\"'):
                  _WriteTransportRaw( _TyMarkupTraits::s_kszEntityDoubleQuote, StaticStringLen( _TyMarkupTraits::s_kszEntityDoubleQuote ) );
                break;
                case t_TyChar('&'):
                {
                  // Reference. _edrDetectReferences now comes into play:
                  if ( edrNoReferences == _edrDetectReferences )
                  {
                    _WriteTransportRaw( _TyMarkupTraits::s_kszEntityAmpersand, StaticStringLen( _TyMarkupTraits::s_kszEntityAmpersand ) );
                  }
                  else
                  {
                    // We might have a reference (at least) in this case - figure out what we have:
                    const t_TyChar * pcMatchReference = _l_match< t_TyChar >::PszMatch( kpspAllReferences, pcCur-1, _pcEnd - ( pcCur - 1 ), &pspAccept );
                    VerifyThrowSz( pspAccept || ( edrAlwaysReference != _edrDetectReferences ), "Found a & that didn't match the Reference production." );
                    if ( !pspAccept )
                    {
                      Assert( ( edrAutoReference == _edrDetectReferences ) || ( edrAutoReferenceNoError == _edrDetectReferences ) )
                      // Then this doesn't match the Reference production and we are on "auto" - just replace with an amp:
                      _WriteTransportRaw( _TyMarkupTraits::s_kszEntityAmpersand, StaticStringLen( _TyMarkupTraits::s_kszEntityAmpersand ) );
                    }
                    else
                    {
                      typedef basic_string_view< t_TyChar > _TyStrView;
                      typedef basic_string< t_TyChar > _TyString;
                      switch( pspAccept->m_tidAccept )
                      {
                        case s_knTokenEntityRef:
                        {
                          // Must be a valid reference:
                          _TyStrView svRef( pcCur, pcMatchReference - 1 );
                          const _TyMapEntities & rmapEntities = GetEntityMap();
                          bool fFoundEntityReference = ( rmapEntities.end() != rmapEntities.find( svRef ) );
                          VerifyThrowSz( fFoundEntityReference || ( edrAutoReferenceNoError == _edrDetectReferences ), "Entity reference to [%s] not found.", StrConvertString<char>( svRef ).c_str() );
                          if ( fFoundEntityReference )
                          { // write it out.
                            _WriteTransportRaw( pcCur - 1, pcMatchReference );
                          }
                          else
                          {
                            // escape the bogus entity reference:
                            _WriteTransportRaw( _TyMarkupTraits::s_kszEntityAmpersand, StaticStringLen( _TyMarkupTraits::s_kszEntityAmpersand ) );
                            _WriteTransportRaw( pcCur, pcMatchReference );
                          }
                          pcCur = pcMatchReference;
                        }
                        break;
                        case s_knTokenCharRefDec:
                        case s_knTokenCharRefHex:
                        {
                          // The production ensures we have a valid set of characters, we must validate overflow.
                          // Character references are always in UTF32.
                          _TyStrView svRef( pcCur + ( pspAccept->m_tidAccept == s_knTokenCharRefDec ? 1 : 2 ), pcMatchReference - 1 );
                          [[maybe_unused]] char32_t utf32;
                          int iResult = IReadPositiveNum( s_knTokenCharRefDec == pspAccept->m_tidAccept ? 10 : 16, &svRef[0], svRef.length(), utf32, _l_char_type_map<char32_t>::ms_kcMax, false );
                          if ( !!iResult && ( edrAutoReferenceNoError != _edrDetectReferences ) ) // error - should be overflow since the production validates content.
                          {
                            Assert( vkerrOverflow == GetLastErrNo() );
                            THROWXMLPARSEEXCEPTIONERRNO( GetLastErrNo(), "Error processing character reference [%s] - very likely overflow." );
                          }
                          if ( !iResult )
                            _WriteTransportRaw( pcCur - 1, pcMatchReference );
                          else
                          {
                            // escape the bogus character reference:
                            _WriteTransportRaw( _TyMarkupTraits::s_kszEntityAmpersand, StaticStringLen( _TyMarkupTraits::s_kszEntityAmpersand ) );
                            _WriteTransportRaw( pcCur, pcMatchReference );
                          }
                          pcCur = pcMatchReference;
                        }
                        break;
                      }
                    }
                  }
                }
                break;
              }
            }
            Assert( pcCur != dbg_pcAtBlockStart ); // Must advance pcCur during this loop.
          }
        }
      );
    }
    else
    {
      // In this case we may have rich data in the typed data - i.e. a series of CharData and interspersed references.
      // We will peruse and write out according to the sub-tokens present in the typed data. We will have to augment with
      //  sub-token (trigger) markup as we go.
      typedef basic_string_view< typename t_TyXmlToken::_TyChar > _TyStrView;
      _TyData const & krdtCharData = _rval.GetTypedData();
      krdtCharData.Apply(
        [this,&_rtok]( const _TyData * _pdtrBegin, const _TyData * _pdtrEnd )
        {
          for ( const _l_data_typed_range * pdtrCur = _pdtrBegin; _pdtrEnd != pdtrCur; ++pdtrCur )
          {
            // We could use type() or id() here - but the type is more flexible.
            switch( pdtrCur->type() )
            {
              case s_kdtPlainText:
              {
                _TyStrView svPlain;
                _rtok.KGetStringView( *pdtrCur, svPlain );
                _WriteTransportRaw( &svPlain[0], svPlain.length() );
              }
              break;
              case s_kdtCharDecRef:
              {
                _WriteTransportRaw( _TyMarkupTraits::s_kszCharDecRefBegin, StaticStringLen( _TyMarkupTraits::s_kszCharDecRefBegin ) );
                _TyStrView svCharDecRef;
                _rtok.KGetStringView( *pdtrCur, svCharDecRef );
                _WriteTransportRaw( &svCharDecRef[0], svCharDecRef.length() );
                _WriteTransportRaw( _TyMarkupTraits::s_kszReferenceEnd, StaticStringLen( _TyMarkupTraits::s_kszReferenceEnd ) );
              }
              break;
              case s_kdtCharHexRef:
              {
                _WriteTransportRaw( _TyMarkupTraits::s_kszCharHexRefBegin, StaticStringLen( _TyMarkupTraits::s_kszCharHexRefBegin ) );
                _TyStrView svCharHexRef;
                _rtok.KGetStringView( *pdtrCur, svCharHexRef );
                _WriteTransportRaw( &svCharHexRef[0], svCharHexRef.length() );
                _WriteTransportRaw( _TyMarkupTraits::s_kszReferenceEnd, StaticStringLen( _TyMarkupTraits::s_kszReferenceEnd ) );
              }
              break;
              case s_kdtEntityRef:
              {
                _WriteTransportRaw( _TyMarkupTraits::s_kszEntityRefRefBegin, StaticStringLen( _TyMarkupTraits::s_kszEntityRefRefBegin ) );
                _TyStrView svEntityRef;
                _rtok.KGetStringView( *pdtrCur, svEntityRef );
                _WriteTransportRaw( &svEntityRef[0], svEntityRef.length() );
                _WriteTransportRaw( _TyMarkupTraits::s_kszReferenceEnd, StaticStringLen( _TyMarkupTraits::s_kszReferenceEnd ) );
              }
              break;
              default:
              {
                VerifyThrowSz( false, "Invalid data type()[%u]", pdtrCur->type() );
              }
            }
          }
        }
      );
    }
  }
  typedef optional< _TyXmlTransportOut > _TyOptTransportOut;
  _TyOptTransportOut m_optTransportOut;
  _TyXmlDocumentContext m_xdcxtDocumentContext;
  string m_strFileName; // Save this since it is cheap - might be enpty.
  _TyListWriteContexts m_lContexts;
// options:
  bool m_fWriteBOM{true}; // Write a BOM because that's just a nice thing to do.
  bool m_fWriteXMLDecl{true}; // Whether to write an XMLDecl. This is honored in all scenarios.
  bool m_fUseNamespaces{true}; // Using namespaces?
// state:
  
  bool m_fWroteFirstTag{false};
};

// xml_writer_var:
// Variant writer. This supports multiple character types (potentially). 
// Once again not sure that this is necessary, but potentially just as a receiver for xml_cursor_var - instant adaptor.
template <  template < class ... > class t_tempTyTransport, class t_TyTp2DCharPack >
class xml_writer_var
{
public:
};




__XMLP_END_NAMESPACE
