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
  typedef xml_token< t_TyXmlTraits > _TyXmlToken;
  typedef _xml_read_context< _TyXmlTraits > _TyReadContext;
  typedef xml_read_cursor< _TyXmlTraits > _TyReadCursor;
  typedef std::variant< _TyThis, _TyXmlToken > _TyVariant;
  typedef std::vector< _TyVariant > _TyRgTokens;

  xml_tag() = default;
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
  }

  void AcquireTag( _TyXmlToken && _rrtok )
  {
    m_opttokTag = std::move( _rrtok );
  }
  // Read from this read cursor into this object.
  void FromXmlStream( _TyReadCursor & _rxrc )
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
        _TyThis xmlTag;
        xmlTag.FromXmlStream( _rxrc );
        fNextTag = _rxrc.FNextTag( &xmlTag.m_opttokTag );
        _AcquireContent( std::move( xmlTag ) );
        if ( !fNextTag )
        {
          _AcquireContent( _rxrc.GetContextCur() );
          fNextTag = _rxrc.FMoveDown();
        }
      }
      while ( fNextTag );
    }
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
  void _AcquireContent( _TyThis && _rrtag )
  {
    m_rgTokens.emplace_back( std::move( _rrtag ) );
  }
  typedef optional< _TyXmlToken > _TyOptXmlToken; // Need this because token is not default constructible.
  _TyOptXmlToken m_opttokTag; // The token corresponding to the tag. This is either an XMLDecl token or 
  _TyRgTokens m_rgTokens; // The content for this token.
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
public:
  typedef t_TyXmlTraits _TyXmlTraits;
  typedef typename _TyXmlTraits::_TyChar _TyChar;
  typedef typename _TyXmlTraits::_TyTransport _TyTransport;
  typedef typename _TyXmlTraits::_TyLexUserObj _TyLexUserObj;
  typedef typename _xml_namespace_map_traits< _TyChar >::_TyUriAndPrefixMap _TyUriAndPrefixMap;
  typedef xml_read_cursor< _TyXmlTraits > _TyReadCursor;
  typedef XMLDeclProperties< _TyChar > _TyXMLDeclProperties;
  typedef _xml_document_context< _TyXmlTraits > _TyXmlDocumentContext;

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

  // Read from this read cursor into this object.
  void FromXmlStream( _TyReadCursor & _rxrc )
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
      _AcquireContent( _rxrc.GetContextCur() );
    }
    // Since we are done we can obtain the root tag: The XMLDecl pseudo-tag.
    _TyBase::AcquireTag( std::move( _rxrc.XMLDeclAcquireDocumentContext( m_xdcxtDocumentContext ) ) );
  }
protected:
  _TyXmlDocumentContext m_xdcxtDocumentContext;
};

__XMLP_END_NAMESPACE
