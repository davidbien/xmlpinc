#pragma once

//          Copyright David Lawrence Bien 1997 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt).

// _xmlpdtd.h
// Generated parser - LALR(1): 225 states.
// Do not edit: this is written by the parser generator ( pgen ).
//
// The grammar:
//  0: $accept : $entry:doctypedecl doctypedecl
//  1: $accept : $entry:extSubset extSubset
//  2: $accept : $entry:extSubsetDecl extSubsetDecl
//  3: doctypedecl : '<!DOCTYPE' S Name ( S ExternalID S? | S? ) ( '[' intSubset ']' S? )? '>'  [doctypedecl]
//  4: DeclSep : PEReference  [DeclSep_PEReference]
//  5: DeclSep : S
//  6: intSubset : ( markupdecl | DeclSep )*
//  7: markupdecl : elementdecl
//  8: markupdecl : AttlistDecl
//  9: markupdecl : EntityDecl
//  10: markupdecl : NotationDecl
//  11: markupdecl : PI
//  12: markupdecl : Comment
//  13: extSubset : TextDecl? extSubsetDecl
//  14: extSubsetDecl : ( markupdecl | conditionalSect | DeclSep )*
//  15: elementdecl : '<!ELEMENT' S Name S contentspec S? '>'  [elementdecl]
//  16: contentspec : 'EMPTY'
//  17: contentspec : 'ANY'
//  18: contentspec : Mixed
//  19: contentspec : children
//  20: children : ( choice | seq ) ( '?' | '*' | '+' )?  [children]
//  21: cp : ( Name | choice | seq ) ( '?' | '*' | '+' )?  [cp]
//  22: choice : '(' S? cp ( S? '|' S? cp )+ S? ')'  [choice]
//  23: seq : '(' S? cp ( S? ',' S? cp )+ S? ')'  [seq_list]
//  24: seq : '(' S? cp S? ')'  [seq_single]
//  25: Mixed : '(' S? '#PCDATA' ( S? '|' S? Name )+ S? ')' '*'  [Mixed_names]
//  26: Mixed : '(' S? '#PCDATA' S? ')' '*'?  [Mixed_pcdata]
//  27: AttlistDecl : '<!ATTLIST' S Name AttDef* S? '>'  [AttlistDecl]
//  28: AttDef : S Name S AttType S DefaultDecl  [AttDef]
//  29: AttType : StringType
//  30: AttType : TokenizedType
//  31: AttType : EnumeratedType
//  32: StringType : 'CDATA'
//  33: TokenizedType : 'ID'
//  34: TokenizedType : 'IDREF'
//  35: TokenizedType : 'IDREFS'
//  36: TokenizedType : 'ENTITY'
//  37: TokenizedType : 'ENTITIES'
//  38: TokenizedType : 'NMTOKEN'
//  39: TokenizedType : 'NMTOKENS'
//  40: EnumeratedType : NotationType
//  41: EnumeratedType : Enumeration
//  42: NotationType : 'NOTATION' S '(' S? Name ( S? '|' S? Name )* S? ')'  [NotationType]
//  43: Enumeration : '(' S? Nmtoken ( S? '|' S? Nmtoken )* S? ')'  [Enumeration]
//  44: DefaultDecl : '#REQUIRED'
//  45: DefaultDecl : '#IMPLIED'
//  46: DefaultDecl : ( '#FIXED' S )? AttValue  [DefaultDecl_value]
//  47: conditionalSect : includeSect
//  48: conditionalSect : ignoreSect
//  49: includeSect : '<![' S? 'INCLUDE' S? '[' extSubsetDecl ']]>'
//  50: ignoreSect : '<![' S? 'IGNORE' S? '[' ignoreSectContents ']]>'
//  51: ignoreSectContents : Ignore* ( '<![' ignoreSectContents ']]>' Ignore* )*
//  52: EntityDecl : GEDecl
//  53: EntityDecl : PEDecl
//  54: GEDecl : '<!ENTITY' S Name S EntityDef '>'  [GEDecl]
//  55: PEDecl : '<!ENTITY' S '%' S Name S PEDef S? '>'  [PEDecl]
//  56: EntityDef : EntityValue S?  [EntityDef_value]
//  57: EntityDef : ExternalID S?  [EntityDef_external]
//  58: EntityDef : ExternalID NDataDecl S?  [EntityDef_unparsed]
//  59: PEDef : EntityValue
//  60: PEDef : ExternalID
//  61: ExternalID : 'SYSTEM' S SystemLiteral  [ExternalID_system]
//  62: ExternalID : PublicID S SystemLiteral  [ExternalID_public]
//  63: NDataDecl : S 'NDATA' S Name  [NDataDecl]
//  64: NotationDecl : '<!NOTATION' S Name S ( ExternalID S? | PublicID S? ) '>'  [NotationDecl]
//  65: PublicID : 'PUBLIC' S PubidLiteral  [PublicID]
//  66: ( S ExternalID S? | S? ) : S ExternalID S?
//  67: ( S ExternalID S? | S? ) : S?
//  68: S? : <empty>
//  69: S? : S
//  70: ( '[' intSubset ']' S? ) : '[' intSubset ']' S?
//  71: ( '[' intSubset ']' S? )? : <empty>
//  72: ( '[' intSubset ']' S? )? : ( '[' intSubset ']' S? )
//  73: ( markupdecl | DeclSep ) : markupdecl
//  74: ( markupdecl | DeclSep ) : DeclSep
//  75: ( markupdecl | DeclSep )* : <empty>
//  76: ( markupdecl | DeclSep )* : ( markupdecl | DeclSep )* ( markupdecl | DeclSep )
//  77: TextDecl? : <empty>
//  78: TextDecl? : TextDecl
//  79: ( markupdecl | conditionalSect | DeclSep ) : markupdecl
//  80: ( markupdecl | conditionalSect | DeclSep ) : conditionalSect
//  81: ( markupdecl | conditionalSect | DeclSep ) : DeclSep
//  82: ( markupdecl | conditionalSect | DeclSep )* : <empty>
//  83: ( markupdecl | conditionalSect | DeclSep )* : ( markupdecl | conditionalSect | DeclSep )* ( markupdecl | conditionalSect | DeclSep )
//  84: ( choice | seq ) : choice
//  85: ( choice | seq ) : seq
//  86: ( '?' | '*' | '+' ) : '?'
//  87: ( '?' | '*' | '+' ) : '*'
//  88: ( '?' | '*' | '+' ) : '+'
//  89: ( '?' | '*' | '+' )? : <empty>
//  90: ( '?' | '*' | '+' )? : ( '?' | '*' | '+' )
//  91: ( Name | choice | seq ) : Name
//  92: ( Name | choice | seq ) : choice
//  93: ( Name | choice | seq ) : seq
//  94: ( S? '|' S? cp ) : S? '|' S? cp
//  95: ( S? '|' S? cp )+ : ( S? '|' S? cp )
//  96: ( S? '|' S? cp )+ : ( S? '|' S? cp )+ ( S? '|' S? cp )
//  97: ( S? ',' S? cp ) : S? ',' S? cp
//  98: ( S? ',' S? cp )+ : ( S? ',' S? cp )
//  99: ( S? ',' S? cp )+ : ( S? ',' S? cp )+ ( S? ',' S? cp )
//  100: ( S? '|' S? Name ) : S? '|' S? Name
//  101: ( S? '|' S? Name )+ : ( S? '|' S? Name )
//  102: ( S? '|' S? Name )+ : ( S? '|' S? Name )+ ( S? '|' S? Name )
//  103: '*'? : <empty>
//  104: '*'? : '*'
//  105: AttDef* : <empty>
//  106: AttDef* : AttDef* AttDef
//  107: ( S? '|' S? Name )* : <empty>
//  108: ( S? '|' S? Name )* : ( S? '|' S? Name )* ( S? '|' S? Name )
//  109: ( S? '|' S? Nmtoken ) : S? '|' S? Nmtoken
//  110: ( S? '|' S? Nmtoken )* : <empty>
//  111: ( S? '|' S? Nmtoken )* : ( S? '|' S? Nmtoken )* ( S? '|' S? Nmtoken )
//  112: ( '#FIXED' S ) : '#FIXED' S
//  113: ( '#FIXED' S )? : <empty>
//  114: ( '#FIXED' S )? : ( '#FIXED' S )
//  115: Ignore* : <empty>
//  116: Ignore* : Ignore* Ignore
//  117: ( '<![' ignoreSectContents ']]>' Ignore* ) : '<![' ignoreSectContents ']]>' Ignore*
//  118: ( '<![' ignoreSectContents ']]>' Ignore* )* : <empty>
//  119: ( '<![' ignoreSectContents ']]>' Ignore* )* : ( '<![' ignoreSectContents ']]>' Ignore* )* ( '<![' ignoreSectContents ']]>' Ignore* )
//  120: ( ExternalID S? | PublicID S? ) : ExternalID S?
//  121: ( ExternalID S? | PublicID S? ) : PublicID S?

#include "_p_parse.h"

#define __XMLPDTD_BEGIN_NAMESPACE namespace ns_xmlpdtd {
#define __XMLPDTD_END_NAMESPACE }
#define __XMLPDTD_USING_NAMESPACE using namespace ns_xmlpdtd;
#define __XMLPDTD_NAMESPACE ns_xmlpdtd

__XMLPDTD_BEGIN_NAMESPACE
USE_PGENOBJ_NAMESPACE

// The terminals - the columns of the action table - and the identifiers of their tokens.
//  1: $end - token 2147483647
inline constexpr vtyParseTerminal s_knTerminal_S = 2; // token 4000
inline constexpr vtyParseTerminal s_knTerminal_Name = 3; // token 4001
inline constexpr vtyParseTerminal s_knTerminal_Nmtoken = 4; // token 4002
inline constexpr vtyParseTerminal s_knTerminal_EntityValue = 5; // token 4037
inline constexpr vtyParseTerminal s_knTerminal_AttValue = 6; // token 4038
inline constexpr vtyParseTerminal s_knTerminal_SystemLiteral = 7; // token 4039
inline constexpr vtyParseTerminal s_knTerminal_PubidLiteral = 8; // token 4040
inline constexpr vtyParseTerminal s_knTerminal_Comment = 9; // token 1003
inline constexpr vtyParseTerminal s_knTerminal_PI = 10; // token 1007
inline constexpr vtyParseTerminal s_knTerminal_PEReference = 11; // token 4041
inline constexpr vtyParseTerminal s_knTerminal_TextDecl = 12; // token 4047
inline constexpr vtyParseTerminal s_knTerminal_Ignore = 13; // token 4046
//  14: '<!DOCTYPE' - token 4003
//  15: '<!ELEMENT' - token 4004
//  16: '<!ATTLIST' - token 4005
//  17: '<!ENTITY' - token 4006
//  18: '<!NOTATION' - token 4007
//  19: '<![' - token 4042
//  20: ']]>' - token 4043
//  21: '[' - token 4008
//  22: ']' - token 4009
//  23: '>' - token 4010
//  24: '(' - token 4011
//  25: ')' - token 4012
//  26: '|' - token 4013
//  27: ',' - token 4014
//  28: '?' - token 4015
//  29: '*' - token 4016
//  30: '+' - token 4017
//  31: '%' - token 4018
//  32: '#PCDATA' - token 4019
//  33: '#REQUIRED' - token 4031
//  34: '#IMPLIED' - token 4032
//  35: '#FIXED' - token 4033
//  36: 'EMPTY' - token 4020
//  37: 'ANY' - token 4021
//  38: 'CDATA' - token 4022
//  39: 'ID' - token 4023
//  40: 'IDREF' - token 4024
//  41: 'IDREFS' - token 4025
//  42: 'ENTITY' - token 4026
//  43: 'ENTITIES' - token 4027
//  44: 'NMTOKEN' - token 4028
//  45: 'NMTOKENS' - token 4029
//  46: 'NOTATION' - token 4030
//  47: 'SYSTEM' - token 4034
//  48: 'PUBLIC' - token 4035
//  49: 'NDATA' - token 4036
//  50: 'INCLUDE' - token 4044
//  51: 'IGNORE' - token 4045
//  52: $entry:doctypedecl - not a token: the terminal of an entry
//  53: $entry:extSubset - not a token: the terminal of an entry
//  54: $entry:extSubsetDecl - not a token: the terminal of an entry
// The non-terminals - the columns of the goto table.
//  0: $accept
inline constexpr uint32_t s_knNonTerminal_doctypedecl = 1;
inline constexpr uint32_t s_knNonTerminal_DeclSep = 2;
inline constexpr uint32_t s_knNonTerminal_intSubset = 3;
inline constexpr uint32_t s_knNonTerminal_markupdecl = 4;
inline constexpr uint32_t s_knNonTerminal_extSubset = 5;
inline constexpr uint32_t s_knNonTerminal_extSubsetDecl = 6;
inline constexpr uint32_t s_knNonTerminal_elementdecl = 7;
inline constexpr uint32_t s_knNonTerminal_contentspec = 8;
inline constexpr uint32_t s_knNonTerminal_children = 9;
inline constexpr uint32_t s_knNonTerminal_cp = 10;
inline constexpr uint32_t s_knNonTerminal_choice = 11;
inline constexpr uint32_t s_knNonTerminal_seq = 12;
inline constexpr uint32_t s_knNonTerminal_Mixed = 13;
inline constexpr uint32_t s_knNonTerminal_AttlistDecl = 14;
inline constexpr uint32_t s_knNonTerminal_AttDef = 15;
inline constexpr uint32_t s_knNonTerminal_AttType = 16;
inline constexpr uint32_t s_knNonTerminal_StringType = 17;
inline constexpr uint32_t s_knNonTerminal_TokenizedType = 18;
inline constexpr uint32_t s_knNonTerminal_EnumeratedType = 19;
inline constexpr uint32_t s_knNonTerminal_NotationType = 20;
inline constexpr uint32_t s_knNonTerminal_Enumeration = 21;
inline constexpr uint32_t s_knNonTerminal_DefaultDecl = 22;
inline constexpr uint32_t s_knNonTerminal_conditionalSect = 23;
inline constexpr uint32_t s_knNonTerminal_includeSect = 24;
inline constexpr uint32_t s_knNonTerminal_ignoreSect = 25;
inline constexpr uint32_t s_knNonTerminal_ignoreSectContents = 26;
inline constexpr uint32_t s_knNonTerminal_EntityDecl = 27;
inline constexpr uint32_t s_knNonTerminal_GEDecl = 28;
inline constexpr uint32_t s_knNonTerminal_PEDecl = 29;
inline constexpr uint32_t s_knNonTerminal_EntityDef = 30;
inline constexpr uint32_t s_knNonTerminal_PEDef = 31;
inline constexpr uint32_t s_knNonTerminal_ExternalID = 32;
inline constexpr uint32_t s_knNonTerminal_NDataDecl = 33;
inline constexpr uint32_t s_knNonTerminal_NotationDecl = 34;
inline constexpr uint32_t s_knNonTerminal_PublicID = 35;
//  36: ( S ExternalID S? | S? )
//  37: S?
//  38: ( '[' intSubset ']' S? )
//  39: ( '[' intSubset ']' S? )?
//  40: ( markupdecl | DeclSep )
//  41: ( markupdecl | DeclSep )*
//  42: TextDecl?
//  43: ( markupdecl | conditionalSect | DeclSep )
//  44: ( markupdecl | conditionalSect | DeclSep )*
//  45: ( choice | seq )
//  46: ( '?' | '*' | '+' )
//  47: ( '?' | '*' | '+' )?
//  48: ( Name | choice | seq )
//  49: ( S? '|' S? cp )
//  50: ( S? '|' S? cp )+
//  51: ( S? ',' S? cp )
//  52: ( S? ',' S? cp )+
//  53: ( S? '|' S? Name )
//  54: ( S? '|' S? Name )+
//  55: '*'?
//  56: AttDef*
//  57: ( S? '|' S? Name )*
//  58: ( S? '|' S? Nmtoken )
//  59: ( S? '|' S? Nmtoken )*
//  60: ( '#FIXED' S )
//  61: ( '#FIXED' S )?
//  62: Ignore*
//  63: ( '<![' ignoreSectContents ']]>' Ignore* )
//  64: ( '<![' ignoreSectContents ']]>' Ignore* )*
//  65: ( ExternalID S? | PublicID S? )
// The named productions.
inline constexpr vtyParseProduction s_knProduction_doctypedecl = 3; // doctypedecl : '<!DOCTYPE' S Name ( S ExternalID S? | S? ) ( '[' intSubset ']' S? )? '>'
inline constexpr vtyParseProduction s_knProduction_DeclSep_PEReference = 4; // DeclSep : PEReference
inline constexpr vtyParseProduction s_knProduction_elementdecl = 15; // elementdecl : '<!ELEMENT' S Name S contentspec S? '>'
inline constexpr vtyParseProduction s_knProduction_children = 20; // children : ( choice | seq ) ( '?' | '*' | '+' )?
inline constexpr vtyParseProduction s_knProduction_cp = 21; // cp : ( Name | choice | seq ) ( '?' | '*' | '+' )?
inline constexpr vtyParseProduction s_knProduction_choice = 22; // choice : '(' S? cp ( S? '|' S? cp )+ S? ')'
inline constexpr vtyParseProduction s_knProduction_seq_list = 23; // seq : '(' S? cp ( S? ',' S? cp )+ S? ')'
inline constexpr vtyParseProduction s_knProduction_seq_single = 24; // seq : '(' S? cp S? ')'
inline constexpr vtyParseProduction s_knProduction_Mixed_names = 25; // Mixed : '(' S? '#PCDATA' ( S? '|' S? Name )+ S? ')' '*'
inline constexpr vtyParseProduction s_knProduction_Mixed_pcdata = 26; // Mixed : '(' S? '#PCDATA' S? ')' '*'?
inline constexpr vtyParseProduction s_knProduction_AttlistDecl = 27; // AttlistDecl : '<!ATTLIST' S Name AttDef* S? '>'
inline constexpr vtyParseProduction s_knProduction_AttDef = 28; // AttDef : S Name S AttType S DefaultDecl
inline constexpr vtyParseProduction s_knProduction_NotationType = 42; // NotationType : 'NOTATION' S '(' S? Name ( S? '|' S? Name )* S? ')'
inline constexpr vtyParseProduction s_knProduction_Enumeration = 43; // Enumeration : '(' S? Nmtoken ( S? '|' S? Nmtoken )* S? ')'
inline constexpr vtyParseProduction s_knProduction_DefaultDecl_value = 46; // DefaultDecl : ( '#FIXED' S )? AttValue
inline constexpr vtyParseProduction s_knProduction_GEDecl = 54; // GEDecl : '<!ENTITY' S Name S EntityDef '>'
inline constexpr vtyParseProduction s_knProduction_PEDecl = 55; // PEDecl : '<!ENTITY' S '%' S Name S PEDef S? '>'
inline constexpr vtyParseProduction s_knProduction_EntityDef_value = 56; // EntityDef : EntityValue S?
inline constexpr vtyParseProduction s_knProduction_EntityDef_external = 57; // EntityDef : ExternalID S?
inline constexpr vtyParseProduction s_knProduction_EntityDef_unparsed = 58; // EntityDef : ExternalID NDataDecl S?
inline constexpr vtyParseProduction s_knProduction_ExternalID_system = 61; // ExternalID : 'SYSTEM' S SystemLiteral
inline constexpr vtyParseProduction s_knProduction_ExternalID_public = 62; // ExternalID : PublicID S SystemLiteral
inline constexpr vtyParseProduction s_knProduction_NDataDecl = 63; // NDataDecl : S 'NDATA' S Name
inline constexpr vtyParseProduction s_knProduction_NotationDecl = 64; // NotationDecl : '<!NOTATION' S Name S ( ExternalID S? | PublicID S? ) '>'
inline constexpr vtyParseProduction s_knProduction_PublicID = 65; // PublicID : 'PUBLIC' S PubidLiteral
// The entries - the non-terminals at which a parse may start: see _parser<>::FParse().
inline constexpr uint32_t s_knEntries = 3;
inline constexpr uint32_t s_knEntry_doctypedecl = 0;
inline constexpr uint32_t s_knEntry_extSubset = 1;
inline constexpr uint32_t s_knEntry_extSubsetDecl = 2;
// The modes of the lexical analyzer - in the order in which they were declared.
inline constexpr uint32_t s_knLexerModes = 9;
inline constexpr uint32_t s_knLexerMode_DtdTextDecl = 0;
inline constexpr uint32_t s_knLexerMode_DtdKeyword = 1;
inline constexpr uint32_t s_knLexerMode_DtdNmtoken = 2;
inline constexpr uint32_t s_knLexerMode_DtdEntityValue = 3;
inline constexpr uint32_t s_knLexerMode_DtdAttValue = 4;
inline constexpr uint32_t s_knLexerMode_DtdSystemLiteral = 5;
inline constexpr uint32_t s_knLexerMode_DtdPubidLiteral = 6;
inline constexpr uint32_t s_knLexerMode_Dtd = 7;
inline constexpr uint32_t s_knLexerMode_DtdIgnore = 8;

// The action table - a row for each state and a column for each terminal - packed: see ParseTableView in _p_parse.h.
// An action: 0 error, > 0 shift to state ( value - 1 ), < 0 reduce production ( -value - 1 ).
// 225 rows x 55 columns = 12375 places with 808 entries, 204 distinct rows, packed into 805 places.
inline constexpr uint32_t s_rgParseActionBase[] = {
  92, 482, 330, 7, 4, 342, 493, 4, 482, 5, 4, 116, 12, 45, 72, 85, 0, 22, 44, 66,
  354, 366, 88, 110, 378, 390, 402, 132, 154, 176, 198, 414, 108, 97, 423, 217, 119, 93, 150, 155,
  224, 441, 441, 160, 182, 186, 178, 226, 158, 189, 315, 213, 251, 478, 216, 135, 310, 187, 261, 274,
  225, 283, 537, 277, 284, 52, 504, 314, 327, 331, 364, 314, 355, 452, 575, 454, 367, 602, 651, 99,
  314, 656, 6, 28, 668, 50, 394, 421, 639, 438, 436, 547, 463, 476, 236, 498, 554, 218, 576, 220,
  185, 582, 242, 20, 314, 631, 558, 264, 165, 695, 68, 90, 112, 134, 669, 670, 248, 268, 314, 564,
  574, 591, 450, 279, 509, 470, 671, 314, 674, 585, 696, 492, 252, 291, 515, 1, 608, 614, 621, 286,
  697, 52, 551, 698, 611, 662, 708, 720, 721, 722, 723, 729, 730, 731, 2, 732, 733, 734, 735, 736,
  737, 641, 288, 252, 675, 295, 309, 252, 451, 480, 23, 724, 308, 688, 716, 738, 169, 10, 434, 676,
  525, 291, 291, 412, 678, 586, 673, 570, 52, 10, 630, 739, 693, 694, 703, 740, 741, 742, 704, 705,
  714, 675, 675, 592, 601, 548, 745, 252, 743, 706, 643, 707, 650, 655, 657, 687, 659, 252, 2, 748,
  689, 661, 747, 750, 663,
};
inline constexpr int16_t s_rgParseActionCheck[] = {
  0, 2, 3, 3, 3, 2, 5, 3, 3, 10, 11, 12, 3, 4, 3, 16, 17, 18, 19, 20,
  21, 15, 23, 2, 3, 3, 26, 27, 28, 24, 3, 10, 11, 12, 29, 30, 31, 16, 17, 18,
  19, 20, 21, 24, 23, 2, 3, 3, 26, 27, 28, 24, 3, 10, 11, 12, 29, 30, 31, 16,
  17, 18, 19, 20, 21, 14, 23, 2, 3, 50, 3, 20, 21, 24, 3, 10, 11, 12, 29, 30,
  31, 16, 17, 18, 19, 20, 21, 3, 23, 2, 3, 24, 3, 26, 27, 28, 4, 10, 11, 12,
  4, 3, 4, 16, 17, 18, 19, 20, 21, 2, 23, 2, 3, 24, 3, 26, 27, 28, 3, 10,
  11, 12, 4, 25, 32, 16, 17, 18, 19, 20, 21, 33, 23, 2, 3, 24, 3, 26, 27, 28,
  6, 10, 11, 12, 53, 54, 55, 16, 17, 18, 19, 20, 21, 4, 23, 2, 3, 24, 4, 26,
  27, 28, 3, 10, 11, 12, 51, 52, 4, 16, 17, 18, 19, 20, 21, 7, 23, 2, 3, 22,
  3, 24, 48, 49, 3, 10, 11, 12, 3, 25, 6, 16, 17, 18, 19, 20, 21, 33, 23, 2,
  3, 24, 34, 35, 36, 48, 49, 10, 11, 12, 22, 25, 24, 16, 17, 18, 19, 20, 21, 4,
  23, 2, 3, 37, 38, 8, 3, 3, 3, 10, 11, 12, 48, 49, 22, 16, 17, 18, 19, 20,
  21, 24, 23, 2, 3, 22, 22, 24, 24, 14, 3, 10, 11, 12, 3, 20, 21, 16, 17, 18,
  19, 20, 21, 3, 23, 2, 3, 51, 52, 22, 3, 24, 22, 10, 11, 12, 3, 26, 27, 16,
  17, 18, 19, 20, 21, 3, 23, 2, 3, 22, 3, 24, 14, 3, 4, 10, 11, 12, 20, 21,
  24, 16, 17, 18, 19, 20, 21, 24, 23, 2, 3, 3, 3, 26, 27, 25, 3, 10, 11, 12,
  26, 27, 28, 16, 17, 18, 19, 20, 21, 3, 23, 2, 3, 24, 26, 27, 22, 24, 24, 10,
  11, 12, 13, 2, 3, 16, 17, 18, 19, 20, 24, 10, 11, 12, 24, 2, 3, 16, 17, 18,
  19, 20, 21, 10, 11, 12, 3, 2, 3, 16, 17, 18, 19, 20, 21, 10, 11, 12, 24, 2,
  3, 16, 17, 18, 19, 20, 21, 10, 11, 12, 24, 2, 3, 16, 17, 18, 19, 20, 21, 10,
  11, 12, 9, 2, 3, 16, 17, 18, 19, 20, 21, 10, 11, 12, 3, 2, 3, 16, 17, 18,
  19, 20, 21, 10, 11, 12, 4, 5, 8, 16, 17, 18, 19, 20, 21, 24, 3, 26, 27, 28,
  29, 30, 31, 3, 22, 8, 24, 25, 26, 27, 28, 2, 3, 3, 3, 33, 3, 24, 23, 10,
  11, 12, 22, 30, 2, 16, 17, 18, 19, 20, 21, 2, 3, 51, 52, 24, 26, 24, 28, 10,
  11, 12, 3, 2, 3, 16, 17, 18, 19, 20, 21, 10, 11, 12, 2, 3, 21, 16, 17, 18,
  19, 20, 10, 11, 12, 26, 3, 28, 16, 17, 18, 19, 20, 10, 11, 12, 25, 3, 21, 16,
  17, 18, 19, 20, 21, 48, 49, 3, 20, 21, 39, 40, 41, 42, 43, 44, 45, 46, 47, 3,
  26, 27, 28, 29, 30, 31, 10, 11, 12, 3, 26, 27, 16, 17, 18, 19, 10, 11, 12, 23,
  3, 14, 16, 17, 18, 19, 3, 20, 21, 23, 20, 21, 3, 10, 11, 12, 3, 24, 4, 16,
  17, 18, 19, 10, 11, 12, 23, 3, 3, 16, 17, 18, 19, 3, 3, 26, 23, 28, 24, 24,
  10, 11, 12, 3, 3, 24, 16, 17, 18, 19, 3, 26, 27, 23, 4, 24, 3, 26, 27, 28,
  29, 30, 31, 3, 24, 24, 26, 27, 28, 29, 30, 31, 3, 26, 27, 28, 29, 30, 31, 26,
  27, 28, 29, 30, 31, 3, 26, 27, 28, 29, 30, 31, 3, 3, 24, 26, 27, 3, 3, 3,
  22, 3, 24, 3, 3, 3, 26, 27, 26, 27, 3, 3, 3, 3, 24, 26, 3, 28, 4, 24,
  26, 27, 26, 27, 26, 27, 26, 27, 26, 27, 3, 24, 24, 24, 24, 3, 3, 24, 26, 25,
  28, 26, 27, 26, 27, 3, 3, 3, 3, 3, 3, 24, 26, 27, 26, 27, 24, 24, 24, 24,
  24, 24, 3, 3, 3, 3, 24, 24, 24, 24, 24, 3, 3, 3, 3, 3, 3, 3, 3, 3,
  25, 3, 5, 30, 21, 4, 7, 7, 4, 7, 3, 5, 3, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
};
inline constexpr int16_t s_rgParseActions[] = {
  0, -5, -5, 35, 35, -1, -69, 34, -85, -5, -5, -5, 35, -69, 37, -5, -5, -5, -5, -5,
  -5, 10, -5, -12, -12, -22, -69, -69, -69, -85, -86, -12, -12, -12, -85, -85, -85, -12, -12, -12,
  -12, -12, -12, -70, -12, -13, -13, 38, -22, -22, -22, -86, -90, -13, -13, -13, -86, -86, -86, -13,
  -13, -13, -13, -13, -13, -116, -13, -6, -6, 130, -89, -116, -116, -90, 39, -6, -6, -6, 113, 112,
  111, -6, -6, -6, -6, -6, -6, 40, -6, -8, -8, -89, -88, -89, -89, -89, 46, -8, -8, -8,
  41, 35, -69, -8, -8, -8, -8, -8, -8, -14, -8, -9, -9, -88, -87, -88, -88, -88, 35, -9,
  -9, -9, 44, -69, 45, -9, -9, -9, -9, -9, -9, -69, -9, -10, -10, -87, -91, -87, -87, -87,
  72, -10, -10, -10, 4, 3, 2, -10, -10, -10, -10, -10, -10, 47, -10, -53, -53, -91, 48, -91,
  -91, -91, 54, -53, -53, -53, -69, -69, 135, -53, -53, -53, -53, -53, -53, -114, -53, -54, -54, -70,
  -106, -70, 60, 59, 55, -54, -54, -54, 56, 134, 127, -54, -54, -54, -54, -54, -54, 133, -54, -11,
  -11, -106, 194, 193, 192, 60, 59, -11, -11, -11, 63, 80, -72, -11, -11, -11, -11, -11, -11, 71,
  -11, -65, -65, 79, 78, 118, 49, 35, 58, -65, -65, -65, 60, 59, 66, -65, -65, -65, -65, -65,
  -65, -70, -65, -55, -55, -69, -69, -69, -69, 124, -62, -55, -55, -55, 35, -119, -119, -55, -55, -55,
  -55, -55, -55, 87, -55, -28, -28, 43, 42, -62, -63, -62, 67, -28, -28, -28, 88, -69, -69, -28,
  -28, -28, -28, -28, -28, 90, -28, -16, -16, -63, -102, -63, -117, 35, -69, -16, -16, -16, -117, -117,
  -73, -16, -16, -16, -16, -16, -16, 93, -16, -56, -56, -96, 75, -102, -102, -69, 35, -56, -56, -56,
  184, 183, 182, -56, -56, -56, -56, -56, -56, 98, -56, -78, -78, -69, -96, -96, -68, -69, -68, -78,
  -78, -78, 7, -15, 20, -78, -78, -78, -78, -78, -69, 19, 18, 17, 100, -82, -82, 16, 15, 14,
  13, 12, -15, -82, -82, -82, 101, -80, -80, -82, -82, -82, -82, -82, -82, -80, -80, -80, 103, -81,
  -81, -80, -80, -80, -80, -80, -80, -81, -81, -81, 108, -48, -48, -81, -81, -81, -81, -81, -81, -48,
  -48, -48, 116, -49, -49, -48, -48, -48, -48, -48, -48, -49, -49, -49, -25, -84, -84, -49, -49, -49,
  -49, -49, -49, -84, -84, -84, -70, -70, 117, -84, -84, -84, -84, -84, -84, -25, -104, -25, -25, -25,
  -25, -25, -25, 35, -70, 118, -70, -70, -70, -70, -70, -51, -51, -99, 104, -70, -107, -104, 119, -51,
  -51, -51, -69, 199, -4, -51, -51, -51, -51, -51, -51, -50, -50, -70, -70, -69, -99, -107, -99, -50,
  -50, -50, 35, -83, -83, -50, -50, -50, -50, -50, -50, -83, -83, -83, -79, -79, 123, -83, -83, -83,
  -83, -83, -79, -79, -79, -69, -83, -69, -79, -79, -79, -79, -79, -83, -83, -83, 155, -92, 126, -83,
  -83, -83, -83, -83, -83, 60, 59, -103, 142, -52, 154, 153, 152, 151, 150, 149, 148, 147, 146, -76,
  -92, -92, -92, -92, -92, -92, -76, -76, -76, 20, -103, -103, -76, -76, -76, -76, 19, 18, 17, -76,
  132, 124, 16, 15, 14, 13, -75, -118, -118, -7, -120, -120, -100, -75, -75, -75, -74, -121, 107, -75,
  -75, -75, -75, -74, -74, -74, -75, 145, -97, -74, -74, -74, -74, -77, -23, -100, -74, -100, -70, -122,
  -77, -77, -77, -24, -18, -57, -77, -77, -77, -77, -93, -97, -97, -77, 174, -23, -94, -23, -23, -23,
  -23, -23, -23, -90, -24, -18, -24, -24, -24, -24, -24, -24, -111, -93, -93, -93, -93, -93, -93, -94,
  -94, -94, -94, -94, -94, -101, -90, -90, -90, 113, 112, 111, -98, -17, -58, -111, -111, -95, -20, -108,
  -67, -112, -67, -109, 175, -110, 179, 178, -101, -101, -19, -21, -66, -60, -17, -98, -61, -98, 135, -20,
  -95, -95, -108, -108, -112, -112, -109, -109, -110, -110, -64, -19, -21, -66, -60, -46, -45, -61, 205, 134,
  182, 201, 178, 204, 183, -29, -105, -27, -47, -26, -40, -64, 220, 219, 224, 178, -46, -45, 140, -59,
  -71, 173, -39, -38, -37, -36, -29, -105, -27, -47, -26, -35, -34, -33, 177, -30, -31, -32, -41, -42,
  190, 209, 191, 212, 189, 211, -115, 210, 215, -113, -44, 225, -43, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
};
// The goto table - a row for each state and a column for each non-terminal: the state to go to after a reduction - packed.
// 225 rows x 66 columns = 14850 places with 134 entries, 60 distinct rows, packed into 230 places.
inline constexpr uint32_t s_rgParseGotoBase[] = {
  164, 7, 10, 2, 164, 0, 164, 164, 33, 164, 164, 3, 164, 164, 164, 164, 164, 164, 164, 164,
  164, 164, 164, 164, 164, 164, 164, 164, 164, 164, 164, 164, 164, 164, 164, 164, 164, 164, 164, 164,
  47, 69, 70, 164, 164, 164, 52, 164, 37, 49, 164, 164, 164, 32, 164, 36, 55, 8, 164, 164,
  72, 164, 73, 164, 164, 59, 80, 75, 76, 164, 164, 78, 164, 56, 164, 164, 164, 164, 164, 81,
  83, 164, 164, 164, 164, 44, 164, 164, 164, 164, 164, 4, 164, 164, 41, 164, 164, 164, 164, 164,
  43, 164, 164, 164, 85, 164, 164, 164, 25, 164, 164, 164, 164, 164, 164, 164, 164, 164, 86, 164,
  164, 164, 164, 164, 63, 164, 164, 90, 164, 164, 164, 29, 28, 91, 164, 5, 164, 164, 54, 164,
  164, 68, 164, 164, 164, 164, 164, 164, 164, 164, 164, 164, 164, 164, 92, 164, 164, 164, 164, 164,
  164, 164, 164, 58, 25, 164, 164, 61, 164, 65, 164, 164, 164, 164, 164, 164, 19, 94, 62, 164,
  164, 95, 96, 164, 164, 164, 164, 164, 74, 97, 79, 164, 164, 164, 164, 164, 164, 164, 164, 164,
  164, 48, 51, 164, 164, 164, 164, 67, 164, 164, 164, 164, 164, 164, 82, 164, 164, 66, 98, 164,
  164, 164, 164, 164, 164,
};
inline constexpr int16_t s_rgParseGotoCheck[] = {
  0, 0, 3, 2, 5, 0, 3, 8, 5, 0, 0, 8, 0, 7, 15, 6, 9, 10, 15, 12,
  13, 14, 0, 24, 25, 26, 0, 28, 29, 30, 0, 28, 29, 30, 35, 11, 12, 13, 35, 7,
  38, 23, 38, 44, 41, 17, 18, 19, 20, 21, 22, 45, 43, 46, 50, 51, 52, 53, 11, 12,
  13, 11, 12, 13, 33, 38, 31, 36, 33, 33, 16, 36, 36, 49, 32, 33, 4, 45, 36, 61,
  62, 54, 55, 37, 38, 27, 7, 39, 40, 34, 47, 48, 38, 38, 27, 38, 49, 66, 38, 49,
  47, 48, 38, 38, 38, 65, 38, 38, 57, 38, 50, 54, 38, 38, 42, 38, 52, 56, 38, 54,
  38, 63, 38, 38, 45, 59, 64, 38, 38, 38, 63, 38, 38, 38, 38, 38, 63, 0, 60, 58,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};
inline constexpr int16_t s_rgParseGotos[] = {
  0, 0, 20, 10, 21, 0, 119, 22, 120, 0, 0, 22, 0, 4, 23, 7, 80, 81, 23, 82,
  83, 84, 0, 24, 25, 26, 0, 27, 28, 29, 0, 27, 28, 29, 30, 135, 136, 137, 30, 32,
  35, 194, 165, 31, 121, 155, 156, 157, 158, 159, 160, 5, 8, 85, 166, 167, 168, 169, 212, 136,
  137, 213, 136, 137, 67, 161, 72, 68, 73, 60, 75, 61, 61, 138, 127, 128, 90, 5, 61, 195,
  196, 162, 163, 49, 50, 93, 95, 63, 64, 104, 113, 114, 76, 105, 171, 179, 138, 69, 184, 138,
  113, 170, 186, 220, 215, 124, 51, 52, 56, 88, 185, 180, 96, 98, 91, 101, 187, 199, 108, 221,
  109, 94, 130, 140, 5, 216, 142, 143, 164, 175, 94, 197, 201, 202, 206, 222, 205, 0, 207, 217,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};
// The productions: the non-terminal, the number of symbols, the name and the text.
inline constexpr ParseProductionInfo s_rgParseProductions[] = {
  { 0, 2, "", "$accept : $entry:doctypedecl doctypedecl" }, // 0
  { 0, 2, "", "$accept : $entry:extSubset extSubset" }, // 1
  { 0, 2, "", "$accept : $entry:extSubsetDecl extSubsetDecl" }, // 2
  { 1, 6, "doctypedecl", "doctypedecl : '<!DOCTYPE' S Name ( S ExternalID S\077 | S\077 ) ( '[' intSubset ']' S\077 )\077 '>'" }, // 3
  { 2, 1, "DeclSep_PEReference", "DeclSep : PEReference" }, // 4
  { 2, 1, "", "DeclSep : S" }, // 5
  { 3, 1, "", "intSubset : ( markupdecl | DeclSep )*" }, // 6
  { 4, 1, "", "markupdecl : elementdecl" }, // 7
  { 4, 1, "", "markupdecl : AttlistDecl" }, // 8
  { 4, 1, "", "markupdecl : EntityDecl" }, // 9
  { 4, 1, "", "markupdecl : NotationDecl" }, // 10
  { 4, 1, "", "markupdecl : PI" }, // 11
  { 4, 1, "", "markupdecl : Comment" }, // 12
  { 5, 2, "", "extSubset : TextDecl\077 extSubsetDecl" }, // 13
  { 6, 1, "", "extSubsetDecl : ( markupdecl | conditionalSect | DeclSep )*" }, // 14
  { 7, 7, "elementdecl", "elementdecl : '<!ELEMENT' S Name S contentspec S\077 '>'" }, // 15
  { 8, 1, "", "contentspec : 'EMPTY'" }, // 16
  { 8, 1, "", "contentspec : 'ANY'" }, // 17
  { 8, 1, "", "contentspec : Mixed" }, // 18
  { 8, 1, "", "contentspec : children" }, // 19
  { 9, 2, "children", "children : ( choice | seq ) ( '\077' | '*' | '+' )\077" }, // 20
  { 10, 2, "cp", "cp : ( Name | choice | seq ) ( '\077' | '*' | '+' )\077" }, // 21
  { 11, 6, "choice", "choice : '(' S\077 cp ( S\077 '|' S\077 cp )+ S\077 ')'" }, // 22
  { 12, 6, "seq_list", "seq : '(' S\077 cp ( S\077 ',' S\077 cp )+ S\077 ')'" }, // 23
  { 12, 5, "seq_single", "seq : '(' S\077 cp S\077 ')'" }, // 24
  { 13, 7, "Mixed_names", "Mixed : '(' S\077 '#PCDATA' ( S\077 '|' S\077 Name )+ S\077 ')' '*'" }, // 25
  { 13, 6, "Mixed_pcdata", "Mixed : '(' S\077 '#PCDATA' S\077 ')' '*'\077" }, // 26
  { 14, 6, "AttlistDecl", "AttlistDecl : '<!ATTLIST' S Name AttDef* S\077 '>'" }, // 27
  { 15, 6, "AttDef", "AttDef : S Name S AttType S DefaultDecl" }, // 28
  { 16, 1, "", "AttType : StringType" }, // 29
  { 16, 1, "", "AttType : TokenizedType" }, // 30
  { 16, 1, "", "AttType : EnumeratedType" }, // 31
  { 17, 1, "", "StringType : 'CDATA'" }, // 32
  { 18, 1, "", "TokenizedType : 'ID'" }, // 33
  { 18, 1, "", "TokenizedType : 'IDREF'" }, // 34
  { 18, 1, "", "TokenizedType : 'IDREFS'" }, // 35
  { 18, 1, "", "TokenizedType : 'ENTITY'" }, // 36
  { 18, 1, "", "TokenizedType : 'ENTITIES'" }, // 37
  { 18, 1, "", "TokenizedType : 'NMTOKEN'" }, // 38
  { 18, 1, "", "TokenizedType : 'NMTOKENS'" }, // 39
  { 19, 1, "", "EnumeratedType : NotationType" }, // 40
  { 19, 1, "", "EnumeratedType : Enumeration" }, // 41
  { 20, 8, "NotationType", "NotationType : 'NOTATION' S '(' S\077 Name ( S\077 '|' S\077 Name )* S\077 ')'" }, // 42
  { 21, 6, "Enumeration", "Enumeration : '(' S\077 Nmtoken ( S\077 '|' S\077 Nmtoken )* S\077 ')'" }, // 43
  { 22, 1, "", "DefaultDecl : '#REQUIRED'" }, // 44
  { 22, 1, "", "DefaultDecl : '#IMPLIED'" }, // 45
  { 22, 2, "DefaultDecl_value", "DefaultDecl : ( '#FIXED' S )\077 AttValue" }, // 46
  { 23, 1, "", "conditionalSect : includeSect" }, // 47
  { 23, 1, "", "conditionalSect : ignoreSect" }, // 48
  { 24, 7, "", "includeSect : '<![' S\077 'INCLUDE' S\077 '[' extSubsetDecl ']]>'" }, // 49
  { 25, 7, "", "ignoreSect : '<![' S\077 'IGNORE' S\077 '[' ignoreSectContents ']]>'" }, // 50
  { 26, 2, "", "ignoreSectContents : Ignore* ( '<![' ignoreSectContents ']]>' Ignore* )*" }, // 51
  { 27, 1, "", "EntityDecl : GEDecl" }, // 52
  { 27, 1, "", "EntityDecl : PEDecl" }, // 53
  { 28, 6, "GEDecl", "GEDecl : '<!ENTITY' S Name S EntityDef '>'" }, // 54
  { 29, 9, "PEDecl", "PEDecl : '<!ENTITY' S '%' S Name S PEDef S\077 '>'" }, // 55
  { 30, 2, "EntityDef_value", "EntityDef : EntityValue S\077" }, // 56
  { 30, 2, "EntityDef_external", "EntityDef : ExternalID S\077" }, // 57
  { 30, 3, "EntityDef_unparsed", "EntityDef : ExternalID NDataDecl S\077" }, // 58
  { 31, 1, "", "PEDef : EntityValue" }, // 59
  { 31, 1, "", "PEDef : ExternalID" }, // 60
  { 32, 3, "ExternalID_system", "ExternalID : 'SYSTEM' S SystemLiteral" }, // 61
  { 32, 3, "ExternalID_public", "ExternalID : PublicID S SystemLiteral" }, // 62
  { 33, 4, "NDataDecl", "NDataDecl : S 'NDATA' S Name" }, // 63
  { 34, 6, "NotationDecl", "NotationDecl : '<!NOTATION' S Name S ( ExternalID S\077 | PublicID S\077 ) '>'" }, // 64
  { 35, 3, "PublicID", "PublicID : 'PUBLIC' S PubidLiteral" }, // 65
  { 36, 3, "", "( S ExternalID S\077 | S\077 ) : S ExternalID S\077", EParseProductionKind::eppkGroup, 0 }, // 66
  { 36, 1, "", "( S ExternalID S\077 | S\077 ) : S\077", EParseProductionKind::eppkGroup, 1 }, // 67
  { 37, 0, "", "S\077 : \316\265", EParseProductionKind::eppkOptionEmpty, 0 }, // 68
  { 37, 1, "", "S\077 : S", EParseProductionKind::eppkOptionPresent, 0 }, // 69
  { 38, 4, "", "( '[' intSubset ']' S\077 ) : '[' intSubset ']' S\077", EParseProductionKind::eppkGroup, 0 }, // 70
  { 39, 0, "", "( '[' intSubset ']' S\077 )\077 : \316\265", EParseProductionKind::eppkOptionEmpty, 0 }, // 71
  { 39, 1, "", "( '[' intSubset ']' S\077 )\077 : ( '[' intSubset ']' S\077 )", EParseProductionKind::eppkOptionPresent, 0 }, // 72
  { 40, 1, "", "( markupdecl | DeclSep ) : markupdecl", EParseProductionKind::eppkGroup, 0 }, // 73
  { 40, 1, "", "( markupdecl | DeclSep ) : DeclSep", EParseProductionKind::eppkGroup, 1 }, // 74
  { 41, 0, "", "( markupdecl | DeclSep )* : \316\265", EParseProductionKind::eppkListEmpty, 0 }, // 75
  { 41, 2, "", "( markupdecl | DeclSep )* : ( markupdecl | DeclSep )* ( markupdecl | DeclSep )", EParseProductionKind::eppkListAppend, 0 }, // 76
  { 42, 0, "", "TextDecl\077 : \316\265", EParseProductionKind::eppkOptionEmpty, 0 }, // 77
  { 42, 1, "", "TextDecl\077 : TextDecl", EParseProductionKind::eppkOptionPresent, 0 }, // 78
  { 43, 1, "", "( markupdecl | conditionalSect | DeclSep ) : markupdecl", EParseProductionKind::eppkGroup, 0 }, // 79
  { 43, 1, "", "( markupdecl | conditionalSect | DeclSep ) : conditionalSect", EParseProductionKind::eppkGroup, 1 }, // 80
  { 43, 1, "", "( markupdecl | conditionalSect | DeclSep ) : DeclSep", EParseProductionKind::eppkGroup, 2 }, // 81
  { 44, 0, "", "( markupdecl | conditionalSect | DeclSep )* : \316\265", EParseProductionKind::eppkListEmpty, 0 }, // 82
  { 44, 2, "", "( markupdecl | conditionalSect | DeclSep )* : ( markupdecl | conditionalSect | DeclSep )* ( markupdecl | conditionalSect | DeclSep )", EParseProductionKind::eppkListAppend, 0 }, // 83
  { 45, 1, "", "( choice | seq ) : choice", EParseProductionKind::eppkGroup, 0 }, // 84
  { 45, 1, "", "( choice | seq ) : seq", EParseProductionKind::eppkGroup, 1 }, // 85
  { 46, 1, "", "( '\077' | '*' | '+' ) : '\077'", EParseProductionKind::eppkGroup, 0 }, // 86
  { 46, 1, "", "( '\077' | '*' | '+' ) : '*'", EParseProductionKind::eppkGroup, 1 }, // 87
  { 46, 1, "", "( '\077' | '*' | '+' ) : '+'", EParseProductionKind::eppkGroup, 2 }, // 88
  { 47, 0, "", "( '\077' | '*' | '+' )\077 : \316\265", EParseProductionKind::eppkOptionEmpty, 0 }, // 89
  { 47, 1, "", "( '\077' | '*' | '+' )\077 : ( '\077' | '*' | '+' )", EParseProductionKind::eppkOptionPresent, 0 }, // 90
  { 48, 1, "", "( Name | choice | seq ) : Name", EParseProductionKind::eppkGroup, 0 }, // 91
  { 48, 1, "", "( Name | choice | seq ) : choice", EParseProductionKind::eppkGroup, 1 }, // 92
  { 48, 1, "", "( Name | choice | seq ) : seq", EParseProductionKind::eppkGroup, 2 }, // 93
  { 49, 4, "", "( S\077 '|' S\077 cp ) : S\077 '|' S\077 cp", EParseProductionKind::eppkGroup, 0 }, // 94
  { 50, 1, "", "( S\077 '|' S\077 cp )+ : ( S\077 '|' S\077 cp )", EParseProductionKind::eppkListFirst, 0 }, // 95
  { 50, 2, "", "( S\077 '|' S\077 cp )+ : ( S\077 '|' S\077 cp )+ ( S\077 '|' S\077 cp )", EParseProductionKind::eppkListAppend, 0 }, // 96
  { 51, 4, "", "( S\077 ',' S\077 cp ) : S\077 ',' S\077 cp", EParseProductionKind::eppkGroup, 0 }, // 97
  { 52, 1, "", "( S\077 ',' S\077 cp )+ : ( S\077 ',' S\077 cp )", EParseProductionKind::eppkListFirst, 0 }, // 98
  { 52, 2, "", "( S\077 ',' S\077 cp )+ : ( S\077 ',' S\077 cp )+ ( S\077 ',' S\077 cp )", EParseProductionKind::eppkListAppend, 0 }, // 99
  { 53, 4, "", "( S\077 '|' S\077 Name ) : S\077 '|' S\077 Name", EParseProductionKind::eppkGroup, 0 }, // 100
  { 54, 1, "", "( S\077 '|' S\077 Name )+ : ( S\077 '|' S\077 Name )", EParseProductionKind::eppkListFirst, 0 }, // 101
  { 54, 2, "", "( S\077 '|' S\077 Name )+ : ( S\077 '|' S\077 Name )+ ( S\077 '|' S\077 Name )", EParseProductionKind::eppkListAppend, 0 }, // 102
  { 55, 0, "", "'*'\077 : \316\265", EParseProductionKind::eppkOptionEmpty, 0 }, // 103
  { 55, 1, "", "'*'\077 : '*'", EParseProductionKind::eppkOptionPresent, 0 }, // 104
  { 56, 0, "", "AttDef* : \316\265", EParseProductionKind::eppkListEmpty, 0 }, // 105
  { 56, 2, "", "AttDef* : AttDef* AttDef", EParseProductionKind::eppkListAppend, 0 }, // 106
  { 57, 0, "", "( S\077 '|' S\077 Name )* : \316\265", EParseProductionKind::eppkListEmpty, 0 }, // 107
  { 57, 2, "", "( S\077 '|' S\077 Name )* : ( S\077 '|' S\077 Name )* ( S\077 '|' S\077 Name )", EParseProductionKind::eppkListAppend, 0 }, // 108
  { 58, 4, "", "( S\077 '|' S\077 Nmtoken ) : S\077 '|' S\077 Nmtoken", EParseProductionKind::eppkGroup, 0 }, // 109
  { 59, 0, "", "( S\077 '|' S\077 Nmtoken )* : \316\265", EParseProductionKind::eppkListEmpty, 0 }, // 110
  { 59, 2, "", "( S\077 '|' S\077 Nmtoken )* : ( S\077 '|' S\077 Nmtoken )* ( S\077 '|' S\077 Nmtoken )", EParseProductionKind::eppkListAppend, 0 }, // 111
  { 60, 2, "", "( '#FIXED' S ) : '#FIXED' S", EParseProductionKind::eppkGroup, 0 }, // 112
  { 61, 0, "", "( '#FIXED' S )\077 : \316\265", EParseProductionKind::eppkOptionEmpty, 0 }, // 113
  { 61, 1, "", "( '#FIXED' S )\077 : ( '#FIXED' S )", EParseProductionKind::eppkOptionPresent, 0 }, // 114
  { 62, 0, "", "Ignore* : \316\265", EParseProductionKind::eppkListEmpty, 0 }, // 115
  { 62, 2, "", "Ignore* : Ignore* Ignore", EParseProductionKind::eppkListAppend, 0 }, // 116
  { 63, 4, "", "( '<![' ignoreSectContents ']]>' Ignore* ) : '<![' ignoreSectContents ']]>' Ignore*", EParseProductionKind::eppkGroup, 0 }, // 117
  { 64, 0, "", "( '<![' ignoreSectContents ']]>' Ignore* )* : \316\265", EParseProductionKind::eppkListEmpty, 0 }, // 118
  { 64, 2, "", "( '<![' ignoreSectContents ']]>' Ignore* )* : ( '<![' ignoreSectContents ']]>' Ignore* )* ( '<![' ignoreSectContents ']]>' Ignore* )", EParseProductionKind::eppkListAppend, 0 }, // 119
  { 65, 2, "", "( ExternalID S\077 | PublicID S\077 ) : ExternalID S\077", EParseProductionKind::eppkGroup, 0 }, // 120
  { 65, 2, "", "( ExternalID S\077 | PublicID S\077 ) : PublicID S\077", EParseProductionKind::eppkGroup, 1 }, // 121
};
// The terminal of each token - in order of token identifier.
inline constexpr ParseTokenTerminal< int32_t > s_rgParseTokens[] = {
  { 1003, 9 }, // Comment
  { 1007, 10 }, // PI
  { 4000, 2 }, // S
  { 4001, 3 }, // Name
  { 4002, 4 }, // Nmtoken
  { 4003, 14 }, // '<!DOCTYPE'
  { 4004, 15 }, // '<!ELEMENT'
  { 4005, 16 }, // '<!ATTLIST'
  { 4006, 17 }, // '<!ENTITY'
  { 4007, 18 }, // '<!NOTATION'
  { 4008, 21 }, // '['
  { 4009, 22 }, // ']'
  { 4010, 23 }, // '>'
  { 4011, 24 }, // '('
  { 4012, 25 }, // ')'
  { 4013, 26 }, // '|'
  { 4014, 27 }, // ','
  { 4015, 28 }, // '?'
  { 4016, 29 }, // '*'
  { 4017, 30 }, // '+'
  { 4018, 31 }, // '%'
  { 4019, 32 }, // '#PCDATA'
  { 4020, 36 }, // 'EMPTY'
  { 4021, 37 }, // 'ANY'
  { 4022, 38 }, // 'CDATA'
  { 4023, 39 }, // 'ID'
  { 4024, 40 }, // 'IDREF'
  { 4025, 41 }, // 'IDREFS'
  { 4026, 42 }, // 'ENTITY'
  { 4027, 43 }, // 'ENTITIES'
  { 4028, 44 }, // 'NMTOKEN'
  { 4029, 45 }, // 'NMTOKENS'
  { 4030, 46 }, // 'NOTATION'
  { 4031, 33 }, // '#REQUIRED'
  { 4032, 34 }, // '#IMPLIED'
  { 4033, 35 }, // '#FIXED'
  { 4034, 47 }, // 'SYSTEM'
  { 4035, 48 }, // 'PUBLIC'
  { 4036, 49 }, // 'NDATA'
  { 4037, 5 }, // EntityValue
  { 4038, 6 }, // AttValue
  { 4039, 7 }, // SystemLiteral
  { 4040, 8 }, // PubidLiteral
  { 4041, 11 }, // PEReference
  { 4042, 19 }, // '<!['
  { 4043, 20 }, // ']]>'
  { 4044, 50 }, // 'INCLUDE'
  { 4045, 51 }, // 'IGNORE'
  { 4046, 13 }, // Ignore
  { 4047, 12 }, // TextDecl
  { 2147483647, 1 }, // $end
};
inline constexpr const char * s_rgpszParseTerminalNames[] = {
  "", "$end", "S", "Name", "Nmtoken", "EntityValue", "AttValue", "SystemLiteral", "PubidLiteral", "Comment", "PI", "PEReference", "TextDecl", "Ignore", "'<!DOCTYPE'", "'<!ELEMENT'", "'<!ATTLIST'", "'<!ENTITY'", "'<!NOTATION'", "'<!['", "']]>'", "'['", "']'", "'>'", "'('", "')'", "'|'", "','", "'\077'", "'*'", "'+'", "'%'", "'#PCDATA'", "'#REQUIRED'", "'#IMPLIED'", "'#FIXED'", "'EMPTY'", "'ANY'", "'CDATA'", "'ID'", "'IDREF'", "'IDREFS'", "'ENTITY'", "'ENTITIES'", "'NMTOKEN'", "'NMTOKENS'", "'NOTATION'", "'SYSTEM'", "'PUBLIC'", "'NDATA'", "'INCLUDE'", "'IGNORE'", "$entry:doctypedecl", "$entry:extSubset", "$entry:extSubsetDecl",
};
inline constexpr const char * s_rgpszParseNonTerminalNames[] = {
  "$accept", "doctypedecl", "DeclSep", "intSubset", "markupdecl", "extSubset", "extSubsetDecl", "elementdecl", "contentspec", "children", "cp", "choice", "seq", "Mixed", "AttlistDecl", "AttDef", "AttType", "StringType", "TokenizedType", "EnumeratedType", "NotationType", "Enumeration", "DefaultDecl", "conditionalSect", "includeSect", "ignoreSect", "ignoreSectContents", "EntityDecl", "GEDecl", "PEDecl", "EntityDef", "PEDef", "ExternalID", "NDataDecl", "NotationDecl", "PublicID", "( S ExternalID S\077 | S\077 )", "S\077", "( '[' intSubset ']' S\077 )", "( '[' intSubset ']' S\077 )\077", "( markupdecl | DeclSep )", "( markupdecl | DeclSep )*", "TextDecl\077", "( markupdecl | conditionalSect | DeclSep )", "( markupdecl | conditionalSect | DeclSep )*", "( choice | seq )", "( '\077' | '*' | '+' )", "( '\077' | '*' | '+' )\077", "( Name | choice | seq )", "( S\077 '|' S\077 cp )", "( S\077 '|' S\077 cp )+", "( S\077 ',' S\077 cp )", "( S\077 ',' S\077 cp )+", "( S\077 '|' S\077 Name )", "( S\077 '|' S\077 Name )+", "'*'\077", "AttDef*", "( S\077 '|' S\077 Name )*", "( S\077 '|' S\077 Nmtoken )", "( S\077 '|' S\077 Nmtoken )*", "( '#FIXED' S )", "( '#FIXED' S )\077", "Ignore*", "( '<![' ignoreSectContents ']]>' Ignore* )", "( '<![' ignoreSectContents ']]>' Ignore* )*", "( ExternalID S\077 | PublicID S\077 )",
};
// The modes of the lexical analyzer to try for the next token in each state: the first in s_rgParseLexerModes and the number.
inline constexpr ParseStateLexerModes s_rgParseStateLexerModes[] = {
  { 0, 1 }, // 0: DtdTextDecl
  { 0, 1 }, // 1: DtdTextDecl
  { 1, 2 }, // 2: DtdTextDecl Dtd
  { 3, 1 }, // 3: Dtd
  { 0, 1 }, // 4: DtdTextDecl
  { 3, 1 }, // 5: Dtd
  { 0, 1 }, // 6: DtdTextDecl
  { 0, 1 }, // 7: DtdTextDecl
  { 0, 1 }, // 8: DtdTextDecl
  { 3, 1 }, // 9: Dtd
  { 0, 1 }, // 10: DtdTextDecl
  { 4, 2 }, // 11: DtdKeyword Dtd
  { 3, 1 }, // 12: Dtd
  { 3, 1 }, // 13: Dtd
  { 3, 1 }, // 14: Dtd
  { 3, 1 }, // 15: Dtd
  { 0, 1 }, // 16: DtdTextDecl
  { 0, 1 }, // 17: DtdTextDecl
  { 0, 1 }, // 18: DtdTextDecl
  { 0, 1 }, // 19: DtdTextDecl
  { 0, 1 }, // 20: DtdTextDecl
  { 0, 1 }, // 21: DtdTextDecl
  { 0, 1 }, // 22: DtdTextDecl
  { 0, 1 }, // 23: DtdTextDecl
  { 0, 1 }, // 24: DtdTextDecl
  { 0, 1 }, // 25: DtdTextDecl
  { 0, 1 }, // 26: DtdTextDecl
  { 0, 1 }, // 27: DtdTextDecl
  { 0, 1 }, // 28: DtdTextDecl
  { 0, 1 }, // 29: DtdTextDecl
  { 0, 1 }, // 30: DtdTextDecl
  { 0, 1 }, // 31: DtdTextDecl
  { 0, 1 }, // 32: DtdTextDecl
  { 3, 1 }, // 33: Dtd
  { 0, 1 }, // 34: DtdTextDecl
  { 6, 1 }, // 35: DtdKeyword
  { 3, 1 }, // 36: Dtd
  { 3, 1 }, // 37: Dtd
  { 3, 1 }, // 38: Dtd
  { 3, 1 }, // 39: Dtd
  { 3, 1 }, // 40: Dtd
  { 3, 1 }, // 41: Dtd
  { 3, 1 }, // 42: Dtd
  { 3, 1 }, // 43: Dtd
  { 3, 1 }, // 44: Dtd
  { 3, 1 }, // 45: Dtd
  { 0, 1 }, // 46: DtdTextDecl
  { 3, 1 }, // 47: Dtd
  { 4, 2 }, // 48: DtdKeyword Dtd
  { 3, 1 }, // 49: Dtd
  { 0, 1 }, // 50: DtdTextDecl
  { 3, 1 }, // 51: Dtd
  { 3, 1 }, // 52: Dtd
  { 6, 1 }, // 53: DtdKeyword
  { 3, 1 }, // 54: Dtd
  { 7, 2 }, // 55: DtdKeyword DtdEntityValue
  { 3, 1 }, // 56: Dtd
  { 4, 2 }, // 57: DtdKeyword Dtd
  { 3, 1 }, // 58: Dtd
  { 3, 1 }, // 59: Dtd
  { 3, 1 }, // 60: Dtd
  { 3, 1 }, // 61: Dtd
  { 0, 1 }, // 62: DtdTextDecl
  { 0, 1 }, // 63: DtdTextDecl
  { 3, 1 }, // 64: Dtd
  { 0, 1 }, // 65: DtdTextDecl
  { 0, 1 }, // 66: DtdTextDecl
  { 3, 1 }, // 67: Dtd
  { 3, 1 }, // 68: Dtd
  { 3, 1 }, // 69: Dtd
  { 3, 1 }, // 70: Dtd
  { 3, 1 }, // 71: Dtd
  { 3, 1 }, // 72: Dtd
  { 3, 1 }, // 73: Dtd
  { 3, 1 }, // 74: Dtd
  { 0, 1 }, // 75: DtdTextDecl
  { 3, 1 }, // 76: Dtd
  { 0, 1 }, // 77: DtdTextDecl
  { 0, 1 }, // 78: DtdTextDecl
  { 3, 1 }, // 79: Dtd
  { 3, 1 }, // 80: Dtd
  { 0, 1 }, // 81: DtdTextDecl
  { 0, 1 }, // 82: DtdTextDecl
  { 0, 1 }, // 83: DtdTextDecl
  { 0, 1 }, // 84: DtdTextDecl
  { 3, 1 }, // 85: Dtd
  { 9, 1 }, // 86: DtdPubidLiteral
  { 10, 1 }, // 87: DtdSystemLiteral
  { 0, 1 }, // 88: DtdTextDecl
  { 10, 1 }, // 89: DtdSystemLiteral
  { 3, 1 }, // 90: Dtd
  { 3, 1 }, // 91: Dtd
  { 0, 1 }, // 92: DtdTextDecl
  { 3, 1 }, // 93: Dtd
  { 11, 1 }, // 94: DtdIgnore
  { 3, 1 }, // 95: Dtd
  { 0, 1 }, // 96: DtdTextDecl
  { 12, 2 }, // 97: DtdSystemLiteral Dtd
  { 0, 1 }, // 98: DtdTextDecl
  { 0, 1 }, // 99: DtdTextDecl
  { 7, 2 }, // 100: DtdKeyword DtdEntityValue
  { 0, 1 }, // 101: DtdTextDecl
  { 0, 1 }, // 102: DtdTextDecl
  { 4, 2 }, // 103: DtdKeyword Dtd
  { 3, 1 }, // 104: Dtd
  { 0, 1 }, // 105: DtdTextDecl
  { 3, 1 }, // 106: Dtd
  { 0, 1 }, // 107: DtdTextDecl
  { 3, 1 }, // 108: Dtd
  { 3, 1 }, // 109: Dtd
  { 0, 1 }, // 110: DtdTextDecl
  { 0, 1 }, // 111: DtdTextDecl
  { 0, 1 }, // 112: DtdTextDecl
  { 0, 1 }, // 113: DtdTextDecl
  { 0, 1 }, // 114: DtdTextDecl
  { 0, 1 }, // 115: DtdTextDecl
  { 0, 1 }, // 116: DtdTextDecl
  { 0, 1 }, // 117: DtdTextDecl
  { 3, 1 }, // 118: Dtd
  { 0, 1 }, // 119: DtdTextDecl
  { 0, 1 }, // 120: DtdTextDecl
  { 0, 1 }, // 121: DtdTextDecl
  { 0, 1 }, // 122: DtdTextDecl
  { 0, 1 }, // 123: DtdTextDecl
  { 3, 1 }, // 124: Dtd
  { 0, 1 }, // 125: DtdTextDecl
  { 0, 1 }, // 126: DtdTextDecl
  { 3, 1 }, // 127: Dtd
  { 0, 1 }, // 128: DtdTextDecl
  { 3, 1 }, // 129: Dtd
  { 0, 1 }, // 130: DtdTextDecl
  { 4, 2 }, // 131: DtdKeyword Dtd
  { 3, 1 }, // 132: Dtd
  { 3, 1 }, // 133: Dtd
  { 0, 1 }, // 134: DtdTextDecl
  { 3, 1 }, // 135: Dtd
  { 0, 1 }, // 136: DtdTextDecl
  { 0, 1 }, // 137: DtdTextDecl
  { 3, 1 }, // 138: Dtd
  { 0, 1 }, // 139: DtdTextDecl
  { 0, 1 }, // 140: DtdTextDecl
  { 0, 1 }, // 141: DtdTextDecl
  { 0, 1 }, // 142: DtdTextDecl
  { 3, 1 }, // 143: Dtd
  { 3, 1 }, // 144: Dtd
  { 3, 1 }, // 145: Dtd
  { 0, 1 }, // 146: DtdTextDecl
  { 0, 1 }, // 147: DtdTextDecl
  { 0, 1 }, // 148: DtdTextDecl
  { 0, 1 }, // 149: DtdTextDecl
  { 0, 1 }, // 150: DtdTextDecl
  { 0, 1 }, // 151: DtdTextDecl
  { 0, 1 }, // 152: DtdTextDecl
  { 0, 1 }, // 153: DtdTextDecl
  { 14, 2 }, // 154: DtdNmtoken Dtd
  { 3, 1 }, // 155: Dtd
  { 0, 1 }, // 156: DtdTextDecl
  { 0, 1 }, // 157: DtdTextDecl
  { 0, 1 }, // 158: DtdTextDecl
  { 0, 1 }, // 159: DtdTextDecl
  { 0, 1 }, // 160: DtdTextDecl
  { 3, 1 }, // 161: Dtd
  { 0, 1 }, // 162: DtdTextDecl
  { 3, 1 }, // 163: Dtd
  { 3, 1 }, // 164: Dtd
  { 3, 1 }, // 165: Dtd
  { 0, 1 }, // 166: DtdTextDecl
  { 3, 1 }, // 167: Dtd
  { 0, 1 }, // 168: DtdTextDecl
  { 3, 1 }, // 169: Dtd
  { 0, 1 }, // 170: DtdTextDecl
  { 3, 1 }, // 171: Dtd
  { 0, 1 }, // 172: DtdTextDecl
  { 0, 1 }, // 173: DtdTextDecl
  { 3, 1 }, // 174: Dtd
  { 16, 1 }, // 175: DtdNmtoken
  { 17, 2 }, // 176: DtdAttValue Dtd
  { 3, 1 }, // 177: Dtd
  { 3, 1 }, // 178: Dtd
  { 3, 1 }, // 179: Dtd
  { 0, 1 }, // 180: DtdTextDecl
  { 3, 1 }, // 181: Dtd
  { 3, 1 }, // 182: Dtd
  { 0, 1 }, // 183: DtdTextDecl
  { 3, 1 }, // 184: Dtd
  { 0, 1 }, // 185: DtdTextDecl
  { 3, 1 }, // 186: Dtd
  { 0, 1 }, // 187: DtdTextDecl
  { 0, 1 }, // 188: DtdTextDecl
  { 3, 1 }, // 189: Dtd
  { 0, 1 }, // 190: DtdTextDecl
  { 3, 1 }, // 191: Dtd
  { 0, 1 }, // 192: DtdTextDecl
  { 0, 1 }, // 193: DtdTextDecl
  { 0, 1 }, // 194: DtdTextDecl
  { 0, 1 }, // 195: DtdTextDecl
  { 19, 1 }, // 196: DtdAttValue
  { 3, 1 }, // 197: Dtd
  { 0, 1 }, // 198: DtdTextDecl
  { 0, 1 }, // 199: DtdTextDecl
  { 3, 1 }, // 200: Dtd
  { 3, 1 }, // 201: Dtd
  { 3, 1 }, // 202: Dtd
  { 0, 1 }, // 203: DtdTextDecl
  { 0, 1 }, // 204: DtdTextDecl
  { 11, 1 }, // 205: DtdIgnore
  { 3, 1 }, // 206: Dtd
  { 3, 1 }, // 207: Dtd
  { 0, 1 }, // 208: DtdTextDecl
  { 0, 1 }, // 209: DtdTextDecl
  { 0, 1 }, // 210: DtdTextDecl
  { 0, 1 }, // 211: DtdTextDecl
  { 0, 1 }, // 212: DtdTextDecl
  { 0, 1 }, // 213: DtdTextDecl
  { 0, 1 }, // 214: DtdTextDecl
  { 3, 1 }, // 215: Dtd
  { 0, 1 }, // 216: DtdTextDecl
  { 3, 1 }, // 217: Dtd
  { 14, 2 }, // 218: DtdNmtoken Dtd
  { 0, 1 }, // 219: DtdTextDecl
  { 3, 1 }, // 220: Dtd
  { 0, 1 }, // 221: DtdTextDecl
  { 16, 1 }, // 222: DtdNmtoken
  { 0, 1 }, // 223: DtdTextDecl
  { 0, 1 }, // 224: DtdTextDecl
};
inline constexpr uint32_t s_rgParseLexerModes[] = {
  0, 0, 7, 7, 1, 7, 1, 1, 3, 6, 5, 8, 5, 7, 2, 7, 2, 4, 7, 4,
};
inline constexpr const char * s_rgpszParseLexerModeNames[] = {
  "DtdTextDecl", "DtdKeyword", "DtdNmtoken", "DtdEntityValue", "DtdAttValue", "DtdSystemLiteral", "DtdPubidLiteral", "Dtd", "DtdIgnore",
};
// The state in which a parse of each entry starts.
inline constexpr uint32_t s_rgParseEntryStates[] = {
  3, 2, 1,
};
// Lazy lookahead: for each state the reduction which is done without reading a token - 0 if a token is needed.
inline constexpr int16_t s_rgParseDefaultActions[] = {
  0, -83, 0, 0, -1, 0, -79, -1, -83, 0, -1, 0, 0, 0, 0, 0, -5, -12, -13, -6,
  -82, -80, -8, -9, -81, -48, -49, -10, -53, -54, -11, -84, -14, 0, -70, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, -106, 0, 0, 0, -68, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, -76, -73, 0, -116, -83, 0, 0, 0, 0, 0, 0, 0, 0, -107, 0, -18, -17, 0,
  0, -20, -85, -86, -19, 0, 0, 0, -67, 0, 0, 0, -4, 0, 0, 0, -121, 0, -122, -65,
  0, -57, -55, 0, 0, -58, 0, -28, 0, 0, -89, -88, -87, -91, -21, -66, -62, -63, 0, -75,
  -74, -77, -51, -117, 0, -50, -60, 0, -61, 0, -59, 0, 0, 0, -92, 0, -93, -94, 0, -16,
  -71, -116, -120, 0, 0, 0, -40, -39, -38, -37, -36, -35, -34, -33, 0, 0, -30, -31, -32, -41,
  -42, 0, -102, 0, 0, 0, -96, 0, -99, 0, -22, 0, -56, -64, 0, 0, 0, 0, 0, 0,
  -103, 0, 0, -25, 0, -97, 0, -100, -116, 0, -111, 0, -46, -45, -29, -115, 0, 0, -105, -27,
  0, 0, 0, -23, -24, 0, 0, 0, -113, -47, -101, -26, -98, -95, -108, 0, -112, 0, 0, -44,
  0, -109, 0, -43, -110,
};
// The parse table: states, terminals, non-terminals, productions, tokens, the start state and the arrays.
inline constexpr ParseTableView< int32_t, int16_t > s_viewParseTable = {
  225, 55, 66, 122, 51, 0,
  s_rgParseActionBase, s_rgParseActionCheck, s_rgParseActions, s_rgParseGotoBase, s_rgParseGotoCheck, s_rgParseGotos,
  s_rgParseProductions, s_rgParseTokens, s_rgpszParseTerminalNames, s_rgpszParseNonTerminalNames,
  9, s_rgParseStateLexerModes, s_rgParseLexerModes, s_rgpszParseLexerModeNames,
  3, s_rgParseEntryStates,
  s_rgParseDefaultActions
};

// _parser_dispatch: Calls the method of the handler for the production which is reduced.
// The handler has these methods - each returns the value of the non-terminal and has a _TyValue & parameter for each symbol:
//    _TyValue doctypedecl( _TyValue & /* '<!DOCTYPE' */, _TyValue & /* S */, _TyValue & /* Name */, _TyValue & /* ( S ExternalID S? | S? ) */, _TyValue & /* ( '[' intSubset ']' S? )? */, _TyValue & /* '>' */ )  -  doctypedecl : '<!DOCTYPE' S Name ( S ExternalID S? | S? ) ( '[' intSubset ']' S? )? '>'
//    _TyValue DeclSep_PEReference( _TyValue & /* PEReference */ )  -  DeclSep : PEReference
//    _TyValue elementdecl( _TyValue & /* '<!ELEMENT' */, _TyValue & /* S */, _TyValue & /* Name */, _TyValue & /* S */, _TyValue & /* contentspec */, _TyValue & /* S? */, _TyValue & /* '>' */ )  -  elementdecl : '<!ELEMENT' S Name S contentspec S? '>'
//    _TyValue children( _TyValue & /* ( choice | seq ) */, _TyValue & /* ( '?' | '*' | '+' )? */ )  -  children : ( choice | seq ) ( '?' | '*' | '+' )?
//    _TyValue cp( _TyValue & /* ( Name | choice | seq ) */, _TyValue & /* ( '?' | '*' | '+' )? */ )  -  cp : ( Name | choice | seq ) ( '?' | '*' | '+' )?
//    _TyValue choice( _TyValue & /* '(' */, _TyValue & /* S? */, _TyValue & /* cp */, _TyValue & /* ( S? '|' S? cp )+ */, _TyValue & /* S? */, _TyValue & /* ')' */ )  -  choice : '(' S? cp ( S? '|' S? cp )+ S? ')'
//    _TyValue seq_list( _TyValue & /* '(' */, _TyValue & /* S? */, _TyValue & /* cp */, _TyValue & /* ( S? ',' S? cp )+ */, _TyValue & /* S? */, _TyValue & /* ')' */ )  -  seq : '(' S? cp ( S? ',' S? cp )+ S? ')'
//    _TyValue seq_single( _TyValue & /* '(' */, _TyValue & /* S? */, _TyValue & /* cp */, _TyValue & /* S? */, _TyValue & /* ')' */ )  -  seq : '(' S? cp S? ')'
//    _TyValue Mixed_names( _TyValue & /* '(' */, _TyValue & /* S? */, _TyValue & /* '#PCDATA' */, _TyValue & /* ( S? '|' S? Name )+ */, _TyValue & /* S? */, _TyValue & /* ')' */, _TyValue & /* '*' */ )  -  Mixed : '(' S? '#PCDATA' ( S? '|' S? Name )+ S? ')' '*'
//    _TyValue Mixed_pcdata( _TyValue & /* '(' */, _TyValue & /* S? */, _TyValue & /* '#PCDATA' */, _TyValue & /* S? */, _TyValue & /* ')' */, _TyValue & /* '*'? */ )  -  Mixed : '(' S? '#PCDATA' S? ')' '*'?
//    _TyValue AttlistDecl( _TyValue & /* '<!ATTLIST' */, _TyValue & /* S */, _TyValue & /* Name */, _TyValue & /* AttDef* */, _TyValue & /* S? */, _TyValue & /* '>' */ )  -  AttlistDecl : '<!ATTLIST' S Name AttDef* S? '>'
//    _TyValue AttDef( _TyValue & /* S */, _TyValue & /* Name */, _TyValue & /* S */, _TyValue & /* AttType */, _TyValue & /* S */, _TyValue & /* DefaultDecl */ )  -  AttDef : S Name S AttType S DefaultDecl
//    _TyValue NotationType( _TyValue & /* 'NOTATION' */, _TyValue & /* S */, _TyValue & /* '(' */, _TyValue & /* S? */, _TyValue & /* Name */, _TyValue & /* ( S? '|' S? Name )* */, _TyValue & /* S? */, _TyValue & /* ')' */ )  -  NotationType : 'NOTATION' S '(' S? Name ( S? '|' S? Name )* S? ')'
//    _TyValue Enumeration( _TyValue & /* '(' */, _TyValue & /* S? */, _TyValue & /* Nmtoken */, _TyValue & /* ( S? '|' S? Nmtoken )* */, _TyValue & /* S? */, _TyValue & /* ')' */ )  -  Enumeration : '(' S? Nmtoken ( S? '|' S? Nmtoken )* S? ')'
//    _TyValue DefaultDecl_value( _TyValue & /* ( '#FIXED' S )? */, _TyValue & /* AttValue */ )  -  DefaultDecl : ( '#FIXED' S )? AttValue
//    _TyValue GEDecl( _TyValue & /* '<!ENTITY' */, _TyValue & /* S */, _TyValue & /* Name */, _TyValue & /* S */, _TyValue & /* EntityDef */, _TyValue & /* '>' */ )  -  GEDecl : '<!ENTITY' S Name S EntityDef '>'
//    _TyValue PEDecl( _TyValue & /* '<!ENTITY' */, _TyValue & /* S */, _TyValue & /* '%' */, _TyValue & /* S */, _TyValue & /* Name */, _TyValue & /* S */, _TyValue & /* PEDef */, _TyValue & /* S? */, _TyValue & /* '>' */ )  -  PEDecl : '<!ENTITY' S '%' S Name S PEDef S? '>'
//    _TyValue EntityDef_value( _TyValue & /* EntityValue */, _TyValue & /* S? */ )  -  EntityDef : EntityValue S?
//    _TyValue EntityDef_external( _TyValue & /* ExternalID */, _TyValue & /* S? */ )  -  EntityDef : ExternalID S?
//    _TyValue EntityDef_unparsed( _TyValue & /* ExternalID */, _TyValue & /* NDataDecl */, _TyValue & /* S? */ )  -  EntityDef : ExternalID NDataDecl S?
//    _TyValue ExternalID_system( _TyValue & /* 'SYSTEM' */, _TyValue & /* S */, _TyValue & /* SystemLiteral */ )  -  ExternalID : 'SYSTEM' S SystemLiteral
//    _TyValue ExternalID_public( _TyValue & /* PublicID */, _TyValue & /* S */, _TyValue & /* SystemLiteral */ )  -  ExternalID : PublicID S SystemLiteral
//    _TyValue NDataDecl( _TyValue & /* S */, _TyValue & /* 'NDATA' */, _TyValue & /* S */, _TyValue & /* Name */ )  -  NDataDecl : S 'NDATA' S Name
//    _TyValue NotationDecl( _TyValue & /* '<!NOTATION' */, _TyValue & /* S */, _TyValue & /* Name */, _TyValue & /* S */, _TyValue & /* ( ExternalID S? | PublicID S? ) */, _TyValue & /* '>' */ )  -  NotationDecl : '<!NOTATION' S Name S ( ExternalID S? | PublicID S? ) '>'
//    _TyValue PublicID( _TyValue & /* 'PUBLIC' */, _TyValue & /* S */, _TyValue & /* PubidLiteral */ )  -  PublicID : 'PUBLIC' S PubidLiteral
//    _TyValue OnReduceDefault( vtyParseProduction, std::span< _TyValue > )  -  for the productions which aren't named.
//  and for the repetitions, options and groups of the grammar - see _p_parse.h:
//    _TyValue OnListEmpty(), _TyValue OnListAppend( _TyValue & /* list */, _TyValue & /* element */ ),
//    _TyValue OnOptionEmpty(), _TyValue OnGroup( uint32_t /* alternative */, std::span< _TyValue > )
//    _TyValue OnShift( _TyToken &&, vtyParseTerminal )  -  the value of a terminal.
//    void OnSyntaxError( ParseError const &, const _TyToken * )  -  optional.
template < class t_TyHandler >
class _parser_dispatch
{
public:
  t_TyHandler & m_rhandler;
  explicit _parser_dispatch( t_TyHandler & _rhandler )
    : m_rhandler( _rhandler )
  {
  }
  template < class t_TyToken >
  auto OnShift( t_TyToken && _rrtok, vtyParseTerminal _nTerminal )
  {
    return m_rhandler.OnShift( std::move( _rrtok ), _nTerminal );
  }
  template < class t_TyValue >
  t_TyValue OnReduce( vtyParseProduction _nProduction, std::span< t_TyValue > _spValues )
  {
    switch( _nProduction )
    {
      case 3: // doctypedecl : '<!DOCTYPE' S Name ( S ExternalID S? | S? ) ( '[' intSubset ']' S? )? '>'
        return m_rhandler.doctypedecl( _spValues[0], _spValues[1], _spValues[2], _spValues[3], _spValues[4], _spValues[5] );
      case 4: // DeclSep : PEReference
        return m_rhandler.DeclSep_PEReference( _spValues[0] );
      case 15: // elementdecl : '<!ELEMENT' S Name S contentspec S? '>'
        return m_rhandler.elementdecl( _spValues[0], _spValues[1], _spValues[2], _spValues[3], _spValues[4], _spValues[5], _spValues[6] );
      case 20: // children : ( choice | seq ) ( '?' | '*' | '+' )?
        return m_rhandler.children( _spValues[0], _spValues[1] );
      case 21: // cp : ( Name | choice | seq ) ( '?' | '*' | '+' )?
        return m_rhandler.cp( _spValues[0], _spValues[1] );
      case 22: // choice : '(' S? cp ( S? '|' S? cp )+ S? ')'
        return m_rhandler.choice( _spValues[0], _spValues[1], _spValues[2], _spValues[3], _spValues[4], _spValues[5] );
      case 23: // seq : '(' S? cp ( S? ',' S? cp )+ S? ')'
        return m_rhandler.seq_list( _spValues[0], _spValues[1], _spValues[2], _spValues[3], _spValues[4], _spValues[5] );
      case 24: // seq : '(' S? cp S? ')'
        return m_rhandler.seq_single( _spValues[0], _spValues[1], _spValues[2], _spValues[3], _spValues[4] );
      case 25: // Mixed : '(' S? '#PCDATA' ( S? '|' S? Name )+ S? ')' '*'
        return m_rhandler.Mixed_names( _spValues[0], _spValues[1], _spValues[2], _spValues[3], _spValues[4], _spValues[5], _spValues[6] );
      case 26: // Mixed : '(' S? '#PCDATA' S? ')' '*'?
        return m_rhandler.Mixed_pcdata( _spValues[0], _spValues[1], _spValues[2], _spValues[3], _spValues[4], _spValues[5] );
      case 27: // AttlistDecl : '<!ATTLIST' S Name AttDef* S? '>'
        return m_rhandler.AttlistDecl( _spValues[0], _spValues[1], _spValues[2], _spValues[3], _spValues[4], _spValues[5] );
      case 28: // AttDef : S Name S AttType S DefaultDecl
        return m_rhandler.AttDef( _spValues[0], _spValues[1], _spValues[2], _spValues[3], _spValues[4], _spValues[5] );
      case 42: // NotationType : 'NOTATION' S '(' S? Name ( S? '|' S? Name )* S? ')'
        return m_rhandler.NotationType( _spValues[0], _spValues[1], _spValues[2], _spValues[3], _spValues[4], _spValues[5], _spValues[6], _spValues[7] );
      case 43: // Enumeration : '(' S? Nmtoken ( S? '|' S? Nmtoken )* S? ')'
        return m_rhandler.Enumeration( _spValues[0], _spValues[1], _spValues[2], _spValues[3], _spValues[4], _spValues[5] );
      case 46: // DefaultDecl : ( '#FIXED' S )? AttValue
        return m_rhandler.DefaultDecl_value( _spValues[0], _spValues[1] );
      case 54: // GEDecl : '<!ENTITY' S Name S EntityDef '>'
        return m_rhandler.GEDecl( _spValues[0], _spValues[1], _spValues[2], _spValues[3], _spValues[4], _spValues[5] );
      case 55: // PEDecl : '<!ENTITY' S '%' S Name S PEDef S? '>'
        return m_rhandler.PEDecl( _spValues[0], _spValues[1], _spValues[2], _spValues[3], _spValues[4], _spValues[5], _spValues[6], _spValues[7], _spValues[8] );
      case 56: // EntityDef : EntityValue S?
        return m_rhandler.EntityDef_value( _spValues[0], _spValues[1] );
      case 57: // EntityDef : ExternalID S?
        return m_rhandler.EntityDef_external( _spValues[0], _spValues[1] );
      case 58: // EntityDef : ExternalID NDataDecl S?
        return m_rhandler.EntityDef_unparsed( _spValues[0], _spValues[1], _spValues[2] );
      case 61: // ExternalID : 'SYSTEM' S SystemLiteral
        return m_rhandler.ExternalID_system( _spValues[0], _spValues[1], _spValues[2] );
      case 62: // ExternalID : PublicID S SystemLiteral
        return m_rhandler.ExternalID_public( _spValues[0], _spValues[1], _spValues[2] );
      case 63: // NDataDecl : S 'NDATA' S Name
        return m_rhandler.NDataDecl( _spValues[0], _spValues[1], _spValues[2], _spValues[3] );
      case 64: // NotationDecl : '<!NOTATION' S Name S ( ExternalID S? | PublicID S? ) '>'
        return m_rhandler.NotationDecl( _spValues[0], _spValues[1], _spValues[2], _spValues[3], _spValues[4], _spValues[5] );
      case 65: // PublicID : 'PUBLIC' S PubidLiteral
        return m_rhandler.PublicID( _spValues[0], _spValues[1], _spValues[2] );
      default:
        if ( EParseProductionKind::eppkUser != s_rgParseProductions[ _nProduction ].m_eKind )
          return ReduceAuxiliaryProduction( m_rhandler, s_rgParseProductions[ _nProduction ], _spValues ); // A repetition, an option or a group.
        return m_rhandler.OnReduceDefault( _nProduction, _spValues );
    }
  }
  template < class t_TyToken >
  void OnSyntaxError( ParseError const & _rerror, const t_TyToken * _ptok )
  {
    if constexpr ( requires { m_rhandler.OnSyntaxError( _rerror, _ptok ); } )
      m_rhandler.OnSyntaxError( _rerror, _ptok );
  }
};

// _parser: The parser for the grammar.
template < class t_TyTraits >
class _parser : public LRParser< t_TyTraits, int16_t >
{
  typedef LRParser< t_TyTraits, int16_t > _TyBase;
  typedef _parser _TyThis;
public:
  typedef t_TyTraits _TyTraits;
  using typename _TyBase::_TyToken;
  using typename _TyBase::_TyValue;
  _parser()
    : _TyBase( s_viewParseTable )
  {
  }
  // Parse the tokens of the source - see LRParser<>::FParse() - calling the method of the handler for each production reduced.
  template < class t_TyTokenSource, class t_TyHandler >
  // _nEntry: which of the entries to parse - s_knEntry_<non-terminal>.
  bool FParse( t_TyTokenSource & _rsource, t_TyHandler & _rhandler, _TyValue & _rvalueResult, uint32_t _nEntry = 0 )
  {
    if constexpr ( requires( vtyParseProduction _nProduction, std::span< _TyValue > _spValues ) { _rhandler.OnReduce( _nProduction, _spValues ); } )
      return _TyBase::FParse( _rsource, _rhandler, _rvalueResult, _nEntry ); // The handler handles every production itself.
    else
    {
      _parser_dispatch< t_TyHandler > dispatch( _rhandler );
      return _TyBase::FParse( _rsource, dispatch, _rvalueResult, _nEntry );
    }
  }
};

__XMLPDTD_END_NAMESPACE
