#pragma once

// xml_transport.h
// Declare XML output transport classes.
// dbien
// 17FEB2021

#include "xml_types.h"
#include "_fdobjs.h"

__XMLP_BEGIN_NAMESPACE

// xml_write_transport_file:
// Write XML to a file.
// REVIEW<dbien>: Purposefully not trying to optimize this yet. Will look into it later.
template < class t_TyChar, class t_TyFSwitchEndian >
class xml_write_transport_file
{
  typedef xml_write_transport_file _TyThis;
public:
  typedef t_TyChar _TyChar;
  typedef t_TyFSwitchEndian _TyFSwitchEndian;

  xml_write_transport_file( FileObj & _rfoFile, bool _fWriteBOM )
    : m_foFile( std::move( _rfoFile ) )
  {
    VerifyThrow( m_foFile.FIsOpen() );
    if ( _fWriteBOM )
      WriteBOM< _TyChar, _TyFSwitchEndian >( m_foFile.HFileGet() );
    // Leave the file right where it is.
  }
  // Write any character type to the transport - it will do the appropriate translation and it needn't even buffer anything...
  template < class t_TyCharWrite >
  void Write( const t_TyCharWrite * _pcBegin, const t_TyCharWrite * _pcEnd )
  {
    // We send to the conversion template which will call back into this:
    VerifyThrow( FWriteUTFStream< _TyChar >( _pcBegin, _pcEnd - _pcBegin, *this ) );
  }
  // We want these calls to be noexcept.
  bool FWrite( const _TyChar * _pcBuf, size_t _nch ) noexcept
  {
    int iResult = FileWrite( m_foFile.HFileGet(), _pcBuf, _nch * sizeof ( _TyChar ) );
    return !iResult;
  }
  static constexpr EFileCharacterEncoding GetEncoding()
  {
    return GetCharacterEncoding< _TyChar, _TyFSwitchEndian >();
  }
protected:
  FileObj m_foFile;
};

// xml_write_transport_mapped:
// Write XML to a mapped file.
template < class t_TyChar, class t_TyFSwitchEndian >
class xml_write_transport_mapped
{
  typedef xml_write_transport_mapped _TyThis;
public:
  typedef t_TyChar _TyChar;
  typedef t_TyFSwitchEndian _TyFSwitchEndian;
  static const size_t s_knGrowFileByBytes = 16384 * sizeof( _TyChar ); // As we write the file grow it in this increment.

  xml_write_transport_mapped( FileObj & _rfoFile, bool _fWriteBOM )
    : m_foFile( std::move( _rfoFile ) )
  {
    VerifyThrow( m_foFile.FIsOpen() );
    // Write the BOM and then initialize the file.
    if ( _fWriteBOM )
      WriteBOM< _TyChar, _TyFSwitchEndian >( m_foFile.HFileGet() );
    // Leave the file right where it is and map it here - the mapping will take care of correctly offsetting everything.
	  m_nbyMapAtPosition = (size_t)NFileSeekAndThrow(_rfoFile.HFileGet(), 0, vkSeekCur);
    size_t stMapBytes = s_knGrowFileByBytes;
    int iResult = FileSetSize( m_foFile.HFileGet(), m_nbyMapAtPosition + stMapBytes ); // Set initial size.
    if ( !!iResult )
      THROWNAMEDEXCEPTIONERRNO(GetLastErrNo(), "FileSetSize() failed.");
    size_t stSizeMapping;
    size_t stResultMapAtPosition = m_nbyMapAtPosition; // The result will be m_nbyMapAtPosition % PageSize().
    m_fmoMap.SetHMMFile( MapReadWriteHandle( m_foFile.HFileGet(), &stSizeMapping, &stResultMapAtPosition ) );
    if ( !m_fmoMap.FIsOpen() )
      THROWNAMEDEXCEPTIONERRNO(GetLastErrNo(), "MapReadWriteHandle() failed.");
    Assert( ( stSizeMapping - stResultMapAtPosition ) == stMapBytes );
    m_pcCur = m_pcBase = (_TyChar*)m_fmoMap.Pby( stResultMapAtPosition );
    m_pcEnd = m_pcCur + ( stMapBytes / sizeof( _TyChar) );
  }

  ~xml_write_transport_mapped()
  {
    bool fInUnwinding = !!std::uncaught_exceptions();
    _Close( !fInUnwinding ); // Only throw on error from close if we are not currently unwinding.
  }

  // Write any character type to the transport - it will do the appropriate translation and it needn't even buffer anything...
  template < class t_TyCharWrite >
  void Write( const t_TyCharWrite * _pcBegin, const t_TyCharWrite * _pcEnd )
  {
    // We send to the conversion template which will call back into this:
    VerifyThrow( FWriteUTFStream< _TyChar >( _pcBegin, _pcEnd - _pcBegin, *this ) );
  }
  // We want these calls to be noexcept.
  bool FWrite( const _TyChar * _pcBuf, size_t _nch ) noexcept
  {
    int iResult = _CheckGrowMapNoThrow( _nch );
    if ( !iResult )
    {
      memcpy( m_pcCur, _pcBuf, _nch * sizeof( _TyChar ) );
      m_pcCur += _nch;
    }
    return !iResult;
  }
protected:
  // Don't throw - return 0 on success, non-zero on failure.
  int _CheckGrowMapNoThrow( size_t _charsByAtLeast ) noexcept
  {
    try
    {
      if ( size_t( m_pcEnd - m_pcCur ) < _charsByAtLeast )
        _GrowMap( _charsByAtLeast );
    }
    catch( std::exception const & _rexc )
    {
      n_SysLog::Log( eslmtError, "Caught exception [%s]", _rexc.what() );
      return -1;
    }
  }
  void _GrowMap( size_t _charsByAtLeast )
  {
    VerifyThrow( m_foFile.FIsOpen() && m_fmoMap.FIsOpen() );
    // unmap, grow file, remap. That was easy!
    size_t nbyGrowAtLeast = _charsByAtLeast * sizeof( _TyChar ); // scale from chars to bytes.
    size_t nbyGrowBy = (((nbyGrowAtLeast - 1) / s_knGrowFileByBytes) + 1) * s_knGrowFileByBytes;
    size_t nbyOldMapping = ( m_pcEnd - m_pcBase ) * sizeof( _TyChar );
    (void)m_fmoMap.Close();
    int iFileSetSize = FileSetSize( m_foFile.HFileGet(), m_nbyMapAtPosition + nbyOldMapping + nbyGrowBy );
    if (-1 == iFileSetSize)
      THROWNAMEDEXCEPTIONERRNO( GetLastErrNo(), "FileSetSize() failed." );
    size_t stSizeMapping;
    size_t stResultMapAtPosition = m_nbyMapAtPosition; // The result will be m_nbyMapAtPosition % PageSize().
    m_fmoMap.SetHMMFile( MapReadWriteHandle( m_foFile.HFileGet(), &stSizeMapping, &stResultMapAtPosition ) );
    if ( !m_fmoMap.FIsOpen() )
      THROWNAMEDEXCEPTIONERRNO( GetLastErrNo(), "Remapping failed." );
    Assert( ( stSizeMapping - stResultMapAtPosition ) == nbyOldMapping + nbyGrowBy );
    size_t nchOffsetCur = m_pcCur - m_pcBase;
    m_pcBase = (_TyChar*)m_fmoMap.Pby( stResultMapAtPosition );
    m_pcEnd = m_pcBase + ( stSizeMapping / sizeof( _TyChar) );
    m_pcCur = m_pcBase + nchOffsetCur;
  }
  // Allow throwing on error because closing actually does something and we need to know if it succeeds.
  void _Close( bool _fThrowOnError ) noexcept(false)
  {
    if ( !m_fmoMap.FIsOpen() )
    {
      // Not much to do cuz we don't know about the size of the mapping, etc. Just close the file.
      m_foFile.Close();
      return 0;
    }
    int iCloseFileMapping = m_fmoMap.Close();
    vtyErrNo errCloseFileMapping = !iCloseFileMapping ? vkerrNullErrNo : GetLastErrNo();
    vtyErrNo errTruncate = vkerrNullErrNo;
    vtyErrNo errCloseFile = vkerrNullErrNo;
    if ( m_foFile.FIsOpen() )
    {
      // We need to truncate the file to m_pcCur - m_pcBase bytes.
      size_t nbySizeTruncate = ( m_pcCur - m_pcBase ) * sizeof( _TyChar );
      int iTruncate = FileSetSize(m_foFile.HFileGet(), m_nbyMapAtPosition + nbySizeTruncate );
      errTruncate = !iTruncate ? vkerrNullErrNo : GetLastErrNo();
      int iClose = m_foFile.Close();
      errCloseFile = !iClose ? vkerrNullErrNo : GetLastErrNo();
    }
    vtyErrNo errFirst;
    unsigned nError;
    if ( ( (nError = 1), ( vkerrNullErrNo != ( errFirst = errTruncate ) ) ) || 
         ( (nError = 2), ( vkerrNullErrNo != ( errFirst = errCloseFileMapping ) ) ) ||
         ( (nError = 3), ( vkerrNullErrNo != ( errFirst = errCloseFile ) ) ) )
    {
      // Ensure that the errno is represented in the last error:
      SetLastErrNo( errFirst );
      if ( _fThrowOnError )
      {
        const char * pcThrow;
        switch( nError )
        {
          case 1: pcThrow = "Error encountered truncating."; break;
          case 2: pcThrow = "Error encountered closing file mapping."; break;
          case 3: pcThrow = "Error encountered closing file."; break;
          default: pcThrow = "Wh-what?!"; break;
        }
        THROWNAMEDEXCEPTIONERRNO( errFirst, pcThrow );
      }
    }
    return errFirst == vkerrNullErrNo ? 0 : -1;
  }

  FileObj m_foFile;
  FileMappingObj m_fmoMap;
  size_t m_nbyMapAtPosition{0}; // We save this because we may be growing the file.
  _TyChar * m_pcBase{nullptr};
  _TyChar * m_pcCur{nullptr};
  _TyChar * m_pcEnd{nullptr};
};

#if 0 // later. easy.
// xml_write_transport_memstream:
// Write XML to a memstream.
template < class t_TyChar, class t_TyFSwitchEndian >
class xml_write_transport_memstream
{
  typedef xml_write_transport_memstream _TyThis;
public:
  typedef MemStream< size_t, false > _TyMemStream;


protected:
  _TyMemStream m_msMemStream;
};
#endif //0

// xml_write_transport_var:
// Variant transport. Not sure if this is necessary.
template < class ... T_TysTransports >
class xml_write_transport_var
{
  typedef xml_write_transport_var _TyThis;
public:

protected:
};

__XMLP_END_NAMESPACE
