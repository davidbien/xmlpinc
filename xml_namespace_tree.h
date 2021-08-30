#pragma once

//          Copyright David Lawrence Bien 1997 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt).

// xml_namespace_tree.h
// dbien
// 14APR2021
// To do namespace lookup "correctly" inside of an xml_document or an xml_document_var we need to have a tree of namespace declarations.
// Each xml_tag or xml_tag_var will reference the nearest namespace lookup node above it on the document tree. When a namespace declaration is
//  added to a xml_tag that previously had none then a new xml_namespace_node is added to the tree.

#include "_shwkptr.h"

__XMLP_BEGIN_NAMESPACE

// xml_namespace_tree_node:
// A single node of the lookup tree. Multiple namespaces might be declared in this node.
// All we need is a simple map from prefix->URI - no stack of such mappings since we have a tree of nodes of mappings.

template < class t_TyChar, class t_TyAllocator >
class xml_namespace_tree_node
{
  typedef xml_namespace_tree_node _TyThis;
	typedef allocator_traits< t_TyAllocator > _TyAllocTraitsAsPassed;
public:
  typedef t_TyChar _TyChar;
  typedef t_TyAllocator _TyAllocator;
  typedef basic_string< _TyChar > _TyStdStr;
	using _TyAllocatorTraitsMapValue = typename _TyAllocTraitsAsPassed::template rebind_traits< typename map< _TyStdStr, const _TyStdStr *, std::less<> >::value_type >;
  // We map to the permanent URI in the document context (somewhere).
  typedef map< _TyStdStr, const _TyStdStr *, std::less<>, typename _TyAllocatorTraitsMapValue::allocator_type > _TyMapNamespaces;
  typedef SharedStrongPtr< _TyThis, _TyAllocator, uint32_t, false > _TyStrongThis;
  typedef SharedWeakPtr< _TyThis, _TyAllocator, uint32_t, false > _TyWeakThis;

  ~xml_namespace_tree_node() = default;
  xml_namespace_tree_node( xml_namespace_tree_node const & ) = delete;
  xml_namespace_tree_node( xml_namespace_tree_node && _rr ) noexcept
  {
    swap( _rr ); // swap with null-initialized object.
  }
  xml_namespace_tree_node( const void * _pvOwner, _TyStrongThis * _pspParent )
    : m_pvOwner( _pvOwner )
  {
    if ( _pspParent )
      m_upwpParent = make_unique< _TyWeakThis >( *_pspParent );
  }
  void swap( _TyThis & _r ) noexcept
  {
    m_mapNamespaces.swap( _r.m_mapNamespaces );
    m_upwpParent.swap( _r.m_upwpParent );
    m_rgspChildren.swap( _r.m_rgspChildren );
    std::swap( m_pvOwner, _r.m_pvOwner );
  }
  _TyThis & operator = ( _TyThis const & ) = delete;
  _TyThis & operator = ( _TyThis && _rr ) noexcept
  {
    _TyThis acquire( std::move( _rr ) );
    swap( acquire );
    return *this;
  }
  // Test if the passed pv indicates the owner of this node of the namespace tree.
  bool FIsOwner( const void * _pv ) const noexcept
  {
    return _pv == m_pvOwner;
  }

  // This will move up the tree and search for the nearest parent that contains the (prefix,URI).
  // It will return nullptr if no such definition.
  template < class t_TyStrViewOrString >
  _TyStdStr const * PStrGetUri( t_TyStrViewOrString const & _rsvPrefix, bool _fCheckParent ) const
  {
    typename _TyMapNamespaces::const_iterator cit = m_mapNamespaces.find( _rsvPrefix );
    if ( m_mapNamespaces.end() == cit )
    {
      if ( _fCheckParent && m_upwpParent && !m_upwpParent->expired() )
      {
        // Then we should have an active parent - i.e. it should be present and so we will throw if we can't get a strong pointer:
        _TyStrongThis spParent( *m_upwpParent );
        return spParent->PStrGetUri( _rsvPrefix, true ); // keep looking.
      }
      return nullptr;
    }
    Assert( cit->second );
    return cit->second;
  }
  void CopyNamespaces( _TyMapNamespaces const & _rmapCopy )
  {
    m_mapNamespaces = _rmapCopy;
  }
  void AddChildNamespaceNode( _TyStrongThis const & _rspChild )
  {
    m_rgspChildren.push_back( _rspChild );
  }
protected:
  _TyMapNamespaces m_mapNamespaces;
  typedef unique_ptr< _TyWeakThis > _TyUPtrWeakThis;
  _TyUPtrWeakThis m_upwpParent; // We have a weak pointer to our parent xml_namespace_tree_node.
	using _TyAllocatorTraitsGcoThis = typename _TyAllocTraitsAsPassed::template rebind_traits< _TyStrongThis >;
  typedef std::vector< _TyStrongThis, typename _TyAllocatorTraitsGcoThis::allocator_type > _TyRgSpChildren;
  _TyRgSpChildren m_rgspChildren;
  // This is the tag that owns this xml_namespace_tree_node - i.e. this node represents the namespace declarations at that tag.
  // This is to distinguish that fact from sub-tags who may share a reference to some parent's namespace declarations.
  const void * m_pvOwner{ nullptr }; 
};

__XMLP_END_NAMESPACE
