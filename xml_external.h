#pragma once

//          Copyright David Lawrence Bien 1997 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt).

// xml_external.h
// External entities which are files: an xml_external_entity_resolver ( xml_dtd.h ) for the documents which are files.
// dbien
// 19NOV2026

// The external subset of a DTD and its external parameter entities are read only if the parser is given a resolver -
//  xml_parser<>::SetExternalEntityResolver() - since what a document names is then read. FReadExternalEntityFile() is
//  one for the entities which are files: xml_parser<>::SetReadExternalEntityFiles().

#include <bit>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
#include "_strutil.h"
#include "xml_ns.h"
#include "xml_types.h"
#include "xml_exc.h"
#include "xml_dtd.h"

__XMLP_BEGIN_NAMESPACE

// The encoding which the text declaration [77] ( or XML declaration ) at the beginning of the bytes declares - in lower
//  case; empty if there is no declaration or it declares none. Only for bytes in an encoding which has ASCII as it is.
inline std::string _StrDeclaredEncoding( const uint8_t * _pby, size_t _nby )
{
  static const char s_kszBegin[] = "<?xml";
  const std::string_view sv( (const char *)_pby, (std::min)( _nby, size_t( 256 ) ) );
  if ( sv.substr( 0, 5 ) != s_kszBegin )
    return std::string();
  const size_t knPosEnd = sv.find( "?>" );
  size_t nPos = sv.find( "encoding" );
  if ( ( std::string_view::npos == nPos ) || ( std::string_view::npos == knPosEnd ) || ( nPos > knPosEnd ) )
    return std::string();
  nPos = sv.find_first_of( "\"'", nPos );
  if ( ( std::string_view::npos == nPos ) || ( nPos > knPosEnd ) )
    return std::string();
  const size_t knPosClose = sv.find( sv[nPos], nPos + 1 );
  if ( ( std::string_view::npos == knPosClose ) || ( knPosClose > knPosEnd ) )
    return std::string();
  std::string strEncoding( sv.substr( nPos + 1, knPosClose - nPos - 1 ) );
  for ( char & rch : strEncoding )
    if ( ( rch >= 'A' ) && ( rch <= 'Z' ) )
      rch = char( rch - 'A' + 'a' );
  return strEncoding;
}

// FDecodeExternalEntity:
// The bytes of an external entity as text in the character type of the parser - 4.3.3: each entity may be in an encoding
//  of its own. What it is in is known by its byte order mark, or by how its first characters are encoded, or by the
//  encoding which its text declaration names - and without any of those it is UTF-8.
// Supported: UTF-8, UTF-16 and UTF-32 of either byte order, ISO-8859-1 and US-ASCII.
// Throws if the bytes are in an encoding which isn't supported or are not good in the encoding which they are in.
template < class t_TyChar >
void DecodeExternalEntity( const uint8_t * _pby, size_t _nby, std::basic_string< t_TyChar > & _rstrText )
{
  _rstrText.clear();
  if ( !_nby )
    return;
  EFileCharacterEncoding efce = efceFileCharacterEncodingCount;
  size_t nbyBOM = 0;
  if ( _nby >= vknBytesBOM )
  {
    uint64_t nbyLen = _nby;
    efce = GetCharacterEncodingFromBOM( const_cast< uint8_t * >( _pby ), nbyLen );
    nbyBOM = size_t( nbyLen );
    if ( efceFileCharacterEncodingCount == efce )
    {
      nbyBOM = 0;
      efce = DetectEncodingXmlFile( const_cast< uint8_t * >( _pby ), _nby );
    }
  }
  else
  if ( ( _nby >= 3 ) && ( 0xEF == _pby[0] ) && ( 0xBB == _pby[1] ) && ( 0xBF == _pby[2] ) )
    nbyBOM = 3;
  if ( efceFileCharacterEncodingCount == efce )
    efce = efceUTF8; // Nothing says otherwise - but for what a text declaration may say below.
  _pby += nbyBOM;
  _nby -= nbyBOM;
  if ( !_nby )
    return;
  const bool kfNativeLittleEndian = ( std::endian::native == std::endian::little );
  switch( efce )
  {
    case efceUTF8:
    {
      const std::string strEncoding = _StrDeclaredEncoding( _pby, _nby );
      if ( ( strEncoding == "iso-8859-1" ) || ( strEncoding == "iso_8859-1" ) || ( strEncoding == "latin1" ) || ( strEncoding == "l1" ) )
      {
        // Each byte is the character.
        std::u32string str32( _nby, 0 );
        for ( size_t n = 0; n < _nby; ++n )
          str32[n] = char32_t( _pby[n] );
        ConvertString( _rstrText, str32 );
        break;
      }
      VerifyThrowSz( strEncoding.empty() || ( strEncoding == "utf-8" ) || ( strEncoding == "utf8" ) || ( strEncoding == "us-ascii" ) || ( strEncoding == "ascii" ),
        "The external entity is in the encoding [%s] which is not supported.", strEncoding.c_str() );
      ConvertString( _rstrText, (const char8_t *)_pby, _nby );
    }
    break;
    case efceUTF16BE:
    case efceUTF16LE:
    {
      VerifyThrowSz( !( _nby % sizeof( char16_t ) ), "The external entity is in UTF-16 and its length [%zu] is odd.", _nby );
      std::u16string str16( _nby / sizeof( char16_t ), 0 );
      memcpy( str16.data(), _pby, _nby );
      if ( ( efceUTF16LE == efce ) != kfNativeLittleEndian )
        for ( char16_t & rch : str16 )
          rch = char16_t( ( uint16_t( rch ) >> 8 ) | ( uint16_t( rch ) << 8 ) );
      ConvertString( _rstrText, str16 );
    }
    break;
    case efceUTF32BE:
    case efceUTF32LE:
    {
      VerifyThrowSz( !( _nby % sizeof( char32_t ) ), "The external entity is in UTF-32 and its length [%zu] is not a multiple of four.", _nby );
      std::u32string str32( _nby / sizeof( char32_t ), 0 );
      memcpy( str32.data(), _pby, _nby );
      if ( ( efceUTF32LE == efce ) != kfNativeLittleEndian )
        for ( char32_t & rch : str32 )
        {
          const uint32_t u = uint32_t( rch );
          rch = char32_t( ( u >> 24 ) | ( ( u >> 8 ) & 0xff00 ) | ( ( u << 8 ) & 0xff0000 ) | ( u << 24 ) );
        }
      ConvertString( _rstrText, str32 );
    }
    break;
    default:
      VerifyThrowSz( false, "Unknown encoding of an external entity." );
    break;
  }
}

// FReadExternalEntityFile:
// An xml_external_entity_resolver for entities which are files: the system identifier is the path of the file - or a
//  "file:" URI - and if it is relative it is relative to the directory of the base, which is the path of the file of the
//  entity that declares it ( of the document for the external subset ). The location which is given back is the path
//  of the file.
// An entity with any other kind of system identifier - "http:" for one - or whose file isn't there is not read.
template < class t_TyChar >
bool FReadExternalEntityFile( xml_dtd_external_id< t_TyChar > const & _rexternalId, std::basic_string_view< t_TyChar > _svBase,
                              std::basic_string< t_TyChar > & _rstrText, std::basic_string< t_TyChar > & _rstrLocation )
{
  if ( !_rexternalId.m_optstrSystem.has_value() || _rexternalId.m_optstrSystem->empty() )
    return false;
  std::u8string strSystem;
  ConvertString( strSystem, *_rexternalId.m_optstrSystem );
  // A scheme? ( Not a single letter: that is a drive. )
  const size_t knPosColon = strSystem.find( u8':' );
  if ( ( std::u8string::npos != knPosColon ) && ( knPosColon > 1 ) )
  {
    bool fScheme = true;
    for ( size_t n = 0; fScheme && ( n < knPosColon ); ++n )
    {
      const char8_t ch = strSystem[n];
      fScheme = ( ( ch >= u8'a' ) && ( ch <= u8'z' ) ) || ( ( ch >= u8'A' ) && ( ch <= u8'Z' ) ) ||
                ( !!n && ( ( ( ch >= u8'0' ) && ( ch <= u8'9' ) ) || ( u8'+' == ch ) || ( u8'-' == ch ) || ( u8'.' == ch ) ) );
    }
    if ( fScheme )
    {
      std::u8string strScheme = strSystem.substr( 0, knPosColon );
      for ( char8_t & rch : strScheme )
        if ( ( rch >= u8'A' ) && ( rch <= u8'Z' ) )
          rch = char8_t( rch - u8'A' + u8'a' );
      if ( strScheme != u8"file" )
        return false;
      strSystem.erase( 0, knPosColon + 1 );
      if ( ( strSystem.length() >= 2 ) && ( u8'/' == strSystem[0] ) && ( u8'/' == strSystem[1] ) )
        strSystem.erase( 0, 2 );
      // "file:///C:/dir/file": the path is what follows the third slash.
      if ( ( strSystem.length() >= 3 ) && ( u8'/' == strSystem[0] ) && ( u8':' == strSystem[2] ) )
        strSystem.erase( 0, 1 );
    }
  }
  std::filesystem::path pathEntity( strSystem );
  if ( pathEntity.is_relative() && !_svBase.empty() )
  {
    std::u8string strBase;
    ConvertString( strBase, _svBase );
    pathEntity = std::filesystem::path( strBase ).parent_path() / pathEntity;
  }
  std::error_code ec;
  if ( !std::filesystem::is_regular_file( pathEntity, ec ) )
    return false;
  std::ifstream ifs( pathEntity, std::ios::binary );
  if ( !ifs )
    return false;
  const std::string strBytes( ( std::istreambuf_iterator< char >( ifs ) ), std::istreambuf_iterator< char >() );
  DecodeExternalEntity( (const uint8_t *)strBytes.data(), strBytes.size(), _rstrText );
  const std::u8string strLocation = pathEntity.lexically_normal().generic_u8string();
  _rstrLocation.clear();
  if ( !strLocation.empty() )
    ConvertString( _rstrLocation, strLocation );
  return true;
}

__XMLP_END_NAMESPACE
