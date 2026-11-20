#pragma once

//          Copyright David Lawrence Bien 1997 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt).

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
#include <deque>
#include "alloca_list.h"
#include "alloca_array.h"
#include "xml_ns.h"
#include "xml_types.h"
#include "xml_traits.h"
#include "xml_parse.h"
#include "xml_dtd.h"
#include "xml_validate.h"
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
    // Assert( ( nSize <= 1 ) || _fIsTail ); - This has changed - we now keep the content until new content is read - this allows
    //  a user to MoveDown, check values (for instance) and then move up without losing data.
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
  _TyXmlToken & AddToken( _TyLexToken && _rrtok )
  {
    return m_saTokens.emplaceAtEnd( std::move( _rrtok ) );
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
    _TyRgStrings rgStrings( nContentTokens, ALLOCA_ARRAY_ALLOC( _TyStdStr, nContentTokens ) );
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
      _KGetContentString( _rstr, nCharsTotal, rgStrings.begin(), rgStrings.end() );
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
    _TyRgStrViews rgStrViews( nContentTokens, ALLOCA_ARRAY_ALLOC( _TyStrViewResult, nContentTokens ) );
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
        if ( psvCur->empty() )
          continue; // e.g. an empty CDATA section.
        memcpy( pcCur, psvCur->data(), psvCur->length() * sizeof( _TyCharResult ) ); // The views are in the result's character type.
        pcCur += psvCur->length();
      }
    }
  }

protected:
  template< class t_TyStringResult >
  void _KGetContentString( t_TyStringResult & _rstr, size_t _nCharsTotal, _TyStdStr * _pstrSrcBegin, _TyStdStr * _pstrSrcEnd ) const
    requires TAreSameSizeTypes_v< typename t_TyStringResult::value_type, _TyChar >
  {
    _rstr.resize( _nCharsTotal );
    if ( !_nCharsTotal )
      return;
    _TyChar * pcCur = (_TyChar*)&_rstr[0]; // t_TyStringResult may be a different character type of the same size.
    _TyStdStr * pstrSrcCur = _pstrSrcBegin;
    for ( ; _pstrSrcEnd != pstrSrcCur; ++pstrSrcCur )
    {
      if ( pstrSrcCur->empty() )
        continue;
      memcpy( pcCur, pstrSrcCur->data(), pstrSrcCur->length() * sizeof( _TyChar ) );
      pcCur += pstrSrcCur->length();
    }
    Assert( pcCur == (_TyChar*)&_rstr[0] + _nCharsTotal );
  }
  template< class t_TyStringResult >
  void _KGetContentString( t_TyStringResult & _rstr, size_t _nCharsTotal, _TyStdStr * _pstrSrcBegin, _TyStdStr * _pstrSrcEnd ) const
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
      pcBuf = (_TyChar*)alloca( _nCharsTotal * sizeof( _TyChar ) );
    // Fill the buffer regardless of where it was allocated.
    _TyChar * pcCur = pcBuf;
    _TyStdStr * pstrSrcCur = _pstrSrcBegin;
    for ( ; _pstrSrcEnd != pstrSrcCur; ++pstrSrcCur )
    {
      if ( pstrSrcCur->empty() )
        continue;
      memcpy( pcCur, pstrSrcCur->data(), pstrSrcCur->length() * sizeof( _TyChar ) );
      pcCur += pstrSrcCur->length();
    }
    Assert( pcCur == pcBuf + _nCharsTotal );
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
  typedef xml_dtd< _TyChar > _TyDtd;

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

// The DTD - only when the XML traits support the DTD ( _TyXmlTraits::s_kfSupportDTD ): otherwise a document type
//  declaration is not read - a document which has one is an error as it was before there was any of this.
  // The document type declaration is in the prolog: it has been read ( or there isn't one ) by the time the first tag is.
  // The cursor doesn't have a token for it: what it declares is the xml_dtd. The general entities which it declares - those
  //  which stand for text alone - are in the entity map of the parser's user object from then on, so references to them
  //  in character data and in the values of attributes are replaced as references to the predefined entities are.
  // An attribute which is declared with a default value and which a tag doesn't have is added to the tag with that
  //  value ( 3.3.2 ): after the attributes which the tag does have, in the order in which they are declared. If
  //  namespaces are being processed such an attribute may declare a namespace - "xmlns" or "xmlns:prefix" - and it does
  //  so for the tag as one which is written in the tag does.
  bool FHasDtd() const
  {
    return !!m_spDtd;
  }
  const _TyDtd & GetDtd() const
  {
    VerifyThrowSz( !!m_spDtd, "The document has no document type declaration - or it hasn't been read yet." );
    return *m_spDtd;
  }

// Validation - only when the XML traits are validating ( _TyXmlTraits::s_kfValidating ): the document is checked against
//  its DTD as it is read - see xml_validate.h for what is checked.
  // A document which is not valid is still read: what is wrong with it is recorded. Since it is checked as it is read
  //  these say what is so of as much of it as has been read - all of it once the cursor is beyond the document's element.
  // And all of it is checked: what the cursor passes over ( FNextTag() from above ) is read as what it stops at is.
  // FIsValid(): there is a DTD and neither its declarations nor the document violate a validity constraint.
  bool FIsValid() const
  {
    VerifyThrowSz( _TyXmlTraits::s_kfValidating, "The XML traits of this cursor are not validating." );
    return !!m_spDtd && m_spDtd->m_rgstrValidityErrors.empty() && ( !m_upValidator || m_upValidator->RgErrors().empty() );
  }
  // What the document violates - not what the declarations of its DTD do: for those see GetDtd().m_rgstrValidityErrors.
  vector< string > const & RgValidityErrors() const
  {
    VerifyThrowSz( _TyXmlTraits::s_kfValidating, "The XML traits of this cursor are not validating." );
    static const vector< string > s_rgstrNone;
    return !m_upValidator ? s_rgstrNone : m_upValidator->RgErrors();
  }
  // A violation is thrown as an xml_parse_exception - as an error of well-formedness is - and not recorded.
  void SetThrowOnValidityError( bool _fThrowOnValidityError = true )
  {
    m_fThrowOnValidityError = _fThrowOnValidityError;
    if ( !!m_upValidator )
      m_upValidator->SetThrowOnError( _fThrowOnValidityError );
  }
  bool FGetThrowOnValidityError() const
  {
    return m_fThrowOnValidityError;
  }
  // The document refers - in its content - to an entity which is declared and whose replacement text is not read: one
  //  which has markup or which is external. What such a reference stands for is not validated, so the document is not
  //  all validated: that nothing is found wrong doesn't say that it is valid.
  bool FValidationIncomplete() const
  {
    return m_fValidationIncomplete;
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
        m_pXp->_SetFilterWhitespaceCharData( _FLexFiltersWhitespaceCharData() );
    }
  }
  bool FUseXMLNamespaces() const
  {
    return m_fUseXMLNamespaces;
  }
  // The white space before the '>' of the end tag [42] which was read last - "</name  >": each call to FNextTag() which
  //  ends an element reads its end tag last, so after it this is that element's. So that a document can be written as
  //  it was read.
  _TyStrView SvEndTagSpace() const
  {
    return m_strEndTagSpace;
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
        // In the case of an empty element we may have content for the parent tag after us, another tag, the end tag of our parent, or nothing at all.
        // So really no restrictions at all... lol.
        Assert( ( vktidInvalidIdToken == tidLookahead ) || _FIsContentToken( tidLookahead ) || ( ( s_knTokenSTag == tidLookahead ) ||
                  ( s_knTokenEmptyElemTag == tidLookahead ) || ( s_knTokenETag == tidLookahead ) ) );
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
  _TyXmlReadContext & GetContextCur()
  {
    Assert( m_lContexts.size() );
    AssertValid();
    return *m_itCurContext;
  }
  const _TyXmlReadContext & GetContextCur() const
  {
    Assert( m_lContexts.size() );
    AssertValid();
    return *m_itCurContext;
  }
  _TyXmlToken & GetTagCur()
  {
    return GetContextCur().GetTag();
  }
  const _TyXmlToken & GetTagCur() const
  {
    return GetContextCur().GetTag();
  }
  // Access the tag name on this curson. If we are in the XMLDecl then this will throw.
  _TyStrView SvTagCur() const
  {
    VerifyThrow( !FInProlog() && !FInEpilog() );
    // If we aren't in the prolog then we should be at a tag.
    const _TyXmlToken & rxtTag = GetTagCur();
    Assert( rxtTag.FIsTag() );
    const _TyLexToken & rltokTag = rxtTag.GetLexToken();
    _TyStrView svTagStart;
    rltokTag.KGetStringView( svTagStart, rltokTag.GetValue()[vknTagNameIdx][vknNameIdx] );
    return svTagStart;
  }
  bool FPseudoXMLDecl() const
  {
    VerifyThrow( m_lContexts.size() );
    return m_lContexts.front().GetTag().GetValue().FIsNull();
  }
  const _TyXMLDeclProperties & GetXMLDeclProperties() const
  {
    Assert( m_pXp );
    return m_XMLDeclProperties;
  }
  template < class t_TyXMLDeclProperties >
  void GetXMLDeclProperties( t_TyXMLDeclProperties & _rxdp )
  {
    t_TyXMLDeclProperties xdpCopy( m_XMLDeclProperties );
    xdpCopy.swap( _rxdp );
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
    _rxdcDocumentContext.m_fIncludePrefixesInAttrNames = m_fIncludePrefixesInAttrNames;
    if constexpr ( _TyXmlTraits::s_kfSupportDTD )
    {
      // What tokens of the document refer to and which is not in the transport: the DTD and the text of the external
      //  entities which we read. They are ours as well - we are still what we were: valid or not.
      _rxdcDocumentContext.m_spDtd = m_spDtd;
      _rxdcDocumentContext.m_spEntityText = m_spMapExternalEntityText;
    }
    return m_lContexts.front().GetTag();
  }
  // This uses the cursor's prefix and uri hash's (the parser's) to create a namespace value wrap of the
  //  given type with the given (prefix,uri).
  template < class t_TyStrViewOrString >
  _TyXmlNamespaceValueWrap GetNamespaceValueWrap( ENamespaceReferenceType _enrtReferenceType, t_TyStrViewOrString const & _rsvPrefix, t_TyStrViewOrString const & _rsvUri )
  {
    const typename _TyUriAndPrefixMap::value_type & rvtPrefix = m_pXp->RStrAddPrefix( _rsvPrefix );
    const typename _TyUriAndPrefixMap::value_type & rvtUri = m_pXp->RStrAddUri( _rsvUri );
    return _TyXmlNamespaceValueWrap( &rvtPrefix, &rvtUri, _enrtReferenceType );
  }
// Navigation:
  // Move down - i.e. to the next subtag.
  // If the context is already present - i.e. we are not at the end of the context list - then we just move the context pointer and return true.
  // If we are at the end of the context list then this has some side effects:
  // 1) If successful then all content tokens in the current context are cleared. Changed: This doesn't allow filtering correctly - i.e.
  //      if you want to move down to check sub-tags and then filter either positively or negatively then the content would be missing.
  //    a) Then the next tag is read, a new context is created below the current one, and all content tokens for that next tag are read.
  // 2) If not sucessful then there is no change to any state - i.e. nothing is read or processed. To move further from here 
  bool FMoveDown()
  {
    AssertValid( true );
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
    // Changed: We don't want to rid this while moving down because this doesn't allow a user to MoveDown and then check sub-tag content and then
    //  move back up and again and then read the content (or not).
    // m_itCurContext->ClearContent();
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
  bool FNextTag( _TyOptXmlToken * _popttokRtnSpentTag = nullptr, bool _fReleaseNamespaces = true )
  {
    AssertValid( true );
    vtyTokenIdent tidCur = m_itCurContext->GetTagTokenId();
    if ( s_knTokenEmptyElemTag == tidCur )
    {
      Assert( _FAtTailContext() ); // must be.
      // Regardless we will be ending the current token. Do it.
      --m_itCurContext; // Must be able to do this because we know that at least XMLDecl is above us.
      _PopContext( _popttokRtnSpentTag, _fReleaseNamespaces );
      return _FProcessNextTag();
    }
    if ( !m_pltokLookahead )
      return false; // at eof.
    if ( s_knTokenSTag == tidCur )
    {
      // We will read until the end of this tag on the current level of the iterator.
      // Then we will check to see what is beyond that to decide whether we will go up or stay at the current level of the iterator.
      if ( !_FAtTailContext() || ( s_knTokenETag != m_pltokLookahead->GetTokenId() ) )
      {
        // Then we need to get to the point where the above conditions hold.
        _SkipOrReadToEndCurrentContext(); // Skip potentially a ton of XML.
      }
      Assert( _FAtTailContext() && ( s_knTokenETag == m_pltokLookahead->GetTokenId() ) );
      // Process the end tag and then consume it.
      _ProcessLookahead("Premature EOF.");
      --m_itCurContext;
      _PopContext( _popttokRtnSpentTag, _fReleaseNamespaces );
      Assert( _FAtTailContext() );
      return _FProcessNextTag();
    }
    Assert( s_knTokenXMLDecl == tidCur );
    if ( !_FAtTailContext() )
      _SkipOrReadToEndCurrentContext(); // Skip potentially a ton of XML.
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

  // Get to the end of the current context: the lexical analyzer is then at its end tag ( or at the end of the document ).
  // What is between here and there is skipped - unless the document is being validated: then it is read as it would be
  //  if the user were moving through it, so that all of it is checked.
  // It is read as well if the DTD declares entities whose text is content of the document: a reference to one in what
  //  is passed over is then replaced as it is anywhere - its text is read, and is well formed or isn't.
  void _SkipOrReadToEndCurrentContext()
  {
    if ( _TyXmlTraits::s_kfValidating || m_fEntitiesAreContent )
    {
      const typename _TyListReadContexts::iterator itContext = m_itCurContext;
      for ( ; ; )
      {
        m_itCurContext = std::prev( m_lContexts.end() );
        while ( FMoveDown() ) // To the first tag which has no sub-tag - reading as we go.
          ;
        if ( itContext == m_itCurContext )
          break; // Nothing is left within it.
        const typename _TyListReadContexts::iterator itBefore = m_itCurContext;
        if ( !FNextTag() && ( itBefore == m_itCurContext ) )
          break; // The end of the document.
      }
      m_itCurContext = itContext;
    }
    else
      _SkipToEndCurrentContext();
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
    // If the caller moved down into an empty element tag and then back up, that (already complete) context is still at the tail.
    // Pop it now so that the end tag in the lookahead is matched against its parent and not against the empty element.
    if ( !_FAtTailContext() && ( s_knTokenEmptyElemTag == m_lContexts.back().GetTagTokenId() ) )
      _PopContext( nullptr, false );
    while( !_FAtTailContext() || ( s_knTokenETag != m_pltokLookahead->GetTokenId() ) )
    {
      // Each token will be either an end tag or a beginning tag.
      if ( s_knTokenETag == m_pltokLookahead->GetTokenId() )
      {
        _ProcessEndTag( *m_pltokLookahead );
        _PopContext( nullptr, false );
      }
      else
      if ( s_knTokenSTag == m_pltokLookahead->GetTokenId() ) // We just skip empty element tags entirely.
      {
        _CheckName( *m_pltokLookahead, (*m_pltokLookahead)[vknTagNameIdx] );
        _ProcessTagName( (*m_pltokLookahead)[vknTagNameIdx] );
        _PushNewContext( *m_pltokLookahead );
      }
      // else we have either an s_knTokenEmptyElemTag or content that is after a s_knTokenEmptyElemTag or one of the above scenarios.
      Assert( ( s_knTokenEmptyElemTag == m_pltokLookahead->GetTokenId() ) || 
              ( s_knTokenEmptyElemTag == m_lContexts.back().GetTagTokenId() ) || m_pltokLookahead->FIsNull() );
      m_pltokLookahead.reset(); // Rid the lookahead.
      if ( s_knTokenEmptyElemTag == m_lContexts.back().GetTagTokenId() )
      {
        _PopContext( nullptr, false );
      }
      VerifyThrowSz( _FGetLexToken( m_pltokLookahead, rgSkipTokens, rgSkipTokens + nSkipTokens, m_pspStartAll ), "Premature EOF." );
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
    m_fUseXMLNamespaces = m_pXp->FUseXMLNamespaces(); // As whoever opened the document said.
    m_pXp->_InitMapsAndUserObj();
    _InitNamespaceMap();
    m_pXp->_SetFilterWhitespaceCharData( _FLexFiltersWhitespaceCharData() );
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
        pltokFirst = make_unique< _TyLexToken >( std::move( ucxtEmpty ), &m_pXp->GetLexicalAnalyzer().template GetActionObj< s_knTokenXMLDecl >() );
      }
      _ProcessToken( pltokFirst ); // This pushes XMLDecl onto the head of the stack as the first token in that context.
    }//EB
    // Now "prime the pump" by reading the read-ahead token and then call _ReadTokenContent().
    VerifyThrow( _FGetToken( m_pltokLookahead ) );
    _ReadTokenContent();
    // [1] document ::= prolog element Misc*: what follows the prolog is the start tag of the document element.
    VerifyThrowSz( !!m_pltokLookahead && ( ( s_knTokenSTag == m_pltokLookahead->GetTokenId() ) || ( s_knTokenEmptyElemTag == m_pltokLookahead->GetTokenId() ) ),
      "The document has no document element: what follows its prolog is not a start tag." );
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
    m_upValidator.reset();
    m_fValidationIncomplete = false;
    if constexpr ( _TyXmlTraits::s_kfSupportDTD )
    {
      m_rgupEntityFrames.clear();
      m_dqPendingTokens.clear();
      m_spMapExternalEntityText.reset();
    }
    m_nLexDepth = 0;
    m_fEntitiesAreContent = false;
    m_fDtdDeclaresEntities = false;
    m_fTokenLiteralLineEnds = false;
    m_nchEntityExpansion = 0;
    m_rgfFilteredWhiteSpace.clear();
    m_spDtd.reset();
  }
// The next token of the lexical analyzer - or of the text of an entity:
  // A reference in content to a general entity whose replacement text is not text alone - it has markup, or the entity
  //  is external - is replaced by that text ( 4.4.2 Included ): the text is content of the document where the reference
  //  is. ( An entity whose text is text alone is in the entity map of the user object: a reference to it is a piece of
  //  the character data which it is in. ) So once there is a DTD the tokens of the document come from here:
  //  - Character data which has such a reference is taken apart at the reference: what is before it is a token, then
  //    the tokens of the text of the entity - which a lexical analyzer of its own reads, and which may refer to such
  //    entities in turn - and then what is after it. s_knTokenEntityContentBegin and s_knTokenEntityContentEnd are
  //    before and after the tokens of the entity.
  //  - The text of an entity is to match content [43] ( 4.3.2 ): its tokens are those of content, an element which
  //    begins in it ends in it and it doesn't end one which began outside of it. WFC: No Recursion [68].
  bool _FGetLexToken( unique_ptr< _TyLexToken > & _rpltok, vtyTokenIdent * _ptidSkipBegin, vtyTokenIdent * _ptidSkipEnd, const _TyStateProto * _pspStart )
  {
    if constexpr ( _TyXmlTraits::s_kfSupportDTD )
    {
      if ( ( m_pspStartAll == _pspStart ) && !!m_spDtd )
        return _FGetTokenOfContent( _rpltok, _ptidSkipBegin, _ptidSkipEnd );
    }
    return m_pXp->GetLexicalAnalyzer().FGetToken( _rpltok, _ptidSkipBegin, _ptidSkipEnd, _pspStart, false );
  }
  bool _FGetTokenOfContent( unique_ptr< _TyLexToken > & _rpltok, vtyTokenIdent * _ptidSkipBegin, vtyTokenIdent * _ptidSkipEnd )
  {
    for ( ; ; )
    {
      bool fLiteralLineEnds = false;
      if ( !m_dqPendingTokens.empty() )
      {
        // What we made of character data which had a reference - and the tokens which mark what an entity has.
        unique_ptr< _TyLexToken > pltokPending( std::move( m_dqPendingTokens.front() ) );
        m_dqPendingTokens.pop_front();
        const vtyTokenIdent tidPending = pltokPending->GetTokenId();
        if ( ( _ptidSkipEnd != std::find( _ptidSkipBegin, _ptidSkipEnd, tidPending ) ) ||
             ( m_fSkippingTags && ( ( s_knTokenEntityContentBegin == tidPending ) || ( s_knTokenEntityContentEnd == tidPending ) ) ) )
          continue; // The lexical analyzer would have passed it over.
        _rpltok = std::move( pltokPending );
      }
      else
      if ( m_rgupEntityFrames.empty() )
      {
        if ( !m_pXp->GetLexicalAnalyzer().FGetToken( _rpltok, _ptidSkipBegin, _ptidSkipEnd, m_pspStartAll, false ) )
          return false;
      }
      else
      {
        _TyEntityFrame & rframe = *m_rgupEntityFrames.back();
        unique_ptr< _TyLexTokenEntity > pltokEntity;
        if ( !rframe.m_analyzer.FGetToken( pltokEntity, _ptidSkipBegin, _ptidSkipEnd, _PspStartAllEntity(), false ) )
        {
          _EndEntityFrame();
          continue;
        }
        // The token as one of ours: with our user object - the entities are those of the document.
        _rpltok = make_unique< _TyLexToken >( m_pXp->GetUserObj(), pltokEntity->GetTokenId() );
        _rpltok->GetValue() = std::move( pltokEntity->GetValue() );
        _SetTransportCtxt( _rpltok->GetTransportCtxt(), pltokEntity->GetTransportCtxt() );
        fLiteralLineEnds = rframe.m_fLiteralLineEnds;
        if ( fLiteralLineEnds && ( s_knTokenCharData == _rpltok->GetTokenId() ) )
          _FlagLiteralLineEnds( _rpltok->GetValue() ); // Before it is taken apart below: the parts are then flagged.
      }
      m_fTokenLiteralLineEnds = fLiteralLineEnds;
      switch( _rpltok->GetTokenId() )
      {
        case s_knTokenSTag:
          ++m_nLexDepth;
        break;
        case s_knTokenETag:
          if ( !m_rgupEntityFrames.empty() && ( m_nLexDepth <= m_rgupEntityFrames.back()->m_nLexDepth ) )
            THROWXMLPARSEEXCEPTION( "The replacement text of the entity [%s] has the end tag of an element which doesn't begin in it.", StrConvertString< char >( m_rgupEntityFrames.back()->m_strName ).c_str() );
          if ( !!m_nLexDepth )
            --m_nLexDepth;
        break;
        case s_knTokenCharData:
          // Within the document's element only: outside of it there is no character data but white space.
          if ( !!m_nLexDepth && _FIncludeEntityOfCharData( _rpltok ) )
            continue;
        break;
        default:
        break;
      }
      return true;
    }
  }
  // Make the transport context of a token of ours that of another token: of one of ours, or of a token of the text
  //  of an entity - whose text is in memory.
  //  - A variant of contexts has no assignment: it is made anew - as the type of context which the other is.
  //  - A context which backs its token with a copy of the text ( a file's ) copies the text of the other.
  template < class t_TyTransportCtxtSrc >
  static void _SetTransportCtxt( _TyTransportCtxt & _rcxtDest, t_TyTransportCtxtSrc const & _rcxtSrc )
  {
    if constexpr ( !TFIsTransportVarCtxt_v< _TyTransportCtxt > )
      _rcxtDest = _rcxtSrc;
    else
    if constexpr ( is_same_v< _TyTransportCtxt, t_TyTransportCtxtSrc > )
    {
      _TyTransportCtxt cxtCopy( _rcxtSrc );
      _rcxtDest.swap( cxtCopy );
    }
    else
      _rcxtDest.GetVariant().template emplace< t_TyTransportCtxtSrc >( _rcxtSrc );
  }
  // The document is declared standalone - only if its XML declaration says so: m_fStandalone is true as well when it says nothing.
  bool _FStandalone() const
  {
    return m_XMLDeclProperties.m_fHasStandalone && m_XMLDeclProperties.m_fStandalone;
  }
  // Flag the pieces of plain text of the value: see s_kdtfLiteralLineEnds.
  static void _FlagLiteralLineEnds( _TyLexValue & _rval )
  {
    if ( !_rval.FHasTypedData() || _rval.template GetVal< _TyData >().FIsNull() )
      return;
    _rval.template GetVal< _TyData >().Apply(
      []( _l_data_typed_range * _pdtrBegin, _l_data_typed_range * const _pdtrEnd )
      {
        for ( ; _pdtrEnd != _pdtrBegin; ++_pdtrBegin )
          if ( s_kdtPlainText == DtGetType( _pdtrBegin->type() ) )
            _pdtrBegin->m_nType |= s_kdtfLiteralLineEnds;
      }
    );
  }
  // If the character data has a reference to an entity whose text is to be included then set up to read that and return
  //  true: the token is then no longer here - what we made of it is among the pending tokens.
  bool _FIncludeEntityOfCharData( unique_ptr< _TyLexToken > & _rpltok )
  {
    _TyLexValue & rval = _rpltok->GetValue();
    if ( !rval.FHasTypedData() )
      return false;
    const _TyData & krdt = rval.template GetVal< _TyData >();
    if ( krdt.FIsNull() || krdt.FContainsSingleDataRange( s_kdtPlainText ) )
      return false; // No reference at all.
    vector< _l_data_typed_range > rgdtr;
    krdt.Apply(
      [&rgdtr]( const _l_data_typed_range * _pdtrBegin, const _l_data_typed_range * const _pdtrEnd )
      {
        rgdtr.insert( rgdtr.end(), _pdtrBegin, _pdtrEnd );
      }
    );
    auto const & rmapEntities = m_pXp->GetUserObj().GetEntityMap();
    size_t nchEntityText = 0; // What the references which remain pieces of this character data stand for.
    for ( size_t nPiece = 0; nPiece < rgdtr.size(); ++nPiece )
    {
      if ( rgdtr[nPiece].FIsNull() || ( s_kdtEntityRef != DtGetType( rgdtr[nPiece].type() ) ) )
        continue;
      _TyStrView svName;
      _rpltok->KGetStringView( svName, rgdtr[nPiece].GetRangeBase() );
      auto citEntityOfMap = rmapEntities.find( svName );
      if ( rmapEntities.end() != citEntityOfMap )
      {
        nchEntityText += citEntityOfMap->second.length();
        continue; // Text alone: the reference remains a piece of the character data.
      }
      // The text of the entity is not included: the reference remains - in the token and in the pieces we have of it.
      auto lambdaNotIncluded = [&]()
      {
        rgdtr[nPiece].m_nType |= s_kdtfEntityRefNotIncluded;
        size_t nPieceOfToken = 0;
        rval.template GetVal< _TyData >().Apply(
          [nPiece,&nPieceOfToken]( _l_data_typed_range * _pdtrBegin, _l_data_typed_range * const _pdtrEnd )
          {
            for ( ; _pdtrEnd != _pdtrBegin; ++_pdtrBegin, ++nPieceOfToken )
              if ( nPiece == nPieceOfToken )
                _pdtrBegin->m_nType |= s_kdtfEntityRefNotIncluded;
          }
        );
      };
      const xml_dtd_entity< _TyChar > * pEntity = m_spDtd->PGetGeneralEntity( svName );
      if ( !pEntity )
      {
        _EntityNotDeclared( svName, "character data" );
        lambdaNotIncluded();
        continue;
      }
      const string strName = StrConvertString< char >( svName );
      // WFC: Entity Declared [68] - of a document which is declared standalone.
      if ( pEntity->m_fDeclaredExternally && _FStandalone() )
        THROWXMLPARSEEXCEPTION( "The document is declared standalone and refers to the entity [%s] - which is not declared in its internal subset.", strName.c_str() );
      // WFC: Parsed Entity [68].
      if ( pEntity->FIsUnparsed() )
        THROWXMLPARSEEXCEPTION( "The unparsed entity [%s] is referenced in content.", strName.c_str() );
      _TyStrView svText;
      if ( !_FGetEntityText( *pEntity, svText ) )
      {
        lambdaNotIncluded(); // An external entity which is not read.
        continue;
      }
      // WFC: No Recursion [68].
      for ( auto const & rupFrame : m_rgupEntityFrames )
        if ( _TyStrView( rupFrame->m_strName ) == svName )
          THROWXMLPARSEEXCEPTION( "The entity [%s] contains a reference to itself.", strName.c_str() );
      {//B
        const size_t knMaxDepth = !m_pXp->GetDtdLimits().m_nMaxEntityDepth ? s_knMaxEntityDepth : m_pXp->GetDtdLimits().m_nMaxEntityDepth;
        VerifyThrowSz( m_rgupEntityFrames.size() < knMaxDepth, "Entities are within each other more than [%zu] deep.", knMaxDepth );
      }//EB
      _AccountEntityExpansion( svText.length() ); // The text which is read in place of the reference.
      if constexpr ( _TyXmlTraits::s_kfValidating )
      {
        // VC: Standalone Document Declaration [32].
        if ( !!m_upValidator && m_upValidator->FGetStandalone() && pEntity->m_fDeclaredExternally )
          m_upValidator->StandaloneViolation( "refers to the entity", svName );
      }
      // What is before the reference and what is after it are character data each - if there is any.
      auto lambdaSetPieces = []( _TyLexToken & _rltok, const _l_data_typed_range * _pdtrBegin, const _l_data_typed_range * const _pdtrEnd )
      {
        _TyData dt;
        for ( ; _pdtrEnd != _pdtrBegin; ++_pdtrBegin )
          dt.Append( *_pdtrBegin );
        _rltok.GetValue().emplaceVal( std::move( dt ) );
      };
      const _TyStdStr strEntityName( svName ); // The token goes: this is of it.
      unique_ptr< _TyLexToken > pltokBefore, pltokAfter;
      if ( nPiece > 0 )
      {
        // ( A token is not copied: one of ours for the same text. )
        pltokBefore = make_unique< _TyLexToken >( m_pXp->GetUserObj(), s_knTokenCharData );
        _SetTransportCtxt( pltokBefore->GetTransportCtxt(), _rpltok->GetTransportCtxt() );
        lambdaSetPieces( *pltokBefore, rgdtr.data(), rgdtr.data() + nPiece );
      }
      if ( nPiece + 1 < rgdtr.size() )
      {
        pltokAfter = std::move( _rpltok );
        lambdaSetPieces( *pltokAfter, rgdtr.data() + nPiece + 1, rgdtr.data() + rgdtr.size() );
      }
      _rpltok.reset();
      auto lambdaMarker = [this,&strEntityName]( vtyTokenIdent _tid )
      {
        unique_ptr< _TyLexToken > pltokMarker = make_unique< _TyLexToken >( m_pXp->GetUserObj(), _tid );
        pltokMarker->template emplaceValue< _TyStdStr >( strEntityName );
        return pltokMarker;
      };
      if ( svText.empty() )
      {
        // Nothing to read: what was before, the two marks and what was after.
        if ( !!pltokAfter )
          m_dqPendingTokens.push_front( std::move( pltokAfter ) );
        m_dqPendingTokens.push_front( lambdaMarker( s_knTokenEntityContentEnd ) );
      }
      else
      {
        unique_ptr< _TyEntityFrame > upFrame = make_unique< _TyEntityFrame >();
        upFrame->m_strName = strEntityName;
        upFrame->m_svText = svText;
        upFrame->m_nLexDepth = m_nLexDepth;
        upFrame->m_pltokAfter = std::move( pltokAfter );
        upFrame->m_analyzer.emplaceTransport( svText.data(), svText.length() );
        upFrame->m_analyzer.GetUserObj().SetFilterWhitespaceCharData( _FLexFiltersWhitespaceCharData() );
        // A #xD in the replacement text of an internal entity is one which a character reference put there: it is
        //  not the end of a line of text which is read from somewhere ( 2.11 ) - see s_kdtfLiteralLineEnds.
        upFrame->m_fLiteralLineEnds = pEntity->m_fInternal && ( _TyStrView::npos != svText.find( _TyChar( 0xD ) ) );
        m_rgupEntityFrames.push_back( std::move( upFrame ) );
      }
      m_dqPendingTokens.push_front( lambdaMarker( s_knTokenEntityContentBegin ) );
      if ( !!pltokBefore )
        m_dqPendingTokens.push_front( std::move( pltokBefore ) );
      return true;
    }
    // The token is given as it is: what its references stand for is counted - once, here. ( What is taken apart above
    //  comes here again as the tokens which were made of it. )
    _AccountEntityExpansion( nchEntityText );
    return false;
  }
  // What the references to entities of the document have stood for so far - held to xml_dtd_limits::m_nMaxEntityExpansion.
  void _AccountEntityExpansion( size_t _nch )
  {
    m_nchEntityExpansion += _nch;
    const size_t knMax = m_pXp->GetDtdLimits().m_nMaxEntityExpansion;
    VerifyThrowSz( !knMax || ( m_nchEntityExpansion <= knMax ), "The references to entities of the document stand for more text than a document's entities may: more than [%zu] characters.", knMax );
  }
  // The text of the entity is all read: what follows is its mark and then what followed the reference to it.
  void _EndEntityFrame()
  {
    _TyEntityFrame & rframe = *m_rgupEntityFrames.back();
    const string strName = StrConvertString< char >( rframe.m_strName );
    // No token was found: at the end of the text - or what is there is not content.
    rframe.m_analyzer.GetStream().ResetToTokenStart();
    if ( size_t( rframe.m_analyzer.GetStream().PosTokenStart() ) < rframe.m_svText.length() )
      THROWXMLPARSEEXCEPTION( "The replacement text of the entity [%s] is not well formed as content.", strName.c_str() );
    if ( m_nLexDepth != rframe.m_nLexDepth )
      THROWXMLPARSEEXCEPTION( "An element begins in the replacement text of the entity [%s] and doesn't end in it.", strName.c_str() );
    unique_ptr< _TyLexToken > pltokMarker = make_unique< _TyLexToken >( m_pXp->GetUserObj(), s_knTokenEntityContentEnd );
    pltokMarker->template emplaceValue< _TyStdStr >( rframe.m_strName );
    m_dqPendingTokens.push_back( std::move( pltokMarker ) );
    if ( !!rframe.m_pltokAfter )
      m_dqPendingTokens.push_back( std::move( rframe.m_pltokAfter ) );
    m_rgupEntityFrames.pop_back();
  }
  // The replacement text of a parsed general entity. For an external entity it is read - if the parser has the means -
  //  and kept: the tokens of it refer to it. Returns false if it isn't to be had.
  bool _FGetEntityText( xml_dtd_entity< _TyChar > const & _rentity, _TyStrView & _rsvText )
  {
    if ( _rentity.m_fInternal )
    {
      _rsvText = _rentity.m_strValue;
      return true;
    }
    if ( !m_spMapExternalEntityText )
      m_spMapExternalEntityText = make_shared< _TyMapExternalEntityText >();
    _TyMapExternalEntityText & rmapExternalEntityText = *m_spMapExternalEntityText;
    typename _TyMapExternalEntityText::const_iterator cit = rmapExternalEntityText.find( _rentity.m_strName );
    if ( rmapExternalEntityText.end() == cit )
    {
      if ( !m_pXp->GetExternalEntityResolver() )
        return false;
      _TyStdStr strText, strLocation;
      if ( !m_pXp->GetExternalEntityResolver()( _rentity.m_externalId, _rentity.m_strBase, strText, strLocation ) )
        return false;
      VerifyThrowSz( !m_pXp->GetDtdLimits().m_nMaxExternalEntity || ( strText.length() <= m_pXp->GetDtdLimits().m_nMaxExternalEntity ),
        "An external entity has more than the [%zu] characters which one may have.", m_pXp->GetDtdLimits().m_nMaxExternalEntity );
      xml_dtd_parser< _TyChar >::StripTextDecl( strText ); // The text declaration [77] is not part of the replacement text ( 4.3.1 ).
      NormalizeLineEnds( strText ); // 2.11: the text is ours.
      cit = rmapExternalEntityText.emplace( _rentity.m_strName, std::move( strText ) ).first;
    }
    _rsvText = cit->second;
    return true;
  }
  bool _FGetToken( unique_ptr< _TyLexToken > & _rpltok, const _TyStateProto * _pspStart = nullptr )
  {
    Assert( !!m_pXp );
    if ( m_fUpdateSkip )
      _UpdateSkip();
    if ( !_pspStart )
      _pspStart = m_pspStartAll;
    for ( ; ; )
    {
      bool fGotToken = _FGetLexToken( _rpltok, &*m_rgSkipTokensCur.begin(), &*m_rgSkipTokensCur.begin() + m_nSkip, _pspStart );
      if constexpr ( _TyXmlTraits::s_kfSupportDTD )
      {
        // The document type declaration is not a token of the DFA of the document: where it is that DFA finds no token.
        // That is in the prolog - after the XML declaration and before the first tag - and there may be only one.
        // We read it and what we return for it is a token whose value is its text: s_knTokenDoctypeDecl - which is then
        //  one of the tokens of the prolog as a comment there is.
        if ( !fGotToken && ( m_pspStartAll == _pspStart ) && !m_fGotFirstTag && !m_lContexts.empty() && !m_spDtd )
          fGotToken = _FReadDoctypeDecl( _rpltok );
      }
      if ( !fGotToken && ( m_pspStartAll == _pspStart ) )
      {
        // No token: the document ends here - or what is here is nothing which a document may have. The lexical analyzer
        //  says which when it is asked to: it throws unless it is at the end of what it reads.
        m_pXp->GetStream().ResetToTokenStart();
        unique_ptr< _TyLexToken > pltokNone;
        (void)m_pXp->GetLexicalAnalyzer().FGetToken( pltokNone, nullptr, nullptr, m_pspStartAll, true );
      }
      if constexpr ( _TyXmlTraits::s_kfValidating )
      {
        // What the user doesn't want - and the validator is to know of: see _FFilterTokenOfValidated().
        if ( fGotToken && ( m_pspStartAll == _pspStart ) && _FFilterTokenOfValidated( *_rpltok ) )
        {
          _rpltok.reset();
          continue;
        }
      }
      return fGotToken;
    }
  }
  // Read the document type declaration [28] if that is what is here. Return false if nothing of the DTD is here - the
  //  end of the input for instance: then nothing was read.
  // The lexical analyzer is left after the '>' of the declaration.
  // _rpltok: the token which we make of the declaration - s_knTokenDoctypeDecl.
  bool _FReadDoctypeDecl( unique_ptr< _TyLexToken > & _rpltok )
  {
    typedef typename _TyXmlParser::_TyLexicalAnalyzer _TyLexicalAnalyzer;
    _TyLexicalAnalyzer & rAnalyzer = m_pXp->GetLexicalAnalyzer();
    m_pXp->GetStream().ResetToTokenStart(); // The DFA of the document found no token: we try another DFA from where it began.
    // The DFA of the DTD which has "<!DOCTYPE": the mode "Dtd" of the parser of the DTD.
    const _TyStateProto * const pspStartDtd = *( _xml_dtd_start_states< _TyChar >::template Get< _TyLexTraits, _TyStateProto >().begin() + ns_xmlpdtd::s_knLexerMode_Dtd );
    unique_ptr< _TyLexToken > pltokDoctype;
    if ( !rAnalyzer.FGetToken( pltokDoctype, nullptr, nullptr, pspStartDtd, false ) )
    {
      m_pXp->GetStream().ResetToTokenStart();
      return false;
    }
    if ( s_knTokenDtdDoctypeBegin != pltokDoctype->GetTokenId() )
      THROWXMLPARSEEXCEPTION( "Found markup in the prolog which is neither a document type declaration nor the document's element." );
    // A document may be one which is not to have a DTD at all - see xml_dtd_limits: then this is an error.
    VerifyThrowSz( m_pXp->GetDtdLimits().m_fAllowDoctypeDecl, "The document has a document type declaration and the parser was told to accept none." );
    m_spDtd = make_shared< _TyDtd >();
    _TyStdStr strDoctypeText;
    xml_dtd_parser< _TyChar > dtdParser( *m_spDtd );
    // Standalone only if the XML declaration says so: m_fStandalone is true as well when it says nothing.
    dtdParser.SetStandalone( m_XMLDeclProperties.m_fHasStandalone && m_XMLDeclProperties.m_fStandalone );
    dtdParser.SetUseXMLNamespaces( m_fUseXMLNamespaces );
    dtdParser.SetLimits( m_pXp->GetDtdLimits() );
    dtdParser.SetKeepDoctypeText( &strDoctypeText );
    // The external subset and the external parameter entities are read if the parser has been given the means.
    if ( !!m_pXp->GetExternalEntityResolver() )
      dtdParser.SetExternalEntityResolver( m_pXp->GetExternalEntityResolver(), m_pXp->StrBaseLocation() );
    dtdParser.ParseDoctypeDecl( rAnalyzer, std::move( pltokDoctype ) );
    // The entities of the document are those which the DTD declares - with the predefined ones, which are there already.
    // ( In a document which is declared standalone those which are declared externally are not declared for a reference:
    //  4.1 WFC: Entity Declared - a reference to one is an error as one to an entity which is not declared at all. )
    m_spDtd->AddGeneralEntitiesTo( m_pXp->GetUserObj().GetEntityMap(), _FStandalone() );
    // Those whose text is other within the value of an attribute - see _ProcessEntityReferencesOfAttrValues().
    m_spDtd->AddGeneralEntitiesInAttributeValuesTo( m_pXp->GetUserObj().GetEntityMap(), m_pXp->GetUserObj().GetEntityMapAttrValues() );
    m_fDtdDeclaresEntities = !m_spDtd->m_mapGeneralEntities.empty();
    // The references which we don't look at as we read - see xml_user_obj<>::m_setEntitiesWithoutText.
    {//B
      auto & ruo = m_pXp->GetUserObj();
      ruo.m_fEntitiesNotDeclaredHaveNoText = _FEntityNotDeclaredIsOfValidity();
      ruo.m_setEntitiesWithoutText.clear();
      for ( auto const & rvtEntity : m_spDtd->m_mapGeneralEntities )
        if ( !rvtEntity.second.m_fInternal && !rvtEntity.second.FIsUnparsed() && !( rvtEntity.second.m_fDeclaredExternally && _FStandalone() ) )
          ruo.m_setEntitiesWithoutText.insert( rvtEntity.first );
    }//EB
    // Whether a reference in content may be to an entity whose text is content - or to one which may not be referenced
    //  there: then nothing of the document is passed over unread - see _SkipOrReadToEndCurrentContext().
    m_fEntitiesAreContent = false;
    for ( auto const & rvtEntity : m_spDtd->m_mapGeneralEntities )
    {
      auto const & rmapEntities = m_pXp->GetUserObj().GetEntityMap();
      if ( rmapEntities.end() != rmapEntities.find( rvtEntity.first ) )
        continue; // Text alone.
      if ( rvtEntity.second.m_fInternal || rvtEntity.second.FIsUnparsed() || !!m_pXp->GetExternalEntityResolver() )
      {
        m_fEntitiesAreContent = true;
        break;
      }
    }
    // The token of the declaration. It was read as many tokens of the DTD so its text is not a piece of the document
    //  which a token of ours refers to: the value is a string.
    _rpltok = make_unique< _TyLexToken >( m_pXp->GetUserObj(), s_knTokenDoctypeDecl );
    _rpltok->template emplaceValue< _TyStdStr >( std::move( strDoctypeText ) );
    return true;
  }
  bool _FIsContentToken( vtyTokenIdent _tid ) const
  {
    return ( s_knTokenComment == _tid ) || ( s_knTokenCDataSection == _tid ) || ( s_knTokenCharData == _tid ) || ( s_knTokenProcessingInstruction == _tid ) ||
           ( s_knTokenDoctypeDecl == _tid ) || // The document type declaration is among the tokens of the prolog - see _FReadDoctypeDecl().
           ( s_knTokenEntityContentBegin == _tid ) || ( s_knTokenEntityContentEnd == _tid ); // See _FGetLexToken().
  }
  void _ReadTokenContent()
  {
    if ( !m_pltokLookahead || ( s_knTokenEmptyElemTag == m_lContexts.back().GetTagTokenId() ) )
      return; // At eof or tag with no content.
    // Clear any existing content here instead of in FMoveDown() as we were previously:
    m_lContexts.back().ClearContent();
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
    // [1], [27]: after the document element there are only comments, processing instructions and white space.
    if ( FInEpilog() && !!m_pltokLookahead )
      THROWXMLPARSEEXCEPTION( "There is markup after the document element which may not be there: only comments, processing instructions and white space may follow it." );
  }
  bool _FAtTailContext() const
  {
    Assert( !m_lContexts.empty() );
    return m_lContexts.empty() ? false : ( &*m_itCurContext == &m_lContexts.back() );
  }
  void _PushNewContext( _TyLexToken & _rltok )
  {
    m_lContexts.emplace_back( std::move( _rltok ) );
    m_lContexts.back().GetTag().AssertValid( m_fUseXMLNamespaces );
    if ( 1 == m_lContexts.size() )
      m_itCurContext = m_lContexts.begin();
  }
  void _AppendContextContent( _TyLexToken & _rltok )
  {
    _TyXmlToken & _rxtok = m_lContexts.back().AddToken( std::move( _rltok ) );
    _rxtok.AssertValid( m_fUseXMLNamespaces );
  }
  void _PopContext( _TyOptXmlToken * _popttokRtnSpentTag, bool _fReleaseNamespaces )
  {
    if constexpr ( _TyXmlTraits::s_kfValidating )
    {
      // The end of an element: its content must be all that its content model requires.
      if ( !!m_upValidator && ( m_lContexts.size() > 1 ) )
        m_upValidator->EndTag();
    }
    if ( _popttokRtnSpentTag )
    {
      *_popttokRtnSpentTag = std::move( m_lContexts.back().GetTag() );
      if ( m_fUseXMLNamespaces && _fReleaseNamespaces )
      {
        // Then we must check if we have any namespace declarations in this tag and if so undeclare them.
        _TyLexValue & rrgvName = (**_popttokRtnSpentTag)[vknTagNameIdx];
        vtySignedLvalueInt & rnNamespaceDecls = rrgvName[vknTagName_NNamespaceDeclsIdx].template GetVal< vtySignedLvalueInt >();
        vtySignedLvalueInt nNamespaceDecls = rnNamespaceDecls;
        rnNamespaceDecls = 0; // Clear the count of namespace decls.
        if ( nNamespaceDecls )
        {
          Assert( nNamespaceDecls >= 0 );
          // Move through looking for namespace declarations and undeclare them. No need to do this in reverse.
          typename _TyLexValue::_TySegArrayValues & rsaAttrs = (**_popttokRtnSpentTag)[vknAttributesIdx].GetValueArray();
          (void)rsaAttrs.NApplyContiguous( 0, rsaAttrs.NElements(), 
            [&nNamespaceDecls]( _TyLexValue * _pattrBegin, _TyLexValue * _pattrEnd ) -> size_t
            {
              _TyLexValue * pattrCur = _pattrBegin;
              for ( ; ( _pattrEnd != pattrCur ) && !!nNamespaceDecls; ++pattrCur )
              {
                _TyLexValue & rvNamespace = (*pattrCur)[vknNamespaceIdx];
                if ( rvNamespace.template FIsA<_TyXmlNamespaceValueWrap>() )
                {
                  _TyXmlNamespaceValueWrap & rxnvw = rvNamespace.template GetVal< _TyXmlNamespaceValueWrap >();
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
          Assert( !nNamespaceDecls ); // should have found them all.
        }
      }
    }
    m_lContexts.pop_back();
  }
  void _ProcessToken( unique_ptr< _TyLexToken > & _rpltok )
  {
    _ValidateWhatWasFiltered(); // What came before this token and was passed over.
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
        // [1] document ::= prolog element Misc*: there is one document element.
        VerifyThrowSz( !( m_fGotFirstTag && ( 1 == m_lContexts.size() ) ), "The document has a second document element: there is to be one." );
        m_fGotFirstTag = true;
        // The attributes which the tag doesn't have and which the DTD gives a default value: found before the names of
        //  the tag are processed below - and those which declare a namespace declare it now so that it is there for them.
        // And the values of the attributes which the tag has and whose declared type is not CDATA are flagged as such.
        _CheckNamesOfTag( *_rpltok ); // First: the names are then what was written - and qualified names if they are to be.
        _TyRgDefaultAttrs rgDefaultAttrs;
        if constexpr ( _TyXmlTraits::s_kfSupportDTD )
        {
          if ( !!m_spDtd && ( m_fDtdDeclaresEntities || _TyXmlTraits::s_kfValidating ) )
            _ProcessEntityReferencesOfAttrValues( *_rpltok );
          if ( !!m_spDtd && !m_spDtd->m_mapAttLists.empty() )
            _GetDefaultAttributes( *_rpltok, rgDefaultAttrs );
        }
        // The tag as it is written and the attributes which it has - their values as the declarations make them - are validated.
        if constexpr ( _TyXmlTraits::s_kfValidating )
          _ValidateStartTag( *_rpltok );
        if ( m_fUseXMLNamespaces )
          _ProcessNamespaces( *_rpltok );
        else
          _ProcessNoNamespaces( *_rpltok );
        if constexpr ( _TyXmlTraits::s_kfSupportDTD )
        {
          if ( !rgDefaultAttrs.empty() )
            _AddDefaultAttributes( *_rpltok, rgDefaultAttrs );
        }
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
      case s_knTokenDoctypeDecl:
        _AppendContextContent( *_rpltok ); // It is as _FReadDoctypeDecl() made it.
      break;
      case s_knTokenEntityContentBegin:
      case s_knTokenEntityContentEnd:
        _AppendContextContent( *_rpltok ); // As _FGetTokenOfContent() made them.
      break;
    }
  }
  void _ProcessEndTag( _TyLexToken & _rltok )
  {
    VerifyThrowSz( FInsideDocumentTag(), "Found end tag before first tag declaration." );
    _TyLexValue & rvalTagEnd = _rltok.GetValue();
    _CheckName( _rltok, rvalTagEnd ); // It is the name of its start tag: checked as that was - and all of it as that is.
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
    // The white space before its '>' - ETag [42] - as it was written: see SvEndTagSpace().
    m_strEndTagSpace.clear();
    {//B
      _l_data_range drTag;
      _rltok.GetTokenDataRange( drTag );
      _TyStrView svTag;
      _rltok.KGetStringView( svTag, drTag );
      const size_t knLenBefore = 2; // "</" - and the name, which has no white space, is before what we find.
      const _TyChar kchBeforeEnd = ( svTag.length() > 3 ) ? svTag[ svTag.length() - 2 ] : _TyChar( 0 );
      if ( ( _TyChar( 0x20 ) == kchBeforeEnd ) || ( _TyChar( 0x9 ) == kchBeforeEnd ) || ( _TyChar( 0xd ) == kchBeforeEnd ) || ( _TyChar( 0xa ) == kchBeforeEnd ) )
      {
        // Back from the '>' over the white space: the name has none.
        size_t nPos = svTag.length() - 1;
        while ( ( nPos > knLenBefore ) && ( ( _TyChar( 0x20 ) == svTag[nPos-1] ) || ( _TyChar( 0x9 ) == svTag[nPos-1] ) || ( _TyChar( 0xd ) == svTag[nPos-1] ) || ( _TyChar( 0xa ) == svTag[nPos-1] ) ) )
          --nPos;
        m_strEndTagSpace = svTag.substr( nPos, svTag.length() - 1 - nPos );
      }
    }//EB
    // The context is always popped later in FNextTag().
    _TyLexToken ltokEat( std::move( _rltok ) ); // eat the token.
  }
  // A reference to an entity which is not declared - 4.1 Entity Declared. In a document whose declarations are all where
  //  we read them - it has no external subset and its internal subset no reference to a parameter entity - or which is
  //  declared standalone it is an error ( WFC ). Otherwise the entity may be declared where we didn't read and it is a
  //  matter of validity ( VC ): the reference remains, and its text is not included - see s_kdtfEntityRefNotIncluded.
  bool _FEntityNotDeclaredIsOfValidity() const
  {
    return !_FStandalone() && ( m_spDtd->m_optExternalId.has_value() || m_spDtd->m_fHasParameterEntityReferences );
  }
  void _EntityNotDeclared( _TyStrView _svName, const char * _pszWhere )
  {
    if ( !_FEntityNotDeclaredIsOfValidity() )
      THROWXMLPARSEEXCEPTION( "The %s refers to the entity [%s] which is not declared.", _pszWhere, StrConvertString< char >( _svName ).c_str() );
    if constexpr ( _TyXmlTraits::s_kfValidating )
    {
      if ( !!m_upValidator )
        m_upValidator->EntityNotDeclared( _svName );
    }
  }
  void _ProcessXMLDecl( _TyLexToken & _rltok )
  {
    // Pull the info we want out of it to store in the cursor.
    m_XMLDeclProperties.FromXMLDeclToken( _rltok );
    // 4.3.3: the document is in the encoding which it declares - unless whoever opened it said not to hold it to that.
    if ( m_XMLDeclProperties.m_fHasEncoding && ( !m_pXp || m_pXp->FCheckDeclaredEncoding() ) )
    {
      typedef typename _TyXmlTraits::_TyTransport _TyTransport;
      EFileCharacterEncoding efceRead = EfceOfCharType< _TyChar >();
      bool fByteOrderKnown = ( 1 == sizeof( _TyChar ) );
      if constexpr ( requires { { _TyTransport::GetSupportedCharacterEncoding() } -> std::convertible_to< EFileCharacterEncoding >; } )
      {
        efceRead = _TyTransport::GetSupportedCharacterEncoding();
        fByteOrderKnown = true;
      }
      VerifyThrowSz( FDeclaredEncodingAgrees( _TyStrView( m_XMLDeclProperties.m_strEncoding ), efceRead, fByteOrderKnown ),
        "The XML declaration names the encoding [%s] but the document is read as %s: a document is to be in the encoding it declares - "
        "xml_parser<>::SetCheckDeclaredEncoding( false ) reads it nonetheless.", StrConvertString< char >( m_XMLDeclProperties.m_strEncoding ).c_str(), PszCharacterEncodingShort( efceRead ) );
    }
    Assert( !m_lContexts.size() );
    // Even if the caller is ignoring the XMLDecl we will start the root context with it if it is present.
    _PushNewContext( _rltok ); // Push on current context. There will be no current context so one will be created.
  }
  void _ProcessComment( _TyLexToken & _rltok )
  {
    _ValidateCommentOrProcessingInstruction();
    _l_data_range drToken;
    _rltok.GetTokenDataRange( drToken );
    drToken.m_posBegin += 4; // <!--
    drToken.m_posEnd -= 3; // -->
    // REVIEW:<dbien>: Could strip whitespace off front and rear but...
    _rltok.template emplaceValue< _TyData >( drToken );
    if ( m_fTokenLiteralLineEnds )
      _FlagLiteralLineEnds( _rltok.GetValue() );
    _AppendContextContent( _rltok );
  }
  void _ProcessProcessingInstruction( _TyLexToken & _rltok )
  {
    _ValidateCommentOrProcessingInstruction();
    {//B
      const _TyLexValue & krvalTarget = _rltok[vknProcessingInstruction_PITargetIdx];
      if ( krvalTarget.FHasTypedData() && !krvalTarget.template GetVal< _TyData >().FIsNull() )
      {
        _TyStrView svTarget;
        _rltok.KGetStringView( svTarget, krvalTarget.template GetVal< _TyData >().DataRangeGetSingle().GetRangeBase() );
        // [17]: the target is not "xml" - however it is written: an XML declaration is only at the beginning of the document.
        if ( ( 3 == svTarget.length() ) && ( ( _TyChar( 'x' ) == svTarget[0] ) || ( _TyChar( 'X' ) == svTarget[0] ) ) &&
             ( ( _TyChar( 'm' ) == svTarget[1] ) || ( _TyChar( 'M' ) == svTarget[1] ) ) && ( ( _TyChar( 'l' ) == svTarget[2] ) || ( _TyChar( 'L' ) == svTarget[2] ) ) )
          THROWXMLPARSEEXCEPTION( "The target of a processing instruction may not be \"xml\": an XML declaration may only be at the beginning of the document." );
        // Namespaces in XML, 6: the target of a processing instruction has no colon.
        if ( m_fUseXMLNamespaces && ( _TyStrView::npos != svTarget.find( _TyChar( ':' ) ) ) )
          THROWXMLPARSEEXCEPTION( "The document is not namespace-well-formed: the target [%s] of a processing instruction has a colon.", StrConvertString< char >( svTarget ).c_str() );
      }
    }//EB
    // We may have some "meat" after the PITarget - or not.
    vtyDataPosition posMeatBegin = _rltok[vknProcessingInstruction_MeatIdx].template GetVal< vtyDataPosition >();
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
      if ( m_fTokenLiteralLineEnds )
        _FlagLiteralLineEnds( _rltok[vknProcessingInstruction_MeatIdx] );
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
    if ( m_fTokenLiteralLineEnds )
      _FlagLiteralLineEnds( _rltok.GetValue() );
    if constexpr ( _TyXmlTraits::s_kfValidating )
    {
      if ( !!m_upValidator )
        m_upValidator->CharacterData( false ); // A CDATA section is not white space among elements whatever is in it.
    }
    _AppendContextContent( _rltok );
  }
  void _ProcessCharData( _TyLexToken & _rltok )
  {
    if ( !FInsideDocumentTag() )
    {
      // We should only see plain whitespace CharData before the first tag:
      _TyLexValue & rVal = _rltok.GetValue();
      Assert( rVal.FHasTypedData() );
      const _TyData & rdt = rVal.template GetVal< _TyData >();
      bool fError = !rdt.FContainsSingleDataRange() || ( rdt.DataRangeGetSingle().type() != s_kdtPlainText );
      if ( !fError )
      {
        // ( The text as it is - not the string of the value, which there is no view of if it has a #xD: 2.11. )
        _TyStrView svData;
        _rltok.KGetStringView( svData, rdt.DataRangeGetSingle().GetRangeBase() );
        fError = ( svData.length() != StrSpn( &svData[0], svData.length(), (const _TyChar *)str_array_cast<_TyChar>( STR_XML_WHITESPACE_TOKEN ) ) );
      }
      VerifyThrowSz( !fError, "Found non-whitespace CharData before first tag declaration." );
    }
    else
    if constexpr ( _TyXmlTraits::s_kfValidating )
    {
      if ( !!m_upValidator )
        m_upValidator->CharacterData( _FIsWhiteSpaceCharData( _rltok ) );
    }
    _AppendContextContent( _rltok ); // Push on current context.
  }
// What the user doesn't want of a document which is validated - comments, processing instructions, white space:
  // A comment, a processing instruction or white space is content which an element may not have - one which is
  //  declared EMPTY has none - so the validator is to know of them. Then the lexical analyzer doesn't pass over them
  //  and doesn't filter: it gives us the tokens and we pass over them - _FFilterTokenOfValidated() - where we get
  //  them, so that what we look ahead to is what it would be if the lexical analyzer had. The validator is told of
  //  them when the next token is processed: by then it knows of everything which came before them - the end of an
  //  element is known when it is left, which is after we have looked ahead beyond its end tag.
  // Whether the lexical analyzer filters the character data which is white space - and never makes a token of it.
  bool _FLexFiltersWhitespaceCharData() const
  {
    return m_fFilterWhitespaceCharData && !_TyXmlTraits::s_kfValidating;
  }
  // Whether the token is one which the user doesn't want: then the validator is to be told of it.
  bool _FFilterTokenOfValidated( _TyLexToken const & _rltok )
  {
    switch( _rltok.GetTokenId() )
    {
      case s_knTokenComment:
        if ( !m_fSkipComments )
          return false;
        m_rgfFilteredWhiteSpace.push_back( false );
      break;
      case s_knTokenProcessingInstruction:
        if ( !m_fSkipPI )
          return false;
        m_rgfFilteredWhiteSpace.push_back( false );
      break;
      case s_knTokenCharData:
        // What the lexical analyzer would have filtered: a single piece of plain text which is white space.
        if ( !m_fFilterWhitespaceCharData || !_FIsFilteredWhiteSpaceCharData( _rltok ) )
          return false;
        m_rgfFilteredWhiteSpace.push_back( true );
      break;
      default:
        return false;
    }
    return true;
  }
  void _ValidateWhatWasFiltered()
  {
    if constexpr ( _TyXmlTraits::s_kfValidating )
    {
      if ( m_rgfFilteredWhiteSpace.empty() )
        return;
      if ( !!m_upValidator && FInsideDocumentTag() )
      {
        for ( const bool kfWhiteSpace : m_rgfFilteredWhiteSpace )
        {
          if ( kfWhiteSpace )
            m_upValidator->CharacterData( true );
          else
            m_upValidator->CommentOrProcessingInstruction();
        }
      }
      m_rgfFilteredWhiteSpace.clear();
    }
  }
  bool _FIsFilteredWhiteSpaceCharData( _TyLexToken const & _rltok ) const
  {
    const _TyLexValue & krval = _rltok.GetValue();
    if ( !krval.FHasTypedData() )
      return false;
    const _TyData & krdt = krval.template GetVal< _TyData >();
    if ( krdt.FIsNull() || !krdt.FContainsSingleDataRange() || ( s_kdtPlainText != DtGetType( krdt.DataRangeGetSingle().type() ) ) )
      return false;
    _TyStrView sv;
    _rltok.KGetStringView( sv, krdt.DataRangeGetSingle().GetRangeBase() );
    return sv.length() == StrSpn( sv.data(), sv.length(), (const _TyChar *)str_array_cast<_TyChar>( STR_XML_WHITESPACE_TOKEN ) );
  }
// The references to entities within the values of the attributes of a tag - 3.1, 3.3.3:
  //  - WFC: No External Entity References [41]: not to an external entity - nor to an unparsed one ( 4.4.4 ).
  //  - WFC: No < in Attribute Values [41]: the replacement text of the entity - and of those to which that refers - has no '<'.
  //  - WFC: No Recursion [68].
  //  - A reference to an entity whose text is other within the value of an attribute than in content - its white space is
  //    spaces there - is flagged so: see s_kdtfEntityRefInAttrValue.
  // A reference to an entity which is not declared is left for whoever asks for the value: that is told then.
  // This is called before the names of the token are processed and only if the DTD declares general entities.
  void _ProcessEntityReferencesOfAttrValues( _TyLexToken & _rltok )
  {
    _TyLexValue & rrgAttrs = _rltok[vknAttributesIdx];
    const size_t knAttrs = rrgAttrs.GetSize();
    auto const & rmapEntities = m_pXp->GetUserObj().GetEntityMap();
    auto const & rmapEntitiesAttrValues = m_pXp->GetUserObj().GetEntityMapAttrValues();
    for ( size_t nAttr = 0; nAttr < knAttrs; ++nAttr )
    {
      _TyLexValue & rvalValue = rrgAttrs[nAttr][vknAttr_ValueIdx];
      if ( !rvalValue.FHasTypedData() )
        continue;
      _TyData & rdt = rvalValue.template GetVal< _TyData >();
      if ( rdt.FIsNull() || rdt.FContainsSingleDataRange( s_kdtPlainText ) )
        continue; // As nearly all are.
      rdt.Apply(
        [this,&_rltok,&rmapEntities,&rmapEntitiesAttrValues]( _l_data_typed_range * _pdtrBegin, _l_data_typed_range * const _pdtrEnd )
        {
          for ( ; _pdtrEnd != _pdtrBegin; ++_pdtrBegin )
          {
            if ( _pdtrBegin->FIsNull() || ( s_kdtEntityRef != DtGetType( _pdtrBegin->type() ) ) )
              continue;
            _TyStrView svName;
            _rltok.KGetStringView( svName, _pdtrBegin->GetRangeBase() );
            auto citEntityOfMap = rmapEntities.find( svName );
            if ( rmapEntities.end() != citEntityOfMap )
            {
              if ( !rmapEntitiesAttrValues.empty() && ( rmapEntitiesAttrValues.end() != rmapEntitiesAttrValues.find( svName ) ) )
                _pdtrBegin->m_nType |= s_kdtfEntityRefInAttrValue;
              _AccountEntityExpansion( citEntityOfMap->second.length() );
              continue;
            }
            const xml_dtd_entity< _TyChar > * pEntity = m_spDtd->PGetGeneralEntity( svName );
            if ( !pEntity )
            {
              _EntityNotDeclared( svName, "value of an attribute" );
              _pdtrBegin->m_nType |= s_kdtfEntityRefNotIncluded;
              continue;
            }
            const string strName = StrConvertString< char >( svName );
            // WFC: Entity Declared [68] - of a document which is declared standalone.
            if ( pEntity->m_fDeclaredExternally && _FStandalone() )
              THROWXMLPARSEEXCEPTION( "The document is declared standalone and the value of an attribute refers to the entity [%s] - which is not declared in its internal subset.", strName.c_str() );
            if ( pEntity->FIsUnparsed() )
              THROWXMLPARSEEXCEPTION( "The value of an attribute refers to the unparsed entity [%s].", strName.c_str() );
            _TyStdStr strText;
            switch( m_spDtd->EGetGeneralEntityTextInAttributeValue( svName, strText ) )
            {
              case exdeExternal:
                THROWXMLPARSEEXCEPTION( "The value of an attribute refers to the external entity [%s] - or to an entity which refers to one.", strName.c_str() );
              break;
              case exdeMarkup:
                THROWXMLPARSEEXCEPTION( "The value of an attribute refers to the entity [%s] and there is a '<' in its replacement text - or in that of an entity which it refers to.", strName.c_str() );
              break;
              case exdeRecursive:
                THROWXMLPARSEEXCEPTION( "The entity [%s] contains a reference to itself.", strName.c_str() );
              break;
              case exdeNotDeclared:
                THROWXMLPARSEEXCEPTION( "The entity [%s] - which the value of an attribute refers to - refers to an entity which is not declared.", strName.c_str() );
              break;
              case exdeLimit:
                THROWXMLPARSEEXCEPTION( "The entity [%s] - which the value of an attribute refers to - stands for more text, or for entities deeper within each other, than is allowed.", strName.c_str() );
              break;
              default:
                THROWXMLPARSEEXCEPTION( "The replacement text of the entity [%s] - which the value of an attribute refers to - is not well formed.", strName.c_str() );
              break;
            }
          }
        }
      );
    }
  }
// The names of tags and of attributes as the lexical analyzer has them - see QName in the generators ( xmlpgen_utf8.cpp, etc. ):
  // The name is any Name [5] of XML in two pieces: the first - at vknNameIdx - is the name up to its first colon which
  //  is not its first character and the second - at vknNamespaceIdx - is what is after that colon. A name which ends
  //  with that colon has no second piece - as a name which has no such colon: we look at the character after the first
  //  piece - the token has one - and the colon is then of the first piece from here, so that the pieces are all of
  //  the name as it is written.
  // With namespaces the name is to be a QName [7] of Namespaces in XML - an NCName [4], or two of them with a colon
  //  between: the first piece doesn't begin with a colon, the name doesn't end with one and the second piece - if
  //  there is one - is an NCName: it has no colon and begins as a name does. A name which isn't is an error: the
  //  document is not namespace-well-formed. Without namespaces every Name is a name.
  // This is called before anything else is done with the name.
  void _CheckName( _TyLexToken const & _rltok, _TyLexValue & _rrgvalName )
  {
    _l_data_typed_range & rdtrFirst = _rrgvalName[vknNameIdx].template GetVal< _TyData >().DataRangeGetSingle();
    _TyData const & krdtSecond = _rrgvalName[vknNamespaceIdx].template GetVal< _TyData >();
    bool fQName = true;
    if ( krdtSecond.FIsNull() )
    {
      _TyStrView svAfter;
      _rltok.KGetStringView( svAfter, _l_data_range( rdtrFirst.end(), rdtrFirst.end() + 1 ) );
      if ( !svAfter.empty() && ( _TyChar( ':' ) == svAfter.front() ) )
      {
        rdtrFirst.m_posEnd += 1;
        fQName = false;
      }
    }
    if ( !m_fUseXMLNamespaces )
      return;
    if ( fQName )
    {
      _TyStrView svFirst;
      _rltok.KGetStringView( svFirst, rdtrFirst.GetRangeBase() );
      fQName = ( _TyChar( ':' ) != svFirst.front() );
      if ( fQName && !krdtSecond.FIsNull() )
      {
        _TyStrView svSecond;
        _rltok.KGetStringView( svSecond, krdtSecond.DataRangeGetSingle().GetRangeBase() );
        fQName = ( _TyStrView::npos == svSecond.find( _TyChar( ':' ) ) ) && _TyDtd::FIsName( svSecond );
      }
    }
    if ( !fQName )
      THROWXMLPARSEEXCEPTION( "The document is not namespace-well-formed: the name [%s] is not a qualified name - it has a colon where one may not be.",
        StrConvertString< char >( _SvNameAsWritten( _rltok, _rrgvalName ) ).c_str() );
  }
  // The name of the tag and those of its attributes.
  void _CheckNamesOfTag( _TyLexToken & _rltok )
  {
    _CheckName( _rltok, _rltok[vknTagNameIdx] );
    _TyLexValue & rrgAttrs = _rltok[vknAttributesIdx];
    const size_t knAttrs = rrgAttrs.GetSize();
    for ( size_t nAttr = 0; nAttr < knAttrs; ++nAttr )
      _CheckName( _rltok, rrgAttrs[nAttr] );
  }
  // Namespaces in XML, 3: the prefix xml is bound to its namespace name and to no other - and no other prefix is bound
  //  to that; the prefix xmlns is not declared and nothing is bound to its namespace name. _rsvPrefix is empty for a
  //  declaration of the default namespace.
  void _CheckReservedNamespace( _TyStrView const & _rsvPrefix, _TyStrView const & _rsvUri ) const
  {
    // ( The arrays are what the views are of: they are to be here for as long as the views are. )
    static constexpr auto s_krgchXmlUri = str_array_cast< _TyChar >( "http://www.w3.org/XML/1998/namespace" );
    static constexpr auto s_krgchXmlnsUri = str_array_cast< _TyChar >( "http://www.w3.org/2000/xmlns/" );
    const _TyStrView svXmlUri( s_krgchXmlUri.c_str() );
    const _TyStrView svXmlnsUri( s_krgchXmlnsUri.c_str() );
    if ( _rsvPrefix == _TyStrView( str_array_cast< _TyChar >( "xmlns" ) ) )
      THROWXMLPARSEEXCEPTION( "The document is not namespace-well-formed: the prefix xmlns is declared." );
    if ( _rsvUri == svXmlnsUri )
      THROWXMLPARSEEXCEPTION( "The document is not namespace-well-formed: the namespace name of the prefix xmlns is bound to [%s].",
        _rsvPrefix.empty() ? "the default namespace" : StrConvertString< char >( _rsvPrefix ).c_str() );
    const bool kfXmlPrefix = ( _rsvPrefix == _TyStrView( str_array_cast< _TyChar >( "xml" ) ) );
    if ( kfXmlPrefix != ( _rsvUri == svXmlUri ) )
      THROWXMLPARSEEXCEPTION( kfXmlPrefix ? "The document is not namespace-well-formed: the prefix xml is bound to a namespace name which is not its own."
                                          : "The document is not namespace-well-formed: the namespace name of the prefix xml is bound to something else." );
  }
  void _ProcessTagName( _TyLexValue & _rrgvalName )
  {
    // Tag names must match literally between start and end tags.
    // As well we want the end user to see a uniform representation of tag names.
    // So if a namespace specifier is present then we will reset the end position of the this namespace specifier to include the ':' and the namespace name.
    _TyData const & krdtName = _rrgvalName[vknNamespaceIdx].template GetVal< _TyData >();
    if ( !krdtName.FIsNull() )
    {
      _TyData & rdtPrefix = _rrgvalName[vknNameIdx].template GetVal< _TyData >();
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
    // First thing to do is to move through all attributes and obtain all namespace declarations and at the same time ensure they are valid.
    typedef AllocaList< const typename _TyUriAndPrefixMap::value_type *, false > _TyListPrefixes;
    _TyListPrefixes lPrefixesUsed; // Can't declare the same prefix in the same tag.
    _TyLexValue & rrgAttrs = _rltok[vknAttributesIdx];
    vtySignedLvalueInt nNamespaceDecls = 0; // Count these so we can store with the tag and then know when we have finished processing on end-tag.
    const size_t knAttrs = rrgAttrs.GetSize();
    for ( size_t nAttr = 0; nAttr < knAttrs; ++nAttr )
    {
      _TyLexValue & rrgAttr = rrgAttrs[nAttr];
      _TyStrView svName;
      rrgAttr[vknNameIdx].KGetStringView( _rltok, svName );
      if ( !svName.compare( str_array_cast< _TyChar >("xmlns") ) )
      {
        // We have a namespace declaration!
        _TyStrView svPrefix;
        rrgAttr[vknNamespaceIdx].KGetStringView( _rltok, svPrefix );
        const typename _TyUriAndPrefixMap::value_type & rvtPrefix = m_pXp->RStrAddPrefix( svPrefix );
        // Check for uniqueness of prefix:
        VerifyThrowSz( !lPrefixesUsed.FFind( &rvtPrefix ), "Namespaces Validity: Cannot use same namespace prefix more than once within the same tag." );
        ALLOCA_LIST_PUSH( lPrefixesUsed, &rvtPrefix );
        // The namespace name is the value of the attribute - as any value: what its references stand for and normalized.
        _TyStrView svUri;
        _TyStdStr strUri;
        if ( !rrgAttr[vknAttr_ValueIdx].FEmptyTypedData() )
          (void)_rltok.FGetStringViewOrString( svUri, strUri, rrgAttr[vknAttr_ValueIdx] );
        _CheckReservedNamespace( svPrefix, svUri );
        _ProcessTagName( rrgAttr ); // For xmlns attr names we process the same as we process the tag name. We want to leave both "xmlns" and any colon and prefix in the attr name.
        // Add the (prefix,uri) pair and put a _TyXmlNamespaceValueWrap in the _l_value to track its lifetime:
        _TyXmlNamespaceValueWrap & rnsvwNew = rrgAttr[vknNamespaceIdx].emplaceVal( m_mapNamespaces.GetNamespaceValueWrap( *m_pXp, enrtAttrNamespaceDeclReference, svPrefix, svUri ) );
        if ( rnsvwNew.FIsNamespaceDeclaration() )
          ++nNamespaceDecls; // rnsvwNew may be a reference and not a declaration since it may already be an active namespace.
      }
    }
    // Now parse the namespace on the tag:
    {//B
      _TyLexValue & rrgTagName = _rltok[vknNameIdx];
      rrgTagName[vknTagName_NNamespaceDeclsIdx].emplaceVal( nNamespaceDecls ); // record the count of namespace decls so we can know when we are done processing the end-tag.
      _TyData const & krdtName = rrgTagName[vknNamespaceIdx].template GetVal< _TyData >();
      if ( !krdtName.FIsNull() )
      {
        _TyData & rdtPrefix = rrgTagName[vknNameIdx].template GetVal< _TyData >();
        _TyStrView svPrefix;
        rrgTagName[vknNameIdx].KGetStringView( _rltok, svPrefix );
        // Update the tag name so that it includes the prefix as is necessary for XML end tag matching.
        rdtPrefix.DataRangeGetSingle().m_posEnd = krdtName.DataRangeGetSingle().m_posEnd;
        rrgTagName[vknNamespaceIdx].emplaceVal( m_mapNamespaces.GetNamespaceValueWrap( enrtTagNameReference, &svPrefix ) );
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
      if ( rrgAttr[vknNamespaceIdx].template FIsA< _TyXmlNamespaceValueWrap >() )
        continue; // This is an xmlns attribute that was processed above.
      if ( rrgAttr[vknNamespaceIdx].FEmptyTypedData() )
      {
        // No default namespaces for attribute names - this has no namespace:
        rrgAttr[vknNamespaceIdx].emplaceVal( false );
        continue;
      }
      _TyStrView svPrefix;
      rrgAttr[vknNameIdx].KGetStringView( _rltok, svPrefix );
      _TyData const & krdtName = rrgAttr[vknNamespaceIdx].template GetVal< _TyData >();
      _TyData & rdtPrefix = rrgAttr[vknNameIdx].template GetVal< _TyData >();
      if ( m_fIncludePrefixesInAttrNames && !m_fCheckDuplicateAttrs )
        rdtPrefix.DataRangeGetSingle().m_posEnd = krdtName.DataRangeGetSingle().m_posEnd;
      else
        rdtPrefix.DataRangeGetSingle() = krdtName.DataRangeGetSingle(); // The user just wants the name without the prefix.
      rrgAttr[vknNamespaceIdx].emplaceVal( m_mapNamespaces.GetNamespaceValueWrap( enrtAttrNameReference, &svPrefix ) );
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
        if ( !rrgAttr[vknNamespaceIdx].template FIsA< _TyXmlNamespaceValueWrap >() )
        {
          Assert( rrgAttr[vknNamespaceIdx].template FIsA< bool >() && !rrgAttr[vknNamespaceIdx].template GetVal< bool >() ); // Not in any namespace at all.
          continue; // This is an xmlns attribute that was processed above.
        }
        _TyXmlNamespaceValueWrap const & rnvw = rrgAttr[vknNamespaceIdx].template GetVal< _TyXmlNamespaceValueWrap >();
        ENamespaceReferenceType nrtCur;
        if ( rnvw.FIsNamespaceDeclaration() || ( rnvw.FIsNamespaceReference( &nrtCur ) && ( enrtAttrNamespaceDeclReference == nrtCur ) ) )
          continue;
        _TyData & rdtName = rrgAttr[vknNameIdx].template GetVal< _TyData >();
        rdtName.DataRangeGetSingle().m_posBegin -= 1 + rnvw.RStringPrefix().length(); // push back to include the prefix.
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
      rrgTagName[vknTagName_NNamespaceDeclsIdx].template emplaceArgs< vtySignedLvalueInt >( 0 ); // record the count of namespace decls so we can know when we are done processing the end-tag.
    }//EB
    if ( m_fCheckDuplicateAttrs )
      _CheckDuplicateAttrs(  _rltok );
  }
  void _CheckDuplicateAttrs( _TyLexToken & _rltok )
  {
    return CheckDuplicateTokenAttrs( m_fStrict, _rltok, false );
  }
// The default values of attributes - 3.3.2:
  // An attribute to add to a tag - and the namespace which it declares if it does.
  struct _TyDefaultAttr
  {
    const xml_dtd_attdef< _TyChar > * m_pAttDef{ nullptr };
    bool m_fNamespaceDecl{ false };
    _TyXmlNamespaceValueWrap m_xnvwNamespaceDecl;
  };
  typedef vector< _TyDefaultAttr > _TyRgDefaultAttrs;
  // The name of a tag or of an attribute as it is written - prefix, colon and all - before the names of the token are
  //  processed: the lexical analyzer has the first part of the name at vknNameIdx and what is after a colon at vknNamespaceIdx.
  _TyStrView _SvNameAsWritten( _TyLexToken const & _rltok, _TyLexValue const & _rrgvName ) const
  {
    _TyData const & krdtFirst = _rrgvName[vknNameIdx].template GetVal< _TyData >();
    _TyData const & krdtAfterColon = _rrgvName[vknNamespaceIdx].template GetVal< _TyData >();
    const _l_data_range dr( krdtFirst.DataRangeGetSingle().begin(), krdtAfterColon.FIsNull() ? krdtFirst.DataRangeGetSingle().end() : krdtAfterColon.DataRangeGetSingle().end() );
    _TyStrView sv;
    _rltok.KGetStringView( sv, dr );
    return sv;
  }
  // What the DTD declares for the attributes of the tag's element type:
  //  - Find the attributes which it declares with a default value and which the tag doesn't have.
  //  - Flag the values of the attributes which the tag has and whose declared type is not CDATA.
  // This is called before the names of the token are processed.
  void _GetDefaultAttributes( _TyLexToken & _rltok, _TyRgDefaultAttrs & _rrgDefaultAttrs )
  {
    const _TyLexValue & krvalTag = _rltok.GetValue();
    const vector< xml_dtd_attdef< _TyChar > > * prgAttDefs = m_spDtd->PGetAttList( _SvNameAsWritten( _rltok, krvalTag[vknTagNameIdx] ) );
    if ( !prgAttDefs )
      return;
    const _TyLexValue & krrgAttrs = krvalTag[vknAttributesIdx];
    const size_t knAttrs = krrgAttrs.GetSize();
    for ( xml_dtd_attdef< _TyChar > const & rad : *prgAttDefs )
    {
      // Only these declarations change anything about the tag: a default value, or a type which is not CDATA.
      const bool kfTokenized = ( exdatCDATA != rad.m_eType );
      if ( !rad.m_fHasDefaultValue && !kfTokenized )
        continue;
      size_t nAttr = 0;
      for ( ; ( nAttr < knAttrs ) && ( _SvNameAsWritten( _rltok, krrgAttrs[nAttr] ) != _TyStrView( rad.m_strName ) ); ++nAttr )
        ;
      if ( nAttr < knAttrs )
      {
        // The tag has the attribute. If its type is not CDATA then the value is without the spaces at its ends and
        //  with one space for a sequence of them ( 3.3.3 ). The pieces of the value are flagged and it is so of the
        //  string which is had from them - they remain all of what the tag has: see s_kdtfAttrTokenized.
        if ( kfTokenized )
        {
          _TyLexValue & rvalValue = _rltok[vknAttributesIdx][nAttr][vknAttr_ValueIdx];
          if ( rvalValue.FHasTypedData() && !rvalValue.template GetVal< _TyData >().FIsNull() )
          {
            rvalValue.template GetVal< _TyData >().Apply(
              []( _l_data_typed_range * _pdtrBegin, _l_data_typed_range * const _pdtrEnd )
              {
                for ( ; _pdtrEnd != _pdtrBegin; ++_pdtrBegin )
                  _pdtrBegin->m_nType |= s_kdtfAttrTokenized;
              }
            );
          }
        }
        continue;
      }
      if ( !rad.m_fHasDefaultValue )
        continue;
      _TyDefaultAttr & rda = _rrgDefaultAttrs.emplace_back();
      rda.m_pAttDef = &rad;
      if ( m_fUseXMLNamespaces )
      {
        // A declaration of a namespace: "xmlns" - the default namespace - or "xmlns:" and the prefix.
        const _TyStrView svName( rad.m_strName );
        static constexpr size_t knLenXmlns = 5;
        if ( !svName.compare( 0, knLenXmlns, str_array_cast< _TyChar >("xmlns") ) && ( ( svName.length() == knLenXmlns ) || ( _TyChar(':') == svName[knLenXmlns] ) ) )
        {
          _TyStrView svPrefix = ( svName.length() == knLenXmlns ) ? _TyStrView() : svName.substr( knLenXmlns + 1 );
          _TyStrView svUri( rad.m_strDefaultValue );
          _CheckReservedNamespace( svPrefix, svUri );
          rda.m_xnvwNamespaceDecl = m_mapNamespaces.GetNamespaceValueWrap( *m_pXp, enrtAttrNamespaceDeclReference, svPrefix, svUri );
          rda.m_fNamespaceDecl = true;
        }
      }
    }
  }
  // Add the attributes to the tag. This is called after the names of the token have been processed: these are made as
  //  those are left - but their names are strings since they are not in the text of the document, and their values are
  //  those of the DTD: see xml_attr_default_value.
  void _AddDefaultAttributes( _TyLexToken & _rltok, _TyRgDefaultAttrs & _rrgDefaultAttrs )
  {
    _TyLexValue & rrgAttrs = _rltok[vknAttributesIdx];
    vtySignedLvalueInt nNamespaceDecls = 0;
    for ( _TyDefaultAttr & rda : _rrgDefaultAttrs )
    {
      _TyStrView svName( rda.m_pAttDef->m_strName );
      _TyLexValue & rrgAttr = rrgAttrs[ rrgAttrs.GetSize() ]; // This adds one.
      rrgAttr.SetSize( vknAttr_ArrayCount );
      const size_t nPosColon = svName.find( _TyChar(':') );
      if ( rda.m_fNamespaceDecl )
      {
        // As _ProcessNamespaces() leaves a declaration which is written in the tag: the whole name and the namespace.
        _TyXmlNamespaceValueWrap & rxnvw = rrgAttr[vknNamespaceIdx].emplaceVal( std::move( rda.m_xnvwNamespaceDecl ) );
        if ( rxnvw.FIsNamespaceDeclaration() )
          ++nNamespaceDecls;
      }
      else
      if ( m_fUseXMLNamespaces && ( _TyStrView::npos != nPosColon ) )
      {
        // A name with a prefix: the namespace of the prefix - and the name with the prefix or without as the user wants it.
        const _TyStrView svPrefix = svName.substr( 0, nPosColon );
        rrgAttr[vknNamespaceIdx].emplaceVal( m_mapNamespaces.GetNamespaceValueWrap( enrtAttrNameReference, &svPrefix ) );
        if ( !m_fIncludePrefixesInAttrNames )
          svName.remove_prefix( nPosColon + 1 );
      }
      else
        rrgAttr[vknNamespaceIdx].emplaceVal( false ); // In no namespace.
      rrgAttr[vknNameIdx].emplaceVal( _TyStdStr( svName ) );
      // The value is the DTD's - nothing is copied - and it is known for what it is: see xml_attr_default_value.
      rrgAttr[vknAttr_ValueIdx].emplaceVal( xml_attr_default_value< _TyChar >( _TyStrView( rda.m_pAttDef->m_strDefaultValue ) ) );
      rrgAttr[vknAttr_FDoubleQuoteIdx].emplaceVal( true );
    }
    if ( !!nNamespaceDecls )
      _rltok[vknTagNameIdx][vknTagName_NNamespaceDeclsIdx].template GetVal< vtySignedLvalueInt >() += nNamespaceDecls; // So that they are undeclared with the tag.
  }
// Validation - see FIsValid():
  // Tell the validator of the tag. This is called before the names of the token are processed - the DTD knows the names as
  //  they are written - and after the values of the attributes whose declared type is not CDATA have been flagged.
  void _ValidateStartTag( _TyLexToken & _rltok )
  {
    if ( !m_upValidator )
    {
      m_upValidator = make_unique< _TyValidator >( m_spDtd.get() );
      m_upValidator->SetThrowOnError( m_fThrowOnValidityError );
      m_upValidator->SetStandalone( m_XMLDeclProperties.m_fHasStandalone && m_XMLDeclProperties.m_fStandalone && !!m_spDtd );
      m_upValidator->SetUseXMLNamespaces( m_fUseXMLNamespaces );
    }
    const _TyLexValue & krvalTag = _rltok.GetValue();
    const _TyLexValue & krrgAttrs = krvalTag[vknAttributesIdx];
    const size_t knAttrs = krrgAttrs.GetSize();
    // The values: a view of the document where the value is one piece of it as it is and otherwise a string.
    if ( m_rgstrValidateValues.size() < knAttrs )
      m_rgstrValidateValues.resize( knAttrs );
    m_rgValidateAttrs.resize( knAttrs );
    for ( size_t nAttr = 0; nAttr < knAttrs; ++nAttr )
    {
      xml_validate_attr< _TyChar > & rattr = m_rgValidateAttrs[nAttr];
      rattr.m_svName = _SvNameAsWritten( _rltok, krrgAttrs[nAttr] );
      rattr.m_svValue = _TyStrView();
      const _TyLexValue & krvalValue = krrgAttrs[nAttr][vknAttr_ValueIdx];
      if ( !krvalValue.FEmptyTypedData() )
      {
        m_rgstrValidateValues[nAttr].clear();
        (void)_rltok.FGetStringViewOrString( rattr.m_svValue, m_rgstrValidateValues[nAttr], krvalValue );
        if ( m_upValidator->FGetStandalone() )
        {
          // VC: Standalone Document Declaration - what the value owes to declarations which are external:
          _ValidateStandaloneEntityReferences( _rltok, krvalValue );
          // If its type is declared so and without the type the value would be another - only white space was taken away, so by its length.
          const xml_dtd_attdef< _TyChar > * pAttDef = m_spDtd->PGetAttDef( _SvNameAsWritten( _rltok, krvalTag[vknTagNameIdx] ), rattr.m_svName );
          if ( !!pAttDef && pAttDef->m_fDeclaredExternally && ( exdatCDATA != pAttDef->m_eType ) && krvalValue.FHasTypedData() )
          {
            _TyLexValue valAsCDATA( krvalValue.template GetVal< _TyData >() );
            valAsCDATA.template GetVal< _TyData >().Apply(
              []( _l_data_typed_range * _pdtrBegin, _l_data_typed_range * const _pdtrEnd )
              {
                for ( ; _pdtrEnd != _pdtrBegin; ++_pdtrBegin )
                  _pdtrBegin->m_nType &= ~s_kdtfAttrTokenized;
              }
            );
            _TyStdStr strAsCDATA;
            _rltok.GetString( strAsCDATA, valAsCDATA );
            if ( strAsCDATA.length() != rattr.m_svValue.length() )
              m_upValidator->StandaloneViolation( "the value of an attribute is normalized by the type of the attribute", rattr.m_svName );
          }
        }
      }
    }
    m_upValidator->StartTag( _SvNameAsWritten( _rltok, krvalTag[vknTagNameIdx] ), m_rgValidateAttrs.data(), m_rgValidateAttrs.data() + knAttrs );
  }
  void _ValidateCommentOrProcessingInstruction()
  {
    if constexpr ( _TyXmlTraits::s_kfValidating )
    {
      if ( !!m_upValidator && FInsideDocumentTag() )
        m_upValidator->CommentOrProcessingInstruction();
    }
  }
  // Whether character data is white space which matches S [3] - which is all that may be among the elements of an element
  //  with element content ( 3.2.1 ): white space written as itself, or a reference to an entity whose text is that. A
  //  reference to a character is not, whatever the character.
  // VC: Standalone Document Declaration: the references to entities in the pieces of a value - of an attribute or
  //  character data - of a document which is declared standalone are not to entities which are declared externally.
  void _ValidateStandaloneEntityReferences( _TyLexToken const & _rltok, _TyLexValue const & _rval )
  {
    if ( !_rval.FHasTypedData() )
      return;
    _rval.template GetVal< _TyData >().Apply(
      [this,&_rltok]( const _l_data_typed_range * _pdtrBegin, const _l_data_typed_range * const _pdtrEnd )
      {
        for ( ; _pdtrEnd != _pdtrBegin; ++_pdtrBegin )
        {
          if ( _pdtrBegin->FIsNull() || ( s_kdtEntityRef != DtGetType( _pdtrBegin->type() ) ) )
            continue;
          _TyStrView svEntity;
          _rltok.KGetStringView( svEntity, _pdtrBegin->GetRangeBase() );
          const xml_dtd_entity< _TyChar > * pEntity = m_spDtd->PGetGeneralEntity( svEntity );
          if ( !!pEntity && pEntity->m_fDeclaredExternally )
            m_upValidator->StandaloneViolation( "refers to the entity", svEntity );
        }
      }
    );
  }
  // If the character data refers to an entity whose text is not read then nothing is said of what that stands for:
  //  see FValidationIncomplete().
  bool _FIsWhiteSpaceCharData( _TyLexToken const & _rltok )
  {
    const _TyLexValue & krval = _rltok.GetValue();
    if ( !krval.FHasTypedData() )
      return false;
    if ( m_upValidator->FGetStandalone() )
      _ValidateStandaloneEntityReferences( _rltok, krval );
    bool fWhiteSpace = true;
    krval.template GetVal< _TyData >().Apply(
      [this,&_rltok,&fWhiteSpace]( const _l_data_typed_range * _pdtrBegin, const _l_data_typed_range * const _pdtrEnd )
      {
        // ( Every piece is looked at even once it is known that this is not white space: a reference to an entity
        //  whose text is not read may be anywhere in it. )
        for ( ; _pdtrEnd != _pdtrBegin; ++_pdtrBegin )
        {
          if ( _pdtrBegin->FIsNull() )
            continue;
          if ( !fWhiteSpace && ( s_kdtEntityRef != DtGetType( _pdtrBegin->type() ) ) )
            continue;
          _TyStrView sv;
          _rltok.KGetStringView( sv, _pdtrBegin->GetRangeBase() );
          switch( DtGetType( _pdtrBegin->type() ) )
          {
            case s_kdtPlainText:
            break;
            case s_kdtEntityRef:
            {
              auto const & rmapEntities = m_pXp->GetUserObj().GetEntityMap();
              auto citEntity = rmapEntities.find( sv );
              if ( rmapEntities.end() == citEntity )
              {
                if ( !!m_spDtd && !!m_spDtd->PGetGeneralEntity( sv ) )
                {
                  // Declared, and we don't have its text: what is within this element is not known.
                  m_fValidationIncomplete = true;
                  m_upValidator->ContentNotKnown();
                }
                else
                  fWhiteSpace = false;
                continue;
              }
              // The replacement text of the entity as it is - not what its references stand for: a reference to a
              //  character is not white space whatever the character is, in the text of an entity as elsewhere.
              const xml_dtd_entity< _TyChar > * pEntityOfMap = !m_spDtd ? nullptr : m_spDtd->PGetGeneralEntity( sv );
              sv = !pEntityOfMap ? _TyStrView( citEntity->second ) : _TyStrView( pEntityOfMap->m_strValue );
            }
            break;
            default:
              fWhiteSpace = false;
            continue;
          }
          fWhiteSpace = fWhiteSpace && ( sv.length() == StrSpn( sv.data(), sv.length(), (const _TyChar *)str_array_cast<_TyChar>( STR_XML_WHITESPACE_TOKEN ) ) );
        }
      }
    );
    return fWhiteSpace;
  }
  void _UpdateSkip()
  {
    if ( !m_fUpdateSkip )
      return;
    m_fUpdateSkip = false;
    vtyTokenIdent * ptidCur = &*m_rgSkipTokensCur.begin();
    // When the document is validated the lexical analyzer passes over nothing: a comment or a processing instruction
    //  is content which the validator is to know of - an element which is declared EMPTY has none. We pass over them:
    //  see _FFilterTokenOfValidated().
    if ( !_TyXmlTraits::s_kfValidating )
    {
      if ( m_fSkipComments )
        *ptidCur++ = s_knTokenComment;
      if ( m_fSkipPI )
        *ptidCur++ = s_knTokenProcessingInstruction;
    }
    m_nSkip = ptidCur - &*m_rgSkipTokensCur.begin();
  }
  static constexpr size_t s_knMaxSkippedTokens = 32; // random number.
  typedef array< vtyTokenIdent, s_knMaxSkippedTokens > _TySkipArray;

// DS:
  _TyXmlNamespaceMap m_mapNamespaces; // Make sure this is around for destruction.
  _TyXmlParser * m_pXp{nullptr}; // the pointer to the XML parser we are reading tokens from.
  const _TyStateProto * m_pspStartXmlDecl{nullptr};
  const _TyStateProto * m_pspStartAll{nullptr};
  _TyListReadContexts m_lContexts;
  unique_ptr< _TyLexToken > m_pltokLookahead;
  typename _TyListReadContexts::iterator m_itCurContext{m_lContexts.end()};
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
  _TyStdStr m_strEndTagSpace; // See SvEndTagSpace().
  // This option allows the user to either see the prefixes that are present in the attr names or see just the names.
  bool m_fIncludePrefixesInAttrNames{true};
  // If m_fStrict then we will not be lenient in cases that may be ambiguous wrt to the XML Specifications.
  bool m_fStrict{false};
  // Don't return tokens for CharData composed entirely of whitespace.
  bool m_fFilterWhitespaceCharData{false};
// Skip settings: Ignore the corresponding token types and don't return to the user.
  bool m_fSkipComments{false};
  bool m_fSkipPI{false};
  _TyXMLDeclProperties m_XMLDeclProperties;
  // What the document type declaration declares - if the XML traits support the DTD and the document has one.
  // Not within this object: the validator - and the values which the DTD gives attributes - refer to it, and we may be moved.
  // And not ours alone: tokens of the document refer to it, and they may be kept when we are gone - then it is kept
  //  with them: see XMLDeclAcquireDocumentContext().
  shared_ptr< _TyDtd > m_spDtd;
// The text of general entities which is read as content - see _FGetLexToken():
  // The lexical analyzer of the text of an entity: the text is in memory. Its tokens are made tokens of ours.
  typedef xml_traits< _l_transport_fixedmem< _TyChar >, _TyXmlTraits::s_kfSupportDTD, _TyXmlTraits::s_kfValidating > _TyXmlTraitsEntity;
  typedef typename _TyXmlTraitsEntity::_TyLexTraits _TyLexTraitsEntity;
  typedef typename _TyLexTraitsEntity::_TyToken _TyLexTokenEntity;
  static constexpr size_t s_knMaxEntityDepth = 64; // A limit, not a constraint of the specification.
  template < class t_TyLexTraitsFrame >
  struct _TEntityFrame
  {
    _TyStdStr m_strName; // The entity.
    _TyStrView m_svText; // Its replacement text: in the DTD, or in the map of the text of the external entities.
    __XMLPLEX_NAMESPACE::_lexical_analyzer< t_TyLexTraitsFrame > m_analyzer;
    size_t m_nLexDepth{ 0 }; // How many elements we were within where the reference is.
    unique_ptr< _TyLexToken > m_pltokAfter; // The character data which followed the reference in its token.
    bool m_fLiteralLineEnds{ false }; // See s_kdtfLiteralLineEnds.
  };
  // ( Only with XML traits which support the DTD is there any of this: otherwise the lexical analyzer of the entities is not made. )
  typedef _TEntityFrame< _TyLexTraitsEntity > _TyEntityFrame;
  typedef conditional_t< _TyXmlTraits::s_kfSupportDTD, vector< unique_ptr< _TyEntityFrame > >, monostate > _TyRgEntityFrames;
  _TyRgEntityFrames m_rgupEntityFrames; // The entities whose text we are within: the last is the one being read.
  deque< unique_ptr< _TyLexToken > > m_dqPendingTokens; // Tokens which are ours to give before the lexical analyzer is asked again.
  size_t m_nLexDepth{ 0 }; // How many elements the lexical analyzer is within.
  size_t m_nchEntityExpansion{ 0 }; // See _AccountEntityExpansion().
  bool m_fEntitiesAreContent{ false }; // The DTD declares entities whose text is read as content - see _SkipOrReadToEndCurrentContext().
  bool m_fDtdDeclaresEntities{ false }; // It declares general entities at all - see _ProcessEntityReferencesOfAttrValues().
  bool m_fTokenLiteralLineEnds{ false }; // The token which was last had is of text which is flagged s_kdtfLiteralLineEnds.
  vector< bool > m_rgfFilteredWhiteSpace; // What was passed over since the last token - see _FFilterTokenOfValidated(): true for white space.
  typedef map< _TyStdStr, _TyStdStr, less<> > _TyMapExternalEntityText;
  shared_ptr< _TyMapExternalEntityText > m_spMapExternalEntityText; // The text of the external entities which were read - kept as the DTD is.
  static const _l_state_proto< _TyChar > * _PspStartAllEntity()
  {
    if constexpr ( is_same_v< _TyChar, char32_t > )
      return (const _l_state_proto< _TyChar > *)&startUTF32All< _TyLexTraitsEntity >;
    else
    if constexpr ( is_same_v< _TyChar, char16_t > )
      return (const _l_state_proto< _TyChar > *)&startUTF16All< _TyLexTraitsEntity >;
    else
      return (const _l_state_proto< _TyChar > *)&startUTF8All< _TyLexTraitsEntity >;
  }
  // Validation - see FIsValid(). The validator is made when the first tag is read: by then the DTD has been read if there is one.
  typedef xml_validator< _TyChar > _TyValidator;
  unique_ptr< _TyValidator > m_upValidator;
  vector< xml_validate_attr< _TyChar > > m_rgValidateAttrs; // The attributes of a tag as the validator is given them.
  vector< _TyStdStr > m_rgstrValidateValues; // Their values where a value is not a piece of the document as it is.
  bool m_fThrowOnValidityError{ false };
  bool m_fValidationIncomplete{ false };
};

// xml_read_cursor_var:
// A variant that is able to hold multiple *character types* of same transports.
// t_TyTpTransports: Is a tuple<> pack of transport types that the cursor will support.
// t_kfSupportDTD, t_kfValidating: as those of xml_traits<> - each of the cursors is one of XML traits which have them.
template < class t_TyTpTransports, bool t_kfSupportDTD, bool t_kfValidating >
class xml_read_cursor_var
{
  typedef xml_read_cursor_var _TyThis;
public:
  typedef t_TyTpTransports _TyTpTransports;
  static constexpr bool s_kfSupportDTD = t_kfSupportDTD;
  static constexpr bool s_kfValidating = t_kfValidating;
  typedef MultiplexTuplePack_t< TGetXmlTraitsOf< t_kfSupportDTD, t_kfValidating >::template _TyGet, _TyTpTransports > _TyTpXmlTraits;
  // Define our variant type - there is no monostate for this (currently - we'll see if we need it).
  typedef MultiplexTuplePack_t< xml_read_cursor, _TyTpXmlTraits, variant > _TyVariant;
  typedef xml_token_var< _TyTpTransports, t_kfSupportDTD > _TyXmlTokenVar;
  typedef optional< _TyXmlTokenVar > _TyOptXmlTokenVar;
  typedef _xml_document_context_transport_var< _TyTpTransports, t_kfSupportDTD, t_kfValidating > _TyXmlDocumentContextVar;

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
  void AssertValid( bool _fIsTail = false ) const
  {
#if ASSERTSENABLED
    std::visit( _VisitHelpOverloadFCall {
      [_fIsTail]( auto & _tCursor )
      {
        _tCursor.AssertValid( _fIsTail );
      }
    }, m_varCursor );    
#endif //ASSERTSENABLED
  }
  _TyVariant & GetVariant()
  {
    return m_varCursor;
  }
  const _TyVariant & GetVariant() const
  {
    return m_varCursor;
  }
// The DTD and validation - see xml_read_cursor<>: what doesn't depend upon the type of character.
  // ( The xml_dtd<> is of the type of character of the document: have it from the cursor of the variant. )
  bool FHasDtd() const
  {
    return std::visit( _VisitHelpOverloadFCall {
      []( auto & _tCursor ) -> bool
      {
        return _tCursor.FHasDtd();
      }
    }, m_varCursor );
  }
  bool FIsValid() const
  {
    return std::visit( _VisitHelpOverloadFCall {
      []( auto & _tCursor ) -> bool
      {
        return _tCursor.FIsValid();
      }
    }, m_varCursor );
  }
  vector< string > const & RgValidityErrors() const
  {
    return std::visit( _VisitHelpOverloadFCall {
      []( auto & _tCursor ) -> vector< string > const &
      {
        return _tCursor.RgValidityErrors();
      }
    }, m_varCursor );
  }
  // The violations of the declarations of the DTD themselves - none if there is no DTD.
  vector< string > const & RgValidityErrorsOfDtd() const
  {
    return std::visit( _VisitHelpOverloadFCall {
      []( auto & _tCursor ) -> vector< string > const &
      {
        static const vector< string > s_rgstrNone;
        return _tCursor.FHasDtd() ? _tCursor.GetDtd().m_rgstrValidityErrors : s_rgstrNone;
      }
    }, m_varCursor );
  }
  bool FValidationIncomplete() const
  {
    return std::visit( _VisitHelpOverloadFCall {
      []( auto & _tCursor ) -> bool
      {
        return _tCursor.FValidationIncomplete();
      }
    }, m_varCursor );
  }
  bool FUseXMLNamespaces() const
  {
    return std::visit( _VisitHelpOverloadFCall {
      []( auto & _tCursor ) -> bool
      {
        return _tCursor.FUseXMLNamespaces();
      }
    }, m_varCursor );
  }
  // See xml_read_cursor<>::SvEndTagSpace(): white space - the same characters whatever the character type.
  std::string StrEndTagSpace() const
  {
    return std::visit( _VisitHelpOverloadFCall {
      []( auto & _tCursor ) -> std::string
      {
        std::string str;
        for ( auto ch : _tCursor.SvEndTagSpace() )
          str += char( ch );
        return str;
      }
    }, m_varCursor );
  }
  void SetThrowOnValidityError( bool _fThrowOnValidityError = true )
  {
    std::visit( _VisitHelpOverloadFCall {
      [_fThrowOnValidityError]( auto & _tCursor )
      {
        _tCursor.SetThrowOnValidityError( _fThrowOnValidityError );
      }
    }, m_varCursor );
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
  bool FPseudoXMLDecl()
  {
    return std::visit( _VisitHelpOverloadFCall {
      []( auto & _tCursor ) -> bool
      {
        return _tCursor.FPseudoXMLDecl();
      }
    }, m_varCursor );    
  }
  // Return any type of XMLDeclProperties.
  template < class t_TyXMLDeclProperties >
  void GetXMLDeclProperties( t_TyXMLDeclProperties & _rxdp )
  {
    std::visit( _VisitHelpOverloadFCall {
      [&_rxdp]( auto & _tCursor )
      {
        _tCursor.GetXMLDeclProperties( _rxdp );
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
