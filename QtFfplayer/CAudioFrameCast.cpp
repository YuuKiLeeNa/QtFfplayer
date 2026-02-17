#include "CAudioFrameCast.h"
#ifdef __cplusplus
extern "C"
{
#endif
#include "libavutil/audio_fifo.h"
#include "libavcodec/avcodec.h"
#include "libswresample/swresample.h"
#include <assert.h>
#ifdef __cplusplus
}
#endif
#include<memory>
#include<functional>

CAudioFrameCast::CAudioFrameCast()
{
}

CAudioFrameCast::~CAudioFrameCast() 
{
	reset();
}

void CAudioFrameCast::reset()
{
	if(m_SwrContext)
		swr_free(&m_SwrContext);
	if (m_fifo)
	{
		av_audio_fifo_free(m_fifo);
		m_fifo = nullptr;
	}
	m_accuSamples = 0;
}

bool CAudioFrameCast::translate(AVFrame* f)//, AVCodecContext* decContext, AVCodecContext* encContext)
{
	if (!m_fifo)
		return false;
	if (!f)
		return true;

	//std::vector<AVFrame*>res;
	std::unique_ptr<AVFrame, std::function<void(AVFrame*)>>free_frame(f, [](AVFrame*f)
		{
			//av_frame_unref(f);
			av_frame_free(&f);
		});

	int64_t nb_samples = av_rescale_q_rnd(f->nb_samples+swr_get_delay(m_SwrContext, m_iInSampleRate), { 1, m_iInSampleRate }, { 1, m_iOutSampleRate }, AVRounding::AV_ROUND_UP);
	uint8_t** pBuff = (uint8_t**)calloc(m_iInChannels, nb_samples);
	if (pBuff = nullptr) 
		return false;
	int ret = av_samples_alloc(pBuff, nullptr, m_iOutChannels, nb_samples, m_outFormat, 1);
	assert(ret == av_samples_get_buffer_size(nullptr, m_iOutChannels, nb_samples, m_outFormat, 1));
		
	if (ret <= 0)
	{
		free(pBuff);
		char szErr[128];
		printf("av_samples_alloc error:%s\n", av_make_error_string(szErr,sizeof(szErr), ret));
		return false;
	}
	std::unique_ptr<uint8_t*, std::function<void(uint8_t**)>>freepBuf(pBuff, [](uint8_t**p)
		{
			av_freep(&(*p)[0]);
			free(p);
		});
	if ((ret = swr_convert(m_SwrContext, pBuff, nb_samples, (const uint8_t**)f->extended_data, f->nb_samples)) < 0)
	{
		char szErr[128];
		printf("swr_convert error:%s\n", av_make_error_string(szErr, sizeof(szErr), ret));
		return false;
	}
	assert(ret == nb_samples);

	if (av_audio_fifo_realloc(m_fifo, ret + av_audio_fifo_size(m_fifo)) != 0)
	{
		printf("av_audio_fifo_realloc error\n");
		return false;
	}
	if ((ret = av_audio_fifo_write(m_fifo, (void**)pBuff, nb_samples)) < 0) 
	{
		char szErr[128];
		printf("av_audio_fifo_write error:%s\n", av_make_error_string(szErr, sizeof(szErr), ret));
		return false;
	}
	return true;
}

AVFrame* CAudioFrameCast::getFrame(int samples) 
{
	int fifoSize = av_audio_fifo_size(m_fifo);
	if(fifoSize > 0 && fifoSize > samples)
	{
		AVFrame* tmp = av_frame_alloc();
		std::unique_ptr<AVFrame, std::function<void(AVFrame*)>>free_frame_tmp(tmp, [](AVFrame*f)
			{
				//av_frame_unref(f);
				av_frame_free(&f);
			});

		tmp->channels = m_iOutChannels;
		tmp->channel_layout = m_iOutChannelLayout;
		tmp->sample_rate = m_iOutSampleRate;
		tmp->nb_samples = (samples <= 0 ? fifoSize: samples);
		tmp->format = m_outFormat;
		
		
		int ret = av_frame_get_buffer(tmp, 1);
		if (ret != 0) 
		{
			char szErr[128];
			printf("av_frame_get_buffer error:%s\n", av_make_error_string(szErr, sizeof(szErr), ret));
			return nullptr;
		}
		if ((ret = av_frame_make_writable(tmp)) != 0) 
		{
			char szErr[128];
			printf("av_frame_make_writable:%s\n", av_make_error_string(szErr, sizeof(szErr), ret));
			return nullptr;
		}
		ret = av_audio_fifo_read(m_fifo, (void**)tmp->extended_data, tmp->nb_samples);
		if (ret < 0)
		{
			char szErr[128];
			printf("av_audio_fifo_read error:%s\n", av_make_error_string(szErr, sizeof(szErr), ret));
			return nullptr;
		}
		tmp->pts = m_accuSamples;
		m_accuSamples += tmp->nb_samples;
		return free_frame_tmp.release();
	}
	return nullptr;
}

bool CAudioFrameCast::set(int iInChannels, AVSampleFormat inFormat, int iInSampleRate, int iOutChannels, AVSampleFormat outFormat, int iOutSampleRate)
{
	reset();
	m_iInChannelLayout = av_get_default_channel_layout(iInChannels);//iInChannelLayout;
	m_inFormat = inFormat;
	m_iInSampleRate = iInSampleRate;
	m_iInChannels = iInChannels;//av_get_channel_layout_nb_channels(m_iInChannelLayout);

	m_iOutChannelLayout = av_get_default_channel_layout(iOutChannels);//iOutChannelLayout;
	m_outFormat = outFormat;
	m_iOutSampleRate = iOutSampleRate;
	m_iOutChannels = iOutChannels;//av_get_channel_layout_nb_channels(m_iOutChannelLayout);
	m_fifo = av_audio_fifo_alloc(m_outFormat, m_iOutChannels, 1);
	m_SwrContext = swr_alloc_set_opts(nullptr, m_iOutChannelLayout, m_outFormat, m_iOutSampleRate, m_iInChannelLayout, m_inFormat, m_iInSampleRate, 0, nullptr);

	if (!m_SwrContext)
	{
		av_audio_fifo_free(m_fifo);
		return false;
	}
	if (swr_init(m_SwrContext) != 0)
	{
		swr_free(&m_SwrContext);
		av_audio_fifo_free(m_fifo);
		return false;
	}
	return true;
}
