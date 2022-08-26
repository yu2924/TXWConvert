//
//  wavfmt.h
//  TXWConvert
//
//  Created by yu2924 on 2018-08-15
//

#pragma once

#include <stdint.h>

#pragma pack(push, 1)

enum
{
	WaveFormatPcm = 1,
	WaveFormatIeeeFloat = 3,
};

struct WaveFormat
{
	uint16_t wFormatTag;		// format type
	uint16_t nChannels;			// number of channels (i.e. mono, stereo...)
	uint32_t nSamplesPerSec;	// sample rate
	uint32_t nAvgBytesPerSec;	// for buffer estimation
	uint16_t nBlockAlign;		// block size of data
};

struct PcmWaveFormat
{
	uint16_t wFormatTag;		// format type
	uint16_t nChannels;			// number of channels (i.e. mono, stereo...)
	uint32_t nSamplesPerSec;	// sample rate
	uint32_t nAvgBytesPerSec;	// for buffer estimation
	uint16_t nBlockAlign;		// block size of data
	uint16_t wBitsPerSample;	// Number of bits per sample of mono data
};

struct WaveFormatEx
{
	uint16_t wFormatTag;		// format type
	uint16_t nChannels;			// number of channels (i.e. mono, stereo...)
	uint32_t nSamplesPerSec;	// sample rate
	uint32_t nAvgBytesPerSec;	// for buffer estimation
	uint16_t nBlockAlign;		// block size of data
	uint16_t wBitsPerSample;	// Number of bits per sample of mono data
	uint16_t cbSize;			// The count in bytes of the size of extra information (after cbSize)
};

enum LoopType
{
	LoopTypeForward		= 0, // forward (normal)
	LoopTypeAlternating	= 1, // alternating (forward/backward)
	LoopTypeBackward	= 2, // backward
	LoopTypeUser		= 32, // sampler specific types (manufacturer defined)
};

struct SamplerLoop
{
	uint32_t dwIdentifier;
	uint32_t dwType;
	uint32_t dwStart;
	uint32_t dwEnd;
	uint32_t dwFraction;
	uint32_t dwPlayCount;
};

struct SamplerInfo
{
	uint32_t dwManufacturer;
	uint32_t dwProduct;
	uint32_t dwSamplePeriod;
	uint32_t dwMIDIUnityNote;
	uint32_t dwMIDIPitchFraction;
	uint32_t dwSMPTEFormat;
	uint32_t dwSMPTEOffset;
	uint32_t cSampleLoops;
	uint32_t cbSamplerData;
	// SamplerLoop Loops[];
};

#pragma pack(pop)
