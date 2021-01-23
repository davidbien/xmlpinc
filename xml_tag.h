#pragma once

// xml_tag.h
// Simple XML object model based on the XML parser.
// dbien
// 23JAN2021

// The root tag is always an XMLDecl "tag" - even if there was no XMLDecl declaration in the XML file.
// Then, as with all tags, there may be some content nodes, then the main document tag, and then subtags.
// Trouble must be taken to ensure that only a single document tag is ever within the XMLDecl "tag" since
//  all other tags have freeform contents in that regard.

__XMLP_BEGIN_NAMESPACE

template < class t_TyXmlTraits >
class xml_tag
{
  typedef xml_tag _TyThis;
public:
  // The content of a tag is a series of tokens and tags.
  typedef xml_token< t_TyXmlTraits > _TyXmlToken;
  typedef std::variant< _TyThis, _TyXmlToken > _TyVariant;
  typedef std::vector< _TyVariant > _TyRgTokens;

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
    m_tokTag.swap( _r.m_tokTag );
    m_rgTokens.swap( _r.m_rgTokens );
  }


protected:
  _TyXmlToken m_tokTag; // The token corresponding to the tag. This is either an XMLDecl token or 
  _TyRgTokens m_rgTokens; // The content for this token.
};

// xml_document:
// This contains the root xml_tag as well as the namespace URI and Prefix maps.
template < class t_TyXmlTraits >
class xml_document
{

};

__XMLP_END_NAMESPACE
