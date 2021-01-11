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
static const vtyDataType s_kdtCharDecRef = 1;
static const vtyDataType s_kdtCharHexRef = 2;
static const vtyDataType s_kdtFirstDTDRef = 3;
static const vtyDataType s_kdtEntityRef = 3;
static const vtyDataType s_kdtPEReference = 4;

// xml_user.h:
template < class t_TyChar >
class _xml_namespace_uri;
template < class t_TyXmlTraits, bool t_kfSupportNamespaces >
class _xml_user_obj_base_namespace;
template < class t_TyXmlTraits, bool t_kfSupportDTD >
class _xml_user_obj_base_dtd;
template < class t_TyXmlTraits >
class xml_user_obj;

// Declare all the various types of the triggers and tokens for the XML lexical analyzer.
static const vtyActionIdent s_knTriggerPITargetStart = 1;
static const vtyActionIdent s_knTriggerPITargetEnd = 2;
template < class t_TyLexTraits >
using TyGetTriggerPITargetStart = _l_trigger_position< t_TyLexTraits, s_knTriggerPITargetStart >;
template < class t_TyLexTraits >
using TyGetTriggerPITargetStart = _l_trigger_string< t_TyLexTraits, s_knTriggerPITargetEnd, s_knTriggerPITargetStart >;
template < class t_TyLexTraits >
using TyGetTriggerPITargetEnd = _l_trigger_string< t_TyLexTraits, s_knTriggerPITargetEnd, s_knTriggerPITargetStart >;
static const vtyActionIdent s_knTriggerPITargetMeatBegin = 3;
static const vtyActionIdent s_knTriggerPITargetMeatEnd = 4;
template < class t_TyLexTraits >
using TyGetTriggerPITargetMeatBegin = _l_trigger_position< t_TyLexTraits, s_knTriggerPITargetMeatBegin >;
template < class t_TyLexTraits >
using TyGetTriggerPITargetMeatEnd = _l_trigger_string< t_TyLexTraits, s_knTriggerPITargetMeatEnd, s_knTriggerPITargetMeatBegin >;

static const vtyActionIdent s_knTriggerMixedNameBegin = 5;
static const vtyActionIdent s_knTriggerMixedNameEnd = 6;
template < class t_TyLexTraits >
using TyGetTriggerMixedNameBegin = _l_trigger_position< t_TyLexTraits, s_knTriggerMixedNameBegin >;
template < class t_TyLexTraits >
using TyGetTriggerMixedNameEnd = _l_trigger_string< t_TyLexTraits, s_knTriggerMixedNameEnd, s_knTriggerMixedNameBegin >;

// For now we will lump together everything that is a subset of CHARDATA (i.e. attr values in which can be no ' or ", etc)
//	into the same triggers and resultant token data. This should be perfectly fine during input of XML - also
//	it saves us implementation space.
static const vtyActionIdent s_knTriggerCharDataBegin = 7;
static const vtyActionIdent s_knTriggerCharDataEnd = 8;
template < class t_TyLexTraits >
using TyGetTriggerCharDataBegin = _l_trigger_position< t_TyLexTraits, s_knTriggerCharDataBegin >;
template < class t_TyLexTraits >
using TyGetTriggerCharDataEnd = _l_trigger_string< t_TyLexTraits, s_knTriggerCharDataEnd, s_knTriggerCharDataBegin >;

// We need to have a different set of triggers for the single-quoted CharData since the actions become ambiguous with each other when the are the same - which makes sense.
static const vtyActionIdent s_knTriggerCharDataSingleQuoteBegin = 9;
static const vtyActionIdent s_knTriggerCharDataSingleQuoteEnd = 10;
template < class t_TyLexTraits >
using TyGetTriggerCharDataSingleQuoteBegin = _l_trigger_position< t_TyLexTraits, s_knTriggerCharDataSingleQuoteBegin >;
template < class t_TyLexTraits >
using TyGetTriggerCharDataSingleQuoteEnd = _l_trigger_string_typed_range< s_kdtPlainText, TyGetTriggerCharDataEnd<t_TyLexTraits>, s_knTriggerCharDataSingleQuoteEnd, s_knTriggerCharDataSingleQuoteBegin >;

static const vtyActionIdent s_knTriggerEntityRefBegin = 11;
static const vtyActionIdent s_knTriggerEntityRefEnd = 12;
template < class t_TyLexTraits >
using TyGetTriggerEntityRefBegin = _l_trigger_position< t_TyLexTraits, s_knTriggerEntityRefBegin >;
template < class t_TyLexTraits >
using TyGetTriggerEntityRefEnd = _l_trigger_string_typed_range< s_kdtEntityRef, TyGetTriggerCharDataEnd<t_TyLexTraits>, s_knTriggerEntityRefEnd, s_knTriggerEntityRefBegin >;

static const vtyActionIdent s_knTriggerCharDecRefBegin = 13;
static const vtyActionIdent s_knTriggerCharDecRefEnd = 14;
template < class t_TyLexTraits >
using TyGetTriggerCharDecRefBegin = _l_trigger_position< t_TyLexTraits, s_knTriggerCharDecRefBegin >;
template < class t_TyLexTraits >
using TyGetTriggerCharDecRefEnd = _l_trigger_string_typed_range< s_kdtCharDecRef, TyGetTriggerCharDataEnd<t_TyLexTraits>, s_knTriggerCharDecRefEnd, s_knTriggerCharDecRefBegin >;

static const vtyActionIdent s_knTriggerCharHexRefBegin = 15;
static const vtyActionIdent s_knTriggerCharHexRefEnd = 16;
template < class t_TyLexTraits >
using TyGetTriggerCharHexRefBegin = _l_trigger_position< t_TyLexTraits, s_knTriggerCharHexRefBegin >;
template < class t_TyLexTraits >
using TyGetTriggerCharHexRefEnd = _l_trigger_string_typed_range< s_kdtCharHexRef, TyGetTriggerCharDataEnd<t_TyLexTraits>, s_knTriggerCharHexRefEnd, s_knTriggerCharHexRefBegin >;

// Not necessary because we can look at the trigger id within the token's constituent _l_data_typed_range objects.
// static const vtyActionIdent s_knTriggerAttValueDoubleQuote = 17;
// TyGetTriggerAttValueDoubleQuote _l_trigger_bool< t_TyLexTraits, s_knTriggerAttValueDoubleQuote >;

static const vtyActionIdent s_knTriggerPrefixBegin = 18;
static const vtyActionIdent s_knTriggerPrefixEnd = 19;
template < class t_TyLexTraits >
using TyGetTriggerPrefixBegin = _l_trigger_position< t_TyLexTraits, s_knTriggerPrefixBegin >;
template < class t_TyLexTraits >
using TyGetTriggerPrefixEnd = _l_trigger_string< t_TyLexTraits, s_knTriggerPrefixEnd, s_knTriggerPrefixBegin >;

static const vtyActionIdent s_knTriggerLocalPartBegin = 20;
static const vtyActionIdent s_knTriggerLocalPartEnd = 21;
template < class t_TyLexTraits >
using TyGetTriggerLocalPartBegin = _l_trigger_position< t_TyLexTraits, s_knTriggerLocalPartBegin >;
template < class t_TyLexTraits >
using TyGetTriggerLocalPartEnd = _l_trigger_string< t_TyLexTraits, s_knTriggerLocalPartEnd, s_knTriggerLocalPartBegin >;

#if 0 // This trigger is ambiguous due to the nature of attribute names and xmlns attribute names - they match each other - so this trigger fires all the time erroneously.
static const vtyActionIdent s_knTriggerDefaultAttName = 20;
template < class t_TyLexTraits >
using TyGetTriggerDefaultAttName _l_trigger_bool< t_TyLexTraits, s_knTriggerDefaultAttName >;
#endif //0

#if 0 // These triggers also cause ambiguity since PrefixedAttName matches a QName.
static const vtyActionIdent s_knTriggerPrefixedAttNameBegin = 21;
static const vtyActionIdent s_knTriggerPrefixedAttNameEnd = 22;
template < class t_TyLexTraits >
using TyGetTriggerPrefixedAttNameBegin = _l_trigger_position< t_TyLexTraits, s_knTriggerPrefixedAttNameBegin >;
template < class t_TyLexTraits >
using TyGetTriggerPrefixedAttNameEnd = _l_trigger_string< t_TyLexTraits, s_knTriggerPrefixedAttNameEnd, s_knTriggerPrefixedAttNameBegin >;
#endif //0

static const vtyActionIdent s_knTriggerSaveTagName = 23;
template < class t_TyLexTraits >
using TyGetTriggerSaveTagName = _l_action_save_data_single< s_knTriggerSaveTagName, true, TyGetTriggerPrefixEnd<t_TyLexTraits>, TyGetTriggerLocalPartEnd<t_TyLexTraits> >;

static const vtyActionIdent s_knTriggerSaveAttributes = 24;
template < class t_TyLexTraits >
using TyGetTriggerSaveAttributes = _l_action_save_data_multiple< s_knTriggerSaveAttributes, true, 
  TyGetTriggerPrefixEnd<t_TyLexTraits>, TyGetTriggerLocalPartEnd<t_TyLexTraits>, TyGetTriggerCharDataEnd<t_TyLexTraits> >;

#if 0 // The comment production has a problem with triggers due to its nature. It is easy to manage without triggers - impossible to try to stick triggers in - they'll never work.
static const vtyActionIdent s_knTriggerCommentBegin = 25;
static const vtyActionIdent s_knTriggerCommentEnd = 26;
template < class t_TyLexTraits >
using TyGetTriggerCommentBegin = _l_trigger_position< t_TyLexTraits, s_knTriggerCommentBegin >;
template < class t_TyLexTraits >
using TyGetTriggerCommentEnd = _l_trigger_string< t_TyLexTraits, s_knTriggerCommentEnd, s_knTriggerCommentBegin >;
#endif //0

static const vtyActionIdent s_knTriggerStandaloneYes = 27;
template < class t_TyLexTraits >
using TyGetTriggerStandaloneYes = _l_trigger_bool< t_TyLexTraits, s_knTriggerStandaloneYes >;
static const vtyActionIdent s_knTriggerStandaloneDoubleQuote = 28;
template < class t_TyLexTraits >
using TyGetTriggerStandaloneDoubleQuote = _l_trigger_bool< t_TyLexTraits, s_knTriggerStandaloneDoubleQuote >;

static const vtyActionIdent s_knTriggerEncodingNameBegin = 29;
static const vtyActionIdent s_knTriggerEncodingNameEnd = 30;
template < class t_TyLexTraits >
using TyGetTriggerEncodingNameBegin = _l_trigger_position< t_TyLexTraits, s_knTriggerEncodingNameBegin >;
template < class t_TyLexTraits >
using TyGetTriggerEncodingNameEnd = _l_trigger_string< t_TyLexTraits, s_knTriggerEncodingNameEnd, s_knTriggerEncodingNameBegin >;
static const vtyActionIdent s_knTriggerEncDeclDoubleQuote = 31;
template < class t_TyLexTraits >
using TyGetTriggerEncDeclDoubleQuote = _l_trigger_bool< t_TyLexTraits, s_knTriggerEncDeclDoubleQuote >;

static const vtyActionIdent s_knTriggerVersionNumBegin = 32;
static const vtyActionIdent s_knTriggerVersionNumEnd = 33;
template < class t_TyLexTraits >
using TyGetTriggerVersionNumBegin = _l_trigger_position< t_TyLexTraits, s_knTriggerVersionNumBegin >;
template < class t_TyLexTraits >
using TyGetTriggerVersionNumEnd = _l_trigger_string< t_TyLexTraits, s_knTriggerVersionNumEnd, s_knTriggerVersionNumBegin >;
static const vtyActionIdent s_knTriggerVersionNumDoubleQuote = 34;
template < class t_TyLexTraits >
using TyGetTriggerVersionNumDoubleQuote = _l_trigger_bool< t_TyLexTraits, s_knTriggerVersionNumDoubleQuote >;

static const vtyActionIdent s_knTriggerPEReferenceBegin = 35;
static const vtyActionIdent s_knTriggerPEReferenceEnd = 36;
template < class t_TyLexTraits >
using TyGetTriggerPEReferenceBegin = _l_trigger_position< t_TyLexTraits, s_knTriggerPEReferenceBegin >;
template < class t_TyLexTraits >
using TyGetTriggerPEReferenceEnd = _l_trigger_string< t_TyLexTraits, s_knTriggerPEReferenceEnd, s_knTriggerPEReferenceBegin >;

static const vtyActionIdent s_knTriggerSystemLiteralBegin = 37;
static const vtyActionIdent s_knTriggerSystemLiteralDoubleQuoteEnd = 38;
static const vtyActionIdent s_knTriggerSystemLiteralSingleQuoteEnd = 39;
template < class t_TyLexTraits >
using TyGetTriggerSystemLiteralBegin = _l_trigger_position< t_TyLexTraits, s_knTriggerSystemLiteralBegin >;
template < class t_TyLexTraits >
using TyGetTriggerSystemLiteralDoubleQuoteEnd = _l_trigger_string< t_TyLexTraits, s_knTriggerSystemLiteralDoubleQuoteEnd, s_knTriggerSystemLiteralBegin >;
template < class t_TyLexTraits >
using TyGetTriggerSystemLiteralSingleQuoteEnd = _l_trigger_string_typed_range< s_kdtPlainText, TyGetTriggerSystemLiteralDoubleQuoteEnd<t_TyLexTraits>, s_knTriggerSystemLiteralSingleQuoteEnd, s_knTriggerSystemLiteralBegin >;

static const vtyActionIdent s_knTriggerPubidLiteralBegin = 40;
static const vtyActionIdent s_knTriggerPubidLiteralDoubleQuoteEnd = 41;
static const vtyActionIdent s_knTriggerPubidLiteralSingleQuoteEnd = 42;
template < class t_TyLexTraits >
using TyGetTriggerPubidLiteralBegin = _l_trigger_position< t_TyLexTraits, s_knTriggerPubidLiteralBegin >;
template < class t_TyLexTraits >
using TyGetTriggerPubidLiteralDoubleQuoteEnd = _l_trigger_string< t_TyLexTraits, s_knTriggerPubidLiteralDoubleQuoteEnd, s_knTriggerPubidLiteralBegin >;
template < class t_TyLexTraits >
using TyGetTriggerPubidLiteralSingleQuoteEnd = _l_trigger_string_typed_range< s_kdtPlainText, TyGetTriggerPubidLiteralDoubleQuoteEnd<t_TyLexTraits>, s_knTriggerPubidLiteralSingleQuoteEnd, s_knTriggerPubidLiteralBegin >;

// Tokens:
static const vtyActionIdent s_knTokenSTag = 1000;
template < class t_TyLexTraits >
using TyGetTokenSTag = _l_action_token< _l_action_save_data_single< s_knTokenSTag, true, TyGetTriggerSaveTagName<t_TyLexTraits>, TyGetTriggerSaveAttributes<t_TyLexTraits> > >;

static const vtyActionIdent s_knTokenETag = 1001;
template < class t_TyLexTraits >
using TyGetTokenETag = _l_action_token< _l_action_save_data_single< s_knTokenETag, true, TyGetTriggerPrefixEnd<t_TyLexTraits>, TyGetTriggerLocalPartEnd<t_TyLexTraits> > >;

static const vtyActionIdent s_knTokenEmptyElemTag = 1002;
template < class t_TyLexTraits >
using TyGetTokenEmptyElemTag = _l_action_token< _l_action_save_data_single< s_knTokenEmptyElemTag, true, TyGetTriggerSaveTagName<t_TyLexTraits>, TyGetTriggerSaveAttributes<t_TyLexTraits> > >;

static const vtyActionIdent s_knTokenComment = 1003;
template < class t_TyLexTraits >
using TyGetTokenComment = _l_action_token< _l_trigger_noop< t_TyLexTraits, s_knTokenComment, true > >;

static const vtyActionIdent s_knTokenXMLDecl = 1004;
template < class t_TyLexTraits >
using TyGetTokenXMLDecl = _l_action_token< _l_action_save_data_single< s_knTokenXMLDecl, true, TyGetTriggerStandaloneYes<t_TyLexTraits>, TyGetTriggerStandaloneYes<t_TyLexTraits>, 
  TyGetTriggerEncodingNameEnd<t_TyLexTraits>, TyGetTriggerEncDeclDoubleQuote<t_TyLexTraits>, TyGetTriggerVersionNumEnd<t_TyLexTraits>, TyGetTriggerVersionNumDoubleQuote<t_TyLexTraits> > >;

static const vtyActionIdent s_knTokenCDataSection = 1004;
template < class t_TyLexTraits >
using TyGetTokenCDataSection _l_action_token< _l_trigger_noop< t_TyLexTraits, s_knTokenCDataSection, true > >;

__XMLP_END_NAMESPACE
