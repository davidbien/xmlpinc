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
  // We always used backed context transport contexts for our tokens in the writer. We probably won't fill them ever, but could do so.
  typedef _l_transport_backed_ctxt< _TyChar > _TyTransportCtxt;
  typedef xml_user_obj< _TyChar, vkfSupportDTD > _TyUserObj;
  typedef tuple< xml_namespace_value_wrap< _TyChar > > _TyTpValueTraitsPack;
  typedef xml_token< _TyTransportCtxt, _TyUserObj, _TyTpValueTraitsPack > _TyXmlToken;

  // The data inside of m_xtkToken is complete - write it out to the output stream.
  // We could then delete the data - except for namespace declarations, but I don't see the
  //  need to preemptively delete since not too much memory is being taken up.
  void CommitTagData()
  {
    VerifyThrowSz( !m_fCommitted, "Can only CommitTagData() once per tag." );
    // 
    m_fCommitted = true;
  }


protected:
  _TyXmlWriter * m_pxwWriter;
  _TyXmlToken m_xtkToken;
  bool m_fCommitted{false}; // Committed yet?

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
  typedef xml_writer< t_TyXmlTransportOut > _TyWriter;

  // This is our attribute value/name interface object:
  // 1) Namespaces might be declared for this tag - default or prefixed.
  // 2) Attribute/value pairs might be added to this tag.
  // 3) This tag may already be populated with data that came from an XML stream but now the user wants to modify the values.

  // This causes the data contained within this tag to be written to the transport.
  // This method may only be called once.
  void Commit()
  {
    m_pwcxtContext->CommitTagData();
  }

protected:
  _TyWriterContext * m_pwcxtContext{nullptr}; // The context in the context stack to which this xml_write_tag corresponds.
};

// TGetPrefixUri (prefix,URI).
template < class t_TyChar >
using TGetPrefixUri = std::pair< basic_string< t_TyChar >, basic_string< t_TyChar > >;

template < class t_TyChar >
const _l_state_proto< t_TyChar > * PspGetCommentCharsStart()
  requires( is_same_v< t_TyChar, char32_t > )
{
  return (const _TyStateProto *)&startUTF32CommentChars;
}
template < class t_TyChar >
const _l_state_proto< t_TyChar > * PspGetCommentCharsStart()
  requires( is_same_v< t_TyChar, char16_t > )
{
  return (const _TyStateProto *)&startUTF16CommentChars;
}
template < class t_TyChar >
const _l_state_proto< t_TyChar > * PspGetCommentCharsStart()
  requires( is_same_v< t_TyChar, char8_t > )
{
  return (const _TyStateProto *)&startUTF8CommentChars;
}
template < class t_TyChar >
const _l_state_proto< t_TyChar > * PspGetCDCharsOutputValidateStart()
  requires( is_same_v< t_TyChar, char32_t > )
{
  return (const _TyStateProto *)&startUTF32CDCharsOutputValidate;
}
template < class t_TyChar >
const _l_state_proto< t_TyChar > * PspGetCDCharsOutputValidateStart()
  requires( is_same_v< t_TyChar, char16_t > )
{
  return (const _TyStateProto *)&startUTF16CDCharsOutputValidate;
}
template < class t_TyChar >
const _l_state_proto< t_TyChar > * PspGetCDCharsOutputValidateStart()
  requires( is_same_v< t_TyChar, char8_t > )
{
  return (const _TyStateProto *)&startUTF8CDCharsOutputValidate;
}


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
  typedef xml_user_obj< _TyChar, false > _TyUserObj;
  // We need to have our own local namespace map which means we need the full on document context:
  typedef _xml_document_context< _TyUserObj > _TyXmlDocumentContext;
  typedef typename _xml_namespace_map_traits< _TyChar >::_TyNamespaceMap _TyNamespaceMap;

  // Open the given file for write in the given encoding.
  // This will write the BOM if we are to do so, but nothing else until another action is performed.
  void OpenFile( const char * _pszFileName )
  {
    
  }
  template < template < class ... > class t_tempTyXmlTransportOut >
  void OpenFileVar( const char * _pszFileName )
  {

  }


  // Write starting at the current tag of the read cursor.
  // If this is the XMLDecl tag then the XMLDecl tag then we either:
  // 1) Write the XMLDecl tag is nothing at all has been written to this writer yet.
  // 2) Skip the XMLDecl tag if we are splicing into the middle of an existing - a writer state for which the XMLDecl tag would have already written.
  template < class t_TyReadCursor >
  void WriteFromReadCursor( t_TyReadCursor & _rxrc )
  {
    // A few things:
    // 1) Figure out how to write the XMLDecl appropriately - or how to skip it if it is present and we don't want to write it.
    // 2) 
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
  _TyXmlWriteTag StartTag( const t_TyChar * _pszTagName, size_t _stLenTag = (numeric_limits<size_t>::max)(), TGetPrefixUri< t_TyChar > const * _ppuNamespace = nullptr )
  {

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
  template < class t_tyXmlToken >
  void WriteToken( t_tyXmlToken const & _rtok )
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
        _WriteCharData( _rtok );
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
  template < class t_tyXmlToken >
  void _WriteComment( t_tyXmlToken const & _rtok )
  {
    typedef typename t_tyXmlToken::_TyLexValue _TyLexValue;
    _WriteTransportRaw( _TyMarkupTraits::s_kszCommentBegin, DimensionOf( _TyMarkupTraits::s_kszCommentBegin ) );
    // Scenarios:
    // 1) We do everything in the character type of t_tyXmlToken. There is no reason to convert because we can just convert on output and that doesn't require a buffer.
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
      // Get a string view in the current token's character type and write that to the output transport.
      typedef basic_string_view< typename t_TyXmlToken::_TyChar > _TyStrView;
      _TyStrView svComment;
      _rtok.KGetStringView( rval, svComment );
      _WriteTransportRaw( &svComment[0], svComment.length() );
    }
    _WriteTransportRaw( _TyMarkupTraits::s_kszCommentEnd, DimensionOf( _TyMarkupTraits::s_kszCommentEnd ) );
  }
  template < class t_tyXmlToken >
  void _WriteCDataSection( t_tyXmlToken const & _rtok )
  {
    typedef typename t_tyXmlToken::_TyLexValue _TyLexValue;
    _WriteTransportRaw( _TyMarkupTraits::s_kszCDataSectionBegin, DimensionOf( _TyMarkupTraits::s_kszCDataSectionBegin ) );
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
            _WriteTransportRaw( _TyMarkupTraits::s_kszCDataSectionReplaceEnd, DimensionOf( _TyMarkupTraits::s_kszCDataSectionReplaceEnd ) );
          }
        }
      );
    }
    else
    {
      // Get a string view in the current token's character type and write that to the output transport.
      typedef basic_string_view< typename t_TyXmlToken::_TyChar > _TyStrView;
      _TyStrView svCDataSection;
      _rtok.KGetStringView( rval, svCDataSection );
      _WriteTransportRaw( &svCDataSection[0], svCDataSection.length() );
    }
    _WriteTransportRaw( _TyMarkupTraits::s_kszCDataSectionEnd, DimensionOf( _TyMarkupTraits::s_kszCDataSectionEnd ) );
  }
  template < class t_tyXmlToken >
  void _WriteCharData( t_tyXmlToken const & _rtok )
  {
    typedef typename t_tyXmlToken::_TyLexValue _TyLexValue;
    // 1) We must recognize embedded CharRefs and EntityRefs.
    //    a) We must validate the CharRefs don't overflow.
    //    b) We must validate the EntityRefs refer to a defined entity.
    // 2) We must insert predefined entity refs when we find illegal characters.
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
            _WriteTransportRaw( _TyMarkupTraits::s_kszCDataSectionReplaceEnd, DimensionOf( _TyMarkupTraits::s_kszCDataSectionReplaceEnd ) );
          }
        }
      );
    }
    else
    {
      // Get a string view in the current token's character type and write that to the output transport.
      typedef basic_string_view< typename t_TyXmlToken::_TyChar > _TyStrView;
      _TyStrView svCDataSection;
      _rtok.KGetStringView( rval, svCDataSection );
      _WriteTransportRaw( &svCDataSection[0], svCDataSection.length() );
    }
  }
  typedef optional< _TyXmlTransportOut > _TyOptTransportOut;
  _TyOptTransportOut m_optTransportOut;
  _TyXmlDocumentContext m_xdcxtDocumentContext;
  _TyNamespaceMap m_mapNamespaces; // We maintain this as we go.
// options:
  bool m_fWriteBOM{true}; // Write a BOM because that's just a nice thing to do.
  bool m_fWriteXMLDecl{true}; // Whether to write an XMLDecl. This is honored in all scenarios.
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
