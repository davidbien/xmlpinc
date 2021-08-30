#pragma once

//          Copyright David Lawrence Bien 1997 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt).

// xml_markup_traits.h
// XML markup traits classes. Really just for organizing markup character strings for output more or less.
// dbien
// 29FEB2021

#include "xml_types.h"

__XMLP_BEGIN_NAMESPACE

template <>
struct xml_markup_traits< char32_t >
{
  typedef char32_t _TyChar;
  static constexpr const _TyChar s_kszXMLDeclBegin[] = U"<?xml version=";
  static constexpr const _TyChar s_kszXMLDeclVersionDoubleQuote[] = U"\"1.0\"";
  static constexpr const _TyChar s_kszXMLDeclVersionSingleQuote[] = U"'1.0'";
  static constexpr const _TyChar s_kszXMLDeclEncodingEquals[] = U"\x20" "encoding=";
  static constexpr const _TyChar s_kszXMLDeclStandaloneEquals[] = U"\x20" "standalone=";
  static constexpr const _TyChar s_kszXMLDeclYes[] = U"yes";
  static constexpr const _TyChar s_kszXMLDeclNo[] = U"no";
  static constexpr const _TyChar s_kszXMLDeclEnd[] = U"?>";

  static constexpr const _TyChar s_kszTagBegin[] = U"<";
  static constexpr const _TyChar s_kszTagEnd[] = U">";
  static constexpr const _TyChar s_kszEmptyElemTagEnd[] = U"/>";
  static constexpr const _TyChar s_kszEndTagBegin[] = U"</";
  static constexpr const _TyChar s_kszTagColon[] = U":";
  static constexpr const _TyChar s_kszEqualSign[] = U"=";
  static constexpr const _TyChar s_kszDoubleQuote[] = U"\"";
  static constexpr const _TyChar s_kszSingleQuote[] = U"'";

  static constexpr const _TyChar s_kszEntityLessThan[] = U"&lt;";
  static constexpr const _TyChar s_kszEntityGreaterThan[] = U"&gt;";
  static constexpr const _TyChar s_kszEntityApostrophe[] = U"&apos;";
  static constexpr const _TyChar s_kszEntityAmpersand[] = U"&amp;";
  static constexpr const _TyChar s_kszEntityDoubleQuote[] = U"&quot;";
  
  static constexpr const _TyChar s_kszEntityRefRefBegin[] = U"&";
  static constexpr const _TyChar s_kszCharDecRefBegin[] = U"&#";
  static constexpr const _TyChar s_kszCharHexRefBegin[] = U"&#x";
  static constexpr const _TyChar s_kszReferenceEnd[] = U";";

  static constexpr const _TyChar s_kszCommentBegin[] = U"<!--";
  static constexpr const _TyChar s_kszCommentEnd[] = U"-->";
  static constexpr const _TyChar s_kszCDataSectionBegin[] = U"<![CDATA[";
  static constexpr const _TyChar s_kszCDataSectionEnd[] = U"]]>";
  static constexpr const _TyChar s_kszCDataSectionReplaceEnd[] = U"]]]]><![CDATA[>";
  
  static constexpr const _TyChar s_kszProcessingInstructionBegin[] = U"<?";
  static constexpr const _TyChar s_kszProcessingInstructionEnd[] = U"?>";
  static constexpr const _TyChar s_kszSpace[] = U"\x20";

  static constexpr const _TyChar s_kszXmlnsEtc[] = U"xmlns:";
  static constexpr const _TyChar s_kszXmlPrefix[] = U"xml";
  static constexpr const _TyChar s_kszXmlUri[] = U"http://www.w3.org/XML/1998/namespace";
};

template <>
struct xml_markup_traits< char16_t >
{
  typedef char16_t _TyChar;
  static constexpr const _TyChar s_kszXMLDeclBegin[] = u"<?xml version=";
  static constexpr const _TyChar s_kszXMLDeclVersionDoubleQuote[] = u"\"1.0\"";
  static constexpr const _TyChar s_kszXMLDeclVersionSingleQuote[] = u"'1.0'";
  static constexpr const _TyChar s_kszXMLDeclEncodingEquals[] = u"\x20" "encoding=";
  static constexpr const _TyChar s_kszXMLDeclStandaloneEquals[] = u"\x20" "standalone=";
  static constexpr const _TyChar s_kszXMLDeclYes[] = u"yes";
  static constexpr const _TyChar s_kszXMLDeclNo[] = u"no";
  static constexpr const _TyChar s_kszXMLDeclEnd[] = u"?>";

  static constexpr const _TyChar s_kszTagBegin[] = u"<";
  static constexpr const _TyChar s_kszTagEnd[] = u">";
  static constexpr const _TyChar s_kszEmptyElemTagEnd[] = u"/>";
  static constexpr const _TyChar s_kszEndTagBegin[] = u"</";
  static constexpr const _TyChar s_kszTagColon[] = u":";
  static constexpr const _TyChar s_kszEqualSign[] = u"=";
  static constexpr const _TyChar s_kszDoubleQuote[] = u"\"";
  static constexpr const _TyChar s_kszSingleQuote[] = u"'";

  static constexpr const _TyChar s_kszEntityLessThan[] = u"&lt;";
  static constexpr const _TyChar s_kszEntityGreaterThan[] = u"&gt;";
  static constexpr const _TyChar s_kszEntityApostrophe[] = u"&apos;";
  static constexpr const _TyChar s_kszEntityAmpersand[] = u"&amp;";
  static constexpr const _TyChar s_kszEntityDoubleQuote[] = u"&quot;";
  
  static constexpr const _TyChar s_kszEntityRefRefBegin[] = u"&";
  static constexpr const _TyChar s_kszCharDecRefBegin[] = u"&#";
  static constexpr const _TyChar s_kszCharHexRefBegin[] = u"&#x";
  static constexpr const _TyChar s_kszReferenceEnd[] = u";";

  static constexpr const _TyChar s_kszCommentBegin[] = u"<!--";
  static constexpr const _TyChar s_kszCommentEnd[] = u"-->";
  static constexpr const _TyChar s_kszCDataSectionBegin[] = u"<![CDATA[";
  static constexpr const _TyChar s_kszCDataSectionEnd[] = u"]]>";
  static constexpr const _TyChar s_kszCDataSectionReplaceEnd[] = u"]]]]><![CDATA[>";

  static constexpr const _TyChar s_kszProcessingInstructionBegin[] = u"<?";
  static constexpr const _TyChar s_kszProcessingInstructionEnd[] = u"?>";
  static constexpr const _TyChar s_kszSpace[] = u"\x20";

  static constexpr const _TyChar s_kszXmlnsEtc[] = u"xmlns:";
  static constexpr const _TyChar s_kszXmlPrefix[] = u"xml";
  static constexpr const _TyChar s_kszXmlUri[] = u"http://www.w3.org/XML/1998/namespace";
};

template <>
struct xml_markup_traits< char8_t >
{
  typedef char8_t _TyChar;
  static constexpr const _TyChar s_kszXMLDeclBegin[] = u8"<?xml version=";
  static constexpr const _TyChar s_kszXMLDeclVersionDoubleQuote[] = u8"\"1.0\"";
  static constexpr const _TyChar s_kszXMLDeclVersionSingleQuote[] = u8"'1.0'";
  static constexpr const _TyChar s_kszXMLDeclEncodingEquals[] = u8"\x20" "encoding=";
  static constexpr const _TyChar s_kszXMLDeclStandaloneEquals[] = u8"\x20" "standalone=";
  static constexpr const _TyChar s_kszXMLDeclYes[] = u8"yes";
  static constexpr const _TyChar s_kszXMLDeclNo[] = u8"no";
  static constexpr const _TyChar s_kszXMLDeclEnd[] = u8"?>";

  static constexpr const _TyChar s_kszTagBegin[] = u8"<";
  static constexpr const _TyChar s_kszTagEnd[] = u8">";
  static constexpr const _TyChar s_kszEmptyElemTagEnd[] = u8"/>";
  static constexpr const _TyChar s_kszEndTagBegin[] = u8"</";
  static constexpr const _TyChar s_kszTagColon[] = u8":";
  static constexpr const _TyChar s_kszEqualSign[] = u8"=";
  static constexpr const _TyChar s_kszDoubleQuote[] = u8"\"";
  static constexpr const _TyChar s_kszSingleQuote[] = u8"'";

  static constexpr const _TyChar s_kszEntityLessThan[] = u8"&lt;";
  static constexpr const _TyChar s_kszEntityGreaterThan[] = u8"&gt;";
  static constexpr const _TyChar s_kszEntityApostrophe[] = u8"&apos;";
  static constexpr const _TyChar s_kszEntityAmpersand[] = u8"&amp;";
  static constexpr const _TyChar s_kszEntityDoubleQuote[] = u8"&quot;";

  static constexpr const _TyChar s_kszEntityRefRefBegin[] = u8"&";
  static constexpr const _TyChar s_kszCharDecRefBegin[] = u8"&#";
  static constexpr const _TyChar s_kszCharHexRefBegin[] = u8"&#x";
  static constexpr const _TyChar s_kszReferenceEnd[] = u8";";

  static constexpr const _TyChar s_kszCommentBegin[] = u8"<!--";
  static constexpr const _TyChar s_kszCommentEnd[] = u8"-->";
  static constexpr const _TyChar s_kszCDataSectionBegin[] = u8"<![CDATA[";
  static constexpr const _TyChar s_kszCDataSectionEnd[] = u8"]]>";
  static constexpr const _TyChar s_kszCDataSectionReplaceEnd[] = u8"]]]]><![CDATA[>";

  static constexpr const _TyChar s_kszProcessingInstructionBegin[] = u8"<?";
  static constexpr const _TyChar s_kszProcessingInstructionEnd[] = u8"?>";
  static constexpr const _TyChar s_kszSpace[] = u8"\x20";

  static constexpr const _TyChar s_kszXmlnsEtc[] = u8"xmlns:";
  static constexpr const _TyChar s_kszXmlPrefix[] = u8"xml";
  static constexpr const _TyChar s_kszXmlUri[] = u8"http://www.w3.org/XML/1998/namespace";
};

__XMLP_END_NAMESPACE
