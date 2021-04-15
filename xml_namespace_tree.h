#pragma once

// xml_namespace_tree.h
// dbien
// 14APR2021
// To do namespace lookup "correctly" inside of an xml_document or an xml_document_var we need to have a tree of namespace declarations.
// Each xml_tag or xml_tag_var will reference the nearest namespace lookup node above it on the document tree. When a namespace declaration is
//  added to a xml_tag that previously had none then a new xml_namespace_node is added to the tree.

__XMLP_BEGIN_NAMESPACE

// xml_namespace_map_node:
// A single node of the lookup tree. Multiple namespaces might be declared in this node.
// All we need is a simple map from prefix->URI - no stack of such mappings since we have a tree of nodes of mappings.

template < class t_TyChar, class t_TyAllocator = allocator< t_TyChar > >
class xml_namespace_map_node
{
  typedef xml_namespace_map_node _TyThis;
	typedef allocator_traits< t_TyAllocator > _TyAllocTraitsAsPassed;
public:
  typedef t_TyChar _TyChar;
  typedef t_TyAllocator _TyAllocator;
  typedef basic_string< _TyChar > _TyStdStr;
	using _TyAllocatorTraitsMapValue = typename _TyAllocTraitsAsPassed::template rebind_traits< typename map< _TyStdStr, const _TyStdStr *, std::less<> >::value_type >;
  // We map to the permanent URI in the socument context (somewhere).
  typedef map< _TyStdStr, const _TyStdStr *, std::less<>, typename _TyAllocatorTraitsMapValue::allocator_type > _TyMapNamespaces;

  
protected:
  _TyMapNamespaces m_mapNamespaces;
  _TyThis * m_pParent{nullptr}; // A non-reference pointer to our parent, it has a reference pointer to us.
  typedef _gco< _TyThis, _TyAllocator > _TyGcoThis;
  typedef _gcp< _TyThis, _TyGcoThis > _TyGcpThis;
	using _TyAllocatorTraitsGcoThis = typename _TyAllocTraitsAsPassed::template rebind_traits< _TyGcpThis >;
  typedef std::vector< _TyGcpThis, _TyAllocatorTraitsGcoThis::allocator_type > _TyRgGcpChildren;
  _TyRgGcpChildren m_rggcpChildren;
};

__XMLP_END_NAMESPACE