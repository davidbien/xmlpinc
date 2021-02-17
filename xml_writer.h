#pragma once

#include <codecvt>

// xml_writer.h
// XML writer. Uses the defined productions to match supplied strings to ensure correctness.
// dbien
// 13FEB2021

// Design goals:
// 1) As with the xml_read_cursor we want to only store information on tags we *currently* know about - i.e. the ones
//    in the write(read) context list.
// 2) We will move the tag and content tags into this object and write them and then just copy the tag name and any 
//    namespace management objects found and leave copies in xml_read_cursor's copy so that normal operation occurs.
// 3) My thought is that this can pretty much be just modelled on how the xml_tag stuff peruses the xml_read_cursor.
//    But we aren't constructing a tree and just need to hold onto the current context list to the root.
// 4) The tags should be able to come either:
//    a) come straight from the xml_read_cursor(_var). In this case there is no need for validation of the data.
//       We assume that if the data is in the form of _l_data positions (or vtyDataPosition) in the _l_value then
//       it came from the parser and thus there is no need for output validation.
//    b) If the data isn't in _l_data or vtyDataPosition form in the _l_value - i.e. it has been written over with
//       some type of string then this data would need to be validated before converting to the output format. Note that
//       this data may need to be converted to be validated by the character's state machine.
//    b) come from the BeginTag() interface which returns an object that can be used to add attributes, etc, from
//       various sources, etc. as well as close the tag.
//    c) we should be able to "splice" in data from an xml_read_cursor(_var) and we want to be able to splice in
//       data from different character encodings from the character encoding we are outputing.
// 5) We want to output any encoding from any other encoding: The output encoding of the writer shouldn't depend
//    at all on the encoding of the source.
//    a) This will involve translating the source tokens in-place to a different character type which, given how we have done
//       things should be pretty easy. The positions within the sub-objects will be exactly the same they are just indexing
//       a different character type. That's not true at all, actually due to the presence of variable length characters.
//       We'll have to think about how to do this.         

__XMLP_BEGIN_NAMESPACE

// xml_write_transport_file:
// Write XML to a file.
template < class t_TyChar, class t_TyFSwitchEndian >
class xml_write_transport_file
{
  typedef xml_write_transport_file _TyThis;
public:


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


protected:
  FileObj m_foFile;
  FileMappedObj m_fmoMap;
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

// xml_write_tag:
// This is a wrapper which stores a references to a tag that has been started in the associated xml_writer<> object.
// When the lifetime of this object ends, the tag is ended, so act accordingly. The nice thing is that even in retail
//  we will throw if you do something wrong - like end a tag early.
template < class t_TyXmlTransportOut >
class xml_write_tag
{
  typedef xml_write_tag _TyThis;
public:
  typedef xml_writer< t_TyXmlTransportOut > _TyWriter;

protected:
  _TyWriter * m_pxwWriter{nullptr};
  _TyWriterContext * m_pwcxtContext{nullptr}; // The context in the context stack to which this xml_write_tag corresponds.
};

// TGetPrefixUri (prefix,URI).
template < class t_TyChar >
using TGetPrefixUri = std::pair< basic_string< t_TyChar >, basic_string< t_TyChar > >;

// xml_writer:
// Writes an XML file stream through an XML write transport.
// The encoding type of t_TyXmlTransportOut is independent of t_TyXmlTraits but we must specify it here to allow splicing, etc.,
//  into a file because we can't mix encoding within a single file so no reason to try to be more flexible than that.
template < class t_TyXmlTransportOut >
class xml_writer
{
  typedef xml_writer _TyThis;
public:
  typedef t_TyXmlTransportOut _TyXmlTransportOut;
  typedef typename _TyXmlTransportOut::_TyChar _TyChar;
  typedef xml_write_tag< _TyChar > _TyXmlWriteTag;
  // We need a user object in order to be able to create tokens.
  typedef xml_user_obj< _TyChar, false > _TyUserObj;
  // We need to have our own local namespace map which means we need the full on document context:
  typedef _xml_document_context< _TyUserObj > _TyXmlDocumentContext;
  typedef typename _xml_namespace_map_traits< _TyChar >::_TyNamespaceMap _TyNamespaceMap;

  // Open the given file in the given encoding.
  void OpenFile( const char * _pszFileName )
  {

  }
  template < template < class ... > class t_tempTyXmlTransportOut >
  void OpenFileVar( const char * _pszFileName )
  {

  }


  // Write starting at the current tag of the read cursor.
  // If this is the XMLDecl tag then the XMLDecl tag then we either:
  // 1) Write the XMLDecl tag is nothing at all has been written to this writer yet.
  // 2) Skip the XMLDecl tag if we are splicing into the middle of an existing - a writer state for which the XMLDecl tag would have already written.
  template < class t_TyReadCursor >
  void WriteFromReadCursor( t_TyReadCursor & _rxrc )
  {
    // A few things:
    // 1) Figure out how to write the XMLDecl appropriately - or how to skip it if it is present and we don't want to write it.
    // 2) 
  }

  // Start the tag. If _ppvNamespace is non-null:
  // 1) Check to see if the (prefix,uri) happens to be current. If it isn't:
  //    a) If prefix isn't the empty string then it will be added to the front of _pszTagName. 
  //    b) The appropriate xmlns= will be added as an attribute in this case.
  // 2) If the prefix is current then we will still add it to the front of _pszTagName but we won't
  //    add the xmlns declaration.
  // 3) We don't validate the contents of the name or any prefix now we do that on output.  
  // 4) For convenience's sake the caller may add a prefix onto the beginning of _pszTagName and that
  //    prefix will be used if it is a current namespace prefix. If it isn't a current namespace prefix
  //    then an exception will be thrown.
  template < class t_TyChar >
  _TyXmlWriteTag StartTag( const t_TyChar * _pszTagName, size_t _stLenTag = (numeric_limits<size_t>::max)(), TGetPrefixUri< t_TyChar > const * _ppuNamespace = nullptr )
  {

  }
#if 0
  // Ends the current tag. If _ptokEnd is passed it needs to match the top of the write stack. If not an exception will be thrown.
  // The safest manner of operation is to pass _ptokEnd bask to check - but I don't see a reason to *require* it.
  void EndTag( _TyXmlWriteTag * _ptokEnd = nullptr )
  {

  }
#endif //0

  // Write a non-tag token to the output transport at the current position.
  // Validation is performed - e.g. only whitespace chardata is allowed before the first tag.
  // The token is passed the production to which it correspond's state machine to ensure that it matches.
  // If a non-match is found that isn't correctable with a CharRef, etc. then we will throw an error.
  template < class t_tyXmlToken >
  void WriteToken( t_tyXmlToken const & _rtok )
  {

  }

protected:
  void _InitTransport()
  {
  }
  _TyXmlDocumentContext m_xdcxtDocumentContext;
  _TyNamespaceMap m_mapNamespaces; // We maintain this as we go.
// options:
  bool m_fWriteBOM{true}; // Write a BOM because that's just a nice thing to do.
  bool m_fWriteXMLDecl{true}; // Whether to write an XMLDecl. This is honored in all scenarios.
// state:
  
  bool m_fWroteFirstTag{false};
};

// xml_writer_var:
// Variant writer. This supports multiple character types (potentially). 
// Once again not sure that this is necessary, but potentially just as a receiver for xml_cursor_var - instant adaptor.
template <  template < class ... > class t_tempTyTransport, class t_TyTp2DCharPack >
class xml_writer_var
{
public:
};




__XMLP_END_NAMESPACE
