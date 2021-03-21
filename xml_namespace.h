#pragma once

// xml_namespace.h
// XML namespace support stuff.
// dbien
// 10JAN2021

#include "xml_ns.h"
#include "xml_types.h"
#include "xml_markup_traits.h"

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
    AssertValid();
  }
  xml_namespace_value_wrap( const _TyUriAndPrefixValue * _pvtPrefix, const _TyUriAndPrefixValue * _pvtUri )
    : m_pvtPrefix( _pvtPrefix ),
      m_pvtUri( _pvtUri )
  {
    AssertValid();
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
  // Shed a non-namespace-declaration reference to this same namespace:
  _TyThis ShedReference() const
  {
    return _TyThis( m_pvtPrefix, m_pvtUri );
  }
  bool FIsNull() const
  {
    AssertValid();
    return !m_pvtUri;
  }
  bool FIsNamespaceReference() const
  {
    return !FIsNull() && !m_pmapReleaseOnDestruct;
  }
  bool FIsNamespaceDeclaration() const
  {
    AssertValid();
    return !!m_pmapReleaseOnDestruct;
  }
  const _TyNamespaceMap * PGetNamespaceMap() const
  {
    return m_pmapReleaseOnDestruct;
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
  template < class t_TyString >
  void GetPrStrPrefixUri( pair< t_TyString, t_TyString > & _rprstrPrefixUri ) const
  {
    ConvertString( _rprstrPrefixUri.first, RStringPrefix() );
    ConvertString( _rprstrPrefixUri.second, RStringUri() );
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
    _rsv = t_TyStringView( (const typename t_TyStringView::value_type*)&_rstr[0], _rstr.length() );;
    return false;
  }
  template < class t_TyJsoValue >
  void ToJsoValue( t_TyJsoValue & _rjv ) const
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
  const _TyUriAndPrefixValue * m_pvtUri{nullptr};
  const _TyUriAndPrefixValue * m_pvtPrefix{nullptr};
};

// xml_namespace_map:
// Wrap the namespace map to centralize operations involving it.
template < class t_TyChar >
class xml_namespace_map
{
  typedef xml_namespace_map _TyThis;
public:
  typedef t_TyChar _TyChar;
  typedef typename _xml_namespace_map_traits< _TyChar >::_TyNamespaceMap _TyNamespaceMap;
  typedef typename _xml_namespace_map_traits< _TyChar >::_TyUriAndPrefixMap _TyUriAndPrefixMap;
  typedef typename _xml_namespace_map_traits< _TyChar >::_TyStdStr _TyStdStr;
  typedef typename _xml_namespace_map_traits< _TyChar >::_TyStrView _TyStrView;
  typedef xml_namespace_value_wrap< _TyChar > _TyXmlNamespaceValueWrap;
  typedef xml_markup_traits< _TyChar > _TyMarkupTraits;

  ~xml_namespace_map() = default;
  xml_namespace_map() = default;
  // Initialize from the container of the prefix and URI sets.
  template < class t_TyUriPrefixContainer >
  xml_namespace_map( t_TyUriPrefixContainer & _rcontUriPrefix )
  {
    _InitNamespaceMap( _rcontUriPrefix );
  }
  xml_namespace_map( xml_namespace_map const & ) = delete;
  xml_namespace_map & operator =( xml_namespace_map const & ) = delete;
  xml_namespace_map( xml_namespace_map && ) = default;
  xml_namespace_map & operator =( xml_namespace_map && ) = default;
  template < class t_TyUriPrefixContainer >
  void Init( t_TyUriPrefixContainer & _rcontUriPrefix )
  {
    _InitNamespaceMap( _rcontUriPrefix );
  }
  void clear()
  {
    m_mapNamespaces.clear();
  }
  // Return whether there is a non-empty current default namespace.
  bool FHasDefaultNamespace( _TyXmlNamespaceValueWrap * _pxnvw = nullptr )
  {
    typename _TyNamespaceMap::iterator it = m_mapNamespaces.find( _TyStrView() );
    bool fHasDefault = ( m_mapNamespaces.end() != it ) && !it->second.second.front().RStrUri().empty();
    if ( fHasDefault )
      *_pxnvw = _TyXmlNamespaceValueWrap( *it, nullptr );
    return fHasDefault;
  }
  // Check to see that the passed _TyXmlNamespaceValueWrap is the currently active URI for the prefix given.
  bool FIsActiveNamespace( _TyXmlNamespaceValueWrap const & _rxnvw ) const
  {
    // We are going to assume that if the caller has a _rxnvw.FIsNamespaceDeclaration() and the map pointer
    //  matches then it is an active namespace.
    if ( _rxnvw.PGetNamespaceMap() == &m_mapNamespaces )
    {
      Assert( ( m_mapNamespaces.end() != m_mapNamespaces.find( _rxnvw.RStringPrefix() ) ) &&
              ( m_mapNamespaces.find( _rxnvw.RStringPrefix() )->second.second.front().RStrUri() == _rxnvw.RStringUri() ) );
      return true;
    }
    Assert( !_rxnvw.FIsNull() );
    if ( _rxnvw.FIsNull() )
      return false;
    typename _TyNamespaceMap::const_iterator cit = m_mapNamespaces.find( _rxnvw.RStringPrefix() );
    return ( m_mapNamespaces.end() != cit ) && ( cit->second.second.front().RStrUri() == _rxnvw.RStringUri() );
  }
  // Add the (prefix,uri) pair as the current (prefix,uri) pair and return a xml_namespace_value_wrap.
  // If only _psvPrefix passed then return a xml_namespace_value_wrap that references (*_psvPrefix) in the 
  //  namespace map or throw.
  template < class t_TyUriPrefixContainer, class t_TyStrViewOrString >
  _TyXmlNamespaceValueWrap GetNamespaceValueWrap( t_TyUriPrefixContainer & _rcontUriPrefix, const t_TyStrViewOrString * _psvPrefix, const t_TyStrViewOrString * _psvUri = nullptr )
    requires TAreSameSizeTypes_v< typename t_TyStrViewOrString::value_type, _TyChar >
  {
    // If both are null then we are asking for the current default namespace - and we will throw later if there isn't one.
    VerifyThrowSz( ( !_psvPrefix && !_psvUri ) || ( !!_psvPrefix && ( ( !!_psvUri && !_psvUri->empty() ) || _psvPrefix->empty() ) ), "Must pass at least a prefix and must pass a non-empty URI unless the prefix is empty." );
    if ( !_psvUri )
      return GetNamespaceValueWrap( *_psvPrefix );
    else
      return _GetNamespaceValueWrap( _rcontUriPrefix, *_psvPrefix, *_psvUri );
  }
  template < class t_TyUriPrefixContainer, class t_TyStrViewOrString >
  _TyXmlNamespaceValueWrap GetNamespaceValueWrap( t_TyUriPrefixContainer & _rcontUriPrefix, t_TyStrViewOrString && _rrsvPrefix, t_TyStrViewOrString && _rrsvUri )
    requires TAreSameSizeTypes_v< typename remove_cvref_t< t_TyStrViewOrString >::value_type, _TyChar >
  {
    VerifyThrowSz( !_rrsvUri.empty() || _rrsvPrefix.empty(), "Must pass a non-empty URI unless the prefix is empty." );
    return _GetNamespaceValueWrap( _rcontUriPrefix, std::forward< t_TyStrViewOrString >( _rrsvPrefix ), std::forward< t_TyStrViewOrString >( _rrsvUri ) );
  }
  // Return the namespace reference for _rsvPrefix or the default namespace if any - throw if no namespace found.
  template < class t_TyStrViewOrString >
  _TyXmlNamespaceValueWrap GetNamespaceValueWrap( const t_TyStrViewOrString * _psvPrefix = nullptr )
    requires TAreSameSizeTypes_v< typename t_TyStrViewOrString::value_type, _TyChar >
  {
    typename _TyNamespaceMap::iterator itNM = m_mapNamespaces.find( !_psvPrefix ? _TyStrView() : *_psvPrefix );
    VerifyThrowSz( m_mapNamespaces.end() != itNM, !_psvPrefix ? "No active default namespace." : "No active namespace for prefix[%s].", StrConvertString< char >( *_psvPrefix ).c_str() );
    return _TyXmlNamespaceValueWrap( *itNM, nullptr );
  }
protected:
  template < class t_TyUriPrefixContainer, class t_TyStrViewOrString >
  _TyXmlNamespaceValueWrap _GetNamespaceValueWrap( t_TyUriPrefixContainer & _rcontUriPrefix, t_TyStrViewOrString && _rrsvPrefix, t_TyStrViewOrString && _rrsvUri )
    requires TAreSameSizeTypes_v< typename remove_cvref_t< t_TyStrViewOrString >::value_type, _TyChar >
  {
    // in the map by now - add _rsvUri as the current URI for this prefix.
    pair< typename _TyNamespaceMap::iterator, bool > pib = _PibAddPrefixUri( _rcontUriPrefix, std::forward< t_TyStrViewOrString >( _rrsvPrefix ), std::forward< t_TyStrViewOrString >( _rrsvUri ) );
    return _TyXmlNamespaceValueWrap( *pib.first, pib.second ? &m_mapNamespaces : nullptr );
  }
  // Return the iterator for the prefix and whether we had to add the URI as the current URI or if it already was the current
  //  URI for the given prefix - in which case we didn't add it to the URI stack.
  template < class t_TyUriPrefixContainer, class t_TyStrViewOrString >
  pair< typename _TyNamespaceMap::iterator, bool > _PibAddPrefixUri( t_TyUriPrefixContainer & _rcontUriPrefix, t_TyStrViewOrString && _rrsvPrefix, t_TyStrViewOrString && _rrsvUri )
  {
    Assert( !_rrsvUri.empty() || _rrsvPrefix.empty() ); // error checking above us.
    typename _TyNamespaceMap::iterator itNM = m_mapNamespaces.find( _rrsvPrefix );
    if ( m_mapNamespaces.end() == itNM )
    {
      // not in map - add it:
      typename _TyUriAndPrefixMap::value_type const & rstrPrefix = _rcontUriPrefix.RStrAddPrefix( std::forward< t_TyStrViewOrString >( _rrsvPrefix ) );
      pair< typename _TyNamespaceMap::iterator, bool > pib = m_mapNamespaces.emplace( std::piecewise_construct, std::forward_as_tuple(rstrPrefix), std::forward_as_tuple() );
      Assert( pib.second );
      itNM = pib.first;
      itNM->second.first = &rstrPrefix;
    }
    else
    {
      // already in map - check for reserved prefix "xml":
      if ( _rrsvPrefix == _TyStrView( _TyMarkupTraits::s_kszXmlPrefix, StaticStringLen( _TyMarkupTraits::s_kszXmlPrefix ) ) )
        VerifyThrowSz( _rrsvUri == _TyStrView( _TyMarkupTraits::s_kszXmlUri, StaticStringLen( _TyMarkupTraits::s_kszXmlUri ) ),
          "Only allowed to declare 'xml' prefix as uri 'http://www.w3.org/XML/1998/namespace' not as[%s]",
          StrConvertString< char >( _rrsvUri ).c_str() );
    }
    // If the current URI for this prefix is the same as the URI on the top of the stack then
    //  there is no reason to push the given URI on the stack. In that case we will return false.
    bool fAddUri = itNM->second.second.empty() || ( itNM->second.second.front().RStrUri() != _rrsvUri );
    if ( fAddUri )
    {
      typename _TyUriAndPrefixMap::value_type const & rstrUri = _rcontUriPrefix.RStrAddUri( std::forward< t_TyStrViewOrString >( _rrsvUri ) );
      itNM->second.second.push( &rstrUri );
    }
    return pair< typename _TyNamespaceMap::iterator, bool >( itNM, fAddUri );
  }

  template < class t_TyUriPrefixContainer >
  void _InitNamespaceMap( t_TyUriPrefixContainer & _rcontUriPrefix )
  {
    Assert( !m_mapNamespaces.size() );
  // The prefix xml is by definition bound to the namespace name http://www.w3.org/XML/1998/namespace
  // As we add it here with nothing to remove it, even if someone declares it, it will remain in the namespace map.
    typename _TyUriAndPrefixMap::value_type const & rstrPrefix = _rcontUriPrefix.RStrAddPrefix( typename _TyUriAndPrefixMap::value_type( _TyMarkupTraits::s_kszXmlPrefix, StaticStringLen( _TyMarkupTraits::s_kszXmlPrefix ) ) );
    typename _TyUriAndPrefixMap::value_type const & rstrUri = _rcontUriPrefix.RStrAddUri( typename _TyUriAndPrefixMap::value_type( _TyMarkupTraits::s_kszXmlUri, StaticStringLen( _TyMarkupTraits::s_kszXmlUri ) ) );
    pair< typename _TyNamespaceMap::iterator, bool > pib = m_mapNamespaces.emplace( std::piecewise_construct, std::forward_as_tuple(rstrPrefix), std::forward_as_tuple() );
    pib.first->second.first = &rstrPrefix;
    pib.first->second.second.push( &rstrUri );
  }
  _TyNamespaceMap m_mapNamespaces;
};

__XMLP_END_NAMESPACE
