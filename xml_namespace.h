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
template < class t_TyUriMap >
class _xml_namespace_uri
{
  typedef _xml_namespace_uri _TyThis;
public:
  typedef t_TyUriMap _TyUriMap;
  typedef typename _TyUriMap::key_type _TyUriKey;
  typedef typename _TyUriMap::value_type _TyUriValue;
  typedef _TyUriKey _TyStdStr;
  typedef typename _TyUriKey::value_type _TyChar;
  typedef basic_string_view< _TyChar > _TyStdStrView;

  ~_xml_namespace_uri() = default;
  _xml_namespace_uri( const _TyUriValue * _pvtUriMap )
    : m_pvtUriMap( _pvtUriMap )
  {
  }
  _xml_namespace_uri() = default;
  _xml_namespace_uri( _xml_namespace_uri const & ) = default;
  _xml_namespace_uri & operator =( _xml_namespace_uri const & ) = default;
  void swap( _TyThis & _r )
  {
    std::swap( m_pvtUriMap, _r.m_pvtUriMap );
  }
  _TyStdStr const & RStrUri() const
  {
    Assert( !!m_pvtUriMap );
    return *m_pvtUriMap;
  }
protected:
  const _TyUriValue * m_pvtUriMap{nullptr};
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

  ~xml_namespace_value_wrap()
  {
    if ( m_pmapReleaseOnDestruct )
    {
      Assert( m_pnleUri == m_pvt->second.PListElFront() ); // Invariant.
      if ( m_pnleUri == m_pvt->second.PListElFront() )
      {
        m_pvt->second.pop();
        if ( m_pvt->second.empty() ) // If the list of Uris became empty...
        {
          size_t stErased = m_pmapReleaseOnDestruct->erase( m_pvt->first );
          Assert( 1 == stErased );
        }
      }
    }
  }
  xml_namespace_value_wrap() = default;
  xml_namespace_value_wrap(  _TyMapValue const & _rvt, _TyNamespaceMap * _pmapReleaseOnDestruct )
    : m_pvt( &_rvt ),
      m_pnleUri( _rvt.second.PListElFront() ),
      m_pmapReleaseOnDestruct( _pmapReleaseOnDestruct )
  {
  }
  xml_namespace_value_wrap( xml_namespace_value_wrap const & ) = default;
  xml_namespace_value_wrap & operator =( xml_namespace_value_wrap const & ) = default;
  void swap( _TyThis & _r )
  {
    std::swap( m_pvt, _r.m_pvt );
    std::swap( m_pnleUri, _r.m_pnleUri );
    std::swap( m_pmapReleaseOnDestruct, _r.m_pmapReleaseOnDestruct );
  }
  void AssertValid() const
  {
#if ASSERTSENABLED
    Assert( m_pvt->second.FFind( m_pnleUri ) );
    Assert( !m_pmapReleaseOnDestruct || ( m_pmapReleaseOnDestruct->end() != m_pmapReleaseOnDestruct->find( m_pvt->first ) ) );
#endif //ASSERTSENABLED
  }

  bool FIsNamespaceDeclaration() const
  {
    return !!m_pmapReleaseOnDestruct;
  }
  const _TyMapValue * PVtNamespaceMapValue() const
  {
    return m_pvt;
  }
  const _TyStdStr & RStringUri() const
  {
    return m_pnleUri->RStrUri();
  }
  // This doesn't do a full compare - only the correct thing for comparing unique attributes.
  std::strong_ordering ICompareForUniqueAttr( const _TyThis & _r ) const
  {
    std::strong_ordering iComp = FIsNamespaceDeclaration() <=> _r.FIsNamespaceDeclaration();
    if ( !iComp )
    {
      if ( FIsNamespaceDeclaration() )
      {
        // Compare prefix - no two declarations should have the same prefix.
        iComp = m_pvt.first <=> _r.m_pvt.first;
      }
      else
      {
        // Compare URI - no two same attribute names should have the same URI reference.
        // The URI is a pointer - we don't need to compare the strings - just the pointer - the strings may not have the same comparison sign and that is ok.
        iComp = &m_pnleUri->RStrUri() <=> &_r.m_pnleUri->RStrUri();
      }
    }
    return iComp;
  }
  // Full compare.
  std::strong_ordering ICompare( const _TyThis & _r ) const
  {
    std::strong_ordering iComp = FIsNamespaceDeclaration() <=> _r.FIsNamespaceDeclaration();
    if ( !iComp )
    {
      iComp = m_pvt.first <=> _r.m_pvt.first;
      if ( !iComp )
        iComp = &m_pnleUri->RStrUri() <=> &_r.m_pnleUri->RStrUri();
    }
    return iComp;
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
    size_t nUri = 0;
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
  // If non-null then destruction of this object will remove m_pnleUri from the top of m_pvt->second's list.
  _TyNamespaceMap * m_pmapReleaseOnDestruct{nullptr};
};

__XMLP_END_NAMESPACE
