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
  typedef typename _xml_namespace_map_traits< _TyChar >::_TyStdStr _TyStdStr;
  typedef typename _xml_namespace_map_traits< _TyChar >::_TyStrView _TyStrView;
  typedef typename _xml_namespace_map_traits< _TyChar >::_TyUriAndPrefixMap _TyUriAndPrefixMap;
  typedef typename _TyUriAndPrefixMap::key_type _TyUriKey;
  typedef typename _TyUriAndPrefixMap::value_type _TyUriValue;

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
template < class t_TyChar >
class xml_namespace_value_wrap
{
  typedef xml_namespace_value_wrap _TyThis;
public:
  typedef t_TyChar _TyChar;
  typedef typename _xml_namespace_map_traits< _TyChar >::_TyStdStr _TyStdStr;
  typedef typename _xml_namespace_map_traits< _TyChar >::_TyStrView _TyStrView;
  typedef typename _xml_namespace_map_traits< _TyChar >::_TyUriAndPrefixMap _TyUriAndPrefixMap;
  typedef typename _xml_namespace_map_traits< _TyChar >::_TyNamespaceUri _TyNamespaceUri;
  typedef typename _xml_namespace_map_traits< _TyChar >::_TyNamespaceMap _TyNamespaceMap;
  typedef typename _TyNamespaceMap::value_type _TyNamespaceMapValue;
  typedef typename _TyUriAndPrefixMap::value_type _TyUriAndPrefixValue;

  ~xml_namespace_value_wrap()
  {
    ResetNamespaceDecls();
  }
  xml_namespace_value_wrap() = default;
  xml_namespace_value_wrap( _TyNamespaceMapValue & _rvt, _TyNamespaceMap * _pmapReleaseOnDestruct )
    : m_pvtNamespaceMap( !_pmapReleaseOnDestruct ? nullptr : &_rvt ),
      m_pmapReleaseOnDestruct( _pmapReleaseOnDestruct ),
      m_pvtUri( &_rvt.second.second.front().RStrUri() ),
      m_pvtPrefix( _rvt.second.first )
  {
  }
  void Init( _TyNamespaceMapValue & _rvt, _TyNamespaceMap * _pmapReleaseOnDestruct )
  {
    Assert( FIsNull() );
    m_pvtNamespaceMap = !_pmapReleaseOnDestruct ? nullptr : &_rvt;
    m_pmapReleaseOnDestruct = _pmapReleaseOnDestruct;
    m_pvtUri = &_rvt.second.second.front().RStrUri();
    m_pvtPrefix = _rvt.second.first;
  }
  xml_namespace_value_wrap( xml_namespace_value_wrap const & ) = delete;
  xml_namespace_value_wrap & operator =( xml_namespace_value_wrap const & ) = delete;
  xml_namespace_value_wrap( xml_namespace_value_wrap && _rr )
  {
    swap( _rr );
  }
  xml_namespace_value_wrap & operator =( xml_namespace_value_wrap && _rr )
  {
    xml_namespace_value_wrap acquire( std::move( _rr ) );
    swap( acquire );
    return *this;
  }
  void swap( _TyThis & _r )
  {
    std::swap( m_pvtNamespaceMap, _r.m_pvtNamespaceMap );
    std::swap( m_pmapReleaseOnDestruct, _r.m_pmapReleaseOnDestruct );
    std::swap( m_pvtUri, _r.m_pvtUri );
    std::swap( m_pvtPrefix, _r.m_pvtPrefix );
  }
  bool FIsNull() const
  {
    AssertValid();
    return !m_pvtUri;
  }
  void AssertValid() const
  {
#if ASSERTSENABLED
    Assert( !m_pvtNamespaceMap == !m_pmapReleaseOnDestruct );
    Assert( !m_pmapReleaseOnDestruct || ( m_pmapReleaseOnDestruct->end() != m_pmapReleaseOnDestruct->find( m_pvtNamespaceMap->first ) ) );
    Assert( !m_pmapReleaseOnDestruct || ( m_pvtNamespaceMap == &*m_pmapReleaseOnDestruct->find( m_pvtNamespaceMap->first ) ) );
    Assert( !m_pmapReleaseOnDestruct || ( m_pvtUri == &m_pvtNamespaceMap->second.second.front().RStrUri() ) );
    Assert( !m_pmapReleaseOnDestruct || ( m_pvtPrefix == m_pvtNamespaceMap->second.first ) );
    Assert( !m_pvtUri == !m_pvtPrefix );
    Assert( !m_pvtNamespaceMap || !!m_pvtPrefix );
#endif //ASSERTSENABLED
  }
  void ResetNamespaceDecls()
  {
    AssertValid();
    if ( m_pmapReleaseOnDestruct )
    {
      if ( m_pvtUri == &m_pvtNamespaceMap->second.second.front().RStrUri() )
      {
        m_pvtNamespaceMap->second.second.pop();
        if ( m_pvtNamespaceMap->second.second.empty() ) // If the list of Uris became empty...
        {
          size_t stErased = m_pmapReleaseOnDestruct->erase( m_pvtNamespaceMap->first );
          Assert( 1 == stErased );
        }
      }
      // regardless we nullify these.
      m_pmapReleaseOnDestruct = nullptr;
      m_pvtNamespaceMap = nullptr;
    }
  }
  bool FIsNamespaceDeclaration() const
  {
    AssertValid();
    return !!m_pmapReleaseOnDestruct;
  }
  const _TyNamespaceMapValue * PVtNamespaceMapValue() const
  {
    return m_pvtNamespaceMap;
  }
  const _TyStdStr & RStringUri() const
  {
    AssertValid();
    return *m_pvtUri;
  }
  const _TyStdStr & RStringPrefix() const
  {
    AssertValid();
    return *m_pvtPrefix;
  }
  // This doesn't do a full compare - only the correct thing for comparing unique attributes.
  std::strong_ordering ICompareForUniqueAttr( const _TyThis & _r ) const
  {
    AssertValid();
    _r.AssertValid();
    std::strong_ordering iComp = FIsNamespaceDeclaration() <=> _r.FIsNamespaceDeclaration();
    if ( 0 == iComp )
    {
      if ( FIsNamespaceDeclaration() )
      {
        // Compare prefix - no two declarations should have the same prefix.
        iComp = m_pvtPrefix <=> _r.m_pvtPrefix;
      }
      else
      {
        // Compare URI - no two same attribute names should have the same URI reference.
        // The URI is a pointer - we don't need to compare the strings - just the pointer - the strings may not have the same comparison sign and that is ok.
        iComp = m_pvtUri <=> _r.m_pvtUri;
      }
    }
    return iComp;
  }
  // Full compare.
  std::strong_ordering ICompare( const _TyThis & _r ) const
  {
    std::strong_ordering iComp = FIsNamespaceDeclaration() <=> _r.FIsNamespaceDeclaration();
    if ( 0 == iComp )
    {
      iComp = m_pvtUri <=> _r.m_pvtUri;
      if (  0 == iComp )
        iComp = m_pvtPrefix <=> _r.m_pvtPrefix;
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
    if ( !m_pvtUri )
      return;
    _rsv = t_TyStringView( (const typename t_TyStringView::value_type*)&(*m_pvtUri)[0], m_pvtUri->length() );;
  }
  template < class t_TyStringView, class t_TyToken >
  void KGetStringView( t_TyStringView & _rsv, t_TyToken & _rtok ) const
    requires( sizeof( _TyChar ) == sizeof( typename t_TyStringView::value_type ) ) // We only support a view on our character type's size - use GetString instead.
  {
    return GetStringView( _rsv, _rtok );
  }
  template < class t_TyString, class t_TyToken >
  void GetString( t_TyString & _rstr, t_TyToken & /* _rtok */ ) const
  {
    AssertValid();
    Assert( _rstr.empty() );
    if ( !m_pvtNamespaceMap )
      return;
    // Allow conversion to any character type - or just copy:
    ConvertString( _rstr, *m_pvtUri );
  }
  template < class t_TyStringView, class t_TyString, class t_TyToken >
  bool FGetStringViewOrString( t_TyStringView & _rsv, t_TyString & _rstr, t_TyToken & /* _rtok */ ) const
    requires( sizeof( _TyChar ) == sizeof( typename t_TyStringView::value_type ) ) // non-converting.
  {
    AssertValid();
    Assert( _rsv.empty() );
    Assert( _rstr.empty() );
    if ( m_pvtUri )
      _rsv = t_TyStringView( (const typename t_TyStringView::value_type*)&(*m_pvtUri)[0], m_pvtUri->length() );;
    return true;
  }
  template < class t_TyStringView, class t_TyString, class t_TyToken >
  bool FGetStringViewOrString( t_TyStringView & _rsv, t_TyString & _rstr, t_TyToken & /* _rtok */ ) const
    requires( sizeof( _TyChar ) != sizeof( typename t_TyStringView::value_type ) ) // non-converting.
  {
    AssertValid();
    Assert( _rsv.empty() );
    Assert( _rstr.empty() );
    if ( !m_pvtUri )
      return true;
    // Allow conversion to any character type:
    ConvertString( _rstr, *m_pvtUri );
    return false;
  }
  template < class t_TyJsoValue >
  void ToJsoValue( t_TyJsoValue & _rjv )
  {
    AssertValid();
    if ( !m_pvtUri )
      return _rjv.SetNullValue();
    _rjv.SetArrayCapacity( !m_pvtNamespaceMap ? 2 : 3 ); // preallocate
    _rjv[0ull].SetStringValue( *m_pvtPrefix );
    _rjv[1].SetStringValue( *m_pvtUri );
    if ( m_pvtNamespaceMap )
    {
      const size_t knUris = m_pvtNamespaceMap->second.second.count();
      t_TyJsoValue & rrgjvUris = _rjv[2];
      rrgjvUris.SetArrayCapacity( knUris );
      size_t nUri = 0;
      m_pvtNamespaceMap->second.second.Apply( 
        [&nUri,&rrgjvUris]( _TyNamespaceUri const & _rnu )
        {
          rrgjvUris[nUri++].SetStringValue( _rnu.RStrUri() );
        }
      );
    }
  }
  template < class t_TyToken >
  void ProcessStrings( t_TyToken & /* _rtok */ ) const
  {
    // nothing to do here
  }
protected:
  // If these are non-null then destruction of this object will pop the top of m_pvtNamespaceMap->second's list.
  // These are only non-null for namespace declaration attributes.
  _TyNamespaceMapValue * m_pvtNamespaceMap{nullptr};
  _TyNamespaceMap * m_pmapReleaseOnDestruct{nullptr};
  const typename _TyUriAndPrefixValue * m_pvtUri{nullptr};
  const typename _TyUriAndPrefixValue * m_pvtPrefix{nullptr};
};

__XMLP_END_NAMESPACE
