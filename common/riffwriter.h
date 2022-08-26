//
//  riffwriter.h
//  TXWConvert
//
//  Created by yu2924 on 2018-10-31
//

#pragma once

#include <filesystem>
#include <fstream>
#include <vector>

class RiffWriter
{
public:
	struct Chunk
	{
		uint32_t ckoffset;
		uint32_t ckid;
		uint32_t cksize;
	};
	std::fstream mStr;
	std::vector<Chunk> mStack;
	RiffWriter(const std::filesystem::path& path) : mStr(path, std::ios::out | std::ios::binary | std::ios::trunc)
	{
		mStr.exceptions(std::ios::badbit | std::ios::eofbit | std::ios::failbit);
		descend("RIFF");
	}
	~RiffWriter()
	{
		while(!mStack.empty()) ascend();
	}
	operator bool() const
	{
		return mStr.good();
	}
	void descend(const char* ckid)
	{
		return descend(*(uint32_t*)ckid);
	}
	void descend(uint32_t ckid)
	{
		uint32_t ckoffset = (uint32_t)mStr.tellp();
		Chunk ck = { ckoffset, ckid, 8 };
		mStack.push_back(ck);
		mStr.write((const char*)&ck.ckid, 4);
		mStr.write((const char*)&ck.cksize, 4);
	}
	void ascend()
	{
		if(mStack.empty()) return;
		Chunk& ck = mStack.back();
		uint32_t endpos = (uint32_t)mStr.tellp();
		ck.cksize = endpos - ck.ckoffset - 8;
		mStr.seekp(ck.ckoffset + 4);
		mStr.write((const char*)&ck.cksize, 4);
		mStr.seekp(endpos);
		if(endpos & 0x01) mStr.put(0);
		mStack.pop_back();
	}
	void write(const void* p, size_t c)
	{
		mStr.write((const char*)p, c);
	}
	class ScopedDescend
	{
	public:
		RiffWriter& writer;
		ScopedDescend(RiffWriter& w, uint32_t ckid) : writer(w) { writer.descend(ckid); }
		ScopedDescend(RiffWriter& w, const char* ckid) : writer(w) { writer.descend(ckid); }
		~ScopedDescend() { writer.ascend(); }
	};
};
