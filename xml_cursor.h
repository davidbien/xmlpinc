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
class xml_read_cursor 
{
  typedef xml_read_cursor _TyThis;
public:
  typedef t_TyXmlTraits _TyXmlTraits;
  typedef typename _TyXmlTraits::_TyLexTraits _TyLexTraits;
  typedef _l_token< _TyLexTraits > _TyLexToken;
  typedef typename _TyXmlTraits::_TyLexTraits _TyLexTraits;
  typedef _l_token< _TyLexTraits > _TyLexToken;
  typedef typename _TyXmlTraits::_TyChar _TyChar;
  typedef typename _TyXmlTraits::_TyStdStr _TyStdStr;
  typedef typename _TyXmlTraits::_TyStrView _TyStrView;
  typedef typename _TyXmlTraits::_TyNamespaceMap _TyNamespaceMap;
  typedef typename _TyXmlTraits::_TyXmlNamespaceValueWrap _TyXmlNamespaceValueWrap;
  typedef _l_value< _TyLexTraits > _TyLexValue;
  typedef _l_data< _TyChar > _TyData;
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


  void SetStrict( bool _fStrict = true )
  {
    m_fStrict = _fStrict;
  }
  bool FGetStrict() const
  {
    return m_fStrict;
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
  void _ProcessTagName( _TyValue & _rrgvalName )
  {
    // Tag names must match literally between start and end tags.
    // As well we want the end user to see a uniform representation of tag names.
    // So if a namespace specifier is present then we will reset the end position of the this namespace specifier to include the ':' and the namespace name.
    _TyData const & krdtName = _rrgvalName[1].GetVal< _TyData >();
    if ( !krdtName.FIsNull() )
    {
      _TyData & rdtPrefix = _rrgvalName[0].GetVal< _TyData >();
      rdtPrefix.DataRangeGetSingle().m_posEnd = krdtName.DataRangeGetSingle().m_posEnd;
    }
    // Leave the name as is - further processing will occur depending on flavor.
  }
  template < class t_TyTransportCtxt >
  _xml_namespace_uri & _RLookupNamespacePrefix( _l_data_typed_range const & _rdr, t_TyTransportCtxt const & _rcxt )
  {
    _TyStrView sv;
    _rcxt.GetStringView( sv, _rdr );
    _TyEntityMap::const_iterator cit = m_mapNamespaces.find( sv );
    if ( m_mapNamespaces.end() == cit )
    {
      THROWXMLPARSEEXCEPTION("Can't find namespace prefix [%s].", StrConvertString<char>( sv ).c_str() );
    }
    return cit->second;
  }
  // Process any namespace declarations in this token and then process any namespace references.
  void _ProcessNamespaces( _TyLexToken & _rltok, bool _fOnlyTagName )
  {
    if ( _fOnlyTagName )
      return _ProcessTagName( _rltok[0] );

    // There is this annoying bit of verbiage:
    // "The prefix xml is by definition bound to the namespace name http://www.w3.org/XML/1998/namespace. It MAY, but need not, be declared, 
    //    and MUST NOT be bound to any other namespace name. Other prefixes MUST NOT be bound to this namespace name, and it MUST NOT be declared as the default namespace."
    // I don't think I am going to worry about this actually.
    // First thing to do is to move through all attributes and obtain all namespace declarations and at the same time ensure they are valid.
    const _TyChar rgszDefaultNamespaceName[] = str_array_cast< _TyChar >("#");
    typedef AllocaList< _TyStrView > _TyListPrefixes;
    _TyListPrefixes lPrefixesUsed; // Can't declare the same prefix in the same tag.
    _TyLexValue & rrgAttrs = _rltok[1];
    const size_t knAttrs = rrgAttrs.GetSize();
    for ( size_t nAttr = 0; nAttr < knAttrs; ++nAttr )
    {
      _TyLexValue & rrgAttr = rrgAttrs[nAttr];
      _TyStrView svName;
      rrgAttr[0].KGetStringView( _rltok, svName );
      if ( !svName.compare( str_array_cast< _TyChar >("xmlns") ) )
      {
        // We have a namespace declaration!
        _TyStrView svPrefix;
        rrgAttr[1].KGetStringView( _rltok, svPrefix );
        // Check for uniqueness of prefix:
        VerifyThrowSz( !lPrefixesUsed.FFind( svPrefix ), "Namespaces Validity: Cannot use same namespace prefix more than once within the same tag." );
        ALLOCA_LIST_PUSH( lPrefixesUsed, svPrefix );
        _TyStrView svUri;
        rrgAttr[3].KGetStringView( _rltok, svUri );
        _ProcessTagName( rrgAttr ); // For xmlns attr names we process the same as we process the tag name. We want to leave both "xmlns" and any colon and prefix in the attr name.
        // Add the URI to the UriMap:
        const _TyUriMap::value_type & rvtUri = m_pXp->RStrAddUri( svUri );
        bool fDefault = svPrefix.empty();
        if ( fDefault )
          svPrefix = rgszDefaultNamespaceName;
        // named prefix:
        // If the URI mapped to is empty this violates a "namespace constraint":
        // "Namespace constraint: No Prefix Undeclaring
        //  In a namespace declaration for a prefix (i.e., where the NSAttName is a PrefixedAttName), the attribute value MUST NOT be empty."
        // As such we will constrain regardless if we are validating - because we are using namespaces.
        VerifyThrowSz( fDefault || !svUri.empty(), "Empty URI given for namespace prefix. This violates the Namespace constraint: No Prefix Undeclaring." );
        // To use transparent key lookup you must use find - not any of the flavors of insert:
        _TyNamespaceMap::iterator it = m_mapNamespaces.find( svPrefix );
        if ( m_mapNamespaces.end() == it  )
        {
          pair< _TyNamespaceMap::iterator, bool > pib = m_mapNamespaces.emplace( std::piecewise_construct, svPrefix, std::forward_as_tuple() );
          Assert( pib.second );
          it = pib.first;
        }
        it->second.push( &rvtUri );
        // Now we want to change the value type in the 1th position to xml_namespace_value_wrap. This causes appropriate removal of this namespace when this tag is destroyed.
        rrgAttr[1].template emplaceArgs< _TyXmlNamespaceValueWrap >( *it, &m_mapNamespaces );
      }
    }
    // Now parse the namespace on the tag:
    {//B
      _TyLexValue & rrgTagName = _rltok[0];
      _TyData const & krdtName = rrgTagName[1].GetVal< _TyData >();
      if ( !krdtName.FIsNull() )
      {
        _TyData & rdtPrefix = rrgTagName[0].GetVal< _TyData >();
        _TyStrView svPrefix;
        rrgTagName[0].KGetStringView( _rltok, svPrefix );
        _TyNamespaceMap::iterator it = m_mapNamespaces.find( svPrefix );
        VerifyThrowSz( m_mapNamespaces.end() != it, "Namespace prefix [%s] not found.", StrConvertString<char>( svPrefix ).c_str() );
        // Update the tag name so that it includes the prefix as is necessary for XML end tag matching.
        rdtPrefix.DataRangeGetSingle().m_posEnd = krdtName.DataRangeGetSingle().m_posEnd;
        // Setup the xml_namespace_value_wrap in the 1th position as is standard:
        rrgTagName[1].template emplaceArgs< _TyXmlNamespaceValueWrap >( *it, nullptr );
      }
      else
      {
        // If we have a default namespace then we need to apply it to the tag.
        _TyNamespaceMap::iterator it = m_mapNamespaces.find( rgszDefaultNamespaceName );
        if ( ( m_mapNamespaces.end() == it ) || it->second.front().RStrUri().empty() )
        {
          // If there is no namespace at all then we just put a "false" in the 1th position:
          rrgTagName[1].emplaceVal( false );
        }
        else
        {
          // Setup the xml_namespace_value_wrap in the 1th position as is standard:
          rrgTagName[1].template emplaceArgs< _TyXmlNamespaceValueWrap >( *it, nullptr );
        }
      }
    }//EB
    // Now process each attribute name, looking up prefixes wherever any prefixes exist.
    for ( size_t nAttr = 0; nAttr < knAttrs; ++nAttr )
    {
      _TyLexValue & rrgAttr = rrgAttrs[nAttr];
      if ( rrgAttr[1].FIsA< _TyXmlNamespaceValueWrap >() )
        continue; // This is an xmlns attribute that was processed above.
      if ( rrgAttr[1].FEmptyTypedData() )
      {
        // No default namespaces for attribute names - this has no namespace:
        rrgAttr[1].emplaceVal( false );
        continue;
      }
      _TyStrView svPrefix;
      rrgAttr[0].KGetStringView( _rltok, svPrefix );
      _TyData const & krdtName = rrgAttr[1].GetVal< _TyData >();
      _TyData & rdtPrefix = rrgAttr[0].GetVal< _TyData >();
      if ( m_fIncludePrefixesInAttrNames && !m_fCheckDuplicateAttrs )
        rdtPrefix.DataRangeGetSingle().m_posEnd = krdtName.DataRangeGetSingle().m_posEnd;
      else
        rdtPrefix.DataRangeGetSingle() = krdtName.DataRangeGetSingle(); // The user just wants the name without the prefix.
      typename _TyNamespaceMap::iterator it = m_mapNamespaces.find( svPrefix );
      VerifyThrowSz( m_mapNamespaces.end() != it, "Prefix [%s] was not found in the namespace map." StrConvertString<char>( svPrefix ).c_str() );
      rrgAttr[1].template emplaceArgs< _TyXmlNamespaceValueWrap >( *it, nullptr );
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
        if ( !rrgAttr[1].FIsA< _TyXmlNamespaceValueWrap >() )
        {
          Assert( rrgAttr[1].FIsA< bool >() && !rrgAttr[1].GetVal< bool >() ); // Not in any namespace at all.
          continue; // This is an xmlns attribute that was processed above.
        }
        _TyXmlNamespaceValueWrap const & rnvw = rrgAttr[1].GetVal< _TyXmlNamespaceValueWrap >();
        if ( rnvw.FIsNamespaceDeclaration() )
          continue;
        _TyData & rdtName = rrgAttr[0].GetVal< _TyData >();
        rdtName.DataRangeGetSingle().m_posBegin -= 1 + rnvw.PVtNamespaceMapValue()->first.length(); // push back to include the prefix.
      }
    }
  }
  }
  // This pass made when namespace are not being validated. Because the productions separate out the namespace prefixes still,
  //  we must combine the name and the prefix and as well indicate that they are in no namespace by setting the 1th element to false.
  void _ProcessNoNamespaces( _TyLexToken & _rltok, bool _fOnlyTagName )
  {
    _ProcessTagName( _rltok[0] );
    if ( _fOnlyTagName )
      return;
    _TyLexValue & rrgAttrs = _rltok[1];
    const size_t knAttrs = rrgAttrs.GetSize();
    for ( size_t nAttr = 0; nAttr < knAttrs; ++nAttr )
    {
      _TyLexValue & rrgAttr = rrgAttrs[nAttr];
        // The name will include the prefix and the colon when we are not namespace aware.
        _ProcessTagName( rrgAttr ); 
        rrgAttr[1].emplaceVal( false ); // This indicates that there is no namespace.
      }
    }
    {//B - indicate there is no namespace on the tag.
      _TyLexValue & rrgTagName = _rltok[0];
      rrgTagName[1].emplaceVal( false );
    }//EB
    if ( m_fCheckDuplicateAttrs )
      _CheckDuplicateAttrs(  _rltok );
  }
  void _CheckDuplicateAttrs( _TyLexToken & _rltok )
  {
    // The default init allocator allows us to resize without initializing the elements...
    // Then we don't have to interact with the vector at all as we fill it...
    typedef std::vector< const _TyLexValue *, default_init_allocator< std::allocator< const _TyLexValue * > > > _TyVectorPtrs;
    _TyVectorPtrs rgPointers;
    static constexpr size_t knptrMaxAllocaSize = vknbyMaxAllocaSize / sizeof( _TyValue* );
    _TyValue & rrgAttrs = _rltok[1];
    size_t nAttrs = rrgAttrs.GetSize();
    const _TyValue ** ppvalBegin, ppvalEnd;
    if ( knptrMaxAllocaSize < nAttrs )
    {
      rgPointers.resize( nAttrs );
      ppvalBegin = rgPointers.begin();
      ppvalEnd = rgPointers.end();
    }
    else
    {
      ppvalBegin = (const _TyValue *)alloca( nAttrs * sizeof( const _TyValue * ) );
      ppvalEnd = ppvalBegin + nAttrs;
    }
    // Fill up the pointers - use ApplyContiguous to do it as quickly as possible.
    {//B
      const _TyValue ** ppvalCur = ppvalBegin;
      rrgAttrs.GetValueArray().ApplyContiguous( 0, nAttrs,
        [&ppvalCur]( const _TyValue * _pvBegin, const _TyValue * _pvEnd )
        {
          for ( ; _pvBegin != _pvEnd; )
            *ppvalCur++ = _pvBegin++;
        }
      );
    }//EB
    Assert( ppvalCur == ppvalEnd );
    auto lambdaCompareAttrPointers =  
      [&_rltok]( const _TyValue *& _rlpt, const _TyValue *& _rrpt ) -> bool
      {
        // First compare the names:
        _TyStrView svNameLeft;
        (*_rlpt)[0].KGetStringView( _rltok, svNameLeft );
        _TyStrView svNameRight;
        (*_rrpt)[0].KGetStringView( _rltok, svNameRight );
        int iComp = svNameLeft.compare( svNameRight );
        if ( !iComp )
        {
          _TyValue const & rlvalNS = (*_rlpt)[1];
          _TyValue const & rrvalNS = (*_rrpt)[1];
          bool fIsNotBoolLeft = !rlvalNS.FIsA< bool >();
          bool fIsNotBoolRight = !rrvalNS.FIsA< bool >();
          iComp = (int)fIsNotBoolLeft - (int)fIsNotBoolRight;
          if ( !iComp && fIsNotBoolLeft )
          {
            const _TyXmlNamespaceValueWrap & rxnvwLeft = rlvalNS.GetVal< _TyXmlNamespaceValueWrap >();
            const _TyXmlNamespaceValueWrap & rxnvwRight = rrvalNS.GetVal< _TyXmlNamespaceValueWrap >();
            iComp = rxnvwLeft.ICompareForUniqueAttr( rxnvwRight );
          }
        }
        return iComp < 0;
      }

    sort( ppvalBegin, ppvalEnd, lambdaCompareAttrPointers );
    // Check for adjacent equal object. We will report all violations, cuz duh...
    bool fAnyDuplicateAttrNames = false;
    for ( const _TyValue ** ppDupeCur = adjacent_find( ppvalBegin, ppvalEnd, lambdaCompareAttrPointers );
          ( ppvalEnd != ppDupeCur );
          ppDupeCur = adjacent_find( ppDupeCur+1, ppvalEnd, lambdaCompareAttrPointers ) )
    {
      // If the same prefix declares the same URI then we will will ignore it because I cannot see an interpretation of this in the W3C XML Namespace Specification.
      // We will throw an error for any duplicate prefix declarations if m_fStrict is on because other parsers may barf on such declarations.
      const _TyValue & rvattrCur = **ppDupeCur;
      const _TyValue & rvnsCur = rvattrCur[1];
      _TyStdStr strAttrName;
      rvattrCur[0].GetString( _rltok, strAttrName );        
      if ( rvnsCur.FIsA<bool>() )
      {
        // no-namespace attribute:
        fAnyDuplicateAttrNames = true;
        LOGSYSLOG( eslmtError, "Duplicate attribute name found [%s].", strAttrName.c_str() );
      }
      else
      {
        Assert( rvnsCur.FIsA<_TyXmlNamespaceValueWrap>() );
        _TyXmlNamespaceValueWrap const & rxnvwCur = rvnsCur.GetVal< _TyXmlNamespaceValueWrap >();
        if ( rxnvwCur.FIsNamespaceDeclaration() )
        {
          Assert( false );  // REVIEW:<dbien>: We are already checking for duplicate namespace prefix declarations in _ProcessNamespaces().
                            // We perform that check regardless if m_fCheckDuplicateAttrs since it would cause issues with interpretation
                            //  of the attributes and it is quick since only several namespaces at most would be declared in a given tag.
                            // I'll leave this code here since it can't really hurt too much and it might be decided to not do the check in _ProcessNamespaces().
          const _TyValue & rvattrNext = *ppDupeCur[1];
          const _TyValue & rvnsNext = rvattrNext[1];
          if ( m_fStrict || !!rxnvwCur.ICompare( rvnsNext.GetVal< _TyXmlNamespaceValueWrap >() ) )
          {
            fAnyDuplicateAttrNames = true;
            LOGSYSLOG( eslmtError, "Duplicate namespace prefix name declared[%s].", strAttrName.c_str() );
          }
        }
        else
        {
          // Attr same name with same URI declared:
          fAnyDuplicateAttrNames = true;
          LOGSYSLOG( eslmtError, "Same named attr[%s] has same URI reference in namespace[%s].", strAttrName.c_str(), rxnvwCur.RStringUri().c_str() );
        }
      }
    }
    VerifyThrowSz( !fAnyDuplicateAttrNames, "Found dupicate (qualified) attribute names or namespace prefix declarations.");
  }
  void _ProcessToken( unique_ptr< _TyLexToken > & _rpltok )
  {
    // 1) If a tag then we need to process namespaces.
    // 2) If CharData or CDATASection then we may need to read a token ahead to see if we need to combine, etc.
    switch( _rpltok->GetTokenId() )
    {
      case s_knTokenXMLDecl:
        // This is only valid at the beginning of the file.
        _ProcessXMLDecl( *_rpltok );
      break;
      case s_knTokenSTag:
      case s_knTokenEmptyElemTag:
      {
        m_fGotFirstTag = true;
        if ( m_fUseXMLNamespaces )
          _ProcessNamespaces( *_rpltok, m_fSkippingTags );
        else
          _ProcessNoNamespaces( *_rpltok, m_fSkippingTags );
      }
      break;
      case s_knTokenETag:
        _ProcessTagName( _rpltok->GetValue() ); // No need to process namespaces on the end tag - just the name itself.
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
  void _ProcessXMLDecl( _TyLexToken & _rltok )
  {
    // Pull the info we want out of it to store in the cursor.
    _TyValue & rrgVals = _rltok.GetValue();
    Assert( rrgVals.FIsArray() );
    m_fStandalone = rrgVals[0].GetVal<bool>();
    rrgVals[2].GetString( _rltok, m_strEncoding );
    _TyStdStr strMinorVNum;
    rrgVals[4].GetString( _rltok, strMinorVNum );
    m_nVersionMinorNumber = strMinorVNum[0] - _TyChar('0');
  }
  void _ProcessComment( _TyLexToken & _rltok )
  {
    _l_data_range drToken;
    _rltok.GetTokenDataRange( drToken );
    drToken.m_posBegin += 4; // <!--
    drToken.m_posEnd -= 3; // -->
    // REVIEW:<dbien>: Could strip whitespace off front and rear but...
    _rltok.template emplace< _TyData >( drToken );
  }
  void _ProcessCharData( )
  void _ProcessComment( _TyLexToken & _rltok )
  {
    _l_data_range drToken;
    _rltok.GetTokenDataRange( drToken );
    drToken.m_posBegin += 4; // <!--
    drToken.m_posEnd -= 3; // -->
    // REVIEW:<dbien>: Could strip whitespace off front and rear but...
    _rltok.template emplace< _TyData >( drToken );
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

  // The prefix xml is by definition bound to the namespace name http://www.w3.org/XML/1998/namespace
  _TyNamespaceMap m_mapNamespaces{ { str_array_cast<_TyChar>("xml"), str_array_cast<_TyChar>("http://www.w3.org/XML/1998/namespace") } };
  // We will hash the default namespace by the invalid prefix "#". This way it essentially works the same as other namespaces.
// State:
  // Before we get the first tag CharData must contain all whitespace - and if not we will throw. Also various other things - DTD declaration, etc.
  bool m_fGotFirstTag{false};
  // The current skip token array and the number of elements in it. This is passed to the lexical analyzer to cut token processing off optimally.
  _TySkipArray m_rgSkipTokensCur{0};
  size_t m_nSkip{0};
  // This is set to true when we are skipping sets of tags due to FNextTag() or FNextToken() being called at a level higher than a leaf tag.
  bool m_fSkippingTags{false}; 
  // This is set to true when one of the "m_fSkip..." settings changes. We will then update m_rgSkipTokensCur and m_nSkip right before checking for more tokens.
  bool m_fUpdateSkip{false};
// Settings:
  // All settings by default should produce the full file if all tokens were copied to the output (invariant).
  // If this is false then we will not check for duplicate attributes.
  bool m_fCheckDuplicateAttrs{true};
  // If this is true then we will process namespaces, otherwise we will still show the prefixes but not apply XML Namespace validation and logic.
  bool m_fUseXMLNamespaces{false};
  // This option allows the user to either see the prefixes that are present in the attr names or see just the names.
  bool m_fIncludePrefixesInAttrNames{false};
  // If m_fStrict then we will not be lenient in cases that may be ambiguous wrt to the XML Specifications.
  bool m_fStrict{false};
  // Don't return tokens for CharData composed entirely of whitespace.
  bool m_fFilterWhitespaceCharData{false};
  // Integrate CDataSections with any adjacent CharData, References, etc.
  bool m_fIntegrateCDataSections{false};
// Skip settings: Ignore the corresponding token types and don't return to the user.
  bool m_fSkipXMLDecl{false};
  bool m_fSkipComments{false};
  bool m_fSkipPI{false};
// XMLDecl properties:
  bool m_fStandalone{false};
  _TyStdStr m_strEncoding;
  uint8_t m_nVersionMinorNumber;
};

__XMLP_END_NAMESPACE
