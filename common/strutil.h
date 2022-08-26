//
//  strutil.h
//  TXWConvert
//
//  Created by yu2924 on 2018-08-15
//

#pragma once

#include <cstdarg>
#include <string>
#include <algorithm>

struct StrUtil
{
	static std::string trim(const std::string& s, const std::string& delim)
	{
		if(s.length() == 0) return s;
		size_t i0 = s.find_first_not_of(delim);
		if(i0 == std::string::npos) return std::string();
		size_t i1 = s.find_last_not_of(delim);
		return std::string(s, i0, i1 - i0 + 1);
	}
	/*
	static std::string unquoted(const std::string& s)
	{
		return trim(s, "\" ");
	}
	static std::string quoted(const std::string& s)
	{
		return "\"" + s + "\"";
	}
	static bool isEqualNoCase(const std::string& a, const std::string& b)
	{
		std::string at = a; std::transform(at.begin(), at.end(), at.begin(), tolower);
		std::string bt = b; std::transform(bt.begin(), bt.end(), bt.begin(), tolower);
		return at == bt;
	}
	static std::string toHex(const void* p, int c)
	{
		std::stringstream str;
		const uint8_t* pb = (const uint8_t*)p;
		for(int i = 0; i < c; i ++) str << StrUtil::format(16, "%02x ", pb[i]);
		return str.str();
	}
	// Scientific pitch notation(SPN) i.e. c-1=0, c4=60
	static std::string spnNoteName(unsigned int k)
	{
		static const char* st[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
		return StrUtil::format(16, "%s%d", st[k % 12], k / 12 - 1);
	}
	*/
	static std::string replaceall(const std::string& s, const std::string& a, const std::string& b)
	{
		std::string rs = s;
		std::string::size_type pos = 0;
		for(;;)
		{
			pos = rs.find(a, pos); if(pos == std::string::npos) break;
			rs.replace(pos, a.size(), b);
			pos += b.size();
		}
		return rs;
	}
	static std::string format(int maxchars, const char* fmt, ...)
	{
		char* s = (char*)alloca(maxchars);
		va_list vp;
		va_start(vp, fmt);
		std::vsnprintf(s, maxchars, fmt, vp);
		va_end(vp);
		return std::string(s);
	}
	static std::string replaceFileSystemUnsafedChars(const std::string& s)
	{
		// NOTE:
		//   '.' at front is unsafe
		//   '.' at back is safe but strange
		//   '/?<>\\:*|\"' are unsafe for NTFS and FAT
		//   '^' is unsafe for FAT
		std::string sr = trim(s, " .");
		static const std::string unsafes = "/?<>\\:*|\"^";
		for(std::string::iterator it = sr.begin();;)
		{
			it = std::find_first_of(it, sr.end(), unsafes.begin(), unsafes.end());
			if(it == sr.end()) break;
			*it = '_';
			it ++;
		}
		return trim(sr, " ");
	}
};
