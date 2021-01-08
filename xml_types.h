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

template < class t_TyChar, bool t_kfSupportDTD >
class _xml_user_obj_base;
template < class t_TyChar, bool t_kfSupportDTD = false >
class xml_user_obj;

// Declare all the various types of the triggers and tokens for the XML lexical analyzer.
static const vtyActionIdent s_knTriggerPITargetStart = 1;
static const vtyActionIdent s_knTriggerPITargetEnd = 2;
template < class t_TyChar >
using TyGetTriggerPITargetStart = _l_trigger_position< t_TyChar, s_knTriggerPITargetStart >;
template < class t_TyChar >
using TyGetTriggerPITargetStart = _l_trigger_string< t_TyChar, s_knTriggerPITargetEnd, s_knTriggerPITargetStart >;
template < class t_tyChar >
using TyGetTriggerPITargetEnd = _l_trigger_string< t_TyChar, s_knTriggerPITargetEnd, s_knTriggerPITargetStart >;
static const vtyActionIdent s_knTriggerPITargetMeatBegin = 3;
static const vtyActionIdent s_knTriggerPITargetMeatEnd = 4;
template < class t_tyChar >
using TyGetTriggerPITargetMeatBegin = _l_trigger_position< t_TyChar, s_knTriggerPITargetMeatBegin >;
template < class t_tyChar >
using TyGetTriggerPITargetMeatEnd = _l_trigger_string< t_TyChar, s_knTriggerPITargetMeatEnd, s_knTriggerPITargetMeatBegin >;

static const vtyActionIdent s_knTriggerMixedNameBegin = 5;
static const vtyActionIdent s_knTriggerMixedNameEnd = 6;
template < class t_tyChar >
using TyGetTriggerMixedNameBegin = _l_trigger_position< t_TyChar, s_knTriggerMixedNameBegin >;
template < class t_tyChar >
using TyGetTriggerMixedNameEnd = _l_trigger_string< t_TyChar, s_knTriggerMixedNameEnd, s_knTriggerMixedNameBegin >;

// For now we will lump together everything that is a subset of CHARDATA (i.e. attr values in which can be no ' or ", etc)
//	into the same triggers and resultant token data. This should be perfectly fine during input of XML - also
//	it saves us implementation space.
static const vtyActionIdent s_knTriggerCharDataBegin = 7;
static const vtyActionIdent s_knTriggerCharDataEnd = 8;
template < class t_tyChar >
using TyGetTriggerCharDataBegin = _l_trigger_position< t_TyChar, s_knTriggerCharDataBegin >;
template < class t_tyChar >
using TyGetTriggerCharDataEnd = _l_trigger_string< t_TyChar, s_knTriggerCharDataEnd, s_knTriggerCharDataBegin >;

// We need to have a different set of triggers for the single-quoted CharData since the actions become ambiguous with each other when the are the same - which makes sense.
static const vtyActionIdent s_knTriggerCharDataSingleQuoteBegin = 9;
static const vtyActionIdent s_knTriggerCharDataSingleQuoteEnd = 10;
template < class t_tyChar >
using TyGetTriggerCharDataSingleQuoteBegin = _l_trigger_position< t_TyChar, s_knTriggerCharDataSingleQuoteBegin >;
template < class t_tyChar >
using TyGetTriggerCharDataSingleQuoteEnd = _l_trigger_string_typed_range< s_kdtPlainText, TyGetTriggerCharDataEnd<t_TyChar>, s_knTriggerCharDataSingleQuoteEnd, s_knTriggerCharDataSingleQuoteBegin >;

static const vtyActionIdent s_knTriggerEntityRefBegin = 11;
static const vtyActionIdent s_knTriggerEntityRefEnd = 12;
template < class t_tyChar >
using TyGetTriggerEntityRefBegin = _l_trigger_position< t_TyChar, s_knTriggerEntityRefBegin >;
template < class t_tyChar >
using TyGetTriggerEntityRefEnd = _l_trigger_string_typed_range< s_kdtEntityRef, TyGetTriggerCharDataEnd<t_TyChar>, s_knTriggerEntityRefEnd, s_knTriggerEntityRefBegin >;

static const vtyActionIdent s_knTriggerCharDecRefBegin = 13;
static const vtyActionIdent s_knTriggerCharDecRefEnd = 14;
template < class t_tyChar >
using TyGetTriggerCharDecRefBegin = _l_trigger_position< t_TyChar, s_knTriggerCharDecRefBegin >;
template < class t_tyChar >
using TyGetTriggerCharDecRefEnd = _l_trigger_string_typed_range< s_kdtCharDecRef, TyGetTriggerCharDataEnd<t_TyChar>, s_knTriggerCharDecRefEnd, s_knTriggerCharDecRefBegin >;

static const vtyActionIdent s_knTriggerCharHexRefBegin = 15;
static const vtyActionIdent s_knTriggerCharHexRefEnd = 16;
template < class t_tyChar >
using TyGetTriggerCharHexRefBegin = _l_trigger_position< t_TyChar, s_knTriggerCharHexRefBegin >;
template < class t_tyChar >
using TyGetTriggerCharHexRefEnd = _l_trigger_string_typed_range< s_kdtCharHexRef, TyGetTriggerCharDataEnd<t_TyChar>, s_knTriggerCharHexRefEnd, s_knTriggerCharHexRefBegin >;

// Not necessary because we can look at the trigger id within the token's constituent _l_data_typed_range objects.
// static const vtyActionIdent s_knTriggerAttValueDoubleQuote = 17;
// TyGetTriggerAttValueDoubleQuote _l_trigger_bool< t_TyChar, s_knTriggerAttValueDoubleQuote >;

static const vtyActionIdent s_knTriggerPrefixBegin = 18;
static const vtyActionIdent s_knTriggerPrefixEnd = 19;
template < class t_tyChar >
using TyGetTriggerPrefixBegin = _l_trigger_position< t_TyChar, s_knTriggerPrefixBegin >;
template < class t_tyChar >
using TyGetTriggerPrefixEnd = _l_trigger_string< t_TyChar, s_knTriggerPrefixEnd, s_knTriggerPrefixBegin >;

static const vtyActionIdent s_knTriggerLocalPartBegin = 20;
static const vtyActionIdent s_knTriggerLocalPartEnd = 21;
template < class t_tyChar >
using TyGetTriggerLocalPartBegin = _l_trigger_position< t_TyChar, s_knTriggerLocalPartBegin >;
template < class t_tyChar >
using TyGetTriggerLocalPartEnd = _l_trigger_string< t_TyChar, s_knTriggerLocalPartEnd, s_knTriggerLocalPartBegin >;

#if 0 // This trigger is ambiguous due to the nature of attribute names and xmlns attribute names - they match each other - so this trigger fires all the time erroneously.
static const vtyActionIdent s_knTriggerDefaultAttName = 20;
template < class t_tyChar >
using TyGetTriggerDefaultAttName _l_trigger_bool< t_TyChar, s_knTriggerDefaultAttName >;
#endif //0

#if 0 // These triggers also cause ambiguity since PrefixedAttName matches a QName.
static const vtyActionIdent s_knTriggerPrefixedAttNameBegin = 21;
static const vtyActionIdent s_knTriggerPrefixedAttNameEnd = 22;
template < class t_tyChar >
using TyGetTriggerPrefixedAttNameBegin = _l_trigger_position< t_TyChar, s_knTriggerPrefixedAttNameBegin >;
template < class t_tyChar >
using TyGetTriggerPrefixedAttNameEnd = _l_trigger_string< t_TyChar, s_knTriggerPrefixedAttNameEnd, s_knTriggerPrefixedAttNameBegin >;
#endif //0

static const vtyActionIdent s_knTriggerSaveTagName = 23;
template < class t_tyChar >
using TyGetTriggerSaveTagName = _l_action_save_data_single< s_knTriggerSaveTagName, true, TyGetTriggerPrefixEnd<t_TyChar>, TyGetTriggerLocalPartEnd<t_TyChar> >;

static const vtyActionIdent s_knTriggerSaveAttributes = 24;
template < class t_tyChar >
using TyGetTriggerSaveAttributes = _l_action_save_data_multiple< s_knTriggerSaveAttributes, true, 
  TyGetTriggerPrefixEnd<t_TyChar>, TyGetTriggerLocalPartEnd<t_TyChar>, /* TyGetTriggerDefaultAttName<t_TyChar>, TyGetTriggerPrefixedAttNameEnd<t_TyChar>,  */
  TyGetTriggerPrefixEnd<t_TyChar>, TyGetTriggerLocalPartEnd<t_TyChar>, /* TyGetTriggerDefaultAttName<t_TyChar>, TyGetTriggerPrefixedAttNameEnd<t_TyChar>,  */
  /* TyGetTriggerAttValueDoubleQuote<t_TyChar>,  */TyGetTriggerCharDataEnd<t_TyChar> >;

#if 0 // The comment production has a problem with triggers due to its nature. It is easy to manage without triggers - impossible to try to stick triggers in - they'll never work.
static const vtyActionIdent s_knTriggerCommentBegin = 25;
static const vtyActionIdent s_knTriggerCommentEnd = 26;
template < class t_tyChar >
using TyGetTriggerCommentBegin = _l_trigger_position< t_TyChar, s_knTriggerCommentBegin >;
template < class t_tyChar >
using TyGetTriggerCommentEnd = _l_trigger_string< t_TyChar, s_knTriggerCommentEnd, s_knTriggerCommentBegin >;
#endif //0

static const vtyActionIdent s_knTriggerStandaloneYes = 27;
template < class t_tyChar >
using TyGetTriggerStandaloneYes = _l_trigger_bool< t_TyChar, s_knTriggerStandaloneYes >;
static const vtyActionIdent s_knTriggerStandaloneDoubleQuote = 28;
template < class t_tyChar >
using TyGetTriggerStandaloneDoubleQuote = _l_trigger_bool< t_TyChar, s_knTriggerStandaloneDoubleQuote >;

static const vtyActionIdent s_knTriggerEncodingNameBegin = 29;
static const vtyActionIdent s_knTriggerEncodingNameEnd = 30;
template < class t_tyChar >
using TyGetTriggerEncodingNameBegin = _l_trigger_position< t_TyChar, s_knTriggerEncodingNameBegin >;
template < class t_tyChar >
using TyGetTriggerEncodingNameEnd = _l_trigger_string< t_TyChar, s_knTriggerEncodingNameEnd, s_knTriggerEncodingNameBegin >;
static const vtyActionIdent s_knTriggerEncDeclDoubleQuote = 31;
template < class t_tyChar >
using TyGetTriggerEncDeclDoubleQuote = _l_trigger_bool< t_TyChar, s_knTriggerEncDeclDoubleQuote >;

static const vtyActionIdent s_knTriggerVersionNumBegin = 32;
static const vtyActionIdent s_knTriggerVersionNumEnd = 33;
template < class t_tyChar >
using TyGetTriggerVersionNumBegin = _l_trigger_position< t_TyChar, s_knTriggerVersionNumBegin >;
template < class t_tyChar >
using TyGetTriggerVersionNumEnd = _l_trigger_string< t_TyChar, s_knTriggerVersionNumEnd, s_knTriggerVersionNumBegin >;
static const vtyActionIdent s_knTriggerVersionNumDoubleQuote = 34;
template < class t_tyChar >
using TyGetTriggerVersionNumDoubleQuote = _l_trigger_bool< t_TyChar, s_knTriggerVersionNumDoubleQuote >;

static const vtyActionIdent s_knTriggerPEReferenceBegin = 35;
static const vtyActionIdent s_knTriggerPEReferenceEnd = 36;
template < class t_tyChar >
using TyGetTriggerPEReferenceBegin = _l_trigger_position< t_TyChar, s_knTriggerPEReferenceBegin >;
template < class t_tyChar >
using TyGetTriggerPEReferenceEnd = _l_trigger_string< t_TyChar, s_knTriggerPEReferenceEnd, s_knTriggerPEReferenceBegin >;

static const vtyActionIdent s_knTriggerSystemLiteralBegin = 37;
static const vtyActionIdent s_knTriggerSystemLiteralDoubleQuoteEnd = 38;
static const vtyActionIdent s_knTriggerSystemLiteralSingleQuoteEnd = 39;
template < class t_tyChar >
using TyGetTriggerSystemLiteralBegin = _l_trigger_position< t_TyChar, s_knTriggerSystemLiteralBegin >;
template < class t_tyChar >
using TyGetTriggerSystemLiteralDoubleQuoteEnd = _l_trigger_string< t_TyChar, s_knTriggerSystemLiteralDoubleQuoteEnd, s_knTriggerSystemLiteralBegin >;
template < class t_tyChar >
using TyGetTriggerSystemLiteralSingleQuoteEnd = _l_trigger_string_typed_range< s_kdtPlainText, TyGetTriggerSystemLiteralDoubleQuoteEnd<t_TyChar>, s_knTriggerSystemLiteralSingleQuoteEnd, s_knTriggerSystemLiteralBegin >;

static const vtyActionIdent s_knTriggerPubidLiteralBegin = 40;
static const vtyActionIdent s_knTriggerPubidLiteralDoubleQuoteEnd = 41;
static const vtyActionIdent s_knTriggerPubidLiteralSingleQuoteEnd = 42;
template < class t_tyChar >
using TyGetTriggerPubidLiteralBegin = _l_trigger_position< t_TyChar, s_knTriggerPubidLiteralBegin >;
template < class t_tyChar >
using TyGetTriggerPubidLiteralDoubleQuoteEnd = _l_trigger_string< t_TyChar, s_knTriggerPubidLiteralDoubleQuoteEnd, s_knTriggerPubidLiteralBegin >;
template < class t_tyChar >
using TyGetTriggerPubidLiteralSingleQuoteEnd = _l_trigger_string_typed_range< s_kdtPlainText, TyGetTriggerPubidLiteralDoubleQuoteEnd<t_TyChar>, s_knTriggerPubidLiteralSingleQuoteEnd, s_knTriggerPubidLiteralBegin >;

// Tokens:
static const vtyActionIdent s_knTokenSTag = 1000;
template < class t_tyChar >
using TyGetTokenSTag = _l_action_token< _l_action_save_data_single< s_knTokenSTag, true, TyGetTriggerSaveTagName<t_TyChar>, TyGetTriggerSaveAttributes<t_TyChar> > >;

static const vtyActionIdent s_knTokenETag = 1001;
template < class t_tyChar >
using TyGetTokenETag = _l_action_token< _l_action_save_data_single< s_knTokenETag, true, TyGetTriggerPrefixEnd<t_TyChar>, TyGetTriggerLocalPartEnd<t_TyChar> > >;

static const vtyActionIdent s_knTokenEmptyElemTag = 1002;
template < class t_tyChar >
using TyGetTokenEmptyElemTag = _l_action_token< _l_action_save_data_single< s_knTokenEmptyElemTag, true, TyGetTriggerSaveTagName<t_TyChar>, TyGetTriggerSaveAttributes<t_TyChar> > >;

static const vtyActionIdent s_knTokenComment = 1003;
template < class t_tyChar >
using TyGetTokenComment = _l_action_token< _l_trigger_noop< t_TyChar, s_knTokenComment, true > >;

static const vtyActionIdent s_knTokenXMLDecl = 1004;
template < class t_tyChar >
using TyGetTokenXMLDecl = _l_action_token< _l_action_save_data_single< s_knTokenXMLDecl, true, TyGetTriggerStandaloneYes<t_TyChar>, TyGetTriggerStandaloneYes<t_TyChar>, 
  TyGetTriggerEncodingNameEnd<t_TyChar>, TyGetTriggerEncDeclDoubleQuote<t_TyChar>, TyGetTriggerVersionNumEnd<t_TyChar>, TyGetTriggerVersionNumDoubleQuote<t_TyChar> > >;

static const vtyActionIdent s_knTokenCDataSection = 1004;
template < class t_tyChar >
using TyGetTokenCDataSection _l_action_token< _l_trigger_noop< t_TyChar, s_knTokenCDataSection, true > >;

__XMLP_END_NAMESPACE
