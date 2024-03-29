#pragma once

//          Copyright David Lawrence Bien 1997 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt).

// xml_util.h
// Utility methods.
// dbien 
// 02MAR2021

#include "xml_types.h"
#include "_strutil.h"

__XMLP_BEGIN_NAMESPACE

// If the same prefix declares the same URI then we will will ignore it because I cannot see an interpretation of this in the W3C XML Namespace Specification.
// We will throw an error for any duplicate prefix declarations if _fStrict is on because other parsers may barf on such declarations.
template < class t_TyLexToken >
void CheckDuplicateTokenAttrs( bool _fStrict, const t_TyLexToken & _rltok, bool _fSkipIfAllTypedData = false )
{
  typedef t_TyLexToken _TyLexToken;
  typedef typename _TyLexToken::_TyValue _TyLexValue;
  typedef typename _TyLexValue::_TyChar _TyChar;
  typedef basic_string_view< _TyChar > _TyStrView;
  typedef xml_namespace_value_wrap< _TyChar > _TyXmlNamespaceValueWrap;
  // The default init allocator allows us to resize without initializing the elements...
  // Then we don't have to interact with the vector at all as we fill it...
  typedef const _TyLexValue * _TyPtrLexValue;
  typedef std::vector< _TyPtrLexValue, default_init_allocator< _TyPtrLexValue > > _TyVectorPtrs;
  _TyVectorPtrs rgPointers;
  static constexpr size_t knptrMaxAllocaSize = vknbyMaxAllocaSize / sizeof( _TyLexValue* );
  const _TyLexValue & rrgAttrs = _rltok[vknAttributesIdx];
  size_t nAttrs = rrgAttrs.GetSize();
  const _TyLexValue ** ppvalBegin;
  const _TyLexValue ** ppvalEnd;
  if ( knptrMaxAllocaSize < nAttrs )
  {
    rgPointers.resize( nAttrs );
    ppvalBegin = &*rgPointers.begin();
    ppvalEnd = ppvalBegin + nAttrs;
  }
  else
  {
    ppvalBegin = (const _TyLexValue **)alloca( nAttrs * sizeof( const _TyLexValue * ) );
    ppvalEnd = ppvalBegin + nAttrs;
  }
  // Fill up the pointers - use ApplyContiguous to do it as quickly as possible.
  {//B
    bool fIsAllTypedData = _fSkipIfAllTypedData;
    const _TyLexValue ** ppvalCur = ppvalBegin;
    rrgAttrs.GetValueArray().ApplyContiguous( 0, nAttrs,
      [&fIsAllTypedData,&ppvalCur]( const _TyLexValue * _pvBegin, const _TyLexValue * _pvEnd )
      {
        for ( ; _pvBegin != _pvEnd; )
        {
          if ( fIsAllTypedData )
            fIsAllTypedData = (*_pvBegin)[vknNameIdx].FHasTypedData();
          *ppvalCur++ = _pvBegin++;
        }
      }
    );
    Assert( ppvalCur == ppvalEnd );
    if ( fIsAllTypedData )
      return; // No need to validate.
  }//EB
  bool fLessThan = true;
  auto lambdaCompareAttrPointers =  
    [&_rltok,&fLessThan]( const _TyLexValue *& _rlpt, const _TyLexValue *& _rrpt ) -> bool
    {
      // First compare the names:
      _TyStrView svNameLeft;
      (*_rlpt)[vknNameIdx].KGetStringView( _rltok, svNameLeft );
      _TyStrView svNameRight;
      (*_rrpt)[vknNameIdx].KGetStringView( _rltok, svNameRight );
      strong_ordering iComp = svNameLeft <=> svNameRight;
      if ( 0 == iComp )
      {
        _TyLexValue const & rlvalNS = (*_rlpt)[vknNamespaceIdx];
        _TyLexValue const & rrvalNS = (*_rrpt)[vknNamespaceIdx];
        bool fIsNotBoolLeft = !rlvalNS.template FIsA< bool >();
        bool fIsNotBoolRight = !rrvalNS.template FIsA< bool >();
        iComp = (int)fIsNotBoolLeft <=> (int)fIsNotBoolRight;
        if ( ( 0 == iComp ) && fIsNotBoolLeft )
        {
          const _TyXmlNamespaceValueWrap & rxnvwLeft = rlvalNS.template GetVal< _TyXmlNamespaceValueWrap >();
          const _TyXmlNamespaceValueWrap & rxnvwRight = rrvalNS.template GetVal< _TyXmlNamespaceValueWrap >();
          iComp = rxnvwLeft.ICompareForUniqueAttr( rxnvwRight );
        }
      }
      return fLessThan ? ( iComp < 0 ) : ( iComp == 0 );
    };

  sort( ppvalBegin, ppvalEnd, lambdaCompareAttrPointers );
  // Check for adjacent equal object. We will report all violations, cuz duh...
  bool fAnyDuplicateAttrNames = false;
  fLessThan = false; // cause the lambda to return equal, not less than.
  for ( const _TyLexValue ** ppDupeCur = adjacent_find( ppvalBegin, ppvalEnd, lambdaCompareAttrPointers );
        ( ppvalEnd != ppDupeCur );
        ppDupeCur = adjacent_find( ppDupeCur+1, ppvalEnd, lambdaCompareAttrPointers ) )
  {
    // If the same prefix declares the same URI then we will will ignore it because I cannot see an interpretation of this in the W3C XML Namespace Specification.
    // We will throw an error for any duplicate prefix declarations if _fStrict is on because other parsers may barf on such declarations.
    const _TyLexValue & rvattrCur = **ppDupeCur;
    const _TyLexValue & rvnsCur = rvattrCur[vknNamespaceIdx];
    _TyStrView svAttrName;
    rvattrCur[vknNameIdx].KGetStringView( _rltok, svAttrName );
    if ( rvnsCur.template FIsA<bool>() )
    {
      // no-namespace attribute:
      fAnyDuplicateAttrNames = true;
      LOGSYSLOG( eslmtError, "Duplicate attribute name found [%s].", StrConvertString< char >( svAttrName ).c_str() );
    }
    else
    {
      Assert( rvnsCur.template FIsA<_TyXmlNamespaceValueWrap>() );
      _TyXmlNamespaceValueWrap const & rxnvwCur = rvnsCur.template GetVal< _TyXmlNamespaceValueWrap >();
      if ( rxnvwCur.FIsNamespaceDeclaration() )
      {
        const _TyLexValue & rvattrNext = *ppDupeCur[1];
        const _TyLexValue & rvnsNext = rvattrNext[vknNamespaceIdx];
        if ( _fStrict || ( 0 != rxnvwCur.ICompare( rvnsNext.template GetVal< _TyXmlNamespaceValueWrap >() ) ) )
        {
          fAnyDuplicateAttrNames = true;
          LOGSYSLOG( eslmtError, "Duplicate namespace prefix name declared[%s].", StrConvertString< char >( svAttrName ).c_str() );
        }
      }
      else
      {
        // Attr same name with same URI declared:
        fAnyDuplicateAttrNames = true;
        LOGSYSLOG( eslmtError, "Same named attr[%s] has same URI reference in namespace[%s].", StrConvertString< char >( svAttrName ).c_str(), rxnvwCur.RStringUri().c_str() );
      }
    }
  }
  VerifyThrowSz( !fAnyDuplicateAttrNames, "Found dupicate (qualified) attribute names or namespace prefix declarations.");
}

__XMLP_END_NAMESPACE
