#pragma once

// xml_tag.h
// Simple XML object model based on the XML parser.
// dbien
// 23JAN2021

// The root tag is always an XMLDecl "tag" - even if there was no XMLDecl declaration in the XML file.
// Then, as with all tags, there may be some content nodes, then the main document tag, and then subtags.
// Trouble must be taken to ensure that only a single document tag is ever within the XMLDecl "tag" since
//  all other tags have freeform contents in that regard.
#include "xml_types.h"

__XMLP_BEGIN_NAMESPACE

template < class t_TyXmlTraits >
class xml_tag
{
  typedef xml_tag _TyThis;
  friend xml_document< t_TyXmlTraits >;
public:
  typedef t_TyXmlTraits _TyXmlTraits;
  // The content of a tag is a series of tokens and tags.
  typedef typename _TyXmlTraits::_TyXmlToken _TyXmlToken;
  typedef _xml_read_context< _TyXmlTraits > _TyReadContext;
  typedef xml_read_cursor< _TyXmlTraits > _TyReadCursor;
  // We may contain tokens which are not shared objects, or tags which are shared for the purpose of
  //  using weak pointer to point to the parent xml_tag. It should allow subtrees to be stored safely
  //  outside of management by the xml_document container.
  // We store pointers to xml_tag object since we may be pointing to an xml_document object (as a parent weak pointer).
  typedef unique_ptr< _TyThis > _TyUPtrThis;
  typedef SharedStrongPtr< _TyUPtrThis, allocator< _TyUPtrThis >, uint32_t, false > _TyStrongThis;
  typedef SharedWeakPtr< _TyUPtrThis, allocator< _TyUPtrThis >, uint32_t, false > _TyWeakThis;
  typedef std::variant< _TyStrongThis, _TyXmlToken > _TyVariant;
  typedef std::vector< _TyVariant > _TyRgTokens;
  typedef _xml_document_context_transport< _TyXmlTraits > _TyXmlDocumentContext;

  xml_tag() = default;
  xml_tag( const _TyStrongThis * _pspParent )
  {
    if ( !!_pspParent )
      m_wpParent = *_pspParent;
  }
  xml_tag( xml_tag const & ) = default;
  xml_tag & operator=( xml_tag const & ) = default;
  xml_tag( xml_tag && ) = default;
  xml_tag & operator=( xml_tag && _rr )
  {
    _TyThis acquire( std::move( _rr ) );
    swap( acquire );
    return *this;
  }
  void swap( _TyThis & _r )
  {
    m_opttokTag.swap( _r.m_opttokTag );
    m_rgTokens.swap( _r.m_rgTokens );
    m_wpParent.swap( _r.m_wpParent );
  }
  void AssertValid( const _TyStrongThis & _rspThis, const _TyStrongThis * _pspParent = nullptr ) const
  {
#if ASSERTSENABLED
    Assert( ( !m_wpParent == !_pspParent ) && ( !_pspParent || ( m_wpParent == *_pspParent ) ) );
    if( !! m_opttokTag )
      m_opttokTag->AssertValid();
    typename _TyRgTokens::const_iterator citCur = m_rgTokens.begin();
    typename _TyRgTokens::const_iterator citEnd = m_rgTokens.end();
    for ( ; citEnd != citCur; ++citCur )
    {
      const _TyVariant & rvCur = *citCur;
      if ( holds_alternative< _TyXmlToken >( rvCur ) )
        std::get< _TyXmlToken >( rvCur ).AssertValid();
      else
        (*std::get< _TyStrongThis >( rvCur ))->AssertValid( std::get< _TyStrongThis >( rvCur ), &_rspThis );
    }
#endif //ASSERTSENABLED
  }
  void AcquireTag( _TyXmlToken && _rrtok )
  {
    m_opttokTag = std::move( _rrtok );
    m_opttokTag->AssertValid();
  }
  // Read from this read cursor into this object.
  void FromXmlStream( _TyReadCursor & _rxrc, _TyStrongThis const & _rspThis )
  {
    Assert( _rxrc.FInsideDocumentTag() );
    VerifyThrowSz( _rxrc.FInsideDocumentTag(), "Read cursor is in an invalid state." );
   
    // We can't acquire the token until the call the FNextTag().
    _AcquireContent( _rxrc.GetContextCur() );

    // We will move down into the next tag and that is the only tag we will capture into the xml_document... necessarily.
    if ( _rxrc.FMoveDown() )
    {
      bool fNextTag;
      do
      {
        _TyStrongThis spXmlTag;
        {//B
          _TyUPtrThis ptrXmlTag = make_unique< _TyThis >( &_rspThis );
          spXmlTag.emplace( std::in_place_t::in_place_t(), std::move( ptrXmlTag ) );
        }//EB
        (*spXmlTag)->FromXmlStream( _rxrc, &spXmlTag );
        fNextTag = _rxrc.FNextTag( &(*spXmlTag)->m_opttokTag );
        _AcquireContent( std::move( spXmlTag ) );
        if ( !fNextTag )
        {
          _AcquireContent( _rxrc.GetContextCur() );
          fNextTag = _rxrc.FMoveDown();
        }
      }
      while ( fNextTag );
    }
  }
  // Write this XML document to the given xml_writer<>.
  // Need to pass the xml document context for the containing xml_document.
  template < class t_TyXmlTransportOut >
  void ToXmlStream( xml_writer< t_TyXmlTransportOut > & _rxw, _TyXmlDocumentContext const & _rxdcxtDocumentContext ) const
  {
    _rxw.SetIncludePrefixesInAttrNames( _rxdcxtDocumentContext.FIncludePrefixesInAttrNames() );
    _ToXmlStream( _rxw );
  }
protected:
  // Add all the current content from passed context.
  void _AcquireContent( _TyReadContext & _rctxt )
  {
    _rctxt.ApplyContent( 0, _rctxt.NContentTokens(),
      [this]( _TyXmlToken * _pxtBegin, _TyXmlToken * _pxtEnd )
      {
        for ( _TyXmlToken * pxtCur = _pxtBegin; _pxtEnd != pxtCur; ++pxtCur )
          m_rgTokens.emplace_back( std::move( *pxtCur ) );
      }
    );
    _rctxt.ClearContent(); // The above created a bunch of empty content nodes, and they would go away naturally without ill effect, but clear them to indicate they contain nothing at all.
  }
  // Add _rrtag tag as content of this tag at the current end for m_rgTokens;
  void _AcquireContent( _TyStrongThis && _rrspTag )
  {
    m_rgTokens.emplace_back( std::move( _rrspTag ) );
  }
  template < class t_TyXmlTransportOut >
  void _ToXmlStream( xml_writer< t_TyXmlTransportOut > & _rxw ) const
  {
    typedef xml_writer< t_TyXmlTransportOut > _TyXmlWriter;
    typedef typename _TyXmlWriter::_TyXmlWriteTag _TyXmlWriteTag;
    _TyXmlWriteTag xwtTag( _rxw.StartTag( *m_opttokTag ) );
    // Just write the tag right away - the tag will be ended when the lifetime of xwtTag ends.    
    xwtTag.Commit();
    _WriteContent( _rxw ); // recurse, potentially.
  }
  template < class t_TyXmlTransportOut >
  void _WriteContent( xml_writer< t_TyXmlTransportOut > & _rxw ) const
  {
    typedef xml_writer< t_TyXmlTransportOut > _TyXmlWriter;
    typename _TyRgTokens::const_iterator citCur = m_rgTokens.begin();
    typename _TyRgTokens::const_iterator citEnd = m_rgTokens.end();
    for ( ; citEnd != citCur; ++citCur )
    {
      const _TyVariant & rvCur = *citCur;
      if ( holds_alternative< _TyXmlToken >( rvCur ) )
      {
        // Just write the token to the writer:
        _rxw.WriteToken( std::get< _TyXmlToken >( rvCur ) );
      }
      else
      { // recurse.
        (*std::get< _TyStrongThis >( rvCur ))->_ToXmlStream( _rxw );
      }
    }
  }
  typedef optional< _TyXmlToken > _TyOptXmlToken; // Need this because token is not default constructible.
  _TyOptXmlToken m_opttokTag; // The token corresponding to the tag. This is either an XMLDecl token or just a normal tag.
  _TyRgTokens m_rgTokens; // The content for this token.
  _TyWeakThis m_wpParent;
};

// xml_document:
// This contains the root xml_tag as well as the namespace URI and Prefix maps and the user object.
template < class t_TyXmlTraits >
class xml_document : protected xml_tag< t_TyXmlTraits >
{
  typedef xml_document _TyThis;
  typedef xml_tag< t_TyXmlTraits > _TyBase;
protected:
  using _TyBase::_AcquireContent;
  using _TyBase::_WriteContent;
  using _TyBase::m_opttokTag;
public:
  using typename _TyBase::_TyUPtrThis;
  using typename _TyBase::_TyWeakThis;
  using typename _TyBase::_TyStrongThis;
  typedef t_TyXmlTraits _TyXmlTraits;
  typedef typename _TyXmlTraits::_TyChar _TyChar;
  typedef typename _TyXmlTraits::_TyTransport _TyTransport;
  typedef typename _TyXmlTraits::_TyLexUserObj _TyLexUserObj;
  typedef typename _xml_namespace_map_traits< _TyChar >::_TyUriAndPrefixMap _TyUriAndPrefixMap;
  typedef xml_read_cursor< _TyXmlTraits > _TyReadCursor;
  typedef XMLDeclProperties< _TyChar > _TyXMLDeclProperties;
  typedef _xml_document_context_transport< _TyXmlTraits > _TyXmlDocumentContext;

  xml_document() = default;
  xml_document( xml_document const & ) = default;
  xml_document & operator=( xml_document const & ) = default;
  xml_document( xml_document && ) = default;
  xml_document & operator=( xml_document && _rr )
  {
    _TyThis acquire( std::move( _rr ) );
    swap( acquire );
    return *this;
  }
  void swap( _TyThis & _r )
  {
    _TyBase::swap( _r );
    m_xdcxtDocumentContext.swap( _r.m_xdcxtDocumentContext );
  }
  void AssertValid() const
  {
#if ASSERTSENABLED
    _TyBase::AssertValid();
#endif //ASSERTSENABLED
  }
  // Returns whether the XMLDecl token is a "pseudo-token" - i.e. it was created not read.
  bool FPseudoXMLDecl() const
  {
    VerifyThrowSz( !!m_opttokTag, "Empty xml_document." );
    return m_opttokTag->GetValue().FIsNull();
  }
  const _TyXMLDeclProperties & GetXMLDeclProperties() const
  {
    return m_xdcxtDocumentContext.GetXMLDeclProperties();
  }
  _TyXMLDeclProperties & GetXMLDeclProperties()
  {
    return m_xdcxtDocumentContext.GetXMLDeclProperties();
  }

  // Read from this read cursor into this object.
  // _rspThis contains the weak pointer for this object. This is required to correctly populate the parent pointers.
  void FromXmlStream( _TyReadCursor & _rxrc, _TyStrongThis const & _rspThis )
  {
    Assert( _rxrc.FInProlog() || _rxrc.FInsideDocumentTag() );
    VerifyThrowSz( _rxrc.FInProlog() || _rxrc.FInsideDocumentTag(), "Read cursor is in an invalid state." );
    // Read each element in order. When done transfer the user object, UriMap and PrefixMap over to this object.
    // We can attach at any point during the iteration and we will glean the XMLDecl top node from the read cursor.
    
    bool fStartedInProlog = _rxrc.FInProlog();
    // We only read the top-level content tokens if we are currently at the XMLDecl tag.
    if ( fStartedInProlog )
      _AcquireContent( _rxrc.GetContextCur() );

    // We will move down into the next tag and that is the only tag we will capture into the xml_document... necessarily.
    VerifyThrowSz( _rxrc.FMoveDown(), "No tag to copy.");
    {//B
      _TyStrongThis spXmlDocument;
      {//B
        _TyUPtrThis ptrXmlTag = make_unique< _TyBase >( &_rspThis );
        spXmlDocument.emplace( std::in_place_t::in_place_t(), std::move( ptrXmlTag ) );
      }//EB
      (*spXmlDocument)->FromXmlStream( _rxrc );
      bool fNextTag = _rxrc.FNextTag( &xmlTagDocument.m_opttokTag );
      Assert( !fNextTag || !fStartedInProlog ); // If we started in the middle of an XML then we might see that there is a next tag.
      _AcquireContent( std::move( spXmlDocument ) );
    }//EB
    
    // Similarly we will only acquire the ending content if we started in the prolog - otherwise it could be bogus CharData at the end of the file.
    if ( fStartedInProlog )
    {
      Assert( _rxrc.FInEpilog() );
      _AcquireContent( _rxrc.GetContextCur() );
    }
    // Since we are done we can obtain the root tag: The XMLDecl pseudo-tag.
    _TyBase::AcquireTag( std::move( _rxrc.XMLDeclAcquireDocumentContext( m_xdcxtDocumentContext ) ) );
  }
  // Write this XML document to the given xml_writer<>.
  template < class t_TyXmlTransportOut >
  void ToXmlStream( xml_writer< t_TyXmlTransportOut > & _rxw ) const
  {
    _rxw.SetIncludePrefixesInAttrNames( m_xdcxtDocumentContext.FIncludePrefixesInAttrNames() );
    // Just write everything to the writer.
    // The writer itself writes the XMLDecl tag based on the output encoding (which it knows about), etc.
    // So we use this method as a specialization to skip what would have happened in the base.
    _WriteContent( _rxw ); // This recurses.
  }
protected:
  _TyXmlDocumentContext m_xdcxtDocumentContext;
};

__XMLP_END_NAMESPACE
