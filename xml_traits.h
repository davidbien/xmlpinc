#pragma once

// xml_traits.h
// XML traits determine which features: namespaces, DTD, validation, etc. that the XML parser supports.
// dbien
// 09JAN2021

#include <string>
#include <string_view>
#include <unordered_map>
#include "_strutil.h"
#include "xml_ns.h"
#include "xml_types.h"
#include "xml_namespace.h"

__XMLP_BEGIN_NAMESPACE

template < class t_TyTransport, bool t_kfSupportNamespaces, bool t_kfSupportDTD, bool t_kfValidating >
struct xml_traits
{
private:
  typedef xml_traits _TyThis;
public:
  typedef t_TyTransport _TyTransport;
  typedef typename t_TyTransport::_TyChar _TyChar;
  typedef xml_user_obj< _TyThis > _TyLexUserObj;
  typedef basic_string< _TyChar > _TyStdStr;
  typedef basic_string_view< _TyChar > _TyStrView;

  static constexpr bool s_kfSupportNamespaces = t_kfSupportNamespaces;
  static constexpr bool s_kfSupportDTD = t_kfSupportDTD;
  static constexpr bool s_kfValidating = t_kfValidating;

// Centralize the declaration of some types - which also allows us to form the types we want for
// extension of the _l_value<>'s variant with our desired implementation types.

// namespace hash:
  typedef _xml_namespace_uri< _TyChar > _TyNamespaceUri;
  typedef unique_ptr< _TyNamespaceUri > _TyPtrNamespaceUri; // always map a ptr since then we can always refer to it without it going away.
  typedef StringTransparentHash< _TyChar > _TyStringTransparentHash; // Allow lookup by string_view without creating a string.
  typedef unordered_map< _TyStdStr, _TyPtrNamespaceUri, _TyStringTransparentHash, std::equal_to<void> > _TyNamespaceMap;

// entity hash and parameter entity hash:
  typedef unordered_map< _TyStdStr, _TyStdStr, _TyStringTransparentHash, std::equal_to<void> > _TyEntityMap;

// Declare our lexical analyzer traits:
  typedef xml_namespace_value_wrap< _TyNamespaceMap > _TyXmlNamespaceValueWrap;
  typedef _l_value_traits< _TyXmlNamespaceValueWrap > _TyValueTraitsPack; // We are extending the types in _l_value by this one type currently.
  typedef _l_traits< _TyTransport, _TyLexUserObj, _TyValueTraitsPack > _TyLexTraits;
};

__XMLP_END_NAMESPACE