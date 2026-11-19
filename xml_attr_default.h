#pragma once

//          Copyright David Lawrence Bien 1997 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt).

// xml_attr_default.h
// The value of an attribute which a tag doesn't have and which the DTD gives it - 3.3.2.
// dbien
// 19NOV2026

#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include "_strutil.h"
#include "_namdexc.h"
#include "xml_ns.h"
#include "xml_types.h"
#include "xml_namespace.h"

__XMLP_BEGIN_NAMESPACE

// xml_attr_default_value:
// One of the types by which the XML parser extends _l_value - as xml_namespace_value_wrap is.
// When the read cursor has the DTD it adds to a tag the attributes which the tag doesn't have and for which the DTD
//  declares a default value. The value of such an attribute is one of these and not a string:
//  1) So that it is known that the attribute was not in the document: a writer which writes the document type
//     declaration needn't write the attribute - whoever reads what it writes will add it again.
//  2) So that nothing has to be remembered about that: a value which is set to anything else is no longer one of these.
//  3) Nothing is copied or allocated for the tag: this refers to the value as the DTD has it - the default with its
//     references replaced and normalized as the declared type says.
// As the read cursor makes it this refers to the DTD: it is good for as long as the DTD is - the xml_dtd of the read cursor.
// When a token is copied into another container - a document, or a writer as it writes the token - the copy is one of
//  these still, with a copy of the value of its own in the character type of that container: it doesn't refer to the DTD
//  and it is still known for what it is. See _xml_document_context<>::TranslateUserType().
template < class t_TyChar >
class xml_attr_default_value
{
  typedef xml_attr_default_value _TyThis;
public:
  typedef t_TyChar _TyChar;
  typedef basic_string< _TyChar > _TyStdStr;
  typedef basic_string_view< _TyChar > _TyStrView;

  xml_attr_default_value() = default;
  xml_attr_default_value( xml_attr_default_value const & ) = default;
  xml_attr_default_value & operator =( xml_attr_default_value const & ) = default;
  // The value is elsewhere and remains there for as long as this is used: in the DTD.
  explicit xml_attr_default_value( _TyStrView _svValue )
    : m_svValue( _svValue )
  {
  }
  // The value is ours. ( Shared by the copies of this: a value is not changed - it is replaced by something which is not one of these. )
  explicit xml_attr_default_value( _TyStdStr && _rrstrValue )
    : m_spstrOwned( std::make_shared< const _TyStdStr >( std::move( _rrstrValue ) ) )
  {
    m_svValue = *m_spstrOwned;
  }
  void swap( _TyThis & _r )
  {
    std::swap( m_svValue, _r.m_svValue );
    m_spstrOwned.swap( _r.m_spstrOwned );
  }
  // Whether the value refers to the DTD - then this is good for as long as the DTD is.
  bool FRefersToDtd() const
  {
    return !m_spstrOwned;
  }
  // The value of the attribute.
  _TyStrView SvValue() const
  {
    return m_svValue;
  }

// The methods which _l_value calls for the types by which it is extended.
  template < class t_TyStringView, class t_TyToken >
  void GetStringView( t_TyStringView & _rsv, t_TyToken & /* _rtok */ ) const
    requires( sizeof( _TyChar ) == sizeof( typename t_TyStringView::value_type ) )
  {
    Assert( _rsv.empty() );
    _rsv = t_TyStringView( (const typename t_TyStringView::value_type*)m_svValue.data(), m_svValue.length() );
  }
  template < class t_TyStringView, class t_TyToken >
  void KGetStringView( t_TyStringView & _rsv, t_TyToken & _rtok ) const
    requires( sizeof( _TyChar ) == sizeof( typename t_TyStringView::value_type ) )
  {
    GetStringView( _rsv, _rtok );
  }
  // A view in a character type of another size can't be had: there is nowhere here to keep a converted string. These
  //  are here so that _l_value's visitors compile for every type of view - they throw if they are reached.
  template < class t_TyStringView, class t_TyToken >
  void GetStringView( t_TyStringView & /* _rsv */, t_TyToken & /* _rtok */ ) const
    requires( sizeof( _TyChar ) != sizeof( typename t_TyStringView::value_type ) )
  {
    THROWNAMEDBADVARIANTACCESSEXCEPTION("Can't get a string view of the default value of an attribute in a character type not matching that of the parser - use GetString() or FGetStringViewOrString() instead.");
  }
  template < class t_TyStringView, class t_TyToken >
  void KGetStringView( t_TyStringView & /* _rsv */, t_TyToken & /* _rtok */ ) const
    requires( sizeof( _TyChar ) != sizeof( typename t_TyStringView::value_type ) )
  {
    THROWNAMEDBADVARIANTACCESSEXCEPTION("Can't get a string view of the default value of an attribute in a character type not matching that of the parser - use GetString() or FGetStringViewOrString() instead.");
  }
  template < class t_TyString, class t_TyToken >
  void GetString( t_TyString & _rstr, t_TyToken & /* _rtok */ ) const
  {
    Assert( _rstr.empty() );
    ConvertString( _rstr, m_svValue ); // Any character type - or just a copy.
  }
  template < class t_TyStringView, class t_TyString, class t_TyToken >
  bool FGetStringViewOrString( t_TyStringView & _rsv, t_TyString & /* _rstr */, t_TyToken & _rtok ) const
    requires( sizeof( _TyChar ) == sizeof( typename t_TyStringView::value_type ) )
  {
    GetStringView( _rsv, _rtok );
    return true;
  }
  template < class t_TyStringView, class t_TyString, class t_TyToken >
  bool FGetStringViewOrString( t_TyStringView & _rsv, t_TyString & _rstr, t_TyToken & /* _rtok */ ) const
    requires( sizeof( _TyChar ) != sizeof( typename t_TyStringView::value_type ) )
  {
    Assert( _rsv.empty() );
    Assert( _rstr.empty() );
    ConvertString( _rstr, m_svValue );
    _rsv = t_TyStringView( (const typename t_TyStringView::value_type*)_rstr.data(), _rstr.length() );
    return false;
  }
  template < class t_TyJsoValue >
  void ToJsoValue( t_TyJsoValue & _rjv ) const
  {
    _rjv.SetStringValue( m_svValue );
  }
  template < class t_TyToken >
  void ProcessStrings( t_TyToken & /* _rtok */ ) const
  {
    // Nothing to do: we remain what we are.
  }
protected:
  _TyStrView m_svValue; // xml_dtd_attdef<>::m_strDefaultValue of the declaration of the attribute - or *m_spstrOwned.
  std::shared_ptr< const _TyStdStr > m_spstrOwned; // The value if it is ours: in a copy of a token.
};

// Whether the value of an attribute is the default which the DTD gave it - not what the tag had nor what it was set to since.
template < class t_TyLexValue >
bool FIsAttrDefaultValue( t_TyLexValue const & _rval )
{
  return _rval.template FIsA< xml_attr_default_value< typename t_TyLexValue::_TyChar > >();
}

// The types by which the XML parser extends _l_value: the tokens of every XML parser and of every XML writer have these.
template < class t_TyChar >
using xml_value_traits_pack = tuple< xml_namespace_value_wrap< t_TyChar >, xml_attr_default_value< t_TyChar > >;

__XMLP_END_NAMESPACE
