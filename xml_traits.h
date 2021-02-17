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
  typedef typename t_TyTransport::_TyTransportCtxt _TyTransportCtxt;
  typedef basic_string< _TyChar > _TyStdStr;
  typedef basic_string_view< _TyChar > _TyStrView;

  static constexpr bool s_kfSupportDTD = t_kfSupportDTD;
  static constexpr bool s_kfValidating = t_kfValidating;

// Declare our lexical analyzer traits:
  typedef xml_user_obj< _TyChar, s_kfSupportDTD > _TyLexUserObj;
  typedef xml_namespace_value_wrap< _TyChar > _TyXmlNamespaceValueWrap;
  typedef tuple< _TyXmlNamespaceValueWrap > _TyTpValueTraitsPack; // We are extending the types in _l_value by this one type currently.
  typedef _l_traits< _TyTransport, _TyLexUserObj, _TyTpValueTraitsPack > _TyLexTraits;
  typedef xml_token< _TyTransportCtxt, _TyLexUserObj, _TyTpValueTraitsPack > _TyXmlToken;
};

__XMLP_END_NAMESPACE