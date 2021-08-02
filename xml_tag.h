#pragma once

// xml_tag.h
// Simple XML object model based on the XML parser.
// dbien
// 23JAN2021

// The root tag is always an XMLDecl "tag" - even if there was no XMLDecl declaration in the XML file.
// Then, as with all tags, there may be some content nodes, then the main document tag, and then subtags.
// Trouble must be taken to ensure that only a single document tag is ever within the XMLDecl "tag" since
//  all other tags have freeform contents in that regard.
#include "_shwkptr.h"
#include "xml_types.h"
#include "xml_namespace_tree.h"

__XMLP_BEGIN_NAMESPACE

// _xml_tag_import_options:
// Options for importing from an xml_read_cursor into the DOM.
template < class t_TyXmlTraits >
class _xml_tag_import_options
{
  typedef _xml_tag_import_options _TyThis;
public:
  _xml_tag_import_options()
  {
  }

  bool m_fFilterRedundantNamespaceDecls{false}; // This will remove redundant namespace declarations while importing into the DOM.
};

// Context for reading from an XML Stream. This keeps track of the current mapping of namespaces
//  to allow appropriate creation of namespace declarations when none are present for a given
//  (prefix,URI). Also stores any options associated with the conversion of the stream into the document
//  object model.
template < class t_TyXmlTraits >
class _xml_tag_import_context
{
  typedef _xml_tag_import_context _TyThis;
public:
  typedef t_TyXmlTraits _TyXmlTraits;
  typedef typename _TyXmlTraits::_TyChar _TyChar;
  typedef typename _TyXmlTraits::_TyStdStr _TyStdStr;
  typedef _xml_tag_import_options< _TyXmlTraits > _TyXmlTagImportOptions;
  typedef _xml_document_context_transport< _TyXmlTraits > _TyXmlDocumentContext;

  // We map to the permanent URI in the document context (somewhere).
  typedef typename xml_namespace_tree_node< _TyChar >::_TyMapNamespaces _TyMapNamespaces;

  _xml_tag_import_context( _xml_tag_import_context const & ) = delete;
  _xml_tag_import_context( _TyXmlDocumentContext & _rxdcDocumentContext )
    : m_rxdcDocumentContext( _rxdcDocumentContext )
  {
  }

  void SetImportOptions( _TyXmlTagImportOptions const & _rxtio )
  {
    m_xtioImportOptions = _rxtio;
  }

  template < class t_TyStrViewOrString >
  const typename _TyMapNamespaces::value_type * PVtFindPrefix( t_TyStrViewOrString const & _rsvPrefix ) const
  {
    typename _TyMapNamespaces::const_iterator cit = m_mapNamespacesDirect.find( _rsvPrefix );
    if ( m_mapNamespacesDirect.end() == cit )
      return nullptr;
    return &*cit;
  }
  template < class t_TyStrViewOrString >
  typename _TyMapNamespaces::value_type * PVtFindPrefix( t_TyStrViewOrString const & _rsvPrefix )
  {
    typename _TyMapNamespaces::iterator it = m_mapNamespacesDirect.find( _rsvPrefix );
    if ( m_mapNamespacesDirect.end() == it )
      return nullptr;
    return &*it;
  }

  // A reference to the containing document context.
  _TyXmlDocumentContext & m_rxdcDocumentContext;

  // This contains the current *direct* lookup from prefix->URI that models the state of the entire namespace tree at this point. 
  _TyMapNamespaces m_mapNamespacesDirect;

  // We store the current namespace node for our 
  typedef typename xml_namespace_tree_node< _TyChar >::_TyStrongThis _TyStrongNamespaceTreeNode;
  _TyStrongNamespaceTreeNode m_spCurNamespaceNode;

  _TyXmlTagImportOptions m_xtioImportOptions;
};

// _xml_local_import_context:
// This is stored on the stack in each xml_tag<>::FromXmlStream() context.
// It contains the set of prefixes that we added this time paired with their previous URI values.
// This allows us to have a single map lookup for all active prefixes at all points in time - one that
//  mirrors the lookup represented in the tree but will lookup (prefix,uri) faster.
template < class t_TyXmlTraits >
class _xml_local_import_context
{
  typedef _xml_local_import_context _TyThis;
public:
  typedef t_TyXmlTraits _TyXmlTraits;
  typedef typename _TyXmlTraits::_TyChar _TyChar;
  typedef typename _TyXmlTraits::_TyStdStr _TyStdStr;
  typedef std::pair< const _TyStdStr *, const _TyStdStr * > _TyPrPStr;
  typedef vector< _TyPrPStr > _TyRgPrPStr;

  typedef _xml_tag_import_context< _TyXmlTraits > _TyXmlTagImportContext;
  typedef typename _TyXmlTagImportContext::_TyMapNamespaces _TyMapNamespaces;
  typedef typename xml_namespace_tree_node< _TyChar >::_TyStrongThis _TyStrongNamespaceTreeNode;

  _xml_local_import_context( _TyXmlTagImportContext & _rctx )
    : m_rctx( _rctx )
  {
  }
  ~_xml_local_import_context()
  {
    typename _TyRgPrPStr::const_iterator citRestore = m_rgprpstrOldMappings.begin();
    typename _TyRgPrPStr::const_iterator citRestoreEnd = m_rgprpstrOldMappings.end();
    for ( ; citRestoreEnd != citRestore; ++citRestore )
    {
      if ( !citRestore->second )
      {
        size_t nErased = m_rctx.m_mapNamespacesDirect.erase( *citRestore->first );
        Assert( 1 == nErased ); // Should always be there.
      }
      else
      {
          typedef typename _TyXmlTagImportContext::_TyMapNamespaces _TyMapNamespaces;
          typename _TyMapNamespaces::iterator itFound = m_rctx.m_mapNamespacesDirect.find( *citRestore->first );
          Assert( m_rctx.m_mapNamespacesDirect.end() != itFound );
          if ( m_rctx.m_mapNamespacesDirect.end() != itFound )
            itFound->second = citRestore->second; // restore it to previous value.
      }
    }
    if ( m_spRestoreNamespaceNode )
      m_rctx.m_spCurNamespaceNode = m_spRestoreNamespaceNode;
  }

  void RegisterPrefixUri( const _TyStdStr & _rstrPrefix, const _TyStdStr & _rstrUri )
  {
    // Lookup the current mapping. If the mapping is different then update the internal structures appropriately and return true.
    // Otherwise return false and don't update anything at all - the mapping is the same.
    typename _TyMapNamespaces::value_type * pvt = m_rctx.PVtFindPrefix( _rstrPrefix );
    if ( pvt && pvt->second && ( *pvt->second == _rstrUri ) )
    {
      Assert( pvt->second == &_rstrUri ); // this should be the case and we want to make it the case cuz then we can compare pointers.
      return; // already map to the same URI for this prefix.
    }
    // New (prefix,uri) pair.
    // Record it so that the old value can be restored:
    m_rgprpstrOldMappings.push_back( _TyPrPStr( &_rstrPrefix, pvt ? pvt->second : nullptr ) );
    if ( !pvt )
    {
      // Must insert:
      m_rctx.m_mapNamespacesDirect.insert( typename _TyMapNamespaces::value_type( _rstrPrefix, &_rstrUri ) );
    }
    else
      pvt->second = &_rstrUri; // update existing entry.
    
    // Update the set of locally declared (prefix,uri) - these will be swapped in to the namespace tree before we move down.
    pair< typename _TyMapNamespaces::iterator, bool > pib = m_mapNamespacesCurrentTag.insert( typename _TyMapNamespaces::value_type( _rstrPrefix, &_rstrUri ) );
    Assert( pib.second ); // No need for a throw here since if the cursor is working correctly then we would have already failed there.
  }
  // If we have mappings for this current tag then create a new namespace tree node, etc.
  void CreateNamespaceTreeNode( const void * _pvOwner )
  {
    if ( m_mapNamespacesCurrentTag.size() )
    {
      m_spRestoreNamespaceNode = m_rctx.m_spCurNamespaceNode;
      m_rctx.m_spCurNamespaceNode.emplace( std::in_place_t(), _pvOwner, &m_spRestoreNamespaceNode );
      m_rctx.m_spCurNamespaceNode->CopyNamespaces( m_mapNamespacesCurrentTag ); // We want to keep a copy for bookkeeping.
      m_spRestoreNamespaceNode->AddChildNamespaceNode( m_rctx.m_spCurNamespaceNode ); // add on to the end of the children.
    }
  }

  _TyXmlTagImportContext & m_rctx;
  // This contains the current lookup for the current tag only which we will update as we find mappings passed to 
  _TyMapNamespaces m_mapNamespacesCurrentTag;
  // This contains the old mappings for each (prefix,uri) so they can be restored.
  _TyRgPrPStr m_rgprpstrOldMappings;
  // This contains the old namespace tree node to restore.
  _TyStrongNamespaceTreeNode m_spRestoreNamespaceNode;
};

template < class t_TyXmlTraits >
class xml_tag
{
  typedef xml_tag _TyThis;
  friend xml_document< t_TyXmlTraits >;
public:
  typedef t_TyXmlTraits _TyXmlTraits;
  typedef typename _TyXmlTraits::_TyChar _TyChar;
  // The content of a tag is a series of tokens and tags.
  typedef typename _TyXmlTraits::_TyXmlToken _TyXmlToken;
  typedef _xml_read_context< _TyXmlTraits > _TyReadContext;
  typedef xml_read_cursor< _TyXmlTraits > _TyReadCursor;
  typedef xml_namespace_value_wrap< _TyChar > _TyXmlNamespaceValueWrap;
  // We may contain tokens which are not shared objects, or tags which are shared for the purpose of
  //  using weak pointer to point to the parent xml_tag. It should allow subtrees to be stored safely
  //  outside of management by the xml_document container. Because we store a pointer to our parent
  //  xml_tag we can't share the same xml_tag in multiple place in an XML document - as might be considered
  //  to be nice.
  // We store pointers to xml_tag object since we may be pointing to an xml_document object (as a parent weak pointer).
  typedef unique_ptr< _TyThis > _TyUPtrThis;
  typedef SharedStrongPtr< _TyUPtrThis, allocator< _TyUPtrThis >, uint32_t, false > _TyStrongThis;
  typedef SharedWeakPtr< _TyUPtrThis, allocator< _TyUPtrThis >, uint32_t, false > _TyWeakThis;
  typedef std::variant< _TyStrongThis, _TyXmlToken > _TyVariant;
  typedef std::vector< _TyVariant > _TyRgTokens;
  typedef _xml_document_context_transport< _TyXmlTraits > _TyXmlDocumentContext;

  // If we contain namespaces then we will have an associated namespace tree.
  typedef xml_namespace_tree_node< _TyChar > _TyXmlNamespaceTreeNode;
  typedef typename _TyXmlNamespaceTreeNode::_TyStrongThis _TyStrongNamespaceTreeNode;

  // When importing via xml_read_cursor we need to maintain a context in order to transmit options
  //  and contain context used in construction of the namespace tree.
  typedef _xml_tag_import_context< _TyXmlTraits > _TyXmlTagImportContext;
  typedef typename _TyXmlTagImportContext::_TyXmlTagImportOptions _TyXmlTagImportOptions;
  typedef _xml_local_import_context< _TyXmlTraits > _TyXmlLocalImportContext;

  virtual ~xml_tag() = default;
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
      if ( holds_alternative< _TyXmlToken >( rvCur ) )
        std::get< _TyXmlToken >( rvCur ).AssertValid();
      else
        (*std::get< _TyStrongThis >( rvCur ))->AssertValid( this );
    }
#endif //ASSERTSENABLED
  }
  virtual bool FEmpty() const
  {
    return !m_opttokTag.has_value() && !m_rgTokens.size() && !m_wpParent;
  }
  void AcquireTag( _TyXmlToken && _rrtok )
  {
    m_opttokTag = std::move( _rrtok );
    m_opttokTag->AssertValid();
  }
  // Read from this read cursor into this object.
  void FromXmlStream( _TyReadCursor & _rxrc, _TyStrongThis const & _rspThis, _TyXmlTagImportContext & _rxtix )
  {
    Assert( _rxrc.FInsideDocumentTag() );
    VerifyThrowSz( _rxrc.FInsideDocumentTag(), "Read cursor is in an invalid state." );

    _TyXmlLocalImportContext xlic( _rxtix );

    Assert( !m_spNamespaceTreeNode );
    if ( _rxtix.m_spCurNamespaceNode )
    {
      typedef typename _TyXmlToken::_TyLexValue _TyLexValue;
      // Then we are processing namespaces and must do so on the current tag before continuing with subtags.
      // The tag remains in the xml_read_cursor necessarily since it must be there to accomodate the read cursor's
      //  namespace system. However we must create and populate all eventual namespace declaraions at this tag now
      //  as we will need these to evaluate the same below in the tree as we move down.
      _TyXmlToken & tokTagCur = _rxrc.GetTagCur();
      Assert( ( tokTagCur.GetTokenId() == s_knTokenSTag ) || ( tokTagCur.GetTokenId() == s_knTokenEmptyElemTag ) );
      typename _TyLexValue::_TySegArrayValues & rsaAttrs = tokTagCur[vknAttributesIdx].GetValueArray();
      (void)rsaAttrs.NApplyContiguous( 0, rsaAttrs.NElements(), 
        [&xlic]( _TyLexValue * _pattrBegin, _TyLexValue * _pattrEnd ) -> size_t
        {
          _TyLexValue * pattrCur = _pattrBegin;
          for ( ; _pattrEnd != pattrCur; ++pattrCur )
          {
            _TyLexValue & rvNamespace = (*pattrCur)[vknNamespaceIdx];
            if ( rvNamespace.template FIsA<_TyXmlNamespaceValueWrap>() )
            {
              _TyXmlNamespaceValueWrap & rxnvw = rvNamespace.template GetVal< _TyXmlNamespaceValueWrap >();
              xlic.RegisterPrefixUri( rxnvw.RStringPrefix(), rxnvw.RStringUri() );
            }
          }
          return ( _pattrEnd - _pattrBegin );
        }
      );
      // Now check if we need any local namespaces declared or need to be declared. If so we create a node in the namespace tree,
      //  and copy this set of namespace to it. We keep the local copy so that when we are done with our subtags we can lookup
      //  namespace decls and determine if any decls need to be added - and which are redundant and might be removed.
      xlic.CreateNamespaceTreeNode( this );
      
      // Set the node for the current tag to the current namespace node - which may be the same as the parent or unique to this.
      m_spNamespaceTreeNode = _rxtix.m_spCurNamespaceNode;
    }
   
   // Aquire any leading content:
    _AcquireContent( _rxrc.GetContextCur() );

    if ( _rxrc.FMoveDown() )
    {
      bool fNextTag;
      do
      {
        _TyStrongThis spXmlTag;
        {//B
          _TyUPtrThis ptrXmlTag = make_unique< _TyThis >( &_rspThis );
          spXmlTag.emplace( std::in_place_t(), std::move( ptrXmlTag ) );
        }//EB
        (*spXmlTag)->FromXmlStream( _rxrc, spXmlTag, _rxtix );
        fNextTag = _rxrc.FNextTag( &(*spXmlTag)->m_opttokTag, false ); // we have already processed the namespaces.
        _AcquireContent( std::move( spXmlTag ) );
        if ( !fNextTag )
        {
          _AcquireContent( _rxrc.GetContextCur() );
          fNextTag = _rxrc.FMoveDown();
        }
      }
      while ( fNextTag );
    }

    // We still don't have the tag token because it's our parent's call to FNextTag that populates it.
    // We have updated our namespace tree with declarations required in this tag but now we might need to
    //  actually create those declarations onto the end of the tag that we don't own yet...
    // If we are ridding redundant namespace declarations then we also might have to delete some active declarations.
    // It's ok - we are allowed to modify the token at this point because we have already processed all subtags of this tag
    //  and this means the xml_read_cursor's namespace context is current, so when we remove namespace declarations it won't 
    //  matter - in fact they are about to be closed by the parent inside of FNextTag().
    if ( _rxtix.m_spCurNamespaceNode )
    {
      _TyXmlToken & tokTagCur = _rxrc.GetTagCur();
      typedef typename _TyXmlToken::_TyLexValue _TyLexValue;
      typename _TyLexValue::_TySegArrayValues & rsaAttrs = tokTagCur[vknAttributesIdx].GetValueArray();
      // If we are to prune redundant namespace declarations then get a bitmask for 
      typedef _simple_bitvec< size_t > _TyBV;
      _TyBV bvDeletions;
      if ( _rxtix.m_xtioImportOptions.m_fFilterRedundantNamespaceDecls )
        bvDeletions = _TyBV( rsaAttrs.NElements() );
      size_t nAttr = 0;
      (void)rsaAttrs.NApplyContiguous( 0, rsaAttrs.NElements(), 
        [&nAttr,&xlic,&_rxtix,&bvDeletions]( _TyLexValue * _pattrBegin, _TyLexValue * _pattrEnd ) -> size_t
        {
          _TyLexValue * pattrCur = _pattrBegin;
          for ( ; _pattrEnd != pattrCur; ++pattrCur )
          {
            _TyLexValue & rvNamespace = (*pattrCur)[vknNamespaceIdx];
            if ( rvNamespace.template FIsA<_TyXmlNamespaceValueWrap>() )
            {
              _TyXmlNamespaceValueWrap & rxnvw = rvNamespace.template GetVal< _TyXmlNamespaceValueWrap >();
              if ( rxnvw.FIsAttributeNamespaceDeclaration() )
              {
                rxnvw.ResetNamespaceDecls(); // The xml_read_cursor also does this but we do it now to save time and then we pass "false" to FNextTag() to stop the read cursor from also doing it.
                typename _TyXmlLocalImportContext::_TyMapNamespaces::const_iterator citFound = xlic.m_mapNamespacesCurrentTag.find( rxnvw.RStringPrefix() );
                if ( xlic.m_mapNamespacesCurrentTag.end() == citFound )
                {
                  // redundant namespace declaration - the (prefix,uri) was already populated.
                  if ( _rxtix.m_xtioImportOptions.m_fFilterRedundantNamespaceDecls )
                    bvDeletions.setbit( nAttr );
                }
                else
                {
                  // Then we found an existing declaration for a required namespace.
                  Assert( citFound->second == &rxnvw.RStringUri() );
                  xlic.m_mapNamespacesCurrentTag.erase( citFound ); // erase it so we know the set of declarations that need to be added.
                }
              }
            }
          }
          ++nAttr; // Since we have a segmented array we can't look up our element number and must maintain it.
          return ( _pattrEnd - _pattrBegin );
        }
      );
      // Remove any namespace decls to be deleted:
      rsaAttrs.RemoveBvElements( bvDeletions );
      // The (prefix,uri) remaining in the xlic.m_mapNamespacesCurrentTag need have namespace declarations added to this tag.
      for ( typename _TyXmlLocalImportContext::_TyMapNamespaces::const_iterator citAddNamespace = xlic.m_mapNamespacesCurrentTag.begin();
            citAddNamespace != xlic.m_mapNamespacesCurrentTag.end();
            ++citAddNamespace )
      {
        // We must create the XmlNamespaceValueWrap using the xml_read_cursor so that we share the (prefix,uri) string values from their hash tables.
        _TyXmlNamespaceValueWrap xnvwAttrDecl( _rxrc.GetNamespaceValueWrap( enrtAttrNamespaceDeclReference, citAddNamespace->first, *citAddNamespace->second ) );
        tokTagCur.DeclareNamespace( _rxtix.m_rxdcDocumentContext.GetBaseContext(), std::move( xnvwAttrDecl ) );
      }
    }
  }
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
  _TyStrongNamespaceTreeNode m_spNamespaceTreeNode; // The nearest namespace declaration above us.
};

// xml_document:
// This contains the root xml_tag as well as the namespace URI and Prefix maps and the user object.
template < class t_TyXmlTraits >
class xml_document : public xml_tag< t_TyXmlTraits >
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
  using typename _TyBase::_TyXmlNamespaceTreeNode;
  using typename _TyBase::_TyStrongNamespaceTreeNode;
  using typename _TyBase::_TyXmlTagImportOptions;
  using typename _TyBase::_TyXmlTagImportContext;
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
  // PrCreateXmlDocument: Returns a created XML document.
  static pair< _TyThis *, _TyStrongThis > PrCreateXmlDocument()
  {
    unique_ptr< _TyThis > upXmlDoc = make_unique< _TyThis >();
    _TyThis * pXmlDoc = upXmlDoc.get();
    return pair< _TyThis *, _TyStrongThis >( pXmlDoc, _TyStrongThis( std::in_place_t(), std::move( upXmlDoc ) ) );
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
  bool FEmpty() const override
  {
    return _TyBase::FEmpty() && m_xdcxtDocumentContext.FEmpty();
  }
  // Add a virtual to return the XMLDocument
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
  // _rspThis contains the strong pointer for this object. This is required to correctly populate the parent pointers.
  void FromXmlStream( _TyReadCursor & _rxrc, _TyStrongThis const & _rspThis, _TyXmlTagImportOptions * _pxtio = nullptr )
  {
    // Sync up any options before we start:
    m_xdcxtDocumentContext.SetIncludePrefixesInAttrNames( _rxrc.GetIncludePrefixesInAttrNames() );
  
    // We can only read into an empty document. We can read into tags that aren't empty but the top node.
    //  in an XML document is the single XMLDecl node.
    VerifyThrowSz( FEmpty(), "Can't read into an non-empty document." );  
    Assert( _rxrc.FInProlog() || _rxrc.FInsideDocumentTag() );
    VerifyThrowSz( _rxrc.FInProlog() || _rxrc.FInsideDocumentTag(), "Read cursor is in an invalid state." );
    // Read each element in order. When done transfer the user object, UriMap and PrefixMap over to this object.
    // We can attach at any point during the iteration and we will glean the XMLDecl top node from the read cursor.

    _TyXmlTagImportContext xticCurContext( m_xdcxtDocumentContext );
    if ( _pxtio )
      xticCurContext.SetImportOptions( *_pxtio );

    // If the cursor is parsing XML namespaces then we will create the root node in the namespace tree.
    // The root of DOM is the XMLDecl node which cannot have namespace declarations on it so the root node will
    //  always be empty. There will only be any children (and there can only be one child of the root) if and
    //  when there are namespace declarations.
    if ( _rxrc.FUseXMLNamespaces() )
    {
      // The namespace tree node takes an "owner" void* to allow detection of ownership by a tag.
      xticCurContext.m_spCurNamespaceNode.emplace( std::in_place_t(), this, nullptr );
    }

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
        spXmlDocument.emplace( std::in_place_t(), std::move( ptrXmlTag ) );
      }//EB
      (*spXmlDocument)->FromXmlStream( _rxrc, spXmlDocument, xticCurContext );
      bool fNextTag = _rxrc.FNextTag( &(*spXmlDocument)->m_opttokTag, false );
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
