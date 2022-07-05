#pragma once

//          Copyright David Lawrence Bien 1997 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt).

// xml_tag_var.h
// Simple XML object model based on the XML parser - variant version.
// Can't just wrap xml_tag objects because we have to store variant token objects inside the xml_tag collection.
// dbien
// 03FEB2021

#include "xml_types.h"

__XMLP_BEGIN_NAMESPACE

template < class t_TyTpTransports >
class xml_tag_var
{
  typedef xml_tag_var _TyThis;
  friend xml_document_var< t_TyTpTransports >;
public:
  typedef t_TyTpTransports _TyTpTransports;
  typedef MultiplexTuplePack_t< TGetXmlTraitsDefault, _TyTpTransports > _TyTpXmlTraits;
  // The content of a tag is a series of tokens and tags.
  typedef xml_token_var< _TyTpTransports > _TyXmlTokenVar;
  typedef xml_read_cursor_var< _TyTpTransports > _TyReadCursorVar;
  // We may contain tokens which are not shared objects, or tags which are shared for the purpose of
  //  using weak pointer to point to the parent xml_tag. It should allow subtrees to be stored safely
  //  outside of management by the xml_document container. Because we store a pointer to our parent
  //  xml_tag we can't share the same xml_tag in multiple place in an XML document - as might be considered
  //  to be nice.
  // We store pointers to xml_tag object since we may be pointing to an xml_document_var object (as a parent weak pointer).
  typedef unique_ptr< _TyThis > _TyUPtrThis;
  typedef SharedStrongPtr< _TyUPtrThis, allocator< _TyUPtrThis >, uint32_t, false > _TyStrongThis;
  typedef SharedWeakPtr< _TyUPtrThis, allocator< _TyUPtrThis >, uint32_t, false > _TyWeakThis;
  typedef std::variant< _TyStrongThis, _TyXmlTokenVar > _TyVariant;
  typedef std::vector< _TyVariant > _TyRgTokens;
  typedef _xml_document_context_transport_var< _TyTpTransports > _TyXmlDocumentContextVar;

  virtual ~xml_tag_var() = default;
  xml_tag_var() = default;
  xml_tag_var( const _TyStrongThis * _pspParent )
  {
    if ( !!_pspParent )
      m_wpParent = *_pspParent;
  }
  xml_tag_var( xml_tag_var const & ) = default;
  xml_tag_var & operator=( xml_tag_var const & ) = default;
  xml_tag_var( xml_tag_var && ) = default;
  xml_tag_var & operator=( xml_tag_var && _rr )
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
  void AssertValid( const _TyThis * _ptagParent = nullptr ) const
  {
#if ASSERTSENABLED
    Assert( m_wpParent.expired() == !_ptagParent );
    if ( _ptagParent )
    {
      _TyStrongThis spParent( _SharedWeakPtr_weak_leave_empty(), m_wpParent );
      Assert( spParent->get() == _ptagParent );
    }
    if( !! m_opttokTag )
      m_opttokTag->AssertValid();
    typename _TyRgTokens::const_iterator citCur = m_rgTokens.begin();
    typename _TyRgTokens::const_iterator citEnd = m_rgTokens.end();
    for ( ; citEnd != citCur; ++citCur )
    {
      const _TyVariant & rvCur = *citCur;
      if ( holds_alternative< _TyXmlTokenVar >( rvCur ) )
        std::get< _TyXmlTokenVar >( rvCur ).AssertValid();
      else
        (*std::get< _TyStrongThis >( rvCur ))->AssertValid( this );// recurse.
    }
#endif //ASSERTSENABLED
  }
  void AcquireTag( _TyXmlTokenVar && _rrtok )
  {
    m_opttokTag = std::move( _rrtok );
  }
  // Read from this read cursor into this object.
  // REVIEW: <dbien>: Should change this to be a non-recursive implementation to avoid stack overrun vulnerability issues.
  void FromXmlStream( _TyReadCursorVar & _rxrc, _TyStrongThis const & _rspThis )
  {
    Assert( _rxrc.FInsideDocumentTag() );
    VerifyThrowSz( _rxrc.FInsideDocumentTag(), "Read cursor is in an invalid state." );

    // We can't acquire the token until the call the FNextTag().
    _AcquireContent( _rxrc );

    // We will move down into the next tag and that is the only tag we will capture into the xml_document_var... necessarily.
    if ( _rxrc.FMoveDown() )
    {
      bool fNextTag;
      do
      {
        _TyStrongThis spXmlTag( std::in_place_t(), make_unique< _TyThis >( &_rspThis ) );
        (*spXmlTag)->FromXmlStream( _rxrc, spXmlTag );
        fNextTag = _rxrc.FNextTag( &(*spXmlTag)->m_opttokTag );
        _AcquireContent( std::move( spXmlTag ) );
        if ( !fNextTag )
        {
          _AcquireContent( _rxrc );
          fNextTag = _rxrc.FMoveDown();
        }
      }
      while ( fNextTag );
    }
  }
  // Write this XML document to the given xml_writer<>.
  // Need to pass the xml document context for the containing xml_document.
  template < class t_TyXmlTransportOut >
  void ToXmlStream( xml_writer< t_TyXmlTransportOut > & _rxw, _TyXmlDocumentContextVar const & _rxdcxtDocumentContext ) const
  {
    _rxw.SetIncludePrefixesInAttrNames( _rxdcxtDocumentContext.FIncludePrefixesInAttrNames() );
    _ToXmlStream( _rxw );
  }
protected:
  // Add all the current content from passed context.
  void _AcquireContent( _TyReadCursorVar & _rxrc )
  {
    _rxrc.ApplyAllContent(
      // This is an xml_token<> type that is returned by a non-var xml_read_cursor.
      [this]( auto * _pxtBegin, auto * _pxtEnd )
      {
        for ( auto * pxtCur = _pxtBegin; _pxtEnd != pxtCur; ++pxtCur )
          m_rgTokens.emplace_back( std::move( *pxtCur ) );
      }
    );
    _rxrc.ClearContent(); // The above created a bunch of empty content nodes, and they would go away naturally without ill effect, but clear them to indicate they contain nothing at all.
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
    // We must start the tag and recurse within the visit lambda:
    std::visit( _VisitHelpOverloadFCall {
      [this,&_rxw]( const auto & _tXmlToken ) -> void
      {
        _TyXmlWriteTag xwtTag( _rxw.StartTag( _tXmlToken ) );
        // Just write the tag right away - the tag will be ended when the lifetime of xwtTag ends.    
        xwtTag.Commit();
        _WriteContent( _rxw ); // recurse, potentially.
      }
    }, m_opttokTag->GetVariant() );
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
      if ( holds_alternative< _TyXmlTokenVar >( rvCur ) )
      {
        // Just write the token to the writer - but resolve type locally:
        _TyXmlTokenVar const & rxtok = std::get< _TyXmlTokenVar >( rvCur );
        std::visit( _VisitHelpOverloadFCall {
          [&_rxw]( const auto & _tXmlToken ) -> void
          {
            _rxw.WriteToken( _tXmlToken );
          }
        }, rxtok.GetVariant() );
      }
      else
      { // recurse.
        (*std::get< _TyStrongThis >( rvCur ))->_ToXmlStream( _rxw );
      }
    }
  }
  typedef optional< _TyXmlTokenVar > _TyOptXmlTokenVar; // Need this because token is not default constructible.
  _TyOptXmlTokenVar m_opttokTag; // The token corresponding to the tag. This is either an XMLDecl token or a tag.
  _TyRgTokens m_rgTokens; // The content for this token.
  _TyWeakThis m_wpParent;
};

// xml_document_var:
// This contains the root xml_tag_var as well as the namespace URI and Prefix maps and the user object.
template < class t_TyTpTransports >
class xml_document_var : public xml_tag_var< t_TyTpTransports >
{
  typedef xml_document_var _TyThis;
  typedef xml_tag_var< t_TyTpTransports > _TyBase;
protected:
   using _TyBase::_AcquireContent;
   using _TyBase::_WriteContent;
   using _TyBase::m_opttokTag;
public:
  using typename _TyBase::_TyUPtrThis;
  using typename _TyBase::_TyWeakThis;
  using typename _TyBase::_TyStrongThis;
  typedef t_TyTpTransports _TyTpTransports;
  typedef MultiplexTuplePack_t< TGetXmlTraitsDefault, _TyTpTransports > _TyTpXmlTraits;
  typedef xml_read_cursor_var< _TyTpTransports > _TyReadCursorVar;
  typedef _xml_document_context_transport_var< _TyTpTransports > _TyXmlDocumentContextVar;

  xml_document_var() = default;
  xml_document_var( xml_document_var const & ) = default;
  xml_document_var & operator=( xml_document_var const & ) = default;
  xml_document_var( xml_document_var && ) = default;
  xml_document_var & operator=( xml_document_var && _rr )
  {
    _TyThis acquire( std::move( _rr ) );
    swap( acquire );
    return *this;
  }
  // PrCreateXmlDocument: Returns a created XML document.
  static pair< _TyThis *, _TyStrongThis > PrCreateXmlDocument()
  {
    unique_ptr< _TyThis > upXmlDoc = make_unique< _TyThis >();
    _TyThis * pXmlDoc = upXmlDoc.get();
    return pair< _TyThis *, _TyStrongThis >( pXmlDoc, _TyStrongThis( std::in_place_t(), std::move( upXmlDoc ) ) );
  }
  void swap( _TyThis & _r )
  {
    if ( this == &_r )
      return;
    _TyBase::swap( _r );
    m_varDocumentContext.swap( _r.m_varDocumentContext );
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
    VerifyThrowSz( !!m_opttokTag, "Empty xml_document_var." );
    return m_opttokTag->FNullValue();
  }
  template < class t_TyXMLDeclProperties >
  void GetXMLDeclProperties( t_TyXMLDeclProperties & _rxdp ) const
  {
    m_varDocumentContext.GetXMLDeclProperties( _rxdp );
  }
  // Read from this read cursor into this object.
  void FromXmlStream( _TyReadCursorVar & _rxrc, _TyStrongThis const & _rspThis )
  {
    Assert( _rxrc.FInProlog() || _rxrc.FInsideDocumentTag() );
    VerifyThrowSz( _rxrc.FInProlog() || _rxrc.FInsideDocumentTag(), "Read cursor is in an invalid state." );
    // Read each element in order. When done transfer the user object, UriMap and PrefixMap over to this object.
    // We can attach at any point during the iteration and we will glean the XMLDecl top node from the read cursor.
    
    bool fStartedInProlog = _rxrc.FInProlog();
    // We only read the top-level content tokens if we are currently at the XMLDecl tag.
    if ( fStartedInProlog )
      _AcquireContent( _rxrc );

    // We will move down into the next tag and that is the only tag we will capture into the xml_document_var... necessarily.
    VerifyThrowSz( _rxrc.FMoveDown(), "No tag to copy.");
    {//B
      _TyStrongThis spXmlDocument;
      {//B
        _TyUPtrThis ptrXmlTag = make_unique< _TyBase >( &_rspThis );
        spXmlDocument.emplace( std::in_place_t(), std::move( ptrXmlTag ) );
      }//EB
      (*spXmlDocument)->FromXmlStream( _rxrc, spXmlDocument );
      bool fNextTag = _rxrc.FNextTag( &(*spXmlDocument)->m_opttokTag );
      Assert( !fNextTag || !fStartedInProlog ); // If we started in the middle of an XML then we might see that there is a next tag.
      _AcquireContent( std::move( spXmlDocument ) );
    }//EB
    
    // Similarly we will only acquire the ending content if we started in the prolog - otherwise it could be bogus CharData at the end of the file.
    if ( fStartedInProlog )
    {
      Assert( _rxrc.FInEpilog() );
      _AcquireContent( _rxrc );
    }
    // Done. Get the pseudo-tag XMLDecl from the root of the cursor.
    _TyBase::AcquireTag( _rxrc.XMLDeclAcquireDocumentContext( m_varDocumentContext ) );
  }
  // Write this XML document to the given xml_writer<>.
  template < class t_TyXmlTransportOut >
  void ToXmlStream( xml_writer< t_TyXmlTransportOut > & _rxw ) const
  {
    _rxw.SetIncludePrefixesInAttrNames( m_varDocumentContext.FIncludePrefixesInAttrNames() );
    // Just write everything to the writer.
    // The writer itself writes the XMLDecl tag based on the output encoding (which it knows about), etc.
    // So we use this method as a specialization to skip what would have happened in the base.
    _WriteContent( _rxw ); // This recurses.
  }
protected:
  _TyXmlDocumentContextVar m_varDocumentContext;
};

__XMLP_END_NAMESPACE
