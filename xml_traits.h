#pragma once

// xml_traits.h
// XML traits determine which features: namespaces, DTD, validation, etc. that the XML parser supports.
// dbien
// 09JAN2021

#include <string>
#include <string_view>
#include <unordered_map>
#include "_strutil.h"
#include "upslist.h"
#include "_l_traits.h"
#include "_l_transport.h"
#include "xml_ns.h"
#include "xml_types.h"
#include "xml_namespace.h"
#include "xml_user.h"

__XMLP_BEGIN_NAMESPACE

template < class t_TyTransport, bool t_kfSupportDTD, bool t_kfValidating >
struct xml_traits
{
private:
  typedef xml_traits _TyThis;
public:
  typedef t_TyTransport _TyTransport;
  typedef typename t_TyTransport::_TyChar _TyChar;
  typedef basic_string< _TyChar > _TyStdStr;
  typedef basic_string_view< _TyChar > _TyStrView;

  static constexpr bool s_kfSupportDTD = t_kfSupportDTD;
  static constexpr bool s_kfValidating = t_kfValidating;

// Centralize the declaration of some types - which also allows us to form the types we want for
// extension of the _l_value<>'s variant with our desired implementation types.
  typedef StringTransparentHash< _TyChar > _TyStringTransparentHash; // Allow lookup by string_view without creating a string.

  // entity hash and parameter entity hash:
  typedef unordered_map< _TyStdStr, _TyStdStr, _TyStringTransparentHash, std::equal_to<void> > _TyEntityMap;
  typedef xml_user_obj< _TyEntityMap, s_kfSupportDTD > _TyLexUserObj;

  // URI hash:
  // We hash every URI associated with every prefix and use the value_type from the hash as the key for that URI. It much less unwieldy and allows constant time comparisons, etc.
  // All URIs are active for the lifetime of the XML parser - i.e. none are removed from this map.
  // This also allows xml_tokens to be copied and squirreled away. (Processing occurs on copy.) As long as the xml_parser object is around the xml_tokens will be valid for use.
  typedef unordered_set< _TyStdStr, _TyStringTransparentHash, std::equal_to<void> > _TyUriAndPrefixMap;

  // namespace hash:
  typedef _xml_namespace_uri< _TyUriAndPrefixMap > _TyNamespaceUri;
  typedef UniquePtrSList< _TyNamespaceUri > _TyUpListNamespaceUris;
  // first is a pointer to a PrefixMap value for the prefix, second is the current list of namespace URIs for that prefix.
  typedef pair< const typename _TyUriAndPrefixMap::value_type *, _TyUpListNamespaceUris > _TyNamespaceMapped;
  typedef unordered_map< _TyStdStr, _TyNamespaceMapped, _TyStringTransparentHash, std::equal_to<void> > _TyNamespaceMap;

// Declare our lexical analyzer traits:
  typedef xml_namespace_value_wrap< _TyNamespaceMap, _TyUriAndPrefixMap > _TyXmlNamespaceValueWrap;
  typedef _l_value_traits< _TyXmlNamespaceValueWrap > _TyValueTraitsPack; // We are extending the types in _l_value by this one type currently.
  typedef _l_traits< _TyTransport, _TyLexUserObj, _TyValueTraitsPack > _TyLexTraits;
};

__XMLP_END_NAMESPACE