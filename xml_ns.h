#pragma once 
//          Copyright David Lawrence Bien 1997 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt).

// xml_ns.h
// Namespaces for xml parser imeplementation.
// dbien
// DEC2020

#include "_l_ns.h"

#define __XMLP_USE_NAMESPACE ns_xmlp
#define __XMLP_BEGIN_NAMESPACE namespace __XMLP_USE_NAMESPACE { __LEXOBJ_USING_NAMESPACE
#define __XMLP_END_NAMESPACE }
#define __XMLP_USING_NAMESPACE using namespace __XMLP_USE_NAMESPACE; __LEXOBJ_USING_NAMESPACE
#define __XMLP_NAMESPACE __XMLP_USE_NAMESPACE::
