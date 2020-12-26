#ifndef __XML_NS_H___
#define __XML_NS_H___

//          Copyright David Lawrence Bien 1997 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt).

// xml_ns.h

// Namespaces for xml parser imeplementation.

#include "_l_ns.h"

#define __XMLP_USE_NAMESPACE ns_xmlp
#define __XMLP_BEGIN_NAMESPACE namespace __XMLP_USE_NAMESPACE { __LEXOBJ_USING_NAMESPACE __BIENUTIL_USING_NAMESPACE using namespace std;
#define __XMLP_END_NAMESPACE }
#define __XMLP_USING_NAMESPACE using namespace __XMLP_USE_NAMESPACE;
#define __XMLP_NAMESPACE __XMLP_USE_NAMESPACE::
#endif //__XMLP_GLOBALNAMESPACE

#ifndef __L_DEFAULT_ALLOCATOR
#define __L_DEFAULT_ALLOCATOR	allocator< char >
#endif //!__L_DEFAULT_ALLOCATOR

#endif //__XML_NS_H___
