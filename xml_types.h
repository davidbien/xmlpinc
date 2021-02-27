#pragma once

// xml_types.h
// dbien
// 06JAN2021
// Types definitions and predeclares for XML parser.

#include <unordered_set>
#include <unordered_map>
#include "upslist.h"
#include "xml_ns.h"
#include "_l_types.h"

__XMLP_BEGIN_NAMESPACE

// There various types of data we might get in XML.
static const vtyDataType s_kdtPlainText = 0;
static const vtyDataType s_kdtCharDecRef = 1;
static const vtyDataType s_kdtCharHexRef = 2;
static const vtyDataType s_kdtFirstDTDRef = 3;
static const vtyDataType s_kdtEntityRef = 3;
static const vtyDataType s_kdtPEReference = 4;

// The string containing the values from the Production S.
#define STR_XML_WHITESPACE_TOKEN "\n\r\x20\t"

// Try to propagate this to the current usage areas that merely contain "false" - this flags where we need to change when we support DTDs.
static constexpr bool vkfSupportDTD = false;

// xml_traits.h
template < class t_TyTransport, bool t_kfSupportDTD, bool t_kfValidating >
struct xml_traits;

// xml_user.h:
template < class t_tyChar, bool t_kfSupportDTD >
class _xml_user_obj_base_dtd;
template < class t_tyChar, bool t_kfSupportDTD >
class xml_user_obj;

// xml_namespace.h
template < class t_TyChar >
class _xml_namespace_uri;
template < class t_TyChar >
class xml_namespace_value_wrap;

// xml_cursor.h:
template < class t_TyXmlTraits >
class _xml_read_context;
template < class t_TyXmlTraits >
class xml_read_cursor;
template < class t_TyTpTransports >
class xml_read_cursor_var;

// xml_token.h:
template < class t_TyTransportCtxt, class t_TyUserObj, class t_TyTpValueTraits >
class xml_token;
template < class t_TyTpTransports >
class xml_token_var;

// xml_tag.h:
template < class t_TyXmlTraits >
class xml_tag;
template < class t_TyXmlTraits >
class xml_document;
// xml_tag_var.h:
template < class t_TyTpTransports >
class xml_tag_var;
template < class t_TyTpTransports >
class xml_document_var;

// xml_parse.h:
template < class t_TyXmlTraits >
class xml_parser;
template <  template < class ... > class t_tempTyTransport, 
            class t_TyTp2DCharPack = tuple< tuple< char32_t, true_type >, tuple< char32_t, false_type >, tuple< char16_t, true_type >, tuple< char16_t, false_type >, tuple< char8_t, false_type > > >
class xml_parser_var;

// xml_exc.h:
class xml_parse_exception;

// When we actually support DTD and validation (if ever because they aren't that important to me) then we might have to make this more complex.
// This is an adaptor for use with MultiplexTuplePack_t<>.
template < class t_TyTransport >
using TGetXmlTraitsDefault = xml_traits< t_TyTransport, false, false >;

// TGetPrefixUri (prefix,URI).
template < class t_TyChar >
using TGetPrefixUri = std::pair< basic_string< t_TyChar >, basic_string< t_TyChar > >;

// _xml_namespace_map_traits:
// This just organizes the types for the namespace map in one place because we reference it in a couple of places and we don't want to templatize by the full type because it results in unreadable log messages, etc.
template < class t_TyChar >
struct _xml_namespace_map_traits
{
  typedef t_TyChar _TyChar;
  typedef basic_string< _TyChar > _TyStdStr;
  typedef basic_string_view< _TyChar > _TyStrView;
  typedef StringTransparentHash< _TyChar > _TyStringTransparentHash; // Allow lookup by string_view without creating a string.
  typedef unordered_set< _TyStdStr, _TyStringTransparentHash, std::equal_to<void> > _TyUriAndPrefixMap;
  typedef _xml_namespace_uri< _TyChar > _TyNamespaceUri;
  typedef UniquePtrSList< _TyNamespaceUri > _TyUpListNamespaceUris;
  // first is a pointer to a PrefixMap value for the prefix, second is the current list of namespace URIs for that prefix.
  typedef pair< const typename _TyUriAndPrefixMap::value_type *, _TyUpListNamespaceUris > _TyNamespaceMapped;
  typedef unordered_map< _TyStdStr, _TyNamespaceMapped, _TyStringTransparentHash, std::equal_to<void> > _TyNamespaceMap;
};

// Declare all the various types of the triggers and tokens for the XML lexical analyzer.
static const vtyActionIdent s_knTriggerPITargetStart = 1;
static const vtyActionIdent s_knTriggerPITargetEnd = 2;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTriggerPITargetStart = _l_trigger_position< t_TyLexTraits, s_knTriggerPITargetStart, t_fInLexGen >;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTriggerPITargetEnd = _l_trigger_string< t_TyLexTraits, s_knTriggerPITargetEnd, s_knTriggerPITargetStart, t_fInLexGen >;
static const vtyActionIdent s_knTriggerPITargetMeatBegin = 3;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTriggerPITargetMeatBegin = _l_trigger_position< t_TyLexTraits, s_knTriggerPITargetMeatBegin, t_fInLexGen >;

static const vtyActionIdent s_knTriggerMixedNameBegin = 5;
static const vtyActionIdent s_knTriggerMixedNameEnd = 6;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTriggerMixedNameBegin = _l_trigger_position< t_TyLexTraits, s_knTriggerMixedNameBegin, t_fInLexGen >;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTriggerMixedNameEnd = _l_trigger_string< t_TyLexTraits, s_knTriggerMixedNameEnd, s_knTriggerMixedNameBegin, t_fInLexGen >;

// For now we will lump together everything that is a subset of CHARDATA (i.e. attr values in which can be no ' or ", etc)
//	into the same triggers and resultant token data. This should be perfectly fine during input of XML - also
//	it saves us implementation space.
static const vtyActionIdent s_knTriggerCharDataBegin = 7;
static const vtyActionIdent s_knTriggerCharDataEnd = 8;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTriggerCharDataBegin = _l_trigger_position< t_TyLexTraits, s_knTriggerCharDataBegin, t_fInLexGen >;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTriggerCharDataEnd = _l_trigger_string< t_TyLexTraits, s_knTriggerCharDataEnd, s_knTriggerCharDataBegin, t_fInLexGen >;

// We need to have a different set of triggers for the single-quoted CharData since the actions become ambiguous with each other when the are the same - which makes sense.
static const vtyActionIdent s_knTriggerCharDataSingleQuoteBegin = 9;
static const vtyActionIdent s_knTriggerCharDataSingleQuoteEnd = 10;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTriggerCharDataSingleQuoteBegin = _l_trigger_position< t_TyLexTraits, s_knTriggerCharDataSingleQuoteBegin , t_fInLexGen >;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTriggerCharDataSingleQuoteEnd = _l_trigger_string_typed_range< s_kdtPlainText, TyGetTriggerCharDataEnd<t_TyLexTraits,t_fInLexGen>, 
                                                                          s_knTriggerCharDataSingleQuoteEnd, s_knTriggerCharDataSingleQuoteBegin >;

static const vtyActionIdent s_knTriggerEntityRefBegin = 11;
static const vtyActionIdent s_knTriggerEntityRefEnd = 12;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTriggerEntityRefBegin = _l_trigger_string_typed_beginpoint< s_kdtEntityRef, TyGetTriggerCharDataEnd<t_TyLexTraits,t_fInLexGen>, s_knTriggerEntityRefBegin >;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTriggerEntityRefEnd = _l_trigger_string_typed_endpoint< s_kdtEntityRef, TyGetTriggerCharDataEnd<t_TyLexTraits,t_fInLexGen>, s_knTriggerEntityRefEnd, s_knTriggerEntityRefBegin >;

static const vtyActionIdent s_knTriggerCharDecRefBegin = 13;
static const vtyActionIdent s_knTriggerCharDecRefEnd = 14;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTriggerCharDecRefBegin = _l_trigger_string_typed_beginpoint< s_kdtCharDecRef, TyGetTriggerCharDataEnd<t_TyLexTraits, t_fInLexGen>, s_knTriggerCharDecRefBegin >;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTriggerCharDecRefEnd = _l_trigger_string_typed_endpoint< s_kdtCharDecRef, TyGetTriggerCharDataEnd<t_TyLexTraits, t_fInLexGen>, s_knTriggerCharDecRefEnd, s_knTriggerCharDecRefBegin >;

static const vtyActionIdent s_knTriggerCharHexRefBegin = 15;
static const vtyActionIdent s_knTriggerCharHexRefEnd = 16;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTriggerCharHexRefBegin = _l_trigger_string_typed_beginpoint< s_kdtCharHexRef, TyGetTriggerCharDataEnd<t_TyLexTraits,t_fInLexGen>, s_knTriggerCharHexRefBegin >;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTriggerCharHexRefEnd = _l_trigger_string_typed_endpoint< s_kdtCharHexRef, TyGetTriggerCharDataEnd<t_TyLexTraits,t_fInLexGen>, s_knTriggerCharHexRefEnd, s_knTriggerCharHexRefBegin >;

static const vtyActionIdent s_knTriggerAttValueDoubleQuote = 17;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTriggerAttValueDoubleQuote  = _l_trigger_bool< t_TyLexTraits, s_knTriggerAttValueDoubleQuote, t_fInLexGen >;

static const vtyActionIdent s_knTriggerPrefixBegin = 18;
static const vtyActionIdent s_knTriggerPrefixEnd = 19;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTriggerPrefixBegin = _l_trigger_position< t_TyLexTraits, s_knTriggerPrefixBegin, t_fInLexGen >;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTriggerPrefixEnd = _l_trigger_string< t_TyLexTraits, s_knTriggerPrefixEnd, s_knTriggerPrefixBegin, t_fInLexGen >;

static const vtyActionIdent s_knTriggerLocalPartBegin = 20;
static const vtyActionIdent s_knTriggerLocalPartEnd = 21;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTriggerLocalPartBegin = _l_trigger_position< t_TyLexTraits, s_knTriggerLocalPartBegin, t_fInLexGen >;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTriggerLocalPartEnd = _l_trigger_string< t_TyLexTraits, s_knTriggerLocalPartEnd, s_knTriggerLocalPartBegin, t_fInLexGen >;

#if 0 // This trigger is ambiguous due to the nature of attribute names and xmlns attribute names - they match each other - so this trigger fires all the time erroneously.
static const vtyActionIdent s_knTriggerDefaultAttName = 20;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTriggerDefaultAttName _l_trigger_bool< t_TyLexTraits, s_knTriggerDefaultAttName, t_fInLexGen >;
#endif //0

#if 0 // These triggers also cause ambiguity since PrefixedAttName matches a QName.
static const vtyActionIdent s_knTriggerPrefixedAttNameBegin = 21;
static const vtyActionIdent s_knTriggerPrefixedAttNameEnd = 22;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTriggerPrefixedAttNameBegin = _l_trigger_position< t_TyLexTraits, s_knTriggerPrefixedAttNameBegin, t_fInLexGen >;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTriggerPrefixedAttNameEnd = _l_trigger_string< t_TyLexTraits, s_knTriggerPrefixedAttNameEnd, s_knTriggerPrefixedAttNameBegin, t_fInLexGen >;
#endif //0

static const vtyActionIdent s_knTriggerSaveTagName = 23;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTriggerSaveTagName = _l_action_save_data_single< s_knTriggerSaveTagName, TyGetTriggerPrefixEnd<t_TyLexTraits,t_fInLexGen>, TyGetTriggerLocalPartEnd<t_TyLexTraits,t_fInLexGen> >;

static const vtyActionIdent s_knTriggerSaveAttributes = 24;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTriggerSaveAttributes = _l_action_save_data_multiple< s_knTriggerSaveAttributes, 
  TyGetTriggerPrefixEnd<t_TyLexTraits,t_fInLexGen>, TyGetTriggerLocalPartEnd<t_TyLexTraits,t_fInLexGen>, TyGetTriggerCharDataEnd<t_TyLexTraits,t_fInLexGen>, TyGetTriggerAttValueDoubleQuote<t_TyLexTraits,t_fInLexGen> >;

#if 0 // The comment production has a problem with triggers due to its nature. It is easy to manage without triggers - impossible to try to stick triggers in - they'll never work.
static const vtyActionIdent s_knTriggerCommentBegin = 25;
static const vtyActionIdent s_knTriggerCommentEnd = 26;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTriggerCommentBegin = _l_trigger_position< t_TyLexTraits, s_knTriggerCommentBegin, t_fInLexGen >;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTriggerCommentEnd = _l_trigger_string< t_TyLexTraits, s_knTriggerCommentEnd, s_knTriggerCommentBegin, t_fInLexGen >;
#endif //0

static const vtyActionIdent s_knTriggerStandaloneYes = 27;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTriggerStandaloneYes = _l_trigger_bool< t_TyLexTraits, s_knTriggerStandaloneYes, t_fInLexGen >;
static const vtyActionIdent s_knTriggerStandaloneDoubleQuote = 28;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTriggerStandaloneDoubleQuote = _l_trigger_bool< t_TyLexTraits, s_knTriggerStandaloneDoubleQuote, t_fInLexGen >;

static const vtyActionIdent s_knTriggerEncodingNameBegin = 29;
static const vtyActionIdent s_knTriggerEncodingNameEnd = 30;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTriggerEncodingNameBegin = _l_trigger_position< t_TyLexTraits, s_knTriggerEncodingNameBegin, t_fInLexGen >;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTriggerEncodingNameEnd = _l_trigger_string< t_TyLexTraits, s_knTriggerEncodingNameEnd, s_knTriggerEncodingNameBegin, t_fInLexGen >;
static const vtyActionIdent s_knTriggerEncDeclDoubleQuote = 31;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTriggerEncDeclDoubleQuote = _l_trigger_bool< t_TyLexTraits, s_knTriggerEncDeclDoubleQuote, t_fInLexGen >;

static const vtyActionIdent s_knTriggerVersionNumBegin = 32;
static const vtyActionIdent s_knTriggerVersionNumEnd = 33;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTriggerVersionNumBegin = _l_trigger_position< t_TyLexTraits, s_knTriggerVersionNumBegin, t_fInLexGen >;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTriggerVersionNumEnd = _l_trigger_string< t_TyLexTraits, s_knTriggerVersionNumEnd, s_knTriggerVersionNumBegin, t_fInLexGen >;
static const vtyActionIdent s_knTriggerVersionNumDoubleQuote = 34;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTriggerVersionNumDoubleQuote = _l_trigger_bool< t_TyLexTraits, s_knTriggerVersionNumDoubleQuote, t_fInLexGen >;

static const vtyActionIdent s_knTriggerPEReferenceBegin = 35;
static const vtyActionIdent s_knTriggerPEReferenceEnd = 36;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTriggerPEReferenceBegin = _l_trigger_position< t_TyLexTraits, s_knTriggerPEReferenceBegin, t_fInLexGen >;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTriggerPEReferenceEnd = _l_trigger_string< t_TyLexTraits, s_knTriggerPEReferenceEnd, s_knTriggerPEReferenceBegin, t_fInLexGen >;

static const vtyActionIdent s_knTriggerSystemLiteralBegin = 37;
static const vtyActionIdent s_knTriggerSystemLiteralDoubleQuoteEnd = 38;
static const vtyActionIdent s_knTriggerSystemLiteralSingleQuoteEnd = 39;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTriggerSystemLiteralBegin = _l_trigger_position< t_TyLexTraits, s_knTriggerSystemLiteralBegin, t_fInLexGen >;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTriggerSystemLiteralDoubleQuoteEnd = _l_trigger_string< t_TyLexTraits, s_knTriggerSystemLiteralDoubleQuoteEnd, s_knTriggerSystemLiteralBegin, t_fInLexGen >;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTriggerSystemLiteralSingleQuoteEnd = _l_trigger_string_typed_range< s_kdtPlainText, 
  TyGetTriggerSystemLiteralDoubleQuoteEnd<t_TyLexTraits,t_fInLexGen>, s_knTriggerSystemLiteralSingleQuoteEnd, s_knTriggerSystemLiteralBegin >;

static const vtyActionIdent s_knTriggerPubidLiteralBegin = 40;
static const vtyActionIdent s_knTriggerPubidLiteralDoubleQuoteEnd = 41;
static const vtyActionIdent s_knTriggerPubidLiteralSingleQuoteEnd = 42;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTriggerPubidLiteralBegin = _l_trigger_position< t_TyLexTraits, s_knTriggerPubidLiteralBegin, t_fInLexGen >;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTriggerPubidLiteralDoubleQuoteEnd = _l_trigger_string< t_TyLexTraits, s_knTriggerPubidLiteralDoubleQuoteEnd, s_knTriggerPubidLiteralBegin, t_fInLexGen >;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTriggerPubidLiteralSingleQuoteEnd = _l_trigger_string_typed_range< s_kdtPlainText, 
  TyGetTriggerPubidLiteralDoubleQuoteEnd<t_TyLexTraits,t_fInLexGen>, s_knTriggerPubidLiteralSingleQuoteEnd, s_knTriggerPubidLiteralBegin >;

// For CharData (in between tags) we can only use a single position since a trigger at the beginning of a production is completely useless and screws things up.
// The parser will immediately "fix up" the positions to make them correct (unless it is skipping data in which case it won't).
static const vtyActionIdent s_knTriggerCharDataEndpoint = 43;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTriggerCharDataEndpoint = _l_trigger_string_typed_endpoint< s_kdtPlainText, TyGetTriggerCharDataEnd<t_TyLexTraits,t_fInLexGen>, s_knTriggerCharDataEndpoint >;

// Tokens:
static const vtyActionIdent s_knTokenSTag = 1000;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenSTag = _l_action_token< _l_action_save_data_single< s_knTokenSTag, TyGetTriggerSaveTagName<t_TyLexTraits,t_fInLexGen>, TyGetTriggerSaveAttributes<t_TyLexTraits,t_fInLexGen> > >;

static const vtyActionIdent s_knTokenETag = 1001;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenETag = _l_action_token< _l_action_save_data_single< s_knTokenETag, TyGetTriggerPrefixEnd<t_TyLexTraits,t_fInLexGen>, TyGetTriggerLocalPartEnd<t_TyLexTraits,t_fInLexGen> > >;

static const vtyActionIdent s_knTokenEmptyElemTag = 1002;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenEmptyElemTag = _l_action_token< _l_action_save_data_single< s_knTokenEmptyElemTag, TyGetTriggerSaveTagName<t_TyLexTraits,t_fInLexGen>, TyGetTriggerSaveAttributes<t_TyLexTraits,t_fInLexGen> > >;

static const vtyActionIdent s_knTokenComment = 1003;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenComment = _l_action_token< _l_trigger_noop< t_TyLexTraits, s_knTokenComment, t_fInLexGen > >;

static const vtyActionIdent s_knTokenXMLDecl = 1004;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenXMLDecl = _l_action_token< _l_action_save_data_single< s_knTokenXMLDecl, TyGetTriggerStandaloneYes<t_TyLexTraits,t_fInLexGen>, TyGetTriggerStandaloneYes<t_TyLexTraits,t_fInLexGen>, 
  TyGetTriggerEncodingNameEnd<t_TyLexTraits,t_fInLexGen>, TyGetTriggerEncDeclDoubleQuote<t_TyLexTraits,t_fInLexGen>, TyGetTriggerVersionNumEnd<t_TyLexTraits,t_fInLexGen>, TyGetTriggerVersionNumDoubleQuote<t_TyLexTraits,t_fInLexGen> > >;

static const vtyActionIdent s_knTokenCDataSection = 1005;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenCDataSection = _l_action_token< _l_trigger_noop< t_TyLexTraits, s_knTokenCDataSection, t_fInLexGen > >;

static const vtyActionIdent s_knTokenCharData = 1006;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenCharData = _l_action_token< _l_action_save_data_single< s_knTokenCharData, TyGetTriggerCharDataEnd<t_TyLexTraits,t_fInLexGen> > >;

static const vtyActionIdent s_knTokenProcessingInstruction = 1007;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenProcessingInstruction = _l_action_token< _l_action_save_data_single< s_knTokenProcessingInstruction, TyGetTriggerPITargetEnd<t_TyLexTraits,t_fInLexGen>, TyGetTriggerPITargetMeatBegin<t_TyLexTraits,t_fInLexGen> > >;

// Write-validation tokens. These are used during writing to:
// 1) Validate written tokens of various types.
// 2) Determine the production of CharRefs withing Attribute Values and CharData and CDataSections.
//    We will automatically substitute CharRefs for disallowed characters in these scenarios. Clearly
//    CDataSections involve a different mechanism than AttrValues and CharData - we will correctly
//    nest CDataSections to allow for the existences of "]]>" strings in the output.
static const vtyActionIdent s_knTokenValidSpaces = 2000;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenValidSpaces = _l_action_token_id< t_TyLexTraits, s_knTokenValidSpaces, t_fInLexGen >;

static const vtyActionIdent s_knTokenValidCommentChars = 2001;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenValidCommentChars = _l_action_token_id< t_TyLexTraits, s_knTokenValidCommentChars, t_fInLexGen >;

// CharData: This validates CharData.
static const vtyActionIdent s_knTokenValidCharData = 2002;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenValidCharData = _l_action_token_id< t_TyLexTraits, s_knTokenValidCharData, t_fInLexGen >;

// CDCharsOutputValidate: This validates CDataSection contents.
static const vtyActionIdent s_knTokenValidCDataSection = 2003;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenValidCDataSection = _l_action_token_id< t_TyLexTraits, s_knTokenValidCDataSection, t_fInLexGen >;

// NCName: This validates prefixes and tag and attribute names.
static const vtyActionIdent s_knTokenValidNCName = 2004;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenValidNCName = _l_action_token_id< t_TyLexTraits, s_knTokenValidNCName, t_fInLexGen >;

// AttCharDataNoDoubleQuote: This validates and helps to process attribute values.
static const vtyActionIdent s_knTokenValidAttCharDataNoDoubleQuote = 2005;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenValidAttCharDataNoDoubleQuote = _l_action_token_id< t_TyLexTraits, s_knTokenValidAttCharDataNoDoubleQuote, t_fInLexGen >;

// AttCharDataNoSingleQuote: This validates and helps to process attribute values.
static const vtyActionIdent s_knTokenValidAttCharDataNoSingleQuote = 2006;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenValidAttCharDataNoSingleQuote = _l_action_token_id< t_TyLexTraits, s_knTokenValidAttCharDataNoSingleQuote, t_fInLexGen >;

// Name: This validates entity reference names and parameter entity reference names.
static const vtyActionIdent s_knTokenValidName = 2007;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenValidName = _l_action_token_id< t_TyLexTraits, s_knTokenValidName, t_fInLexGen >;

// CharRefDec: This validates decimal character references.
static const vtyActionIdent s_knTokenValidCharRefDec = 2008;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenValidCharRefDec = _l_action_token_id< t_TyLexTraits, s_knTokenValidCharRefDec, t_fInLexGen >;

// CharRefHex: This validates hexidecimal character references.
static const vtyActionIdent s_knTokenValidCharRefHex = 2009;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenValidCharRefHex = _l_action_token_id< t_TyLexTraits, s_knTokenValidCharRefHex, t_fInLexGen >;

// EncName: This validates an encoding name inside of an XMLDecl statement.
static const vtyActionIdent s_knTokenValidEncName = 2010;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenValidEncName = _l_action_token_id< t_TyLexTraits, s_knTokenValidEncName, t_fInLexGen >;

// PITarget: This validates the PITarget production for a Processing Instruction.
static const vtyActionIdent s_knTokenValidPITarget = 2011;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenValidPITarget = _l_action_token_id< t_TyLexTraits, s_knTokenValidPITarget, t_fInLexGen >;

// PITargetMeat: This validates the "meat" of the Processing Instruction which happens to be a sequence of Char productions.
static const vtyActionIdent s_knTokenValidPITargetMeat = 2012;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenValidPITargetMeat = _l_action_token_id< t_TyLexTraits, s_knTokenValidPITargetMeat, t_fInLexGen >;

// These actions recognize References while writing XML files. There is a system in place to automatically recognize
//  valid references within CharData and attribute data. We use _l_action_token_id<> to generate no Action###() methods
//  and no m_axn* objects. This allows us not to templatize the state machine generated and thus just use by character type
//  rather than fully specified lexical analyzer object.
static const vtyActionIdent s_knTokenEntityRef = 3000;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenEntityRef = _l_action_token_id< t_TyLexTraits, s_knTokenEntityRef, t_fInLexGen >;

static const vtyActionIdent s_knTokenCharRefDec = 3001;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenCharRefDec = _l_action_token_id< t_TyLexTraits, s_knTokenCharRefDec, t_fInLexGen >;

static const vtyActionIdent s_knTokenCharRefHex = 3002;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenCharRefHex = _l_action_token_id< t_TyLexTraits, s_knTokenCharRefHex, t_fInLexGen >;


__XMLP_END_NAMESPACE
