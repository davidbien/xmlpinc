#pragma once

// xml_markup_traits.h
// XML markup traits classes. Really just for organizing markup character strings for output more or less.
// dbien
// 29FEB2021

__XMLP_BEGIN_NAMESPACE

template < class t_TyChar >
struct xml_markup_traits;

template <>
struct xml_markup_traits< char32_t >
{
  typedef char32_t _TyChar;
  static constexpr _TyChar * s_kszCommentBegin = U"<!--";
  static constexpr _TyChar * s_kszCommentEnd = U"-->";
  static constexpr _TyChar * s_kszCDataSectionBegin = U"<![CDATA[";
  static constexpr _TyChar * s_kszCDataSectionEnd = U"]]>";
  static constexpr _TyChar * s_kszCDataSectionReplaceEnd = U"]]]]><![CDATA[>";
  static constexpr _TyChar * s_kszCDataSectionReplaceEnd = U"]]]]><![CDATA[>";
  
  static constexpr _TyChar * s_kszXmlnsEtc = U"xmlns:";
  static constexpr _TyChar * s_kszXmlPrefix = U"xml";
  static constexpr _TyChar * s_kszXmlUri = U"http://www.w3.org/XML/1998/namespace";
};

template <>
struct xml_markup_traits< char16_t >
{
  typedef char16_t _TyChar;
  static constexpr _TyChar * s_kszCommentBegin = u"<!--";
  static constexpr _TyChar * s_kszCommentEnd = u"-->";
  static constexpr _TyChar * s_kszCDataSectionBegin = u"<![CDATA[";
  static constexpr _TyChar * s_kszCDataSectionEnd = u"]]>";
  static constexpr _TyChar * s_kszCDataSectionReplaceEnd = u"]]]]><![CDATA[>";

  static constexpr _TyChar * s_kszXmlnsEtc = u"xmlns:";
  static constexpr _TyChar * s_kszXmlPrefix = u"xml";
  static constexpr _TyChar * s_kszXmlUri = u"http://www.w3.org/XML/1998/namespace";
};

template <>
struct xml_markup_traits< char8_t >
{
  typedef char8_t _TyChar;
  static constexpr _TyChar * s_kszCommentBegin = u8"<!--";
  static constexpr _TyChar * s_kszCommentEnd = u8"-->";
  static constexpr _TyChar * s_kszCDataSectionBegin = u8"<![CDATA[";
  static constexpr _TyChar * s_kszCDataSectionEnd = u8"]]>";
  static constexpr _TyChar * s_kszCDataSectionReplaceEnd = u8"]]]]><![CDATA[>";

  static constexpr _TyChar * s_kszXmlnsEtc = u8"xmlns:";
  static constexpr _TyChar * s_kszXmlPrefix = u8"xml";
  static constexpr _TyChar * s_kszXmlUri = u8"http://www.w3.org/XML/1998/namespace";
};

__XMLP_END_NAMESPACE
