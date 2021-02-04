#pragma once

// xml_tag_var.h
// Simple XML object model based on the XML parser - variant version.
// Can't just wrap xml_tag objects because we have to store variant objects inside the xml_tag collection.
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
  typedef std::variant< _TyThis, _TyXmlTokenVar > _TyVariant;
  typedef std::vector< _TyVariant > _TyRgTokens;

  xml_tag_var() = default;
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
  }

  void AcquireTag( _TyXmlTokenVar && _rrtok )
  {
    m_opttokTag = std::move( _rrtok );
  }
  // Read from this read cursor into this object.
  void FromXmlStream( _TyReadCursorVar & _rxrc )
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
        _TyThis xmlTag;
        xmlTag.FromXmlStream( _rxrc );
        fNextTag = _rxrc.FNextTag( &xmlTag.m_opttokTag );
        _AcquireContent( std::move( xmlTag ) );
        if ( !fNextTag )
        {
          _AcquireContent( _rxrc );
          fNextTag = _rxrc.FMoveDown();
        }
      }
      while ( fNextTag );
    }
  }

protected:
  // Add all the current content from passed context.
  void _AcquireContent( _TyReadCursorVar & _rxrc )
  {
    _rxrc.ApplyAllContent(
      [this]( _TyXmlToken * _pxtBegin, _TyXmlToken * _pxtEnd )
      {
        for ( _TyXmlToken * pxtCur = _pxtBegin; _pxtEnd != pxtCur; ++pxtCur )
          m_rgTokens.emplace_back( std::move( *pxtCur ) );
      }
    );
    _rctxt.ClearContent(); // The above created a bunch of empty content nodes, and they would go away naturally without ill effect, but clear them to indicate they contain nothing at all.
  }
  // Add _rrtag tag as content of this tag at the current end for m_rgTokens;
  void _AcquireContent( _TyThis && _rrtag )
  {
    m_rgTokens.emplace_back( std::move( _rrtag ) );
  }
  typedef optional< _TyXmlTokenVar > _TyOptXmlTokenVar; // Need this because token is not default constructible.
  _TyOptXmlTokenVar m_opttokTag; // The token corresponding to the tag. This is either an XMLDecl token or a tag.
  _TyRgTokens m_rgTokens; // The content for this token.
};

// xml_document_var:
// This contains the root xml_tag_var as well as the namespace URI and Prefix maps and the user object.
template < class t_TyTpTransports >
class xml_document_var : protected xml_tag_var< t_TyTpTransports >
{
  typedef xml_document_var _TyThis;
  typedef xml_tag_var< t_TyTpTransports > _TyBase;
protected:
   using _TyBase::_AcquireContent;
public:
  typedef t_TyTpTransports _TyTpTransports;
  typedef MultiplexTuplePack_t< TGetXmlTraitsDefault, _TyTpTransports > _TyTpXmlTraits;
  typedef xml_read_cursor_var< _TyTpTransports > _TyReadCursorVar;

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
  void swap( _TyThis & _r )
  {
    if ( this == &_r )
      return;
    _TyBase::swap( _r );
    m_varDocumentContext.swap( _r.m_varDocumentContext );
  }
  // Read from this read cursor into this object.
  void FromXmlStream( _TyReadCursorVar & _rxrc )
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
      _TyBase xmlTagDocument; // The main actual document tag is a subtag of the XMLDecl tag.
      xmlTagDocument.FromXmlStream( _rxrc );
      bool fNextTag = _rxrc.FNextTag( &xmlTagDocument.m_opttokTag );
      Assert( !fNextTag || !fStartedInProlog ); // If we started in the middle of an XML then we might see that there is a next tag.
      _AcquireContent( std::move( xmlTagDocument ) );
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
protected:
  _TyXmlDocumentContextVar m_varDocumentContext;
};

__XMLP_END_NAMESPACE
