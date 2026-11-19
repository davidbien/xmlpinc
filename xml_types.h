#pragma once

//          Copyright David Lawrence Bien 1997 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt).

// xml_types.h
// dbien
// 06JAN2021
// Types definitions and predeclares for XML parser.

#ifdef _MSC_VER
#include <unordered_set>
#include <unordered_map>
#else
#include <set>
#include <map>
#endif 
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
// A run of the white space characters which are not the space character - tabs and the ends of lines - within the literal
//  value of an attribute. In the value of the attribute each is a space ( 3.3.3 ) - an end of line written as #xD #xA is
//  one ( 2.11 ). The characters themselves are what is in the document: they are what is written when the token is written.
static const vtyDataType s_kdtAttrWhitespace = 5;
// A flag in the type of data of each of the pieces of a value: the value is that of an attribute whose declared type
//  is not CDATA. Then the spaces at the ends of the value are not in it and a sequence of spaces within it is one
//  space ( 3.3.3 ). The cursor sets this when it has the declaration of the attribute. As everything else about the
//  value of an attribute this is so of the string which is had from the value - the pieces are still all of what is in
//  the document.
static const vtyDataType s_kdtfAttrTokenized = 0x80000000;
// The type of data without the flags.
inline constexpr vtyDataType DtGetType( vtyDataType _dt )
{
  return _dt & ~s_kdtfAttrTokenized;
}

// The string containing the values from the Production S.
#define STR_XML_WHITESPACE_TOKEN "\n\r\x20\t"

// Try to propagate this to the current usage areas that merely contain "false" - this flags where we need to change when we support DTDs.
static constexpr bool vkfSupportDTD = false;

// xml_traits.h
template < class t_TyTransport, bool t_kfSupportDTD, bool t_kfValidating >
struct xml_traits;

template < class t_TyChar >
struct xml_markup_traits;

// xml_user.h:
template < class t_tyChar, bool t_kfSupportDTD >
class _xml_user_obj_base_dtd;
template < class t_tyChar, bool t_kfSupportDTD >
class xml_user_obj;

// xml_context.h:
template < class t_TyChar >
class XMLDeclProperties;
class _xml_output_format;
class _xml_output_context;
template < class t_TyChar >
class _xml_token_copy_context;
template < class t_TyLexUserObj >
class _xml_document_context;
template < class t_TyXmlTraits >
class _xml_document_context_transport;
template < class t_TyTpTransports >
class _xml_document_context_transport_var;

// xml_namespace.h
template < class t_TyChar >
class _xml_namespace_uri;
template < class t_TyChar >
class xml_namespace_value_wrap;
template < class t_TyChar >
class xml_namespace_map;

// xml_namespace_tree.h
template < class t_TyChar, class t_TyAllocator = allocator< t_TyChar > >
class xml_namespace_tree_node;

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
template < class t_TyXmlTraits >
class _xml_tag_import_options;
template < class t_TyXmlTraits >
class _xml_tag_import_context;
template < class t_TyXmlTraits >
class _xml_local_import_context;
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

// xml_transport.h:
template < class t_TyChar, class t_TyFSwitchEndian >
class xml_write_transport_file;
template < class t_TyChar, class t_TyFSwitchEndian >
class xml_write_transport_mapped;
template < class t_TyChar, class t_TyFSwitchEndian >
class xml_write_transport_memstream;

// xml_writer.h:
template < class t_TyXmlTransportOut >
class xml_write_context;
template < class t_TyXmlTransportOut >
class xml_write_tag;
template < class t_TyXmlTransportOut >
class xml_writer;

// Implement a mapping from lex read transports to xml output transports - mostly for unit testing.
template < class t_TyLexTransport >
struct map_input_to_output_transport;
template < class t_TyChar, class t_TyBoolSwitchEndian >
struct map_input_to_output_transport< _l_transport_file< t_TyChar, t_TyBoolSwitchEndian > >
{
  typedef xml_write_transport_file< t_TyChar, t_TyBoolSwitchEndian > _TyXmlWriteTransport;
};
template < class t_TyChar, class t_TyBoolSwitchEndian >
struct map_input_to_output_transport< _l_transport_mapped< t_TyChar, t_TyBoolSwitchEndian > >
{
  typedef xml_write_transport_mapped< t_TyChar, t_TyBoolSwitchEndian > _TyXmlWriteTransport;
};
template < class t_TyChar, class t_TyBoolSwitchEndian >
struct map_input_to_output_transport< _l_transport_fixedmem< t_TyChar, t_TyBoolSwitchEndian > >
{
  typedef xml_write_transport_memstream< t_TyChar, t_TyBoolSwitchEndian > _TyXmlWriteTransport;
};
// map_input_to_any_output_transport: Map to any (char,switchendian) combo for output transport type.
template < class t_TyLexTransport, class t_TyChar, class t_TyBoolSwitchEndian >
struct map_input_to_any_output_transport;
template < class t_TyLexInputChar, class t_TyLexInputBoolSwitchEndian, class t_TyChar, class t_TyBoolSwitchEndian >
struct map_input_to_any_output_transport< _l_transport_file< t_TyLexInputChar, t_TyLexInputBoolSwitchEndian >, t_TyChar, t_TyBoolSwitchEndian >
{
  typedef xml_write_transport_file< t_TyChar, t_TyBoolSwitchEndian > _TyXmlWriteTransport;
};
template < class t_TyLexInputChar, class t_TyLexInputBoolSwitchEndian, class t_TyChar, class t_TyBoolSwitchEndian >
struct map_input_to_any_output_transport< _l_transport_mapped< t_TyLexInputChar, t_TyLexInputBoolSwitchEndian >, t_TyChar, t_TyBoolSwitchEndian >
{
  typedef xml_write_transport_mapped< t_TyChar, t_TyBoolSwitchEndian > _TyXmlWriteTransport;
};
template < class t_TyLexInputChar, class t_TyLexInputBoolSwitchEndian, class t_TyChar, class t_TyBoolSwitchEndian >
struct map_input_to_any_output_transport< _l_transport_fixedmem< t_TyLexInputChar, t_TyLexInputBoolSwitchEndian >, t_TyChar, t_TyBoolSwitchEndian >
{
  typedef xml_write_transport_memstream< t_TyChar, t_TyBoolSwitchEndian > _TyXmlWriteTransport;
};

// When we actually support DTD and validation (if ever because they aren't that important to me) then we might have to make this more complex.
// This is an adaptor for use with MultiplexTuplePack_t<>.
template < class t_TyTransport >
using TGetXmlTraitsDefault = xml_traits< t_TyTransport, false, false >;

// TGetPrefixUri (prefix,URI).
template < class t_TyChar >
using TGetPrefixUri = std::pair< basic_string< t_TyChar >, basic_string< t_TyChar > >;

// TFIsReadCursorVar: Test for read cursor for so can overload same function names with constraints.
template < class t_TyReadCursor >
struct TFIsReadCursorVar
{
  static constexpr bool value = false;
};
template < class t_TyTpTransports >
struct TFIsReadCursorVar< xml_read_cursor_var< t_TyTpTransports > >
{
  static constexpr bool value = true;
};
template < class t_TyReadCursor >
inline constexpr bool TFIsReadCursorVar_v = TFIsReadCursorVar< t_TyReadCursor >::value;

// _xml_namespace_map_traits:
// This just organizes the types for the namespace map in one place because we reference it in a couple of places and we don't want to templatize by the full type because it results in unreadable log messages, etc.
template < class t_TyChar >
struct _xml_namespace_map_traits
{
  typedef t_TyChar _TyChar;
  typedef basic_string< _TyChar > _TyStdStr;
  typedef basic_string_view< _TyChar > _TyStrView;
  typedef _xml_namespace_uri< _TyChar > _TyNamespaceUri;
  typedef UniquePtrSList< _TyNamespaceUri > _TyUpListNamespaceUris;
#ifdef _MSC_VER
  typedef StringTransparentHash< _TyChar > _TyStringTransparentHash; // Allow lookup by string_view without creating a string.
  typedef unordered_set< _TyStdStr, _TyStringTransparentHash, std::equal_to<void> > _TyUriAndPrefixMap;
  // first is a pointer to a PrefixMap value for the prefix, second is the current list of namespace URIs for that prefix.
  typedef pair< const typename _TyUriAndPrefixMap::value_type *, _TyUpListNamespaceUris > _TyNamespaceMapped;
  typedef unordered_map< _TyStdStr, _TyNamespaceMapped, _TyStringTransparentHash, std::equal_to<void> > _TyNamespaceMap;
#else // __linux__ or any other unknown system - which should support heterogeneous lookup for set and map:
  // Clang's trunk is currently supporting heterogeneous lookup for set and map so we can flavor this a bit different later (today is 09APR2021).
  typedef set< _TyStdStr, std::less<> > _TyUriAndPrefixMap;
  // first is a pointer to a PrefixMap value for the prefix, second is the current list of namespace URIs for that prefix.
  typedef pair< const typename _TyUriAndPrefixMap::value_type *, _TyUpListNamespaceUris > _TyNamespaceMapped;
  typedef map< _TyStdStr, _TyNamespaceMapped, std::less<> > _TyNamespaceMap;
#endif
};

// These are positions within the _l_value<> aggregate for tags:
static const size_t vknTagNameIdx = 0;
static const size_t vknAttributesIdx = 1;
static const size_t vknTagArrayCount = 2;
// Name positions: Shared between tag and attribute names:
static const size_t vknNameIdx = 0;
static const size_t vknNamespaceIdx = 1;
// Tag-name specific:
static const size_t vknTagName_NNamespaceDeclsIdx = 2;
static const size_t vknTagName_ArrayCount = 3;
// Attribute specific:
static const size_t vknAttr_ValueIdx = 2;
static const size_t vknAttr_FDoubleQuoteIdx = 3;
static const size_t vknAttr_ArrayCount = 4;

// Positions in the _l_value<> aggregate for Processing Instruction.
static const size_t vknProcessingInstruction_PITargetIdx = 0;
static const size_t vknProcessingInstruction_MeatIdx = 1;
static const size_t vknProcessingInstruction_ArrayCount = 2;

// Positions in the _l_value<> aggregate for XMLDecl.
static const size_t vknXMLDecl_StandaloneYesIdx = 0;
static const size_t vknXMLDecl_StandaloneNoIdx = 1;
static const size_t vknXMLDecl_StandaloneDoubleQuoteIdx = 2;
static const size_t vknXMLDecl_EncodingIdx = 3;
static const size_t vknXMLDecl_EncodingDoubleQuoteIdx = 4;
static const size_t vknXMLDecl_EncodingSingleQuoteIdx = 5;
static const size_t vknXMLDecl_VersionMinorNumberIdx = 6;
static const size_t vknXMLDecl_VersionMinorNumberDoubleQuoteIdx = 7;
static const size_t vknXMLDecl_ArrayCount = 8;

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

// A run of tabs and ends of lines within an AttValue [10] is a piece of the value of its own type: s_kdtAttrWhitespace.
// Such are rare so the triggers seldom fire - and a value without any is recorded just as it was without them.
// Where such a run begins is where a run of other characters would: the trigger of the beginning is the same one - a
//  state may have but one trigger leaving it - and which of the triggers of the end is reached says which kind the run was.
static const vtyActionIdent s_knTriggerAttWhitespaceEnd = 48;
static const vtyActionIdent s_knTriggerAttWhitespaceSingleQuoteEnd = 49;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTriggerAttWhitespaceEnd = _l_trigger_string_typed_range< s_kdtAttrWhitespace, TyGetTriggerCharDataEnd<t_TyLexTraits,t_fInLexGen>,
                                                                    s_knTriggerAttWhitespaceEnd, s_knTriggerCharDataBegin >;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTriggerAttWhitespaceSingleQuoteEnd = _l_trigger_string_typed_range< s_kdtAttrWhitespace, TyGetTriggerCharDataEnd<t_TyLexTraits,t_fInLexGen>,
                                                                    s_knTriggerAttWhitespaceSingleQuoteEnd, s_knTriggerCharDataSingleQuoteBegin >;

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
static const vtyActionIdent s_knTriggerStandaloneNo = 44;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTriggerStandaloneNo = _l_trigger_bool< t_TyLexTraits, s_knTriggerStandaloneNo, t_fInLexGen >;
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
static const vtyActionIdent s_knTriggerEncDeclSingleQuote = 45;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTriggerEncDeclSingleQuote = _l_trigger_bool< t_TyLexTraits, s_knTriggerEncDeclSingleQuote, t_fInLexGen >;

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
using TyGetTokenXMLDecl = _l_action_token< _l_action_save_data_single< s_knTokenXMLDecl, 
  TyGetTriggerStandaloneYes<t_TyLexTraits,t_fInLexGen>, TyGetTriggerStandaloneNo<t_TyLexTraits,t_fInLexGen>, TyGetTriggerStandaloneDoubleQuote<t_TyLexTraits,t_fInLexGen>, 
  TyGetTriggerEncodingNameEnd<t_TyLexTraits,t_fInLexGen>, TyGetTriggerEncDeclDoubleQuote<t_TyLexTraits,t_fInLexGen>, TyGetTriggerEncDeclSingleQuote<t_TyLexTraits,t_fInLexGen>, 
  TyGetTriggerVersionNumEnd<t_TyLexTraits,t_fInLexGen>, TyGetTriggerVersionNumDoubleQuote<t_TyLexTraits,t_fInLexGen> > >;

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


// The DTD: the tokens of the document type declaration and of the internal and external subsets - [28] through [83].
// These are the terminals of the grammar of the DTD ( xmlpgen_dtd.h ): the DTD is parsed by a parser generated by pgen
//  from productions which are those of the specification - unlike the tags, each of which is a single token.
// So the tokens are small - a name, a keyword, a bracket - and most have no data other than the characters they matched.
// Some of them are the same characters: "ID" is a keyword and a Name and an Nmtoken, a quoted string is an EntityValue,
//  an AttValue, a SystemLiteral and a PubidLiteral. Such tokens are in different DFAs of the lexical analyzer - its "modes" -
//  and the parser, which knows which of them can occur where it is, chooses the DFA(s) for each token.

// A parameter entity reference within an EntityValue [9]: its name is recorded with the rest of the value - as the names of
//  general entity references are within an AttValue.
static const vtyActionIdent s_knTriggerPEReferenceValueBegin = 46;
static const vtyActionIdent s_knTriggerPEReferenceValueEnd = 47;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTriggerPEReferenceValueBegin = _l_trigger_string_typed_beginpoint< s_kdtPEReference, TyGetTriggerCharDataEnd<t_TyLexTraits,t_fInLexGen>, s_knTriggerPEReferenceValueBegin >;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTriggerPEReferenceValueEnd = _l_trigger_string_typed_endpoint< s_kdtPEReference, TyGetTriggerCharDataEnd<t_TyLexTraits,t_fInLexGen>, s_knTriggerPEReferenceValueEnd, s_knTriggerPEReferenceValueBegin >;

// The tokens which are just the characters they matched:
static const vtyActionIdent s_knTokenDtdS = 4000; // S, [3]. In the DTD white space is a terminal of the grammar: where it is required and where it may not be is part of the grammar.
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenDtdS = _l_action_token< _l_trigger_noop< t_TyLexTraits, s_knTokenDtdS, t_fInLexGen > >;
static const vtyActionIdent s_knTokenDtdName = 4001; // Name, [5].
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenDtdName = _l_action_token< _l_trigger_noop< t_TyLexTraits, s_knTokenDtdName, t_fInLexGen > >;
static const vtyActionIdent s_knTokenDtdNmtoken = 4002; // Nmtoken, [7].
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenDtdNmtoken = _l_action_token< _l_trigger_noop< t_TyLexTraits, s_knTokenDtdNmtoken, t_fInLexGen > >;
static const vtyActionIdent s_knTokenDtdDoctypeBegin = 4003; // '<!DOCTYPE'
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenDtdDoctypeBegin = _l_action_token< _l_trigger_noop< t_TyLexTraits, s_knTokenDtdDoctypeBegin, t_fInLexGen > >;
static const vtyActionIdent s_knTokenDtdElementBegin = 4004; // '<!ELEMENT'
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenDtdElementBegin = _l_action_token< _l_trigger_noop< t_TyLexTraits, s_knTokenDtdElementBegin, t_fInLexGen > >;
static const vtyActionIdent s_knTokenDtdAttlistBegin = 4005; // '<!ATTLIST'
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenDtdAttlistBegin = _l_action_token< _l_trigger_noop< t_TyLexTraits, s_knTokenDtdAttlistBegin, t_fInLexGen > >;
static const vtyActionIdent s_knTokenDtdEntityBegin = 4006; // '<!ENTITY'
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenDtdEntityBegin = _l_action_token< _l_trigger_noop< t_TyLexTraits, s_knTokenDtdEntityBegin, t_fInLexGen > >;
static const vtyActionIdent s_knTokenDtdNotationBegin = 4007; // '<!NOTATION'
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenDtdNotationBegin = _l_action_token< _l_trigger_noop< t_TyLexTraits, s_knTokenDtdNotationBegin, t_fInLexGen > >;
static const vtyActionIdent s_knTokenDtdLBracket = 4008; // '['
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenDtdLBracket = _l_action_token< _l_trigger_noop< t_TyLexTraits, s_knTokenDtdLBracket, t_fInLexGen > >;
static const vtyActionIdent s_knTokenDtdRBracket = 4009; // ']'
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenDtdRBracket = _l_action_token< _l_trigger_noop< t_TyLexTraits, s_knTokenDtdRBracket, t_fInLexGen > >;
static const vtyActionIdent s_knTokenDtdGreater = 4010; // '>'
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenDtdGreater = _l_action_token< _l_trigger_noop< t_TyLexTraits, s_knTokenDtdGreater, t_fInLexGen > >;
static const vtyActionIdent s_knTokenDtdLParen = 4011; // '('
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenDtdLParen = _l_action_token< _l_trigger_noop< t_TyLexTraits, s_knTokenDtdLParen, t_fInLexGen > >;
static const vtyActionIdent s_knTokenDtdRParen = 4012; // ')'
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenDtdRParen = _l_action_token< _l_trigger_noop< t_TyLexTraits, s_knTokenDtdRParen, t_fInLexGen > >;
static const vtyActionIdent s_knTokenDtdBar = 4013; // '|'
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenDtdBar = _l_action_token< _l_trigger_noop< t_TyLexTraits, s_knTokenDtdBar, t_fInLexGen > >;
static const vtyActionIdent s_knTokenDtdComma = 4014; // ','
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenDtdComma = _l_action_token< _l_trigger_noop< t_TyLexTraits, s_knTokenDtdComma, t_fInLexGen > >;
static const vtyActionIdent s_knTokenDtdQuestion = 4015; // '?'
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenDtdQuestion = _l_action_token< _l_trigger_noop< t_TyLexTraits, s_knTokenDtdQuestion, t_fInLexGen > >;
static const vtyActionIdent s_knTokenDtdStar = 4016; // '*'
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenDtdStar = _l_action_token< _l_trigger_noop< t_TyLexTraits, s_knTokenDtdStar, t_fInLexGen > >;
static const vtyActionIdent s_knTokenDtdPlus = 4017; // '+'
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenDtdPlus = _l_action_token< _l_trigger_noop< t_TyLexTraits, s_knTokenDtdPlus, t_fInLexGen > >;
static const vtyActionIdent s_knTokenDtdPercent = 4018; // '%' - of a parameter entity declaration [72]: a '%' which doesn't begin a PEReference.
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenDtdPercent = _l_action_token< _l_trigger_noop< t_TyLexTraits, s_knTokenDtdPercent, t_fInLexGen > >;
static const vtyActionIdent s_knTokenDtdPCDATA = 4019; // '#PCDATA'
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenDtdPCDATA = _l_action_token< _l_trigger_noop< t_TyLexTraits, s_knTokenDtdPCDATA, t_fInLexGen > >;
static const vtyActionIdent s_knTokenDtdEMPTY = 4020; // 'EMPTY'
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenDtdEMPTY = _l_action_token< _l_trigger_noop< t_TyLexTraits, s_knTokenDtdEMPTY, t_fInLexGen > >;
static const vtyActionIdent s_knTokenDtdANY = 4021; // 'ANY'
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenDtdANY = _l_action_token< _l_trigger_noop< t_TyLexTraits, s_knTokenDtdANY, t_fInLexGen > >;
static const vtyActionIdent s_knTokenDtdCDATA = 4022; // 'CDATA'
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenDtdCDATA = _l_action_token< _l_trigger_noop< t_TyLexTraits, s_knTokenDtdCDATA, t_fInLexGen > >;
static const vtyActionIdent s_knTokenDtdID = 4023; // 'ID'
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenDtdID = _l_action_token< _l_trigger_noop< t_TyLexTraits, s_knTokenDtdID, t_fInLexGen > >;
static const vtyActionIdent s_knTokenDtdIDREF = 4024; // 'IDREF'
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenDtdIDREF = _l_action_token< _l_trigger_noop< t_TyLexTraits, s_knTokenDtdIDREF, t_fInLexGen > >;
static const vtyActionIdent s_knTokenDtdIDREFS = 4025; // 'IDREFS'
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenDtdIDREFS = _l_action_token< _l_trigger_noop< t_TyLexTraits, s_knTokenDtdIDREFS, t_fInLexGen > >;
static const vtyActionIdent s_knTokenDtdENTITY = 4026; // 'ENTITY' - the attribute type [56], not '<!ENTITY'.
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenDtdENTITY = _l_action_token< _l_trigger_noop< t_TyLexTraits, s_knTokenDtdENTITY, t_fInLexGen > >;
static const vtyActionIdent s_knTokenDtdENTITIES = 4027; // 'ENTITIES'
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenDtdENTITIES = _l_action_token< _l_trigger_noop< t_TyLexTraits, s_knTokenDtdENTITIES, t_fInLexGen > >;
static const vtyActionIdent s_knTokenDtdNMTOKEN = 4028; // 'NMTOKEN' - the attribute type [56], not an Nmtoken.
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenDtdNMTOKEN = _l_action_token< _l_trigger_noop< t_TyLexTraits, s_knTokenDtdNMTOKEN, t_fInLexGen > >;
static const vtyActionIdent s_knTokenDtdNMTOKENS = 4029; // 'NMTOKENS'
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenDtdNMTOKENS = _l_action_token< _l_trigger_noop< t_TyLexTraits, s_knTokenDtdNMTOKENS, t_fInLexGen > >;
static const vtyActionIdent s_knTokenDtdNOTATION = 4030; // 'NOTATION' - the attribute type [58], not '<!NOTATION'.
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenDtdNOTATION = _l_action_token< _l_trigger_noop< t_TyLexTraits, s_knTokenDtdNOTATION, t_fInLexGen > >;
static const vtyActionIdent s_knTokenDtdREQUIRED = 4031; // '#REQUIRED'
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenDtdREQUIRED = _l_action_token< _l_trigger_noop< t_TyLexTraits, s_knTokenDtdREQUIRED, t_fInLexGen > >;
static const vtyActionIdent s_knTokenDtdIMPLIED = 4032; // '#IMPLIED'
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenDtdIMPLIED = _l_action_token< _l_trigger_noop< t_TyLexTraits, s_knTokenDtdIMPLIED, t_fInLexGen > >;
static const vtyActionIdent s_knTokenDtdFIXED = 4033; // '#FIXED'
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenDtdFIXED = _l_action_token< _l_trigger_noop< t_TyLexTraits, s_knTokenDtdFIXED, t_fInLexGen > >;
static const vtyActionIdent s_knTokenDtdSYSTEM = 4034; // 'SYSTEM'
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenDtdSYSTEM = _l_action_token< _l_trigger_noop< t_TyLexTraits, s_knTokenDtdSYSTEM, t_fInLexGen > >;
static const vtyActionIdent s_knTokenDtdPUBLIC = 4035; // 'PUBLIC'
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenDtdPUBLIC = _l_action_token< _l_trigger_noop< t_TyLexTraits, s_knTokenDtdPUBLIC, t_fInLexGen > >;
static const vtyActionIdent s_knTokenDtdNDATA = 4036; // 'NDATA'
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenDtdNDATA = _l_action_token< _l_trigger_noop< t_TyLexTraits, s_knTokenDtdNDATA, t_fInLexGen > >;
static const vtyActionIdent s_knTokenDtdCondSectBegin = 4042; // '<![' - of a conditional section [62], [63].
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenDtdCondSectBegin = _l_action_token< _l_trigger_noop< t_TyLexTraits, s_knTokenDtdCondSectBegin, t_fInLexGen > >;
static const vtyActionIdent s_knTokenDtdCondSectEnd = 4043; // ']]>'
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenDtdCondSectEnd = _l_action_token< _l_trigger_noop< t_TyLexTraits, s_knTokenDtdCondSectEnd, t_fInLexGen > >;
static const vtyActionIdent s_knTokenDtdINCLUDE = 4044; // 'INCLUDE'
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenDtdINCLUDE = _l_action_token< _l_trigger_noop< t_TyLexTraits, s_knTokenDtdINCLUDE, t_fInLexGen > >;
static const vtyActionIdent s_knTokenDtdIGNORE = 4045; // 'IGNORE'
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenDtdIGNORE = _l_action_token< _l_trigger_noop< t_TyLexTraits, s_knTokenDtdIGNORE, t_fInLexGen > >;
static const vtyActionIdent s_knTokenDtdIgnore = 4046; // A piece of the contents of an ignored conditional section [65] - see the generators.
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenDtdIgnore = _l_action_token< _l_trigger_noop< t_TyLexTraits, s_knTokenDtdIgnore, t_fInLexGen > >;

// The tokens with data - recorded by triggers:
// EntityValue [9] and the AttValue [10] of the default of an attribute [60]: the pieces of the value - text and references -
//  and whether it was in double quotes. The same data as the value of an attribute of a tag.
static const vtyActionIdent s_knTokenDtdEntityValue = 4037;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenDtdEntityValue = _l_action_token< _l_action_save_data_single< s_knTokenDtdEntityValue, TyGetTriggerCharDataEnd<t_TyLexTraits,t_fInLexGen>, TyGetTriggerAttValueDoubleQuote<t_TyLexTraits,t_fInLexGen> > >;
static const vtyActionIdent s_knTokenDtdAttValue = 4038;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenDtdAttValue = _l_action_token< _l_action_save_data_single< s_knTokenDtdAttValue, TyGetTriggerCharDataEnd<t_TyLexTraits,t_fInLexGen>, TyGetTriggerAttValueDoubleQuote<t_TyLexTraits,t_fInLexGen> > >;
// SystemLiteral [11] and PubidLiteral [12]: what is between the quotes.
static const vtyActionIdent s_knTokenDtdSystemLiteral = 4039;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenDtdSystemLiteral = _l_action_token< _l_action_save_data_single< s_knTokenDtdSystemLiteral, TyGetTriggerSystemLiteralDoubleQuoteEnd<t_TyLexTraits,t_fInLexGen> > >;
static const vtyActionIdent s_knTokenDtdPubidLiteral = 4040;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenDtdPubidLiteral = _l_action_token< _l_action_save_data_single< s_knTokenDtdPubidLiteral, TyGetTriggerPubidLiteralDoubleQuoteEnd<t_TyLexTraits,t_fInLexGen> > >;
// PEReference [69] between declarations. Just the characters: the name is what is between the '%' and the ';'. ( It can't
//  have triggers for the name: a '%' alone is a token of the same DFA [72] and a trigger may not leave an accepting state. )
static const vtyActionIdent s_knTokenDtdPEReference = 4041;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenDtdPEReference = _l_action_token< _l_trigger_noop< t_TyLexTraits, s_knTokenDtdPEReference, t_fInLexGen > >;
// TextDecl [77] - of an external subset or an external parameter entity: the encoding and the version as for XMLDecl.
static const vtyActionIdent s_knTokenDtdTextDecl = 4047;
template < class t_TyLexTraits, bool t_fInLexGen = true >
using TyGetTokenDtdTextDecl = _l_action_token< _l_action_save_data_single< s_knTokenDtdTextDecl,
  TyGetTriggerEncodingNameEnd<t_TyLexTraits,t_fInLexGen>, TyGetTriggerEncDeclDoubleQuote<t_TyLexTraits,t_fInLexGen>, TyGetTriggerEncDeclSingleQuote<t_TyLexTraits,t_fInLexGen>,
  TyGetTriggerVersionNumEnd<t_TyLexTraits,t_fInLexGen>, TyGetTriggerVersionNumDoubleQuote<t_TyLexTraits,t_fInLexGen> > >;
// A comment and a processing instruction within the DTD are the tokens they are everywhere else: s_knTokenComment and s_knTokenProcessingInstruction.

// Positions in the _l_value<> aggregates of the tokens of the DTD which have data.
static const size_t vknDtdValue_ValueIdx = 0; // EntityValue, AttValue.
static const size_t vknDtdValue_FDoubleQuoteIdx = 1;
static const size_t vknDtdValue_ArrayCount = 2;
static const size_t vknDtdTextDecl_EncodingIdx = 0;
static const size_t vknDtdTextDecl_EncodingDoubleQuoteIdx = 1;
static const size_t vknDtdTextDecl_EncodingSingleQuoteIdx = 2;
static const size_t vknDtdTextDecl_VersionMinorNumberIdx = 3;
static const size_t vknDtdTextDecl_VersionMinorNumberDoubleQuoteIdx = 4;
static const size_t vknDtdTextDecl_ArrayCount = 5;

__XMLP_END_NAMESPACE
