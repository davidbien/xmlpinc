#pragma once

// xml_namespace.h
// XML namespace support stuff.
// dbien
// 10JAN2021

#include "xml_ns.h"
#include "xml_types.h"

__XMLP_BEGIN_NAMESPACE

// REVIEW: May want to hash unique (prefix,uri) pairs in an unordered_set that is around for the lifetime of the parser.
// Then the prefix field in the will point to this value and thus the tokens will be valid for the lifetime of the parser without further modification.

// _xml_namespace_uri:
// This holds the current URI associated with the key which maps to this.
template < class t_TyChar >
class _xml_namespace_uri
{
  typedef _xml_namespace_uri _TyThis;
public:
  typedef t_TyChar _TyChar;
  typedef unique_ptr< _TyThis > _TyPtrThis;
  typedef basic_string< _TyChar > _TyStdStr;

  ~_xml_namespace_uri() = default;
  _xml_namespace_uri( _TyStdStr && _rrstrUri )
    : m_strUri( std::move( _rrstrUri ) )
  {
  }
  _xml_namespace_uri( const _TyChar * _pcUri )
    : m_strUri( _pcUri )
  {
  }
  _xml_namespace_uri() = default;
  _xml_namespace_uri( _xml_namespace_uri const & ) = default;
  _xml_namespace_uri & operator =( _xml_namespace_uri const & ) = default;
  _xml_namespace_uri( _xml_namespace_uri && ) = default;
  _xml_namespace_uri & operator =( _xml_namespace_uri && ) = default;
  void swap( _TyThis & _r )
  {
    m_upThisNext.swap( _r.m_upThisNext );
    m_strUri.swap( _r.m_strUri );
  }
  _TyStdStr const & RStrUri() const
  {
    return m_strUri;
  }
protected:
  _TyStdStr m_strUri;
};

// Define a wrapper for namespace map value of some type.
template < class t_TyNamespaceMap >
class xml_namespace_value_wrap
{
  typedef xml_namespace_value_wrap _TyThis;
public:
  typedef t_TyNamespaceMap _TyNamespaceMap;
  typedef typename _TyNamespaceMap::key_type _TyKey;
  typedef typename _TyKey::value_type _TyChar;
  typedef typename _TyNamespaceMap::mapped_type _TyMapped; // UniquePtrSList< _xml_namespace_uri<t_TyChar> >
  typedef typename _TyMapped::value_type _TyMappedValueType; // _xml_namespace_uri<t_TyChar>
  typedef typename _TyMapped::_TyListEl _TyNamespaceListEl;
  typedef typename _TyMappedValueType::_TyStdStr _TyStdStr;
  typedef typename _TyNamespaceMap::value_type _TyMapValue;

  xml_namespace_value_wrap() = default;
  xml_namespace_value_wrap(  _TyMapValue const & _rvt )
    : m_pvt( &_rvt ),
      m_pnleUri( _rvt.second.PListElFront() )
  {
  }
  xml_namespace_value_wrap( xml_namespace_value_wrap const & ) = default;
  xml_namespace_value_wrap & operator =( xml_namespace_value_wrap const & ) = default;
  xml_namespace_value_wrap( xml_namespace_value_wrap && ) = default;
  xml_namespace_value_wrap & operator =( xml_namespace_value_wrap && ) = default;
  void swap( _TyThis & _r )
  {
    std::swap( m_pvt, _r.m_pvt );
    std::swap( m_pnleUri, _r.m_pnleUri );
  }
  void AssertValid() const
  {
#if ASSERTSENABLED
    Assert( m_pvt->second.FFind( m_pnleUri ) );
#endif //ASSERTSENABLED
  }

// _l_value boilerplate methods: must support these even if they don't mean much to us.
// The string we return is our URI. Since it is constant we can also return a string_view to it.
  template < class t_TyStringView, class t_TyToken >
  void GetStringView( t_TyStringView & _rsv, t_TyToken & /* _rtok */ ) const
    requires( sizeof( _TyChar ) == sizeof( typename t_TyStringView::value_type ) ) // We only support a view on our character type's size - use GetString instead.
  {
    AssertValid();
    Assert( _rsv.empty() );
    if ( !m_pvt )
      return;
    _rsv = t_TyStringView( (const typename t_TyStringView::value_type*)&(*m_pnleUri)->RStrUri()[0], (*m_pnleUri)->RStrUri().length() );;
  }
  template < class t_TyString, class t_TyToken >
  void GetString( t_TyString & _rstr, t_TyToken & /* _rtok */ ) const
  {
    AssertValid();
    Assert( _rstr.empty() );
    if ( !m_pvt )
      return;
    // Allow conversion to any character type - or just copy:
    ConvertString( _rstr, (*m_pnleUri)->RStrUri() );
  }
  template < class t_TyStringView, class t_TyString, class t_TyToken >
  bool FGetStringViewOrString( t_TyStringView & _rsv, t_TyString & _rstr, t_TyToken & /* _rtok */ ) const
    requires( sizeof( _TyChar ) == sizeof( typename t_TyStringView::value_type ) ) // non-converting.
  {
    AssertValid();
    Assert( _rsv.empty() );
    Assert( _rstr.empty() );
    if ( !m_pvt )
      return true;
    _rsv = t_TyStringView( (const typename t_TyStringView::value_type*)&(*m_pnleUri)->RStrUri()[0], (*m_pnleUri)->RStrUri().length() );;
    return true;
  }
  template < class t_TyStringView, class t_TyString, class t_TyToken >
  bool FGetStringViewOrString( t_TyStringView & _rsv, t_TyString & _rstr, t_TyToken & /* _rtok */ ) const
    requires( sizeof( _TyChar ) != sizeof( typename t_TyStringView::value_type ) ) // non-converting.
  {
    AssertValid();
    Assert( _rsv.empty() );
    Assert( _rstr.empty() );
    if ( !m_pvt )
      return true;
    // Allow conversion to any character type:
    ConvertString( _rstr, (*m_pnleUri)->RStrUri() );
    return false;
  }
  template < class t_TyJsoValue >
  void ToJsoValue( t_TyJsoValue & _rjv )
  {
    AssertValid();
    if ( !m_pvt )
    {
      _rjv.SetNullValue();
      return;
    }
    _rjv.SetArrayCapacity( 3 ); // preallocate
    _rjv[0].SetStringValue( m_pvt->first );
    _rjv[1].SetStringValue( (*m_pnleUri)->RStrUri() );
    const size_t knUris = m_pvt->second.NUrisInStack();
    t_TyJsoValue & rrgjvUris = _rjv[2];
    rrgjvUris.SetArrayCapacity( knUris );
    size_type nUri = 0;
    m_pvt->second.ApplyUri( 
      [&nUri,&rrgjvUris]( _TyStdStr const & _rstr )
      {
        rrgjvUris[nUri++].SetStringValue( _rstr );
      }
    );
  }
  template < class t_TyToken >
  void ProcessStrings( t_TyToken & /* _rtok */ ) const
  {
    // nothing to do here
  }
protected:
  const _TyMapValue * m_pvt{nullptr};
  const _TyNamespaceListEl * m_pnleUri; // Invariant: m_pnleUri is contained in m_pvt->second's list.
};

__XMLP_END_NAMESPACE
