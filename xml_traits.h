#pragma once

//          Copyright David Lawrence Bien 1997 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt).

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
#include "xml_attr_default.h"
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
  typedef typename t_TyTransport::_TyTransportCtxt _TyTransportCtxt;
  typedef basic_string< _TyChar > _TyStdStr;
  typedef basic_string_view< _TyChar > _TyStrView;

  static constexpr bool s_kfSupportDTD = t_kfSupportDTD;
  // The document is validated against its DTD as it is read: see xml_read_cursor<>::FIsValid() and xml_validate.h.
  static constexpr bool s_kfValidating = t_kfValidating;
  static_assert( !t_kfValidating || t_kfSupportDTD, "Validation is against the DTD: XML traits which are validating must support the DTD." );

// Declare our lexical analyzer traits:
  typedef xml_user_obj< _TyChar, s_kfSupportDTD > _TyLexUserObj;
  typedef xml_namespace_value_wrap< _TyChar > _TyXmlNamespaceValueWrap;
  typedef xml_value_traits_pack< _TyChar > _TyTpValueTraitsPack; // The types by which we extend those of _l_value: see xml_attr_default.h.
  typedef _l_traits< _TyTransport, _TyLexUserObj, _TyTpValueTraitsPack > _TyLexTraits;
  typedef xml_token< _TyTransportCtxt, _TyLexUserObj, _TyTpValueTraitsPack > _TyXmlToken;
};

__XMLP_END_NAMESPACE