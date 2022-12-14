//
//  txw2wav.cpp
//  txw2wav
//
//  Created by yu2924 on 2018-11-04
//

#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <regex>

#include "tx16wtypes.h"

#if defined _MSC_VER
#include <crtdbg.h>
#endif

static std::vector<std::filesystem::path> findMatchFiles(const std::filesystem::path& pathspec)
{
	std::filesystem::path dir = pathspec.parent_path();
	std::string spec = pathspec.filename().string();
	spec = StrUtil::replaceall(spec, "*", ".*");
	spec = StrUtil::replaceall(spec, "?", ".?");
	std::regex rx(spec);
	std::vector<std::filesystem::path> pathlist;
	for(const auto& ent : std::filesystem::directory_iterator(dir))
	{
		if(ent.is_directory()) continue;
		if(!std::regex_match(ent.path().filename().string(), rx)) continue;
		pathlist.push_back(ent.path());
	}
	return pathlist;
}

int main(int argc, char** argv)
{
#if defined _MSC_VER
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	std::filesystem::path inputspec;
	std::filesystem::path outputspec;
	bool defdir = false;
	bool help = false;
	bool overwrite = false;
	bool verbose = false;
	for(int argi = 1; argi < argc; argi ++)
	{
		std::string arg(argv[argi]);
		if(arg[0] == '-')
		{
			arg = arg.substr(1);
			if((arg[0] == 'd') || (arg[0] == 'D')) defdir = true;
			if((arg[0] == 'h') || (arg[0] == 'H')) help = true;
			if((arg[0] == 'o') || (arg[0] == 'O')) overwrite = true;
			if((arg[0] == 'v') || (arg[0] == 'V')) verbose = true;
		}
		else
		{
			if     (inputspec.empty()) inputspec = std::filesystem::absolute(StrUtil::trim(arg, "\r\n\t \""));
			else if(outputspec.empty()) outputspec = std::filesystem::absolute(StrUtil::trim(arg, "\r\n\t \""));
		}
	}
	if(verbose)
	{
		std::cout << "txw2wav" << std::endl;
	}
	if(help)
	{
		std::cout << "usage: txw2wav [input spec] [output spec] [-d][-h][-o][-v]" << std::endl;
		std::cout << "  -d: use default output directory 'wav'" << std::endl;
		std::cout << "  -h: help" << std::endl;
		std::cout << "  -o: overwrite" << std::endl;
		std::cout << "  -v: verbose" << std::endl;
		std::cout << "omit [input spec] and [output directory] to enter the interactive mode" << std::endl;
		return 0;
	}
	if(inputspec.empty())
	{
		std::cout << "input>";
		std::string arg; std::getline(std::cin, arg);
		inputspec = std::filesystem::absolute(StrUtil::trim(arg, "\r\n\t \""));
		if(inputspec.empty()) return -1;
	}
	int r = -1;
	if(std::filesystem::is_directory(inputspec))
	{
		// multiple files
		std::filesystem::path inputdir = inputspec;
		std::vector<std::filesystem::path> inputlist = findMatchFiles(inputdir / "*.W??");
		std::filesystem::path outputdir = outputspec;
		if(outputdir.empty() && defdir) outputdir = inputdir / "wav";
		if(outputdir.empty())
		{
			std::cout << "output directory>";
			std::string arg; std::getline(std::cin, arg);
			outputdir = std::filesystem::absolute(StrUtil::trim(arg, "\r\n\t \""));
			if(outputdir.empty()) return -1;
		}
		std::filesystem::create_directories(outputdir);
		for(const auto& inputpath : inputlist)
		{
			std::filesystem::path outputpath = outputdir / inputpath.filename().replace_extension("wav");
			if(verbose) std::cout << "converting wave " << inputpath.filename() << " =>" << outputpath.filename() << std::endl;
			std::string err;
			if(!TXWUtil::convertWave(inputpath, outputpath, 60, overwrite, &err)) std::cerr << "ERROR: " << err << std::endl;
		}
		r = 0;
	}
	else
	{
		// single file
		std::filesystem::path inputpath = inputspec;
		std::filesystem::path inputdir = inputpath.parent_path();
		if(outputspec.empty() && defdir)
		{
			std::filesystem::path outputdir = inputdir / "wav";
			outputspec = outputdir;
			std::filesystem::create_directories(outputdir);
		}
		if(outputspec.empty())
		{
			std::cout << "output file>";
			std::string arg; std::getline(std::cin, arg);
			outputspec = std::filesystem::absolute(StrUtil::trim(arg, "\r\n\t \""));
			if(outputspec.empty()) return -1;
		}
		std::filesystem::path outputpath = std::filesystem::is_directory(outputspec)
			? (outputspec / inputpath.filename().replace_extension("wav"))
			: outputspec;
		std::filesystem::create_directories(outputpath.parent_path());
		if(verbose) std::cout << "converting wave " << inputpath.filename() << " =>" << outputpath.filename() << std::endl;
		std::string err;
		if(!TXWUtil::convertWave(inputpath, outputpath, 60, overwrite, &err)) std::cerr << "ERROR: " << err << std::endl;
		r = 0;
	}
	return r;
}
