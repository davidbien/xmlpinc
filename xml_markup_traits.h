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
};

template <>
struct xml_markup_traits< char16_t >
{
  typedef char16_t _TyChar;
  static constexpr _TyChar * s_kszCommentBegin = u"<!--";
  static constexpr _TyChar * s_kszCommentEnd = u"-->";
};

template <>
struct xml_markup_traits< char8_t >
{
  typedef char8_t _TyChar;
  static constexpr _TyChar * s_kszCommentBegin = u8"<!--";
  static constexpr _TyChar * s_kszCommentEnd = u8"-->";
};

__XMLP_END_NAMESPACE