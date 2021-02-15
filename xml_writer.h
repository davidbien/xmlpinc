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
// Variant transport.
template < class ... T_TysTransports >
class xml_write_transport_var
{
  typedef xml_write_transport_var _TyThis;
public:

protected:
};


// xml_writer:
// Writes an XML file stream through an XML write transport.
template < class t_TyTransport >
class xml_writer
{
  typedef xml_writer _TyThis;
public:

protected:

};

// xml_writer_var:
// Variant writer. This supports multiple character types (potentially).
template <  template < class ... > class t_tempTyTransport, class t_TyTp2DCharPack >
class xml_writer_var
{
public:
};




__XMLP_END_NAMESPACE
