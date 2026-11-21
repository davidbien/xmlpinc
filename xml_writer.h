#pragma once

//          Copyright David Lawrence Bien 1997 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt).

// xml_writer.h
// XML writer. Uses the defined productions to match supplied strings to ensure correctness.
// dbien
// 13FEB2021

// Design goals:
// 1) As with the xml_read_cursor we want to only store information on tags we *currently* know about - i.e. the ones
//    in the write(read) context list.
// 2) We will move the tag and content tags into this object and write them and then just copy the tag name and any 
//    namespace management objects found and leave copies in xml_read_cursor's copy so that normal operation occurs.
// 3) My thought is that this can pretty much be just modelled on how the xml_tag stuff peruses the xml_read_cursor.
//    But we aren't constructing a tree and just need to hold onto the current context list to the root.
// 4) The tags should be able to come either:
//    a) come straight from the xml_read_cursor(_var). In this case there is no need for validation of the data.
//       We assume that if the data is in the form of _l_data positions (or vtyDataPosition) in the _l_value then
//       it came from the parser and thus there is no need for output validation.
//    b) If the data isn't in _l_data or vtyDataPosition form in the _l_value - i.e. it has been written over with
//       some type of string then this data would need to be validated before converting to the output format. Note that
//       this data may need to be converted to be validated by the character's state machine.
//    b) come from the BeginTag() interface which returns an object that can be used to add attributes, etc, from
//       various sources, etc. as well as close the tag.
//    c) we should be able to "splice" in data from an xml_read_cursor(_var) and we want to be able to splice in
//       data from different character encodings from the character encoding we are outputing.
// 5) We want to output any encoding from any other encoding: The output encoding of the writer shouldn't depend
//    at all on the encoding of the source.
//    a) This will involve translating the source tokens in-place to a different character type which, given how we have done
//       things should be pretty easy. The positions within the sub-objects will be exactly the same they are just indexing
//       a different character type. That's not true at all, actually due to the presence of variable length characters.
//       We'll have to think about how to do this.         

#include <set>
#include <charconv>
#include <concepts>
#include "xml_output_tokens.h"
#include "xml_attr_default.h"
#include "xml_dtd.h"
#include "xml_validate.h"
#include "_l_match.h"

__XMLP_BEGIN_NAMESPACE

// The strings which the writer is given for what it makes - and the tags which it makes are: a string, a string view or a
//  null-terminated string ( null is the empty string ) of any type of character. char is UTF-8 and wchar_t is UTF-16 or
//  UTF-32 as it is on the platform: each is had as the type of character of its size.
template < class t_TyChar >
concept CXmlCharArg = is_same_v< t_TyChar, char > || is_same_v< t_TyChar, wchar_t > || is_same_v< t_TyChar, char8_t > ||
  is_same_v< t_TyChar, char16_t > || is_same_v< t_TyChar, char32_t >;
template < class t_TyChar >
using TXmlCharOfSize_t = conditional_t< sizeof( t_TyChar ) == sizeof( char8_t ), char8_t, conditional_t< sizeof( t_TyChar ) == sizeof( char16_t ), char16_t, char32_t > >;
template < CXmlCharArg t_TyChar >
basic_string_view< TXmlCharOfSize_t< t_TyChar > > SvXmlStrArg( const t_TyChar * _psz )
{
  typedef TXmlCharOfSize_t< t_TyChar > _TyCharXml;
  if ( !_psz )
    return basic_string_view< _TyCharXml >();
  return basic_string_view< _TyCharXml >( (const _TyCharXml *)_psz, basic_string_view< t_TyChar >( _psz ).length() );
}
template < CXmlCharArg t_TyChar, class t_TyTraits, class t_TyAllocator >
basic_string_view< TXmlCharOfSize_t< t_TyChar > > SvXmlStrArg( basic_string< t_TyChar, t_TyTraits, t_TyAllocator > const & _rstr )
{
  return basic_string_view< TXmlCharOfSize_t< t_TyChar > >( (const TXmlCharOfSize_t< t_TyChar > *)_rstr.data(), _rstr.length() );
}
template < CXmlCharArg t_TyChar, class t_TyTraits >
basic_string_view< TXmlCharOfSize_t< t_TyChar > > SvXmlStrArg( basic_string_view< t_TyChar, t_TyTraits > _sv )
{
  return basic_string_view< TXmlCharOfSize_t< t_TyChar > >( (const TXmlCharOfSize_t< t_TyChar > *)_sv.data(), _sv.length() );
}
template < class t_TyStr >
concept CXmlStrArg = requires( t_TyStr const & _rstr ) { SvXmlStrArg( _rstr ); };
// A token - of a tag which was read: see xml_writer<>::StartTag().
template < class t_TyXmlToken >
concept CXmlTokenArg = requires( remove_cvref_t< t_TyXmlToken > const & _rtok ) { _rtok.FIsTag(); _rtok.GetLexToken(); };
// A number which is the value of an attribute - not a character.
template < class t_TyNumber >
concept CXmlNumberArg = ( std::integral< t_TyNumber > && !CXmlCharArg< t_TyNumber > ) || std::floating_point< t_TyNumber >;
// A Char [2].
inline bool FIsXmlChar( char32_t _uc )
{
  return ( 0x9 == _uc ) || ( 0xA == _uc ) || ( 0xD == _uc ) || ( ( _uc >= 0x20 ) && ( _uc <= 0xD7FF ) ) ||
    ( ( _uc >= 0xE000 ) && ( _uc <= 0xFFFD ) ) || ( ( _uc >= 0x10000 ) && ( _uc <= 0x10FFFF ) );
}

enum EDetectReferences
{
   edrNoReferences, // then any embedded '&' is always escaped with "&amp;"
   edrAlwaysReference, // then any embedded '&' *must* correspond to a valid Reference of some sort.
   edrAutoReference, // try to detect if we should replace with "&amp;" or not:
   edrAutoReferenceNoError, 
     //  1) If we see what looks like a CharRef (which is pretty obvious) then we will validate it.
     //  2) If we see what looks like just an EntityRef then we check if it is in the entity ref map:
     //     a) If it isn't in the entity map and edrAutoReferenceNoError then we will replace the leading '&' with "&amp;"
     //     b) If it isn't in the enity map and edrAutoReference then we will throw an error for a missing reference.
  edrDetectReferencesCount // This always at the end.
};

// xml_write_context:
// There is a stack of write contexts corresponding to the current set of unclosed tags from root to current leaf.
template < class t_TyXmlTransportOut >
class xml_write_context
{
  typedef xml_write_context _TyThis;
public:
  typedef typename t_TyXmlTransportOut::_TyChar _TyChar;
  typedef xml_writer< t_TyXmlTransportOut > _TyXmlWriter;
  // We always used backed context transport contexts for our tokens in the writer. When translating tokens
  //  any transport context can be converted into a backed context.
  typedef _l_transport_backed_ctxt< _TyChar > _TyTransportCtxt;
  typedef xml_user_obj< _TyChar, vkfSupportDTD > _TyLexUserObj;
  typedef xml_value_traits_pack< _TyChar > _TyTpValueTraitsPack;
  typedef xml_token< _TyTransportCtxt, _TyLexUserObj, _TyTpValueTraitsPack > _TyXmlToken;
  typedef _l_action_object_base< _TyChar, false > _TyAxnObjBase;

  xml_write_context( _TyXmlWriter * _pxwWriter, _TyLexUserObj & _ruoUserObj, vtyTokenIdent _tidAccept )
    : m_pxwWriter( _pxwWriter ),
      m_xtkToken( _ruoUserObj, _tidAccept )
  {
  }
#if 0
  xml_write_context( _TyXmlWriter * _pxwWriter, _TyLexUserObj & _ruoUserObj, const _TyAxnObjBase * _paobCurToken )
    : m_pxwWriter( _pxwWriter ),
      m_xtkToken( _ruoUserObj, _paobCurToken )
  {
  }
#endif //0
  xml_write_context( _TyXmlWriter * _pxwWriter, _TyXmlToken && _rrxtkToken )
    : m_pxwWriter( _pxwWriter ),
      m_xtkToken( std::move( _rrxtkToken ) )
  {
  }
  void SetDetectReferences( EDetectReferences _edrDetectReferences = edrDetectReferencesCount )
  {
    m_edrDetectReferences = _edrDetectReferences;
  }
  EDetectReferences GetDetectReferences() const
  {
    return m_edrDetectReferences;
  }
  // Pretty printing - see xml_writer<>::SetOutputFormat(): whether what is within the element is indented - it isn't once
  //  text is written within it, nor where xml:space="preserve" is in scope.
  bool FIndentWithin() const
  {
    return !m_fMixed && !m_fPreserveSpace;
  }
  void SetMixed()
  {
    m_fMixed = true;
  }
  void SetPreserveSpace( bool _fPreserveSpace )
  {
    m_fPreserveSpace = _fPreserveSpace;
  }
  bool FHadChildMarkup() const
  {
    return m_fHadChildMarkup;
  }
  void SetHadChildMarkup()
  {
    m_fHadChildMarkup = true;
  }
  // An element is as the one which it is within - until what it has says otherwise.
  void InheritFormat( _TyThis const & _rwcxWithin )
  {
    m_fMixed = _rwcxWithin.m_fMixed;
    m_fPreserveSpace = _rwcxWithin.m_fPreserveSpace;
  }
  // The document context of the writer - that of the tokens which it makes.
  auto & GetDocumentContext()
  {
    return m_pxwWriter->m_xdcxtDocumentContext;
  }
  bool FCommitted() const
  {
    return m_fCommitted;
  }
  // Whether the end of the tag is an end tag when nothing is written within it - <name></name> rather than <name/>: as
  //  it was read - s_knTokenSTag - unless it is set.
  void SetFullEnd( bool _fFullEnd )
  {
    m_optfFullEnd = _fFullEnd;
  }
  bool FFullEnd() const
  {
    return m_optfFullEnd.has_value() ? *m_optfFullEnd : ( s_knTokenSTag == m_xtkToken.GetTokenId() );
  }
  const _TyXmlToken & GetToken() const
  {
    return m_xtkToken;
  }
  _TyXmlToken & GetToken()
  {
    return m_xtkToken;
  }
  // The data inside of m_xtkToken is complete - write it out to the output stream.
  // We could then delete the data - except for namespace declarations, but I don't see the
  //  need to preemptively delete since not too much memory is being taken up.
  // If _fHasContent is true then it is known that the tag has content and thus the tag end characters can be written.
  void CommitTagData()
  {
    if ( m_fCommitted )
      return; // no-op - valid to call more than once.
    m_pxwWriter->_CommitTag( m_xtkToken, m_edrDetectReferences );
    m_fCommitted = true; // If we throw whilte committing we are not committed - even though it may not be fixable.
  }
  void EndTag()
  {
    CommitTagData();
    m_pxwWriter->_EndTag( this );
    // this has been deleted as of this statement.
  }
  // The white space before the '>' of the end tag [42] - "</name  >" - as the tag was read: see
  //  xml_read_cursor<>::SvEndTagSpace(). It is written only if an end tag is.
  template < class t_TyCharSpace >
  void SetEndTagSpace( basic_string_view< t_TyCharSpace > _svSpace )
  {
    m_strEndTagSpace.clear();
    for ( t_TyCharSpace ch : _svSpace )
      m_strEndTagSpace += _TyChar( ch ); // White space: the same in each encoding.
  }
  basic_string< _TyChar > const & RStrEndTagSpace() const
  {
    return m_strEndTagSpace;
  }
protected:
  basic_string< _TyChar > m_strEndTagSpace;
  _TyXmlWriter * m_pxwWriter;
  _TyXmlToken m_xtkToken; // This is always a tag token. It really doesn't matter if the id is s_knTokenSTag or s_knTokenEmptyElemTag.
  bool m_fCommitted{false}; // Committed yet?
  optional< bool > m_optfFullEnd; // See SetFullEnd().
  // Pretty printing - see xml_writer<>::SetOutputFormat():
  bool m_fMixed{false}; // Text was written within it - or within an element which it is within, before it was begun.
  bool m_fPreserveSpace{false}; // xml:space="preserve" is in scope.
  bool m_fHadChildMarkup{false}; // Markup was written within it on a line of its own: its end tag is on one as well.
  EDetectReferences m_edrDetectReferences{ edrDetectReferencesCount }; // User can override this inside of xml_write_tag.
};

// xml_write_tag:
// This is a wrapper which stores a references to a tag that has been started in the associated xml_writer<> object.
// When the lifetime of this object ends, the tag is ended, so act accordingly. The nice thing is that even in retail
//  we will throw if you do something wrong - like end a tag early.
// The Commit() method must be called on this tag before adding another tag. We will throw an error if Commit() hasn't
//  been called on the top tag on the stack when a new tag as been added.
// If Commit() hasn't been called when the xml_write_tag's lifetime is ended then Commit() is called automatically.
template < class t_TyXmlTransportOut >
class xml_write_tag
{
  typedef xml_write_tag _TyThis;
public:
  typedef t_TyXmlTransportOut _TyXmlTransportOut;
  typedef typename _TyXmlTransportOut::_TyChar _TyChar;
  typedef xml_writer< _TyXmlTransportOut > _TyWriter;
  typedef xml_write_context< _TyXmlTransportOut > _TyWriteContext;
  typedef xml_namespace_value_wrap< _TyChar > _TyXmlNamespaceValueWrap;

  // This is our attribute value/name interface object:
  // 1) Namespaces might be declared for this tag - default or prefixed.
  // 2) Attribute/value pairs might be added to this tag.
  // 3) This tag may already be populated with data that came from an XML stream but now the user wants to modify the values.

  xml_write_tag( xml_write_tag const & ) = delete;
  xml_write_tag & operator =( xml_write_tag const & ) = delete;
  xml_write_tag( xml_write_tag && _rr )
  {
    swap( _rr );
  }
  xml_write_tag & operator =( xml_write_tag && _rr )
  {
    _TyThis acquire( std::move( _rr ) );
    swap( acquire );
    return *this;
  }

  xml_write_tag( _TyWriteContext & _rwcxt )
    : m_pwcxt( &_rwcxt )
  {
  }
  ~xml_write_tag() noexcept(false)
  {
    // We shouldn't throw out of this destructor if we are within an unwinding:
    if ( !!m_pwcxt )
    {
      bool fInUnwinding = !!std::uncaught_exceptions();
      try
      {
        EndTag();
      }
      catch ( std::exception const & )
      {
        if ( !fInUnwinding )
          throw; // let someone else deal with this.
        // we could log here - not much else to be done besides aborting and that's draconian.
      }
    }
  }
  void swap( _TyThis & _r )
  {
    std::swap( m_pwcxt, _r.m_pwcxt );
    m_xnvwAttributes.swap( _r.m_xnvwAttributes );
  }

  // This causes the data contained within this tag to be written to the transport.
  // This method may only be called once.
  // Note that starting another tag under this one will also cause an implicit Commit() to occur ( as a shortcut ).
  // Note that writing a token to the when there is a non-committed tag causes an auto-commit.
  // Calling commit on a committed tag is not an error - it is merely ignored - this takes care of cases where
  //  the addition of a sub-tag or sub-token causes a commit.
  // If _fCallerEndsTagDirectly then we "forget" about the ending of the tag using the lifetime of this object.
  // This allows the caller to use the write context stack as a write stack and not maintain a separate stack
  //  of xml_write_tag<> objects. The caller is then responsible for calling xml_write_context<>::EndTag().
  void Commit( bool _fCallerEndsTagDirectly = false )
  {
    Assert( m_pwcxt );
    if ( m_pwcxt )
    {
      m_pwcxt->CommitTagData();
      if ( _fCallerEndsTagDirectly )
        m_pwcxt = nullptr;
    }
  }
  // End this tag. This is called from the destructor but may also be called directly.
  void EndTag()
  {
    if ( !!m_pwcxt )
    {
      _TyWriteContext * pwcxt = m_pwcxt;
      m_pwcxt = nullptr; // After call to _TyWriteContext::EndTag() the context is deleted.
      pwcxt->EndTag();
    }
  }
  // This determines how references that may be present in attribute values are detected or not.
  void SetDetectReferences( EDetectReferences _edrDetectReferences = edrDetectReferencesCount )
  {
    m_pwcxt->SetDetectReferences( _edrDetectReferences );
  }
  // See xml_write_context<>::SetEndTagSpace().
  template < class t_TyCharSpace >
  void SetEndTagSpace( basic_string_view< t_TyCharSpace > _svSpace )
  {
    m_pwcxt->SetEndTagSpace( _svSpace );
  }
  EDetectReferences GetDetectReferences() const
  {
    return m_pwcxt->GetDetectReferences();
  }
  // Return if the tag has a namespace or not.
  bool FHasNamespace() const
  {
    Assert( m_pwcxt );
    return m_pwcxt->GetToken().FHasNamespace();
  }
  // Get a namespace reference from this tag - the one that is on the tag itself.
  _TyXmlNamespaceValueWrap GetNamespaceReference() const
  {
    Assert( FHasNamespace() ); // We will throw in the variant accessing the value wrap if it isn't there.
    return m_pwcxt->GetToken().GetNamespaceReference( enrtStandaloneReference );
  }
  // What a tag which is made has - see xml_writer<>::StartTag(). Each may be given until the tag is committed: by
  //  Commit(), or when anything within it is written. The strings are as xml_writer<> has them - see SvXmlStrArg().
  // Declare the namespace (prefix,URI) on this tag - unless it is the one in scope for the prefix. The tag, and its
  //  attributes of the prefix, are in it. What is returned refers to it - for attributes and for tags within this one.
  //  See xml_token<>::AddNamespace().
  template < class t_TyStrPrefix, class t_TyStrUri >
  _TyXmlNamespaceValueWrap AddNamespace( t_TyStrPrefix const & _rPrefix, t_TyStrUri const & _rUri )
    requires( CXmlStrArg< t_TyStrPrefix > && CXmlStrArg< t_TyStrUri > )
  {
    _CheckNotCommitted();
    return m_pwcxt->GetToken().AddNamespace( m_pwcxt->GetDocumentContext(), SvXmlStrArg( _rPrefix ), SvXmlStrArg( _rUri ) );
  }
  // The namespace of the attributes without a prefix which are added - unless one is given for an attribute.
  void SetDefaultAttributeNamespace( _TyXmlNamespaceValueWrap const & _rxnvw )
  {
    VerifyThrowSz( !_rxnvw.RStringPrefix().empty(), "The namespace of an attribute has a prefix." );
    m_xnvwAttributes = _rxnvw.ShedReference( enrtStandaloneReference );
  }
  void ClearDefaultAttributeNamespace()
  {
    m_xnvwAttributes.Clear();
  }
  // Add the attribute: its value is the characters of the value - what must be is escaped when it is written. See
  //  xml_token<>::AddAttribute() for its namespace - _pxnvw is that of an attribute without a prefix.
  template < class t_TyStrName, class t_TyStrValue >
  void AddAttribute( t_TyStrName const & _rName, t_TyStrValue const & _rValue, const _TyXmlNamespaceValueWrap * _pxnvw = nullptr )
    requires( CXmlStrArg< t_TyStrName > && CXmlStrArg< t_TyStrValue > )
  {
    _CheckNotCommitted();
    m_pwcxt->GetToken().AddAttribute( m_pwcxt->GetDocumentContext(), SvXmlStrArg( _rName ), SvXmlStrArg( _rValue ), _PGetDefaultAttributeNamespace( _pxnvw ) );
  }
  template < class t_TyStrName >
  void AddAttribute( _TyXmlNamespaceValueWrap const & _rxnvw, t_TyStrName const & _rName, auto const & _rValue )
    requires( CXmlStrArg< t_TyStrName > )
  {
    AddAttribute( _rName, _rValue, &_rxnvw );
  }
  // An attribute whose value is a number - as std::to_chars() writes it: the shortest which is read as the same number.
  //  A bool is "true" or "false" ( as xs:boolean ).
  template < class t_TyStrName, class t_TyNumber >
  void AddAttribute( t_TyStrName const & _rName, t_TyNumber _tValue, const _TyXmlNamespaceValueWrap * _pxnvw = nullptr )
    requires( CXmlStrArg< t_TyStrName > && CXmlNumberArg< t_TyNumber > )
  {
    if constexpr ( is_same_v< t_TyNumber, bool > )
      AddAttribute( _rName, _tValue ? "true" : "false", _pxnvw );
    else
    {
      char rgcValue[ 64 ];
      std::to_chars_result tcr = std::to_chars( rgcValue, rgcValue + sizeof( rgcValue ), _tValue );
      VerifyThrowSz( std::errc() == tcr.ec, "The number couldn't be written." );
      AddAttribute( _rName, basic_string_view< char >( rgcValue, tcr.ptr - rgcValue ), _pxnvw );
    }
  }
  // An attribute whose value is as printf() writes it - for a format of char ( UTF-8 ) or of wchar_t.
  template < class t_TyStrName, class t_TyCharFormat >
  void FormatAttribute( t_TyStrName const & _rName, const t_TyCharFormat * _pszFormat, ... )
    requires( CXmlStrArg< t_TyStrName > && ( is_same_v< t_TyCharFormat, char > || is_same_v< t_TyCharFormat, wchar_t > ) )
  {
    basic_string< t_TyCharFormat > strValue;
    va_list ap;
    va_start( ap, _pszFormat );
    try
    {
      VPrintfStdStr( strValue, _pszFormat, ap );
    }
    catch ( ... )
    {
      va_end( ap );
      throw;
    }
    va_end( ap );
    AddAttribute( _rName, strValue );
  }
  // Write <name></name> - and not <name/> - if nothing is written within the tag. See xml_write_context<>::FFullEnd().
  void SetFullEnd( bool _fFullEnd = true )
  {
    VerifyThrowSz( !!m_pwcxt, "The tag has been ended." );
    m_pwcxt->SetFullEnd( _fFullEnd );
  }
protected:
  void _CheckNotCommitted() const
  {
    VerifyThrowSz( !!m_pwcxt, "The tag has been ended." );
    VerifyThrowSz( !m_pwcxt->FCommitted(), "The tag has been written - nothing may be added to it." );
  }
  const _TyXmlNamespaceValueWrap * _PGetDefaultAttributeNamespace( const _TyXmlNamespaceValueWrap * _pxnvw ) const
  {
    // The default namespace of attributes of the document is xml_token<>'s.
    return !!_pxnvw ? _pxnvw : ( m_xnvwAttributes.FIsNull() ? nullptr : &m_xnvwAttributes );
  }
  _TyWriteContext * m_pwcxt{nullptr}; // The context in the context stack to which this xml_write_tag corresponds.
  _TyXmlNamespaceValueWrap m_xnvwAttributes; // When this is set then it is used for marking any added attributes.
};

// xml_writer:
// Writes an XML file stream through an XML write transport.
// The encoding type of t_TyXmlTransportOut is independent of t_TyXmlTraits but we must specify it here to allow splicing, etc.,
//  into a file because we can't mix encoding within a single file so no reason to try to be more flexible than that.
template < class t_TyXmlTransportOut >
class xml_writer
{
  typedef xml_writer _TyThis;
public:
  typedef t_TyXmlTransportOut _TyXmlTransportOut;
  typedef typename _TyXmlTransportOut::_TyChar _TyChar;
  typedef xml_markup_traits< _TyChar > _TyMarkupTraits;
  typedef xml_write_tag< t_TyXmlTransportOut > _TyXmlWriteTag;
  // We need a user object in order to be able to create tokens.
  typedef xml_user_obj< _TyChar, false > _TyLexUserObj;
  // We need to have our own local namespace map which means we need the full on document context:
  typedef _xml_document_context< _TyLexUserObj > _TyXmlDocumentContext;
  typedef typename _TyXmlDocumentContext::_TyPrFormatContext _TyPrFormatContext;
  typedef typename _TyXmlDocumentContext::_TyXMLDeclProperties _TyXMLDeclProperties;
  typedef typename _xml_namespace_map_traits< _TyChar >::_TyNamespaceMap _TyNamespaceMap;
  typedef xml_write_context< _TyXmlTransportOut > _TyWriteContext;
  friend _TyWriteContext;
  typedef typename _TyWriteContext::_TyXmlToken _TyXmlToken;
  typedef list< _TyWriteContext > _TyListWriteContexts;
  typedef xml_namespace_value_wrap< _TyChar > _TyXmlNamespaceValueWrap;
  typedef typename _TyXmlDocumentContext::_TyEntityMap _TyEntityMap;
  typedef basic_string< _TyChar > _TyStdStr;

  xml_writer()
  {
  }
  ~xml_writer() = default;

  // Open the given file for write in the given encoding.
  // This will also write the XMLDecl if we are to do so, and regardless it will create a
  //  write context stack containing a single element which is the XMLDecl element regardless
  //  if we are to write the element.
  // Unless _fKeepEncoding is false we will always write the encoding with the XMLDecl from the encoding of the output transport.
  void OpenFile( const char * _pszFileName, _TyXMLDeclProperties const & _rXmlDeclProperties = _TyXMLDeclProperties(), const _xml_output_format * _pxofFormat = nullptr, bool _fKeepEncoding = false )
  {
    m_strFileName = _pszFileName;
    FileObj foFile( CreateFileMaybeReadWrite< typename _TyXmlTransportOut::_TyFOpenFileReadWrite >( _pszFileName ) );
    VerifyThrowSz( foFile.FIsOpen(), "Unable to open file[%s]", _pszFileName );
    // emplace the transport - it will write the BOM or not:
    emplaceTransport( foFile, m_fWriteBOM );
    _Init( _rXmlDeclProperties, _pxofFormat, _fKeepEncoding ); // This might write the XMLDecl tag to the file, but definitely will init the context stack.
  }
  // Memfile support:
  void OpenMemFile( _TyXMLDeclProperties const & _rXmlDeclProperties = _TyXMLDeclProperties(), const _xml_output_format * _pxofFormat = nullptr, bool _fKeepEncoding = false )
  {
    emplaceTransport( m_fWriteBOM );
    _Init( _rXmlDeclProperties, _pxofFormat, _fKeepEncoding ); // This might write the XMLDecl tag to the file, but definitely will init the context stack.
  }
// options:
  // How what is written is formatted - see _xml_output_format. Unless m_fNoFormatting it is pretty printed - to be looked
  //  at: each tag, comment and processing instruction within an element is on a line of its own, indented by its depth.
  //  Character data which is only white space is not written where the indentation is in its place - between the markup
  //  within an element and at the top level - and the white space within tags is a single space. An element within which
  //  text is written - character data, a CDATA section, a reference - has mixed content: nothing more within it is
  //  indented, as nothing is where xml:space="preserve" is in scope ( 2.10 ). So the white space of the document is
  //  changed - but for that the document is as it was.
  // It applies when a transport is opened - the format given to OpenFile() or OpenMemFile() is used instead if one is.
  void SetOutputFormat( _xml_output_format const & _rxof )
  {
    m_optxofFormat = _rxof;
  }
  void ClearOutputFormat()
  {
    m_optxofFormat.reset();
  }
  bool FPrettyPrint() const
  {
    return m_fPrettyPrint;
  }
  void SetWriteBOM( bool _fWriteBOM )
  {
    VerifyThrowSz( ( m_fWriteBOM == _fWriteBOM ) || !FHasTransport(), "Mustn't change the use of namespaces after opening a transport." );
    m_fWriteBOM = _fWriteBOM;
  }
  bool FGetWriteBOM() const
  {
    return m_fWriteBOM;
  }
  // Whether to write the document type declaration when its token is written - s_knTokenDoctypeDecl, as when a document
  //  is written from a read cursor which has one. If not then the token is passed over.
  void SetWriteDoctypeDecl( bool _fWriteDoctypeDecl )
  {
    m_fWriteDoctypeDecl = _fWriteDoctypeDecl;
  }
  bool FGetWriteDoctypeDecl() const
  {
    return m_fWriteDoctypeDecl;
  }
  // Whether a document type declaration has been written to the output.
  bool FWroteDoctypeDecl() const
  {
    return m_fWroteDoctypeDecl;
  }
  // When the token of a document type declaration is written - as when a document is written from a read cursor - write
  //  that of _rdtd in its place: its text as it is now - one which was edited, say ( see xml_dtd<>::Edit() ). The text is
  //  copied: _rdtd may be edited after.
  template < class t_TyCharDtd >
  void SetDoctypeDeclToWrite( xml_dtd< t_TyCharDtd > const & _rdtd )
  {
    VerifyThrowSz( !_rdtd.m_strDoctypeText.empty(), "The DTD has no document type declaration - see xml_dtd<>::SetRootName()." );
    _TyStdStr str;
    ConvertString( str, _rdtd.m_strDoctypeText );
    m_optstrDoctypeDeclToWrite.emplace( std::move( str ) );
    m_fnValidatorOfDoctypeDeclToWrite = _FnValidatorOfDtd( make_shared< const xml_dtd< t_TyCharDtd > >( _rdtd ) );
  }
  void ClearDoctypeDeclToWrite()
  {
    m_optstrDoctypeDeclToWrite.reset();
    m_fnValidatorOfDoctypeDeclToWrite = nullptr;
  }
  // Validation of what is written: it is checked against the DTD of the document type declaration which is written - as
  //  a validating read cursor checks what it reads ( see xml_validator ). The DTD is all of it when it is given -
  //  WriteDoctypeDecl( xml_dtd ), SetDoctypeDeclToWrite(), or the DTD of a read cursor which has one and from which the
  //  declaration is written: see NWriteFromReadCursor() - and otherwise that of the text of the declaration alone,
  //  whose external subset isn't read. Without a document type declaration the document isn't valid.
  // A violation is recorded - RgValidityErrors() - and isn't an error: what violates is written. Unless
  //  SetThrowOnValidityError(): then it is thrown when it is found, and the document is not to be written further.
  // To be set before the document type declaration is written - and before the first tag.
  void SetValidate( bool _fValidate = true )
  {
    VerifyThrowSz( ( _fValidate == m_fValidate ) || ( !m_fWroteDoctypeDecl && !m_fWroteFirstTag ), "Validation is set before the document type declaration and the first tag are written." );
    m_fValidate = _fValidate;
  }
  bool FGetValidate() const
  {
    return m_fValidate;
  }
  void SetThrowOnValidityError( bool _fThrowOnValidityError = true )
  {
    m_fThrowOnValidityError = _fThrowOnValidityError;
    if ( !!m_upValidator )
      m_upValidator->SetThrowOnError( _fThrowOnValidityError );
  }
  bool FGetThrowOnValidityError() const
  {
    return m_fThrowOnValidityError;
  }
  // Whether what has been written is valid - with the declarations of its DTD. False if validation isn't set or no
  //  document type declaration was written.
  bool FIsValid() const
  {
    return !!m_upValidator && m_upValidator->FIsValid();
  }
  // What has been written violates - not what the declarations of its DTD do: see RgValidityErrorsOfDtd().
  vector< string > const & RgValidityErrors() const
  {
    static const vector< string > s_rgstrNone;
    return !m_upValidator ? s_rgstrNone : m_upValidator->RgErrors();
  }
  vector< string > const & RgValidityErrorsOfDtd() const
  {
    static const vector< string > s_rgstrNone;
    return !m_upValidator ? s_rgstrNone : m_upValidator->RgErrorsOfDtd();
  }
  // Not all of what has been written could be validated: not all of the declarations of the DTD are known - it has an
  //  external subset which wasn't read, say - or content is in an entity whose text has markup or is external.
  bool FValidationIncomplete() const
  {
    return !!m_upValidator && m_upValidator->FIncomplete();
  }
  // Whether to write the attributes which a tag didn't have and which the DTD gave it when it was read - those whose
  //  value is an xml_attr_default_value.
  // By default this follows the document type declaration: if it has been written then they are not - whoever reads
  //  what we write adds them again, and without them the tags are written as they were read. If it hasn't been written
  //  then they are what the document means and they are written.
  void SetWriteDefaultedAttributes( bool _fWriteDefaultedAttributes )
  {
    m_optfWriteDefaultedAttributes = _fWriteDefaultedAttributes;
  }
  void ClearWriteDefaultedAttributes() // Back to the default.
  {
    m_optfWriteDefaultedAttributes.reset();
  }
  bool FWriteDefaultedAttributes() const
  {
    return m_optfWriteDefaultedAttributes.has_value() ? *m_optfWriteDefaultedAttributes : !m_fWroteDoctypeDecl;
  }
  void SetWriteXMLDecl( bool _fWriteXMLDecl )
  {
    VerifyThrowSz( ( m_fWriteXMLDecl == _fWriteXMLDecl ) || !FHasTransport(), "Mustn't change the use of namespaces after opening a transport." );
    m_fWriteXMLDecl = _fWriteXMLDecl;
  }
  bool FGetWriteXMLDecl() const
  {
    return m_fWriteXMLDecl;
  }
  void SetUseNamespaces( bool _fUseNamespaces )
  {
    VerifyThrowSz( ( m_fUseNamespaces == _fUseNamespaces ) || !FHasTransport(), "Mustn't change the use of namespaces after opening a transport." );
    m_fUseNamespaces = _fUseNamespaces;
  }
  bool FGetUseNamespaces() const
  {
    return m_fUseNamespaces;
  }
  void SetDefaultDetectReferences( EDetectReferences _edrDefaultDetectReferences )
  {
    VerifyThrow( edrDetectReferencesCount != _edrDefaultDetectReferences ); // must set some valid option here.
    m_edrDefaultDetectReferences = _edrDefaultDetectReferences;
  }
  EDetectReferences EGetDefaultDetectReferences() const
  {
    return m_edrDefaultDetectReferences;
  }
  void SetIncludePrefixesInAttrNames( bool _fIncludePrefixesInAttrNames )
  {
    if ( _fIncludePrefixesInAttrNames != m_xdcxtDocumentContext.FIncludePrefixesInAttrNames() )
    {
      VerifyThrowSz( !FInsideDocumentTag(), "Can't change _fIncludePrefixesInAttrNames once we have written the first tag." );
      m_xdcxtDocumentContext.SetIncludePrefixesInAttrNames( _fIncludePrefixesInAttrNames );
    }
  }
  bool FIncludePrefixesInAttrNames() const
  {
    return m_xdcxtDocumentContext.FIncludePrefixesInAttrNames();
  }
// status:
  bool FIsOpen() const
  {
    return FHasTransport();
  }
  bool FHasTransport() const
  {
    return m_optTransportOut.has_value();
  }
  _TyXmlTransportOut & GetTransportOut()
  {
    Assert( FHasTransport() );
    return *m_optTransportOut;
  }
  const _TyXmlTransportOut & GetTransportOut() const
  {
    Assert( FHasTransport() );
    return *m_optTransportOut;
  }
  bool FInsideDocumentTag() const
  {
    return m_lContexts.size() > 1; // the claim is constant time since C++17.
  }
  bool FInProlog() const
  {
    return !m_fWroteFirstTag && ( 1 == m_lContexts.size() );
  }
  bool FInEpilog() const
  {
    return m_fWroteFirstTag && ( 1 == m_lContexts.size() );
  }

  // Write from a xml_read_cursor_var<>. Easy, just grab the underlying read cursor and use that.
  template < class t_TyReadCursor >
  size_t NWriteFromReadCursor( t_TyReadCursor & _rxrc, size_t _nNextTags = size_t(-1) )
    requires( TFIsReadCursorVar_v< t_TyReadCursor > )
  {
    return std::visit( _VisitHelpOverloadFCall {
      [this,_nNextTags]( auto & _tCursor ) -> size_t
      {
        return NWriteFromReadCursor( _tCursor, _nNextTags );
      }
    }, _rxrc.GetVariant() );
  }
  // Write from a read cursor.
  // Will write up to _nNextTags if the current position of the output allows for that.
  // 1) If the output context stack has a size of 1 (only XMLDecl node) then we can only write one tag.
  // 2) If the output context stack has a size greater than 1 then we can write <n> tags at the same level.
  // 3) If _nNextTags is zero, or we are in the epilog, then we will write all non-tags until the first tag.
  //    We will stop at any non-blank CharData as well. This allows splicing comments and blank CharData at
  //    the end of an XML document.
  // Returns the number of NextTags written.
  template < class t_TyReadCursor >
  size_t NWriteFromReadCursor( t_TyReadCursor & _rxrc, size_t _nNextTags = size_t(-1) )
    requires( !TFIsReadCursorVar_v< t_TyReadCursor > )
  {
    // If the following statement throws then you are mixing "including prefixes" and this isn't supported (currrently).
    SetIncludePrefixesInAttrNames( _rxrc.GetIncludePrefixesInAttrNames() );
    // The DTD of the cursor - all of it, as it was read - is what what is written is validated against if the document
    //  type declaration of the cursor is written: see SetValidate().
    struct _TyClearValidatorOfToken
    {
      _TyThis & m_r;
      ~_TyClearValidatorOfToken()
      {
        m_r.m_fnValidatorOfDoctypeToken = nullptr;
      }
    } clearValidatorOfToken{ *this };
    if constexpr ( requires { _rxrc.FHasDtd(); _rxrc.GetDtd(); } )
    {
      if ( m_fValidate && !m_fWroteDoctypeDecl && _rxrc.FHasDtd() )
      {
        auto const * pdtdCursor = &_rxrc.GetDtd();
        m_fnValidatorOfDoctypeToken = [this,pdtdCursor]() { return _FnValidatorOfDtd( make_shared< const remove_cvref_t< decltype( *pdtdCursor ) > >( *pdtdCursor ) )(); };
      }
    }

    if ( _nNextTags > 0 )
      _nNextTags = FInProlog() ? 1 : ( FInEpilog() ? 0 : _nNextTags );
    
    // The read cursor is within *some* tag - which may be the initial XMLDecl "tag" or some subtag thereof.
    bool fStartedInPrologWrite = FInProlog();
    bool fStartedInPrologRead = _rxrc.FInProlog();

    // Record the top of the stack, we will stop when we reach this point _nNextTag times.
    ssize_t nTagsRemaining = _nNextTags;
    if ( nTagsRemaining < 0 )
      nTagsRemaining = (numeric_limits< ssize_t >::max)();
    size_t nNextTagsWritten = 0;
    size_t nCurDepth = ( fStartedInPrologWrite == fStartedInPrologRead ) ? 1 : 0; // We use this to control whether to write content at the begin of the first and the end of the last loops.
    size_t nDepthStart = nCurDepth;
    do
    {
      if ( nCurDepth > 0 )
        _WriteReadCursorContent( _rxrc ); // We may not write initial content.
      if ( !nTagsRemaining )
        break; // done.
      if ( _rxrc.FMoveDown() )
      {
        // A tag is current on the read cursor - as always. We can move data out of it since it only needs its name and its namespace decls.
        _TyXmlWriteTag xwt( StartTag( std::move( _rxrc.GetTagCur() ), true ) ); // push tag on output context stack.
        xwt.Commit( true ); // Write the tag's data. We don't use xwt's lifetime to end the tag because we don't want to recurse here.
        ++nCurDepth;
        continue;
      }
      else
      {
        bool fNextTag = _rxrc.FNextTag();
        m_lContexts.back().SetEndTagSpace( _rxrc.SvEndTagSpace() ); // Its end tag was the last read.
        _EndTag( &m_lContexts.back() );
        if ( fNextTag )
        {
          // Then encountered a next tag without any intervening content.
          _TyXmlWriteTag xwt( StartTag( std::move( _rxrc.GetTagCur() ), true ) ); // push tag on output context stack.
          xwt.Commit( true ); // Write the tag's data. We don't use xwt's lifetime to end the tag because we don't want to recurse here.
          continue; // depth remains the same.
        }
        else
        if ( --nCurDepth == nDepthStart )
        {
          ++nNextTagsWritten;
          --nTagsRemaining;
        }
      }
    }
    while( nTagsRemaining >= 0 );
    return nNextTagsWritten;
  }
  // Write from a xml_read_cursor_var<>. Easy, just grab the underlying read cursor and use that.
  // This one allows special tags and attributes to unit test various additional features of the cursor.
  template < class t_TyReadCursor >
  size_t NWriteFromReadCursorUnitTest( t_TyReadCursor & _rxrc, bool& _rfSkippedSomething, size_t _nNextTags = size_t(-1) )
    requires( TFIsReadCursorVar_v< t_TyReadCursor > )
  {
    return std::visit( _VisitHelpOverloadFCall {
      [this,_nNextTags,&_rfSkippedSomething]( auto & _tCursor ) -> size_t
      {
        return NWriteFromReadCursorUnitTest( _tCursor, _rfSkippedSomething, _nNextTags );
      }
    }, _rxrc.GetVariant() );
  }
  // Write from a read cursor.
  // This one allows special tags and attributes to unit test various additional features of the cursor.
  // Will write up to _nNextTags if the current position of the output allows for that.
  // 1) If the output context stack has a size of 1 (only XMLDecl node) then we can only write one tag.
  // 2) If the output context stack has a size greater than 1 then we can write <n> tags at the same level.
  // 3) If _nNextTags is zero, or we are in the epilog, then we will write all non-tags until the first tag.
  //    We will stop at any non-blank CharData as well. This allows splicing comments and blank CharData at
  //    the end of an XML document.
  // Returns the number of NextTags written.
  template < class t_TyReadCursor >
  size_t NWriteFromReadCursorUnitTest( t_TyReadCursor & _rxrc, bool& _rfSkippedSomething, size_t _nNextTags = size_t(-1) )
    requires( !TFIsReadCursorVar_v< t_TyReadCursor > )
  {
    // If the following statement throws then you are mixing "including prefixes" and this isn't supported (currrently).
    SetIncludePrefixesInAttrNames( _rxrc.GetIncludePrefixesInAttrNames() );
    // The DTD of the cursor - all of it, as it was read - is what what is written is validated against if the document
    //  type declaration of the cursor is written: see SetValidate().
    struct _TyClearValidatorOfToken
    {
      _TyThis & m_r;
      ~_TyClearValidatorOfToken()
      {
        m_r.m_fnValidatorOfDoctypeToken = nullptr;
      }
    } clearValidatorOfToken{ *this };
    if constexpr ( requires { _rxrc.FHasDtd(); _rxrc.GetDtd(); } )
    {
      if ( m_fValidate && !m_fWroteDoctypeDecl && _rxrc.FHasDtd() )
      {
        auto const * pdtdCursor = &_rxrc.GetDtd();
        m_fnValidatorOfDoctypeToken = [this,pdtdCursor]() { return _FnValidatorOfDtd( make_shared< const remove_cvref_t< decltype( *pdtdCursor ) > >( *pdtdCursor ) )(); };
      }
    }

    _rfSkippedSomething = false; // Set this to true if we end up skipping something.

    if ( _nNextTags > 0 )
      _nNextTags = FInProlog() ? 1 : ( FInEpilog() ? 0 : _nNextTags );
    
    // The read cursor is within *some* tag - which may be the initial XMLDecl "tag" or some subtag thereof.
    bool fStartedInPrologWrite = FInProlog();
    bool fStartedInPrologRead = _rxrc.FInProlog();

    // Record the top of the stack, we will stop when we reach this point _nNextTag times.
    ssize_t nTagsRemaining = _nNextTags;
    if ( nTagsRemaining < 0 )
      nTagsRemaining = (numeric_limits< ssize_t >::max)();
    size_t nNextTagsWritten = 0;
    size_t nCurDepth = ( fStartedInPrologWrite == fStartedInPrologRead ) ? 1 : 0; // We use this to control whether to write content at the begin of the first and the end of the last loops.
    size_t nDepthStart = nCurDepth;
    bool fSkipCurrentTag = false; // We will need to remember this the next time through.
    do
    {
      if ( !fSkipCurrentTag && ( nCurDepth > 0 ) )
        _WriteReadCursorContent( _rxrc ); // We may not write initial content.
      if ( !nTagsRemaining )
        break; // done.
      if ( !fSkipCurrentTag && _rxrc.FMoveDown() )
      {
        // Check to see if we are to skip the current tag, and also which flavor of skipping if so.
        typedef typename t_TyReadCursor::_TyStrView _TyStrViewCursor;
        typedef typename t_TyReadCursor::_TyChar _TyCharCursor;
        _TyStrViewCursor svTag = _rxrc.SvTagCur();
        if ( !svTag.compare( str_array_cast< _TyCharCursor >( "skip" ) ) )
          fSkipCurrentTag = true;
        else
        if ( !svTag.compare( str_array_cast< _TyCharCursor >( "skipdownup" ) ) )
        {
          if ( _rxrc.FMoveDown() )
            (void)_rxrc.FMoveUp();
          fSkipCurrentTag = true;
        }
        else
        if ( !svTag.compare( str_array_cast< _TyCharCursor >( "skipleafup" ) ) )
        {
          size_t nMoveDown = 0;
          while ( _rxrc.FMoveDown() )
            ++nMoveDown;
          while ( nMoveDown-- )
            (void)_rxrc.FMoveUp();
          fSkipCurrentTag = true;
        }
        else
        if ( !svTag.compare( str_array_cast< _TyCharCursor >( "movedownup" ) ) )
        {
          if ( _rxrc.FMoveDown() )
            (void)_rxrc.FMoveUp();
        }
        else
        if ( !svTag.compare( str_array_cast< _TyCharCursor >( "moveleafup" ) ) )
        {
          size_t nMoveDown = 0;
          while ( _rxrc.FMoveDown() )
            ++nMoveDown;
          while ( nMoveDown-- )
            (void)_rxrc.FMoveUp();
        }
        _rfSkippedSomething = _rfSkippedSomething || fSkipCurrentTag;
        if ( !fSkipCurrentTag ) // otherwise we just want to call FNextTag() to move to the next tag and not write the content.
        {
          // A tag is current on the read cursor - as always. We can move data out of it since it only needs its name and its namespace decls.
          _TyXmlWriteTag xwt( StartTag( std::move( _rxrc.GetTagCur() ), true ) ); // push tag on output context stack.
          xwt.Commit( true ); // Write the tag's data. We don't use xwt's lifetime to end the tag because we don't want to recurse here.
          ++nCurDepth;
        }
        continue;
      }
      else
      {
        bool fNextTag = _rxrc.FNextTag();
        if ( !fSkipCurrentTag )
        {
          // Otherwise the above was just skipping this current tag - we didn't start a tag above and thus we shouldn't end a tag here.
          m_lContexts.back().SetEndTagSpace( _rxrc.SvEndTagSpace() ); // Its end tag was the last read.
          _EndTag( &m_lContexts.back() );
          if ( fNextTag )
          {
            // Then encountered a next tag without any intervening content.
            _TyXmlWriteTag xwt( StartTag( std::move( _rxrc.GetTagCur() ), true ) ); // push tag on output context stack.
            xwt.Commit( true ); // Write the tag's data. We don't use xwt's lifetime to end the tag because we don't want to recurse here.
            continue; // depth remains the same.
          }
          else
          if ( --nCurDepth == nDepthStart )
          {
            ++nNextTagsWritten;
            --nTagsRemaining;
          }
        }
        else
          fSkipCurrentTag = false;
      }
    }
    while( nTagsRemaining >= 0 );
    return nNextTagsWritten;
  }
  // Add all the current content from passed context.
  template < class t_TyReadCursor >
  void _WriteReadCursorContent( t_TyReadCursor & _rxrc )
  {
    typedef typename t_TyReadCursor::_TyXmlToken _TyXmlToken;
    typename t_TyReadCursor::_TyXmlReadContext & rctxt = _rxrc.GetContextCur();
    rctxt.ApplyContent( 0, rctxt.NContentTokens(),
      [this]( const _TyXmlToken * _pxtBegin, const _TyXmlToken * _pxtEnd )
      {
        for ( const _TyXmlToken * pxtCur = _pxtBegin; _pxtEnd != pxtCur; ++pxtCur )
          WriteToken( *pxtCur );
      }
    );
    rctxt.ClearContent(); // Clear the content as a sanity check. The above doesn't modify the content tokens.
  }
  // Writing what is made - rather than what was read:
  // The strings may be strings, string views or null-terminated strings of any type of character - see SvXmlStrArg(). Each
  //  is checked as what it is: what can't be written throws - a character which XML hasn't, "--" within a comment - and
  //  what must be is escaped - a '<' or an '&' of character data. A reference is written by WriteEntityReference() or
  //  WriteCharReference(): an '&' of a string is the character '&' ( see SetDetectReferences() for those of a tag ).
  // A tag is written when anything within it is - or when it is committed or ended: until then what it has may be added.
  // The xml_write_tag<> which is returned ends the tag when it is destroyed - or by its EndTag().
  // Start a tag - see xml_token<>::SetTagName() for its name and its namespace:
  //  StartTag( name ): the namespace in scope for the prefix of the name - or the default namespace.
  //  StartTag( name, prefix, uri ): that namespace - declared on the tag unless it is the one in scope for the prefix.
  //  StartTag( namespace, name ): that namespace, which is in scope - as AddNamespace() returned it.
  template < class t_TyStrName >
  _TyXmlWriteTag StartTag( t_TyStrName const & _rName )
    requires( CXmlStrArg< t_TyStrName > )
  {
    _TyWriteContext & rwcxNew = _StartMadeTag();
    _StartedMadeTag( rwcxNew, [&]( _TyXmlToken & _rtok ) { _rtok.SetTagName( m_xdcxtDocumentContext, SvXmlStrArg( _rName ) ); } );
    return _TyXmlWriteTag( rwcxNew );
  }
  template < class t_TyStrName, class t_TyStrPrefix, class t_TyStrUri >
  _TyXmlWriteTag StartTag( t_TyStrName const & _rName, t_TyStrPrefix const & _rPrefix, t_TyStrUri const & _rUri )
    requires( CXmlStrArg< t_TyStrName > && CXmlStrArg< t_TyStrPrefix > && CXmlStrArg< t_TyStrUri > )
  {
    _TyWriteContext & rwcxNew = _StartMadeTag();
    _StartedMadeTag( rwcxNew, [&]( _TyXmlToken & _rtok ) { _rtok.SetTagName( m_xdcxtDocumentContext, SvXmlStrArg( _rName ), SvXmlStrArg( _rPrefix ), SvXmlStrArg( _rUri ) ); } );
    return _TyXmlWriteTag( rwcxNew );
  }
  template < class t_TyStrName >
  _TyXmlWriteTag StartTag( _TyXmlNamespaceValueWrap const & _rxnvw, t_TyStrName const & _rName )
    requires( CXmlStrArg< t_TyStrName > )
  {
    _TyWriteContext & rwcxNew = _StartMadeTag();
    _StartedMadeTag( rwcxNew, [&]( _TyXmlToken & _rtok ) { _rtok.SetTagName( m_xdcxtDocumentContext, _rxnvw, SvXmlStrArg( _rName ) ); } );
    return _TyXmlWriteTag( rwcxNew );
  }
  // <name>text</name> - or <name/> if the text is empty.
  template < class t_TyStrName, class t_TyStrText >
  void WriteElement( t_TyStrName const & _rName, t_TyStrText const & _rText )
    requires( CXmlStrArg< t_TyStrName > && CXmlStrArg< t_TyStrText > )
  {
    _TyXmlWriteTag xwt( StartTag( _rName ) );
    WriteCharData( _rText );
  }
  // Character data: the characters which it is - '<', '&' and the '>' of "]]>" are written as references, as is a #xD.
  //  Outside of the document element it may be only white space.
  template < class t_TyStr >
  void WriteCharData( t_TyStr const & _rStr )
    requires( CXmlStrArg< t_TyStr > )
  {
    _CheckNotEnded();
    auto sv = SvXmlStrArg( _rStr );
    if ( sv.empty() )
      return; // Nothing is written - and the tag which it would be within is as it was: <name/> if nothing else is.
    if ( m_fPrettyPrint && _FSkipWhiteSpace( _FIsWhiteSpace( sv.data(), sv.data() + sv.length() ) ) )
      return; // The indentation is in its place - see SetOutputFormat().
    _WriteChecked( [this,&sv]()
      {
        _WriteCharAndAttrString< TGetCharDataStart >( sv.data(), sv.data() + sv.length(), edrNoReferences );
      } );
    _MarkMixed();
    _ValidateCharacterData( _FIsWhiteSpaceAsWritten( sv.data(), sv.data() + sv.length() ) );
  }
  // A reference to the general entity _rName: one of the five which XML predefines ( 4.6 ), or one which the document type
  //  declaration which was written declares - or may declare, if it has an external subset or a reference to a parameter
  //  entity ( 4.1 Entity Declared ). Within the document element.
  template < class t_TyStr >
  void WriteEntityReference( t_TyStr const & _rName )
    requires( CXmlStrArg< t_TyStr > )
  {
    _CheckNotEnded();
    VerifyThrowSz( FInsideDocumentTag(), "A reference is within the document element." );
    _TyStdStr strName;
    {
      auto sv = SvXmlStrArg( _rName );
      ConvertString( strName, sv.data(), sv.length() );
    }
    _CheckNameOfDeclaration( strName, "entity" );
    if ( !_FIsPredefinedEntity( strName ) )
    {
      VerifyThrowSz( m_fWroteDoctypeDecl, "The entity [%s] is not declared - no document type declaration was written.", StrConvertString< char >( strName ).c_str() );
      VerifyThrowSz( !m_fDeclaredEntitiesComplete || ( m_setDeclaredEntities.end() != m_setDeclaredEntities.find( strName ) ),
        "The entity [%s] is not declared by the document type declaration which was written.", StrConvertString< char >( strName ).c_str() );
    }
    _CheckCommitCur();
    _WriteTransportRaw( _TyMarkupTraits::s_kszEntityRefRefBegin, StaticStringLen( _TyMarkupTraits::s_kszEntityRefRefBegin ) );
    _WriteTransportRaw( strName.data(), strName.length() );
    _WriteTransportRaw( _TyMarkupTraits::s_kszReferenceEnd, StaticStringLen( _TyMarkupTraits::s_kszReferenceEnd ) );
    _MarkMixed();
    if ( !!m_upValidator )
      m_upValidator->CharacterData( m_upValidator->FEntityReferenceIsWhiteSpace( strName ) );
  }
  // A reference to the character _ucChar: &#xhhhh; - it is to be a Char [2]. Within the document element.
  void WriteCharReference( char32_t _ucChar )
  {
    _CheckNotEnded();
    VerifyThrowSz( FInsideDocumentTag(), "A reference is within the document element." );
    VerifyThrowSz( FIsXmlChar( _ucChar ), "[U+%X] is not a character of XML.", unsigned( _ucChar ) );
    _CheckCommitCur();
    char rgcHex[ 16 ];
    std::to_chars_result tcr = std::to_chars( rgcHex, rgcHex + sizeof( rgcHex ), uint32_t( _ucChar ), 16 );
    _WriteTransportRaw( _TyMarkupTraits::s_kszCharHexRefBegin, StaticStringLen( _TyMarkupTraits::s_kszCharHexRefBegin ) );
    for ( const char * pc = rgcHex; tcr.ptr != pc; ++pc )
    {
      const _TyChar ch = _TyChar( ( *pc >= 'a' ) ? ( *pc - 'a' + 'A' ) : *pc );
      _WriteTransportRaw( &ch, 1 );
    }
    _WriteTransportRaw( _TyMarkupTraits::s_kszReferenceEnd, StaticStringLen( _TyMarkupTraits::s_kszReferenceEnd ) );
    _MarkMixed();
    _ValidateCharacterData( false ); // A reference to a character isn't white space among elements, whatever the character.
  }
  // <!--text--> - the text hasn't "--" and doesn't end with '-' ( [15] ).
  template < class t_TyStr >
  void WriteComment( t_TyStr const & _rStr )
    requires( CXmlStrArg< t_TyStr > )
  {
    _CheckNotEnded();
    auto sv = SvXmlStrArg( _rStr );
    _WriteChecked( [this,&sv]()
      {
        _FormatBeforeContentMarkup();
        _WriteTransportRaw( _TyMarkupTraits::s_kszCommentBegin, StaticStringLen( _TyMarkupTraits::s_kszCommentBegin ) );
        _WriteCommentString( sv.data(), sv.data() + sv.length() );
        _WriteTransportRaw( _TyMarkupTraits::s_kszCommentEnd, StaticStringLen( _TyMarkupTraits::s_kszCommentEnd ) );
        _FormatAfterTopLevel();
      } );
    _ValidateCommentOrProcessingInstruction();
  }
  // <?target data?> - or <?target?> if the data is empty ( [16] ). The target is a name and not "xml" in any case; the
  //  data hasn't "?>".
  template < class t_TyStrTarget, class t_TyStrData >
  void WriteProcessingInstruction( t_TyStrTarget const & _rTarget, t_TyStrData const & _rData )
    requires( CXmlStrArg< t_TyStrTarget > && CXmlStrArg< t_TyStrData > )
  {
    _CheckNotEnded();
    _TyStdStr strTarget;
    {
      auto sv = SvXmlStrArg( _rTarget );
      ConvertString( strTarget, sv.data(), sv.length() );
    }
    _CheckNameOfDeclaration( strTarget, "processing instruction target" );
    VerifyThrowSz( !( ( 3 == strTarget.length() ) && ( ( _TyChar('x') == strTarget[0] ) || ( _TyChar('X') == strTarget[0] ) ) &&
                      ( ( _TyChar('m') == strTarget[1] ) || ( _TyChar('M') == strTarget[1] ) ) && ( ( _TyChar('l') == strTarget[2] ) || ( _TyChar('L') == strTarget[2] ) ) ),
      "The target of a processing instruction is not \"xml\" - in any case ( [17] )." );
    auto svData = SvXmlStrArg( _rData );
    _WriteChecked( [this,&strTarget,&svData]()
      {
        _FormatBeforeContentMarkup();
        _WriteTransportRaw( _TyMarkupTraits::s_kszProcessingInstructionBegin, StaticStringLen( _TyMarkupTraits::s_kszProcessingInstructionBegin ) );
        _WritePIString< TGetPITargetStart >( strTarget.data(), strTarget.data() + strTarget.length() );
        if ( !svData.empty() )
        {
          _WriteTransportRaw( _TyMarkupTraits::s_kszSpace, StaticStringLen( _TyMarkupTraits::s_kszSpace ) );
          _WritePIString< TGetPITargetMeatStart >( svData.data(), svData.data() + svData.length() );
        }
        _WriteTransportRaw( _TyMarkupTraits::s_kszProcessingInstructionEnd, StaticStringLen( _TyMarkupTraits::s_kszProcessingInstructionEnd ) );
        _FormatAfterTopLevel();
      } );
    _ValidateCommentOrProcessingInstruction();
  }
  // <![CDATA[text]]> - a "]]>" of the text is written in two sections, as is a #xD. Within the document element.
  template < class t_TyStr >
  void WriteCDataSection( t_TyStr const & _rStr )
    requires( CXmlStrArg< t_TyStr > )
  {
    _CheckNotEnded();
    VerifyThrowSz( FInsideDocumentTag(), "A CDATA section is within the document element." );
    auto sv = SvXmlStrArg( _rStr );
    _WriteChecked( [this,&sv]()
      {
        _WriteTransportRaw( _TyMarkupTraits::s_kszCDataSectionBegin, StaticStringLen( _TyMarkupTraits::s_kszCDataSectionBegin ) );
        _WriteCDataString( sv.data(), sv.data() + sv.length() );
        _WriteTransportRaw( _TyMarkupTraits::s_kszCDataSectionEnd, StaticStringLen( _TyMarkupTraits::s_kszCDataSectionEnd ) );
      } );
    _MarkMixed();
    _ValidateCharacterData( false ); // A CDATA section is not white space among elements whatever is in it.
  }
  // The document type declaration - all of its text, from its "<!DOCTYPE" through its '>': it is parsed as that of a
  //  document is - nothing external is read - and the general entities which it declares may then be referred to. Once,
  //  and before the document element.
  template < class t_TyStr >
  void WriteDoctypeDecl( t_TyStr const & _rStr )
    requires( CXmlStrArg< t_TyStr > )
  {
    _WriteDoctypeDeclText( SvXmlStrArg( _rStr ), nullptr );
  }
protected:
  // _pfnValidator: makes the validator of what is written - see SetValidate() - if it is to be of another DTD than that of
  //  the text: one which has all of the declarations.
  template < class t_TyCharDtd >
  void _WriteDoctypeDeclText( basic_string_view< t_TyCharDtd > _sv, const std::function< unique_ptr< _xml_write_validator_base< _TyChar > >() > * _pfnValidator )
  {
    _CheckNotEnded();
    VerifyThrowSz( FInProlog(), "The document type declaration may only be written before the first tag." );
    VerifyThrowSz( !m_fWroteDoctypeDecl, "A document type declaration has already been written - there may be only one." );
    auto sv = _sv;
    typedef t_TyCharDtd _TyCharDtd;
    xml_dtd< _TyCharDtd > dtd;
    xml_dtd_parser< _TyCharDtd >::CheckDoctypeDeclText( sv, dtd );
    // The entities which it declares - all of them, unless there are declarations which weren't read.
    m_setDeclaredEntities.clear();
    for ( auto const & rvtEntity : dtd.m_mapGeneralEntities )
    {
      _TyStdStr strName;
      ConvertString( strName, rvtEntity.first );
      m_setDeclaredEntities.insert( std::move( strName ) );
    }
    m_fDeclaredEntitiesComplete = !dtd.m_optExternalId.has_value() &&
      ( dtd.m_rgItems.end() == std::find_if( dtd.m_rgItems.begin(), dtd.m_rgItems.end(), []( auto const & _ritem ) { return exdiParameterEntityReference == _ritem.m_eItem; } ) );
    _WriteTransportRaw( sv.data(), sv.length() );
    m_fWroteDoctypeDecl = true;
    _FormatAfterTopLevel();
    if ( m_fValidate )
      _SetValidator( ( !!_pfnValidator && !!*_pfnValidator ) ? (*_pfnValidator)() : _FnValidatorOfDtd( make_shared< const xml_dtd< _TyCharDtd > >( std::move( dtd ) ) )() );
  }
public:
  // The document type declaration of _rdtd - its text as it is now: see xml_dtd<>::m_strDoctypeText. A DTD which was read,
  //  edited or made - see xml_dtd<>::Edit().
  template < class t_TyCharDtd >
  void WriteDoctypeDecl( xml_dtd< t_TyCharDtd > const & _rdtd )
  {
    VerifyThrowSz( !_rdtd.m_strDoctypeText.empty(), "The DTD has no document type declaration - see xml_dtd<>::SetRootName()." );
    std::function< unique_ptr< _TyValidatorBase >() > fnValidator;
    if ( m_fValidate )
      fnValidator = _FnValidatorOfDtd( make_shared< const xml_dtd< t_TyCharDtd > >( _rdtd ) );
    _WriteDoctypeDeclText( basic_string_view< t_TyCharDtd >( _rdtd.m_strDoctypeText ), &fnValidator );
  }
  // <!DOCTYPE name PUBLIC "publicId" "systemId" [internalSubset]> - an empty string is one which isn't given: there is a
  //  system identifier if there is a public one. A literal is in '"' unless it has one.
  template < class t_TyStrName, class t_TyStrPublic, class t_TyStrSystem, class t_TyStrSubset >
  void WriteDoctypeDecl( t_TyStrName const & _rName, t_TyStrPublic const & _rPublicId, t_TyStrSystem const & _rSystemId, t_TyStrSubset const & _rInternalSubset )
    requires( CXmlStrArg< t_TyStrName > && CXmlStrArg< t_TyStrPublic > && CXmlStrArg< t_TyStrSystem > && CXmlStrArg< t_TyStrSubset > )
  {
    std::u32string strText( U"<!DOCTYPE " );
    auto Append = [&strText]( auto _sv )
    {
      std::u32string str;
      ConvertString( str, _sv.data(), _sv.length() );
      strText += str;
    };
    auto AppendLiteral = [&strText,&Append]( auto _sv )
    {
      strText += U' ';
      const char32_t kchQuote = ( decltype( _sv )::npos == _sv.find( typename decltype( _sv )::value_type( '"' ) ) ) ? U'"' : U'\'';
      strText += kchQuote;
      Append( _sv );
      strText += kchQuote;
    };
    Append( SvXmlStrArg( _rName ) );
    auto svPublic = SvXmlStrArg( _rPublicId );
    auto svSystem = SvXmlStrArg( _rSystemId );
    VerifyThrowSz( svPublic.empty() || !svSystem.empty(), "A public identifier is followed by a system identifier ( [75] )." );
    if ( !svPublic.empty() )
    {
      strText += U" PUBLIC";
      AppendLiteral( svPublic );
      AppendLiteral( svSystem );
    }
    else
    if ( !svSystem.empty() )
    {
      strText += U" SYSTEM";
      AppendLiteral( svSystem );
    }
    auto svSubset = SvXmlStrArg( _rInternalSubset );
    if ( !svSubset.empty() )
    {
      strText += U" [";
      Append( svSubset );
      strText += U']';
    }
    strText += U'>';
    WriteDoctypeDecl( strText );
  }
  // The document is complete: its element was written and ended. Nothing more may be written.
  // ( The tags are ended by their xml_write_tag<>s - none is open. )
  void EndDocument()
  {
    _CheckNotEnded();
    VerifyThrowSz( !FInsideDocumentTag(), "A tag hasn't been ended - each is ended by its xml_write_tag<>." );
    VerifyThrowSz( m_fWroteFirstTag, "A document has an element ( [1] ) - none was written." );
    m_fEndedDocument = true;
  }
  bool FEndedDocument() const
  {
    return m_fEndedDocument;
  }
  // Start a tag by copying the passed token.
  // If the namespace of the tag isn't current then the namespace is declared as an additional attribute.
  // We also copy current namespace declarations even if they are not referred to by the tag's namespace prefix.
  template < class t_TyXmlToken >
  _TyXmlWriteTag StartTag( t_TyXmlToken const & _rtok )
    requires( CXmlTokenArg< t_TyXmlToken > )
  {
    VerifyThrowSz( _rtok.FIsTag(), "Trying to start a tag with a non-tag token." );
    _CheckStartTag();
    _rtok.AssertValid();
    _ImportEntitiesOfSource( _rtok.GetLexToken().GetUserObj() );
    // Here's how things will work: We call to copy the lex token that is inside of the xml token.
    // We pass the m_xdcxtDocumentContext with the token to copy.
    // If the namespace *reference* on the tag isn't the current URI for that (prefix,URI) then the namespace on the token copy
    //  will be a namespace *declaration* object and a new namespace will have been added.
    // We must, as we have the context here, add this namespace declaration as a new attribute on the tag and then move the
    //  namespace declaration object to the new attribute and make the namespace on the tag a reference to this declaration.
    // Then we will be all set up. Except that we need to do this for any namespace that might also be on attribute names and may
    //  also be declared locally or somewhere where we won't see the declaration.
    // So we need a list of declarations and a list of references and then we need to match the two up.
    typename _TyXmlDocumentContext::_TyTokenCopyContext ctxtTokenCopy;
    _TyXmlToken tokThis( m_xdcxtDocumentContext, _rtok.GetLexToken(), &ctxtTokenCopy );
    // Now inside of ctxtTokenCopy we have two arrays of pointers to names of tag/attrbutes that contain namespace declarations and namespace references.
    // We need to match the declarations to attribute declarations (if they are present). If there is no attribute namespace declaraion for some
    //  set of namespace declarations then those declarations will need new attribute declarations.
    // There is also the possibility of redundant attribute declarations with a namespace reference attached to it. These will be filtered upon write - 
    //  i.e. there is no need to write an attribute declaration that has a namespace reference and not a namespace declaration attached to it.
    // Note that these references shouldn't be part of vknTagName_NNamespaceDeclsIdx's count of namespace declarations - even if the references are part of
    //  a attribute namespace declaration. Only actual declaration objects.
    if ( m_fUseNamespaces )
      tokThis._FixupNamespaceDeclarations( m_xdcxtDocumentContext, ctxtTokenCopy );
    tokThis.AssertValid( m_fUseNamespaces );
    _TyWriteContext & rwcxNew = m_lContexts.emplace_back( this, std::move( tokThis ) );
    rwcxNew.InheritFormat( *std::prev( std::prev( m_lContexts.end() ) ) );
    return _TyXmlWriteTag( rwcxNew );
  }
  // Start a tag by moving its contents into this object.
  // We leave the tag name intact in the old object and we must leave any active namespace declarations there and just copy them.
  template < class t_TyXmlToken >
  _TyXmlWriteTag StartTag( t_TyXmlToken && _rrtok, bool _fKeepTagName = false )
    requires( CXmlTokenArg< t_TyXmlToken > )
  {
    VerifyThrowSz( _rrtok.FIsTag(), "Trying to start a tag with a non-tag token." );
    _CheckStartTag();
    _rrtok.AssertValid();
    _ImportEntitiesOfSource( _rrtok.GetLexToken().GetUserObj() );
    // First make a copy of the tag name's _l_value<>. We shouldn't have to copy anything else since clearly the caller
    //  doesn't care much about the tag's content. We'll copy it back into the tag after moving it.
    typename t_TyXmlToken::_TyLexValue rvNameCopy;
    if ( _fKeepTagName )
      rvNameCopy = _rrtok[vknTagNameIdx][vknNameIdx].GetCopy();
    typename _TyXmlDocumentContext::_TyTokenCopyContext ctxtTokenCopy;
    _TyXmlToken tokThis( m_xdcxtDocumentContext, std::move( _rrtok.GetLexToken() ), &ctxtTokenCopy );
    if ( m_fUseNamespaces )
      tokThis._FixupNamespaceDeclarations( m_xdcxtDocumentContext, ctxtTokenCopy );
    tokThis.AssertValid( m_fUseNamespaces );
    if ( _fKeepTagName )
      _rrtok[vknTagNameIdx][vknNameIdx] = std::move( rvNameCopy );
    _TyWriteContext & rwcxNew = m_lContexts.emplace_back( this, std::move( tokThis ) );
    rwcxNew.InheritFormat( *std::prev( std::prev( m_lContexts.end() ) ) );
    return _TyXmlWriteTag( rwcxNew );
  }
  // The entities of the document which a token comes from: those which its DTD declared, which the user object of its
  //  parser has. We have them as well from here - our tokens are copies and what they have is our user object - so
  //  that we may have the strings of the values which refer to them. They are not entities of the document which we
  //  are writing unless we write the document type declaration which declares them: see _WriteCharAndAttrData().
  // This is called for every token which we are given: nothing is done unless it has another user object than the last.
  template < class t_TyUserObjSource >
  void _ImportEntitiesOfSource( t_TyUserObjSource const & _ruoSource )
  {
    if ( (const void*)&_ruoSource == m_pvUserObjSource )
      return;
    m_pvUserObjSource = (const void*)&_ruoSource;
    if ( (const void*)&_ruoSource == (const void*)&m_xdcxtDocumentContext.GetUserObj() )
      return; // A token of our own.
    _TyEntityMap & rmapOurs = m_xdcxtDocumentContext.GetUserObj().GetEntityMap();
    for ( auto const & rvtEntity : _ruoSource.GetEntityMap() )
    {
      _TyStdStr strName;
      ConvertString( strName, rvtEntity.first );
      if ( rmapOurs.end() != rmapOurs.find( strName ) )
        continue; // One which we have: the predefined ones for a start.
      _TyStdStr strText;
      ConvertString( strText, rvtEntity.second );
      rmapOurs.emplace( strName, std::move( strText ) );
      m_setEntitiesOfSource.insert( std::move( strName ) );
    }
    // And what those of them whose text is other within the value of an attribute stand for there.
    _TyEntityMap & rmapOursAttrValues = m_xdcxtDocumentContext.GetUserObj().GetEntityMapAttrValues();
    for ( auto const & rvtEntity : _ruoSource.GetEntityMapAttrValues() )
    {
      _TyStdStr strName;
      ConvertString( strName, rvtEntity.first );
      if ( ( m_setEntitiesOfSource.end() == m_setEntitiesOfSource.find( strName ) ) || ( rmapOursAttrValues.end() != rmapOursAttrValues.find( strName ) ) )
        continue;
      _TyStdStr strText;
      ConvertString( strText, rvtEntity.second );
      rmapOursAttrValues.emplace( std::move( strName ), std::move( strText ) );
    }
  }
  // We are writing a token - we are not writing the end tag for a token.
  void _CheckCommitCur()
  {
    if ( !FInsideDocumentTag() )
      return;
    // The tag is written - if it hasn't been - and then the end of its start tag: there is content.
    _TyWriteContext & rwcxCur = m_lContexts.back();
    rwcxCur.CommitTagData();
    _CheckWriteTagEnd( true );
  }
  // A dry run: nothing is written while it lasts - see _WriteChecked().
  struct _TyDryRun
  {
    size_t & m_rnDryRun;
    explicit _TyDryRun( size_t & _rnDryRun )
      : m_rnDryRun( _rnDryRun )
    {
      ++m_rnDryRun;
    }
    ~_TyDryRun()
    {
      --m_rnDryRun;
    }
  };
  // What is made is checked before any of it is written: _rftor writes it - it is run once with nothing written, and if
  //  that doesn't throw the tag which it is within is written and it is run again. If it throws nothing was written.
  template < class t_TyFunctor >
  void _WriteChecked( t_TyFunctor const & _rftor )
  {
    {
      _TyDryRun dr( m_nDryRun );
      _rftor();
    }
    _CheckCommitCur();
    _rftor();
  }
  // Check to see if we have started a tag and if so end it allowing for an end tag.
  void _CheckWriteTagEnd( bool _fWithContent )
  {
    if ( m_fHaveUnendedTag )
    {
      m_fHaveUnendedTag = false;
      _WriteTransportRaw( m_strUnendedTagSpace.data(), m_strUnendedTagSpace.length() ); // See _CommitTag().
      m_strUnendedTagSpace.clear();
      if ( _fWithContent )
        _WriteTransportRaw( _TyMarkupTraits::s_kszTagEnd, StaticStringLen( _TyMarkupTraits::s_kszTagEnd ) );
      else
        _WriteTransportRaw( _TyMarkupTraits::s_kszEmptyElemTagEnd, StaticStringLen( _TyMarkupTraits::s_kszEmptyElemTagEnd ) );
    }
  }

  void _EndTag( _TyWriteContext * _pwcxEnd )
  {
    VerifyThrowSz( _pwcxEnd == &m_lContexts.back(), "Ending a tag that is not at the top of the context stack." );
    // So if we currently have an unended tag then there must be no content and we needn't write a separate end tag token.
    // NOPE: We want to be able to duplicate the input as much as possible while still allowing any options for the internal user
    //  so in that regard we will check the token id and when we a s_knTokenSTag we know if was paired with a s_knTokenETag when read from a file.
    bool fHadUnendedTag;
    if ( ( fHadUnendedTag = m_fHaveUnendedTag ) )
      _CheckWriteTagEnd( ( _pwcxEnd->FFullEnd() ) );
    if ( !fHadUnendedTag || ( _pwcxEnd->FFullEnd() ) )
    {
      // We have already validate the tag name so we can just write it:
      // Pretty printing: on a line of its own if what was within it was.
      if ( m_fPrettyPrint && _pwcxEnd->FHadChildMarkup() && _pwcxEnd->FIndentWithin() )
        _WriteNewLineIndent( m_lContexts.size() - 2 );
      _WriteTransportRaw( _TyMarkupTraits::s_kszEndTagBegin, StaticStringLen( _TyMarkupTraits::s_kszEndTagBegin ) );
      _WriteName( _pwcxEnd->GetToken(), _pwcxEnd->GetToken()[vknTagNameIdx] );
      if ( !m_fPrettyPrint ) // Pretty printed there is no white space within a tag but a single space before each attribute.
        _WriteTransportRaw( _pwcxEnd->RStrEndTagSpace().data(), _pwcxEnd->RStrEndTagSpace().length() );
      _WriteTransportRaw( _TyMarkupTraits::s_kszTagEnd, StaticStringLen( _TyMarkupTraits::s_kszTagEnd ) );
    }
    // The validator was told of the tag when it was committed - see _ValidateStartTag().
    if ( !!m_upValidator && ( m_lContexts.size() - 1 == m_nValidatedDepth ) )
    {
      --m_nValidatedDepth;
      m_upValidator->EndTag();
    }
    m_lContexts.pop_back();
    _FormatAfterTopLevel(); // The document element was ended.
  }
  // Write a non-tag token to the output transport at the current position.
  // Validation is performed - e.g. only whitespace chardata is allowed before the first tag.
  // The token is passed the production to which it correspond's state machine to ensure that it matches.
  // If a non-match is found that isn't correctable with a CharRef, etc. then we will throw an error.
  template < class t_TyXmlToken >
  void WriteToken( t_TyXmlToken const & _rtok )
  {
    _ImportEntitiesOfSource( _rtok.GetLexToken().GetUserObj() );
    switch( _rtok.GetTokenId() )
    {
      case s_knTokenSTag:
      case s_knTokenETag:
      case s_knTokenEmptyElemTag:
      case s_knTokenXMLDecl: // This included as it is written automagically and shouldn't be written here.
        VerifyThrowSz( false, "WriteToken() is to be used to write any token except tag-related tokens." );
      break;
      case s_knTokenComment:
        _WriteComment( _rtok );
        _ValidateCommentOrProcessingInstruction();
      break;
      case s_knTokenCDataSection:
        _WriteCDataSection( _rtok );
        _ValidateCharacterData( false );
      break;
      case s_knTokenCharData:
      {
        // Must be a single value here:
        if ( m_fPrettyPrint && _FSkipWhiteSpace( _FIsWhiteSpaceValue( _rtok, _rtok.GetValue() ) ) )
          break; // The indentation is in its place - see SetOutputFormat().
        _CheckCommitCur();
        _WriteCharAndAttrData< TGetCharDataStart >( _rtok, _rtok.GetValue() );
        _MarkMixed();
        _ValidateCharacterDataToken( _rtok, _rtok.GetValue() );
      }
      break;
      case s_knTokenProcessingInstruction:
        _WriteProcessingInstruction( _rtok );
        _ValidateCommentOrProcessingInstruction();
      break;
      case s_knTokenDoctypeDecl:
        _WriteDoctypeDecl( _rtok );
      break;
      case s_knTokenEntityContentBegin:
        _WriteEntityContentBegin( _rtok );
      break;
      case s_knTokenEntityContentEnd:
        _WriteEntityContentEnd();
      break;
    }
  }
  template < class... t_TysArgs >
  _TyXmlTransportOut & emplaceTransport( t_TysArgs&&... _args )
  {
    return m_optTransportOut.emplace( std::forward< t_TysArgs >(_args) ... );
  }
protected:
  // Initialize the xml_writer. Write the XMLDecl root "tag" if that is desired. Regardless create a XMLDecl pseudo
  //  tag as the root of the context list.
  void _Init( _TyXMLDeclProperties const & _rXmlDeclProperties, const _xml_output_format * _pxofFormat, bool _fKeepEncoding )
  {
    _TyPrFormatContext prFormatContext;
    if ( _pxofFormat )
      prFormatContext.first = *_pxofFormat;
    else
    if ( m_optxofFormat.has_value() )
      prFormatContext.first = *m_optxofFormat;
    // Pretty printing - see SetOutputFormat().
    m_fPrettyPrint = !prFormatContext.first.m_fNoFormatting;
    m_strIndent.clear();
    if ( prFormatContext.first.FUseTabsForIndent() )
      m_strIndent.assign( 1, _TyChar( '\t' ) );
    else
      m_strIndent.assign( prFormatContext.first.NSpacesPerIndent(), _TyChar( ' ' ) );
    m_nMaxIndentLevels = prFormatContext.first.m_nMaximumIndentLevelForWhitespace;
    // Always obtain the encoding from the passed XMLDeclProperties.
    m_xdcxtDocumentContext.Init( _fKeepEncoding ? efceFileCharacterEncodingCount : _TyXmlTransportOut::GetEncoding(), m_fUseNamespaces, &_rXmlDeclProperties, &prFormatContext );
    m_lContexts.clear();
    m_fWroteFirstTag = false;
    m_fWroteDoctypeDecl = false;
    m_fEndedDocument = false;
    m_upValidator.reset();
    m_nValidatedDepth = 0;
    m_setDeclaredEntities.clear();
    m_fDeclaredEntitiesComplete = false;
    m_nWithinEntityReference = 0;
    m_setEntitiesOfSource.clear();
    m_pvUserObjSource = nullptr;
    m_fHaveUnendedTag = false;
    m_strUnendedTagSpace.clear();
    m_lContexts.emplace_back( this, m_xdcxtDocumentContext.GetUserObj(), s_knTokenXMLDecl  ); // create the XMLDecl pseudo-tag.
    if ( m_fWriteXMLDecl )
    {
      _WriteXmlDecl();
      _FormatAfterTopLevel();
    }
  }
  template < class t_TyChar >
  void _WriteTransportRaw( t_TyChar const * _pcBegin, t_TyChar const * _pcEnd )
  {
    Assert( !!m_optTransportOut );
    Assert( _pcEnd >=_pcBegin );
    VerifyThrow( !!m_optTransportOut && ( _pcEnd >=_pcBegin ) );
    if ( _pcEnd == _pcBegin )
      return; // no-op.
    if ( !!m_nWithinEntityReference )
      return; // The reference to the entity is what was written for this - see _WriteEntityContentBegin().
    if ( !!m_nDryRun )
      return; // See _WriteChecked().
    m_optTransportOut->Write( _pcBegin, _pcEnd );
  }
  template < class t_TyChar >
  void _WriteTransportRaw( t_TyChar const * _pcBegin, size_t _nchLen )
  {
    _WriteTransportRaw( _pcBegin, _pcBegin + _nchLen );
  }
  template < class t_TyXmlToken >
  void _WriteTypedData(t_TyXmlToken const & _rtok, typename t_TyXmlToken::_TyLexValue const & _rval )
  {
    typedef basic_string_view< typename t_TyXmlToken::_TyChar > _TyStrView;
    if ( _rval.FEmptyTypedData() )
      return; // no-op.
    // The text as it is in the document - not the string of the value: that is without the #xD's of its ends of lines ( 2.11 ).
    typedef _l_data<> _TyData;
    _TyData const & krdt = _rval.GetTypedData();
    VerifyThrowSz( krdt.FContainsSingleDataRange(), "Expected a single piece of the document." );
    _TyStrView sv;
    _rtok.GetLexToken().KGetStringView( sv, krdt.DataRangeGetSingle().GetRangeBase() );
    _WriteTransportRaw( &sv[0], sv.length() );
  }
  // Pretty printing - see xml_writer<>::SetOutputFormat():
  // A new line and the indentation of _nLevel - the element at the top level is at level zero.
  void _WriteNewLineIndent( size_t _nLevel )
  {
    static constexpr _TyChar s_kszNewLine[] = { _TyChar( '\n' ), 0 };
    _WriteTransportRaw( s_kszNewLine, 1 );
    const size_t knLevels = ( std::min )( _nLevel, m_nMaxIndentLevels );
    for ( size_t nLevel = 0; nLevel < knLevels; ++nLevel )
      _WriteTransportRaw( m_strIndent.data(), m_strIndent.length() );
  }
  // Before markup - a tag, a comment, a processing instruction - within _rwcxWithin: within an element which is indented
  //  it is on a line of its own at _nLevel. ( At the top level each line was ended after what was on it - see
  //  _FormatAfterTopLevel(). ) Nothing is done in a dry run.
  void _FormatBefore( _TyWriteContext & _rwcxWithin, size_t _nLevel )
  {
    if ( !m_fPrettyPrint || !!m_nDryRun || ( &_rwcxWithin == &m_lContexts.front() ) || !_rwcxWithin.FIndentWithin() )
      return;
    _WriteNewLineIndent( _nLevel );
    _rwcxWithin.SetHadChildMarkup();
  }
  // Before markup within the current element - or at the top level.
  void _FormatBeforeContentMarkup()
  {
    _FormatBefore( m_lContexts.back(), m_lContexts.size() - 1 );
  }
  // After what is at the top level - the XML declaration, the document type declaration, a comment, a processing
  //  instruction, the document element: its line is ended.
  void _FormatAfterTopLevel()
  {
    static constexpr _TyChar s_kszNewLine[] = { _TyChar( '\n' ), 0 };
    if ( m_fPrettyPrint && !m_nDryRun && ( 1 == m_lContexts.size() ) )
      _WriteTransportRaw( s_kszNewLine, 1 );
  }
  // Text - character data, a CDATA section, a reference - was written within the current element: its content is mixed and
  //  nothing more within it is indented.
  void _MarkMixed()
  {
    if ( FInsideDocumentTag() && !m_nDryRun )
      m_lContexts.back().SetMixed();
  }
  // Whether character data isn't written when pretty printing: white space where the indentation is in its place - between
  //  the markup within an element which is indented and at the top level.
  bool _FSkipWhiteSpace( bool _fIsWhiteSpace ) const
  {
    return m_fPrettyPrint && _fIsWhiteSpace && ( !FInsideDocumentTag() || m_lContexts.back().FIndentWithin() );
  }
  template < class t_TyChar >
  static bool _FIsWhiteSpace( const t_TyChar * _pcBegin, const t_TyChar * const _pcEnd )
  {
    for ( ; _pcEnd != _pcBegin; ++_pcBegin )
      if ( ( t_TyChar( 0x20 ) != *_pcBegin ) && ( t_TyChar( 0x9 ) != *_pcBegin ) && ( t_TyChar( 0xA ) != *_pcBegin ) && ( t_TyChar( 0xD ) != *_pcBegin ) )
        return false;
    return true;
  }
  // Whether the character data of a token is white space only - text which is that and no reference.
  template < class t_TyXmlToken >
  static bool _FIsWhiteSpaceValue( t_TyXmlToken const & _rtok, typename t_TyXmlToken::_TyLexValue const & _rval )
  {
    if ( !_rval.FHasTypedData() )
    {
      bool fIsWhiteSpace = true;
      _rval.ApplyString(
        [&fIsWhiteSpace]< typename t_TyChar >( const t_TyChar * _pcBegin, const t_TyChar * const _pcEnd )
        {
          fIsWhiteSpace = _FIsWhiteSpace( _pcBegin, _pcEnd );
        }
      );
      return fIsWhiteSpace;
    }
    _l_data<> const & krdt = _rval.GetTypedData();
    if ( krdt.FIsNull() )
      return true;
    bool fIsWhiteSpace = true;
    krdt.Apply(
      [&_rtok,&fIsWhiteSpace]( const _l_data_typed_range * _pdtrBegin, const _l_data_typed_range * _pdtrEnd )
      {
        for ( const _l_data_typed_range * pdtrCur = _pdtrBegin; fIsWhiteSpace && ( _pdtrEnd != pdtrCur ); ++pdtrCur )
        {
          if ( s_kdtPlainText != DtGetType( pdtrCur->type() ) )
          {
            fIsWhiteSpace = false;
            break;
          }
          basic_string_view< typename t_TyXmlToken::_TyChar > sv;
          _rtok.GetLexToken().KGetStringView( sv, pdtrCur->GetRangeBase() );
          fIsWhiteSpace = _FIsWhiteSpace( sv.data(), sv.data() + sv.length() );
        }
      }
    );
    return fIsWhiteSpace;
  }
  // xml:space ( 2.10 ) of the tag which is committed: "preserve" - its white space is kept and nothing within it is
  //  indented - or "default". Otherwise it is as the element which it is within.
  template < class t_TyXmlToken >
  void _SetXmlSpace( t_TyXmlToken const & _rtok, _TyWriteContext & _rwcx )
  {
    typedef typename t_TyXmlToken::_TyLexValue _TyLexValue;
    typedef typename t_TyXmlToken::_TyChar _TyCharTok;
    typedef basic_string_view< _TyCharTok > _TyStrViewTok;
    auto FEquals = []( _TyStrViewTok _sv, const char * _psz ) -> bool
    {
      const size_t knLen = strlen( _psz );
      if ( knLen != _sv.length() )
        return false;
      for ( size_t n = 0; n < knLen; ++n )
        if ( _sv[n] != _TyCharTok( _psz[n] ) )
          return false;
      return true;
    };
    const _TyLexValue & rrgAttrs = _rtok.GetValue()[vknAttributesIdx];
    for ( size_t nAttr = 0; nAttr < rrgAttrs.GetSize(); ++nAttr )
    {
      const _TyLexValue & rvalAttr = rrgAttrs[nAttr];
      _TyStrViewTok svName;
      basic_string< _TyCharTok > strName;
      (void)_rtok.GetLexToken().FGetStringViewOrString( svName, strName, rvalAttr[vknNameIdx] );
      // Its name is as it was read or made: with its prefix, or without it when the namespace has it.
      const _TyLexValue & rvalNS = rvalAttr[vknNamespaceIdx];
      const bool kfPrefixXml = rvalNS.template FIsA< xml_namespace_value_wrap< _TyCharTok > >() &&
        FEquals( _TyStrViewTok( rvalNS.template GetVal< xml_namespace_value_wrap< _TyCharTok > >().RStringPrefix() ), "xml" );
      if ( !FEquals( svName, "xml:space" ) && !( kfPrefixXml && FEquals( svName, "space" ) ) )
        continue;
      const _TyLexValue & rvalValue = rvalAttr[vknAttr_ValueIdx];
      basic_string< _TyCharTok > strValue;
      if ( FIsAttrDefaultValue( rvalValue ) )
        strValue = rvalValue.template GetVal< xml_attr_default_value< _TyCharTok > >().SvValue();
      else
        _rtok.GetLexToken().GetString( strValue, rvalValue );
      if ( FEquals( strValue, "preserve" ) )
        _rwcx.SetPreserveSpace( true );
      else
      if ( FEquals( strValue, "default" ) )
        _rwcx.SetPreserveSpace( false );
    }
  }
// Validation - see SetValidate():
  typedef _xml_write_validator_base< _TyChar > _TyValidatorBase;
  typedef typename _TyValidatorBase::_TyAttr _TyValidatorAttr;
  // The characters of a DTD of the size of ours.
  typedef conditional_t< sizeof( _TyChar ) == sizeof( char8_t ), char8_t, conditional_t< sizeof( _TyChar ) == sizeof( char16_t ), char16_t, char32_t > > _TyCharDtdOfSize;
  // What makes a validator of the DTD.
  template < class t_TyCharDtd >
  static std::function< unique_ptr< _TyValidatorBase >() > _FnValidatorOfDtd( shared_ptr< const xml_dtd< t_TyCharDtd > > const & _rspDtd )
  {
    return [_rspDtd]() -> unique_ptr< _TyValidatorBase >
    {
      return make_unique< _xml_write_validator< _TyChar, t_TyCharDtd > >( _rspDtd );
    };
  }
  void _SetValidator( unique_ptr< _TyValidatorBase > && _rrupValidator )
  {
    m_upValidator = std::move( _rrupValidator );
    m_upValidator->SetThrowOnError( m_fThrowOnValidityError );
    _TyXMLDeclProperties const & rxdp = m_xdcxtDocumentContext.GetXMLDeclProperties();
    m_upValidator->SetStandalone( rxdp.m_fHasStandalone && rxdp.m_fStandalone );
    m_upValidator->SetUseXMLNamespaces( m_fUseNamespaces );
    m_nValidatedDepth = 0;
  }
  // The name as it is written - the DTD knows the names so: prefix and all.
  template < class t_TyXmlToken >
  void _GetNameAsWritten( t_TyXmlToken const & _rtok, typename t_TyXmlToken::_TyLexValue const & _rvalRgName, _TyStdStr & _rstrName ) const
  {
    typedef typename t_TyXmlToken::_TyLexValue _TyLexValue;
    _rstrName.clear();
    const _TyLexValue & rvalName = _rvalRgName[vknNameIdx];
    const _TyLexValue & rvalNS = _rvalRgName[vknNamespaceIdx];
    if ( !rvalNS.FIsBool() && !m_xdcxtDocumentContext.FIncludePrefixesInAttrNames() ) // As _WriteName() writes it.
    {
      const _TyXmlNamespaceValueWrap & rxnvw = rvalNS.template GetVal< _TyXmlNamespaceValueWrap >();
      if ( !rxnvw.FIsAttributeNamespaceDeclaration() && !rxnvw.RStringPrefix().empty() )
      {
        _rstrName += rxnvw.RStringPrefix();
        _rstrName += _TyChar( ':' );
      }
    }
    basic_string_view< _TyChar > sv;
    _TyStdStr str;
    (void)_rtok.GetLexToken().FGetStringViewOrString( sv, str, rvalName );
    _rstrName += sv;
  }
  // The tag is committed: the validator is told of it with the attributes which are written.
  template < class t_TyXmlToken >
  void _ValidateStartTag( t_TyXmlToken const & _rtok )
  {
    typedef typename t_TyXmlToken::_TyLexValue _TyLexValue;
    if ( !m_upValidator ) // No document type declaration was written: nothing is valid.
      _SetValidator( make_unique< _xml_write_validator< _TyChar, _TyCharDtdOfSize > >( nullptr ) );
    const _TyLexValue & rvalRoot = _rtok.GetValue();
    _GetNameAsWritten( _rtok, rvalRoot[vknTagNameIdx], m_strValidateName );
    const _TyLexValue & rrgAttrs = rvalRoot[vknAttributesIdx];
    const size_t knAttrs = rrgAttrs.GetSize();
    // The strings are all there before any view of one is had.
    if ( m_rgstrValidate.size() < 2 * knAttrs )
      m_rgstrValidate.resize( 2 * knAttrs );
    m_rgValidateAttrs.clear();
    for ( size_t nAttr = 0; nAttr < knAttrs; ++nAttr )
    {
      const _TyLexValue & rvalAttr = rrgAttrs[nAttr];
      const _TyLexValue & rvalValue = rvalAttr[vknAttr_ValueIdx];
      const bool kfAttrDefaultValue = FIsAttrDefaultValue( rvalValue );
      if ( kfAttrDefaultValue && !FWriteDefaultedAttributes() )
        continue; // Not written: whoever reads what we write has it from the DTD.
      _TyValidatorAttr & rattr = m_rgValidateAttrs.emplace_back();
      _TyStdStr & rstrName = m_rgstrValidate[ 2 * nAttr ];
      _GetNameAsWritten( _rtok, rvalAttr, rstrName );
      rattr.m_svName = rstrName;
      _TyStdStr & rstrValue = m_rgstrValidate[ 2 * nAttr + 1 ];
      rstrValue.clear();
      basic_string_view< _TyChar > svValue;
      if ( kfAttrDefaultValue )
        rstrValue = rvalValue.template GetVal< xml_attr_default_value< typename t_TyXmlToken::_TyChar > >().SvValue();
      else
      if ( _rtok.GetLexToken().FGetStringViewOrString( svValue, rstrValue, rvalValue ) )
        rstrValue = svValue;
      rattr.m_svValue = rstrValue;
      // A string is read as it is - but for the normalization by the type of the attribute; what was read is that already.
      rattr.m_fNormalizeByType = rvalValue.FIsString();
    }
    m_upValidator->StartTag( m_strValidateName, m_rgValidateAttrs.data(), m_rgValidateAttrs.data() + m_rgValidateAttrs.size() );
    m_nValidatedDepth = m_lContexts.size() - 1;
  }
  void _ValidateCharacterData( bool _fWhiteSpace )
  {
    if ( !!m_upValidator && FInsideDocumentTag() )
      m_upValidator->CharacterData( _fWhiteSpace );
  }
  void _ValidateCommentOrProcessingInstruction()
  {
    if ( !!m_upValidator && FInsideDocumentTag() )
      m_upValidator->CommentOrProcessingInstruction();
  }
  // Character data which is a string is white space - which matches S [3] - when it is read if each character is a
  //  space, a tab or a #xA: a #xD is written as a reference to it.
  template < class t_TyCharStr >
  static bool _FIsWhiteSpaceAsWritten( const t_TyCharStr * _pcBegin, const t_TyCharStr * const _pcEnd )
  {
    for ( ; _pcEnd != _pcBegin; ++_pcBegin )
      if ( !( ( t_TyCharStr( 0x20 ) == *_pcBegin ) || ( t_TyCharStr( 0x9 ) == *_pcBegin ) || ( t_TyCharStr( 0xa ) == *_pcBegin ) ) )
        return false;
    return true;
  }
  // Character data of a token: a string - or the text of a document, which is white space where it is and where the
  //  references are to entities whose text is. A reference to a character is not.
  template < class t_TyXmlToken >
  void _ValidateCharacterDataToken( t_TyXmlToken const & _rtok, typename t_TyXmlToken::_TyLexValue const & _rval )
  {
    if ( !m_upValidator || !FInsideDocumentTag() )
      return;
    if ( !_rval.FHasTypedData() )
    {
      bool fWhiteSpace = true;
      _rval.ApplyString(
        [&fWhiteSpace]< typename t_TyCharStr >( const t_TyCharStr * _pcBegin, const t_TyCharStr * const _pcEnd )
        {
          fWhiteSpace = _FIsWhiteSpaceAsWritten( _pcBegin, _pcEnd );
        }
      );
      m_upValidator->CharacterData( fWhiteSpace );
      return;
    }
    bool fWhiteSpace = true;
    _l_data<> const & krdt = _rval.GetTypedData();
    if ( !krdt.FIsNull() )
    {
      krdt.Apply(
        [this,&_rtok,&fWhiteSpace]( const _l_data_typed_range * _pdtrBegin, const _l_data_typed_range * _pdtrEnd )
        {
          // ( Each reference to an entity is looked at: what it refers to may be not known. )
          for ( const _l_data_typed_range * pdtrCur = _pdtrBegin; _pdtrEnd != pdtrCur; ++pdtrCur )
          {
            if ( pdtrCur->FIsNull() )
              continue;
            const vtyDataType kdt = DtGetType( pdtrCur->type() );
            if ( ( s_kdtEntityRef != kdt ) && !fWhiteSpace )
              continue;
            basic_string_view< typename t_TyXmlToken::_TyChar > sv;
            _rtok.GetLexToken().KGetStringView( sv, pdtrCur->GetRangeBase() );
            if ( s_kdtPlainText == kdt )
              fWhiteSpace = _FIsWhiteSpace( sv.data(), sv.data() + sv.length() );
            else
            if ( s_kdtEntityRef == kdt )
            {
              _TyStdStr strName;
              ConvertString( strName, sv.data(), sv.length() );
              if ( !m_upValidator->FEntityReferenceIsWhiteSpace( strName ) )
                fWhiteSpace = false;
            }
            else
              fWhiteSpace = false;
          }
        }
      );
    }
    m_upValidator->CharacterData( fWhiteSpace );
  }
  void _CheckNotEnded() const
  {
    VerifyThrowSz( !m_fEndedDocument, "The document was ended - see EndDocument()." );
  }
  // Before a tag is started: there is one document element ( [1] ).
  void _CheckStartTag()
  {
    _CheckNotEnded();
    VerifyThrowSz( !FInEpilog(), "The document has one element - and it was written ( [1] )." );
    _CheckCommitCur(); // The tag which this one is within is written.
  }
  // The context of a tag which is made: see StartTag(). What is given to such a tag is strings: an '&' of one is the
  //  character - as it is of what is given to the writer.
  _TyWriteContext & _StartMadeTag()
  {
    _CheckStartTag();
    _TyWriteContext & rwcxNew = m_lContexts.emplace_back( this, m_xdcxtDocumentContext.GetUserObj(), s_knTokenEmptyElemTag );
    rwcxNew.InheritFormat( *std::prev( std::prev( m_lContexts.end() ) ) );
    rwcxNew.SetDetectReferences( edrNoReferences );
    return rwcxNew;
  }
  // The tag is named - if that throws there is no tag.
  template < class t_TyFunctor >
  void _StartedMadeTag( _TyWriteContext & _rwcxNew, t_TyFunctor && _rrftor )
  {
    Assert( &_rwcxNew == &m_lContexts.back() );
    try
    {
      std::forward< t_TyFunctor >( _rrftor )( _rwcxNew.GetToken() );
    }
    catch ( ... )
    {
      m_lContexts.pop_back();
      throw;
    }
  }
  // The name of an entity or the target of a processing instruction: a Name [5] - and with namespaces one without a
  //  colon ( Namespaces in XML 7 ).
  void _CheckNameOfDeclaration( _TyStdStr const & _rstrName, const char * _pszWhat ) const
  {
    if ( m_fUseNamespaces )
      VerifyThrowSz( !_rstrName.empty() && ( _TyXmlToken::NLenNCName( _rstrName ) == _rstrName.length() ), "The %s [%s] is not an NCName - a name without a colon.", _pszWhat, StrConvertString< char >( _rstrName ).c_str() );
    else
      _TyXmlToken::ValidateName( _rstrName );
  }
  // amp, lt, gt, apos, quot ( 4.6 ).
  static bool _FIsPredefinedEntity( _TyStdStr const & _rstrName )
  {
    static const char * const s_rgpszPredefined[] = { "amp", "lt", "gt", "apos", "quot" };
    for ( const char * pszPredefined : s_rgpszPredefined )
    {
      const size_t knLen = strlen( pszPredefined );
      if ( ( knLen == _rstrName.length() ) && std::equal( _rstrName.begin(), _rstrName.end(), pszPredefined, []( _TyChar _chL, char _chR ) { return _chL == _TyChar( _chR ); } ) )
        return true;
    }
    return false;
  }
  // The text of a comment - checked: it hasn't "--" and doesn't end with '-' ( [15] ).
  template < class t_TyChar >
  void _WriteCommentString( const t_TyChar * _pcBegin, const t_TyChar * const _pcEnd )
  {
    if ( _pcEnd == _pcBegin )
      return; // <!---->
    // No anti-accepting states in the the CommentChar production - no need to obtain the accept state. No fancy processing for comments.
    VerifyThrowSz( _pcEnd == _l_match< t_TyChar >::PszMatch( PspGetCommentCharsStart< t_TyChar >(), _pcBegin, ( _pcEnd - _pcBegin ) ),
      "Invalid comment - did you put two dashes in a row - or end it with a dash?" );
    _WriteTransportRaw( _pcBegin, _pcEnd );
  }
  // The target or the rest of a processing instruction - checked against t_tempGetStartToken.
  template < template < class t_TyChar > class t_tempGetStartToken, class t_TyChar >
  void _WritePIString( const t_TyChar * _pcBegin, const t_TyChar * const _pcEnd )
  {
    typedef _l_state_proto< t_TyChar > _TyStateProto;
    const _TyStateProto * const kpspValidateTokenStart = t_tempGetStartToken< t_TyChar >::s_kpspStart;
    const bool kfIsPITargetMeat = ( kpspValidateTokenStart == TGetPITargetMeatStart< t_TyChar >::s_kpspStart );
    // In this case there is no remedy to any encountered anti-accept state, but we want to know if we hit one
    //  so we can give a more informative error message.
    const _TyStateProto * pspAccept;
    const t_TyChar * pcMatch = _l_match< t_TyChar >::PszMatch( kpspValidateTokenStart, _pcBegin, _pcEnd - _pcBegin, &pspAccept );
    VerifyThrowSz( !( !pspAccept || pspAccept->FIsAntiAcceptingState() || ( pcMatch != _pcEnd ) ),
      kfIsPITargetMeat ? "Invalid characters found in PITargetMeat - or \"?>\"." : "Invalid characters found in PITarget." );
    _WriteTransportRaw( _pcBegin, _pcEnd );
  }
  // The text of a CDATA section - checked: each "]]>" of it is written as "]]]]><![CDATA[>" - in two sections.
  template < class t_TyChar >
  void _WriteCDataString( const t_TyChar * _pcBegin, const t_TyChar * const _pcEnd )
  {
    typedef _l_state_proto< t_TyChar > _TyStateProto;
    // Move through and appropriately replace each found "]]>" with "]]]]><![CDATA[>", writing as we go.
    // Since we are using an anti-accepting state in the CDCharsOutputValidate production we need to examine the returned state for anti-accepting.
    const _TyStateProto * pspCDCharsOutputValidate = PspGetCDCharsOutputValidateStart< t_TyChar >();
    for ( const t_TyChar * pcCur = _pcBegin; _pcEnd != pcCur;  )
    {
      const _TyStateProto * pspAccept;
      const t_TyChar * pcMatch = _l_match< t_TyChar >::PszMatch( pspCDCharsOutputValidate, pcCur, _pcEnd - pcCur, &pspAccept );
      // If we find an accepting state that doesn't complete the entire remainder of characters then we are at a failure point:
      //  there are no transitions from this final accepting state.
      // However if we find an anti-accepting state then that means we have encountered an embedded "]]>" and we can appropriately escape.
      VerifyThrowSz( !( !pcMatch || !pspAccept || ( !pspAccept->FIsAntiAcceptingState() && ( pcMatch != _pcEnd ) ) ),
        "Invalid characters found inside of a CDataSection." );
      // Write what we can. We should be pointing just after the "]]>" so we should be able to write all the way to pcMatch-1:
      if ( pcMatch == _pcEnd ) // most likely occurence
      {
        Assert( !pspAccept->FIsAntiAcceptingState() );
        _WriteCDataRun( pcCur, _pcEnd );
        return; // all done.
      }
      Assert( ( pcMatch[-3] == t_TyChar(']') ) && ( pcMatch[-2] == t_TyChar(']') ) && ( pcMatch[-1] == t_TyChar('>') ) );
      // The "]]>" is replaced - all three of its characters: what is before it is written and then the replacement.
      _WriteCDataRun( pcCur, pcMatch-3 );
      pcCur = pcMatch;
      _WriteTransportRaw( _TyMarkupTraits::s_kszCDataSectionReplaceEnd, StaticStringLen( _TyMarkupTraits::s_kszCDataSectionReplaceEnd ) );
    }
  }
  // This just commits the start of the tag. And we can't write the last ">" or "/>" until
  //  we know if there will be sub-tag data at all. This is because one invariant is that we
  //  never seek the output file which allows us to write to stdout, etc.
  // We will set the variable m_fHaveUnendedTag to true to indicate that either a ">" or "/>"
  //  needs writing.
  // Validation:
  // 1) Tag and attribute names have already been validated as they were accumulated.
  // 2) Naemspaces were validated for activity at a given usage point, and tag
  //  declarations which already had a prefix are re-namespaced when that same prefix
  //  is redeclared on that tag. 
  // 3) Need to validate that there are no duplicate attribute declarations. This include unique prefixes using the same system.
  template < class t_TyXmlToken >
  void _CommitTag( t_TyXmlToken const & _rtok, EDetectReferences _edrDetectReferences )
  {
    typedef typename t_TyXmlToken::_TyLexValue _TyLexValue;
    _CheckWriteTagEnd( true ); // We might have an unended tag and since we are a sub tag then that tag has content.
    // We shouldn't need to validate the attributes if this token came from an xml_read_cursor - as it would have been validated on the way in.
    // We must check all attribute (name,value) pairs and if they are all typed data (i.e. not user added strings) then they all came from
    //  an xml_read_cursor and they needn't be validated.
    CheckDuplicateTokenAttrs( true, _rtok.GetLexToken(), true );
    // The values which are strings - those of a tag which was made, and those which were set - are checked before any of
    //  the tag is written: if one can't be written nothing is.
    {
      const _TyLexValue & rrgAttrsCheck = _rtok.GetValue()[vknAttributesIdx];
      _TyDryRun dr( m_nDryRun );
      for ( size_t nAttr = 0; nAttr < rrgAttrsCheck.GetSize(); ++nAttr )
      {
        const _TyLexValue & rvalValue = rrgAttrsCheck[nAttr][vknAttr_ValueIdx];
        if ( !rvalValue.FIsString() )
          continue;
        if ( rrgAttrsCheck[nAttr][vknAttr_FDoubleQuoteIdx].template GetVal< bool >() )
          _WriteCharAndAttrData< TGetAttCharDataNoDoubleQuoteStart >( _rtok, rvalValue, _edrDetectReferences );
        else
          _WriteCharAndAttrData< TGetAttCharDataNoSingleQuoteStart >( _rtok, rvalValue, _edrDetectReferences );
      }
    }
    if ( m_fValidate )
      _ValidateStartTag( _rtok );
    // Pretty printing: the tag is on a line of its own within an element which is indented - and xml:space of the tag is
    //  had for what is within it. ( The tag which is committed is the current one. )
    if ( m_fPrettyPrint )
    {
      Assert( ( m_lContexts.size() >= 2 ) && ( &_rtok == &m_lContexts.back().GetToken() ) );
      _FormatBefore( *std::prev( std::prev( m_lContexts.end() ) ), m_lContexts.size() - 2 );
      _SetXmlSpace( _rtok, m_lContexts.back() );
    }
    _WriteTransportRaw( _TyMarkupTraits::s_kszTagBegin, StaticStringLen( _TyMarkupTraits::s_kszTagBegin ) );
    m_fHaveUnendedTag = true;
    // We are no longer in the prolog: a string which is written from here - the value of an attribute of this very tag
    //  for one - is validated as what it is and not as the white space which is all that the prolog may have.
    m_fWroteFirstTag = true;
    const _TyLexValue & rvalRoot = _rtok.GetValue();
    _WriteName( _rtok, rvalRoot[vknTagNameIdx] );
    // The white space of the tag as it was written - if it was read.
    _TyTagSpaces tagSpaces;
    if ( !m_fPrettyPrint ) // Pretty printed it is a single space before each attribute.
      _GetTagSpaces( _rtok, tagSpaces );
    m_strUnendedTagSpace = tagSpaces.m_strBeforeEnd;
    // Move through all attributes writing each.
    const _TyLexValue & rrgAttrs = rvalRoot[vknAttributesIdx];
    size_t nAttrs = rrgAttrs.GetSize();
    rrgAttrs.GetValueArray().ApplyContiguous( 0, nAttrs,
      [this,&_rtok,_edrDetectReferences,&tagSpaces]( const _TyLexValue * _pvBegin, const _TyLexValue * _pvEnd )
      {
        for ( const _TyLexValue * pvCur = _pvBegin; _pvEnd != pvCur; ++pvCur )
        {
          // An attribute which the tag didn't have and which the DTD gave it - see SetWriteDefaultedAttributes().
          const bool kfAttrDefaultValue = FIsAttrDefaultValue( (*pvCur)[vknAttr_ValueIdx] );
          if ( kfAttrDefaultValue && !FWriteDefaultedAttributes() )
            continue;
          const _TyLexValue & rvCur = *pvCur;
          // The attribute as it was written - found by where its name is: one which wasn't has a space before it.
          const typename _TyTagSpaces::_TyAttr * pattrSpaces = nullptr;
          if ( tagSpaces.m_fHave && !kfAttrDefaultValue && rvCur[vknNameIdx].FHasTypedData() )
          {
            const vtyDataPosition kposName = rvCur[vknNameIdx].template GetVal< _l_data<> >().DataRangeGetSingle().begin();
            for ( typename _TyTagSpaces::_TyAttr const & rattr : tagSpaces.m_rgAttrs )
              if ( rattr.m_posName == kposName )
              {
                pattrSpaces = &rattr;
                break;
              }
          }
          if ( !!pattrSpaces )
            _WriteTransportRaw( pattrSpaces->m_strBefore.data(), pattrSpaces->m_strBefore.length() );
          else
            _WriteTransportRaw( _TyMarkupTraits::s_kszSpace, StaticStringLen( _TyMarkupTraits::s_kszSpace ) );
          _WriteName( _rtok, rvCur );
          if ( !!pattrSpaces )
            _WriteTransportRaw( pattrSpaces->m_strEq.data(), pattrSpaces->m_strEq.length() );
          else
            _WriteTransportRaw( _TyMarkupTraits::s_kszEqualSign, StaticStringLen( _TyMarkupTraits::s_kszEqualSign ) );
          // The tag itself specifies whether it wants double or single quotes.
          bool fUseDoubleQuote = rvCur[vknAttr_FDoubleQuoteIdx].template GetVal< bool >();
          _WriteTransportRaw( fUseDoubleQuote ? _TyMarkupTraits::s_kszDoubleQuote : _TyMarkupTraits::s_kszSingleQuote, StaticStringLen( _TyMarkupTraits::s_kszSingleQuote ) );
          // Now write the value using the appropriate start token as the validator.
          if ( kfAttrDefaultValue )
            _WriteAttrDefaultValue( rvCur[vknAttr_ValueIdx].template GetVal< xml_attr_default_value< typename t_TyXmlToken::_TyChar > >().SvValue(), fUseDoubleQuote );
          else
          if ( fUseDoubleQuote )
            _WriteCharAndAttrData< TGetAttCharDataNoDoubleQuoteStart >( _rtok, rvCur[vknAttr_ValueIdx], _edrDetectReferences );
          else
            _WriteCharAndAttrData< TGetAttCharDataNoSingleQuoteStart >( _rtok, rvCur[vknAttr_ValueIdx], _edrDetectReferences );
          _WriteTransportRaw( fUseDoubleQuote ? _TyMarkupTraits::s_kszDoubleQuote : _TyMarkupTraits::s_kszSingleQuote, StaticStringLen( _TyMarkupTraits::s_kszSingleQuote ) );
        }
      }
    );
    // we don't write any ending since we don't know yet which ending we should write. This also lets us
    //  know what we need to do when we are ending a tag.
  }
  // The white space within a start tag as it was written: before each attribute and around its '=' - S? '=' S? [25] -
  //  and before the end of the tag. A tag which was read has all of its text: its token keeps it, from its '<' to its
  //  '>', whatever the transport - the text is in memory, or the token has a copy of it ( _l_transport_backed_ctxt ).
  //  The pieces of the tag are positions in that text. An attribute which isn't in the text - the DTD gave it, or it
  //  was added - has a space before it and nothing around its '='.
  struct _TyTagSpaces
  {
    struct _TyAttr
    {
      vtyDataPosition m_posName; // Where its name begins.
      _TyStdStr m_strBefore; // S [40], [44].
      _TyStdStr m_strEq; // Eq [25].
    };
    vector< _TyAttr > m_rgAttrs;
    _TyStdStr m_strBeforeEnd; // S? before the '>' or the "/>".
    bool m_fHave{ false }; // The text of the tag was had - and is a tag as we know them.
  };
  template < class t_TyXmlToken >
  static void _GetTagSpaces( t_TyXmlToken const & _rtok, _TyTagSpaces & _rts )
  {
    typedef typename t_TyXmlToken::_TyChar _TyCharTok;
    auto const & rltok = _rtok.GetLexToken();
    _l_data_range dr;
    rltok.GetTokenDataRange( dr );
    if ( dr.FIsNull() || ( dr.end() <= dr.begin() ) )
      return; // Not read: made.
    basic_string_view< _TyCharTok > sv;
    rltok.KGetStringView( sv, dr );
    if ( ( sv.length() < 3 ) || ( _TyCharTok( '<' ) != sv[0] ) )
      return;
    auto FIsS = []( _TyCharTok _ch ) { return ( _TyCharTok( 0x20 ) == _ch ) || ( _TyCharTok( 0x9 ) == _ch ) || ( _TyCharTok( 0xd ) == _ch ) || ( _TyCharTok( 0xa ) == _ch ); };
    // The white space and the '=' are characters of ASCII: the same in each encoding.
    auto StrOf = []( basic_string_view< _TyCharTok > _svPart )
    {
      _TyStdStr str;
      str.reserve( _svPart.length() );
      for ( _TyCharTok ch : _svPart )
        str += _TyChar( ch );
      return str;
    };
    const size_t knLen = sv.length();
    size_t n = 1;
    while ( ( n < knLen ) && !FIsS( sv[n] ) && ( _TyCharTok( '>' ) != sv[n] ) && ( _TyCharTok( '/' ) != sv[n] ) )
      ++n; // The name of the tag.
    for ( ; ; )
    {
      const size_t knPosSpace = n;
      while ( ( n < knLen ) && FIsS( sv[n] ) )
        ++n;
      if ( n >= knLen )
        return;
      if ( ( _TyCharTok( '>' ) == sv[n] ) || ( _TyCharTok( '/' ) == sv[n] ) )
      {
        _rts.m_strBeforeEnd = StrOf( sv.substr( knPosSpace, n - knPosSpace ) );
        _rts.m_fHave = true;
        return;
      }
      typename _TyTagSpaces::_TyAttr attr;
      attr.m_posName = dr.begin() + vtyDataPosition( n );
      attr.m_strBefore = StrOf( sv.substr( knPosSpace, n - knPosSpace ) );
      while ( ( n < knLen ) && !FIsS( sv[n] ) && ( _TyCharTok( '=' ) != sv[n] ) )
        ++n; // The name of the attribute.
      const size_t knPosEq = n;
      while ( ( n < knLen ) && ( _TyCharTok( '"' ) != sv[n] ) && ( _TyCharTok( '\'' ) != sv[n] ) )
        ++n;
      if ( n >= knLen )
        return;
      attr.m_strEq = StrOf( sv.substr( knPosEq, n - knPosEq ) );
      const size_t knPosClose = sv.find( sv[n], n + 1 ); // The value has no character which is its quote.
      if ( basic_string_view< _TyCharTok >::npos == knPosClose )
        return;
      n = knPosClose + 1;
      _rts.m_rgAttrs.push_back( std::move( attr ) );
    }
  }
  // Write characters of the value of an attribute which may be written as they are. But a tab or an end of line would be
  //  a space to whoever reads it ( 3.3.3 ): those are written as references to them so that the value which is read is
  //  the value which we were given.
  template < class t_TyCharValue >
  void _WriteAttrValueRun( const t_TyCharValue * _pcBegin, const t_TyCharValue * const _pcEnd )
  {
    static constexpr _TyChar s_kszTab[] = { _TyChar('9'), 0 };
    static constexpr _TyChar s_kszLineFeed[] = { _TyChar('1'), _TyChar('0'), 0 };
    static constexpr _TyChar s_kszCarriageReturn[] = { _TyChar('1'), _TyChar('3'), 0 };
    const t_TyCharValue * pcRun = _pcBegin;
    for ( const t_TyCharValue * pcCur = _pcBegin; _pcEnd != pcCur; ++pcCur )
    {
      const _TyChar * pszCharRef;
      switch( *pcCur )
      {
        case t_TyCharValue(0x9):
          pszCharRef = s_kszTab;
        break;
        case t_TyCharValue(0xA):
          pszCharRef = s_kszLineFeed;
        break;
        case t_TyCharValue(0xD):
          pszCharRef = s_kszCarriageReturn;
        break;
        default:
          continue;
      }
      _WriteTransportRaw( pcRun, pcCur );
      pcRun = pcCur + 1;
      _WriteTransportRaw( _TyMarkupTraits::s_kszCharDecRefBegin, StaticStringLen( _TyMarkupTraits::s_kszCharDecRefBegin ) );
      _WriteTransportRaw( pszCharRef, StrNLen( pszCharRef ) );
      _WriteTransportRaw( _TyMarkupTraits::s_kszReferenceEnd, StaticStringLen( _TyMarkupTraits::s_kszReferenceEnd ) );
    }
    _WriteTransportRaw( pcRun, _pcEnd );
  }
  // Write characters of character data which may be written as they are. But a #xD would be read as the end of a line -
  //  as #xA ( 2.11 ): it is written as a reference to it so that what is read is what we were given.
  template < class t_TyCharValue >
  void _WriteCharDataRun( const t_TyCharValue * _pcBegin, const t_TyCharValue * const _pcEnd )
  {
    static constexpr _TyChar s_kszCarriageReturn[] = { _TyChar('1'), _TyChar('3'), 0 };
    const t_TyCharValue * pcRun = _pcBegin;
    for ( const t_TyCharValue * pcCur = std::find( _pcBegin, _pcEnd, t_TyCharValue( 0xD ) ); _pcEnd != pcCur; pcCur = std::find( pcCur, _pcEnd, t_TyCharValue( 0xD ) ) )
    {
      _WriteTransportRaw( pcRun, pcCur );
      pcRun = ++pcCur;
      _WriteTransportRaw( _TyMarkupTraits::s_kszCharDecRefBegin, StaticStringLen( _TyMarkupTraits::s_kszCharDecRefBegin ) );
      _WriteTransportRaw( s_kszCarriageReturn, StrNLen( s_kszCarriageReturn ) );
      _WriteTransportRaw( _TyMarkupTraits::s_kszReferenceEnd, StaticStringLen( _TyMarkupTraits::s_kszReferenceEnd ) );
    }
    _WriteTransportRaw( pcRun, _pcEnd );
  }
  // Write the value of an attribute which the DTD gave the tag - see xml_attr_default_value.
  // It is the value itself: the references of the default were replaced and it was found to be well formed when the DTD
  //  was read. So there is nothing to validate and no reference to detect: every character which can't be written as
  //  itself between the quotes is written as a reference to it. Those are the markup characters, the quote in use, and
  //  the tab and the ends of lines - which would be spaces to whoever reads them as themselves ( 3.3.3 ).
  template < class t_TyCharValue >
  void _WriteAttrDefaultValue( basic_string_view< t_TyCharValue > _svValue, bool _fUseDoubleQuote )
  {
    static constexpr _TyChar s_kszTab[] = { _TyChar('9'), 0 };
    static constexpr _TyChar s_kszLineFeed[] = { _TyChar('1'), _TyChar('0'), 0 };
    static constexpr _TyChar s_kszCarriageReturn[] = { _TyChar('1'), _TyChar('3'), 0 };
    const t_TyCharValue * pcRun = _svValue.data(); // The characters which are written as themselves, so far.
    const t_TyCharValue * const pcEnd = pcRun + _svValue.length();
    for ( const t_TyCharValue * pcCur = pcRun; pcEnd != pcCur; ++pcCur )
    {
      const _TyChar * pszReference = nullptr;
      const _TyChar * pszCharRef = nullptr;
      switch( *pcCur )
      {
        case t_TyCharValue('&'):
          pszReference = _TyMarkupTraits::s_kszEntityAmpersand;
        break;
        case t_TyCharValue('<'):
          pszReference = _TyMarkupTraits::s_kszEntityLessThan;
        break;
        case t_TyCharValue('"'):
          if ( _fUseDoubleQuote )
            pszReference = _TyMarkupTraits::s_kszEntityDoubleQuote;
        break;
        case t_TyCharValue('\''):
          if ( !_fUseDoubleQuote )
            pszReference = _TyMarkupTraits::s_kszEntityApostrophe;
        break;
        case t_TyCharValue(0x9):
          pszCharRef = s_kszTab;
        break;
        case t_TyCharValue(0xA):
          pszCharRef = s_kszLineFeed;
        break;
        case t_TyCharValue(0xD):
          pszCharRef = s_kszCarriageReturn;
        break;
        default:
        break;
      }
      if ( !pszReference && !pszCharRef )
        continue;
      _WriteTransportRaw( pcRun, pcCur );
      pcRun = pcCur + 1;
      if ( pszReference )
        _WriteTransportRaw( pszReference, StrNLen( pszReference ) );
      else
      {
        _WriteTransportRaw( _TyMarkupTraits::s_kszCharDecRefBegin, StaticStringLen( _TyMarkupTraits::s_kszCharDecRefBegin ) );
        _WriteTransportRaw( pszCharRef, StrNLen( pszCharRef ) );
        _WriteTransportRaw( _TyMarkupTraits::s_kszReferenceEnd, StaticStringLen( _TyMarkupTraits::s_kszReferenceEnd ) );
      }
    }
    _WriteTransportRaw( pcRun, pcEnd );
  }
  // Write the name - it will have already been validated.
  // _rvalRgName is a value array containing at least (name,namespacewrap).
  // It may contains any number of things beyond that and they don't figure into this method.
  template < class t_TyXmlToken >
  void _WriteName( t_TyXmlToken const & _rtok, typename t_TyXmlToken::_TyLexValue const & _rvalRgName )
  {
    typedef t_TyXmlToken _TyXmlToken;
    typedef typename _TyXmlToken::_TyLexValue _TyLexValue;
    typedef typename _TyXmlToken::_TyChar _TyChar;
    typedef xml_namespace_value_wrap< _TyChar > _TyXmlNamespaceValueWrap;
    typedef basic_string_view< _TyChar > _TyStrView;
    const _TyLexValue & rvalName = _rvalRgName[vknNameIdx];
    const _TyLexValue & rvalNS = _rvalRgName[vknNamespaceIdx];
    if ( !rvalNS.FIsBool() && !m_xdcxtDocumentContext.FIncludePrefixesInAttrNames() )
    {
      const _TyXmlNamespaceValueWrap & rxnvw = rvalNS.template GetVal< _TyXmlNamespaceValueWrap >();
      if ( !rxnvw.FIsAttributeNamespaceDeclaration() ) // attribute namespace declarations always include the prefix.
      {
        _TyStrView svPrefix = rxnvw.RStringPrefix();
        if ( !svPrefix.empty() )
        {
          _WriteTransportRaw( &svPrefix[0], svPrefix.length() );
          _WriteTransportRaw( _TyMarkupTraits::s_kszTagColon, StaticStringLen( _TyMarkupTraits::s_kszTagColon ) );
        }
      }
    }
    // Now write the name.
    if ( !rvalName.FHasTypedData() )
    {
      rvalName.ApplyString( 
        [this]< typename t_TyChar >( const t_TyChar * _pcBegin, const t_TyChar * const _pcEnd )
        {
          _WriteTransportRaw( _pcBegin, _pcEnd );
        }
      );
    }
    else
      _WriteTypedData( _rtok, rvalName );
  }
  // Write the XMLDecl object that is contained in m_xdcxtDocumentContext.
  // We always validate the encoding because its origin is unknown at this point.
  void _WriteXmlDecl()
  {
    _TyXMLDeclProperties const & rxdp = m_xdcxtDocumentContext.GetXMLDeclProperties();
    _WriteTransportRaw( _TyMarkupTraits::s_kszXMLDeclBegin, StaticStringLen( _TyMarkupTraits::s_kszXMLDeclBegin ) );
    _WriteTransportRaw( rxdp.m_fVersionDoubleQuote ? _TyMarkupTraits::s_kszXMLDeclVersionDoubleQuote : _TyMarkupTraits::s_kszXMLDeclVersionSingleQuote, 
        StaticStringLen( _TyMarkupTraits::s_kszXMLDeclVersionDoubleQuote ) );
    if ( rxdp.m_fHasEncoding )
    {
      _WriteTransportRaw( _TyMarkupTraits::s_kszXMLDeclEncodingEquals, StaticStringLen( _TyMarkupTraits::s_kszXMLDeclEncodingEquals ) );
      _WriteTransportRaw( rxdp.m_fEncodingDoubleQuote ? _TyMarkupTraits::s_kszDoubleQuote : _TyMarkupTraits::s_kszSingleQuote, StaticStringLen( _TyMarkupTraits::s_kszDoubleQuote ) );
      if ( rxdp.m_strEncoding.length() )
      {
        const _TyChar * pcBeg = &rxdp.m_strEncoding[0];
        const _TyChar * pcEnd = pcBeg + rxdp.m_strEncoding.length();
        VerifyThrowSz( pcEnd == _l_match< _TyChar >::PszMatch( PspGetEncNameStart< _TyChar >(), pcBeg, rxdp.m_strEncoding.length() ), 
          "Invalid encoding name in XML declaration." );
        _WriteTransportRaw( pcBeg, rxdp.m_strEncoding.length() );
      }
      _WriteTransportRaw( rxdp.m_fEncodingDoubleQuote ? _TyMarkupTraits::s_kszDoubleQuote : _TyMarkupTraits::s_kszSingleQuote, StaticStringLen( _TyMarkupTraits::s_kszDoubleQuote ) );
    }
    if ( rxdp.m_fHasStandalone )
    {
      _WriteTransportRaw( _TyMarkupTraits::s_kszXMLDeclStandaloneEquals, StaticStringLen( _TyMarkupTraits::s_kszXMLDeclStandaloneEquals ) );
      _WriteTransportRaw( rxdp.m_fStandaloneDoubleQuote ? _TyMarkupTraits::s_kszDoubleQuote : _TyMarkupTraits::s_kszSingleQuote, StaticStringLen( _TyMarkupTraits::s_kszDoubleQuote ) );
      if ( rxdp.m_fStandalone )
        _WriteTransportRaw( _TyMarkupTraits::s_kszXMLDeclYes, StaticStringLen( _TyMarkupTraits::s_kszXMLDeclYes ) );
      else
        _WriteTransportRaw( _TyMarkupTraits::s_kszXMLDeclNo, StaticStringLen( _TyMarkupTraits::s_kszXMLDeclNo ) );
      _WriteTransportRaw( rxdp.m_fStandaloneDoubleQuote ? _TyMarkupTraits::s_kszDoubleQuote : _TyMarkupTraits::s_kszSingleQuote, StaticStringLen( _TyMarkupTraits::s_kszDoubleQuote ) );
    }
    _WriteTransportRaw( _TyMarkupTraits::s_kszXMLDeclEnd, StaticStringLen( _TyMarkupTraits::s_kszXMLDeclEnd ) );
  }
  template < class t_TyXmlToken >
  void _WriteComment( t_TyXmlToken const & _rtok )
  {
    typedef typename t_TyXmlToken::_TyLexValue _TyLexValue;
    _CheckCommitCur();
    _FormatBeforeContentMarkup();
    _WriteTransportRaw( _TyMarkupTraits::s_kszCommentBegin, StaticStringLen( _TyMarkupTraits::s_kszCommentBegin ) );
    // Scenarios:
    // 1) We do everything in the character type of t_TyXmlToken. There is no reason to convert because we can just convert on output and that doesn't require a buffer.
    // 2) Validate if necessary and then write.
    // 3) If we came from an xml_read_cursor: We will have positions and not a string in the _l_value object. In this case validation is not necessary as it was validated on input.
    // For a comment we should see a single, non-empty, _l_value element. If it is empty we will throw because that is not a valid comment.
    const _TyLexValue & rval = _rtok.GetValue();
    // We should see either a single data range here or some type of string.
    VerifyThrow( rval.FHasTypedData() || rval.FIsString() );
    if ( !rval.FHasTypedData() ) // We assume that if we have data positions then it came from an xml_read_cursor and thereby doesn't need validation.
    {
      // Then we need to validate the string in whatever character type it is in - don't matter none.
      rval.ApplyString( 
        [this]< typename t_TyChar >( const t_TyChar * _pcBegin, const t_TyChar * const _pcEnd )
        {
          _WriteCommentString( _pcBegin, _pcEnd );
        }
      );
    }
    else
    {
      _WriteTypedData( _rtok, rval );
    }
    _WriteTransportRaw( _TyMarkupTraits::s_kszCommentEnd, StaticStringLen( _TyMarkupTraits::s_kszCommentEnd ) );
    _FormatAfterTopLevel();
  }
  // The content of a general entity - see s_knTokenEntityContentBegin: the tokens between these two are what the entity
  //  of the document which was read has.
  // If we have written the document type declaration - which declares the entity - we write the reference to the entity
  //  as the document had it and nothing of what is between the two tokens: all goes on as it would - the tags are
  //  started and ended - and nothing of it is written. If we haven't then what the entity has is the content which is
  //  written, since whoever reads what we write doesn't know the entity.
  template < class t_TyXmlToken >
  void _WriteEntityContentBegin( t_TyXmlToken const & _rtok )
  {
    if ( !m_fWroteDoctypeDecl )
      return;
    _CheckCommitCur(); // The tag which the reference is within is written - all of it - before nothing more is.
    if ( !m_nWithinEntityReference )
    {
      _WriteTransportRaw( _TyMarkupTraits::s_kszEntityRefRefBegin, StaticStringLen( _TyMarkupTraits::s_kszEntityRefRefBegin ) );
      VerifyThrow( _rtok.GetValue().FIsString() );
      _rtok.GetValue().ApplyString(
        [this]< typename t_TyChar >( const t_TyChar * _pcBegin, const t_TyChar * const _pcEnd )
        {
          _WriteTransportRaw( _pcBegin, _pcEnd );
        }
      );
      _WriteTransportRaw( _TyMarkupTraits::s_kszReferenceEnd, StaticStringLen( _TyMarkupTraits::s_kszReferenceEnd ) );
      _MarkMixed();
    }
    ++m_nWithinEntityReference;
  }
  void _WriteEntityContentEnd()
  {
    if ( !!m_nWithinEntityReference )
      --m_nWithinEntityReference;
  }
  // Write the document type declaration: the token's value is its text from "<!DOCTYPE" through its '>' - see
  //  s_knTokenDoctypeDecl. It may be written once and only in the prolog.
  // The text is written as it is - once it is known to be a document type declaration: it is parsed as that of a
  //  document is - see xml_dtd_parser<>::CheckDoctypeDeclText(). ( Also that of a token which a read cursor made: its
  //  value is a string as any other and may be anyone's by now. )
  template < class t_TyXmlToken >
  void _WriteDoctypeDecl( t_TyXmlToken const & _rtok )
  {
    typedef typename t_TyXmlToken::_TyLexValue _TyLexValue;
    if ( !m_fWriteDoctypeDecl )
      return;
    if ( m_optstrDoctypeDeclToWrite.has_value() ) // See SetDoctypeDeclToWrite().
    {
      _WriteDoctypeDeclText( basic_string_view< _TyChar >( *m_optstrDoctypeDeclToWrite ), &m_fnValidatorOfDoctypeDeclToWrite );
      return;
    }
    VerifyThrowSz( FInProlog(), "The document type declaration may only be written before the first tag." );
    VerifyThrowSz( !m_fWroteDoctypeDecl, "A document type declaration has already been written - there may be only one." );
    const _TyLexValue & rval = _rtok.GetValue();
    VerifyThrow( rval.FHasTypedData() || rval.FIsString() );
    if ( rval.FHasTypedData() )
      _WriteTypedData( _rtok, rval );
    else
    {
      rval.ApplyString(
        [this]< typename t_TyChar >( const t_TyChar * _pcBegin, const t_TyChar * const _pcEnd )
        {
          // The parser is that of the type of character of this size.
          typedef conditional_t< sizeof( t_TyChar ) == sizeof( char8_t ), char8_t, conditional_t< sizeof( t_TyChar ) == sizeof( char16_t ), char16_t, char32_t > > _TyCharDtd;
          xml_dtd< _TyCharDtd > dtd;
          xml_dtd_parser< _TyCharDtd >::CheckDoctypeDeclText( basic_string_view< _TyCharDtd >( (const _TyCharDtd *)_pcBegin, size_t( _pcEnd - _pcBegin ) ), dtd );
          _WriteTransportRaw( _pcBegin, _pcEnd );
          // The DTD of the cursor which the token is from - or else that of the text: see SetValidate().
          if ( m_fValidate )
            _SetValidator( !!m_fnValidatorOfDoctypeToken ? m_fnValidatorOfDoctypeToken() : _FnValidatorOfDtd( make_shared< const xml_dtd< _TyCharDtd > >( std::move( dtd ) ) )() );
        }
      );
    }
    m_fWroteDoctypeDecl = true;
    _FormatAfterTopLevel();
    m_setDeclaredEntities.clear(); // Those of the document which it is of - see _ImportEntitiesOfSource() - and perhaps more.
    m_fDeclaredEntitiesComplete = false;
  }
  template < template < class t_TyChar > class t_tempGetStartToken, class t_TyXmlToken >
  void _WritePIPortion( t_TyXmlToken const & _rtok, typename t_TyXmlToken::_TyLexValue const & _rval )
  {
    VerifyThrow( _rval.FHasTypedData() || _rval.FIsString() );
    if ( _rval.FHasTypedData() )
      _WriteTypedData( _rtok, _rval );
    else
    {      
      _rval.ApplyString(
        [this]< typename t_TyChar >(const t_TyChar * _pcBegin, const t_TyChar* const _pcEnd)
        {
          _WritePIString< t_tempGetStartToken >( _pcBegin, _pcEnd );
        }
      );
    }
  }
  template < class t_TyXmlToken >
  void _WriteProcessingInstruction( t_TyXmlToken const & _rtok )
  {
    typedef typename t_TyXmlToken::_TyLexValue _TyLexValue;
    _CheckCommitCur();
    _FormatBeforeContentMarkup();
    _WriteTransportRaw( _TyMarkupTraits::s_kszProcessingInstructionBegin, StaticStringLen( _TyMarkupTraits::s_kszProcessingInstructionBegin ) );
    const _TyLexValue & rval = _rtok.GetValue();
    VerifyThrow( rval.FIsArray() );
    // We may have just a PITarget or a PITarget + PITargetMeat.
    _WritePIPortion< TGetPITargetStart >( _rtok, rval[vknProcessingInstruction_PITargetIdx] );
    if ( ( rval.GetSize() > 1 ) && !rval[vknProcessingInstruction_MeatIdx].FEmptyTypedData() )
    {
      _WriteTransportRaw( _TyMarkupTraits::s_kszSpace, StaticStringLen( _TyMarkupTraits::s_kszSpace ) );
      _WritePIPortion< TGetPITargetMeatStart >( _rtok, rval[vknProcessingInstruction_MeatIdx] );
    }
    _WriteTransportRaw( _TyMarkupTraits::s_kszProcessingInstructionEnd, StaticStringLen( _TyMarkupTraits::s_kszProcessingInstructionEnd ) );
    _FormatAfterTopLevel();
  }
  template < class t_TyXmlToken >
  void _WriteCDataSection( t_TyXmlToken const & _rtok )
  {
    typedef typename t_TyXmlToken::_TyLexValue _TyLexValue;
    _CheckCommitCur();
    _WriteTransportRaw( _TyMarkupTraits::s_kszCDataSectionBegin, StaticStringLen( _TyMarkupTraits::s_kszCDataSectionBegin ) );
    // If we find illegal characters within a CDATA section then there is no remedy. (Well we could end the CDataSection and put in a CharRef, but we aren't doing that now.)
    // That is what is done for a #xD - which is not illegal, and which would be read as the end of a line: see _WriteCDataRun().
    // If we find a "]]>" embedded in the CDataSection then we will appropriately escape it with an overlapping CDataSection. I.e.: "<![CDATA[]]]]><![CDATA[>]]>".
    const _TyLexValue & rval = _rtok.GetValue();
    // We should see either a single data range here or some type of string.
    VerifyThrow( rval.FHasTypedData() || rval.FIsString() );
    if ( !rval.FHasTypedData() ) // We assume that if we have data positions then it came from an xml_read_cursor and thereby doesn't need validation.
    {
      // Then we need to validate the string in whatever character type it is in - don't matter none.
      rval.ApplyString( 
        [this]< typename t_TyChar >( const t_TyChar * _pcBegin, const t_TyChar * const _pcEnd )
        {
          _WriteCDataString( _pcBegin, _pcEnd );
        }
      );
    }
    else
    {
      // The text of the document as it is - but for text whose #xD's are characters of it: see s_kdtfLiteralLineEnds.
      typedef _l_data<> _TyData;
      _TyData const & krdt = rval.GetTypedData();
      if ( !krdt.FIsNull() && krdt.FContainsSingleDataRange() && !!( s_kdtfLiteralLineEnds & krdt.DataRangeGetSingle().type() ) )
      {
        basic_string_view< typename t_TyXmlToken::_TyChar > sv;
        _rtok.GetLexToken().KGetStringView( sv, krdt.DataRangeGetSingle().GetRangeBase() );
        _WriteCDataRun( sv.data(), sv.data() + sv.length() );
      }
      else
        _WriteTypedData( _rtok, rval );
    }
    _WriteTransportRaw( _TyMarkupTraits::s_kszCDataSectionEnd, StaticStringLen( _TyMarkupTraits::s_kszCDataSectionEnd ) );
    _MarkMixed();
  }
  // Write characters of a CDATA section which may be written as they are. But a #xD would be read as the end of a
  //  line - as #xA ( 2.11 ) - and within the section there is no writing a reference to it: the section is ended, the
  //  reference is written and the section is begun again. What is read is then what we were given - as a section,
  //  character data which is the #xD, and a section.
  template < class t_TyCharValue >
  void _WriteCDataRun( const t_TyCharValue * _pcBegin, const t_TyCharValue * const _pcEnd )
  {
    static constexpr _TyChar s_kszCarriageReturn[] = { _TyChar('1'), _TyChar('3'), 0 };
    const t_TyCharValue * pcRun = _pcBegin;
    for ( const t_TyCharValue * pcCur = std::find( _pcBegin, _pcEnd, t_TyCharValue( 0xD ) ); _pcEnd != pcCur; pcCur = std::find( pcCur, _pcEnd, t_TyCharValue( 0xD ) ) )
    {
      _WriteTransportRaw( pcRun, pcCur );
      pcRun = ++pcCur;
      _WriteTransportRaw( _TyMarkupTraits::s_kszCDataSectionEnd, StaticStringLen( _TyMarkupTraits::s_kszCDataSectionEnd ) );
      _WriteTransportRaw( _TyMarkupTraits::s_kszCharDecRefBegin, StaticStringLen( _TyMarkupTraits::s_kszCharDecRefBegin ) );
      _WriteTransportRaw( s_kszCarriageReturn, StrNLen( s_kszCarriageReturn ) );
      _WriteTransportRaw( _TyMarkupTraits::s_kszReferenceEnd, StaticStringLen( _TyMarkupTraits::s_kszReferenceEnd ) );
      _WriteTransportRaw( _TyMarkupTraits::s_kszCDataSectionBegin, StaticStringLen( _TyMarkupTraits::s_kszCDataSectionBegin ) );
    }
    _WriteTransportRaw( pcRun, _pcEnd );
  }
  // Write a string which is character data or the value of an attribute - validating it and escaping what must be: see
  //  _WriteCharAndAttrData() below for what is done and for _edrDetectReferences, which may not be edrDetectReferencesCount here.
  template < template < class > class t_tempGetStartToken, class t_TyCharApplyString >
  void _WriteCharAndAttrString( const t_TyCharApplyString * _pcBegin, const t_TyCharApplyString * const _pcEnd, EDetectReferences _edrDetectReferences )
  {
    typedef _l_state_proto< t_TyCharApplyString > _TyStateProto;
    const _TyStateProto * const kpspValidateTokenStart = t_tempGetStartToken< t_TyCharApplyString >::s_kpspStart;
    const bool kfIsCharData = ( kpspValidateTokenStart == PspGetCharDataStart< t_TyCharApplyString >() );
    if ( kfIsCharData && !FInsideDocumentTag() )
    {
      // CharData before the first tag - and after the last - should match the S production.
      const t_TyCharApplyString * pcMatchSpace = _l_match< t_TyCharApplyString >::PszMatch( PspGetSpacesStart< t_TyCharApplyString >(), _pcBegin, _pcEnd - _pcBegin );
      VerifyThrowSz( pcMatchSpace == _pcEnd, "CharData outside of the document's tag must be composed entirely of spaces." );
      _WriteTransportRaw( _pcBegin, _pcEnd );
      return;
    }
    Assert( m_fWroteFirstTag || !!m_nDryRun ); // The value of an attribute is written when its tag is - see _CommitTag().
    const _TyStateProto * const kpspAllReferences = PspGetAllReferencesStart< t_TyCharApplyString >();
    // Move through and validate as we have described above: each time around we write the characters which may be
    //  written as they are and then deal with the one at which they end.
    for ( const t_TyCharApplyString * pcCur = _pcBegin; _pcEnd != pcCur;  )
    {
      const _TyStateProto * pspAccept;
      const t_TyCharApplyString * pcMatch = _l_match< t_TyCharApplyString >::PszMatch( kpspValidateTokenStart, pcCur, _pcEnd - pcCur, &pspAccept );
      // Since we are using an anti-accepting state in the CharData production we need to examine the returned state for anti-accepting:
      //  that means we have encountered an embedded "]]>" - pcMatch is just beyond it - and we can appropriately escape.
      if ( !!pspAccept && pspAccept->FIsAntiAcceptingState() )
      {
        Assert( kfIsCharData && ( pcMatch - pcCur >= 3 ) );
        Assert( ( pcMatch[-3] == t_TyCharApplyString(']') ) && ( pcMatch[-2] == t_TyCharApplyString(']') ) && ( pcMatch[-1] == t_TyCharApplyString('>') ) );
        _WriteCharDataRun( pcCur, pcMatch-1 );
        _WriteTransportRaw( _TyMarkupTraits::s_kszEntityGreaterThan, StaticStringLen( _TyMarkupTraits::s_kszEntityGreaterThan ) );
        pcCur = pcMatch; // The '>' is written.
        continue;
      }
      // Otherwise the characters up to pcMatch may be written as they are - which is the most common thing to happen - and
      //  they may be all that there is. If no state accepted then there are none of them: we are at a character which may not be.
      if ( !pspAccept )
        pcMatch = pcCur;
      if ( kfIsCharData )
        _WriteCharDataRun( pcCur, pcMatch );
      else
        _WriteAttrValueRun( pcCur, pcMatch );
      pcCur = pcMatch;
      if ( _pcEnd == pcCur )
        break;
      // In this case we should be pointing at a character which can't be written as itself:
      // 1) Might be at the start of a reference.
      // 2) Might be at a '<', or at the quote which is in use for the value of an attribute.
      switch( *pcCur++ )
      {
        default:
          VerifyThrowSz( false, "A character which may not be written was found in character data or in the value of an attribute." );
        break;
        case t_TyCharApplyString('<'):
          _WriteTransportRaw( _TyMarkupTraits::s_kszEntityLessThan, StaticStringLen( _TyMarkupTraits::s_kszEntityLessThan ) );
        break;
        case t_TyCharApplyString('\''):
          _WriteTransportRaw( _TyMarkupTraits::s_kszEntityApostrophe, StaticStringLen( _TyMarkupTraits::s_kszEntityApostrophe ) );
        break;
        case t_TyCharApplyString('\"'):
          _WriteTransportRaw( _TyMarkupTraits::s_kszEntityDoubleQuote, StaticStringLen( _TyMarkupTraits::s_kszEntityDoubleQuote ) );
        break;
        case t_TyCharApplyString('&'):
        {
          // Reference. _edrDetectReferences now comes into play:
          if ( edrNoReferences == _edrDetectReferences )
          {
            _WriteTransportRaw( _TyMarkupTraits::s_kszEntityAmpersand, StaticStringLen( _TyMarkupTraits::s_kszEntityAmpersand ) );
            break;
          }
          // We might have a reference (at least) in this case - figure out what we have:
          const _TyStateProto * pspReference;
          const t_TyCharApplyString * pcMatchReference = _l_match< t_TyCharApplyString >::PszMatch( kpspAllReferences, pcCur-1, _pcEnd - ( pcCur - 1 ), &pspReference );
          const bool kfIsReference = !!pspReference && !pspReference->FIsAntiAcceptingState();
          VerifyThrowSz( kfIsReference || ( edrAlwaysReference != _edrDetectReferences ), "Found a & that didn't match the Reference production." );
          if ( !kfIsReference )
          {
            Assert( ( edrAutoReference == _edrDetectReferences ) || ( edrAutoReferenceNoError == _edrDetectReferences ) );
            // Then this doesn't match the Reference production and we are on "auto" - just replace with an amp:
            _WriteTransportRaw( _TyMarkupTraits::s_kszEntityAmpersand, StaticStringLen( _TyMarkupTraits::s_kszEntityAmpersand ) );
            break;
          }
          // It reads as a reference: it is written as one - as it is - if it is good, and otherwise ( unless that
          //  is an error ) its '&' is escaped so that what is written is the characters which we were given.
          typedef basic_string_view< t_TyCharApplyString > _TyStrViewApply;
          typedef basic_string_view< _TyChar > _TyStrViewOutput;
          bool fGoodReference;
          if ( s_knTokenEntityRef == pspReference->m_tidAccept )
          {
            // Must be a reference to an entity which is known:
            _TyStrViewApply svRef( pcCur, ( pcMatchReference - 1 ) - pcCur );
            _TyStrViewOutput svEntity = m_xdcxtDocumentContext.GetUserObj().SvLookupEntity( svRef );
            fGoodReference = !svEntity.empty();
            VerifyThrowSz( fGoodReference || ( edrAutoReferenceNoError == _edrDetectReferences ), "Entity reference to [%s] not found.", StrConvertString<char>( svRef ).c_str() );
          }
          else
          {
            Assert( ( s_knTokenCharRefDec == pspReference->m_tidAccept ) || ( s_knTokenCharRefHex == pspReference->m_tidAccept ) );
            // The production ensures we have a valid set of characters, we must validate overflow - and that it is a character.
            // Character references are always in UTF32.
            const t_TyCharApplyString * pcDigits = pcCur + ( pspReference->m_tidAccept == s_knTokenCharRefDec ? 1 : 2 );
            _TyStrViewApply svRef( pcDigits, ( pcMatchReference - 1 ) - pcDigits );
            char32_t utf32 = 0;
            int iResult = IReadPositiveNum( s_knTokenCharRefDec == pspReference->m_tidAccept ? 10 : 16, &svRef[0], svRef.length(), utf32, _l_char_type_map<char32_t>::ms_kcMax, false );
            fGoodReference = !iResult && ( ( 0x9 == utf32 ) || ( 0xA == utf32 ) || ( 0xD == utf32 ) || ( ( utf32 >= 0x20 ) && ( utf32 <= 0xD7FF ) ) ||
                                            ( ( utf32 >= 0xE000 ) && ( utf32 <= 0xFFFD ) ) || ( ( utf32 >= 0x10000 ) && ( utf32 <= 0x10FFFF ) ) );
            VerifyThrowSz( fGoodReference || ( edrAutoReferenceNoError == _edrDetectReferences ),
              "The character reference [%s] is not to a character - very likely overflow.", StrConvertString<char>( svRef ).c_str() );
          }
          if ( !fGoodReference )
            _WriteTransportRaw( _TyMarkupTraits::s_kszEntityAmpersand, StaticStringLen( _TyMarkupTraits::s_kszEntityAmpersand ) ); // escape the bogus reference.
          else
            _WriteTransportRaw( pcCur - 1, pcCur ); // The '&' itself - and then the rest of it.
          _WriteTransportRaw( pcCur, pcMatchReference );
          pcCur = pcMatchReference;
        }
        break;
      }
    }
  }
  // Three possibilities:
  // 1) _rtok contains typed data. In this case we won't validate at all and _edrDetectReferences DOES NOT come into play.
  // 2) _rtok contains a single string. In this _edrDetectReferences is used to validate and segment the string, etc.
  //    During validation, in addition to the the functionality of _edrDetectReferences below, we will substitute:
  //    a) '<' is escaped with "&lt;".
  //    b) '"' is escapted with "&quot;".
  //    c) ''' is escapted with "&apos;".
  //    d) Also if an anti-accepting state if found then it indicates that we found "]]>" within CharData
  //       and we will escape the '>' with "&gt;".
  // 3) If we haven't written the first tag then the only possibility is that this is a CharData tag.
  //    In this case we must validate that the tag contains only spaces.
  // _edrDetectReferences:
  // edrNoReferences: then any embedded '&' is escaped with "&amp;"
  // edrAlwaysReference: then any embedded '&' *must* correspond to a valid Reference of some sort.
  // edrAutoReference/edrAutoReferenceNoError: try to detect if we should replace with "&amp;" or not:
  //  1) If we see what looks like a CharRef (which is pretty obvious) then we will validate it.
  //  2) If we see what looks like just an EntityRef then we check if it is in the entity ref map:
  //     a) If it isn't in the entity map and edrAutoReferenceNoError then we will replace the leading '&' with "&amp;"
  //     b) If it isn't in the enity map and edrAutoReference then we will throw an error for a missing reference.
  // _rval: This is the value that we are currently writing. This may be one of the attribute values of a tag or chardata.
  // t_tempGetStartToken: This is one of TGetCharDataStart<>, TGetAttCharDataNoSingleQuoteStart<>, TGetAttCharDataNoDoubleQuoteStart<>.
  template < template < class > class t_tempGetStartToken, class t_TyXmlToken >
  void _WriteCharAndAttrData( t_TyXmlToken const & _rtok, typename t_TyXmlToken::_TyLexValue const & _rval, EDetectReferences _edrDetectReferences = edrDetectReferencesCount )
  {
    typedef typename t_TyXmlToken::_TyLexValue _TyLexValue;
    if ( _edrDetectReferences == edrDetectReferencesCount )
      _edrDetectReferences = m_edrDefaultDetectReferences; // use default which the user can set.
    // We should see either a single data range here or some type of string.
    VerifyThrow( _rval.FHasTypedData() || _rval.FIsString() );
    if ( !_rval.FHasTypedData() ) // We assume that if we have data positions then it came from an xml_read_cursor and thereby doesn't need validation.
    {
      // Then we need to validate the string in whatever character type it is in - don't matter none.
      _rval.ApplyString( 
        [this,_edrDetectReferences]< typename t_TyCharApplyString >( const t_TyCharApplyString * _pcBegin, const t_TyCharApplyString * const _pcEnd )
        {
          _WriteCharAndAttrString< t_tempGetStartToken >( _pcBegin, _pcEnd, _edrDetectReferences );
        }
      );
    }
    else
    {
      // In this case we may have rich data in the typed data - i.e. a series of CharData and interspersed references.
      // We will peruse and write out according to the sub-tokens present in the typed data. We will have to augment with
      //  sub-token (trigger) markup as we go.
      typedef basic_string_view< typename t_TyXmlToken::_TyChar > _TyStrView;
      typedef _l_data<> _TyData;
      _TyData const & krdtCharData = _rval.GetTypedData();
      // The value of an attribute whose declared type is not CDATA - see s_kdtfAttrTokenized: it is without the spaces
      //  at its ends and has one space for a sequence of them because the DTD says so. If the document type declaration
      //  is not written then nothing will say so to whoever reads what we write: we write the value itself.
      if ( !m_fWroteDoctypeDecl && !krdtCharData.FIsNull() &&
            !!( s_kdtfAttrTokenized & ( krdtCharData.FContainsSingleDataRange() ? krdtCharData.DataRangeGetSingle().type() : krdtCharData.GetSegArrayDataRanges().ElGet( 0 ).type() ) ) )
      {
        basic_string< typename t_TyXmlToken::_TyChar > strValue;
        _rtok.GetLexToken().GetString( strValue, _rval );
        _WriteCharAndAttrString< t_tempGetStartToken >( strValue.data(), strValue.data() + strValue.length(), edrNoReferences );
        return;
      }
      if ( !krdtCharData.FIsNull() )
      {
        krdtCharData.Apply(
          [this,&_rtok]( const _l_data_typed_range * _pdtrBegin, const _l_data_typed_range * _pdtrEnd )
          {
            for ( const _l_data_typed_range * pdtrCur = _pdtrBegin; _pdtrEnd != pdtrCur; ++pdtrCur )
            {
              // We could use type() or id() here - but the type is more flexible.
              // What is written is what the document had: the white space of the value of an attribute as it was, and the
              //  flags of the type - which are about the string of the value - don't matter here.
              switch( DtGetType( pdtrCur->type() ) )
              {
                case s_kdtPlainText:
                case s_kdtAttrWhitespace:
                {
                  _TyStrView svPlain;
                  _rtok.GetLexToken().KGetStringView( svPlain, pdtrCur->GetRangeBase() );
                  // ( Text whose #xD's are characters of it and not the ends of its lines - see s_kdtfLiteralLineEnds:
                  //  written as they are they would be read as the ends of lines. )
                  if ( !!( s_kdtfLiteralLineEnds & pdtrCur->type() ) )
                    _WriteCharDataRun( svPlain.data(), svPlain.data() + svPlain.length() );
                  else
                    _WriteTransportRaw( &svPlain[0], svPlain.length() );
                }
                break;
                case s_kdtCharDecRef:
                {
                  _WriteTransportRaw( _TyMarkupTraits::s_kszCharDecRefBegin, StaticStringLen( _TyMarkupTraits::s_kszCharDecRefBegin ) );
                  _TyStrView svCharDecRef;
                  _rtok.GetLexToken().KGetStringView( svCharDecRef, pdtrCur->GetRangeBase() );
                  _WriteTransportRaw( &svCharDecRef[0], svCharDecRef.length() );
                  _WriteTransportRaw( _TyMarkupTraits::s_kszReferenceEnd, StaticStringLen( _TyMarkupTraits::s_kszReferenceEnd ) );
                }
                break;
                case s_kdtCharHexRef:
                {
                  _WriteTransportRaw( _TyMarkupTraits::s_kszCharHexRefBegin, StaticStringLen( _TyMarkupTraits::s_kszCharHexRefBegin ) );
                  _TyStrView svCharHexRef;
                  _rtok.GetLexToken().KGetStringView( svCharHexRef, pdtrCur->GetRangeBase() );
                  _WriteTransportRaw( &svCharHexRef[0], svCharHexRef.length() );
                  _WriteTransportRaw( _TyMarkupTraits::s_kszReferenceEnd, StaticStringLen( _TyMarkupTraits::s_kszReferenceEnd ) );
                }
                break;
                case s_kdtEntityRef:
                {
                  _TyStrView svEntityRef;
                  _rtok.GetLexToken().KGetStringView( svEntityRef, pdtrCur->GetRangeBase() );
                  // A reference is written as it is if whoever reads what we write knows the entity: it is one of ours -
                  //  the predefined ones are - or it is one which the document which was read declares and we have
                  //  written the document type declaration, where it does so. Otherwise that declaration is not
                  //  being written: we write the text which the entity stands for. A reference whose text isn't known
                  //  - see s_kdtfEntityRefNotIncluded - is written as it is: there is nothing else to write.
                  if ( !m_fWroteDoctypeDecl && !m_setEntitiesOfSource.empty() && !( s_kdtfEntityRefNotIncluded & pdtrCur->type() ) )
                  {
                    _TyStdStr strEntityName;
                    ConvertString( strEntityName, svEntityRef );
                    if ( m_setEntitiesOfSource.end() != m_setEntitiesOfSource.find( strEntityName ) )
                    {
                      // The text of the entity may be other here than in content - see s_kdtfEntityRefInAttrValue.
                      const _TyStdStr * pstrAttrValue = !( s_kdtfEntityRefInAttrValue & pdtrCur->type() ) ? nullptr :
                        m_xdcxtDocumentContext.GetUserObj().PStrLookupEntityAttrValue( strEntityName );
                      if ( !!pstrAttrValue )
                      {
                        _WriteCharAndAttrString< t_tempGetStartToken >( pstrAttrValue->data(), pstrAttrValue->data() + pstrAttrValue->length(), edrNoReferences );
                        break;
                      }
                      _TyEntityMap const & rmapEntities = m_xdcxtDocumentContext.GetUserObj().GetEntityMap();
                      typename _TyEntityMap::const_iterator citEntity = rmapEntities.find( strEntityName );
                      VerifyThrowSz( rmapEntities.end() != citEntity, "Entity reference to [%s] not found.", StrConvertString<char>( svEntityRef ).c_str() );
                      // The text is the characters themselves - its own references were replaced when the DTD was read.
                      _WriteCharAndAttrString< t_tempGetStartToken >( citEntity->second.data(), citEntity->second.data() + citEntity->second.length(), edrNoReferences );
                      break;
                    }
                  }
                  _WriteTransportRaw( _TyMarkupTraits::s_kszEntityRefRefBegin, StaticStringLen( _TyMarkupTraits::s_kszEntityRefRefBegin ) );
                  _WriteTransportRaw( &svEntityRef[0], svEntityRef.length() );
                  _WriteTransportRaw( _TyMarkupTraits::s_kszReferenceEnd, StaticStringLen( _TyMarkupTraits::s_kszReferenceEnd ) );
                }
                break;
                default:
                {
                  VerifyThrowSz( false, "Invalid data type()[%u]", pdtrCur->type() );
                }
              }
            }
          }
        );
      }
    }
  }
  typedef optional< _TyXmlTransportOut > _TyOptTransportOut;
  _TyOptTransportOut m_optTransportOut;
  _TyXmlDocumentContext m_xdcxtDocumentContext;
  string m_strFileName; // Save this since it is cheap - might be enpty.
  _TyListWriteContexts m_lContexts;
// options:
  EDetectReferences m_edrDefaultDetectReferences{edrAutoReferenceNoError};
  bool m_fWriteBOM{true}; // Write a BOM because that's just a nice thing to do.
  bool m_fWriteXMLDecl{true}; // Whether to write an XMLDecl. This is honored in all scenarios.
  bool m_fUseNamespaces{true}; // Using namespaces?
  bool m_fWriteDoctypeDecl{true}; // Write the document type declaration when its token is written.
  optional< _TyStdStr > m_optstrDoctypeDeclToWrite; // See SetDoctypeDeclToWrite().
  std::function< unique_ptr< _xml_write_validator_base< _TyChar > >() > m_fnValidatorOfDoctypeDeclToWrite; // Of its DTD - see SetValidate().
  bool m_fValidate{false}; // See SetValidate().
  bool m_fThrowOnValidityError{false};
  unique_ptr< _xml_write_validator_base< _TyChar > > m_upValidator; // Once the document type declaration - or the first tag - is written.
  size_t m_nValidatedDepth{0}; // The depth of the last tag of which the validator was told - see _ValidateStartTag().
  std::function< unique_ptr< _xml_write_validator_base< _TyChar > >() > m_fnValidatorOfDoctypeToken; // Of the DTD of the read cursor being written - see NWriteFromReadCursor().
  _TyStdStr m_strValidateName;
  vector< _TyStdStr > m_rgstrValidate; // The names and values of the attributes of a tag.
  vector< typename _xml_write_validator_base< _TyChar >::_TyAttr > m_rgValidateAttrs;
  optional< bool > m_optfWriteDefaultedAttributes; // Write the attributes which the DTD gave a tag - see SetWriteDefaultedAttributes().
// state:
  bool m_fWroteFirstTag{false};
  bool m_fWroteDoctypeDecl{false};
  bool m_fEndedDocument{false}; // See EndDocument().
  // The general entities which the document type declaration which was written declares - see WriteDoctypeDecl(): all of
  //  them, if m_fDeclaredEntitiesComplete - otherwise some may not be known to us.
  set< _TyStdStr, std::less<> > m_setDeclaredEntities;
  bool m_fDeclaredEntitiesComplete{false};
  size_t m_nWithinEntityReference{0};
  size_t m_nDryRun{0}; // Nothing is written - see _WriteChecked().
  // Pretty printing - see SetOutputFormat():
  optional< _xml_output_format > m_optxofFormat; // The format of what is written - unless OpenFile() or OpenMemFile() is given one.
  bool m_fPrettyPrint{false};
  _TyStdStr m_strIndent; // The indentation of a level.
  size_t m_nMaxIndentLevels{0}; // Deeper than this the indentation is that of this. // We are within what an entity has and its reference is what was written: nothing is.
  // The names of the entities which we have from the documents whose tokens we are given - see _ImportEntitiesOfSource().
  set< _TyStdStr, std::less<> > m_setEntitiesOfSource;
  const void * m_pvUserObjSource{nullptr}; // The user object from which we last had them.
  bool m_fHaveUnendedTag{false};
  _TyStdStr m_strUnendedTagSpace; // The white space which was before the end of that tag - see _CommitTag().
};

// xml_writer_var:
// Variant writer. This supports multiple character types (potentially). 
// Once again not sure that this is necessary, but potentially just as a receiver for xml_cursor_var - instant adaptor.
template <  template < class ... > class t_tempTyTransport, class t_TyTp2DCharPack >
class xml_writer_var
{
public:
};

__XMLP_END_NAMESPACE
