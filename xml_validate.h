#pragma once

//          Copyright David Lawrence Bien 1997 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt).

// xml_validate.h
// Validation of a document against its DTD: the validity constraints of the XML 1.0 specification.
// dbien
// 19NOV2026

// xml_validator is told of a document as it is read - the start of each tag with its attributes, what is within it and
//  its end - and checks it against an xml_dtd. A read cursor whose XML traits are validating
//  ( xml_traits< transport, true, true > ) has one and tells it: see xml_read_cursor<>::FIsValid().
// The validity constraints ( VC ) which are checked - the declarations themselves are checked when they are read, see
//  xml_dtd<>::m_rgstrValidityErrors:
//  - VC: Root Element Type [28]: the document's element is of the type which the document type declaration names.
//  - VC: Element Valid [39]: the element type is declared and what is within the element is as it is declared:
//    EMPTY: nothing at all. ANY: anything. Mixed [51]: character data and elements of the types listed.
//    children [47]: elements in the order which the content model says, with nothing but white space, comments and
//    processing instructions among them.
//  - VC: Attribute Value Type [41]: each attribute is declared and its value is of its declared type:
//    VC: ID [56] - a Name which no other ID in the document is. VC: IDREF - a Name ( or Names ) which is an ID of the
//    document. VC: Entity Name - the name ( or names ) of an unparsed entity which is declared. VC: Name Token - an
//    Nmtoken ( or Nmtokens ). VC: Notation Attributes [58] and VC: Enumeration [59] - one of those listed.
//  - VC: Required Attribute [60]: an attribute declared #REQUIRED is in the tag.
//  - VC: Fixed Attribute Default [60]: an attribute declared #FIXED has that value.
//  - VC: Standalone Document Declaration [32]: a document which is declared standalone is not changed by declarations
//    which are external - those of the external subset and of parameter entities: it has no attribute by a default
//    which is declared so, no white space within an element whose element content is declared so - and it doesn't
//    refer to an entity nor have an attribute whose value is normalized by a type which is declared so: the read
//    cursor finds those two, see StandaloneViolation().
// A violation is not an error of well-formedness: it is recorded - RgErrors() - and the document is read on, unless
//  errors are to be thrown.
// The DTD knows nothing of namespaces: names are compared as they are written, prefix and all, and an attribute which
//  declares a namespace must be declared as any other.

#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <vector>
#include "_strutil.h"
#include "xml_ns.h"
#include "xml_types.h"
#include "xml_exc.h"
#include "xml_dtd.h"

__XMLP_BEGIN_NAMESPACE

// _xml_content_model:
// The content model of an element type with element content [47] as an automaton: the positions of the names in the
//  model are its states ( Glushkov ). We are at a set of positions - the content models of XML are meant to be
//  deterministic, in which case the set has one member, but nothing here depends on that.
template < class t_TyChar >
class _xml_content_model
{
  typedef _xml_content_model _TyThis;
public:
  typedef basic_string< t_TyChar > _TyStdStr;
  typedef basic_string_view< t_TyChar > _TyStrView;
  typedef xml_dtd_content_particle< t_TyChar > _TyParticle;
  typedef uint32_t _TyPosition;
  typedef vector< _TyPosition > _TyRgPositions;

  explicit _xml_content_model( _TyParticle const & _rcp )
  {
    _TySets sets = _Compile( _rcp );
    m_fNullable = sets.m_fNullable;
    m_rgFirst = std::move( sets.m_rgFirst );
    m_rgfLast.assign( m_rgpstrNames.size(), false );
    for ( _TyPosition pos : sets.m_rgLast )
      m_rgfLast[pos] = true;
    for ( _TyRgPositions & rrgFollow : m_rgFollow )
      _Unique( rrgFollow );
  }
  // The element types which may be first - or which may follow the positions which we are at.
  // _prgPositions: the positions which we are at - null if at the start.
  // _svName: an element of this type is next: _rrgNext is the positions which we are then at - none if it may not be.
  void GetNext( const _TyRgPositions * _prgPositions, _TyStrView _svName, _TyRgPositions & _rrgNext ) const
  {
    _rrgNext.clear();
    auto lambdaAdd = [&]( _TyRgPositions const & _rrg )
    {
      for ( _TyPosition pos : _rrg )
        if ( _TyStrView( *m_rgpstrNames[pos] ) == _svName )
          _rrgNext.push_back( pos );
    };
    if ( !_prgPositions )
      lambdaAdd( m_rgFirst );
    else
      for ( _TyPosition pos : *_prgPositions )
        lambdaAdd( m_rgFollow[pos] );
    if ( _rrgNext.size() > 1 )
      _Unique( _rrgNext );
  }
  // Whether the content may end where we are.
  bool FMayEnd( const _TyRgPositions * _prgPositions ) const
  {
    if ( !_prgPositions )
      return m_fNullable;
    for ( _TyPosition pos : *_prgPositions )
      if ( m_rgfLast[pos] )
        return true;
    return false;
  }
  // The names of the element types which may be next - for a message.
  string StrExpected( const _TyRgPositions * _prgPositions ) const
  {
    set< _TyStrView > setNames;
    auto lambdaAdd = [&]( _TyRgPositions const & _rrg )
    {
      for ( _TyPosition pos : _rrg )
        setNames.insert( _TyStrView( *m_rgpstrNames[pos] ) );
    };
    if ( !_prgPositions )
      lambdaAdd( m_rgFirst );
    else
      for ( _TyPosition pos : *_prgPositions )
        lambdaAdd( m_rgFollow[pos] );
    string str;
    for ( _TyStrView svName : setNames )
      str += ( str.empty() ? "<" : ", <" ) + StrConvertString< char >( svName ) + ">";
    if ( FMayEnd( _prgPositions ) )
      str += str.empty() ? "the end of the element" : " or the end of the element";
    return str;
  }
protected:
  struct _TySets
  {
    bool m_fNullable{ false };
    _TyRgPositions m_rgFirst;
    _TyRgPositions m_rgLast;
  };
  static void _Unique( _TyRgPositions & _rrg )
  {
    std::sort( _rrg.begin(), _rrg.end() );
    _rrg.erase( std::unique( _rrg.begin(), _rrg.end() ), _rrg.end() );
  }
  void _AddFollow( _TyRgPositions const & _rrgFrom, _TyRgPositions const & _rrgTo )
  {
    for ( _TyPosition posFrom : _rrgFrom )
      m_rgFollow[posFrom].insert( m_rgFollow[posFrom].end(), _rrgTo.begin(), _rrgTo.end() );
  }
  _TySets _Compile( _TyParticle const & _rcp )
  {
    _TySets sets;
    switch( _rcp.m_eParticle )
    {
      case exdpName:
      {
        const _TyPosition pos = _TyPosition( m_rgpstrNames.size() );
        m_rgpstrNames.push_back( &_rcp.m_strName );
        m_rgFollow.emplace_back();
        sets.m_rgFirst.push_back( pos );
        sets.m_rgLast.push_back( pos );
      }
      break;
      case exdpChoice:
      {
        for ( _TyParticle const & rcpChild : _rcp.m_rgChildren )
        {
          _TySets setsChild = _Compile( rcpChild );
          sets.m_fNullable = sets.m_fNullable || setsChild.m_fNullable;
          sets.m_rgFirst.insert( sets.m_rgFirst.end(), setsChild.m_rgFirst.begin(), setsChild.m_rgFirst.end() );
          sets.m_rgLast.insert( sets.m_rgLast.end(), setsChild.m_rgLast.begin(), setsChild.m_rgLast.end() );
        }
      }
      break;
      case exdpSeq:
      {
        sets.m_fNullable = true;
        for ( _TyParticle const & rcpChild : _rcp.m_rgChildren )
        {
          _TySets setsChild = _Compile( rcpChild );
          // What may be last so far may be followed by what may be first in this one.
          _AddFollow( sets.m_rgLast, setsChild.m_rgFirst );
          if ( sets.m_fNullable )
            sets.m_rgFirst.insert( sets.m_rgFirst.end(), setsChild.m_rgFirst.begin(), setsChild.m_rgFirst.end() );
          if ( setsChild.m_fNullable )
            sets.m_rgLast.insert( sets.m_rgLast.end(), setsChild.m_rgLast.begin(), setsChild.m_rgLast.end() );
          else
            sets.m_rgLast = std::move( setsChild.m_rgLast );
          sets.m_fNullable = sets.m_fNullable && setsChild.m_fNullable;
        }
      }
      break;
      default:
        Assert( false );
      break;
    }
    if ( ( exdoZeroOrMore == _rcp.m_eOccurrence ) || ( exdoOneOrMore == _rcp.m_eOccurrence ) )
      _AddFollow( sets.m_rgLast, sets.m_rgFirst ); // Again.
    if ( ( exdoZeroOrMore == _rcp.m_eOccurrence ) || ( exdoOptional == _rcp.m_eOccurrence ) )
      sets.m_fNullable = true;
    return sets;
  }
  vector< const _TyStdStr * > m_rgpstrNames; // The element type of each position: the names are those of the DTD.
  vector< _TyRgPositions > m_rgFollow; // The positions which may follow each position.
  _TyRgPositions m_rgFirst; // The positions which may be first.
  vector< bool > m_rgfLast; // Whether each position may be last.
  bool m_fNullable{ false }; // The content may be nothing.
};

// An attribute of a tag as the validator is given it: its name as it is written and its value - normalized.
template < class t_TyChar >
struct xml_validate_attr
{
  basic_string_view< t_TyChar > m_svName;
  basic_string_view< t_TyChar > m_svValue;
};

template < class t_TyChar >
class xml_validator
{
  typedef xml_validator _TyThis;
public:
  typedef t_TyChar _TyChar;
  typedef basic_string< _TyChar > _TyStdStr;
  typedef basic_string_view< _TyChar > _TyStrView;
  typedef xml_dtd< _TyChar > _TyDtd;
  typedef xml_dtd_element< _TyChar > _TyElement;
  typedef xml_dtd_attdef< _TyChar > _TyAttDef;
  typedef xml_validate_attr< _TyChar > _TyAttr;
  typedef _xml_content_model< _TyChar > _TyContentModel;

  // _pdtd: the DTD of the document - null if the document has no document type declaration: then it is not valid and
  //  that is all that is said of it. The DTD must remain where it is for as long as this is used.
  explicit xml_validator( const _TyDtd * _pdtd )
    : m_pdtd( _pdtd )
  {
  }
  xml_validator( xml_validator const & ) = delete;
  xml_validator & operator =( xml_validator const & ) = delete;

  // A violation is thrown - as an xml_parse_exception - and not recorded.
  void SetThrowOnError( bool _fThrowOnError = true )
  {
    m_fThrowOnError = _fThrowOnError;
  }
  bool FGetThrowOnError() const
  {
    return m_fThrowOnError;
  }
  // The document is declared standalone in its XML declaration: VC: Standalone Document Declaration is checked.
  void SetStandalone( bool _fStandalone = true )
  {
    m_fStandalone = _fStandalone;
  }
  // The document is read with Namespaces in XML: then it is to be namespace-valid as well - the value of an attribute
  //  whose declared type is ID, IDREF(S), ENTITY(IES) or NOTATION has no colon.
  void SetUseXMLNamespaces( bool _fUseXMLNamespaces = true )
  {
    m_fUseXMLNamespaces = _fUseXMLNamespaces;
  }
  bool FGetStandalone() const
  {
    return m_fStandalone;
  }
  // VC: Standalone Document Declaration - what whoever reads the document finds: a reference to an entity which is
  //  declared externally, an attribute whose value is another by its type which is declared externally.
  void StandaloneViolation( const char * _pszWhat, _TyStrView _svName )
  {
    _Error( "The document is declared standalone and %s [%s] - which is declared externally.", _pszWhat, _StrName( _svName ).c_str() );
  }
  // The violations found so far - those of the document, not those of the declarations: see xml_dtd<>::m_rgstrValidityErrors.
  vector< string > const & RgErrors() const
  {
    return m_rgstrErrors;
  }
  // Whether the document - as much of it as we have been told of - is valid: with the declarations of its DTD.
  bool FIsValid() const
  {
    return !!m_pdtd && m_pdtd->m_rgstrValidityErrors.empty() && m_rgstrErrors.empty();
  }

// What the document has, in the order in which it has it:
  // The start tag of an element - or an empty element tag, which is then followed by EndTag().
  // _pattrBegin, _pattrEnd: the attributes which the tag has - not those which the DTD gives it.
  void StartTag( _TyStrView _svName, const _TyAttr * _pattrBegin, const _TyAttr * _pattrEnd )
  {
    if ( !m_pdtd )
    {
      if ( m_rgElements.empty() && !m_fDocumentElementEnded )
        _Error( "The document has no document type declaration: there is nothing which it is valid against." );
      m_rgElements.emplace_back();
      return;
    }
    if ( m_rgElements.empty() )
    {
      // VC: Root Element Type.
      if ( _svName != _TyStrView( m_pdtd->m_strRootName ) )
        _Error( "The document's element is <%s> and its document type declaration is for <%s>.", _StrName( _svName ).c_str(), _StrName( m_pdtd->m_strRootName ).c_str() );
    }
    else
      _ChildElement( m_rgElements.back(), _svName );
    // VC: Element Valid - the element type is declared.
    _TyElementState & res = m_rgElements.emplace_back();
    res.m_pElement = m_pdtd->PGetElement( _svName );
    if ( !res.m_pElement )
      _Error( "The element type <%s> is not declared.", _StrName( _svName ).c_str() );
    else
    if ( exdcChildren == res.m_pElement->m_eContent )
      res.m_pModel = &_RModel( *res.m_pElement );
    _Attributes( _svName, _pattrBegin, _pattrEnd );
  }
  // Character data - or a CDATA section - within the current element.
  // _fWhiteSpace: it is white space which matches S [3]: that is all which may be among the elements of element content.
  //  A CDATA section is never that and neither is anything which has a character reference.
  void CharacterData( bool _fWhiteSpace )
  {
    if ( m_rgElements.empty() )
      return;
    _TyElementState & res = m_rgElements.back();
    if ( !res.m_pElement || res.m_fContentError )
      return;
    switch( res.m_pElement->m_eContent )
    {
      case exdcEmpty:
        res.m_fContentError = true;
        _Error( "The element <%s> is declared EMPTY and has %s.", _StrName( res.m_pElement->m_strName ).c_str(), _fWhiteSpace ? "white space" : "character data" );
      break;
      case exdcChildren:
        if ( !_fWhiteSpace )
        {
          res.m_fContentError = true;
          _Error( "The element <%s> is declared to have element content and has character data.", _StrName( res.m_pElement->m_strName ).c_str() );
        }
        else
        if ( m_fStandalone && res.m_pElement->m_fDeclaredExternally && !res.m_fStandaloneWhiteSpace )
        {
          // VC: Standalone Document Declaration: without the declaration this white space is character data of the document.
          res.m_fStandaloneWhiteSpace = true;
          _Error( "The document is declared standalone and has white space within the element <%s> - whose element content is declared externally.",
                  _StrName( res.m_pElement->m_strName ).c_str() );
        }
      break;
      default:
      break;
    }
  }
  // Something is within the current element which we are not told of: a reference to an entity whose replacement text
  //  is not read. Then nothing more is said of what is within the element - whether it is as declared is not known.
  void ContentNotKnown()
  {
    if ( !m_rgElements.empty() )
      m_rgElements.back().m_fContentError = true;
  }
  // A comment or a processing instruction within the current element.
  void CommentOrProcessingInstruction()
  {
    if ( m_rgElements.empty() )
      return;
    _TyElementState & res = m_rgElements.back();
    if ( !!res.m_pElement && !res.m_fContentError && ( exdcEmpty == res.m_pElement->m_eContent ) )
    {
      res.m_fContentError = true;
      _Error( "The element <%s> is declared EMPTY and has a comment or a processing instruction.", _StrName( res.m_pElement->m_strName ).c_str() );
    }
  }
  // The end of the current element.
  void EndTag()
  {
    Assert( !m_rgElements.empty() );
    if ( m_rgElements.empty() )
      return;
    {//B
      _TyElementState & res = m_rgElements.back();
      if ( !!res.m_pModel && !res.m_fContentError && !res.m_pModel->FMayEnd( res.m_fAtStart ? nullptr : &res.m_rgPositions ) )
        _Error( "The content of the element <%s> ends where its content model has more: %s.", _StrName( res.m_pElement->m_strName ).c_str(),
                res.m_pModel->StrExpected( res.m_fAtStart ? nullptr : &res.m_rgPositions ).c_str() );
    }//EB
    m_rgElements.pop_back();
    if ( m_rgElements.empty() )
    {
      // The end of the document's element - every ID of the document has been seen. VC: IDREF.
      m_fDocumentElementEnded = true;
      for ( _TyStdStr const & rstrIdRef : m_rgstrIdRefs )
      {
        if ( m_setIds.end() == m_setIds.find( rstrIdRef ) )
          _Error( "The IDREF [%s] is not the ID of any element of the document.", _StrName( rstrIdRef ).c_str() );
      }
      m_rgstrIdRefs.clear();
    }
  }
protected:
  struct _TyElementState
  {
    const _TyElement * m_pElement{ nullptr }; // Null if the element type is not declared: then nothing is known of its content.
    const _TyContentModel * m_pModel{ nullptr }; // Element content.
    typename _TyContentModel::_TyRgPositions m_rgPositions; // Where we are in the content model - once we have left its start.
    bool m_fAtStart{ true };
    bool m_fContentError{ false }; // The content is not as declared - said once - or is not known: the rest of it is not looked at.
    bool m_fStandaloneWhiteSpace{ false }; // Said - once for the element.
  };
  static string _StrName( _TyStrView _sv )
  {
    return _sv.empty() ? string() : StrConvertString< char >( _sv ); // An empty view may have no characters to point to.
  }
  void _Error( const char * _pszFmt, ... )
  {
    string strError;
    va_list ap;
    va_start( ap, _pszFmt );
    VPrintfStdStr( strError, _pszFmt, ap );
    va_end( ap );
    if ( m_fThrowOnError )
      THROWXMLPARSEEXCEPTION( "The document is not valid: %s", strError.c_str() );
    m_rgstrErrors.push_back( std::move( strError ) );
  }
  _TyContentModel const & _RModel( _TyElement const & _rElement )
  {
    typename _TyMapModels::iterator it = m_mapModels.find( &_rElement );
    if ( m_mapModels.end() == it )
      it = m_mapModels.emplace( &_rElement, _rElement.m_cpChildren ).first;
    return it->second;
  }
  // VC: Element Valid - an element of the type _svName within the element _rres.
  void _ChildElement( _TyElementState & _rres, _TyStrView _svName )
  {
    if ( !_rres.m_pElement || _rres.m_fContentError )
      return;
    const _TyElement & rElement = *_rres.m_pElement;
    switch( rElement.m_eContent )
    {
      case exdcEmpty:
        _rres.m_fContentError = true;
        _Error( "The element <%s> is declared EMPTY and has the element <%s>.", _StrName( rElement.m_strName ).c_str(), _StrName( _svName ).c_str() );
      break;
      case exdcAny:
      break; // The element must be of a type which is declared - and that is said of it when it is started.
      case exdcMixed:
      {
        if ( rElement.m_rgstrMixedNames.end() == std::find_if( rElement.m_rgstrMixedNames.begin(), rElement.m_rgstrMixedNames.end(),
              [_svName]( _TyStdStr const & _rstr ) { return _TyStrView( _rstr ) == _svName; } ) )
        {
          // Each such element is said: this is not an order which is lost once it is wrong.
          _Error( "The element <%s> may not be within the element <%s>: it is not among the element types of its mixed content.",
                  _StrName( _svName ).c_str(), _StrName( rElement.m_strName ).c_str() );
        }
      }
      break;
      case exdcChildren:
      {
        typename _TyContentModel::_TyRgPositions rgNext;
        _rres.m_pModel->GetNext( _rres.m_fAtStart ? nullptr : &_rres.m_rgPositions, _svName, rgNext );
        if ( rgNext.empty() )
        {
          _rres.m_fContentError = true;
          _Error( "The element <%s> may not be here within the element <%s>: its content model has %s.", _StrName( _svName ).c_str(),
                  _StrName( rElement.m_strName ).c_str(), _rres.m_pModel->StrExpected( _rres.m_fAtStart ? nullptr : &_rres.m_rgPositions ).c_str() );
        }
        else
        {
          _rres.m_rgPositions.swap( rgNext );
          _rres.m_fAtStart = false;
        }
      }
      break;
      default:
        Assert( false );
      break;
    }
  }
// The attributes:
  void _Attributes( _TyStrView _svElement, const _TyAttr * _pattrBegin, const _TyAttr * _pattrEnd )
  {
    const vector< _TyAttDef > * prgAttDefs = m_pdtd->PGetAttList( _svElement );
    // VC: Attribute Value Type - each attribute which the tag has is declared, and its value is of the declared type.
    for ( const _TyAttr * pattrCur = _pattrBegin; _pattrEnd != pattrCur; ++pattrCur )
    {
      const _TyAttDef * pAttDef = nullptr;
      if ( prgAttDefs )
      {
        for ( _TyAttDef const & rad : *prgAttDefs )
        {
          if ( _TyStrView( rad.m_strName ) == pattrCur->m_svName )
          {
            pAttDef = &rad;
            break;
          }
        }
      }
      if ( !pAttDef )
      {
        _Error( "The attribute [%s] of the element <%s> is not declared.", _StrName( pattrCur->m_svName ).c_str(), _StrName( _svElement ).c_str() );
        continue;
      }
      _AttributeValue( _svElement, *pAttDef, pattrCur->m_svValue );
      // VC: Fixed Attribute Default.
      if ( ( exdadFixed == pAttDef->m_eDefault ) && pAttDef->m_fHasDefaultValue && ( pattrCur->m_svValue != _TyStrView( pAttDef->m_strDefaultValue ) ) )
        _Error( "The attribute [%s] of the element <%s> is declared #FIXED as [%s] and its value is [%s].", _StrName( pAttDef->m_strName ).c_str(),
                _StrName( _svElement ).c_str(), _StrName( pAttDef->m_strDefaultValue ).c_str(), _StrName( pattrCur->m_svValue ).c_str() );
    }
    if ( !prgAttDefs )
      return;
    // Those which the tag doesn't have.
    for ( _TyAttDef const & rad : *prgAttDefs )
    {
      const _TyAttr * pattrCur = _pattrBegin;
      for ( ; ( _pattrEnd != pattrCur ) && ( pattrCur->m_svName != _TyStrView( rad.m_strName ) ); ++pattrCur )
        ;
      if ( _pattrEnd != pattrCur )
        continue;
      // VC: Required Attribute.
      if ( exdadRequired == rad.m_eDefault )
        _Error( "The attribute [%s] of the element <%s> is declared #REQUIRED and the tag doesn't have it.", _StrName( rad.m_strName ).c_str(), _StrName( _svElement ).c_str() );
      else
      if ( rad.m_fHasDefaultValue )
      {
        // VC: Standalone Document Declaration: without the declaration the tag doesn't have the attribute.
        if ( m_fStandalone && rad.m_fDeclaredExternally )
          _Error( "The document is declared standalone and a tag <%s> has the attribute [%s] by its default - which is declared externally.",
                  _StrName( _svElement ).c_str(), _StrName( rad.m_strName ).c_str() );
        _AttributeValue( _svElement, rad, rad.m_strDefaultValue ); // The tag has it by default - and what it refers to must be there.
      }
    }
  }
  // VC: Attribute Value Type - the value of an attribute by its declared type.
  void _AttributeValue( _TyStrView _svElement, _TyAttDef const & _rad, _TyStrView _svValue )
  {
    const char * pszWhat = nullptr; // What the value should be and isn't.
    switch( _rad.m_eType )
    {
      case exdatCDATA:
      break;
      case exdatID:
      {
        // VC: ID.
        if ( !_TyDtd::FIsName( _svValue ) )
          pszWhat = "a Name";
        else
        if ( !m_setIds.insert( _TyStdStr( _svValue ) ).second )
          _Error( "The ID [%s] - of the attribute [%s] of an element <%s> - is the ID of another element as well.", _StrName( _svValue ).c_str(),
                  _StrName( _rad.m_strName ).c_str(), _StrName( _svElement ).c_str() );
      }
      break;
      case exdatIDREF:
      {
        // VC: IDREF - whether it is an ID of the document is known at the end of the document.
        if ( !_TyDtd::FIsName( _svValue ) )
          pszWhat = "a Name";
        else
          m_rgstrIdRefs.emplace_back( _svValue );
      }
      break;
      case exdatIDREFS:
      {
        if ( !_FForEachToken( _svValue, [this]( _TyStrView _svToken ) -> bool
              {
                if ( !_TyDtd::FIsName( _svToken ) )
                  return false;
                m_rgstrIdRefs.emplace_back( _svToken );
                return true;
              } ) )
          pszWhat = "Names";
      }
      break;
      case exdatENTITY:
      {
        // VC: Entity Name.
        if ( !_TyDtd::FIsName( _svValue ) )
          pszWhat = "a Name";
        else
          _UnparsedEntity( _svElement, _rad, _svValue );
      }
      break;
      case exdatENTITIES:
      {
        if ( !_FForEachToken( _svValue, [this,_svElement,&_rad]( _TyStrView _svToken ) -> bool
              {
                if ( !_TyDtd::FIsName( _svToken ) )
                  return false;
                _UnparsedEntity( _svElement, _rad, _svToken );
                return true;
              } ) )
          pszWhat = "Names";
      }
      break;
      case exdatNMTOKEN:
        // VC: Name Token.
        if ( !_TyDtd::FIsNmtoken( _svValue ) )
          pszWhat = "a name token";
      break;
      case exdatNMTOKENS:
        if ( !_FForEachToken( _svValue, []( _TyStrView _svToken ) { return _TyDtd::FIsNmtoken( _svToken ); } ) )
          pszWhat = "name tokens";
      break;
      case exdatNOTATION:
      case exdatEnumeration:
      {
        // VC: Notation Attributes, VC: Enumeration.
        if ( _rad.m_rgstrEnumeration.end() == std::find_if( _rad.m_rgstrEnumeration.begin(), _rad.m_rgstrEnumeration.end(),
              [_svValue]( _TyStdStr const & _rstr ) { return _TyStrView( _rstr ) == _svValue; } ) )
          pszWhat = ( exdatNOTATION == _rad.m_eType ) ? "one of the notations declared for it" : "one of the values declared for it";
      }
      break;
      default:
        Assert( false );
      break;
    }
    if ( !pszWhat && m_fUseXMLNamespaces && ( _TyStrView::npos != _svValue.find( _TyChar( ':' ) ) ) )
    {
      switch( _rad.m_eType )
      {
        case exdatID:
        case exdatIDREF:
        case exdatIDREFS:
        case exdatENTITY:
        case exdatENTITIES:
        case exdatNOTATION:
          pszWhat = "without a colon - which the value of an attribute of its type is to be for the document to be namespace-valid";
        break;
        default:
        break;
      }
    }
    if ( pszWhat )
      _Error( "The value [%s] of the attribute [%s] of the element <%s> is not %s.", _StrName( _svValue ).c_str(), _StrName( _rad.m_strName ).c_str(),
              _StrName( _svElement ).c_str(), pszWhat );
  }
  void _UnparsedEntity( _TyStrView _svElement, _TyAttDef const & _rad, _TyStrView _svEntity )
  {
    const xml_dtd_entity< _TyChar > * pEntity = m_pdtd->PGetGeneralEntity( _svEntity );
    if ( !pEntity || !pEntity->FIsUnparsed() )
      _Error( "The value [%s] of the attribute [%s] of the element <%s> is not the name of an unparsed entity which is declared.", _StrName( _svEntity ).c_str(),
              _StrName( _rad.m_strName ).c_str(), _StrName( _svElement ).c_str() );
  }
  // Each of the tokens of a normalized value: they are separated by single spaces. There must be one at least.
  template < class t_TyFunctor >
  static bool _FForEachToken( _TyStrView _svValue, t_TyFunctor && _rrftor )
  {
    if ( _svValue.empty() )
      return false;
    for ( ; ; )
    {
      const size_t knPosSpace = _svValue.find( _TyChar( ' ' ) );
      if ( !_rrftor( _svValue.substr( 0, knPosSpace ) ) )
        return false;
      if ( _TyStrView::npos == knPosSpace )
        return true;
      _svValue.remove_prefix( knPosSpace + 1 );
    }
  }

  const _TyDtd * m_pdtd;
  vector< _TyElementState > m_rgElements; // The elements which we are within - the document's element first.
  typedef map< const _TyElement *, _TyContentModel > _TyMapModels;
  _TyMapModels m_mapModels; // The content models, made as they are first needed.
  set< _TyStdStr, less<> > m_setIds; // The IDs of the document.
  vector< _TyStdStr > m_rgstrIdRefs; // The IDREFs of the document - until its end.
  vector< string > m_rgstrErrors;
  bool m_fThrowOnError{ false };
  bool m_fStandalone{ false };
  bool m_fUseXMLNamespaces{ false };
  bool m_fDocumentElementEnded{ false };
};

__XMLP_END_NAMESPACE
