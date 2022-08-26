//
//  tx16w.h
//  TXWConvert
//
//  Created by yu2924 on 2018-08-15
//

#pragma once

#include <stdint.h>
#include <string>
#include "strutil.h"
#include "riffwriter.h"
#include "wavfmt.h"

// references:
//   setup, performances, voices, timbres: voltex.c, MIDI bulk dump format specification
//   waves: tx16w.tec.txt

#pragma pack(push, 1)

// ================================================================================
// types

enum
{
	TXW_COUNTOF_PERFS = 32,
	TXW_COUNTOF_VOICES = 32,
	TXW_COUNTOF_TIMBRES_V1 = 64,
	TXW_COUNTOF_TIMBRES_V2 = 128,
	TXW_COUNTOF_VOICES_PERPERF = 16,
	TXW_COUNTOF_TIMBRES_PERVOICE = 32,
	TXW_COUNTOF_WAVES = 64,
};

// cf. voltex.c, but it seems to be shifted 1 octave...
// Yamaha OS encodes key likes this:
// bits:AAAABBCC,  Midi Note = 12*(AAAA)+3*(BB)+(CC)-11
// Was that LSD or 'shrooms?
// #define MIDI(k) ((12*((k)& 0xF0))/0x10 + (3*((k) & 0x0C))/4 + ((k)& 0x03)-11)
struct TXWMIDIKEY
{
	uint8_t raw;
	uint8_t get() const
	{
		unsigned int k = raw;
		return (uint8_t)((12 * (k & 0xf0)) / 16 + (3 * (k & 0x0c)) / 4 + (k & 0x03) + 1);
	}
};

struct TXWINT16
{
	uint8_t msb, lsb;
	int16_t get() const
	{
		return (int16_t)(((uint16_t)msb << 8) | (uint16_t)lsb);
	}
};

// 16(0x10) bytes
struct TXWFILEHDR
{
	char signature[6]; // "LM8953"
	uint8_t reserved0[1]; // zeros
	uint8_t version[4]; // V1:zeros, V2:"0200"
	uint8_t reserved1[5]; // zeros
};

// 231(0xe7) bytes
struct TXWSETUPV1
{
	TXWINT16 MasterTune; // -64~63(0)
	uint8_t MasterVolI, MasterVolII; // 0~99(99)
	uint8_t ControlTable[90]; // 0~7, (0:off,1:modwheel,2:breathctrl,3:footctrl,4:sustain,5:volume,6:increment,7:decrement)
	uint8_t ProgramChange[4][32]; // 0~31
	uint8_t ProgramChangeSwitch; // 0~1(0), (0:off,1:on)
	uint8_t DeviceNumber; // 0~17(17), (0:off,1:1ch,2:2ch,...,17:all)
	uint8_t reserved[2];
	uint8_t MIDIChProgramChange, MIDIChControlChange, MIDIChAfterTouch, MIDIChPitchbend, MIDIChNoteOnOff;
};

// 233(0xe9) bytes
struct TXWSETUPV2
{
	TXWINT16 MasterTune; // -64~63(0)
	uint8_t MasterVolI, MasterVolII; // 0~99(99)
	uint8_t ControlTable[90]; // 0~7, (0:off,1:modwheel,2:breathctrl,3:footctrl,4:sustain,5:volume,6:increment,7:decrement)
	uint8_t ProgramChange[4][32]; // 0~31
	uint8_t ProgramChangeSwitch; // 0~1(0), (0:off,1:on)
	uint8_t DeviceNumber; // 0~17(17), (0:off,1:1ch,2:2ch,...,17:all)
	uint8_t reserved[4];
	uint8_t MIDIChProgramChange, MIDIChControlChange, MIDIChAfterTouch, MIDIChPitchbend, MIDIChNoteOnOff;
};

// 16(0x10) bytes
struct TXWWAVENAME
{
	char name[8];
	uint8_t sizeinfo[8];
};

// 146(0x92) bytes
struct TXWPERF
{
	uint8_t AlternativeAssign[2];
	uint8_t midich[TXW_COUNTOF_VOICES_PERPERF]; // 0~16(16), (0~15,16:omni,17:off)
	uint8_t voice[TXW_COUNTOF_VOICES_PERPERF]; // 0~31(0)
	uint8_t group[TXW_COUNTOF_VOICES_PERPERF]; // 0~15(0)
	uint8_t output[TXW_COUNTOF_VOICES_PERPERF]; // 0~3(3), (0:off,1:I,2:II,3:I+II)
	uint8_t volume[TXW_COUNTOF_VOICES_PERPERF]; // 0~99(99)
	int8_t detune[TXW_COUNTOF_VOICES_PERPERF]; // -7~7(0), (TBV: -50~50 cents?)
	int8_t shift[TXW_COUNTOF_VOICES_PERPERF]; // -24~24(0)
	uint8_t LFOSpeed, LFODelay, LFOWave, LFOSync, LFOAMD, LFOPMD;
	char name[20];
	uint8_t IndivOutput; // 0~1(0), (0:off,1:on)
	uint8_t reserved;
	uint8_t ExtTriggerMIDIChannel, ExtTriggerKey, ExtTriggerLevel, ExtTriggerGate;
};

// 146(0x92) bytes
struct TXWVOICE
{
	struct TIMBRE
	{
		uint8_t Number;
		TXWMIDIKEY LoKey; // 16~142(255), MIDI-ENCODED-KEY
		TXWMIDIKEY HiKey; // 16~142(255), MIDI-ENCODED-KEY
		uint8_t Fade; // 0~32(0)
	} timbres[TXW_COUNTOF_TIMBRES_PERVOICE];
	uint8_t reserved0[6];
	char name[10];
	uint8_t EditSlot; // 0~32(0)
	uint8_t reserved1;
};

// 64(0x40) bytes
struct TXWVOICEV2EXTRA
{
	struct TIMBRE
	{
		// original pitch key is 14 bits logf code
		// i.e.
		//   0x0400(1024) => c#-1(13) (C#0 in SPN)
		//   0x13aa(5034) => c3(60)  (C4 in SPN)
		//   0x23aa(9130) => c7(108) (C8 in SPN)
		TXWINT16 original_pitch_key;
		uint8_t getOriginalPitchKey() const
		{
			unsigned int v = original_pitch_key.get();
			return (uint8_t)((v - 5034) * (108 - 60) / (9130 - 5034) + 60);
		}
	} timbres[TXW_COUNTOF_TIMBRES_PERVOICE];
};

// 56(0x38) bytes
struct TXWTIMBRE
{
	uint8_t WaveNumber;
	TXWMIDIKEY OriginalPitch; // 16~142(106), V1 only
	uint8_t FilterNumber;
	uint8_t PitchbendRange; // 0~12(2)
	uint8_t PitchbendStep; // 0~12(0)
	uint8_t LFOSpeed, LFOPMD, LFOAMD;
	uint8_t reserved; // V2:EG rate scalling 0~99(50) ?
	uint8_t filler; // V2:velocity bias mode 0~1(0), (0:normal,1:zerobias) ?
	TXWINT16 tune; // -200~200(0), (TBV: -100~100 cents?)
	uint8_t AEGD1L, AEGD2L, AEGAR, AEGD1R, AEGD2R, AEGRR; // 0~99(99)
	uint8_t PEGR1, PEGR2, PEGR3, PEGR4, PEGL1, PEGL2, PEGL3, PEGL4; // rates:0~99(99), levels:0~99(50)
	uint8_t VCVolume, VCSwitch, VCBreakPoint1, VCLevel1, VCDepth1, VCBreakPoint2, VCLevel2, VCDepth2;
	uint8_t AmpModWheel, AmpModFoot, AmpModAfterTouch, AmpModBreath;
	uint8_t PitchModWheel, PitchModFoot, PitchModAfterTouch, PitchModBreath;
	uint8_t VelModWheel, VelModFoot, VelModAfterTouch, VelModBreath;
	char name[10];
};

// 8(0x08) bytes
struct TXWTIMBREV2EXTRA
{
	TXWINT16 fixpitch; // -48~36(0), (0x7fff:off)
	TXWINT16 oneshot_trigger; // -3000~0(0), (-3000:15000ms,-1:5ms,0:off)
	uint8_t reserved[4];
	int getOneshotTrigger() const
	{
		int v = -oneshot_trigger.get();
		return v * 5;
	}
};

// 16(0x10) bytes
struct TXWWAVE
{
	uint8_t aegdata[6]; // space for the AEG (never mind this)
	uint8_t format; // 0x49 = looped, 0xC9 = non-looped
	uint8_t samplerate; // 1 = 33 kHz, 2 = 50 kHz, 3 = 16 kHz
	uint8_t atc_length[3]; // I'll get to this...
	uint8_t rpt_length[3];
	uint8_t reserved1[2]; // set these to null, to be on the safe side
	uint32_t getAttackLength() const
	{
		return (((uint32_t)atc_length[2] & 0x01) << 16) | ((uint32_t)atc_length[1] << 8) | (uint32_t)atc_length[0];
	}
	uint32_t getRepeatLength() const
	{
		return (((uint32_t)rpt_length[2] & 0x01) << 16) | ((uint32_t)rpt_length[1] << 8) | (uint32_t)rpt_length[0];
	}
	uint32_t getSampleRate() const
	{
		if(samplerate == 1) return 33333;
		if(samplerate == 2) return 50000;
		if(samplerate == 3) return 16667;
		uint16_t w = (((uint16_t)atc_length[2] & 0xfe) << 8) | ((uint16_t)rpt_length[2] & 0xfe);
		if(w == 0x0652) return 33333;
		if(w == 0x1000) return 50000;
		if(w == 0xf652) return 16667;
		return 0;
	}
};

#pragma pack(pop)

// ================================================================================
// utilities

struct TXWUtil
{
	static bool parseFileHeader(std::istream& s, bool* v2)
	{
		static const std::string sig("LM8953");
		TXWFILEHDR fh;
		s.read((char*)&fh, sizeof(fh));
		if(sig != fh.signature) return false;
		*v2 = (fh.version[1] == '2') ? true : false;
		return true;
	}
	template<class T> static std::string extractName(const T& st)
	{
		size_t l = std::size(((T*)0)->name);
		std::string s(l, ' ');
		for(size_t i = 0; i < l; i ++)
		{
			char c = st.name[i];
			s[i] = (32 <= c) ? c : ' ';
		}
		return s;
	}
	static std::string extractWaveName(const TXWWAVENAME& wn)
	{
		unsigned int l = 0; while((l < std::size(wn.sizeinfo)) && (wn.sizeinfo[l] == 0)) l ++;
		if(std::size(wn.sizeinfo) <= l) return ""; // sizeinfo == all zero
		return StrUtil::trim(extractName(wn), " ");
	}
	static bool convertWave(const std::filesystem::path& txwpath, const std::filesystem::path& wavpath, uint8_t orgkey, bool overwrite, std::string* err)
	{
		bool r = false;
		try
		{
			// read
			std::fstream txw(txwpath, std::ios::in | std::ios::binary);
			if(!txw) throw std::runtime_error("failed to open: \"" + txwpath.filename().string() + "\"");
			txw.exceptions(std::ios::badbit | std::ios::eofbit | std::ios::failbit);
			bool iswavev2 = false; if(!parseFileHeader(txw, &iswavev2)) throw std::runtime_error("invalid signature");
			TXWWAVE wave; txw.read((char*)&wave, sizeof(wave));
			uint32_t pcmoffset = (uint32_t)txw.tellg();
			txw.seekg(0, std::ios_base::end);
			uint32_t pcmlength = ((uint32_t)txw.tellg() - pcmoffset) / 3 * 2;
			txw.seekg(pcmoffset);
			uint32_t samplerate = wave.getSampleRate();
			if(samplerate == 0) throw std::runtime_error("invalid samplerate");
			uint32_t lattack = wave.getAttackLength(), lrepeat = wave.getRepeatLength();
			uint32_t loopbegin = lattack;
			uint32_t loopend = lattack + lrepeat - 1; // includes the last sample (TBV)
			bool looped = (wave.format & 0x80) ? false : true;
			// write
			if(!overwrite && std::filesystem::exists(wavpath)) throw std::runtime_error("path exists");
			RiffWriter wav(wavpath);
			if(!wav) throw std::runtime_error("failed to create: \"" + wavpath.filename().string() + "\"");
			wav.write("WAVE", 4);
			{
				RiffWriter::ScopedDescend sd(wav, "fmt ");
				WaveFormatEx wf = {};
				wf.wFormatTag = WaveFormatPcm;
				wf.nChannels = 1;
				wf.nSamplesPerSec = samplerate;
				wf.nAvgBytesPerSec = samplerate * 2;
				wf.nBlockAlign = 2;
				wf.wBitsPerSample = 16;
				wav.write(&wf, sizeof(wf));
			}
			{
				RiffWriter::ScopedDescend sd(wav, "smpl");
				SamplerInfo si = {};
				si.dwSamplePeriod = (uint32_t)(1000000000ui64 / samplerate);
				si.dwMIDIUnityNote = orgkey;
				si.cSampleLoops = looped ? 1 : 0;
				wav.write(&si, sizeof(si));
				if(looped)
				{
					SamplerLoop sl = {};
					sl.dwType = LoopTypeForward;
					sl.dwStart = loopbegin;
					sl.dwEnd = loopend;
					wav.write(&sl, sizeof(sl));
				}
			}
			{
				RiffWriter::ScopedDescend sd(wav, "data");
				uint8_t bb[3]; uint16_t bw[2];
				for(unsigned int is = 0; is < pcmlength; is += 2)
				{
					txw.read((char*)bb, 3);
					bw[0] = ((uint16_t)bb[0] << 8) | ((uint16_t)bb[1] & 0xF0);
					bw[1] = ((uint16_t)bb[2] << 8) | (((uint16_t)bb[1] & 0x0f) << 4);
					wav.write(bw, sizeof(bw));
				}
			}
			r = true;
		}
		catch(std::exception& e)
		{
			*err = e.what();
			r = false;
		}
		return r;
	}
};
