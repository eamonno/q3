//-----------------------------------------------------------------------------
// File: ziptypes.h
//
// Data types for zip archives. Note that only normal zip archives are supported
// here. Zip64 archives are not supported. Most of the information here is based
// on information supplied in the file appnote.txt which is available from the
// pkware websute at ???. 
//    The data structures below are designed for accessing zip archives where
// the entire file has either been loaded into memory or (preferably) memory
// mapped. The constructors and destructors are all private making it impossible
// to either create or delete any of these objects. They must simply be made to
// point to the correct region of memory.
//    The data structures for the Zip64 files are not present here and so Zip64
// archives are not currently supported. Neither are disk spanning archives
// supported.
//
// A zip archive contains the following sections:
//    [zip_entry_t      ]+
//    [zip_dir_entry_t  ]+
//    [zip_digital_sig_t]
//    {zip64 end of central directory record}
//    {zip64 end of central directory locator}
//    [zip_dir_t        ]
// For more information on the individual sections and their formats see their
// definitions below
//-----------------------------------------------------------------------------

#ifndef ZIPTYPES_H
#define ZIPTYPES_H

#include "types.h"

// The various signitures used to validate parts of the archive
#define	zip_file_sig				(0x04034b50)
#define zip_dir_entry_sig			(0x02014b50)
#define zip_digital_sig_sig			(0x05054b50)
#define zip_entry_sig				(0x06054b50)

// The magic number used for the 32 bit crc
#define zip_crc_magic_number		(0xdebb20e3)

// General purpose bit flags. Note that the zip_flag_implode* flags only matter
// if the compression method used was method 6 (imploding). Since that method
// is not supported here the values are included just for completeness. The
// zip_flag_enhanced_deflate is reserved for use with method 8 for enhanced
// deflating
#define zip_flag_encrypted			(0x01)
#define zip_flag_implode8k			(0x02)
#define zip_flag_implode3tree		(0x04)
#define zip_flag_descriptor			(0x08)
#define zip_flag_enhanced_deflate	(0x10)
#define zip_flag_patched_data		(0x20)

// General purpose bit flag masks. These are only important if the compression
// method used was either method 8 (deflate) or method 9 (deflate64)
#define zip_deflate_mask			(0x6)
#define zip_deflate_normal			(0x0)
#define zip_deflate_max				(0x2)
#define zip_deflate_fast			(0x4)
#define zip_deflate_super_fast		(0x6)

// The various compression methods. Only zip_method_deflated is supported
enum {
	zip_method_stored,		// The file is stored (no compression)
	zip_method_shrunk,		// The file is Shrunk
	zip_method_reduced1,	// The file is Reduced with compression factor 1
	zip_method_reduced2,	// The file is Reduced with compression factor 2
	zip_method_reduced3,	// The file is Reduced with compression factor 3
	zip_method_reduced4,	// The file is Reduced with compression factor 4
	zip_method_imploded,	// The file is Imploded
	zip_method_reserved,	// Reserved for Tokenizing compression algorithm
	zip_method_deflated,	// The file is Deflated
	zip_method_deflate64,	// Enhanced Deflating using Deflate64(tm PKWARE)
	zip_method_date_implode,// PKWARE Date Compression Library Imploding
};

// The sizes of the various structs
#define zip_descriptor_size			(sizeof(zip_descriptor_t))
#define zip_dir_entry_size			(sizeof(zip_dir_entry_t))
#define zip_dir_size				(sizeof(zip_dir_t))
#define zip_digital_sig_size		(sizeof(zip_digital_sig_size))
#define zip_entry_size				(sizeof(zip_entry_t))

#pragma pack ( push, 1 )

class zip_descriptor_t {
	// The data descriptor for a single file in the archive. If bit 3 of the flags
	// field of a zip_file_t (zip_flag_descriptor) is set then the file has a data
	// descriptor following the compressed data for the file. This is only present
	// if the file had to be created sequentially. If this was so then the functions
	// for accessing these fields in zip_file_t will retrun the values from the
	// descriptor instead
public:
	uint	crc()				{ return z_crc; }
	uint	compressed_size()	{ return z_compressed_size; }
	uint	uncompressed_size()	{ return z_uncompressed_size; }

private:
	zip_descriptor_t()  {}	// Never create these
	~zip_descriptor_t() {}	// Never delete these

	uint	z_crc;
	uint	z_compressed_size;
	uint	z_uncompressed_size;
};

class zip_entry_t {
	// The actual files within a zip archive are stored sequentially from the start
	// of the archive. Each entry consists of the following:
	//    [entry header                         ]
	//    [entry name (name_length chars)       ]
	//    [entry extra data (extra_length bytes)]
	//    [entry data (compressed_size bytes)   ]
	//    {entry descriptor (not always present)}
	// The entry descriptor is only present if the zip_flag_descriptor flag is set
	// in the flags field. If this is set then the crc, compressed_size and
	// uncompressed_size methods below return the value from the descriptor 
public:
	uint	signature()			{ return z_signature; }
	ushort	version_to_extract(){ return z_version_to_extract; }
	ushort	flags()				{ return z_flags; }
	ushort	method()			{ return z_method; }
	ushort	time()				{ return z_time; }
	ushort	date()				{ return z_date; }
	uint	crc()				{ return (z_flags & zip_flag_descriptor) ? descriptor()->crc() : z_crc; }
	uint	compressed_size()	{ return (z_flags & zip_flag_descriptor) ? descriptor()->compressed_size() : z_compressed_size; }
	uint	uncompressed_size()	{ return (z_flags & zip_flag_descriptor) ? descriptor()->uncompressed_size() : z_uncompressed_size; }
	ushort	name_length()		{ return z_name_length; }
	ushort	extra_length()		{ return z_extra_length; }
	char*	name()				{ return reinterpret_cast<char*>(this + 1); }
	ubyte*	extra()				{ return reinterpret_cast<ubyte*>(name() + z_name_length); }
	ubyte*	compressed_data()	{ return extra() + z_extra_length; }

	bool	is_valid()
		{ return (z_signature == zip_file_sig && (z_method == zip_method_stored || z_method == zip_method_deflated)); }
	size_t	extent()
		{ return sizeof(zip_entry_t) + z_compressed_size + z_name_length + z_extra_length + (z_flags & zip_flag_descriptor ? sizeof(zip_descriptor_t) : 0); }

private:
	zip_entry_t();	// Never create these
	~zip_entry_t();	// Never delete these

	zip_descriptor_t* descriptor()
	{ return reinterpret_cast<zip_descriptor_t*>(name() + z_name_length + z_extra_length + z_compressed_size); }

	uint	z_signature;
	ushort	z_version_to_extract;
	ushort	z_flags;
	ushort	z_method;
	ushort	z_time;
	ushort	z_date;
	uint	z_crc;
	uint	z_compressed_size;
	uint	z_uncompressed_size;
	ushort	z_name_length;
	ushort	z_extra_length;
};

class zip_dir_entry_t {
	// After the files themselves zip archives contain a central directory. This
	// structure stores additional information about the various files in the zip
	// archive as well as duplicating all of the fields of the files themselves.
	// Since this information is stored in a contiguous lump in the file it is much
	// quicker to use this for listing contents etc than to use the individual file
	// entries. The file entries themselves are accessed through these entries
	// A zip dir entry has the following structure:
	//   [dir entry header              ]
	//   [name (name_length bytes)      ]
	//   [extra (extra_length bytes)    ]
	//   [comment (comment_length bytes)]
public:
	uint	signature()			 { return z_signature; }
	ushort	version_made_by()	 { return z_version_made_by; }
	ushort	version_to_extract() { return z_version_to_extract; }
	ushort	flags()				 { return z_flags; }
	ushort	method()			 { return z_method; }
	ushort	time()				 { return z_time; }
	ushort	date()				 { return z_date; }
	uint	crc()				 { return z_crc; }
	uint	compressed_size()	 { return z_compressed_size; }
	uint	uncompressed_size()	 { return z_uncompressed_size; }
	ushort	name_length()		 { return z_name_length; }
	ushort	extra_length()		 { return z_extra_length; }
	ushort	comment_length()	 { return z_comment_length; }
	ushort	disk_number()		 { return z_disk_number; }
	ushort	internal_attributes(){ return z_internal_attributes; }
	uint	external_attributes(){ return z_external_attributes; }
	uint	offset()			 { return z_offset; }
	char*	name()				 { return reinterpret_cast<char*>(this + 1); }
	ubyte*	extra()				 { return reinterpret_cast<ubyte*>(name() + z_name_length); }
	char*	comment()			 { return name() + z_name_length + z_extra_length; }

	bool	is_valid()
		{ return (z_signature == zip_dir_entry_sig && (z_method == zip_method_stored || z_method == zip_method_deflated)); }
	size_t	extent()
		{ return sizeof(zip_dir_entry_t) + z_name_length + z_extra_length + z_comment_length; }

private:
	zip_dir_entry_t()  {}	// Never create these
	~zip_dir_entry_t() {}	// Never delete these
	
	uint	z_signature;
	ushort	z_version_made_by;
	ushort	z_version_to_extract;
	ushort	z_flags;
	ushort	z_method;
	ushort	z_time;
	ushort	z_date;
	uint	z_crc;
	uint	z_compressed_size;
	uint	z_uncompressed_size;
	ushort	z_name_length;
	ushort	z_extra_length;
	ushort	z_comment_length;
	ushort	z_disk_number;
	ushort	z_internal_attributes;
	uint	z_external_attributes;
	uint	z_offset;
};

class zip_digital_sig_t {
	// Digital signature for the archive. 
public:
	uint	signature()			{ return z_signature; }
	ushort	data_length()		{ return z_data_length; }
	ubyte*	data()				{ return reinterpret_cast<ubyte*>(this + 1); }

	bool	is_valid()			{ return z_signature == zip_digital_sig_sig; }
	size_t	extent()			{ return sizeof(zip_digital_sig_t) + z_data_length; }

private:
	zip_digital_sig_t()  {}	// Never create these
	~zip_digital_sig_t() {}	// Never delete these

	uint	z_signature;
	ushort	z_data_length;
};

class zip_dir_t {
	// The zip dir describes the central directory itself. This is always the last
	// field in the file and is the starting point for accessing the archive. 
public:
	uint	signature()			{ return z_signature; }
	ushort	disk_number()		{ return z_disk_number; }
	ushort	dir_start_disk()	{ return z_dir_start_disk; }
	ushort	disk_entries()		{ return z_disk_entries; }
	ushort	total_entries()		{ return z_total_entries; }
	uint	size()				{ return z_size; }
	uint	offset()			{ return z_offset; }
	ushort	comment_length()	{ return z_comment_length; }
	char*	comment()			{ return reinterpret_cast<char*>(this + 1); }

	bool	is_valid()			{ return (z_signature == zip_entry_sig && z_disk_number == 0 && z_dir_start_disk == 0); }
	size_t	extent()			{ return sizeof(zip_dir_t) + z_comment_length; }

private:
	zip_dir_t()  {}	// Never create these
	~zip_dir_t() {}	// Never delete these

	uint   z_signature;
	ushort z_disk_number;
	ushort z_dir_start_disk;
	ushort z_disk_entries;
	ushort z_total_entries;
	uint   z_size;
	uint   z_offset;
	ushort z_comment_length;
};

#pragma pack ( pop )

#endif // ZIPTYPES_H
