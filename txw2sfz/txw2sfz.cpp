//
//  txw2sfz.cpp
//  txw2sfz
//
//  Created by yu2924 on 2018-10-31
//

#include <iostream>
#include <filesystem>
#include <fstream>
#include <vector>
#include <array>
#include <sstream>

#include "tx16wtypes.h"
#include "CurveMapping.h"

#if defined _MSC_VER
#include <crtdbg.h>
#endif

// ================================================================================
// TX16WData

class TX16WData
{
public:
	static std::string formatChannel(unsigned int ch)
	{
		if(17 <= ch) return "----";
		if(ch == 16) return "omni";
		return StrUtil::format(16, "%4u", ch);
	}
	static std::string formatOutput(unsigned int o)
	{
		static const char* so[] = {" off", "I   ", "  II", "I+II"};
		return (o < std::size(so)) ? so[o] : "----";
	}
	// YAMAHA style pitch notation
	static std::string formatNoteName(unsigned int k)
	{
		static const char* st[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
		return StrUtil::format(16, "%s%d", st[k % 12], k / 12 - 2);
	}
	union { TXWSETUPV1 v1; TXWSETUPV2 v2; } mSetup;
	std::array<TXWWAVENAME, TXW_COUNTOF_WAVES> mWaveNames; // 16*64=1024 bytes
	std::array<TXWPERF, TXW_COUNTOF_PERFS> mPerformances; // 146*32=4672 bytes
	std::array<TXWVOICE, TXW_COUNTOF_VOICES> mVoices; // 146*32=4672 bytes
	std::vector<TXWTIMBRE> mTimbres; // 56*64=3584 or 56*128=7168 bytes
	std::array<TXWVOICEV2EXTRA, TXW_COUNTOF_VOICES> mVoicesV2X; // 64x32=2048 bytes
	std::vector<TXWTIMBREV2EXTRA> mTimbresV2X; // 8*128=1024 bytes
	bool mIsSetupV2, mIsPerfV2, mIsVoiceV2;
	struct WAVE
	{
		std::string name;
		std::string filename;
		TXWWAVE txw;
		uint32_t pcmlength;
		bool isv2;
	};
	std::array<WAVE, TXW_COUNTOF_WAVES> mWaves;
	TX16WData() : mSetup(), mIsSetupV2(false), mIsPerfV2(false), mIsVoiceV2(false)
	{
	}
	void clear()
	{
		mSetup = {};
		mWaveNames = {};
		mPerformances = {};
		mVoices = {};
		mTimbres.clear();
		mVoicesV2X = {};
		mTimbresV2X.clear();
		mIsSetupV2 = false;
		mIsPerfV2 = false;
		mIsVoiceV2 = false;
		mWaves = {};
	}
	void load(const std::filesystem::path& inputdir, const std::filesystem::path& inputbasename, bool verbose)
	{
		clear();
		// setup
		{
			std::filesystem::path path = inputdir / (inputbasename.string() + ".S01");
			if(verbose) std::cout << "reading " << path.filename() << std::endl;
			std::fstream str(path, std::ios::in | std::ios::binary);
			if(!str) throw std::runtime_error("failed to open: \"" + path.filename().string() + "\"");
			str.exceptions(std::ios::badbit | std::ios::eofbit | std::ios::failbit);
			if(!TXWUtil::parseFileHeader(str, &mIsSetupV2)) throw std::runtime_error("setup: invalid format");
			if(mIsSetupV2) str.read((char*)&mSetup.v2, sizeof(mSetup.v2));
			else		   str.read((char*)&mSetup.v1, sizeof(mSetup.v1));
			str.read((char*)mWaveNames.data(), mWaveNames.size() * sizeof(TXWWAVENAME));
		}
		// performances
		{
			std::filesystem::path path = inputdir / (inputbasename.string() + ".U01");
			if(verbose) std::cout << "reading " << path.filename() << std::endl;
			std::fstream str(path, std::ios_base::in | std::ios_base::binary);
			if(!str) throw std::runtime_error("open failed: \"" + path.filename().string() + "\"");
			str.exceptions(std::ios::badbit | std::ios::eofbit | std::ios::failbit);
			if(!TXWUtil::parseFileHeader(str, &mIsPerfV2)) throw std::runtime_error("performance: invalid format");
			str.read((char*)mPerformances.data(), mPerformances.size() * sizeof(TXWPERF));
		}
		// voices and timbres
		{
			std::filesystem::path path = inputdir / (inputbasename.string() + ".V01");
			if(verbose) std::cout << "reading " << path.filename() << std::endl;
			std::fstream str(path, std::ios_base::in | std::ios_base::binary);
			if(!str) throw std::runtime_error("failed to open: \"" + path.filename().string() + "\"");
			str.exceptions(std::ios::badbit | std::ios::eofbit | std::ios::failbit);
			if(!TXWUtil::parseFileHeader(str, &mIsVoiceV2)) throw std::runtime_error("voice: invalid format");
			str.read((char*)mVoices.data(), mVoices.size() * sizeof(TXWVOICE));
			mTimbres.resize(mIsVoiceV2 ? TXW_COUNTOF_TIMBRES_V2 : TXW_COUNTOF_TIMBRES_V1);
			mTimbresV2X.resize(mIsVoiceV2 ? TXW_COUNTOF_TIMBRES_V2 : TXW_COUNTOF_TIMBRES_V1);
			str.read((char*)mTimbres.data(), mTimbres.size() * sizeof(TXWTIMBRE));
			// V2 extra area
			if(mIsVoiceV2)
			{
				str.read((char*)mVoicesV2X.data(), mVoicesV2X.size() * sizeof(TXWVOICEV2EXTRA));
				str.read((char*)mTimbresV2X.data(), mTimbresV2X.size() * sizeof(TXWTIMBREV2EXTRA));
			}
			// and still remains extra 432 bytes
		}
		// waves
		{
			for(size_t cw = mWaveNames.size(), iw = 0; iw < cw; iw ++)
			{
				const TXWWAVENAME& wavename = mWaveNames[iw];
				WAVE& wave = mWaves[iw];
				wave.name = TXWUtil::extractWaveName(wavename);
				if(!wave.name.empty())
				{
					wave.filename = wave.name + StrUtil::format(128, ".W%02u", iw + 1);
					std::filesystem::path path = inputdir / wave.filename;
					if(verbose) std::cout << "reading " << path.filename() << std::endl;
					std::fstream str(path, std::ios::in | std::ios::binary);
					if(!str) throw std::runtime_error("failed to open: \"" + path.filename().string() + "\"");
					str.exceptions(std::ios::badbit | std::ios::eofbit | std::ios::failbit);
					if(TXWUtil::parseFileHeader(str, &wave.isv2)) str.read((char*)&wave.txw, sizeof(wave.txw));
					uint32_t pcmoffset = (uint32_t)str.tellg();
					str.seekg(0, std::ios_base::end);
					wave.pcmlength = ((uint32_t)str.tellg() - pcmoffset) / 3 * 2;
					str.seekg(pcmoffset);
				}
			}
		}
	}
	void writeCatalog(const std::filesystem::path& outputdir, const std::filesystem::path& inputbasename, bool overwrite, bool verbose) const
	{
		std::filesystem::path path = outputdir / (inputbasename.string() + ".txt");
		if(verbose) std::cout << "writing text " << path.filename() << std::endl;
		if(!overwrite && std::filesystem::exists(path)) throw std::runtime_error("path exists");
		std::fstream cat(path, std::ios::out | std::ios_base::trunc);
		if(!cat) throw std::runtime_error("failed to create: \"" + path.filename().string() + "\"");
		cat.exceptions(std::ios::badbit | std::ios::eofbit | std::ios::failbit);
		cat << "txw2sfz" << std::endl;
		cat << "bank: " << inputbasename << std::endl;
		cat << std::endl;
		cat << StrUtil::format(128, "SETUP(V%d)", mIsVoiceV2 ? 2 : 1) << std::endl;
		cat << StrUtil::format(128, "tune=%02d, volume=[%02u,%02u]",
			mSetup.v1.MasterTune.get(),
			mSetup.v1.MasterVolI,
			mSetup.v1.MasterVolII) << std::endl;
		cat << std::endl;
		cat << "WAVENAMES" << std::endl;
		cat << "#    name       file           fs    length atclen rptlen v2 " << std::endl;
		cat << "---: ---------- -------------- ----- ------ ------ ------ ---" << std::endl;
		for(size_t cw = mWaveNames.size(), iw = 0; iw < cw; iw ++)
		{
			const TXWWAVENAME& wavename = mWaveNames[iw];
			const WAVE& wave = mWaves[iw]; if(wave.name.empty()) continue;
			cat << StrUtil::format(128, "W%02u: %-10s %-14s %5u %6u %6u %6u %3s",
				iw,
				("\"" + wave.name + "\"").c_str(),
				("\"" + wave.filename + "\"").c_str(),
				wave.txw.getSampleRate(),
				wave.pcmlength,
				wave.txw.getAttackLength(),
				wave.txw.getRepeatLength(),
				wave.isv2 ? "yes" : "no") << std::endl;
		}
		cat << std::endl;
		cat << StrUtil::format(128, "PERFORMANCES(V%d)", mIsPerfV2 ? 2 : 1) << std::endl;
		cat << "\t#  Voice Chn  Out  Volume Detune Shift" << std::endl;
		cat << "\t-- ----- ---- ---- ------ ------ -----" << std::endl;
		for(size_t cp = mPerformances.size(), ip = 0; ip < cp; ip ++)
		{
			const TXWPERF& perf = mPerformances[ip];
			std::string perfname = TXWUtil::extractName(perf);
			cat << StrUtil::format(128, "P%02u: \"", (unsigned int)ip) << perfname << "\"" << std::endl;
			unsigned int group = (unsigned int)-1;
			for(size_t iv = 0; iv < TXW_COUNTOF_VOICES_PERPERF; iv ++)
			{
				if(perf.group[iv] == group) continue;
				group = perf.group[iv];
				cat << StrUtil::format(128, "\t%2u %5u %4s %4s %6u %6d %5d",
					(unsigned int)iv,
					perf.voice[iv],
					formatChannel(perf.midich[iv]).c_str(),
					formatOutput(perf.output[iv]).c_str(),
					perf.volume[iv],
					perf.detune[iv],
					perf.shift[iv]) << std::endl;
			}
		}
		cat << std::endl;
		cat << StrUtil::format(128, "VOICES(V%d)", mIsVoiceV2 ? 2 : 1) << std::endl;
		cat << "\t#  timber lo-k hi-k fade orgk " << std::endl;
		cat << "\t-- ------ ---- ---- ---- ----" << std::endl;
		for(size_t cv = mVoices.size(), iv = 0; iv < cv; iv ++)
		{
			const TXWVOICE& voice = mVoices[iv];
			const TXWVOICEV2EXTRA& voiceext = mVoicesV2X[iv];
			std::string voicename = TXWUtil::extractName(voice);
			cat << StrUtil::format(128, "V%02u: \"", (unsigned int)iv) << voicename << "\"" << std::endl;
			for(size_t it = 0; it < TXW_COUNTOF_TIMBRES_PERVOICE; it ++)
			{
				const TXWVOICE::TIMBRE& vtmbr = voice.timbres[it];
				const TXWVOICEV2EXTRA::TIMBRE& vtmbrext = voiceext.timbres[it];
				size_t timbreindex = vtmbr.Number;
				if(mTimbres.size() <= timbreindex) continue;
				const TXWTIMBRE& timbre = mTimbres[timbreindex];
				size_t waveindex = timbre.WaveNumber;
				if(mWaveNames.size() <= waveindex) continue;
				const TXWWAVENAME& wavename = mWaveNames[waveindex];
				if(TXWUtil::extractWaveName(wavename).empty()) continue;
				uint8_t lokey = vtmbr.LoKey.get(), hikey = vtmbr.HiKey.get();
				if((127 < lokey) || (127 < hikey) || (hikey < lokey)) continue;
				cat << StrUtil::format(128, "\t%2u %6u %4s %4s %4u %4s",
					(unsigned int)it,
					vtmbr.Number,
					formatNoteName(lokey).c_str(),
					formatNoteName(hikey).c_str(),
					vtmbr.Fade,
					mIsVoiceV2 ? formatNoteName(vtmbrext.getOriginalPitchKey()).c_str() : "----") << std::endl;
			}
		}
		cat << std::endl;
		cat << StrUtil::format(128, "TIMBRES(V%d)", mIsVoiceV2 ? 2 : 1) << std::endl;
		cat << "#     name         wav root tune AAR AD1R AD1L AD2R AD2L ARR PR1 PL1 PR2 PL2 PR3 PL3 PR4 PL4 fixp 1shot" << std::endl;
		cat << "----: ------------ --- ---- ---- --- ---- ---- ---- ---- --- --- --- --- --- --- --- --- --- ---- -----" << std::endl;
		for(size_t ct = mTimbres.size(), it = 0; it < ct; it ++)
		{
			const TXWTIMBRE& timbre = mTimbres[it];
			const TXWTIMBREV2EXTRA& tv2ext = mTimbresV2X[it];
			std::string timbrename = TXWUtil::extractName(timbre);
			if(mWaveNames.size() <= timbre.WaveNumber) continue;
			std::string wavname = TXWUtil::extractWaveName(mWaveNames[timbre.WaveNumber]); if(wavname.empty()) continue;
			cat << StrUtil::format(128, "T%03u: %-12s %3u %4s %4d %3u %4u %4u %4u %4u %3u %3u %3u %3u %3u %3u %3u %3u %3u %4s %5s",
				(unsigned int)it,
				("\"" + timbrename + "\"").c_str(),
				timbre.WaveNumber,
				mIsVoiceV2 ? "----" : formatNoteName(timbre.OriginalPitch.get()).c_str(),
				timbre.tune.get(),
				timbre.AEGAR,
				timbre.AEGD1R,
				timbre.AEGD1L,
				timbre.AEGD2R,
				timbre.AEGD2L,
				timbre.AEGRR,
				timbre.PEGR1,
				timbre.PEGL1,
				timbre.PEGR2,
				timbre.PEGL2,
				timbre.PEGR3,
				timbre.PEGL3,
				timbre.PEGR4,
				timbre.PEGL4,
				mIsVoiceV2 ? ((tv2ext.fixpitch.get() != 0x7fff) ? StrUtil::format(128, "%4d", tv2ext.fixpitch.get()).c_str() : " off") : "----",
				mIsVoiceV2 ? StrUtil::format(128, "%4d", tv2ext.getOneshotTrigger()).c_str() : "----") << std::endl;
		}
	}
	void writeSFZ(const std::filesystem::path& outputdir, bool overwrite, bool verbose) const
	{
		// Perf => sfz
		// Voice => <group>
		// Timbre => <region>
		for(size_t cp = mPerformances.size(), ip = 0; ip < cp; ip ++)
		{
			const TXWPERF& perf = mPerformances[ip];
			std::stringstream perfstr(std::ios::out);
			int numvoices = 0;
			uint8_t group = (uint8_t)-1;
			for(size_t iv = 0; iv < TXW_COUNTOF_VOICES_PERPERF; iv ++)
			{
				if(perf.group[iv] == group) continue;
				group = perf.group[iv];
				// voice
				size_t voiceindex = perf.voice[iv];
				if(mVoices.size() <= voiceindex) continue;
				const TXWVOICE& voice = mVoices[voiceindex];
				const TXWVOICEV2EXTRA& voiceext = mVoicesV2X[voiceindex];
				// voice stream
				std::stringstream voicestr(std::ios::out);
				voicestr << "<group>" << std::endl;
				uint8_t vmidich = perf.midich[iv]; // 0~15,16:omni,17:off
				uint8_t voutput = perf.output[iv]; // 0:off,1:I,2:II,3:I+II
				uint8_t vvolume = perf.volume[iv]; // 0~99
				int8_t vdetune = perf.detune[iv]; // -7~7
				int8_t vshift = perf.shift[iv]; // -24~24
				if     (vmidich <  16) voicestr << StrUtil::format(128, "lochan=%u hichan=%u", vmidich + 1, vmidich + 1) << std::endl;
				else if(vmidich == 16) voicestr << "lochan=1 hichan=16" << std::endl;
				else continue;
				if     (voutput == 0) continue;
				else if(voutput == 1) voicestr << "pan=-100" << std::endl;
				else if(voutput == 2) voicestr << "pan=100" << std::endl;
				else				  voicestr << "pan=0" << std::endl;
				if(0 < vvolume) voicestr << StrUtil::format(128, "volume=%g", (double)vvolume - 99) << std::endl;
				else continue;
				int numtimbres = 0;
				for(size_t it = 0; it < TXW_COUNTOF_TIMBRES_PERVOICE; it ++)
				{
					// voice timbre
					const TXWVOICE::TIMBRE& vtmbr = voice.timbres[it];
					const TXWVOICEV2EXTRA::TIMBRE& vtmbrext = voiceext.timbres[it];
					// timbre
					size_t timbreindex = vtmbr.Number;
					if(mTimbres.size() <= timbreindex) continue;
					const TXWTIMBRE& timbre = mTimbres[timbreindex];
					const TXWTIMBREV2EXTRA& timbreext = mTimbresV2X[timbreindex];
					// wavename
					size_t waveindex = timbre.WaveNumber;
					if(mWaves.size() <= waveindex) continue;
					const WAVE& wave = mWaves[waveindex];
					if(wave.name.empty()) continue;
					// timbre stream
					std::stringstream timbrestr(std::ios::out);
					timbrestr << "<region>";
					uint8_t lokey = vtmbr.LoKey.get(), hikey = vtmbr.HiKey.get();
					if((127 < lokey) || (127 < hikey) || (hikey < lokey)) continue;
					uint8_t orgkey = mIsVoiceV2 ? vtmbrext.getOriginalPitchKey() : timbre.OriginalPitch.get();
					uint8_t fixedpitchshift = mIsVoiceV2 ? timbreext.fixpitch.get() : 0x7fff;
					bool fixedpitchenabled = fixedpitchshift == 0x7fff;
					int oneshotms = mIsVoiceV2 ? timbreext.getOneshotTrigger() : 0;
					bool oneshotenabled = oneshotms != 0;
					int tune = timbre.tune.get(); // -200~200
					timbrestr << " sample=" << (wave.name + ".wav");
					timbrestr << StrUtil::format(128, " lokey=%u hikey=%u", std::clamp(lokey + vshift, 0, 127), std::clamp(hikey + vshift, 0, 127));
					timbrestr << StrUtil::format(128, " pitch_keycenter=%u", std::clamp(orgkey + vshift, 0, 127));
					timbrestr << StrUtil::format(128, " tune=%d", (int)((vdetune * 50.0 / 7.0) + (tune * 100.0 / 200.0)));
					if(fixedpitchenabled) timbrestr << StrUtil::format(128, " transpose=%d pitch_keytrack=0", fixedpitchshift);
					if(oneshotenabled) timbrestr << " loop_mode=one_shot";
					// aeg
					auto fcnvrate = [](uint8_t v) -> double // (99,0) => (0.001,10)
					{
						FABB::CurveMapExponentialD conv(0,99,0.1,1000);
						return 1.0 / conv.Map(v);
					};
					auto fcnvval = [](uint8_t v) -> double // (0,99) => (0,100)
					{
						FABB::CurveMapLinearD conv(0,99,0,100);
						return conv.Map(v);
					}; // (0,99) => (0,100)
					timbrestr << StrUtil::format(128, " ampeg_attack=%g", fcnvrate(timbre.AEGAR));
					timbrestr << StrUtil::format(128, " ampeg_decay=%g", fcnvrate(timbre.AEGD1R) + fcnvrate(timbre.AEGD2R));
					timbrestr << StrUtil::format(128, " ampeg_sustain=%g", fcnvval(timbre.AEGD2L));
					timbrestr << StrUtil::format(128, " ampeg_release=%g", fcnvrate(timbre.AEGRR));
					//
					// TODO: more modulations
					//
					timbrestr << std::endl;
					voicestr << timbrestr.str();
					numtimbres ++;
				}
				if(0 < numtimbres)
				{
					perfstr << voicestr.str();
					numvoices ++;
				}
			}
			std::string perfname = TXWUtil::extractName(perf);
			if(0 < numvoices)
			{
				std::string fn = StrUtil::format(16, "%02u ", (unsigned int)ip) + StrUtil::replaceFileSystemUnsafedChars(perfname) + ".sfz";
				std::filesystem::path path = outputdir / fn;
				if(verbose) std::cout << "writing sfz " << path.filename() << std::endl;
				if(!overwrite && std::filesystem::exists(path)) throw std::runtime_error("path exists");
				std::fstream sfz(path, std::ios::out | std::ios_base::trunc);
				if(!sfz) throw std::runtime_error("failed to create: \"" + path.filename().string() + "\"");
				sfz.exceptions(std::ios::badbit | std::ios::eofbit | std::ios::failbit);
				sfz << perfstr.str();
			}
		}
	}
	void writeWaves(const std::filesystem::path& inputdir, const std::filesystem::path& outputdir, bool overwrite, bool verbose) const
	{
		for(size_t cw = mWaves.size(), iw = 0; iw < cw; iw ++)
		{
			const WAVE& wave = mWaves[iw];
			if(wave.name.empty()) continue;
			// determine the original key
			uint32_t orgkey = 60;
			if(mIsVoiceV2)
			{
				for(size_t cv = mVoices.size(), iv = 0; iv < cv; iv ++)
				{
					bool found = false;
					for(size_t it = 0; it < TXW_COUNTOF_TIMBRES_PERVOICE; it ++)
					{
						const TXWVOICE::TIMBRE& vtmbr = mVoices[iv].timbres[it];
						const TXWVOICEV2EXTRA::TIMBRE& vtmbrext = mVoicesV2X[iv].timbres[it];
						if(vtmbr.Number == iw) { orgkey = vtmbrext.getOriginalPitchKey(); found = true; break; }
					}
					if(found) break;
				}
			}
			else
			{
				for(size_t ct = mTimbres.size(), it = 0; it < ct; it ++)
				{
					const TXWTIMBRE& timbre = mTimbres[it];
					if(timbre.WaveNumber == iw) { orgkey = timbre.OriginalPitch.get(); break; }
				}
			}
			// convert
			std::filesystem::path txwpath = inputdir / wave.filename;
			std::filesystem::path wavpath = outputdir / (wave.name + ".wav");
			if(verbose) std::cout << "converting wave \"" << wave.filename << "\" =>" << wavpath.filename() << std::endl;
			std::string err;
			if(!TXWUtil::convertWave(txwpath, wavpath, orgkey, overwrite, &err)) { std::cerr << "ERROR: " << err << std::endl; continue; }
		}
	}
};

// ================================================================================
// the entry point

int main(int argc, char** argv)
{
#if defined _MSC_VER
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	std::filesystem::path inputfile;
	std::filesystem::path outputdir;
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
			if     (inputfile.empty()) inputfile = std::filesystem::absolute(StrUtil::trim(arg, "\r\n\t \""));
			else if(outputdir.empty()) outputdir = std::filesystem::absolute(StrUtil::trim(arg, "\r\n\t \""));
		}
	}
	if(verbose)
	{
		std::cout << "txw2sfz" << std::endl;
	}
	if(help)
	{
		std::cout << "usage: txw2sfz [input file] [output directory] [-d][-h][-o][-v]" << std::endl;
		std::cout << "  -d: use default output directory 'sfz'" << std::endl;
		std::cout << "  -h: help" << std::endl;
		std::cout << "  -o: overwrite" << std::endl;
		std::cout << "  -v: verbose" << std::endl;
		std::cout << "omit [input file] and [output directory] to enter the interactive mode" << std::endl;
		return 0;
	}
	int r = -1;
	if(inputfile.empty())
	{
		std::cout << "input file>";
		std::string arg; std::getline(std::cin, arg);
		inputfile = std::filesystem::absolute(StrUtil::trim(arg, "\r\n\t \""));
		if(inputfile.empty()) return -1;
	}
	std::filesystem::path inputdir = inputfile.parent_path();
	std::filesystem::path inputbasename = inputfile.filename().replace_extension();
	if(outputdir.empty() && defdir)
	{
		outputdir = inputdir / "sfz";
	}
	if(outputdir.empty())
	{
		std::cout << "output directory>";
		std::string arg; std::getline(std::cin, arg);
		outputdir = std::filesystem::absolute(StrUtil::trim(arg, "\r\n\t \""));
		if(outputdir.empty()) return -1;
	}
	try
	{
		if(verbose)
		{
			std::cout << "input file        : \"" << (inputdir / inputbasename).string() << "\"" << std::endl;
			std::cout << "output directory  : \"" << outputdir.string() << "\"" << std::endl;
			std::cout << "overwrite         : " << (overwrite ? "yes" : "no") << std::endl;
		}
		std::filesystem::create_directories(outputdir);
		TX16WData txwdata;
		txwdata.load(inputdir, inputbasename, verbose);
		txwdata.writeCatalog(outputdir, inputbasename, overwrite, verbose);
		txwdata.writeSFZ(outputdir, overwrite, verbose);
		txwdata.writeWaves(inputdir, outputdir, overwrite, verbose);
		r = 0;
	}
	catch(std::exception& e)
	{
		std::cerr << "exception: " << e.what() << std::endl;
		r = -1;
	}
	return r;
}
