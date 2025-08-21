#pragma once
#include<map>
#include<vector>
#include<utility>
#include<stdint.h>
#ifdef __cplusplus
extern "C"
{
#endif
#include "libavutil/samplefmt.h"
#ifdef __cplusplus
}
#endif

struct AVFrame;
struct AVCodecContext;
struct AVAudioFifo;
struct SwrContext;

class CAudioFrameCast 
{
public:
	CAudioFrameCast();
	~CAudioFrameCast();
	void reset();
	bool translate(AVFrame* f);//, AVCodecContext* decContext,AVCodecContext*encContext);
	AVFrame* getFrame(int samples);
	bool set(int iInChannels, AVSampleFormat inFormat, int iInSampleRate, int iOutChannels, AVSampleFormat outFormat, int iOutSampleRate);
	
protected:
	uint64_t m_iInChannelLayout;
	AVSampleFormat m_inFormat;
	int m_iInSampleRate;
	int m_iInChannels;

	uint64_t m_iOutChannelLayout;
	AVSampleFormat m_outFormat;
	int m_iOutSampleRate;
	int m_iOutChannels;

	SwrContext* m_SwrContext = nullptr;
	AVAudioFifo* m_fifo = nullptr;
	int64_t m_accuSamples = 0;
};