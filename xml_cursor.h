#pragma once

// xml_cursor.h
// Cursor allowing movement through the current parse tree.
// dbien
// 10JAN2021

// This object does the actual work of reading and maintaining the parse state of the XML parser.
// This would allow for multiple attachments to a given file/mapping/memory - and indeed as
//  long as the file is dup()'d then this should be totally fine - it needs to have a separate seek pointer.

#include "xml_ns.h"
#include "xml_types.h"
#include "xml_traits.h"

__XMLP_BEGIN_NAMESPACE

// _xml_read_context
// This maintains the a single level of the current read state of the XML parser.
// The read cursor maintains a stack of these.
// We don't need to maintain file positions, etc, as these are maintained by the lexical analyzer.
// The invariant is that the parser is always located at the position at the end of the 
template < class t_TyXmlTraits >
class _xml_read_context
{
  typedef _xml_read_context _TyThis;
public:
  typedef t_TyXmlTraits _TyXmlTraits;
  typedef typename _TyXmlTraits::_TyLexTraits _TyLexTraits;
  typedef _l_token< _TyLexTraits > _TyLexToken;
  typedef xml_token< _TyXmlTraits > _TyXmlToken;

  ~_xml_read_context() = default;
  _xml_read_context( _TyLexToken && _rrtok )
    : m_tok( std::move( _rrtok ) )
  {
  }
  _xml_read_context() = delete;
  _xml_read_context( _xml_read_context const & ) = delete;
  _xml_read_context & operator=( _xml_read_context const & ) = delete;
  _xml_read_context & operator=( _xml_read_context && ) = delete;

  const _TyXmlToken & GetToken() const
  {
    return m_tok;
  }
  _TyXmlToken & GetToken()
  {
    return m_tok;
  }
protected:
  _TyXmlToken m_tok;
};

// _xml_read_cursor_base_namespace:
// Contains some infrastructure to support the Namespace if desired.
template < class t_TyXmlTraits, bool t_kfSupportNamespaces = t_TyXmlTraits::s_kfSupportNamespaces >
class _xml_read_cursor_base_namespace
{
  typedef _xml_read_cursor_base_namespace _TyThis;
public:
  static constexpr bool s_kfSupportNamespaces = true;
  typedef t_TyXmlTraits _TyXmlTraits;
  typedef typename _TyXmlTraits::_TyLexTraits _TyLexTraits;
  typedef _l_token< _TyLexTraits > _TyLexToken;
  typedef typename _TyXmlTraits::_TyChar _TyChar;
  typedef typename _TyXmlTraits::_TyStdStr _TyStdStr;
  typedef typename _TyXmlTraits::_TyStrView _TyStrView;
  typedef typename _TyXmlTraits::_TyNamespaceMap _TyNamespaceMap;

  // The prefix xml is by definition bound to the namespace name http://www.w3.org/XML/1998/namespace
  _TyNamespaceMap m_mapNamespaces{ { str_array_cast<_TyChar>("xml"), str_array_cast<_TyChar>("http://www.w3.org/XML/1998/namespace") } };

  ~_xml_read_cursor_base_namespace() = default;
  _xml_read_cursor_base_namespace() = default;
  _xml_read_cursor_base_namespace( _xml_read_cursor_base_namespace const & ) = delete;
  _xml_read_cursor_base_namespace & operator =( _xml_read_cursor_base_namespace const & ) = delete;
  _xml_read_cursor_base_namespace( _xml_read_cursor_base_namespace && ) = default;
  _xml_read_cursor_base_namespace & operator =( _xml_read_cursor_base_namespace && ) = default;
  void swap( _TyThis & _r )
  {
    m_mapNamespaces.swap( _r.m_mapNamespaces );
  }
  template < class t_TyTransportCtxt >
  _xml_namespace_uri & _RLookupNamespacePrefix( _l_data_typed_range const & _rdr, t_TyTransportCtxt const & _rcxt )
  {
    _TyStrView sv;
    _rcxt.GetStringView( sv, _rdr );
    _TyEntityMap::const_iterator cit = m_mapNamespaces.find( sv );
    if ( m_mapNamespaces.end() == cit )
    {
      _TyStdStr str( sv );
      THROWXMLPARSEEXCEPTION("Can't find namespace prefix [%s].", str.c_str() );
    }
    return cit->second;
  }
  void _ProcessNamespaces( _TyLexToken & _rltok )
  {
    
  }
};
// Non-Namespace base.
template < class t_TyXmlTraits >
class _xml_read_cursor_base_namespace< t_TyXmlTraits, false >
{
  typedef _xml_read_cursor_base_namespace _TyThis;
public:
  static constexpr bool s_kfSupportNamespaces = false;
  typedef t_TyXmlTraits _TyXmlTraits;
  void swap( _TyThis & _r )
  {
  }
  void _ProcessNamespaces( _TyLexToken & _rltok )
  {
    
  }
};

template < class t_TyXmlTraits >
class xml_read_cursor 
  : public _xml_read_cursor_base_namespace< t_TyXmlTraits, t_TyXmlTraits::s_kfSupportNamespaces >
{
  typedef xml_read_cursor _TyThis;
  typedef _xml_read_cursor_base_namespace< t_TyXmlTraits, t_TyXmlTraits::s_kfSupportNamespaces > _TyBase;
public:
  typedef t_TyXmlTraits _TyXmlTraits;
  using _TyBase::s_kfSupportNamespaces;
  typedef typename _TyXmlTraits::_TyLexTraits _TyLexTraits;
  typedef _l_token< _TyLexTraits > _TyLexToken;
  typedef xml_parser< _TyXmlTraits > _TyXmlParser;
  friend _TyXmlParser;
  typedef _xml_read_context< _TyXmlTraits > _TyXmlReadContext;
  typedef list< _TyXmlReadContext > _TyListReadContexts;

  ~xml_read_cursor();
  xml_read_cursor() = default;
  xml_read_cursor( xml_read_cursor const & ) = delete;
  xml_read_cursor & operator=( xml_read_cursor const & ) = delete;
  xml_read_cursor( xml_read_cursor && ) = default;
  xml_read_cursor & operator=( xml_read_cursor && ) = default;

  void SetSkipXMLDecl( bool _fSkipXMLDecl = true )
  {
    if ( m_fSkipXMLDecl != _fSkipXMLDecl )
    {
      m_fSkipXMLDecl = _fSkipXMLDecl;
      m_fUpdateSkip = true;
    }
  }
  bool FGetSkipXMLDecl() const
  {
    return m_fSkipXMLDecl;
  }
  void SetSkipComments( bool _fSkipComments = true )
  {
    if ( m_fSkipComments != _fSkipComments )
    {
      m_fSkipComments = _fSkipComments;
      m_fUpdateSkip = true;
    }
  }
  bool FGetSkipComments() const
  {
    return m_fSkipComments;
  }
  void SetSkipProcessingInstructions( bool _fSkipPI = true )
  {
    if ( m_fSkipPI != _fSkipPI )
    {
      m_fSkipPI = _fSkipPI;
      m_fUpdateSkip = true;
    }
  }
  bool FetSkipProcessingInstructions() const
  {
    return m_fSkipPI;
  }
  



protected:
  void _AttachXmlParser( _TyXmlParser * _pXp )
  {
    if ( !!m_pXp )
      _DetachXmlParser();
    _UpdateSkip();
    unique_ptr< _TyLexToken > pltokFirst;
    VerifyThrowSz( _pXp->RLex().FGetToken( pltokFirst, m_rgSkipTokensCur.begin(), m_rgSkipTokensCur.begin() + m_nSkip ), 
      "You failed at the first thing you tried to do! No token found at beginning of stream." );
    VerifyThrowSz( s_knTokenETag != pltokFirst->GetTokenId(), "Got an end tag as the first tag in the stream." );
    _ProcessToken( pltokFirst ); // process any namespaces, etc.
    m_lContexts.emplace_front( std::move( *pltokFirst ) );
    m_itCurContext = m_lContexts.begin();
  }
  void _ProcessToken( _TyLexToken & _rltok )
  {

  }
  void _DetachXmlParser
  {
    m_pXp = nullptr;
    m_lContexts.clear();
    m_itCurContext = m_lContexts.end();
  }
  void _UpdateSkip()
  {
    if ( !m_fUpdateSkip )
      return;
    m_fUpdateSkip = false;
    size_t * pstCur = m_rgSkipTokensCur.begin();
    if ( m_fSkipXMLDecl )
      *pstCur++ = s_knTokenXMLDecl;
    if ( m_fSkipComments )
      *pstCur++ = s_knTokenComment;
    if ( m_fSkipPI )
      *pstCur++ = s_knTokenProcessingInstruction;
    m_nSkip = pstCur - m_rgSkipTokensCur.begin();
  }
  static constexpr size_t s_knMaxSkippedTokens = 32; // random number.
  typedef array< vtyTokenIdent, s_knMaxSkippedTokens > _TySkipArray;

  _TyXmlParser * m_pXp{nullptr}; // the pointer to the XML parser we are reading tokens from.
  _TyListReadContexts m_lContexts;
  typename _TyListReadContexts::iterator m_itCurContext{m_lContexts.end()};
  _TySkipArray m_rgSkipTokensCur{0};
  size_t m_nSkip{0};
  bool m_fSkipXMLDecl{false};
  bool m_fSkipComments{false};
  bool m_fSkipPI{false};
  bool m_fUpdateSkip{false};
};

__XMLP_END_NAMESPACE
