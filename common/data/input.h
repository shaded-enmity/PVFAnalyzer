/**********************************************************************
          DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
                    Version 2, December 2004

 Copyright (C) 2004 Sam Hocevar <sam@hocevar.net>

 Everyone is permitted to copy and distribute verbatim or modified
 copies of this license document, and changing it is allowed as long
 as the name is changed.

            DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
   TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION

  0. You just DO WHAT THE FUCK YOU WANT TO.

**********************************************************************/
#pragma once

/**
 * @brief Raw input data structures working on bytes read from some source.
 */

#include <cassert>		// assert
#include <cstring>		// memset
#include <climits>		// ULONG_MAX
#include <cerrno>		// errno
#include <cstdlib>		// free
#include <cstdio>		// perror
#include <cstdint>

#include <fstream>
#include <iostream>
#include <vector>
#include <boost/concept_check.hpp>

#include "memory.h"

/**
 * @brief Representation of a raw data buffer
 **/
class DataSection
{
public:
	DataSection()
		: _data(0), _data_idx(0), _reloc(0), _name("unnamed")
	{
	}

	~DataSection()
	{
		if (_data) {
			free(_data);
		}
	}

	DataSection(const DataSection& orig)
		: DataSection()
	{
		RelocatedMemRegion buf = orig.getBuffer();
		if (buf.base and buf.size) {
			_data     = static_cast<uint8_t*>(realloc(_data, buf.size));
			_data_idx = buf.size;
			memcpy(_data, reinterpret_cast<uint8_t*>(buf.base), buf.size);
		}
		_name = orig.name();
	}

	/**
	 * @brief Add a single byte to the buffer.
	 *
	 * @param byte byte to add
	 * @return void
	 **/
	void addByte(uint8_t byte);

	/**
	 * @brief Add multiple bytes at once
	 *
	 * @param buf buffer containing bytes to add
	 * @param count number of bytes to add from buffer
	 * @return void
	 **/
	void addBytes(uint8_t* buf, size_t count);


	/**
	 * @brief Get underlying buffer
	 *
	 * @return RelocatedMemRegion
	 **/
	RelocatedMemRegion const getBuffer() const
	{
		return RelocatedMemRegion((Address)(_data), _data_idx, _reloc);
	}

	/**
	 * @brief Dump buffer content to stdout
	 *
	 * @return void
	 **/
	void dump();

	/**
	 * @brief Get number of bytes that are stored in the stream.
	 *
	 * @return uint32_t number of bytes
	 **/
	uint32_t bytes() { return _data_idx; }

	Address relocationAddress() { return _reloc; }
	void relocationAddress(Address a) { _reloc = a; }

	std::string const & name() const 	{ return _name; }
	void name(char const * n) 			{ _name = std::string(n); }
	void name(std::string&  n) 			{ _name = n; }

private:
	enum { DATA_INCREMENT = 1024, };

	uint8_t* _data;			// buffer ptr
	uint32_t _data_idx;		// next idx to write to
	Address  _reloc;		// relocation target address
	std::string _name;      // dbg: section name


	DataSection& operator=(DataSection &) { return *this; }

	/**
	 * @brief Make sure the available data area is big enough
	 *
	 * Make sure that the input buffer is big enough to read in at
	 * least one more chunk of input data.
	 **/
	void fit_data(size_t count = 1);
};


/**
 * @brief Generic interface of an input reader.
 *
 * An input reader reads input from a source and streams the bytes
 * found at the source into a set of DataSection objects.
 **/
class InputReader
{
protected:
	std::vector<DataSection> _sections;
	Address                  _entry;

public:
	InputReader()
		: _sections(), _entry(0)
	{ }

	virtual ~InputReader();

	/**
	 * @brief Read input
	 *
	 * The input comes from the command line and its interpretation depends
	 * on the concrete implementation of the input reader. The argument may
	 * be a string on the command line, a filename to open, a URL to download
	 * from or whatever else may suit.
	 *
	 * @param input input
	 **/
	virtual void addData(char const *input) = 0;

	Address entry() { return _entry; }

	DataSection* section(unsigned number) // XXX: return a const reference?
	{
		if (number > section_count())
			return 0;
		return &_sections[number];
	}

	DataSection* sectionForAddress(Address a)
	{
		for (unsigned n = 0; n < section_count(); ++n) {
			RelocatedMemRegion mbuf = section(n)->getBuffer();
			if ((mbuf.base >= a) and
			    (a < mbuf.base + mbuf.size))
				return section(n);
		}
		return 0;
	}

	unsigned section_count() { return _sections.size(); }

private:
	InputReader(InputReader const&) : _sections(), _entry(0) { }
	InputReader& operator=(const InputReader&) { return *this; }
};


/**
 * @brief Reader for hexadecimal bytes from the command line
 *
 * Input consisting of two-digit hexadecimal numbers is read directly from the
 * command line and stored in a buffer.
 *
 * So, the command line string "AB CD EF 12 34 56" will end up in a buffer
 * containing 7 bytes:
 *
 * 0xAB 0xCD 0xEF 0x12 0x34 0x 56
 **/
class HexbyteInputReader : public InputReader
{
public:
	HexbyteInputReader()
		: InputReader()
	{
		// we always have a single DataSection
		_sections.push_back(DataSection());
		section(0)->name("hex input");
	}

	virtual ~HexbyteInputReader()
	{ }

	virtual void addData(char const *byte);

private:
	
	HexbyteInputReader(HexbyteInputReader const &) { }
	HexbyteInputReader& operator= (HexbyteInputReader const& ) { return *this; }
};


class FileInputReader : public InputReader
{
public:
	FileInputReader()
		: InputReader()
	{ }

	virtual void addData(char const *file);

private:
	FileInputReader(FileInputReader const&)
	{ }

	FileInputReader& operator= (FileInputReader const &) { return *this; }

	/**
	 * @brief Determine if the input file stream refers to an ELF binary.
	 *
	 * @param str input stream
	 * @return bool true if ELF binary, false otherwise
	 **/
	bool is_elf_file(std::ifstream& str);
};