#pragma once
/**
	@file
	@brief stream and line stream class

	Copyright (C) Cybozu Labs, Inc., all rights reserved.
*/
#include <string>
#include <iosfwd>
#include <cybozu/exception.hpp>
#include <cybozu/stream_fwd.hpp>

namespace cybozu {

namespace stream_local {

template <typename From, typename To>
struct is_convertible {
	typedef char yes;
	typedef int no;

	static no test(...);
	static yes test(const To*);
	static const bool value = sizeof(test(static_cast<const From*>(0))) == sizeof(yes);
};

template <bool b, class T = void>
struct enable_if { typedef T type; };

template <class T>
struct enable_if<false, T> {};

/* specialization for istream */
template<class InputStream>
size_t readSome_inner(InputStream& is, void *buf, size_t size, typename enable_if<is_convertible<InputStream, std::istream>::value>::type* = 0)
{
	is.read(static_cast<char *>(buf), size);
	const std::streamsize readSize = is.gcount();
	if (readSize < 0) throw cybozu::Exception("stream:readSome_inner:bad readSize") << size << readSize;
	if (readSize > 0xffffffff && size_t(-1) == 0xffffffff) throw cybozu::Exception("stream:readSome_inner:too large") << readSize;
	return static_cast<size_t>(readSize);
}

/* generic version for size_t readSome(void *, size_t) */
template<class InputStream>
size_t readSome_inner(InputStream& is, void *buf, size_t size, typename enable_if<!is_convertible<InputStream, std::istream>::value>::type* = 0)
{
	return is.readSome(buf, size);
}

/* specialization for istream */
template<class InputStream>
bool hasNext_inner(InputStream& is, typename enable_if<is_convertible<InputStream, std::istream>::value>::type* = 0)
{
	std::ios_base::iostate state = is.rdstate();
	char c;
	if (is.get(c)) {
		is.unget();
		return true;
	}
	is.clear(state);
	return false;
}

/* generic version for bool hasNext() */
template<class InputStream>
bool hasNext_inner(InputStream& is, typename enable_if<!is_convertible<InputStream, std::istream>::value>::type* = 0)
{
	return is.hasNext();
}

/* specialization for ostream */
template<class OutputStream>
void writeSub(OutputStream& os, const void *buf, size_t size, typename enable_if<is_convertible<OutputStream, std::ostream>::value>::type* = 0)
{
	if (!os.write(static_cast<const char *>(buf), size)) throw cybozu::Exception("stream:writeSub") << size;
}

/* generic version for void write(const void*, size_t), which writes all data */
template<class OutputStream>
void writeSub(OutputStream& os, const void *buf, size_t size, typename enable_if<!is_convertible<OutputStream, std::ostream>::value>::type* = 0)
{
	os.write(buf, size);
}

} // stream_local

/*
	make a specializaiton of class to use new InputStream, OutputStream
*/
template<class InputStream>
struct InputStreamTag {
	static inline size_t readSome(InputStream& is, void *buf, size_t size)
	{
		return stream_local::readSome_inner<InputStream>(is, buf, size);
	}
	static inline bool hasNext(InputStream& is)
	{
		return stream_local::hasNext_inner<InputStream>(is);
	}
	static inline char readChar(InputStream& is)
	{
		char c;
		if (readSome(is, &c, 1) != 1) throw cybozu::Exception("InputStreamTag:readChar");
		return c;
	}
};

template<class OutputStream>
struct OutputStreamTag {
	static inline void write(OutputStream& os, const void *buf, size_t size)
	{
		stream_local::writeSub<OutputStream>(os, buf, size);
	}
};

class MemoryInputStream {
	const char *p_;
	size_t size_;
	size_t pos;
public:
	MemoryInputStream(const void *p, size_t size) : p_(static_cast<const char *>(p)), size_(size), pos(0) {}
	size_t readSome(void *buf, size_t size)
	{
		if (size > size_  - pos) size = size_ - pos;
		memcpy(buf, p_ + pos, size);
		pos += size;
		return size;
	}
	size_t getPos() const { return pos; }
	bool hasNext() const { return pos < size_; }
};

class MemoryOutputStream {
	char *p_;
	size_t size_;
	size_t pos;
public:
	MemoryOutputStream(void *p, size_t size) : p_(static_cast<char *>(p)), size_(size), pos(0) {}
	void write(const void *buf, size_t size)
	{
		if (size > size_ - pos) throw cybozu::Exception("MemoryOutputStream:write") << size << size_ << pos;
		memcpy(p_ + pos, buf, size);
		pos += size;
	}
	size_t getPos() const { return pos; }
};

class StringInputStream {
	const std::string& str_;
	size_t pos;
	StringInputStream(const StringInputStream&);
	void operator=(const StringInputStream&);
public:
	explicit StringInputStream(const std::string& str) : str_(str), pos(0) {}
	size_t readSome(void *buf, size_t size)
	{
		const size_t remainSize = str_.size() - pos;
		if (size > remainSize) size = remainSize;
		memcpy(buf, &str_[pos], size);
		pos += size;
		return size;
	}
	size_t getPos() const { return pos; }
	bool hasNext() const { return pos < str_.size(); }
};

class StringOutputStream {
	std::string& str_;
	StringOutputStream(const StringOutputStream&);
	void operator=(const StringOutputStream&);
public:
	explicit StringOutputStream(std::string& str) : str_(str) {}
	void write(const void *buf, size_t size)
	{
		str_.append(static_cast<const char *>(buf), size);
	}
	size_t getPos() const { return str_.size(); }
};

template<class InputStream>
size_t readSome(InputStream& is, void *buf, size_t size)
{
	return stream_local::readSome_inner(is, buf, size);
}

template<class OutputStream>
void write(OutputStream& os, const void *buf, size_t size)
{
	stream_local::writeSub(os, buf, size);
}

} // cybozu
