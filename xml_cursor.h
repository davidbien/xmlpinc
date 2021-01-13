#pragma once

// xml_cursor.h
// Cursor allowing movement through the current parse tree.
// dbien
// 10JAN2021

// This object does the actual work of reading and maintaining the parse state of the XML parser.
// This would allow for multiple attachments to a given file/mapping/memory - and indeed as
//  long as the file is dup()'d then this should be totally fine - it needs to have a separate seek pointer.
// Note that due to the necessity of combining CharData and CDATA sections (in any combination as long as they are consecutive)
//  requires that we have one token of lookahead. This is only done with m_fIntegrateCDATASections is set to true in the parser.
// We need the ability to iterate CharData and CDATASections separately as well to correctly duplicate the XML on output (pretty printing, filtering, etc).
// Note that as far as my brief research could surmise, for a start tag and end tag that are expected to match each other, the literal text of the tag
//  declarations themselves must match. This allows non-namespace aware readers to correctly process XML with namespaces.

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

template < class t_TyXmlTraits >
struct _xml_read_cursor_base_namespace_base
{
  typedef t_TyXmlTraits _TyXmlTraits;
  typedef typename _TyXmlTraits::_TyChar _TyChar;
  typedef typename _TyXmlTraits::_TyLexTraits _TyLexTraits;
  typedef _l_token< _TyLexTraits > _TyLexToken;
  typedef _l_value< _TyLexTraits > _TyLexValue;
  typedef _l_data< _TyChar > _TyData;
  void _ProcessTagName( _TyLexToken & _rltok )
  {
    // Tag names must match literally between start and end tags.
    // As well we want the end user to see a uniform representation of tag names.
    // So if a namespace specifier is present then we will reset the end position of the this namespace specifier to include the ':' and the namespace name.
    _TyLexValue & rrgName = _rltok.GetValue()[0];
    _TyData const & krdtName = rrgName[1].GetVal< _TyData >();
    if ( !krdtName.FIsNull() )
    {
      _TyData & rdtPrefix = rrgName[0].GetVal< _TyData >();
      rdtPrefix.DataRangeGetSingle().m_posEnd = krdtName.DataRangeGetSingle().m_posEnd;
    }
    // Leave the name as is - further processing will occur depending on flavor.
  }
};

// _xml_read_cursor_base_namespace:
// Contains some infrastructure to support the Namespace if desired.
template < class t_TyXmlTraits, bool t_kfSupportNamespaces = t_TyXmlTraits::s_kfSupportNamespaces >
class _xml_read_cursor_base_namespace 
  : public _xml_read_cursor_base_namespace_base< t_TyXmlTraits >
{
  typedef _xml_read_cursor_base_namespace _TyThis;
  typedef _xml_read_cursor_base_namespace_base< t_TyXmlTraits > _TyBase;
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
  // Process any namespace declarations in this token and then process any namespace references.
  // Also record whether double quotes were used for attributes.
  void _ProcessNamespaces( _TyLexToken & _rltok, bool _fOnlyTagName )
  {
    // First thing to do is to move through all attributes and obtain all namespace declarations and at the same time ensure they are valid.
    if ( !_fOnlyTagName )
    {
      _TyLexValue & rrgAttrs = _rltok.GetValue()[1];
      size_t nAttrs = rrgAttrs.GetSize();
      for ( size_t nAttr = 0; nAttr < nAttrs; ++nAttr )
      {
        _TyLexValue & rrgAttr = rrgAttrs[nAttr];
        _TyStrView svName;
        rrgAttr[0].GetStringView( _rltok, svName );
        if ( !svName.compare( str_array_cast< _TyChar >("xmlns") ) )
        {
          // We have a namespace declaration!
          _TyStrView svUri;
          rrgAttr[1].GetStringView( _rltok, svUri );
          
        }

        
      }
      
    }
    _ProcessTagName( _rltok );
  }
};
// Non-Namespace base.
template < class t_TyXmlTraits >
class _xml_read_cursor_base_namespace< t_TyXmlTraits, false > 
  : public _xml_read_cursor_base_namespace_base< t_TyXmlTraits >
{
  typedef _xml_read_cursor_base_namespace _TyThis;
  typedef _xml_read_cursor_base_namespace_base< t_TyXmlTraits > _TyBase;
public:
  static constexpr bool s_kfSupportNamespaces = false;
  typedef t_TyXmlTraits _TyXmlTraits;
  void swap( _TyThis & _r )
  {
  }
  // This method:
  // 1) Consolidates any namespaces detected by lex productions into the combined name with a ':' in the middle.
  // 2) Changes the namespace field into a boolean field indicating whether double quotes were used during the attribute declaration (attrs only).
  void _ProcessNamespaces( _TyLexToken & _rltok, bool _fOnlyTagName )
  {
    _ProcessTagName( _rltok );
    
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

  bool FGotFirstTag() const
  {
    return m_fGotFirstTag;
  }

  // If this is set to true then all consecutive CharData and CDataSections are integrated into a single CharData token.
  void SetIntegrateCDataSections( bool _fIntegrateCDataSections = true )
  {
    m_fIntegrateCDataSections = _fIntegrateCDataSections;
  }
  bool FGetIntegrateCDataSections() const
  {
    return m_fIntegrateCDataSections;
  }
  void SetFilterWhitespaceCharData( bool _fFilterWhitespaceCharData = true )
  {
    if ( m_fFilterWhitespaceCharData != _fFilterWhitespaceCharData )
    {
      m_fFilterWhitespaceCharData = _fFilterWhitespaceCharData;
      if ( !!m_pXp )
        m_pXp->SetFilterWhitespaceCharData( m_fFilterWhitespaceCharData );
    }
  }
  bool FFilterWhitespaceCharData() const
  {
    return m_fFilterWhitespaceCharData;
  }
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
    m_pXp = _pXp;
    m_pXp->SetFilterWhitespaceCharData( m_fFilterWhitespaceCharData );
    _UpdateSkip();
    unique_ptr< _TyLexToken > pltokFirst;
    VerifyThrowSz( _pXp->RLex().FGetToken( pltokFirst, m_rgSkipTokensCur.begin(), m_rgSkipTokensCur.begin() + m_nSkip ), 
      "You failed at the first thing you tried to do! No token found at beginning of stream." );
    vtyTokenIdent rgtidInvalidFirstToken = { s_knTokenETag, s_knTokenCDataSection, s_knTokenCharData };
    const vtyTokenIdent * ptidEndInvalidFirstToken = rgtidInvalidFirstToken + DimensionOf( rgtidInvalidFirstToken );
    VerifyThrowSz( ptidEndInvalidFirstToken == find( rgtidInvalidFirstToken, ptidEndInvalidFirstToken, pltokFirst->GetTokenId() ), "Got an invalid tag as the first tag in the stream." );
    _ProcessToken( pltokFirst ); // process any namespaces, etc.
    if ( ( s_knTokenXMLDecl == pltokFirst->GetTokenId() ) && m_fSkipXMLDecl )
    {
      // Then we must get another token because we needed to record the XMLDecl info anyway even if we didn't give it to the user.
      pltokFirst.reset();
      VerifyThrowSz( _pXp->RLex().FGetToken( pltokFirst, m_rgSkipTokensCur.begin(), m_rgSkipTokensCur.begin() + m_nSkip ), 
        "Second token was not found at beginning of stream." );
      vtyTokenIdent rgtidInvalidSecondToken = { s_knTokenETag, s_knTokenCDataSection, s_knTokenXMLDecl };
      const vtyTokenIdent * ptidEndInvalidSecondToken = rgtidInvalidSecondToken + DimensionOf( rgtidInvalidSecondToken );
      VerifyThrowSz( ptidEndInvalidSecondToken == find( rgtidInvalidSecondToken, ptidEndInvalidSecondToken, pltokFirst->GetTokenId() ), "Got an invalid tag as the first tag in the stream." );
      _ProcessToken( pltokFirst );
    }
    m_lContexts.emplace_front( std::move( *pltokFirst ) );
    m_itCurContext = m_lContexts.begin();
  }
  void _ProcessToken( unique_ptr< _TyLexToken > & _rpltok )
  {
    // 1) If a tag then we need to process namespaces.
    // 2) If CharData or CDATASection then we may need to read a token ahead to see if we need to combine, etc.
    switch( _rpltok->GetTokenId() )
    {
      case s_knTokenXMLDecl:
        // This is only valid at the beginning of the file.
        _ProcessXMLDecl( _rpltok );
      break;
      case s_knTokenSTag:
      case s_knTokenETag:
      case s_knTokenEmptyElemTag:
        _ProcessTagName( _rpltok ); // Must do this before the namespace processing.
        _ProcessNamespaces( _rpltok );
        _ProcessTag( _rpltok );
      break;
      case s_knTokenComment:
        _ProcessComment( _rpltok );
      break;
      case s_knTokenCDataSection:
      case s_knTokenCharData:
        _ProcessCharData( _rpltok );
      break;
    }
  }
  void _ProcessXMLDecl( unique_ptr< _TyLexToken > & _rpltok )
  {
    // Pull the info we want out of it to store in the cursor.
    _TyValue & rrgVals = _rpltok->GetValue();
    Assert( rrgVals.FIsArray() );
    m_fStandalone = rrgVals[0].GetVal<bool>();
    rrgVals[2].GetString( *_rpltok, m_strEncoding );
    _TyStdStr strMinorVNum;
    rrgVals[4].GetString( *_rpltok, strMinorVNum );
    m_nVersionMinorNumber = strMinorVNum[0] - _TyChar('0');
  }
  void _DetachXmlParser
  {
#error make sure this is complete.
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
  unique_ptr< _TyLexToken > m_pltokLookahead; // When m_fIntegrateCDataSections is true then this may be populated with the next token.
  typename _TyListReadContexts::iterator m_itCurContext{m_lContexts.end()};
  _TySkipArray m_rgSkipTokensCur{0};
  size_t m_nSkip{0};
  // All settings by default should produce the full file if all tokens were copied to the output (invariant).
  bool m_fFilterWhitespaceCharData{false}; // Don't return tokens for CharData composed entirely of whitespace.
  bool m_fIntegrateCDataSections{false};
  bool m_fSkipXMLDecl{false};
  bool m_fSkipComments{false};
  bool m_fSkipPI{false};
  bool m_fUpdateSkip{false};
  // XMLDecl properties:
  bool m_fStandalone{false};
  _TyStdStr m_strEncoding;
  uint8_t m_nVersionMinorNumber;
  // State:
  bool m_fGotFirstTag{false}; // Before we get the first tag CharData must contain all spaces- and if not we will throw.
};

__XMLP_END_NAMESPACE
