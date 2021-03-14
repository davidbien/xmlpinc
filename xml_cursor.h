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

#include <algorithm>
#include <array>
#include "alloca_list.h"
#include "xml_ns.h"
#include "xml_types.h"
#include "xml_traits.h"
#include "xml_parse.h"
#include "xml_context.h"
#include "xml_util.h"

__XMLP_BEGIN_NAMESPACE

// _xml_read_context
// This maintains the a single level of the current read state of the XML parser.
// The read cursor maintains a stack of these.
// We don't need to maintain file positions, etc, as these are maintained by the lexical analyzer.
// The invariant is that the parser is always located at the position at the end of the last token of the
//  tail of the stack of read contexts.
template < class t_TyXmlTraits >
class _xml_read_context
{
  typedef _xml_read_context _TyThis;
public:
  typedef t_TyXmlTraits _TyXmlTraits;
  typedef typename _TyXmlTraits::_TyChar _TyChar;
  typedef typename _TyXmlTraits::_TyStdStr _TyStdStr;
  typedef typename _TyXmlTraits::_TyStrView _TyStrView;
  typedef typename _TyXmlTraits::_TyLexTraits _TyLexTraits;
  typedef typename _TyLexTraits::_TyToken _TyLexToken;
  typedef typename _TyXmlTraits::_TyXmlToken _TyXmlToken;
  static constexpr size_t s_knbySegSize = sizeof( _TyXmlToken ) * 16;

  ~_xml_read_context() = default;
  _xml_read_context( _TyLexToken && _rrtok )
    : m_saTokens( s_knbySegSize )
  {
    AddToken( std::move( _rrtok ) );
  }
  _xml_read_context() = delete;
  _xml_read_context( _xml_read_context const & ) = delete;
  _xml_read_context & operator=( _xml_read_context const & ) = delete;
  _xml_read_context & operator=( _xml_read_context && ) = delete;

  void AssertValid( bool _fIsTail = false ) const
  {
#if ASSERTSENABLED
    size_t nSize = m_saTokens.GetSize();
    Assert( nSize ); // should always have at least the tag token at element 0.
    Assert( ( nSize <= 1 ) || _fIsTail );
#endif //ASSERTSENABLED
  }

  // Return the token ID of the tag at element 0 of m_saTokens.
  vtyTokenIdent GetTagTokenId() const
  {
    return GetTag().GetTokenId();
  }
  // This will either return an XMLDecl (for the root/head context) or a s_knTokenSTag or s_knTokenEmptyElemTag.
  const _TyXmlToken & GetTag() const
  {
    return const_cast< _TyThis * >( this )->GetTag(); // avoid detritus due to the throw being unique.
  }
  _TyXmlToken & GetTag()
  {
    VerifyThrowSz( !!m_saTokens.GetSize(), "No tag." );
    return m_saTokens.ElGet( 0 );
  }
  size_t NContentTokens() const
  {
    size_t nEls = m_saTokens.NElements();
    Assert( nEls );
    return !nEls ? 0 : ( nEls - 1 );
  }
  _TyXmlToken const & GetContentToken( size_t _nEl ) const
  {
    return m_saTokens.ElGet( _nEl+1 );
  }
  _TyXmlToken & GetContentToken( size_t _nEl )
  {
    return m_saTokens.ElGet( _nEl+1 );
  }
  _TyXmlToken const & operator []( size_t _nEl ) const
  {
    return GetContentToken( _nEl );
  }
  _TyXmlToken & operator []( size_t _nEl )
  {
    return GetContentToken( _nEl );
  }
  void ClearContent() 
  {
    if ( m_saTokens.GetSize() > 1 )
      m_saTokens.SetSizeSmaller( 1 );
  }
  // Peruse all content using a functor.
  template < class t_TyFunctor >
  void ApplyContent( size_t _nTokenBegin, size_t _nTokenEnd, t_TyFunctor && _rrftor )
  {
    m_saTokens.ApplyContiguous( _nTokenBegin+1, _nTokenEnd+1, std::forward< t_TyFunctor >( _rrftor ) );
  }
  template < class t_TyFunctor >
  void ApplyContent( size_t _nTokenBegin, size_t _nTokenEnd, t_TyFunctor && _rrftor ) const
  {
    m_saTokens.ApplyContiguous( _nTokenBegin+1, _nTokenEnd+1, std::forward< t_TyFunctor >( _rrftor ) );
  }
  // Peruse conditionally - see SegArray::NApplyContiguous().
  template < class t_TyFunctor >
  size_t NApplyContent( size_t _nTokenBegin, size_t _nTokenEnd, t_TyFunctor && _rrftor )
  {
    return m_saTokens.NApplyContiguous( _nTokenBegin+1, _nTokenEnd+1, std::forward< t_TyFunctor >( _rrftor ) );
  }
  template < class t_TyFunctor >
  size_t NApplyContent( size_t _nTokenBegin, size_t _nTokenEnd, t_TyFunctor && _rrftor ) const
  {
    return m_saTokens.NApplyContiguous( _nTokenBegin+1, _nTokenEnd+1, std::forward< t_TyFunctor >( _rrftor ) );
  }
  void AddToken( _TyLexToken && _rrtok )
  {
    m_saTokens.emplaceAtEnd( std::move( _rrtok ) );
  }
  // Coalesce all content CharData and CDataSection content tokens into a single string of the given character type.
  // The non-const version of this will store rendered strings in their elements.
  template < class t_TyString >
  void KGetContentString( t_TyString & _rstr ) const
  {
    _rstr.clear();
    size_t nContentTokens = NContentTokens();
    if ( !nContentTokens )
      return;
    // Use a AllocaArray to efficiently store pieces of the string so that we can then allocate once.
    // We will get the content strings in token's character type and then perform a single conversion at the end.
    typedef AllocaArray< _TyStdStr, true > _TyRgStrings;
    _TyRgStrings rgStrings( ALLOCA_ARRAY_ALLOC( _TyStdStr, NContentTokens() ) );
    size_t nCharsTotal = 0;
    m_saTokens.ApplyContiguous( 1, m_saTokens.NElements(),
      [&rgStrings,&nCharsTotal]( _TyXmlToken const * _ptokBegin, _TyXmlToken const * _ptokEnd )
      {
        _TyXmlToken const * ptokCur = _ptokBegin;
        for ( ; _ptokEnd != ptokCur; ++ptokCur )
        {
          vtyTokenIdent tid = ptokCur->GetTokenId();
          if ( ( tid == s_knTokenCDataSection ) || ( tid == s_knTokenCharData ) )
          {
            _TyStdStr * pstrEmplaced;
            ptokCur->GetLexToken().GetString( *( pstrEmplaced = &rgStrings.emplaceAtEnd() ) );
            nCharsTotal += pstrEmplaced->length();
          }
        }
      }
    );
    if ( rgStrings.GetSize() )
      _KGetContentString( _rstr, rgStrings.begin(), rgStrings.end() );
  }
  // The non-const version of this will store rendered strings in their elements.
  // That's this one. We will store the strings in the character type requested here in the tokens themselves.
  // This makes for faster parsing in the future when and if these same values are accessed (using the same character type as this).
  template < class t_TyString >
  void GetContentString( t_TyString & _rstr )
  {
    typedef typename t_TyString::value_type _TyCharResult;
    typedef basic_string_view< _TyCharResult > _TyStrViewResult;
    _rstr.clear();
    size_t nContentTokens = NContentTokens();
    if ( !nContentTokens )
      return;
    // Use a AllocaArray to efficiently store pieces of the string so that we can then allocate once.
    // We will get the content strings in token's character type and then perform a single conversion at the end.
    typedef AllocaArray< _TyStrViewResult, true > _TyRgStrViews;
    _TyRgStrViews rgStrViews( ALLOCA_ARRAY_ALLOC( _TyStrViewResult, NContentTokens() ) );
    size_t nCharsTotal = 0;
    m_saTokens.ApplyContiguous( 1, m_saTokens.NElements(),
      [&rgStrViews,&nCharsTotal]( _TyXmlToken * _ptokBegin, _TyXmlToken * _ptokEnd )
      {
        _TyXmlToken * ptokCur = _ptokBegin;
        for ( ; _ptokEnd != ptokCur; ++ptokCur )
        {
          vtyTokenIdent tid = ptokCur->GetTokenId();
          if ( ( tid == s_knTokenCDataSection ) || ( tid == s_knTokenCharData ) )
          {
            _TyStrViewResult * psvEmplaced;
            ptokCur->GetLexToken().GetStringView( *( psvEmplaced = &rgStrViews.emplaceAtEnd() ) );
            nCharsTotal += psvEmplaced->length();
          }
        }
      }
    );
    if ( rgStrViews.GetSize() )
    {
      _rstr.resize( nCharsTotal );
      _TyCharResult * pcCur = &_rstr[0];
      _TyStrViewResult * psvCur = rgStrViews.begin();
      _TyStrViewResult * const psvEnd = rgStrViews.end();
      for ( ; psvEnd != psvCur; ++psvCur )
      {
        memcpy( pcCur, &psvCur->at(0), psvCur->length() * sizeof( _TyChar ) );
        pcCur += psvCur->length();
      }
    }
  }

protected:
  template< class t_TyStringResult >
  void _KGetContentString( t_TyStringResult & _rstr, size_t _nCharsTotal, _TyStdStr * _pstrSrcBegin, _TyStdStr * _pstrSrcEnd )
    requires TAreSameSizeTypes_v< typename t_TyStringResult::value_type, _TyChar >
  {
    if ( 1 == ( _pstrSrcEnd - _pstrSrcBegin ) )
      _pstrSrcBegin->swap( _rstr );
    else
    {
      _rstr.resize( _nCharsTotal );
      _TyChar * pcCur = &_rstr[0];
      _TyStdStr * pstrSrcCur = _pstrSrcBegin;
      for ( ; _pstrSrcEnd != pstrSrcCur; ++pstrSrcCur )
      {
        memcpy( pcCur, &pstrSrcCur->at(0), pstrSrcCur->length() * sizeof( _TyChar ) );
        pcCur += pstrSrcCur->length();
      }
      Assert( pcCur == &_rstr[0] + _nCharsTotal );
    }
  }
  template< class t_TyStringResult >
  void _KGetContentString( t_TyStringResult & _rstr, size_t _nCharsTotal, _TyStdStr * _pstrSrcBegin, _TyStdStr * _pstrSrcEnd )
    requires ( !TAreSameSizeTypes_v< typename t_TyStringResult::value_type, _TyChar > )
  {
    _TyStdStr strTempBuf;
    static constexpr size_t knchMaxAllocaSize = vknbyMaxAllocaSize / sizeof( _TyChar );
    _TyChar * pcBuf;
    if ( _nCharsTotal > knchMaxAllocaSize )
    {
      strTempBuf.resize( _nCharsTotal );
      pcBuf = &strTempBuf[0];
    }
    else
    {
      pcBuf = (_TyChar*)alloca( _nCharsTotal * sizeof( _TyChar ) );
      _TyChar * pcCur = pcBuf;
      _TyStdStr * pstrSrcCur = _pstrSrcBegin;
      for ( ; _pstrSrcEnd != pstrSrcCur; ++pstrSrcCur )
      {
        memcpy( pcCur, &pstrSrcCur->at(0), pstrSrcCur->length() * sizeof( _TyChar ) );
        pcCur += pstrSrcCur->length();
      }
      Assert( pcCur == pcBuf + _nCharsTotal );
    }
    ConvertString( _rstr, pcBuf, _nCharsTotal );
  }

  // The first token is always:
  // 1) An XMLDecl token if we are at the head of the context list.
  // 2) A Tag token (either s_knTokenSTag or s_knTokenEmptyElemTag).
  // The remaining tokens are content for that (pseudo)tag.
  typedef SegArray< _TyXmlToken, true_type > _TySegArrayTokens;
  _TySegArrayTokens m_saTokens;
};

template < class t_TyXmlTraits >
class xml_read_cursor
{
  typedef xml_read_cursor _TyThis;
public:
  typedef t_TyXmlTraits _TyXmlTraits;
  typedef typename _TyXmlTraits::_TyXmlToken _TyXmlToken;
  typedef typename _TyXmlTraits::_TyLexTraits _TyLexTraits;
  typedef typename _TyLexTraits::_TyTpValueTraits _TyTpValueTraits;
  typedef typename _TyLexTraits::_TyTransportCtxt _TyTransportCtxt;
  typedef typename _TyLexTraits::_TyUserObj _TyUserObj;
  typedef _l_token< _TyTransportCtxt, _TyUserObj, _TyTpValueTraits > _TyLexToken;
  typedef typename _TyXmlTraits::_TyChar _TyChar;
  typedef typename _TyXmlTraits::_TyStdStr _TyStdStr;
  typedef typename _TyXmlTraits::_TyStrView _TyStrView;
  typedef typename _xml_namespace_map_traits< _TyChar >::_TyUriAndPrefixMap _TyUriAndPrefixMap;
  typedef typename _xml_namespace_map_traits< _TyChar >::_TyNamespaceUri _TyNamespaceUri;
  typedef xml_namespace_map< _TyChar > _TyXmlNamespaceMap;
  typedef xml_namespace_value_wrap< _TyChar > _TyXmlNamespaceValueWrap;
  typedef _l_value< _TyChar, _TyTpValueTraits > _TyLexValue;
  typedef _l_user_context< _TyTransportCtxt, _TyUserObj, _TyTpValueTraits > _TyUserContext;
  typedef _l_data<> _TyData;
  typedef _l_state_proto< _TyChar > _TyStateProto;
  typedef xml_parser< _TyXmlTraits > _TyXmlParser;
  friend _TyXmlParser;
  typedef _xml_read_context< _TyXmlTraits > _TyXmlReadContext;
  typedef list< _TyXmlReadContext > _TyListReadContexts;
  typedef XMLDeclProperties< _TyChar > _TyXMLDeclProperties;
  typedef _xml_document_context_transport< _TyXmlTraits > _TyXmlDocumentContext;

  ~xml_read_cursor() = default;
  xml_read_cursor() = default;
  xml_read_cursor( xml_read_cursor const & ) = delete;
  xml_read_cursor & operator=( xml_read_cursor const & ) = delete;
  xml_read_cursor( xml_read_cursor && ) noexcept = default;
  xml_read_cursor & operator=( xml_read_cursor && ) = default;

// accessors:
  _TyXmlParser & GetXmlParser() const
  {
    Assert( m_pXp );
    return *m_pXp;
  }

// State querying:
  // This is true while we are parsing inside of the element tag.
  // This is true after the first tag is read and then false
  //  after the end tag for that first tag is read.
  bool FInsideDocumentTag() const
  {
    return m_lContexts.size() > 1; // the claim is constant time since C++17.
  }
  bool FInProlog() const
  {
    return ( 1 == m_lContexts.size() ) && !m_fGotFirstTag;
  }
  bool FInEpilog() const
  {
    return ( 1 == m_lContexts.size() ) && m_fGotFirstTag;
  }
  bool FAtEOF() const
  {
    Assert( !!m_pXp ); // kinda meaningless...
    return !m_pXp || !m_pltokLookahead;
  }
// Parse settings.
  void SetStrict( bool _fStrict = true )
  {
    m_fStrict = _fStrict;
  }
  bool FGetStrict() const
  {
    return m_fStrict;
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
  bool GetSkipProcessingInstructions() const
  {
    return m_fSkipPI;
  }
  void SetIncludePrefixesInAttrNames( bool _fIncludePrefixesInAttrNames = true )
  {
    m_fIncludePrefixesInAttrNames = _fIncludePrefixesInAttrNames;
  }
  bool GetIncludePrefixesInAttrNames() const
  {
    return m_fIncludePrefixesInAttrNames;
  }
  void SetCheckDuplicateAttrs( bool _fCheckDuplicateAttrs = true )
  {
    m_fCheckDuplicateAttrs = _fCheckDuplicateAttrs;
  }
  bool GetCheckDuplicateAttrs() const
  {
    return m_fCheckDuplicateAttrs;
  }
  
  void AssertValid( bool _fIncludeContextList = false ) const
  {
  #if ASSERTSENABLED
    if ( !!m_pXp && !!m_lContexts.size() )
    {
      Assert( m_itCurContext != m_lContexts.end() ); // We should never be at the end.
      vtyTokenIdent tidTail = m_lContexts.back().GetTagTokenId();
      Assert( ( tidTail == s_knTokenXMLDecl ) == ( 1 == m_lContexts.size() ) );
      Assert( ( ( tidTail == s_knTokenSTag ) || ( tidTail == s_knTokenEmptyElemTag ) ) == ( 1 < m_lContexts.size() ) );
      vtyTokenIdent tidLookahead = !m_pltokLookahead ? vktidInvalidIdToken : m_pltokLookahead->GetTokenId();
      if ( s_knTokenSTag == tidTail )
      {
        Assert( ( s_knTokenSTag == tidLookahead ) || ( s_knTokenEmptyElemTag == tidLookahead ) || ( s_knTokenETag == tidLookahead ) );
      }
      else
      if ( s_knTokenEmptyElemTag == tidTail )
      {
        // In the case of an empty element we may have content for the parent tag after us, another tag, or the end tag of our parent.
        // So really no restrictions at all... lol.
        Assert( _FIsContentToken( tidLookahead ) || ( ( s_knTokenSTag == tidLookahead ) || ( s_knTokenEmptyElemTag == tidLookahead ) || ( s_knTokenETag == tidLookahead ) ) );
      }
      if ( _fIncludeContextList )
      {
        typename _TyListReadContexts::const_iterator cit = m_lContexts.begin();
        typename _TyListReadContexts::const_iterator citEnd = m_lContexts.end();
        for ( ; citEnd != cit; ++cit )
          cit->AssertValid( &*cit == &m_lContexts.back() );
      }
    }
  #endif //ASSERTSENABLED
  }

  _TyXmlReadContext &
  GetContextCur()
  {
    Assert( m_lContexts.size() );
    AssertValid();
    return *m_itCurContext;
  }

  _TyXmlToken & GetXMLDeclToken( _TyXMLDeclProperties * _pXMLDeclProperties = 0 )
  {
    Assert( m_pXp );
    if ( _pXMLDeclProperties )
      *_pXMLDeclProperties = m_XMLDeclProperties;
    return m_lContexts.front().GetTag();
  }
  _TyXmlToken & XMLDeclAcquireDocumentContext( _TyXmlDocumentContext & _rxdcDocumentContext )
  {
    _rxdcDocumentContext.m_XMLDeclProperties = std::move( m_XMLDeclProperties );
    // Now transfer/copy over the various objects from the parser to allow this object to exist solo - while,
    //  of course, messing with the parser state. So we must make sure to disconnect the parser after we are done
    //  with this.
    // The UserObj must be transferred because each token has a context which has a reference to this user object - i.e. the user object cannot be copied.
    _rxdcDocumentContext.m_upUserObj = std::move( GetXmlParser().GetUserObjPtr() );
    // We can bopy both the uri map and the prefix map but since the parser will be useless for parsing after this why not just transfer them:
    _rxdcDocumentContext.m_mapUris = std::move( GetXmlParser().GetUriMap() );
    _rxdcDocumentContext.m_mapPrefixes = std::move( GetXmlParser().GetPrefixMap() );
    // Some transports don't require moving. Also the var_transport will report if it's current transport requires moving. We won't touch the transport but we may need it to remain open.
    if ( GetXmlParser().GetTransport().FDependentTransportContexts() )
      _rxdcDocumentContext.m_opttpImpl.emplace( std::move( GetXmlParser().GetTransport() ) );
    return m_lContexts.front().GetTag();
  }

// Navigation:
  // Move down - i.e. to the next subtag.
  // If the context is already present - i.e. we are not at the end of the context list - then we just move the context pointer and return true.
  // If we are at the end of the context list then this has some side effects:
  // 1) If successful then all content tokens in the current context are cleared. There are operational reasons for this as well as logical reasons.
  //    a) Then the next tag is read, a new context is created below the current one, and all content tokens for that next tag are read.
  // 2) If not sucessful then there is no change to any state - i.e. nothing is read or processed. To move further from here 
  bool FMoveDown()
  {
    AssertValid( true );
    Assert( !!m_pltokLookahead );
    if ( !_FAtTailContext() )
    {
      ++m_itCurContext;
      return true;
    }
    // At the tail context.
    vtyTokenIdent tidLookahead = !m_pltokLookahead ? vktidInvalidIdToken : m_pltokLookahead->GetTokenId();
    vtyTokenIdent tidTail = m_itCurContext->GetTagTokenId();
    if ( ( s_knTokenEmptyElemTag == tidTail ) || ( tidLookahead == s_knTokenETag ) || ( vktidInvalidIdToken == tidLookahead ) )
      return false;
    
    // Able to move down. First process the next tag:
    _ProcessToken( m_pltokLookahead );
    Assert( !_FAtTailContext() ); // A new context should have been inserted below us.
    Assert( m_pltokLookahead->FIsNull() ); // We should have moved this token away...
    m_pltokLookahead.reset(); // Rid the null token object.
    // Now we must read the lookahead to prime the ending.
    VerifyThrowSz( _FGetToken( m_pltokLookahead ) || ( ( s_knTokenEmptyElemTag == tidLookahead ) && ( m_lContexts.size() == 2 ) ), "Premature EOF." );
    _ReadTokenContent();
    // Now remove the token content from our current context (the old tail) and move down to our newly read context.
    m_itCurContext->ClearContent();
    ++m_itCurContext;
    return true; // We moved down.
  }
  // Just return if we could move down if we wanted to.
  bool FCanMoveDown() const
  {
    AssertValid( true );
    Assert( !!m_pltokLookahead );
    return !_FAtTailContext() || ( ( m_itCurContext->GetTagTokenId() != s_knTokenEmptyElemTag ) && !!m_pltokLookahead && ( m_pltokLookahead->GetTokenId() != s_knTokenETag ) );
  }
  // Move up in the context list. This allows moving all the way up to the XMLDecl context at the front of the list.
  bool FMoveUp()
  {
    AssertValid( true );
    Assert( !!m_pltokLookahead );
    if ( m_itCurContext != m_lContexts.begin() )
    {
      --m_itCurContext;
      return true;
    }
    return false;
  }
  // Return if we could move up from our current position.
  bool FCanMoveUp() const
  {
    AssertValid( true );
    Assert( !!m_pltokLookahead );
    return m_itCurContext != m_lContexts.begin();
  }
  // This moves to the next tag at the current level of the context iterator. This may result in tags and context being skipped if
  //  we are not located at a leaf tag within the XML document.
  // NOTE: THIS METHOD ALWAYS RESULTS IN A CHANGE IN CONTEXT - unless we are already at EOF.
  // 1) On a false return m_itCurContext has been moved one level up towards the front of the list.
  //  a) If we were already at the XMLDecl tag at the front of the list then this ends the iteration entirely and all data structures are left empty.
  //  a) If we are not at the front of the list then this will read all data up until the end of the tag of the current level.
  // 1) 
  typedef optional< _TyXmlToken > _TyOptXmlToken; // no default constructor for _TyXmlToken.
  bool FNextTag( _TyOptXmlToken * _popttokRtnSpentTag = nullptr )
  {
    AssertValid( true );
    if ( !m_pltokLookahead )
      return false; // at eof.
    vtyTokenIdent tidCur = m_itCurContext->GetTagTokenId();
    if ( s_knTokenEmptyElemTag == tidCur )
    {
      Assert( _FAtTailContext() ); // must be.
      // Regardless we will be ending the current token. Do it.
      --m_itCurContext; // Must be able to do this because we know that at least XMLDecl is above us.
      _PopContext( _popttokRtnSpentTag );
      return _FProcessNextTag();
    }
    if ( s_knTokenSTag == tidCur )
    {
      // We will read until the end of this tag on the current level of the iterator.
      // Then we will check to see what is beyond that to decide whether we will go up or stay at the current level of the iterator.
      if ( !_FAtTailContext() || ( s_knTokenETag != m_pltokLookahead->GetTokenId() ) )
      {
        // Then we need to get to the point where the above conditions hold.
        _SkipToEndCurrentContext(); // Skip potentially a ton of XML.
      }
      Assert( _FAtTailContext() && ( s_knTokenETag == m_pltokLookahead->GetTokenId() ) );
      // Process the end tag and then consume it.
      _ProcessLookahead("Premature EOF.");
      --m_itCurContext;
      _PopContext( _popttokRtnSpentTag );
      Assert( _FAtTailContext() );
      return _FProcessNextTag();
    }
    Assert( s_knTokenXMLDecl == tidCur );
    if ( !_FAtTailContext() )
      _SkipToEndCurrentContext(); // Skip potentially a ton of XML.
    Assert( _FAtTailContext() );
    Assert( m_lContexts.size() == 1 );
    return false;
  }
  template < class t_TyFunctor >
  void ApplyAllContent( t_TyFunctor && _rrftor )
  {
    _TyXmlReadContext & rxrcxt = GetContextCur();
    rxrcxt.ApplyContent( 0, rxrcxt.NContentTokens(), std::forward< t_TyFunctor >( _rrftor ) );
  }
  void ClearContent()
  {
    GetContextCur().ClearContent();
  }
  
protected:
  bool _FProcessNextTag()
  {
    if ( !m_pltokLookahead )
      return false;
    if ( ( s_knTokenSTag == m_pltokLookahead->GetTokenId() ) || ( s_knTokenEmptyElemTag == m_pltokLookahead->GetTokenId() ) )
    {
      _ProcessLookahead("Premature EOF.");
      Assert( !_FAtTailContext() ); // A new context should have been inserted below us.
      _ReadTokenContent();
      ++m_itCurContext;
      return true; // We remained at the current level and read in a tag and its contents.
    }
    else
    {
      // Read any token content but don't read any end tag token - that requires another call to FNextTag().
      _ReadTokenContent();
      return false; // We moved to the context above us.
    }
  }
  void _ProcessLookahead( const char * _pszMesg )
  {
    vtyTokenIdent tid = m_pltokLookahead->GetTokenId();
    _ProcessToken( m_pltokLookahead );
    Assert( m_pltokLookahead->FIsNull() ); // We should have moved this token away...
    m_pltokLookahead.reset(); // Rid the null token object.
    // Now we must read the lookahead to prime the ending.
    VerifyThrowSz( _FGetToken( m_pltokLookahead ) || ( ( tid == s_knTokenETag ) && ( m_lContexts.size() == 2 ) ), _pszMesg );
  }

  void _SkipToEndCurrentContext()
  {
    Assert( !_FAtTailContext() || ( s_knTokenETag != m_pltokLookahead->GetTokenId() ) );
    // Setup to skip all tokens except for start tags and end tags.
    vtyTokenIdent rgSkipTokens[] = { s_knTokenEmptyElemTag, s_knTokenComment, s_knTokenCDataSection, s_knTokenCharData, s_knTokenProcessingInstruction };
    size_t nSkipTokens = DimensionOf( rgSkipTokens );
    // Tell the xml_user_obj to filter all token data so that we don't pass around data that we are going to throw away.
    m_pXp->_SetFilterAllTokenData( true );
    m_fSkippingTags = true; // This optimizes processing a bit.
    while( !_FAtTailContext() )
    {
      // Each token will be either an end tag or a beginning tag.
      if ( s_knTokenETag == m_pltokLookahead->GetTokenId() )
        _ProcessEndTag( *m_pltokLookahead );
      else
      {
        _ProcessTagName( (*m_pltokLookahead)[vknTagNameIdx] );
        _PushNewContext( *m_pltokLookahead );
      }
      Assert( m_pltokLookahead->FIsNull() ); // We should have moved this token away...
      m_pltokLookahead.reset(); // Rid the null token object.
      VerifyThrowSz( m_pXp->GetLexicalAnalyzer().FGetToken( m_pltokLookahead, rgSkipTokens, rgSkipTokens + nSkipTokens, nullptr, false ), "Premature EOF." );
    }
    Assert( !m_pltokLookahead || ( s_knTokenETag == m_pltokLookahead->GetTokenId() ) ); // Should always be the case or we have a bug.
    m_pXp->_SetFilterAllTokenData( false );
    m_fSkippingTags = false; // This optimizes processing a bit.
  }
  void _InitStartStates()
    requires( is_same_v< _TyChar, char32_t > )
  {
    m_pspStartXmlDecl = (const _TyStateProto *)&startUTF32XMLDecl< _TyLexTraits >;
    m_pspStartAll = (const _TyStateProto *)&startUTF32All< _TyLexTraits >;
  }
  void _InitStartStates()
    requires( is_same_v< _TyChar, char16_t > )
  {
    m_pspStartXmlDecl = (const _TyStateProto *)&startUTF16XMLDecl< _TyLexTraits >;
    m_pspStartAll = (const _TyStateProto *)&startUTF16All< _TyLexTraits >;
  }
  void _InitStartStates()
    requires( is_same_v< _TyChar, char8_t > )
  {
    m_pspStartXmlDecl = (const _TyStateProto *)&startUTF8XMLDecl< _TyLexTraits >;
    m_pspStartAll = (const _TyStateProto *)&startUTF8All< _TyLexTraits >;
  }
  void _AttachXmlParser( _TyXmlParser * _pXp )
  {
    Assert( !!_pXp );
    VerifyThrow( !!_pXp );
    if ( !!m_pXp )
      _DetachXmlParser();
    _InitStartStates();
    m_pXp = _pXp;
    m_pXp->_InitMapsAndUserObj();
    _InitNamespaceMap();
    m_pXp->_SetFilterWhitespaceCharData( m_fFilterWhitespaceCharData );
    { //B
      unique_ptr< _TyLexToken > pltokFirst;
      // For the first token we must check to see if we get an XMLDecl token since it is optional.
      bool fGotXMLDecl = _FGetToken( pltokFirst, m_pspStartXmlDecl );
      if ( !fGotXMLDecl )
      {
        m_pXp->GetStream().ResetToTokenStart(); // XMLDecl is optional.
        // Then we are going to create a "fake" XMLDecl token because the invariant is that the first
        //  token of the top context is an XMLDecl token. Our fake token will be completely empty.
        _TyUserContext ucxtEmpty( m_pXp->GetUserObj() );
        pltokFirst = make_unique< _TyLexToken >( std::move( ucxtEmpty ), &m_pXp->GetLexicalAnalyzer().GetActionObj< s_knTokenXMLDecl >() );
      }
      _ProcessToken( pltokFirst ); // This pushes XMLDecl onto the head of the stack as the first token in that context.
    }//EB
    // Now "prime the pump" by reading the read-ahead token and then call _ReadTokenContent().
    VerifyThrow( _FGetToken( m_pltokLookahead ) );
    _ReadTokenContent();
  }
  void _InitNamespaceMap()
  {
    Assert( !!m_pXp );
    m_mapNamespaces.Init( *m_pXp );
  }
  void _DetachXmlParser()
  {
    m_pXp = nullptr;
    m_lContexts.clear();
    m_itCurContext = m_lContexts.end();
    // Must clear all namespaces, prefixes, URIs, entities and parameter entities.
    m_XMLDeclProperties.clear();
    m_mapNamespaces.clear();
  }
  bool _FGetToken( unique_ptr< _TyLexToken > & _rpltok, const _TyStateProto * _pspStart = nullptr )
  {
    Assert( !!m_pXp );
    if ( m_fUpdateSkip )
      _UpdateSkip();
    if ( !_pspStart )
      _pspStart = m_pspStartAll;
    return m_pXp->GetLexicalAnalyzer().FGetToken( _rpltok, &*m_rgSkipTokensCur.begin(), &*m_rgSkipTokensCur.begin() + m_nSkip, _pspStart, false );
  }
  bool _FIsContentToken( vtyTokenIdent _tid ) const
  {
    return ( s_knTokenComment == _tid ) || ( s_knTokenCDataSection == _tid ) || ( s_knTokenCharData == _tid ) || ( s_knTokenProcessingInstruction == _tid );
  }
  void _ReadTokenContent()
  {
    if ( !m_pltokLookahead || ( s_knTokenEmptyElemTag == m_lContexts.back().GetTagTokenId() ) )
      return; // At eof or tag with no content.
    // The invarient is that we have a token in the m_pltokLookahead or we have hit eof.
    // After the last end token of the file, the Misc production is active - which means
    //  that whitespace CharData, Comments and PIs are still allowed.
    // So once that last end tag is read we fill the XMLDecl tag with the epilogue Misc tokens.
    // The user is left with that which can then be read and the iteration is over.
    while( _FIsContentToken( m_pltokLookahead->GetTokenId() ) )
    {
      _ProcessToken( m_pltokLookahead );
      Assert( m_pltokLookahead->FIsNull() ); // We should have moved this token away...
      m_pltokLookahead.reset(); // Rid the null token object.
      bool fGotToken = _FGetToken( m_pltokLookahead );
      VerifyThrowSz( fGotToken || FInEpilog(), "Hit premature EOF." );
      if ( !fGotToken )
        break;
    }
  }
  bool _FAtTailContext() const
  {
    Assert( !m_lContexts.empty() );
    return m_lContexts.empty() ? false : ( &*m_itCurContext == &m_lContexts.back() );
  }
  void _PushNewContext( _TyLexToken & _rltok )
  {
    m_lContexts.emplace_back( std::move( _rltok ) );
    if ( 1 == m_lContexts.size() )
      m_itCurContext = m_lContexts.begin();
  }
  void _AppendContextContent( _TyLexToken & _rltok )
  {
    m_lContexts.back().AddToken( std::move( _rltok ) );
  }
  void _PopContext( _TyOptXmlToken * _popttokRtnSpentTag )
  {
    if ( _popttokRtnSpentTag )
    {
      *_popttokRtnSpentTag = std::move( m_lContexts.back().GetTag() );
      if ( m_fUseXMLNamespaces )
      {
        // Then we must check if we have any namespace declarations in this tag and if so undeclare them.
        _TyLexValue const & rrgvName = (**_popttokRtnSpentTag)[vknTagNameIdx];
        size_t nNamespaceDecls = rrgvName[vknTagName_NNamespaceDeclsIdx].GetVal< size_t >();
        if ( nNamespaceDecls )
        {
          // Move through looking for namespace declarations and undeclare them. No need to do this in reverse.
          typename _TyLexValue::_TySegArrayValues & rsaAttrs = (**_popttokRtnSpentTag)[vknAttributesIdx].GetValueArray();
          (void)rsaAttrs.NApplyContiguous( 0, rsaAttrs.NElements(), 
            [&nNamespaceDecls]( _TyLexValue * _pattrBegin, _TyLexValue * _pattrEnd ) -> size_t
            {
              _TyLexValue * pattrCur = _pattrBegin;
              for ( ; ( _pattrEnd != pattrCur ) && !!nNamespaceDecls; ++pattrCur )
              {
                _TyLexValue & rvNamespace = (*pattrCur)[vknNamespaceIdx];
                if ( rvNamespace.FIsA<_TyXmlNamespaceValueWrap>() )
                {
                  _TyXmlNamespaceValueWrap & rxnvw = rvNamespace.GetVal< _TyXmlNamespaceValueWrap >();
                  if ( rxnvw.FIsNamespaceDeclaration() )
                  {
                    --nNamespaceDecls;
                    rxnvw.ResetNamespaceDecls();
                  }
                }
              }
              return nNamespaceDecls ? ( _pattrEnd - _pattrBegin ) : 0; // only continue iterating if we still have namespace decls left.
            }
          );
        }
      }
    }
    m_lContexts.pop_back();
  }
  void _ProcessToken( unique_ptr< _TyLexToken > & _rpltok )
  {
    // 1) If a tag then we need to process namespaces.
    // 2) If CharData or CDATASection then we may need to read a token ahead to see if we need to combine, etc.
    switch( _rpltok->GetTokenId() )
    {
      case s_knTokenXMLDecl:
        _ProcessXMLDecl( *_rpltok );
      break;
      case s_knTokenSTag:
      case s_knTokenEmptyElemTag:
      {
        m_fGotFirstTag = true;
        if ( m_fUseXMLNamespaces )
          _ProcessNamespaces( *_rpltok );
        else
          _ProcessNoNamespaces( *_rpltok );
        _PushNewContext( *_rpltok );
      }
      break;
      case s_knTokenETag:
        _ProcessEndTag( *_rpltok );
      break;
      case s_knTokenCDataSection:
        _ProcessCDataSection( *_rpltok );
      break;
      case s_knTokenCharData:
        _ProcessCharData( *_rpltok );
      break;
      case s_knTokenComment:
        _ProcessComment( *_rpltok );
      break;
      case s_knTokenProcessingInstruction:
        _ProcessProcessingInstruction( *_rpltok );
      break;
    }
  }
  void _ProcessEndTag( _TyLexToken & _rltok )
  {
    VerifyThrowSz( FInsideDocumentTag(), "Found end tag before first tag declaration." );
    _TyLexValue & rvalTagEnd = _rltok.GetValue();
    _ProcessTagName( rvalTagEnd ); // No need to process namespaces on the end tag - just the name itself.
    const _TyXmlToken & rxtTagStart = m_lContexts.back().GetTag();
    Assert( rxtTagStart.FIsTag() );
    const _TyLexToken & rltokTagStart = rxtTagStart.GetLexToken();
    const _TyLexValue & rvalTagStart = rltokTagStart.GetValue()[vknTagNameIdx];
    _TyStrView svTagStart;
    rltokTagStart.KGetStringView( svTagStart, rvalTagStart[vknNameIdx] );
    _TyStrView svTagEnd;
    _rltok.KGetStringView( svTagEnd, rvalTagEnd[vknNameIdx] );
    VerifyThrowSz( svTagStart == svTagEnd, "Start tag[%s] doesn't match end tag[%s]", StrConvertString<char>( svTagStart ).c_str(), StrConvertString<char>( svTagEnd ).c_str() );
    // The context is always popped later in FNextTag().
    _TyLexToken ltokEat( std::move( _rltok ) ); // eat the token.
  }
  void _ProcessXMLDecl( _TyLexToken & _rltok )
  {
    // Pull the info we want out of it to store in the cursor.
    m_XMLDeclProperties.FromXMLDeclToken( _rltok );
    Assert( !m_lContexts.size() );
    // Even if the caller is ignoring the XMLDecl we will start the root context with it if it is present.
    _PushNewContext( _rltok ); // Push on current context. There will be no current context so one will be created.
  }
  void _ProcessComment( _TyLexToken & _rltok )
  {
    _l_data_range drToken;
    _rltok.GetTokenDataRange( drToken );
    drToken.m_posBegin += 4; // <!--
    drToken.m_posEnd -= 3; // -->
    // REVIEW:<dbien>: Could strip whitespace off front and rear but...
    _rltok.template emplaceValue< _TyData >( drToken );
    _AppendContextContent( _rltok );
  }
  void _ProcessProcessingInstruction( _TyLexToken & _rltok )
  {
    // We may have some "meat" after the PITarget - or not.
    vtyDataPosition posMeatBegin = _rltok[vknProcessingInstruction_MeatIdx].GetVal< vtyDataPosition >();
    if ( vkdpNullDataPosition == posMeatBegin )
    {
      // Then insert a null _TyData:
      _rltok[vknProcessingInstruction_MeatIdx].template emplaceArgs< _TyData >();
    }
    else
    {
      _l_data_range drToken;
      _rltok.GetTokenDataRange( drToken );
      _rltok[vknProcessingInstruction_MeatIdx].template emplaceArgs< _TyData >( posMeatBegin, drToken.end()-2, s_kdtPlainText, s_knTriggerPITargetMeatBegin ); // subtract "?>".
    }
    _AppendContextContent( _rltok );
  }
  void _ProcessCDataSection( _TyLexToken & _rltok )
  {
    VerifyThrowSz( FInsideDocumentTag(), "Found CDataSection before first tag declaration." );
    _l_data_range drToken;
    _rltok.GetTokenDataRange( drToken );
    drToken.m_posBegin += 9; // <![CDATA[
    drToken.m_posEnd -= 3; // ]]>
    _rltok.template emplaceValue< _TyData >( drToken );
    _AppendContextContent( _rltok );
  }
  void _ProcessCharData( _TyLexToken & _rltok )
  {
    if ( !FInsideDocumentTag() )
    {
      // We should only see plain whitespace CharData before the first tag:
      _TyLexValue & rVal = _rltok.GetValue();
      Assert( rVal.FHasTypedData() );
      const _TyData & rdt = rVal.GetVal< _TyData >();
      bool fError = !rdt.FContainsSingleDataRange() || ( rdt.DataRangeGetSingle().type() != s_kdtPlainText );
      if ( !fError )
      {
        _TyStrView svData;
        rVal.KGetStringView( _rltok, svData );
        fError = ( svData.length() != StrSpn( &svData[0], svData.length(), (const _TyChar *)str_array_cast<_TyChar>( STR_XML_WHITESPACE_TOKEN ) ) );
      }
      VerifyThrowSz( !fError, "Found non-whitespace CharData before first tag declaration." );
    }
    _AppendContextContent( _rltok ); // Push on current context.
  }
  void _ProcessTagName( _TyLexValue & _rrgvalName )
  {
    // Tag names must match literally between start and end tags.
    // As well we want the end user to see a uniform representation of tag names.
    // So if a namespace specifier is present then we will reset the end position of the this namespace specifier to include the ':' and the namespace name.
    _TyData const & krdtName = _rrgvalName[vknNamespaceIdx].GetVal< _TyData >();
    if ( !krdtName.FIsNull() )
    {
      _TyData & rdtPrefix = _rrgvalName[vknNameIdx].GetVal< _TyData >();
      rdtPrefix.DataRangeGetSingle().m_posEnd = krdtName.DataRangeGetSingle().m_posEnd;
    }
    // Leave the name as is - further processing will occur depending on flavor.
  }
#if 0 // unused.
  template < class t_TyTransportCtxt >
  _TyNamespaceUri & _RLookupNamespacePrefix( _l_data_typed_range const & _rdr, t_TyTransportCtxt const & _rcxt )
  {
    _TyStrView sv;
    _rcxt.GetStringView( sv, _rdr );
    typename _TyXmlNamespaceMap::const_iterator cit = m_mapNamespaces.find( sv );
    if ( m_mapNamespaces.end() == cit )
      THROWXMLPARSEEXCEPTION("Can't find namespace prefix [%s].", StrConvertString<char>( sv ).c_str() );
    return cit->second;
  }
#endif //0
  // Process any namespace declarations in this token and then process any namespace references.
  void _ProcessNamespaces( _TyLexToken & _rltok )
  {
    // There is this annoying bit of verbiage:
    // "The prefix xml is by definition bound to the namespace name http://www.w3.org/XML/1998/namespace. It MAY, but need not, be declared, 
    //    and MUST NOT be bound to any other namespace name. Other prefixes MUST NOT be bound to this namespace name, and it MUST NOT be declared as the default namespace."
    // I don't think I am going to worry about this actually.
    // First thing to do is to move through all attributes and obtain all namespace declarations and at the same time ensure they are valid.
    typedef AllocaList< const typename _TyUriAndPrefixMap::value_type *, false > _TyListPrefixes;
    _TyListPrefixes lPrefixesUsed; // Can't declare the same prefix in the same tag.
    _TyLexValue & rrgAttrs = _rltok[vknAttributesIdx];
    size_t nNamespaceDecls = 0; // Count these so we can store with the tag and then know when we have finished processing on end-tag.
    const size_t knAttrs = rrgAttrs.GetSize();
    for ( size_t nAttr = 0; nAttr < knAttrs; ++nAttr )
    {
      _TyLexValue & rrgAttr = rrgAttrs[nAttr];
      _TyStrView svName;
      rrgAttr[vknNameIdx].KGetStringView( _rltok, svName );
      if ( !svName.compare( str_array_cast< _TyChar >("xmlns") ) )
      {
        ++nNamespaceDecls;
        // We have a namespace declaration!
        _TyStrView svPrefix;
        rrgAttr[vknNamespaceIdx].KGetStringView( _rltok, svPrefix );
        const _TyUriAndPrefixMap::value_type & rvtPrefix = m_pXp->RStrAddPrefix( svPrefix );
        // Check for uniqueness of prefix:
        VerifyThrowSz( !lPrefixesUsed.FFind( &rvtPrefix ), "Namespaces Validity: Cannot use same namespace prefix more than once within the same tag." );
        ALLOCA_LIST_PUSH( lPrefixesUsed, &rvtPrefix );
        _TyStrView svUri;
        rrgAttr[vknAttr_ValueIdx].KGetStringView( _rltok, svUri );
        _ProcessTagName( rrgAttr ); // For xmlns attr names we process the same as we process the tag name. We want to leave both "xmlns" and any colon and prefix in the attr name.
        // Add the (prefix,uri) pair and put a _TyXmlNamespaceValueWrap in the _l_value to track its lifetime:
        rrgAttr[vknNamespaceIdx].emplaceVal( m_mapNamespaces.GetNamespaceValueWrap( *m_pXp, svPrefix, svUri ) );
      }
    }
    // Now parse the namespace on the tag:
    {//B
      _TyLexValue & rrgTagName = _rltok[vknNameIdx];
      rrgTagName[vknTagName_NNamespaceDeclsIdx].template emplaceArgs< size_t >( nNamespaceDecls ); // record the count of namespace decls so we can know when we are done processing the end-tag.
      _TyData const & krdtName = rrgTagName[vknNamespaceIdx].GetVal< _TyData >();
      if ( !krdtName.FIsNull() )
      {
        _TyData & rdtPrefix = rrgTagName[vknNameIdx].GetVal< _TyData >();
        _TyStrView svPrefix;
        rrgTagName[vknNameIdx].KGetStringView( _rltok, svPrefix );
        // Update the tag name so that it includes the prefix as is necessary for XML end tag matching.
        rdtPrefix.DataRangeGetSingle().m_posEnd = krdtName.DataRangeGetSingle().m_posEnd;
        rrgTagName[vknNamespaceIdx].emplaceVal( m_mapNamespaces.GetNamespaceValueWrap( &svPrefix ) );
      }
      else
      {
        // If we have a default namespace then we need to apply it to the tag.
        _TyXmlNamespaceValueWrap xnvw;
        if ( !m_mapNamespaces.FHasDefaultNamespace( &xnvw ) )
        {
          // If there is no namespace at all then we just put a "false" in the 1th position:
          rrgTagName[vknNamespaceIdx].emplaceVal( false );
        }
        else
        {
          // Setup the xml_namespace_value_wrap in the 1th position as is standard:
          rrgTagName[vknNamespaceIdx].emplaceVal( std::move( xnvw ) );
        }
      }
    }//EB
    // Now process each attribute name, looking up prefixes wherever any prefixes exist.
    for ( size_t nAttr = 0; nAttr < knAttrs; ++nAttr )
    {
      _TyLexValue & rrgAttr = rrgAttrs[nAttr];
      if ( rrgAttr[vknNamespaceIdx].FIsA< _TyXmlNamespaceValueWrap >() )
        continue; // This is an xmlns attribute that was processed above.
      if ( rrgAttr[vknNamespaceIdx].FEmptyTypedData() )
      {
        // No default namespaces for attribute names - this has no namespace:
        rrgAttr[vknNamespaceIdx].emplaceVal( false );
        continue;
      }
      _TyStrView svPrefix;
      rrgAttr[vknNameIdx].KGetStringView( _rltok, svPrefix );
      _TyData const & krdtName = rrgAttr[vknNamespaceIdx].GetVal< _TyData >();
      _TyData & rdtPrefix = rrgAttr[vknNameIdx].GetVal< _TyData >();
      if ( m_fIncludePrefixesInAttrNames && !m_fCheckDuplicateAttrs )
        rdtPrefix.DataRangeGetSingle().m_posEnd = krdtName.DataRangeGetSingle().m_posEnd;
      else
        rdtPrefix.DataRangeGetSingle() = krdtName.DataRangeGetSingle(); // The user just wants the name without the prefix.
      rrgAttr[vknNamespaceIdx].emplaceVal( m_mapNamespaces.GetNamespaceValueWrap( &svPrefix ) );
    }
    // Now if the user wants to check for duplicate attributes - before we update the attribute names to include the prefixes.
    if ( m_fCheckDuplicateAttrs )
      _CheckDuplicateAttrs( _rltok );

    if ( m_fIncludePrefixesInAttrNames && m_fCheckDuplicateAttrs )
    {
      // Now we must post-process to pull back the view of the attribute name to pull in the prefix that is present in some cases.
      for ( size_t nAttr = 0; nAttr < knAttrs; ++nAttr )
      {
        _TyLexValue & rrgAttr = rrgAttrs[nAttr];
        if ( !rrgAttr[vknNamespaceIdx].FIsA< _TyXmlNamespaceValueWrap >() )
        {
          Assert( rrgAttr[vknNamespaceIdx].FIsA< bool >() && !rrgAttr[vknNamespaceIdx].GetVal< bool >() ); // Not in any namespace at all.
          continue; // This is an xmlns attribute that was processed above.
        }
        _TyXmlNamespaceValueWrap const & rnvw = rrgAttr[vknNamespaceIdx].GetVal< _TyXmlNamespaceValueWrap >();
        if ( rnvw.FIsNamespaceDeclaration() )
          continue;
        _TyData & rdtName = rrgAttr[vknNameIdx].GetVal< _TyData >();
        rdtName.DataRangeGetSingle().m_posBegin -= 1 + rnvw.PVtNamespaceMapValue()->first.length(); // push back to include the prefix.
      }
    }
  }  
  // This pass made when namespace are not being validated. Because the productions separate out the namespace prefixes still,
  //  we must combine the name and the prefix and as well indicate that they are in no namespace by setting the 1th element to false.
  void _ProcessNoNamespaces( _TyLexToken & _rltok )
  {
    _ProcessTagName( _rltok[vknTagNameIdx] );
    _TyLexValue & rrgAttrs = _rltok[vknAttributesIdx];
    const size_t knAttrs = rrgAttrs.GetSize();
    for ( size_t nAttr = 0; nAttr < knAttrs; ++nAttr )
    {
      _TyLexValue & rrgAttr = rrgAttrs[nAttr];
      // The name will include the prefix and the colon when we are not namespace aware.
      _ProcessTagName( rrgAttr ); 
      rrgAttr[vknNamespaceIdx].emplaceVal( false ); // This indicates that there is no namespace.
    }
    {//B - indicate there is no namespace on the tag.
      _TyLexValue & rrgTagName = _rltok[vknTagNameIdx];
      _ProcessTagName( rrgTagName ); 
      rrgTagName[vknNamespaceIdx].emplaceVal( false );
      rrgTagName[vknTagName_NNamespaceDeclsIdx].template emplaceArgs< size_t >( 0 ); // record the count of namespace decls so we can know when we are done processing the end-tag.
    }//EB
    if ( m_fCheckDuplicateAttrs )
      _CheckDuplicateAttrs(  _rltok );
  }
  void _CheckDuplicateAttrs( _TyLexToken & _rltok )
  {
    return CheckDuplicateTokenAttrs( m_fStrict, _rltok, false );
  }
  void _UpdateSkip()
  {
    if ( !m_fUpdateSkip )
      return;
    m_fUpdateSkip = false;
    vtyTokenIdent * ptidCur = &*m_rgSkipTokensCur.begin();
    if ( m_fSkipComments )
      *ptidCur++ = s_knTokenComment;
    if ( m_fSkipPI )
      *ptidCur++ = s_knTokenProcessingInstruction;
    m_nSkip = ptidCur - &*m_rgSkipTokensCur.begin();
  }
  static constexpr size_t s_knMaxSkippedTokens = 32; // random number.
  typedef array< vtyTokenIdent, s_knMaxSkippedTokens > _TySkipArray;

// DS:
  _TyXmlParser * m_pXp{nullptr}; // the pointer to the XML parser we are reading tokens from.
  const _TyStateProto * m_pspStartXmlDecl{nullptr};
  const _TyStateProto * m_pspStartAll{nullptr};
  _TyListReadContexts m_lContexts;
  unique_ptr< _TyLexToken > m_pltokLookahead;
  typename _TyListReadContexts::iterator m_itCurContext{m_lContexts.end()};
  _TyXmlNamespaceMap m_mapNamespaces;
// State:
  // The current skip token array and the number of elements in it. This is passed to the lexical analyzer to cut token processing off optimally.
  _TySkipArray m_rgSkipTokensCur{0};
  size_t m_nSkip{0};
  // Whether we have read the first tag of the file yet:
  bool m_fGotFirstTag{false};
  // This is set to true when we are skipping sets of tags due to FNextTag() or FNextToken() being called at a level higher than a leaf tag.
  bool m_fSkippingTags{false}; 
  // This is set to true when one of the "m_fSkip..." settings changes. We will then update m_rgSkipTokensCur and m_nSkip right before checking for more tokens.
  bool m_fUpdateSkip{false};
// Settings:
  // All settings by default should produce the full file if all tokens were copied to the output (invariant).
  // If this is false then we will not check for duplicate attributes.
  bool m_fCheckDuplicateAttrs{true};
  // If this is true then we will process namespaces, otherwise we will still show the prefixes but not apply XML Namespace validation and logic.
  bool m_fUseXMLNamespaces{true};
  // This option allows the user to either see the prefixes that are present in the attr names or see just the names.
  bool m_fIncludePrefixesInAttrNames{false};
  // If m_fStrict then we will not be lenient in cases that may be ambiguous wrt to the XML Specifications.
  bool m_fStrict{false};
  // Don't return tokens for CharData composed entirely of whitespace.
  bool m_fFilterWhitespaceCharData{false};
// Skip settings: Ignore the corresponding token types and don't return to the user.
  bool m_fSkipComments{false};
  bool m_fSkipPI{false};
  _TyXMLDeclProperties m_XMLDeclProperties;
};

// xml_read_cursor_var:
// A variant that is able to hold multiple *character types* of same transports.
// t_TyTpTransports: Is a tuple<> pack of transport types that the cursor will support.
template < class t_TyTpTransports >
class xml_read_cursor_var
{
  typedef xml_read_cursor_var _TyThis;
public:
  typedef t_TyTpTransports _TyTpTransports;
  typedef MultiplexTuplePack_t< TGetXmlTraitsDefault, _TyTpTransports > _TyTpXmlTraits;
  // Define our variant type - there is no monostate for this (currently - we'll see if we need it).
  typedef MultiplexTuplePack_t< xml_read_cursor, _TyTpXmlTraits, variant > _TyVariant;
  typedef xml_token_var< _TyTpTransports > _TyXmlTokenVar;
  typedef optional< _TyXmlTokenVar > _TyOptXmlTokenVar;
  typedef _xml_document_context_transport_var< _TyTpTransports > _TyXmlDocumentContextVar;

  ~xml_read_cursor_var() = default;
  xml_read_cursor_var() = default; // We have no monostate in our variant. We could still allow this, but I'd prefer not to. REVIEW<dbien>: Need it for the compile...
  template < class t_TyXmlReadCursor >
  explicit xml_read_cursor_var( t_TyXmlReadCursor && _rrxrc )
    : m_varCursor( std::move( _rrxrc ) )
  {
  }
  xml_read_cursor_var( xml_read_cursor_var const & ) = delete;
  xml_read_cursor_var& operator =( xml_read_cursor_var const & ) = delete;
  xml_read_cursor_var( xml_read_cursor_var && ) = default;
  xml_read_cursor_var& operator =( xml_read_cursor_var && ) = default;
  void swap( xml_read_cursor_var & _r )
  {
    m_varCursor.swap( _r.m_varCursor );
  }

  bool FInsideDocumentTag() const
  {
    return std::visit( _VisitHelpOverloadFCall {
      []( auto & _tCursor ) -> bool
      {
        return _tCursor.FInsideDocumentTag();
      }
    }, m_varCursor );
  }
  bool FInProlog() const
  {
    return std::visit( _VisitHelpOverloadFCall {
      []( auto & _tCursor ) -> bool
      {
        return _tCursor.FInProlog();
      }
    }, m_varCursor );
  }
  bool FInEpilog() const
  {
    return std::visit( _VisitHelpOverloadFCall {
      []( auto & _tCursor ) -> bool
      {
        return _tCursor.FInEpilog();
      }
    }, m_varCursor );
  }
  bool FAtEOF() const
  {
    return std::visit( _VisitHelpOverloadFCall {
      []( auto & _tCursor ) -> bool
      {
        return _tCursor.FAtEOF();
      }
    }, m_varCursor );
  }
  bool FMoveDown()
  {
    return std::visit( _VisitHelpOverloadFCall {
      []( auto & _tCursor ) -> bool
      {
        return _tCursor.FMoveDown();
      }
    }, m_varCursor );
  }
  bool FCanMoveDown()
  {
    return std::visit( _VisitHelpOverloadFCall {
      []( auto & _tCursor ) -> bool
      {
        return _tCursor.FCanMoveDown();
      }
    }, m_varCursor );
  }
  bool FMoveUp()
  {
    return std::visit( _VisitHelpOverloadFCall {
      []( auto & _tCursor ) -> bool
      {
        return _tCursor.FMoveUp();
      }
    }, m_varCursor );
  }
  bool FCanMoveUp()
  {
    return std::visit( _VisitHelpOverloadFCall {
      []( auto & _tCursor ) -> bool
      {
        return _tCursor.FCanMoveUp();
      }
    }, m_varCursor );
  }
  bool FNextTag( _TyOptXmlTokenVar * _popttokRtnSpentTag = nullptr )
  {
    return std::visit( _VisitHelpOverloadFCall {
      [this,_popttokRtnSpentTag]( auto & _tCursor ) -> bool
      {
        return _FNextTag( _tCursor, _popttokRtnSpentTag );
      }
    }, m_varCursor );    
  }
  template < class t_TyFunctor >
  void ApplyAllContent( t_TyFunctor && _rrftor )
  {
    std::visit( _VisitHelpOverloadFCall {
      [ _rrftor = FWD_CAPTURE(_rrftor) ]( auto & _tCursor )
      {
        _tCursor.ApplyAllContent( access_fwd( _rrftor ) );
      }
    }, m_varCursor );    
  }
  void ClearContent()
  {
    std::visit( _VisitHelpOverloadFCall {
      []( auto & _tCursor )
      {
        _tCursor.ClearContent();
      }
    }, m_varCursor );    
  }
  // This transfers the lifetime of the XMLDecl xml_token - can only be called once to obtain that token. Could return a copy but the object is spent anyway so...
  _TyXmlTokenVar XMLDeclAcquireDocumentContext( _TyXmlDocumentContextVar & _rxdcDocumentContextVar )
  {
    return std::visit( _VisitHelpOverloadFCall {
      [this,&_rxdcDocumentContextVar]( auto & _tCursor ) -> _TyXmlTokenVar
      {
        return _XMLDeclAcquireDocumentContext( _tCursor, _rxdcDocumentContextVar );
      }
    }, m_varCursor );    
  }
protected:
  template < class t_TyXmlReadCursor >
  bool _FNextTag( t_TyXmlReadCursor & _rxrc, _TyOptXmlTokenVar * _popttokRtnSpentTag )
  {
    typedef typename t_TyXmlReadCursor::_TyOptXmlToken _TyOptXmlToken;
    _TyOptXmlToken opttokRtnSpentTag;
    bool fNextTag = _rxrc.FNextTag( _popttokRtnSpentTag ? &opttokRtnSpentTag : nullptr );
    if ( _popttokRtnSpentTag )
    {
      if ( !opttokRtnSpentTag.has_value() )
        _popttokRtnSpentTag->reset();
      else
        _popttokRtnSpentTag->emplace( std::move( *opttokRtnSpentTag ) );
    }
    return fNextTag;
  }
  template < class t_TyXmlReadCursor >
  _TyXmlTokenVar _XMLDeclAcquireDocumentContext( t_TyXmlReadCursor & _rxrc, _TyXmlDocumentContextVar & _rxdcDocumentContextVar )
  {
    typedef typename t_TyXmlReadCursor::_TyXmlTraits _TyXmlTraits;
    typedef _xml_document_context_transport< _TyXmlTraits > _TyXmlDocumentContext;
    typedef typename _TyXmlTraits::_TyXmlToken _TyXmlToken;
    _TyXmlDocumentContext xdc;
    _TyXmlToken & rtokXMLDecl = _rxrc.XMLDeclAcquireDocumentContext( xdc );
    _rxdcDocumentContextVar.emplace( std::move( xdc ) );
    return _TyXmlTokenVar( std::move( rtokXMLDecl ) );
  }
  
  _TyVariant m_varCursor;
};

__XMLP_END_NAMESPACE
