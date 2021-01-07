#pragma once

// xml_types.h
// dbien
// 06JAN2021
// Types definitions and predeclares for XML parser.

#include "xml_ns.h"
#include "_l_types.h"

__XMLP_BEGIN_NAMESPACE

	// There various types of data we might get in XML.
	static const vtyDataType s_kdtPlainText = 0;
	static const vtyDataType s_kdtEntityRef = 1;
	static const vtyDataType s_kdtCharDecRef = 2;
	static const vtyDataType s_kdtCharHexRef = 3;
	static const vtyDataType s_kdtPEReference = 4;

__XMLP_END_NAMESPACE
