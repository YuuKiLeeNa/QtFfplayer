#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include "libavutil/frame.h"
#include "libavcodec/avcodec.h"
#include "libavcodec/packet.h"
#include "libavutil/audio_fifo.h"
#ifdef __cplusplus
}
#endif
#include<memory>

struct free_pkt
{
	void operator()(AVPacket* pkt) {av_packet_free(&pkt); }
};

struct free_frame 
{
	void operator()(AVFrame* f) { av_frame_free(&f); }
};

struct free_audiofifo 
{
	void operator()(AVAudioFifo* fifo) { av_audio_fifo_free(fifo); }
};



typedef std::unique_ptr<AVPacket, free_pkt>UPTR_PKT;
typedef std::unique_ptr<AVFrame, free_frame>UPTR_FME;
typedef std::unique_ptr<AVAudioFifo, free_audiofifo>UPTR_AFIFO;

