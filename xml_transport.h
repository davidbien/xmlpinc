#pragma once

// xml_transport.h
// Declare XML output transport classes.
// dbien
// 17FEB2021

__XMLP_BEGIN_NAMESPACE

// xml_write_transport_file:
// Write XML to a file.
template < class t_TyChar, class t_TyFSwitchEndian >
class xml_write_transport_file
{
  typedef xml_write_transport_file _TyThis;
public:
  xml_write_transport_file( FileObj & _rfoFile, bool _fWriteBOM )
    : m_foFile( std::move( _rfoFile ) )
  {
    VerifyThrow( m_foFile.FIsOpen() );
    if ( _fWriteBOM )
      WriteBOM< _TyChar, _TyFSwitchEndian >( m_foFile.HFileGet() );
    // Leave the file right where it is.
  }

  // Write any character type to the transport - it will do the appropriate translation and it needn't even buffer anything...
  template < class t_TyChar >
  void Write( t_TyChar )

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
  static size_t s_knGrowFileByBytes = 16384 * sizeof( _TyChar ); // As we write the file grow it in this increment.

  xml_write_transport_file( FileObj & _rfoFile, bool _fWriteBOM )
    : m_foFile( std::move( _rfoFile ) )
  {
    VerifyThrow( m_foFile.FIsOpen() );
    // Write the BOM and then initialize the file.
    if ( _fWriteBOM )
      WriteBOM< _TyChar, _TyFSwitchEndian >( m_foFile.HFileGet() );
    // Leave the file right where it is and map it here - the mapping will take care of correctly offsetting everything.
	  size_t stMapAtPosition = (size_t)NFileSeekAndThrow(_rfoFile.HFileGet(), 0, vkSeekCur);
    size_t stMapBytes = s_knGrowFileByBytes;
    int iResult = FileSetSize( foFile.HFileGet(), stMapAtPosition + stMapBytes ); // Set initial size.
    if ( !!iResult )
      THROWNAMEDEXCEPTIONERRNO(GetLastErrNo(), "FileSetSize() failed.");
    size_t stSizeMapping;
    size_t stResultMapAtPosition = stMapAtPosition; // The result will be stMapAtPosition % PageSize().
    m_fmoMap.SetHMMFile( MapReadWriteHandle( m_foFile.HFileGet(), &stSizeMapping, &stResultMapAtPosition ) );
    if ( !m_fmoMap.FIsOpen() )
      THROWNAMEDEXCEPTIONERRNO(GetLastErrNo(), "MapReadWriteHandle() failed.");
    Assert( ( stSizeMapping - stResultMapAtPosition ) == stMapBytes );
    m_pcCur = m_pcBase = (_TyChar*)m_fmoMap.Pby( stResultMapAtPosition );
    m_pcEnd = m_pcCur + ( stMapBytes / sizeof( _TyChar) );
  }
  ~xml_write_transport_file()
  {
    // Must truncate file. Mustn't throw from here if we are in an unwinding - must handle any exceptions locally.
  }



protected:
  FileObj m_foFile;
  FileMappedObj m_fmoMap;
  size_t m_stMapAtPosition{0}; // We save this because we may be growing the file.
  _TyChar * m_pcBase{nullptr};
  _TyChar * m_pcCur{nullptr};
  _TyChar * m_pcEnd{nullptr};
};

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
