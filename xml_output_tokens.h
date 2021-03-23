#pragma once

// xml_output_tokens.h
// XML output validation tokens.
// dbien
// 25FEB2021

// Just a place to put these things.

__XMLP_BEGIN_NAMESPACE

// PspGetSpacesStart:
template < class t_TyChar >
const _l_state_proto< t_TyChar > * PspGetSpacesStart()
  requires( is_same_v< t_TyChar, char32_t > )
{
  return (const _l_state_proto< t_TyChar > *)&startUTF32Spaces;
}
template < class t_TyChar >
const _l_state_proto< t_TyChar > * PspGetSpacesStart()
  requires( is_same_v< t_TyChar, char16_t > )
{
  return (const _l_state_proto< t_TyChar > *)&startUTF16Spaces;
}
template < class t_TyChar >
const _l_state_proto< t_TyChar > * PspGetSpacesStart()
  requires( is_same_v< t_TyChar, char8_t > )
{
  return (const _l_state_proto< t_TyChar > *)&startUTF8Spaces;
}
// PspGetEncNameStart:
template < class t_TyChar >
const _l_state_proto< t_TyChar > * PspGetEncNameStart()
  requires( is_same_v< t_TyChar, char32_t > )
{
  return (const _l_state_proto< t_TyChar > *)&startUTF32EncName;
}
template < class t_TyChar >
const _l_state_proto< t_TyChar > * PspGetEncNameStart()
  requires( is_same_v< t_TyChar, char16_t > )
{
  return (const _l_state_proto< t_TyChar > *)&startUTF16EncName;
}
template < class t_TyChar >
const _l_state_proto< t_TyChar > * PspGetEncNameStart()
  requires( is_same_v< t_TyChar, char8_t > )
{
  return (const _l_state_proto< t_TyChar > *)&startUTF8EncName;
}
// PspGetCommentCharsStart:
template < class t_TyChar >
const _l_state_proto< t_TyChar > * PspGetCommentCharsStart()
  requires( is_same_v< t_TyChar, char32_t > )
{
  return (const _l_state_proto< t_TyChar > *)&startUTF32CommentChars;
}
template < class t_TyChar >
const _l_state_proto< t_TyChar > * PspGetCommentCharsStart()
  requires( is_same_v< t_TyChar, char16_t > )
{
  return (const _l_state_proto< t_TyChar > *)&startUTF16CommentChars;
}
template < class t_TyChar >
const _l_state_proto< t_TyChar > * PspGetCommentCharsStart()
  requires( is_same_v< t_TyChar, char8_t > )
{
  return (const _l_state_proto< t_TyChar > *)&startUTF8CommentChars;
}
// PspGetCharDataStart:
template < class t_TyChar >
const _l_state_proto< t_TyChar > * PspGetCharDataStart()
  requires( is_same_v< t_TyChar, char32_t > )
{
  return (const _l_state_proto< t_TyChar > *)&startUTF32CharData;
}
template < class t_TyChar >
const _l_state_proto< t_TyChar > * PspGetCharDataStart()
  requires( is_same_v< t_TyChar, char16_t > )
{
  return (const _l_state_proto< t_TyChar > *)&startUTF16CharData;
}
template < class t_TyChar >
const _l_state_proto< t_TyChar > * PspGetCharDataStart()
  requires( is_same_v< t_TyChar, char8_t > )
{
  return (const _l_state_proto< t_TyChar > *)&startUTF8CharData;
}
// PspGetNCNameStart:
template < class t_TyChar >
const _l_state_proto< t_TyChar > * PspGetNCNameStart()
  requires( is_same_v< t_TyChar, char32_t > )
{
  return (const _l_state_proto< t_TyChar > *)&startUTF32NCName;
}
template < class t_TyChar >
const _l_state_proto< t_TyChar > * PspGetNCNameStart()
  requires( is_same_v< t_TyChar, char16_t > )
{
  return (const _l_state_proto< t_TyChar > *)&startUTF16NCName;
}
template < class t_TyChar >
const _l_state_proto< t_TyChar > * PspGetNCNameStart()
  requires( is_same_v< t_TyChar, char8_t > )
{
  return (const _l_state_proto< t_TyChar > *)&startUTF8NCName;
}
// PspGetCDCharsOutputValidateStart:
template < class t_TyChar >
const _l_state_proto< t_TyChar > * PspGetCDCharsOutputValidateStart()
  requires( is_same_v< t_TyChar, char32_t > )
{
  return (const _l_state_proto< t_TyChar > *)&startUTF32CDCharsOutputValidate;
}
template < class t_TyChar >
const _l_state_proto< t_TyChar > * PspGetCDCharsOutputValidateStart()
  requires( is_same_v< t_TyChar, char16_t > )
{
  return (const _l_state_proto< t_TyChar > *)&startUTF16CDCharsOutputValidate;
}
template < class t_TyChar >
const _l_state_proto< t_TyChar > * PspGetCDCharsOutputValidateStart()
  requires( is_same_v< t_TyChar, char8_t > )
{
  return (const _l_state_proto< t_TyChar > *)&startUTF8CDCharsOutputValidate;
}
// PspGetAllReferencesStart:
template < class t_TyChar >
const _l_state_proto< t_TyChar > * PspGetAllReferencesStart()
  requires( is_same_v< t_TyChar, char32_t > )
{
  return (const _l_state_proto< t_TyChar > *)&startUTF32AllReferences;
}
template < class t_TyChar >
const _l_state_proto< t_TyChar > * PspGetAllReferencesStart()
  requires( is_same_v< t_TyChar, char16_t > )
{
  return (const _l_state_proto< t_TyChar > *)&startUTF16AllReferences;
}
template < class t_TyChar >
const _l_state_proto< t_TyChar > * PspGetAllReferencesStart()
  requires( is_same_v< t_TyChar, char8_t > )
{
  return (const _l_state_proto< t_TyChar > *)&startUTF8AllReferences;
}

// TGetCharDataStart: CharData output validation token adaptor:
template < class t_TyChar >
struct TGetCharDataStart;
template < >
struct TGetCharDataStart< char8_t >
{
  static inline const _l_state_proto< char8_t >* s_kpspStart = (const _l_state_proto< char8_t >*)&startUTF8CharData;
};
template < >
struct TGetCharDataStart< char16_t >
{
  static inline const _l_state_proto< char16_t >* s_kpspStart = (const _l_state_proto< char16_t >*)&startUTF16CharData;
};
template < >
struct TGetCharDataStart< char32_t >
{
  static inline const _l_state_proto< char32_t >* s_kpspStart = (const _l_state_proto< char32_t >*)&startUTF32CharData;
};
// TGetAttCharDataNoSingleQuoteStart: AttCharDataNoSingleQuote output validation token adaptor:
template < class t_TyChar >
struct TGetAttCharDataNoSingleQuoteStart;
template < >
struct TGetAttCharDataNoSingleQuoteStart< char8_t >
{
  static inline const _l_state_proto< char8_t >* s_kpspStart = (const _l_state_proto< char8_t >*)&startUTF8AttCharDataNoSingleQuote;
};
template < >
struct TGetAttCharDataNoSingleQuoteStart< char16_t >
{
  static inline const _l_state_proto< char16_t >* s_kpspStart = (const _l_state_proto< char16_t >*)&startUTF16AttCharDataNoSingleQuote;
};
template < >
struct TGetAttCharDataNoSingleQuoteStart< char32_t >
{
  static inline const _l_state_proto< char32_t >* s_kpspStart = (const _l_state_proto< char32_t >*)&startUTF32AttCharDataNoSingleQuote;
};
// TGetAttCharDataNoDoubleQuoteStart: AttCharDataNoDoubleQuote output validation token adaptor:
template < class t_TyChar >
struct TGetAttCharDataNoDoubleQuoteStart;
template < >
struct TGetAttCharDataNoDoubleQuoteStart< char8_t >
{
  static inline const _l_state_proto< char8_t >* s_kpspStart = (const _l_state_proto< char8_t >*)&startUTF8AttCharDataNoDoubleQuote;
};
template < >
struct TGetAttCharDataNoDoubleQuoteStart< char16_t >
{
  static inline const _l_state_proto< char16_t >* s_kpspStart = (const _l_state_proto< char16_t >*)&startUTF16AttCharDataNoDoubleQuote;
};
template < >
struct TGetAttCharDataNoDoubleQuoteStart< char32_t >
{
  static inline const _l_state_proto< char32_t >* s_kpspStart = (const _l_state_proto< char32_t >*)&startUTF32AttCharDataNoDoubleQuote;
};
// TGetPITargetStart: PITarget output validation token adaptor:
template < class t_TyChar >
struct TGetPITargetStart;
template < >
struct TGetPITargetStart< char8_t >
{
  static inline const _l_state_proto< char8_t >* s_kpspStart = (const _l_state_proto< char8_t >*)&startUTF8PITarget;
};
template < >
struct TGetPITargetStart< char16_t >
{
  static inline const _l_state_proto< char16_t >* s_kpspStart = (const _l_state_proto< char16_t >*)&startUTF16PITarget;
};
template < >
struct TGetPITargetStart< char32_t >
{
  static inline const _l_state_proto< char32_t >* s_kpspStart = (const _l_state_proto< char32_t >*)&startUTF32PITarget;
};
// TGetPITargetMeatStart: PITargetMeat output validation token adaptor:
template < class t_TyChar >
struct TGetPITargetMeatStart;
template < >
struct TGetPITargetMeatStart< char8_t >
{
  static inline const _l_state_proto< char8_t >* s_kpspStart = (const _l_state_proto< char8_t >*)&startUTF8PITargetMeatOutputValidate;
};
template < >
struct TGetPITargetMeatStart< char16_t >
{
  static inline const _l_state_proto< char16_t >* s_kpspStart = (const _l_state_proto< char16_t >*)&startUTF16PITargetMeatOutputValidate;
};
template < >
struct TGetPITargetMeatStart< char32_t >
{
  static inline const _l_state_proto< char32_t >* s_kpspStart = (const _l_state_proto< char32_t >*)&startUTF32PITargetMeatOutputValidate;
};

__XMLP_END_NAMESPACE
