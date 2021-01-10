#pragma once

// xml_traits.h
// XML traits determine which features: namespaces, DTD, validation, etc. that the XML parser supports.
// dbien
// 09JAN2021

#include "xml_ns.h"

__XMLP_BEGIN_NAMESPACE

template < class t_TyChar, bool t_kfSupportNamespaces, bool t_kfSupportDTD, bool t_kfValidating >
struct xml_traits
{
  typedef t_TyChar _TyChar;
  static constexpr bool s_kfSupportNamespaces = t_kfSupportNamespaces;
  static constexpr bool s_kfSupportDTD = t_kfSupportDTD;
  static constexpr bool s_kfValidating = t_kfValidating;
};

__XMLP_END_NAMESPACE