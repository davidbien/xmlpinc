#pragma once

//          Copyright David Lawrence Bien 1997 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt).

// xml_compare.h
// Whether two XML documents are the same document - however each of them is written.
// dbien
// 19NOV2026

// Two documents are compared as they are read by two read cursors - which may be of different XML traits and character
//  types. What is compared is what the documents say and not how they say it:
//  - The tags and how they are within each other, by their names.
//  - The attributes of each tag by name and by value - the value as an application has it: its references replaced and
//    normalized ( 3.3.3 ). The attributes which a DTD gives a tag are attributes of the tag as those which are written
//    in it are. The order of the attributes of a tag doesn't matter unless it is said to.
//  - The character data of each tag where it is among its sub-tags: the characters, however they are written - as
//    themselves, as references or within CDATA sections - and in however many pieces.
//  - Comments and processing instructions where they are, unless they are not to be compared.
//  Not compared: the XML declaration, white space outside of the document's tag, which kind of quote a value has, how
//   an empty element is written - and the document type declaration unless it is to be compared, as text.
// So a document which has a DTD that gives its tags attributes is the same as that document written without the DTD
//  and with those attributes in its tags - when the first is read by a cursor which has the DTD.

#include <algorithm>
#include <deque>
#include <string>
#include <string_view>
#include <vector>
#include "_strutil.h"
#include "xml_ns.h"
#include "xml_types.h"
#include "xml_namespace.h"

__XMLP_BEGIN_NAMESPACE

struct xml_compare_options
{
  bool m_fCompareComments{ true };
  bool m_fCompareProcessingInstructions{ true };
  // The document type declaration is compared as its text - or not at all: what it does to the document is in the document as it is read.
  bool m_fCompareDoctypeDecl{ false };
  // A name with a prefix is the same name as another if the namespaces are the same and the local parts are - and, if
  //  this is true, the prefixes are as well.
  bool m_fComparePrefixes{ true };
  // The attributes which declare namespaces - "xmlns" and "xmlns:prefix" - are compared as the other attributes are.
  bool m_fCompareNamespaceDeclarations{ true };
  bool m_fCompareAttributeOrder{ false };
  // Character data which is all white space is as if it were not there.
  bool m_fIgnoreWhitespaceText{ false };
};

// What a document is made of as it is compared.
enum _EXmlCompareItem : uint8_t
{
  excmpStartTag,
  excmpEndTag,
  excmpText,
  excmpComment,
  excmpProcessingInstruction,
  excmpDoctypeDecl,
  excmpEndOfDocument,
  excmpEXmlCompareItemCount
};
inline const char * _PszXmlCompareItem( _EXmlCompareItem _excmp )
{
  static const char * const s_rgpsz[] = { "a start tag", "an end tag", "character data", "a comment", "a processing instruction", "a document type declaration", "the end of the document" };
  return s_rgpsz[ _excmp ];
}
// Everything is compared in UTF-32 so that the character types of the two documents don't matter.
struct _xml_compare_name
{
  std::u32string m_strUri; // The namespace - empty if none or if namespaces are not in use.
  std::u32string m_strPrefix;
  std::u32string m_strLocalName;
  bool m_fNamespaceDecl{ false }; // An attribute which declares a namespace: m_strLocalName is the prefix which it declares.

  bool FIsSame( _xml_compare_name const & _r, bool _fComparePrefixes ) const
  {
    return ( m_fNamespaceDecl == _r.m_fNamespaceDecl ) && ( m_strUri == _r.m_strUri ) && ( m_strLocalName == _r.m_strLocalName ) &&
           ( !_fComparePrefixes || ( m_strPrefix == _r.m_strPrefix ) );
  }
  // An order for the attributes of a tag. The prefix is last so that it only tells apart what is otherwise the same.
  bool operator < ( _xml_compare_name const & _r ) const
  {
    if ( m_fNamespaceDecl != _r.m_fNamespaceDecl )
      return m_fNamespaceDecl;
    if ( int iComp = m_strUri.compare( _r.m_strUri ) )
      return iComp < 0;
    if ( int iComp = m_strLocalName.compare( _r.m_strLocalName ) )
      return iComp < 0;
    return m_strPrefix < _r.m_strPrefix;
  }
  std::string StrName() const
  {
    std::string str;
    if ( m_fNamespaceDecl )
      str = m_strLocalName.empty() ? "xmlns" : "xmlns:";
    else
    if ( !m_strPrefix.empty() )
      str = StrConvertString< char >( m_strPrefix ) + ":";
    str += StrConvertString< char >( m_strLocalName );
    if ( !m_strUri.empty() )
      str += " {" + StrConvertString< char >( m_strUri ) + "}";
    return str;
  }
};
struct _xml_compare_attr
{
  _xml_compare_name m_name;
  std::u32string m_strValue;
};
struct _xml_compare_item
{
  _EXmlCompareItem m_excmp{ excmpEndOfDocument };
  _xml_compare_name m_name; // excmpStartTag. excmpProcessingInstruction: the target is m_strLocalName.
  std::vector< _xml_compare_attr > m_rgAttrs; // excmpStartTag
  std::u32string m_strText; // The character data, the comment, what follows the target of the processing instruction, the declaration.
};

// _xml_compare_reader:
// The items of a document one after another, read from a read cursor which is at the beginning of the document - as the
//  writer writes a document from a read cursor: see xml_writer<>::NWriteFromReadCursor().
template < class t_TyReadCursor >
class _xml_compare_reader
{
  typedef _xml_compare_reader _TyThis;
public:
  typedef t_TyReadCursor _TyReadCursor;
  typedef typename _TyReadCursor::_TyChar _TyChar;
  typedef typename _TyReadCursor::_TyXmlToken _TyXmlToken;
  typedef typename _TyXmlToken::_TyLexToken _TyLexToken;
  typedef typename _TyXmlToken::_TyLexValue _TyLexValue;
  typedef xml_namespace_value_wrap< _TyChar > _TyXmlNamespaceValueWrap;

  _xml_compare_reader( _TyReadCursor & _rxrc, xml_compare_options const & _roptions )
    : m_rxrc( _rxrc ),
      m_roptions( _roptions )
  {
    VerifyThrowSz( m_rxrc.FInProlog(), "The read cursor must be at the beginning of its document to compare the document." );
  }
  // The next item of the document. Character data is all that there is up to the next item which is compared.
  void GetNext( _xml_compare_item & _ritem )
  {
    for ( ; ; )
    {
      if ( m_dqItems.empty() && !m_fDone )
        _Advance();
      if ( !m_dqItems.empty() && ( excmpText == m_dqItems.front().m_excmp ) )
      {
        m_strTextPending += m_dqItems.front().m_strText;
        m_dqItems.pop_front();
        continue;
      }
      if ( m_dqItems.empty() && !m_fDone )
        continue;
      // An item which is not character data - or the end. First the character data before it.
      if ( !m_strTextPending.empty() )
      {
        const bool kfIgnore = m_roptions.m_fIgnoreWhitespaceText &&
          ( std::u32string::npos == m_strTextPending.find_first_not_of( U"\x20\x09\x0d\x0a" ) );
        if ( !kfIgnore )
        {
          _ritem = _xml_compare_item();
          _ritem.m_excmp = excmpText;
          _ritem.m_strText.swap( m_strTextPending );
          return;
        }
        m_strTextPending.clear();
      }
      if ( m_dqItems.empty() )
      {
        _ritem = _xml_compare_item(); // excmpEndOfDocument
        return;
      }
      _ritem = std::move( m_dqItems.front() );
      m_dqItems.pop_front();
      return;
    }
  }
protected:
  // Move the cursor on and add the items which that gives.
  void _Advance()
  {
    if ( !m_fStarted )
    {
      m_fStarted = true;
      _AddContent(); // That of the prolog.
      return;
    }
    if ( m_rxrc.FMoveDown() )
    {
      _AddStartTag();
      ++m_nDepth;
      _AddContent();
      return;
    }
    if ( 1 == m_nDepth )
    {
      m_fDone = true; // After the document's tag - or there was none.
      return;
    }
    // The current tag has no ( more ) sub-tags: it ends. Then either its next sibling begins or we are in its parent again.
    const bool kfNextTag = m_rxrc.FNextTag();
    m_dqItems.emplace_back().m_excmp = excmpEndTag;
    if ( kfNextTag )
      _AddStartTag();
    else
      --m_nDepth;
    _AddContent();
  }
  template < class t_TyValue >
  static std::u32string _StrValue( _TyLexToken const & _rltok, t_TyValue const & _rval )
  {
    std::u32string str;
    if ( !_rval.FIsNull() && !_rval.FEmptyTypedData() )
      _rval.GetString( _rltok, str );
    return str;
  }
  // The name of a tag or of an attribute: _rrgvalName has the name and its namespace as the cursor leaves them.
  void _GetName( _TyLexToken const & _rltok, _TyLexValue const & _rrgvalName, _xml_compare_name & _rname, bool _fIsAttr ) const
  {
    std::u32string strName = _StrValue( _rltok, _rrgvalName[vknNameIdx] );
    const _TyLexValue & rvalNamespace = _rrgvalName[vknNamespaceIdx];
    const bool kfHasNamespace = rvalNamespace.template FIsA< _TyXmlNamespaceValueWrap >() && !rvalNamespace.template GetVal< _TyXmlNamespaceValueWrap >().FIsNull();
    // The name may have its prefix or not - as the cursor is told.
    const size_t knPosColon = strName.find( U':' );
    if ( std::u32string::npos != knPosColon )
    {
      _rname.m_strPrefix = strName.substr( 0, knPosColon );
      _rname.m_strLocalName = strName.substr( knPosColon + 1 );
    }
    else
      _rname.m_strLocalName = std::move( strName );
    if ( _fIsAttr && ( ( _rname.m_strPrefix == U"xmlns" ) || ( _rname.m_strPrefix.empty() && ( _rname.m_strLocalName == U"xmlns" ) ) ) )
    {
      // A declaration of a namespace: of the prefix which is the local part - or of the default namespace.
      _rname.m_fNamespaceDecl = true;
      if ( _rname.m_strPrefix.empty() )
        _rname.m_strLocalName.clear();
      _rname.m_strPrefix.clear();
      return;
    }
    if ( kfHasNamespace )
    {
      const _TyXmlNamespaceValueWrap & rxnvw = rvalNamespace.template GetVal< _TyXmlNamespaceValueWrap >();
      ConvertString( _rname.m_strUri, rxnvw.RStringUri() );
      _rname.m_strPrefix.clear();
      ConvertString( _rname.m_strPrefix, rxnvw.RStringPrefix() );
    }
  }
  void _AddStartTag()
  {
    const _TyXmlToken & rxtTag = m_rxrc.GetTagCur();
    const _TyLexToken & rltokTag = rxtTag.GetLexToken();
    const _TyLexValue & rvalTag = rltokTag.GetValue();
    _xml_compare_item & ritem = m_dqItems.emplace_back();
    ritem.m_excmp = excmpStartTag;
    _GetName( rltokTag, rvalTag[vknTagNameIdx], ritem.m_name, false );
    const _TyLexValue & rrgAttrs = rvalTag[vknAttributesIdx];
    const size_t knAttrs = rrgAttrs.FIsArray() ? rrgAttrs.GetSize() : 0;
    ritem.m_rgAttrs.reserve( knAttrs );
    for ( size_t nAttr = 0; nAttr < knAttrs; ++nAttr )
    {
      const _TyLexValue & rrgAttr = rrgAttrs[nAttr];
      _xml_compare_attr attr;
      _GetName( rltokTag, rrgAttr, attr.m_name, true );
      if ( attr.m_name.m_fNamespaceDecl && !m_roptions.m_fCompareNamespaceDeclarations )
        continue;
      attr.m_strValue = _StrValue( rltokTag, rrgAttr[vknAttr_ValueIdx] );
      ritem.m_rgAttrs.push_back( std::move( attr ) );
    }
    if ( !m_roptions.m_fCompareAttributeOrder )
      std::sort( ritem.m_rgAttrs.begin(), ritem.m_rgAttrs.end(), []( _xml_compare_attr const & _rl, _xml_compare_attr const & _rr ) { return _rl.m_name < _rr.m_name; } );
  }
  // The content which the cursor has read for the current tag: up to its next sub-tag or to its end.
  void _AddContent()
  {
    typename _TyReadCursor::_TyXmlReadContext & rctxt = m_rxrc.GetContextCur();
    rctxt.ApplyContent( 0, rctxt.NContentTokens(),
      [this]( const _TyXmlToken * _pxtBegin, const _TyXmlToken * _pxtEnd )
      {
        for ( const _TyXmlToken * pxtCur = _pxtBegin; _pxtEnd != pxtCur; ++pxtCur )
          _AddContentToken( *pxtCur );
      }
    );
    rctxt.ClearContent(); // It remains until the cursor reads more for this tag - and it may not: it is done with.
  }
  void _AddContentToken( _TyXmlToken const & _rxt )
  {
    const _TyLexToken & rltok = _rxt.GetLexToken();
    const _TyLexValue & rval = rltok.GetValue();
    switch( _rxt.GetTokenId() )
    {
      case s_knTokenCharData:
      case s_knTokenCDataSection:
      {
        if ( 1 == m_nDepth )
          break; // White space outside of the document's tag.
        _xml_compare_item & ritem = m_dqItems.emplace_back();
        ritem.m_excmp = excmpText;
        ritem.m_strText = _StrValue( rltok, rval );
      }
      break;
      case s_knTokenComment:
      {
        if ( !m_roptions.m_fCompareComments )
          break;
        _xml_compare_item & ritem = m_dqItems.emplace_back();
        ritem.m_excmp = excmpComment;
        ritem.m_strText = _StrValue( rltok, rval );
      }
      break;
      case s_knTokenProcessingInstruction:
      {
        if ( !m_roptions.m_fCompareProcessingInstructions )
          break;
        _xml_compare_item & ritem = m_dqItems.emplace_back();
        ritem.m_excmp = excmpProcessingInstruction;
        ritem.m_name.m_strLocalName = _StrValue( rltok, rval[vknProcessingInstruction_PITargetIdx] );
        if ( rval.GetSize() > vknProcessingInstruction_MeatIdx )
          ritem.m_strText = _StrValue( rltok, rval[vknProcessingInstruction_MeatIdx] );
      }
      break;
      case s_knTokenDoctypeDecl:
      {
        if ( !m_roptions.m_fCompareDoctypeDecl )
          break;
        _xml_compare_item & ritem = m_dqItems.emplace_back();
        ritem.m_excmp = excmpDoctypeDecl;
        ritem.m_strText = _StrValue( rltok, rval );
      }
      break;
      case s_knTokenEntityContentBegin:
      case s_knTokenEntityContentEnd:
      break; // How the document is written: what an entity has is content of the document however it came to be there.
      default:
        VerifyThrowSz( false, "Unexpected token [%u] within the content of a tag.", unsigned( _rxt.GetTokenId() ) );
      break;
    }
  }

  _TyReadCursor & m_rxrc;
  xml_compare_options const & m_roptions;
  std::deque< _xml_compare_item > m_dqItems; // Read and not yet given.
  std::u32string m_strTextPending; // The character data up to the next item which is not character data.
  size_t m_nDepth{ 1 }; // 1: outside of the document's tag.
  bool m_fStarted{ false };
  bool m_fDone{ false };
};

// FCompareXmlDocuments:
// Compare the documents of two read cursors, each of which is at the beginning of its document: see the top of this file
//  for what makes two documents the same. The cursors are read to the ends of their documents - or to where they differ.
// Returns true if they are the same. If not then *_pstrDifference - if it is given - says the first difference: where
//  it is, by the tags from the document's tag down, and what it is. "left" is the first document and "right" the second.
// Throws what the cursors throw: a document which is not well formed is not compared.
// Either cursor may be a variant - an xml_read_cursor_var<>: then it is the cursor which that has which is read.
template < class t_TyReadCursorLeft, class t_TyReadCursorRight >
bool _FCompareXmlDocumentsOfCursors(  t_TyReadCursorLeft & _rxrcLeft, t_TyReadCursorRight & _rxrcRight,
                                      xml_compare_options const & _roptions, std::string * _pstrDifference )
{
  _xml_compare_reader< t_TyReadCursorLeft > readerLeft( _rxrcLeft, _roptions );
  _xml_compare_reader< t_TyReadCursorRight > readerRight( _rxrcRight, _roptions );
  std::vector< std::string > rgstrPath; // The names of the tags which we are within.
  auto lambdaDifference = [&]( std::string const & _rstrWhat ) -> bool
  {
    if ( _pstrDifference )
    {
      _pstrDifference->clear();
      for ( std::string const & rstrTag : rgstrPath )
        *_pstrDifference += "/" + rstrTag;
      if ( rgstrPath.empty() )
        *_pstrDifference = "/";
      *_pstrDifference += ": " + _rstrWhat;
    }
    return false;
  };
  auto lambdaQuote = []( std::u32string const & _rstr ) -> std::string
  {
    return "[" + StrConvertString< char >( _rstr ) + "]";
  };
  _xml_compare_item itemLeft, itemRight;
  for ( ; ; )
  {
    readerLeft.GetNext( itemLeft );
    readerRight.GetNext( itemRight );
    if ( itemLeft.m_excmp != itemRight.m_excmp )
    {
      std::string strWhat = std::string( "left has " ) + _PszXmlCompareItem( itemLeft.m_excmp );
      if ( excmpStartTag == itemLeft.m_excmp )
        strWhat += " <" + itemLeft.m_name.StrName() + ">";
      else
      if ( ( excmpText == itemLeft.m_excmp ) || ( excmpComment == itemLeft.m_excmp ) )
        strWhat += " " + lambdaQuote( itemLeft.m_strText );
      strWhat += std::string( " where right has " ) + _PszXmlCompareItem( itemRight.m_excmp );
      if ( excmpStartTag == itemRight.m_excmp )
        strWhat += " <" + itemRight.m_name.StrName() + ">";
      else
      if ( ( excmpText == itemRight.m_excmp ) || ( excmpComment == itemRight.m_excmp ) )
        strWhat += " " + lambdaQuote( itemRight.m_strText );
      return lambdaDifference( strWhat );
    }
    switch( itemLeft.m_excmp )
    {
      case excmpEndOfDocument:
        return true;
      case excmpStartTag:
      {
        if ( !itemLeft.m_name.FIsSame( itemRight.m_name, _roptions.m_fComparePrefixes ) )
          return lambdaDifference( "the tag is <" + itemLeft.m_name.StrName() + "> in left and <" + itemRight.m_name.StrName() + "> in right" );
        rgstrPath.push_back( itemLeft.m_name.StrName() );
        // The attributes: both are in the same order - that of their names unless their order is being compared.
        const size_t knAttrsLeft = itemLeft.m_rgAttrs.size();
        const size_t knAttrsRight = itemRight.m_rgAttrs.size();
        for ( size_t nLeft = 0, nRight = 0; ( nLeft < knAttrsLeft ) || ( nRight < knAttrsRight ); ++nLeft, ++nRight )
        {
          if ( nRight == knAttrsRight )
            return lambdaDifference( "left has the attribute [" + itemLeft.m_rgAttrs[nLeft].m_name.StrName() + "] and right doesn't" );
          if ( nLeft == knAttrsLeft )
            return lambdaDifference( "right has the attribute [" + itemRight.m_rgAttrs[nRight].m_name.StrName() + "] and left doesn't" );
          _xml_compare_attr const & rattrLeft = itemLeft.m_rgAttrs[nLeft];
          _xml_compare_attr const & rattrRight = itemRight.m_rgAttrs[nRight];
          if ( !rattrLeft.m_name.FIsSame( rattrRight.m_name, _roptions.m_fComparePrefixes ) )
          {
            if ( _roptions.m_fCompareAttributeOrder )
              return lambdaDifference( "attribute " + std::to_string( nLeft + 1 ) + " is [" + rattrLeft.m_name.StrName() + "] in left and [" + rattrRight.m_name.StrName() + "] in right" );
            // In the order of the names: the lesser one is the one which the other document doesn't have.
            if ( rattrLeft.m_name < rattrRight.m_name )
              return lambdaDifference( "left has the attribute [" + rattrLeft.m_name.StrName() + "] and right doesn't" );
            return lambdaDifference( "right has the attribute [" + rattrRight.m_name.StrName() + "] and left doesn't" );
          }
          if ( rattrLeft.m_strValue != rattrRight.m_strValue )
            return lambdaDifference( "the value of the attribute [" + rattrLeft.m_name.StrName() + "] is " + lambdaQuote( rattrLeft.m_strValue ) +
                                     " in left and " + lambdaQuote( rattrRight.m_strValue ) + " in right" );
        }
      }
      break;
      case excmpEndTag:
        Assert( !rgstrPath.empty() );
        if ( !rgstrPath.empty() )
          rgstrPath.pop_back();
      break;
      case excmpText:
        if ( itemLeft.m_strText != itemRight.m_strText )
          return lambdaDifference( "the character data is " + lambdaQuote( itemLeft.m_strText ) + " in left and " + lambdaQuote( itemRight.m_strText ) + " in right" );
      break;
      case excmpComment:
        if ( itemLeft.m_strText != itemRight.m_strText )
          return lambdaDifference( "the comment is " + lambdaQuote( itemLeft.m_strText ) + " in left and " + lambdaQuote( itemRight.m_strText ) + " in right" );
      break;
      case excmpProcessingInstruction:
        if ( ( itemLeft.m_name.m_strLocalName != itemRight.m_name.m_strLocalName ) || ( itemLeft.m_strText != itemRight.m_strText ) )
          return lambdaDifference( "the processing instruction is " + lambdaQuote( itemLeft.m_name.m_strLocalName + U" " + itemLeft.m_strText ) +
                                   " in left and " + lambdaQuote( itemRight.m_name.m_strLocalName + U" " + itemRight.m_strText ) + " in right" );
      break;
      case excmpDoctypeDecl:
        if ( itemLeft.m_strText != itemRight.m_strText )
          return lambdaDifference( "the document type declarations are not the same text" );
      break;
      default:
        Assert( false );
      break;
    }
  }
}
template < class t_TyReadCursorLeft, class t_TyReadCursorRight >
bool FCompareXmlDocuments(  t_TyReadCursorLeft & _rxrcLeft, t_TyReadCursorRight & _rxrcRight,
                            xml_compare_options const & _roptions = xml_compare_options(), std::string * _pstrDifference = nullptr )
{
  if constexpr ( TFIsReadCursorVar_v< t_TyReadCursorLeft > )
  {
    return std::visit(
      [&_rxrcRight,&_roptions,_pstrDifference]( auto & _tCursorLeft ) -> bool
      {
        return FCompareXmlDocuments( _tCursorLeft, _rxrcRight, _roptions, _pstrDifference );
      }, _rxrcLeft.GetVariant() );
  }
  else
  if constexpr ( TFIsReadCursorVar_v< t_TyReadCursorRight > )
  {
    return std::visit(
      [&_rxrcLeft,&_roptions,_pstrDifference]( auto & _tCursorRight ) -> bool
      {
        return FCompareXmlDocuments( _rxrcLeft, _tCursorRight, _roptions, _pstrDifference );
      }, _rxrcRight.GetVariant() );
  }
  else
    return _FCompareXmlDocumentsOfCursors( _rxrcLeft, _rxrcRight, _roptions, _pstrDifference );
}

__XMLP_END_NAMESPACE
