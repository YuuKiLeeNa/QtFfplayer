#pragma once


/*
 * Copyright (c) 2003 Fabrice Bellard
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

 /**
  * @file
  * simple media player based on the FFmpeg libraries
  */

  //#include "config.h"
#ifdef __cplusplus
extern"C" {
#endif
#include <inttypes.h>
#include <math.h>
#include <limits.h>
#include <signal.h>
#include <stdint.h>

#include "libavutil/avstring.h"
#include "libavutil/eval.h"
#include "libavutil/mathematics.h"
#include "libavutil/pixdesc.h"
#include "libavutil/imgutils.h"
#include "libavutil/dict.h"
#include "libavutil/fifo.h"
#include "libavutil/parseutils.h"
#include "libavutil/samplefmt.h"
#include "libavutil/avassert.h"
#include "libavutil/time.h"
#include "libavutil/bprint.h"
#include "libavformat/avformat.h"
#include "libavdevice/avdevice.h"
#include "libswscale/swscale.h"
#include "libavutil/opt.h"
#include "libavcodec/avfft.h"
#include "libswresample/swresample.h"
#include <stdint.h>

#include "libavcodec/avcodec.h"
#include "libavfilter/avfilter.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#if CONFIG_AVFILTER
# include "libavfilter/avfilter.h"
# include "libavfilter/buffersink.h"
# include "libavfilter/buffersrc.h"
#endif

#include <SDL.h>
#include <SDL_thread.h>

//#include "cmdutils.h"

#include <assert.h>

#if HAVE_COMMANDLINETOARGVW && defined(_WIN32)
#include <shellapi.h>
#endif

#ifdef __cplusplus
}
#endif
#include<functional>
#include<atomic>
#include"SoundTouchDLL.h"
#include<list>
//#include "CMergeMediaPkt.h"
#include<condition_variable>
#include<mutex>
class ffplay
{
public:
    struct Frame;
    void start(int argc, char**argv);
    //void setCallBackGetWndWidthHeight(const std::function<std::pair<int,int>(void)>&callBackGetWidthHeight);
    std::function<std::pair<int, int>(void)>m_callBackGetWidthHeight;
    void setCallBackGetFrame(const std::function<void(Frame*)>& callBackGetAVFrame);
    std::function<void(Frame*)>m_callBackGetAVFame;
    void setExitThread(bool b);
    void setGetVideoLength(const std::function<void(int64_t)>&callBackGetVideoLength);
    void setGetCurTime(const std::function<void(int64_t)>& callBackGetCurTime);
    void setReqeustSeek(long long pos, long long incr);
    void setNoticeBeginAndEnd(const std::function<void()>&beg, const std::function<void()>& end);

    void callEndFun();
    void callBeginFun();
    void judegSendEndFunSignal();
    void setCallBackGetVolume(const std::function<int()>&fun);
    static enum AVPixelFormat get_format(struct AVCodecContext* s, const enum AVPixelFormat* fmt);
    void resetData()
    {
        isss.reset();
    }
    void setAllClockSpeed(double dSpeed);
    double getClockSpeed()const;
    void setGetSpeedCallBack(const std::function<double()>&fun) 
    {
        m_getSpeedCallBack = std::make_unique<std::decay<decltype(fun)>::type>(fun);
    }
    double calcNumOfSamplesInAudioBuffer(int iRealBytes);
    int hw_decoder_init(AVCodecContext* ctx, const enum AVHWDeviceType type);

    std::atomic_bool m_isExitThread = false;
    std::unique_ptr<std::function<void(int64_t)>>m_ptrFunGetVideoLength;
    std::unique_ptr<std::function<void(int64_t)>>m_ptrFunGetCurTime;
    std::atomic_bool m_bReqSeekSuspend = false;
    std::unique_ptr<std::function<void()>>m_beginCallBack;
    std::unique_ptr<std::function<void()>>m_endCallBack;
    std::unique_ptr<std::function<int()>>m_getVolumnCallBack;
    std::unique_ptr<std::remove_pointer<HANDLE>::type,std::function<void(HANDLE p)>> m_st;
    std::unique_ptr<std::function<double()>>m_getSpeedCallBack;
    std::atomic_int64_t m_seekTime = AV_NOPTS_VALUE;
    double m_dSavePrevAudioClock;
    std::list<std::pair<int,double>>m_listRealBytesMapToAudioSrcBytes;
    AVPixelFormat m_swpix = AV_PIX_FMT_NONE;

    //std::atomic_bool m_bSaveMedia = false;
    //std::string m_strSaveFileName;// = "E:\\saveMedia.ts";

    std::mutex m_mutexProtectedRunStart;
    std::condition_variable m_cond;
    bool m_bIsRunStart;


#ifdef DELAY_ANALYSIS
    void setDelayCallBack(const std::function<void(double)>& fun) 
    {
        m_funDelayCallBack.reset(new std::decay<decltype(fun)>::type(fun));
    }
    void setUndulateCallBack(const std::function<void(double)>& fun) 
    {
        m_funUndulateCallBack.reset(new std::decay<decltype(fun)>::type(fun));
    }
    void setAudioDataCallBack(const std::function<void(double)>& fun) 
    {
        m_funAudioDataCallBack.reset(new std::decay<decltype(fun)>::type(fun));
    }
    void setVideoDataCallBack(const std::function<void(double)>& fun) 
    {
        m_funVideoDataCallBack.reset(new std::decay<decltype(fun)>::type(fun));
    }

    void setAudioDataFrameCallBack(const std::function<void(double)>& fun)
    {
        m_funAudioDataFrameCallBack.reset(new std::decay<decltype(fun)>::type(fun));
    }
    void setVideoDataFrameCallBack(const std::function<void(double)>& fun)
    {
        m_funVideoDataFrameCallBack.reset(new std::decay<decltype(fun)>::type(fun));
    }

    void setBufferCallBack(const std::function<void(double)>& fun) 
    {
        m_funBufferCallBack.reset(new std::decay<decltype(fun)>::type(fun));
    }
    void setSDLFeedBufferCallBack(const std::function<void(double)>& fun) 
    {
        m_funSDLFeedBufferCallBack.reset(new std::decay<decltype(fun)>::type(fun));
    }
    void setSoundTouchTranslateBufferCallBack(const std::function<void(double)>& fun) 
    {
        m_funSoundTouchTranslateBufferCallBack.reset(new std::decay<decltype(fun)>::type(fun));
    }
    void setSoundTouchUnprocessedBufferCallBack(const std::function<void(double)>& fun) 
    {
        m_funSoundTouchUnprocessedBufferCallBack.reset(new std::decay<decltype(fun)>::type(fun));
    }
    void setTotalDelayCallBack(const std::function<void(double)>& fun) 
    {
        m_funTotalDelayCallBack.reset(new std::decay<decltype(fun)>::type(fun));
    }
    void setSyncTypeCallBack(const std::function<void(int)>& fun)
    {
        m_funSyncTypeCallBack.reset(new std::decay<decltype(fun)>::type(fun));
    }

    std::unique_ptr<std::function<void(double)>>m_funDelayCallBack;
    std::unique_ptr<std::function<void(double)>>m_funUndulateCallBack;
    std::unique_ptr<std::function<void(double)>>m_funAudioDataCallBack;
    std::unique_ptr<std::function<void(double)>>m_funVideoDataCallBack;
    std::unique_ptr<std::function<void(double)>>m_funAudioDataFrameCallBack;
    std::unique_ptr<std::function<void(double)>>m_funVideoDataFrameCallBack;
    std::unique_ptr<std::function<void(double)>>m_funBufferCallBack;
    std::unique_ptr<std::function<void(double)>>m_funSDLFeedBufferCallBack;
    std::unique_ptr<std::function<void(double)>>m_funSoundTouchTranslateBufferCallBack;
    std::unique_ptr<std::function<void(double)>>m_funSoundTouchUnprocessedBufferCallBack;
    std::unique_ptr<std::function<void(double)>>m_funTotalDelayCallBack;
    std::unique_ptr<std::function<void(int)>>m_funSyncTypeCallBack;

#define Delay(X) (*m_funDelayCallBack)(X)
#define Undulate(X) (*m_funUndulateCallBack)(X)
#define AudioData(X) (*m_funAudioDataCallBack)(X)
#define VideoData(X) (*m_funVideoDataCallBack)(X)
#define AudioDataFrame(X) (*m_funAudioDataFrameCallBack)(X)
#define VideoDataFrame(X) (*m_funVideoDataFrameCallBack)(X)
#define Buffer(X) (*m_funBufferCallBack)(X)
#define Buffer_ff(X,O) (*O->m_funBufferCallBack)(X)

#define SDLFeedBuffer(X) (*m_funSDLFeedBufferCallBack)(X)
#define SDLFeedBuffer_ff(X,O) (*O->m_funSDLFeedBufferCallBack)(X)

#define SoundTouchTranslateBuffer(X) (*m_funSoundTouchTranslateBufferCallBack)(X)
#define SoundTouchTranslateBuffer_ff(X,O) (*O->m_funSoundTouchTranslateBufferCallBack)(X)

#define SoundTouchUnprocessedBuffer(X) (*m_funSoundTouchUnprocessedBufferCallBack)(X)
#define SoundTouchUnprocessedBuffer_ff(X,O) (*O->m_funSoundTouchUnprocessedBufferCallBack)(X)

#define TotalDelay(X) (*m_funTotalDelayCallBack)(X)
#define SyncType(X) (*m_funSyncTypeCallBack)(X)
#define SyncType_ff(X,O) (*O->m_funSyncTypeCallBack)(X)

#define SEND_PACKET_QUEUE_SIZE(que) \
    double t;\
    if (que == &isss->videoq)\
    {\
        t = que->duration * av_q2d(isss->video_st->time_base)*1000;\
        VideoData(t);\
    }\
    else if (que == &isss->audioq)\
    {\
        t = que->duration * av_q2d(isss->audio_st->time_base)*1000;\
        AudioData(t);\
    }

#define FLUSH_PACKET_QUEUE_SIZE(que)\
    double t=0.0;\
    if (que == &isss->videoq)\
    {\
        VideoData(t);\
    }\
    else if (que == &isss->audioq)\
    {\
        AudioData(t);\
    }


#define SEND_FRAME_QUEUE_SIZE(f) \
    if (f == &isss->sampq || f == &isss->pictq)\
    {\
        double t = 0.0;\
        int queLeftSize = f->size - f->rindex_shown;\
        int index = f->rindex + f->rindex_shown;\
        while (queLeftSize > 0)\
        {\
            t += f->queue[index % f->max_size].duration*1000;\
            --queLeftSize;\
            ++index;\
        }\
        if (f == &isss->sampq)\
            AudioDataFrame(t);\
        else\
            VideoDataFrame(t);\
    }

#define FLUSH_FRAME_QUEUE_SIZE(f) \
    if (f == &isss->sampq || f == &isss->pictq)\
    {\
        double t = 0.0;\
        if (f == &isss->sampq)\
            AudioDataFrame(t);\
        else\
            VideoDataFrame(t);\
    }



#else
#define Delay(X) 
#define Undulate(X) 
#define AudioData(X) 
#define VideoData(X) 
#define AudioDataFrame(X) 
#define VideoDataFrame(X) 
#define Buffer(X) 
#define Buffer_ff(X,O) 
#define SDLFeedBuffer(X) 
#define SDLFeedBuffer_ff(X,O)

#define SoundTouchTranslateBuffer(X)
#define SoundTouchTranslateBuffer_ff(X,O) 

#define SoundTouchUnprocessedBuffer(X) 
#define SoundTouchUnprocessedBuffer_ff(X,O) 
#define TotalDelay(X) 
#define SyncType(X)
#define SyncType_ff(X,O)
#define SEND_PACKET_QUEUE_SIZE(que) 
#define SEND_FRAME_QUEUE_SIZE(f) 
#define FLUSH_FRAME_QUEUE_SIZE(f) 
#endif





//static std::vector<const AVCodecHWConfig*>m_vHWConfig;
//void start(const char* filePath);
    const char program_name[64] = "ffplay";
    const int program_birth_year = 2003;
#define MAX_QUEUE_SIZE (15 * 1024 * 1024)
#define MIN_FRAMES 25
#define EXTERNAL_CLOCK_MIN_FRAMES 2
#define EXTERNAL_CLOCK_MAX_FRAMES 10

    /* Minimum SDL audio buffer size, in samples. */
#define SDL_AUDIO_MIN_BUFFER_SIZE 512
/* Calculate actual buffer size keeping in mind not cause too frequent audio callbacks */
#define SDL_AUDIO_MAX_CALLBACKS_PER_SEC 30

/* Step size for volume control in dB */
#define SDL_VOLUME_STEP (0.75)

/* no AV sync correction is done if below the minimum AV sync threshold */
#define AV_SYNC_THRESHOLD_MIN 0.04
/* AV sync correction is done if above the maximum AV sync threshold */
#define AV_SYNC_THRESHOLD_MAX 0.1
/* If a frame duration is longer than this, it will not be duplicated to compensate AV sync */
#define AV_SYNC_FRAMEDUP_THRESHOLD 0.1
/* no AV correction is done if too big error */
#define AV_NOSYNC_THRESHOLD 10.0

/* maximum audio speed change to get correct sync */
#define SAMPLE_CORRECTION_PERCENT_MAX 10

/* external clock speed adjustment constants for realtime sources based on buffer fullness */
#define EXTERNAL_CLOCK_SPEED_MIN  0.900
#define EXTERNAL_CLOCK_SPEED_MAX  1.010
#define EXTERNAL_CLOCK_SPEED_STEP 0.001

/* we use about AUDIO_DIFF_AVG_NB A-V differences to make the average */
#define AUDIO_DIFF_AVG_NB   20

/* polls for possible required screen refresh at least this often, should be less than 1/fps */
#define REFRESH_RATE 0.01

/* NOTE: the size must be big enough to compensate the hardware audio buffersize size */
/* TODO: We assume that a decoded and resampled frame fits into this buffer */
#define SAMPLE_ARRAY_SIZE (8 * 65536)

#define CURSOR_HIDE_DELAY 1000000

#define USE_ONEPASS_SUBTITLE_RENDER 1
    //synchronize_audio
#define AUDIO_SYNCHRONIZE_BYTES 512

    unsigned sws_flags = SWS_BICUBIC;

    typedef struct MyAVPacketList {
        AVPacket* pkt;
        int serial;
    } MyAVPacketList;

    typedef struct PacketQueue {
        AVFifoBuffer* pkt_list;
        int nb_packets;
        int size;
        int64_t duration;
        int abort_request;
        int serial;
        SDL_mutex* mutex;
        SDL_cond* cond;
    } PacketQueue;

#define VIDEO_PICTURE_QUEUE_SIZE 3
#define SUBPICTURE_QUEUE_SIZE 16
#define SAMPLE_QUEUE_SIZE 9
#define FRAME_QUEUE_SIZE FFMAX(SAMPLE_QUEUE_SIZE, FFMAX(VIDEO_PICTURE_QUEUE_SIZE, SUBPICTURE_QUEUE_SIZE))

    typedef struct AudioParams {
        int freq;
        int channels;
        int64_t channel_layout;
        enum AVSampleFormat fmt;
        int frame_size;
        int bytes_per_sec;
    } AudioParams;

    typedef struct Clock {
        double pts;           /* clock base */
        double pts_drift;     /* clock base minus time at which we updated the clock */
        double last_updated;
        std::atomic<double>speed = 1.0;
        int serial;           /* clock is based on a packet with this serial */
        int paused;
        int* queue_serial;    /* pointer to the current packet queue serial, used for obsolete clock detection */
    } Clock;

    /* Common struct for handling all types of decoded data and allocated render buffers. */
    typedef struct Frame {
        AVFrame* frame;
        AVSubtitle sub;
        int serial;
        double pts;           /* presentation timestamp for the frame */
        double duration;      /* estimated duration of the frame */
        int64_t pos;          /* byte position of the frame in the input file */
        int width;
        int height;
        int format;
        AVRational sar;
        int uploaded;
        int flip_v;
    } Frame;

    typedef struct FrameQueue {
        Frame queue[FRAME_QUEUE_SIZE];
        int rindex;
        int windex;
        int size;
        int max_size;
        int keep_last;
        int rindex_shown;
        SDL_mutex* mutex;
        SDL_cond* cond;
        PacketQueue* pktq;
    } FrameQueue;

    enum {
        AV_SYNC_AUDIO_MASTER, /* default choice */
        AV_SYNC_VIDEO_MASTER,
        AV_SYNC_EXTERNAL_CLOCK, /* synchronize to an external clock */
    };

    typedef struct Decoder {
        AVPacket* pkt;
        PacketQueue* queue;
        AVCodecContext* avctx;
        int pkt_serial;
        int finished;
        int packet_pending;
        SDL_cond* empty_queue_cond;
        int64_t start_pts;
        AVRational start_pts_tb;
        int64_t next_pts;
        AVRational next_pts_tb;
        SDL_Thread* decoder_tid;
        std::atomic_bool isReadAllFrame = false;
    } Decoder;
    enum ShowMode {
        SHOW_MODE_NONE = -1, SHOW_MODE_VIDEO = 0, SHOW_MODE_WAVES, SHOW_MODE_RDFT, SHOW_MODE_NB
    };
    typedef struct VideoState {
        SDL_Thread* read_tid;
        const AVInputFormat* iformat;
        int abort_request;
        int force_refresh;
        int paused;
        int last_paused;
        int queue_attachments_req;
        int seek_req;
        int seek_flags;
        int64_t seek_pos;
        int64_t seek_rel;
        int read_pause_return;
        AVFormatContext* ic;
        int realtime;

        Clock audclk;
        Clock vidclk;
        Clock extclk;

        FrameQueue pictq;
        FrameQueue subpq;
        FrameQueue sampq;

        Decoder auddec;
        Decoder viddec;
        Decoder subdec;


        int audio_stream;

        std::atomic_int av_sync_type;

        double audio_clock;
        int audio_clock_serial;
        double audio_diff_cum; /* used for AV difference average computation */
        double audio_diff_avg_coef;
        double audio_diff_threshold;
        int audio_diff_avg_count;
        AVStream* audio_st;
        PacketQueue audioq;
        int audio_hw_buf_size;
        uint8_t* audio_buf;
        uint8_t* audio_buf1;

        uint8_t* audio_sound_touch_buf = nullptr;

        unsigned int audio_buf_size; /* in bytes */
        unsigned int audio_buf1_size;

        unsigned int audio_sound_touch_data_size = 0;
        unsigned int audio_sound_touch_data_off = 0;

        int audio_buf_index; /* in bytes */
        int audio_write_buf_size;
        std::atomic_int audio_volume;
        int muted;
        /*struct*/ AudioParams audio_src;
#if CONFIG_AVFILTER
        /*struct*/ AudioParams audio_filter_src;
#endif
        /*struct*/ AudioParams audio_tgt;
        struct SwrContext* swr_ctx;
        int frame_drops_early;
        int frame_drops_late;
        ShowMode show_mode;
        /*enum ShowMode {
            SHOW_MODE_NONE = -1, SHOW_MODE_VIDEO = 0, SHOW_MODE_WAVES, SHOW_MODE_RDFT, SHOW_MODE_NB
        } show_mode;*/
        int16_t sample_array[SAMPLE_ARRAY_SIZE];
        int sample_array_index;
        int last_i_start;
        RDFTContext* rdft;
        int rdft_bits;
        FFTSample* rdft_data;
        int xpos;
        double last_vis_time;
        SDL_Texture* vis_texture;
        SDL_Texture* sub_texture;
        SDL_Texture* vid_texture;

        int subtitle_stream;
        AVStream* subtitle_st;
        PacketQueue subtitleq;

        double frame_timer;
        double frame_last_returned_time;
        double frame_last_filter_delay;
        int video_stream;
        AVStream* video_st;
        PacketQueue videoq;
        double max_frame_duration;      // maximum duration of a frame - above this, we consider the jump a timestamp discontinuity
        struct SwsContext* img_convert_ctx;
        struct SwsContext* sub_convert_ctx;
        int eof;

        char* filename;
        int width, height, xleft, ytop;
        int step;

#if CONFIG_AVFILTER
        int vfilter_idx;
        AVFilterContext* in_video_filter;   // the first filter in the video chain
        AVFilterContext* out_video_filter;  // the last filter in the video chain
        AVFilterContext* in_audio_filter;   // the first filter in the audio chain
        AVFilterContext* out_audio_filter;  // the last filter in the audio chain
        AVFilterGraph* agraph;              // audio filter graph
#endif

        int last_video_stream, last_audio_stream, last_subtitle_stream;

        SDL_cond* continue_read_thread;
    } VideoState;
    std::unique_ptr<VideoState, std::function<void(VideoState*)>> isss;// = nullptr;


    //using ShowMode = VideoState::ShowMode;
    typedef struct OptionDef {
        const char* name;
        int flags;
#define HAS_ARG    0x0001
#define OPT_BOOL   0x0002
#define OPT_EXPERT 0x0004
#define OPT_STRING 0x0008
#define OPT_VIDEO  0x0010
#define OPT_AUDIO  0x0020
#define OPT_INT    0x0080
#define OPT_FLOAT  0x0100
#define OPT_SUBTITLE 0x0200
#define OPT_INT64  0x0400
#define OPT_EXIT   0x0800
#define OPT_DATA   0x1000
#define OPT_PERFILE  0x2000     /* the option is per-file (currently ffmpeg-only).
                                   implied by OPT_OFFSET or OPT_SPEC */
#define OPT_OFFSET 0x4000       /* option is specified as an offset in a passed optctx */
#define OPT_SPEC   0x8000       /* option is to be stored in an array of SpecifierOpt.
                                   Implies OPT_OFFSET. Next element after the offset is
                                   an int containing element count in the array. */
#define OPT_TIME  0x10000
#define OPT_DOUBLE 0x20000
#define OPT_INPUT  0x40000
#define OPT_OUTPUT 0x80000
        union OptionDefUION {
            OptionDefUION() {}
            OptionDefUION(int (ffplay::* func_arg1)(void*, const char*, const char*)) :func_arg(func_arg1) {}
            OptionDefUION(void* ptr):dst_ptr(ptr) {}
            OptionDefUION(size_t t) :off(t) {}
            void* dst_ptr;
            int (ffplay::* func_arg)(void*, const char*, const char*);
            size_t off;
        } u;
        const char* help;
        const char* argname;
    } OptionDef;
    /**
     * An option extracted from the commandline.
     * Cannot use AVDictionary because of options like -map which can be
     * used multiple times.
     */


    typedef struct Option {
        const OptionDef* opt;
        const char* key;
        const char* val;
    } Option;

    typedef struct OptionGroupDef {
        /**< group name */
        const char* name;
        /**
         * Option to be used as group separator. Can be NULL for groups which
         * are terminated by a non-option argument (e.g. ffmpeg output files)
         */
        const char* sep;
        /**
         * Option flags that must be set on each option that is
         * applied to this group
         */
        int flags;
    } OptionGroupDef;

    typedef struct OptionGroup {
        const OptionGroupDef* group_def;
        const char* arg;

        Option* opts;
        int  nb_opts;

        AVDictionary* codec_opts;
        AVDictionary* format_opts;
        AVDictionary* resample_opts;
        AVDictionary* sws_dict;
        AVDictionary* swr_opts;
    } OptionGroup;

    /**
     * A list of option groups that all have the same group type
     * (e.g. input files or output files)
     */
    typedef struct OptionGroupList {
        const OptionGroupDef* group_def;

        OptionGroup* groups;
        int       nb_groups;
    } OptionGroupList;

    typedef struct OptionParseContext {
        OptionGroup global_opts;

        OptionGroupList* groups;
        int           nb_groups;

        /* parsing state */
        OptionGroup cur_group;
    } OptionParseContext;




    /* options specified by the user */
    const AVInputFormat* file_iformat = nullptr;
    const char* input_filename = nullptr;
    const char* window_title = nullptr;
    int default_width = 640;
    int default_height = 480;
    int screen_width = 0;
    int screen_height = 0;
    int screen_left = SDL_WINDOWPOS_CENTERED;
    int screen_top = SDL_WINDOWPOS_CENTERED;
    int audio_disable = 0;
    int video_disable = 0;
    int subtitle_disable = 0;
    const char* wanted_stream_spec[AVMEDIA_TYPE_NB] = { 0 };
    int seek_by_bytes = -1;
    float seek_interval = 10;
    int display_disable = 0;
    int borderless;
    int alwaysontop;
    int startup_volume = 100;
    int show_status = -1;
    std::atomic_int av_sync_type = AV_SYNC_AUDIO_MASTER;
    int64_t start_time = AV_NOPTS_VALUE;
    int64_t duration = AV_NOPTS_VALUE;
    int fast = 0;
    int genpts = 0;
    int lowres = 0;
    int decoder_reorder_pts = -1;
    int autoexit;
    int exit_on_keydown;
    int exit_on_mousedown;
    int loop = 1;
    int framedrop = -1;
    int infinite_buffer = -1;
    ShowMode show_mode = ShowMode::SHOW_MODE_NONE;
    const char* audio_codec_name = nullptr;
    const char* subtitle_codec_name = nullptr;
    const char* video_codec_name = nullptr;
    double rdftspeed = 0.02;
    int64_t cursor_last_shown;
    int cursor_hidden = 0;
#if CONFIG_AVFILTER
    const char** vfilters_list = NULL;
    int nb_vfilters = 0;
    const char* afilters = "[in]volume=volume=1.0[out]";//NULL;
#endif
    int autorotate = 1;
    int find_stream_info = 1;
    int filter_nbthreads = 0;

    /* current context */
    int is_full_screen;
    int64_t audio_callback_time;

#define FF_QUIT_EVENT    (SDL_USEREVENT + 2)

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_RendererInfo renderer_info = { 0 };
    SDL_AudioDeviceID audio_dev;

    const struct TextureFormatEntry {
        enum AVPixelFormat format;
        int texture_fmt;
    } sdl_texture_format_map[20] = {
        { AV_PIX_FMT_RGB8,           SDL_PIXELFORMAT_RGB332 },
        { AV_PIX_FMT_RGB444,         SDL_PIXELFORMAT_RGB444 },
        { AV_PIX_FMT_RGB555,         SDL_PIXELFORMAT_RGB555 },
        { AV_PIX_FMT_BGR555,         SDL_PIXELFORMAT_BGR555 },
        { AV_PIX_FMT_RGB565,         SDL_PIXELFORMAT_RGB565 },
        { AV_PIX_FMT_BGR565,         SDL_PIXELFORMAT_BGR565 },
        { AV_PIX_FMT_RGB24,          SDL_PIXELFORMAT_RGB24 },
        { AV_PIX_FMT_BGR24,          SDL_PIXELFORMAT_BGR24 },
        { AV_PIX_FMT_0RGB32,         SDL_PIXELFORMAT_RGB888 },
        { AV_PIX_FMT_0BGR32,         SDL_PIXELFORMAT_BGR888 },
        { AV_PIX_FMT_NE(RGB0, 0BGR), SDL_PIXELFORMAT_RGBX8888 },
        { AV_PIX_FMT_NE(BGR0, 0RGB), SDL_PIXELFORMAT_BGRX8888 },
        { AV_PIX_FMT_RGB32,          SDL_PIXELFORMAT_ARGB8888 },
        { AV_PIX_FMT_RGB32_1,        SDL_PIXELFORMAT_RGBA8888 },
        { AV_PIX_FMT_BGR32,          SDL_PIXELFORMAT_ABGR8888 },
        { AV_PIX_FMT_BGR32_1,        SDL_PIXELFORMAT_BGRA8888 },
        { AV_PIX_FMT_YUV420P,        SDL_PIXELFORMAT_IYUV },
        { AV_PIX_FMT_YUYV422,        SDL_PIXELFORMAT_YUY2 },
        { AV_PIX_FMT_UYVY422,        SDL_PIXELFORMAT_UYVY },
        { AV_PIX_FMT_NONE,           SDL_PIXELFORMAT_UNKNOWN },
    };

#if CONFIG_AVFILTER
    int opt_add_vfilter(void* optctx, const char* opt, const char* arg);
#endif

    int cmp_audio_fmts(enum AVSampleFormat fmt1, int64_t channel_count1,
        enum AVSampleFormat fmt2, int64_t channel_count2);


    int64_t get_valid_channel_layout(int64_t channel_layout, int channels);

    int packet_queue_put_private(PacketQueue* q, AVPacket* pkt);
    int packet_queue_put(PacketQueue* q, AVPacket* pkt);

    int packet_queue_put_nullpacket(PacketQueue* q, AVPacket* pkt, int stream_index);
    /* packet queue handling */
    int packet_queue_init(PacketQueue* q);

    void packet_queue_flush(PacketQueue* q);

    void packet_queue_destroy(PacketQueue* q);

    void packet_queue_abort(PacketQueue* q);

    void packet_queue_start(PacketQueue* q);

    /* return < 0 if aborted, 0 if no packet and > 0 if packet.  */
    int packet_queue_get(PacketQueue* q, AVPacket* pkt, int block, int* serial);
    int decoder_init(Decoder* d, AVCodecContext* avctx, PacketQueue* queue, SDL_cond* empty_queue_cond);
    int decoder_decode_frame(Decoder* d, AVFrame* frame, AVSubtitle* sub);

    void decoder_destroy(Decoder* d);

    void frame_queue_unref_item(Frame* vp);

    int frame_queue_init(FrameQueue* f, PacketQueue* pktq, int max_size, int keep_last);

    void frame_queue_destory(FrameQueue* f);

    void frame_queue_signal(FrameQueue* f);

    Frame* frame_queue_peek(FrameQueue* f);

    Frame* frame_queue_peek_next(FrameQueue* f);

    Frame* frame_queue_peek_last(FrameQueue* f);

    Frame* frame_queue_peek_writable(FrameQueue* f);

    Frame* frame_queue_peek_readable(FrameQueue* f);

    void frame_queue_push(FrameQueue* f);

    void frame_queue_next(FrameQueue* f);

    /* return the number of undisplayed frames in the queue */
    int frame_queue_nb_remaining(FrameQueue* f);
    /* return last shown position */
    int64_t frame_queue_last_pos(FrameQueue* f);
    void decoder_abort(Decoder* d, FrameQueue* fq);

    inline void fill_rectangle(int x, int y, int w, int h);

    int realloc_texture(SDL_Texture** texture, Uint32 new_format, int new_width, int new_height, SDL_BlendMode blendmode, int init_texture);

    void calculate_display_rect(SDL_Rect* rect,
        int scr_xleft, int scr_ytop, int scr_width, int scr_height,
        int pic_width, int pic_height, AVRational pic_sar);

    void get_sdl_pix_fmt_and_blendmode(int format, Uint32* sdl_pix_fmt, SDL_BlendMode* sdl_blendmode);

    int upload_texture(SDL_Texture** tex, AVFrame* frame, struct SwsContext** img_convert_ctx);

    void set_sdl_yuv_conversion_mode(AVFrame* frame);
    void video_image_display(VideoState* is);

    int compute_mod(int a, int b);

    void video_audio_display(VideoState* s);

    void stream_component_close(VideoState* is, int stream_index);

    void stream_close(VideoState* is);
    void do_exit(VideoState*is);

    static void sigterm_handler(int sig);

    //void set_default_window_size(int width, int height, AVRational sar);
    //int video_open(VideoState* is);

    /* display the current picture, if any */
    void video_display(VideoState* is);
    double get_clock(Clock* c);

    void set_clock_at(Clock* c, double pts, int serial, double time);
    void set_clock(Clock* c, double pts, int serial);

    void set_clock_speed(Clock* c, double speed);

    void init_clock(Clock* c, int* queue_serial);

    void sync_clock_to_slave(Clock* c, Clock* slave);

    int get_master_sync_type(VideoState* is);
    /* get the current master clock value */
    double get_master_clock(VideoState* is);

    void check_external_clock_speed(VideoState* is);
    /* seek in the stream */
    void stream_seek(VideoState* is, int64_t pos, int64_t rel, int seek_by_bytes);

    /* pause or resume the video */
    void stream_toggle_pause(VideoState* is);

    void toggle_pause(VideoState* is);

    void toggle_mute(VideoState* is);

    void update_volume(VideoState* is, int sign, double step);

    void step_to_next_frame(VideoState* is);

    double compute_target_delay(double delay, VideoState* is);

    double vp_duration(VideoState* is, Frame* vp, Frame* nextvp);

    void update_video_pts(VideoState* is, double pts, int64_t pos, int serial);

    /* called to display each frame */
    void video_refresh(void* opaque, double* remaining_time);

    int queue_picture(VideoState* is, AVFrame* src_frame, double pts, double duration, int64_t pos, int serial);

    int get_video_frame(VideoState* is, AVFrame* frame);
#if CONFIG_AVFILTER
    int configure_filtergraph(AVFilterGraph* graph, const char* filtergraph,
        AVFilterContext* source_ctx, AVFilterContext* sink_ctx);
    int configure_video_filters(AVFilterGraph* graph, VideoState* is, const char* vfilters, AVFrame* frame);

    int configure_audio_filters(VideoState* is, const char* afilters, int force_output_format);
#endif  /* CONFIG_AVFILTER */

    static int audio_thread(void* arg);

    int decoder_start(Decoder* d, int (*fn)(void*), const char* thread_name, void* arg);

    static int video_thread(void* arg);

    static int subtitle_thread(void* arg);

    /* copy samples for viewing in editor window */
    void update_sample_display(VideoState* is, short* samples, int samples_size);

    /* return the wanted number of samples to get better sync if sync_type is video
     * or external master clock */
    int synchronize_audio(VideoState* is, int nb_samples);
    int synchronize_audio1(VideoState* is, int nb_samples);
    /**
     * Decode one audio frame and return its uncompressed size.
     *
     * The processed audio frame is decoded, converted if required, and
     * stored in is->audio_buf, with size in bytes given by the return
     * value.
     */
    int audio_decode_frame(VideoState* is,int iNeedLen);

    /* prepare a new audio buffer */
    static void sdl_audio_callback(void* opaque, Uint8* stream, int len);

    int audio_open(void* opaque, int64_t wanted_channel_layout, int wanted_nb_channels, int wanted_sample_rate, /*struct*/ AudioParams* audio_hw_params);
    /* open a given stream. Return 0 if OK */
    int stream_component_open(VideoState* is, int stream_index);

    static int decode_interrupt_cb(void* ctx);

    int stream_has_enough_packets(AVStream* st, int stream_id, PacketQueue* queue);

    int is_realtime(AVFormatContext* s);
    /* this thread gets the stream from the disk or the network */
    static int read_thread(void* arg);

    VideoState* stream_open(const char* filename, const AVInputFormat* iformat);

    void stream_cycle_channel(VideoState* is, int codec_type);


    void toggle_full_screen(VideoState* is);

    void toggle_audio_display(VideoState* is);

    void refresh_loop_wait_event(VideoState* is, SDL_Event* event);

    void seek_chapter(VideoState* is, int incr);

    /* handle an event sent by the GUI */
    void event_loop(VideoState* cur_stream);

    int opt_frame_size(void* optctx, const char* opt, const char* arg);

    int opt_width(void* optctx, const char* opt, const char* arg);

    int opt_height(void* optctx, const char* opt, const char* arg);

    int opt_format(void* optctx, const char* opt, const char* arg);

    int opt_frame_pix_fmt(void* optctx, const char* opt, const char* arg);

    int opt_sync(void* optctx, const char* opt, const char* arg);

    int opt_seek(void* optctx, const char* opt, const char* arg);

    int opt_duration(void* optctx, const char* opt, const char* arg);

    int opt_show_mode(void* optctx, const char* opt, const char* arg);

    void opt_input_file(void* optctx, const char* filename);
    int opt_codec(void* optctx, const char* opt, const char* arg);

    int dummy;

    


#if CONFIG_AVDEVICE
#define CMDUTILS_COMMON_OPTIONS_AVDEVICE                                                                                \
    { "sources"    , OPT_EXIT | HAS_ARG, { .func_arg = show_sources },                                                  \
      "list sources of the input device", "device" },                                                                   \
    { "sinks"      , OPT_EXIT | HAS_ARG, { .func_arg = show_sinks },                                                    \
      "list sinks of the output device", "device" },                                                                    \

#else
#define CMDUTILS_COMMON_OPTIONS_AVDEVICE
#endif

#define CMDUTILS_COMMON_OPTIONS                                                                                         \
    { "L",           OPT_EXIT,             &ffplay::show_license ,     "show license" },                          \
    { "h",           OPT_EXIT,             &ffplay::show_help ,        "show help", "topic" },                    \
    { "?",           OPT_EXIT,            &ffplay::show_help ,        "show help", "topic" },                    \
    { "help",        OPT_EXIT,            &ffplay::show_help ,        "show help", "topic" },                    \
    { "-help",       OPT_EXIT,             &ffplay::show_help ,        "show help", "topic" },                    \
    { "version",     OPT_EXIT,            &ffplay::show_version ,     "show version" },                          \
    { "buildconf",   OPT_EXIT,             &ffplay::show_buildconf ,   "show build configuration" },              \
    { "formats",     OPT_EXIT,             &ffplay::show_formats ,     "show available formats" },                \
    { "muxers",      OPT_EXIT,             &ffplay::show_muxers ,      "show available muxers" },                 \
    { "demuxers",    OPT_EXIT,             &ffplay::show_demuxers ,    "show available demuxers" },               \
    { "devices",     OPT_EXIT,             &ffplay::show_devices ,     "show available devices" },                \
    { "codecs",      OPT_EXIT,             &ffplay::show_codecs ,      "show available codecs" },                 \
    { "decoders",    OPT_EXIT,             &ffplay::show_decoders ,    "show available decoders" },               \
    { "encoders",    OPT_EXIT,            &ffplay::show_encoders ,    "show available encoders" },               \
    { "bsfs",        OPT_EXIT,             &ffplay::show_bsfs ,        "show available bit stream filters" },     \
    { "protocols",   OPT_EXIT,             &ffplay::show_protocols ,   "show available protocols" },              \
    { "filters",     OPT_EXIT,             &ffplay::show_filters ,     "show available filters" },                \
    { "pix_fmts",    OPT_EXIT,             &ffplay::show_pix_fmts ,    "show available pixel formats" },          \
    { "layouts",     OPT_EXIT,              &ffplay::show_layouts ,     "show standard channel layouts" },         \
    { "sample_fmts", OPT_EXIT,             &ffplay::show_sample_fmts , "show available audio sample formats" },   \
    { "colors",      OPT_EXIT,            &ffplay::show_colors ,      "show available color names" },            \
    { "loglevel",    HAS_ARG,               &ffplay::opt_loglevel ,     "set logging level", "loglevel" },         \
    { "v",           HAS_ARG,              &ffplay::opt_loglevel ,     "set logging level", "loglevel" },         \
    { "report",      0,                    &ffplay::opt_report ,       "generate a report" },                     \
    { "max_alloc",   HAS_ARG,              &ffplay::opt_max_alloc ,    "set maximum size of a single allocated block", "bytes" }, \
    { "cpuflags",    HAS_ARG | OPT_EXPERT, &ffplay::opt_cpuflags ,     "force specific cpu flags", "flags" },     \
    { "hide_banner", OPT_BOOL | OPT_EXPERT, &hide_banner,     "do not show program banner", "hide_banner" },          \
    CMDUTILS_COMMON_OPTIONS_AVDEVICE  

    const OptionDef options[1024] = {
       CMDUTILS_COMMON_OPTIONS
       { "x", HAS_ARG, &ffplay::opt_width, "force displayed width", "width" },
       { "y", HAS_ARG, &ffplay::opt_height, "force displayed height", "height" },
       { "s", HAS_ARG | OPT_VIDEO, &ffplay::opt_frame_size, "set frame size (WxH or abbreviation)", "size" },
       { "fs", OPT_BOOL, &is_full_screen , "force full screen" },
       { "an", OPT_BOOL,&audio_disable , "disable audio" },
       { "vn", OPT_BOOL,  &video_disable , "disable video" },
       { "sn", OPT_BOOL,  &subtitle_disable , "disable subtitling" },
       { "ast", OPT_STRING | HAS_ARG | OPT_EXPERT, {&wanted_stream_spec[AVMEDIA_TYPE_AUDIO] }, "select desired audio stream", "stream_specifier" },
       { "vst", OPT_STRING | HAS_ARG | OPT_EXPERT, { &wanted_stream_spec[AVMEDIA_TYPE_VIDEO] }, "select desired video stream", "stream_specifier" },
       { "sst", OPT_STRING | HAS_ARG | OPT_EXPERT, { &wanted_stream_spec[AVMEDIA_TYPE_SUBTITLE] }, "select desired subtitle stream", "stream_specifier" },
       { "ss", HAS_ARG, &ffplay::opt_seek , "seek to a given position in seconds", "pos" },
       { "t", HAS_ARG, &ffplay::opt_duration , "play  \"duration\" seconds of audio/video", "duration" },
       { "bytes", OPT_INT | HAS_ARG, { &seek_by_bytes }, "seek by bytes 0=off 1=on -1=auto", "val" },
       { "seek_interval", OPT_FLOAT | HAS_ARG, { &seek_interval }, "set seek interval for left/right keys, in seconds", "seconds" },
       { "nodisp", OPT_BOOL, { &display_disable }, "disable graphical display" },
       { "noborder", OPT_BOOL, { &borderless }, "borderless window" },
       { "alwaysontop", OPT_BOOL, { &alwaysontop }, "window always on top" },
       { "volume", OPT_INT | HAS_ARG, { &startup_volume}, "set startup volume 0=min 100=max", "volume" },
       { "f", HAS_ARG, &ffplay::opt_format , "force format", "fmt" },
       { "pix_fmt", HAS_ARG | OPT_EXPERT | OPT_VIDEO, &ffplay::opt_frame_pix_fmt , "set pixel format", "format" },
       { "stats", OPT_BOOL | OPT_EXPERT, { &show_status }, "show status", "" },
       { "fast", OPT_BOOL | OPT_EXPERT, { &fast }, "non spec compliant optimizations", "" },
       { "genpts", OPT_BOOL | OPT_EXPERT, { &genpts }, "generate pts", "" },
       { "drp", OPT_INT | HAS_ARG | OPT_EXPERT, { &decoder_reorder_pts }, "let decoder reorder pts 0=off 1=on -1=auto", ""},
       { "lowres", OPT_INT | HAS_ARG | OPT_EXPERT, { &lowres }, "", "" },
       { "sync", HAS_ARG | OPT_EXPERT, &ffplay::opt_sync , "set audio-video sync. type (type=audio/video/ext)", "type" },
       { "autoexit", OPT_BOOL | OPT_EXPERT, { &autoexit }, "exit at the end", "" },
       { "exitonkeydown", OPT_BOOL | OPT_EXPERT, { &exit_on_keydown }, "exit on key down", "" },
       { "exitonmousedown", OPT_BOOL | OPT_EXPERT, { &exit_on_mousedown }, "exit on mouse down", "" },
       { "loop", OPT_INT | HAS_ARG | OPT_EXPERT, { &loop }, "set number of times the playback shall be looped", "loop count" },
       { "framedrop", OPT_BOOL | OPT_EXPERT, { &framedrop }, "drop frames when cpu is too slow", "" },
       { "infbuf", OPT_BOOL | OPT_EXPERT, { &infinite_buffer }, "don't limit the input buffer size (useful with realtime streams)", "" },
       { "window_title", OPT_STRING | HAS_ARG, { &window_title }, "set window title", "window title" },
       { "left", OPT_INT | HAS_ARG | OPT_EXPERT, { &screen_left }, "set the x position for the left of the window", "x pos" },
       { "top", OPT_INT | HAS_ARG | OPT_EXPERT, { &screen_top }, "set the y position for the top of the window", "y pos" },
   #if CONFIG_AVFILTER
       { "vf", OPT_EXPERT | HAS_ARG, &ffplay::opt_add_vfilter, "set video filters", "filter_graph" },
       { "af", OPT_STRING | HAS_ARG, { &afilters }, "set audio filters", "filter_graph" },
   #endif
       { "rdftspeed", OPT_INT | HAS_ARG | OPT_AUDIO | OPT_EXPERT, { &rdftspeed }, "rdft speed", "msecs" },
       { "showmode", HAS_ARG, &ffplay::opt_show_mode, "select show mode (0 = video, 1 = waves, 2 = RDFT)", "mode" },
       { "default", HAS_ARG | OPT_AUDIO | OPT_VIDEO | OPT_EXPERT, &ffplay::opt_default , "generic catch all option", "" },
       { "i", OPT_BOOL, { &dummy}, "read specified file", "input_file"},
       { "codec", HAS_ARG,  &ffplay::opt_codec, "force decoder", "decoder_name" },
       { "acodec", HAS_ARG | OPT_STRING | OPT_EXPERT, {    &audio_codec_name }, "force audio decoder",    "decoder_name" },
       { "scodec", HAS_ARG | OPT_STRING | OPT_EXPERT, { &subtitle_codec_name }, "force subtitle decoder", "decoder_name" },
       { "vcodec", HAS_ARG | OPT_STRING | OPT_EXPERT, {    &video_codec_name }, "force video decoder",    "decoder_name" },
       { "autorotate", OPT_BOOL, { &autorotate }, "automatically rotate video", "" },
       { "find_stream_info", OPT_BOOL | OPT_INPUT | OPT_EXPERT, { &find_stream_info },
           "read and decode the streams to fill missing information with heuristics" },
       { "filter_threads", HAS_ARG | OPT_INT | OPT_EXPERT, { &filter_nbthreads }, "number of filter threads per graph" },
       { NULL, },
    };

    void show_usage(void);


    //void show_help_default(const char* opt, const char* arg);

    /////////////////////////////////////////////////
    /////////////////////////////////////////////////





/**
 * Register a program-specific cleanup routine.
 */
    void register_exit(void (*cb)(int ret));

    /**
     * Wraps exit with a program-specific cleanup routine.
     */
    void exit_program(int ret) av_noreturn;

    /**
     * Initialize dynamic library loading
     */
    void init_dynload(void);

    /**
     * Initialize the cmdutils option system, in particular
     * allocate the *_opts contexts.
     */
    void init_opts(void);
    /**
     * Uninitialize the cmdutils option system, in particular
     * free the *_opts contexts and their contents.
     */
    void uninit_opts(void);

    /**
     * Trivial log callback.
     * Only suitable for opt_help and similar since it lacks prefix handling.
     */
    static void log_callback_help(void* ptr, int level, const char* fmt, va_list vl);

    /**
     * Override the cpuflags.
     */
    int opt_cpuflags(void* optctx, const char* opt, const char* arg);

    /**
     * Fallback for options that are not explicitly handled, these will be
     * parsed through AVOptions.
     */
    int opt_default(void* optctx, const char* opt, const char* arg);
    int match_group_separator(const OptionGroupDef* groups, int nb_groups,
        const char* opt);
    void finish_group(OptionParseContext* octx, int group_idx,
        const char* arg);
    void add_opt(OptionParseContext* octx, const OptionDef* opt,
        const char* key, const char* val);
    void init_parse_context(OptionParseContext* octx,
        const OptionGroupDef* groups, int nb_groups);
    void expand_filename_template(AVBPrint* bp, const char* template1,
        struct tm* tm);
    /**
     * Set the libav* libraries log level.
     */
    int opt_loglevel(void* optctx, const char* opt, const char* arg);

    int opt_report(void* optctx, const char* opt, const char* arg);

    int opt_max_alloc(void* optctx, const char* opt, const char* arg);

    int opt_codec_debug(void* optctx, const char* opt, const char* arg);

    /**
     * Limit the execution time.
     */
    int opt_timelimit(void* optctx, const char* opt, const char* arg);

    /**
     * Parse a string and return its corresponding value as a double.
     * Exit from the application if the string cannot be correctly
     * parsed or the corresponding value is invalid.
     *
     * @param context the context of the value to be set (e.g. the
     * corresponding command line option name)
     * @param numstr the string to be parsed
     * @param type the type (OPT_INT64 or OPT_FLOAT) as which the
     * string should be parsed
     * @param min the minimum valid accepted value
     * @param max the maximum valid accepted value
     */
    double parse_number_or_die(const char* context, const char* numstr, int type,
        double min, double max);

    /**
     * Parse a string specifying a time and return its corresponding
     * value as a number of microseconds. Exit from the application if
     * the string cannot be correctly parsed.
     *
     * @param context the context of the value to be set (e.g. the
     * corresponding command line option name)
     * @param timestr the string to be parsed
     * @param is_duration a flag which tells how to interpret timestr, if
     * not zero timestr is interpreted as a duration, otherwise as a
     * date
     *
     * @see av_parse_time()
     */
    int64_t parse_time_or_die(const char* context, const char* timestr,
        int is_duration);

    typedef struct SpecifierOpt {
        char* specifier;    /**< stream/chapter/program/... specifier */
        union {
            uint8_t* str;
            int        i;
            int64_t  i64;
            uint64_t ui64;
            float      f;
            double   dbl;
        } u;
    } SpecifierOpt;

    

    /**
     * Print help for all options matching specified flags.
     *
     * @param options a list of options
     * @param msg title of this group. Only printed if at least one option matches.
     * @param req_flags print only options which have all those flags set.
     * @param rej_flags don't print options which have any of those flags set.
     * @param alt_flags print only options that have at least one of those flags set
     */
    void show_help_options(const OptionDef* options, const char* msg, int req_flags,
        int rej_flags, int alt_flags);

                                                                                  \

    /**
     * Show help for all options with given flags in class and all its
     * children.
     */
    void show_help_children(const AVClass* pclass, int flags);
    const OptionDef* find_option(const OptionDef* po, const char* name);
    /**
     * Per-fftool specific help handler. Implemented in each
     * fftool, called by show_help().
     */
    void show_help_default(const char* opt, const char* arg);

    /**
     * Generic -h handler common to all fftools.
     */
    int show_help(void* optctx, const char* opt, const char* arg);

    /**
     * Parse the command line arguments.
     *
     * @param optctx an opaque options context
     * @param argc   number of command line arguments
     * @param argv   values of command line arguments
     * @param options Array with the definitions required to interpret every
     * option of the form: -option_name [argument]
     * @param parse_arg_function Name of the function called to process every
     * argument without a leading option name flag. NULL if such arguments do
     * not have to be processed.
     */
    void parse_options(void* optctx, int argc, char** argv, const OptionDef* options,
        void (ffplay::*parse_arg_function)(void* optctx, const char*));

    /**
     * Parse one given option.
     *
     * @return on success 1 if arg was consumed, 0 otherwise; negative number on error
     */
    int parse_option(void* optctx, const char* opt, const char* arg,
        const OptionDef* options);

    
    /**
     * Parse an options group and write results into optctx.
     *
     * @param optctx an app-specific options context. NULL for global options group
     */
    int parse_optgroup(void* optctx, OptionGroup* g);

    /**
     * Split the commandline into an intermediate form convenient for further
     * processing.
     *
     * The commandline is assumed to be composed of options which either belong to a
     * group (those with OPT_SPEC, OPT_OFFSET or OPT_PERFILE) or are global
     * (everything else).
     *
     * A group (defined by an OptionGroupDef struct) is a sequence of options
     * terminated by either a group separator option (e.g. -i) or a parameter that
     * is not an option (doesn't start with -). A group without a separator option
     * must always be first in the supplied groups list.
     *
     * All options within the same group are stored in one OptionGroup struct in an
     * OptionGroupList, all groups with the same group definition are stored in one
     * OptionGroupList in OptionParseContext.groups. The order of group lists is the
     * same as the order of group definitions.
     */
    int split_commandline(OptionParseContext* octx, int argc, char* argv[],
        const OptionDef* options,
        const OptionGroupDef* groups, int nb_groups);

    /**
     * Free all allocated memory in an OptionParseContext.
     */
    void uninit_parse_context(OptionParseContext* octx);

    /**
     * Find the '-loglevel' option in the command line args and apply it.
     */
    void parse_loglevel(int argc, char** argv, const OptionDef* options);
    const AVOption* opt_find(void* obj, const char* name, const char* unit,
        int opt_flags, int search_flags);
    /**
     * Return index of option opt in argv or 0 if not found.
     */
    int locate_option(int argc, char** argv, const OptionDef* options,
        const char* optname);
    void dump_argument(const char* a);
    void check_options(const OptionDef* po);
    int init_report(const char* env);
    /**
     * Check if the given stream matches a stream specifier.
     *
     * @param s  Corresponding format context.
     * @param st Stream from s to be checked.
     * @param spec A stream specifier of the [v|a|s|d]:[\<stream index\>] form.
     *
     * @return 1 if the stream matches, 0 if it doesn't, <0 on error
     */
    int check_stream_specifier(AVFormatContext* s, AVStream* st, const char* spec);

    /**
     * Filter out options for given codec.
     *
     * Create a new options dictionary containing only the options from
     * opts which apply to the codec with ID codec_id.
     *
     * @param opts     dictionary to place options in
     * @param codec_id ID of the codec that should be filtered for
     * @param s Corresponding format context.
     * @param st A stream from s for which the options should be filtered.
     * @param codec The particular codec for which the options should be filtered.
     *              If null, the default one is looked up according to the codec id.
     * @return a pointer to the created dictionary
     */
    AVDictionary* filter_codec_opts(AVDictionary* opts, enum AVCodecID codec_id,
        AVFormatContext* s, AVStream* st, const AVCodec* codec);

    /**
     * Setup AVCodecContext options for avformat_find_stream_info().
     *
     * Create an array of dictionaries, one dictionary for each stream
     * contained in s.
     * Each dictionary will contain the options from codec_opts which can
     * be applied to the corresponding stream codec context.
     *
     * @return pointer to the created array of dictionaries, NULL if it
     * cannot be created
     */
    AVDictionary** setup_find_stream_info_opts(AVFormatContext* s,
        AVDictionary* codec_opts);

    /**
     * Print an error message to stderr, indicating filename and a human
     * readable description of the error code err.
     *
     * If strerror_r() is not available the use of this function in a
     * multithreaded application may be unsafe.
     *
     * @see av_strerror()
     */
    void print_error(const char* filename, int err);

    /**
     * Print the program banner to stderr. The banner contents depend on the
     * current version of the repository and of the libav* libraries used by
     * the program.
     */
    void show_banner(int argc, char** argv, const OptionDef* options);

    /**
     * Print the version of the program to stdout. The version message
     * depends on the current versions of the repository and of the libav*
     * libraries.
     * This option processing function does not utilize the arguments.
     */
    int show_version(void* optctx, const char* opt, const char* arg);

    /**
     * Print the build configuration of the program to stdout. The contents
     * depend on the definition of FFMPEG_CONFIGURATION.
     * This option processing function does not utilize the arguments.
     */
    int show_buildconf(void* optctx, const char* opt, const char* arg);

    /**
     * Print the license of the program to stdout. The license depends on
     * the license of the libraries compiled into the program.
     * This option processing function does not utilize the arguments.
     */
    int show_license(void* optctx, const char* opt, const char* arg);

    /**
     * Print a listing containing all the formats supported by the
     * program (including devices).
     * This option processing function does not utilize the arguments.
     */
    int show_formats(void* optctx, const char* opt, const char* arg);
    void print_codec(const AVCodec* c);
    char get_media_type_char(enum AVMediaType type);
    const AVCodec* next_codec_for_id(enum AVCodecID id, void** iter,
        int encoder);
    static int compare_codec_desc(const void* a, const void* b);
    unsigned get_codecs_sorted(const AVCodecDescriptor*** rcodecs);
    void print_codecs_for_id(enum AVCodecID id, int encoder);
    void print_codecs(int encoder);

    /**
     * Print a listing containing all the muxers supported by the
     * program (including devices).
     * This option processing function does not utilize the arguments.
     */
    int show_muxers(void* optctx, const char* opt, const char* arg);

    /**
     * Print a listing containing all the demuxer supported by the
     * program (including devices).
     * This option processing function does not utilize the arguments.
     */
    int show_demuxers(void* optctx, const char* opt, const char* arg);

    /**
     * Print a listing containing all the devices supported by the
     * program.
     * This option processing function does not utilize the arguments.
     */
    int show_devices(void* optctx, const char* opt, const char* arg);

#if CONFIG_AVDEVICE
    /**
     * Print a listing containing autodetected sinks of the output device.
     * Device name with options may be passed as an argument to limit results.
     */
    int show_sinks(void* optctx, const char* opt, const char* arg);

    /**
     * Print a listing containing autodetected sources of the input device.
     * Device name with options may be passed as an argument to limit results.
     */
    int show_sources(void* optctx, const char* opt, const char* arg);
#endif

    /**
     * Print a listing containing all the codecs supported by the
     * program.
     * This option processing function does not utilize the arguments.
     */
    int show_codecs(void* optctx, const char* opt, const char* arg);

    /**
     * Print a listing containing all the decoders supported by the
     * program.
     */
    int show_decoders(void* optctx, const char* opt, const char* arg);

    /**
     * Print a listing containing all the encoders supported by the
     * program.
     */
    int show_encoders(void* optctx, const char* opt, const char* arg);

    /**
     * Print a listing containing all the filters supported by the
     * program.
     * This option processing function does not utilize the arguments.
     */
    int show_filters(void* optctx, const char* opt, const char* arg);

    /**
     * Print a listing containing all the bit stream filters supported by the
     * program.
     * This option processing function does not utilize the arguments.
     */
    int show_bsfs(void* optctx, const char* opt, const char* arg);

    /**
     * Print a listing containing all the protocols supported by the
     * program.
     * This option processing function does not utilize the arguments.
     */
    int show_protocols(void* optctx, const char* opt, const char* arg);

    /**
     * Print a listing containing all the pixel formats supported by the
     * program.
     * This option processing function does not utilize the arguments.
     */
    int show_pix_fmts(void* optctx, const char* opt, const char* arg);

    /**
     * Print a listing containing all the standard channel layouts supported by
     * the program.
     * This option processing function does not utilize the arguments.
     */
    int show_layouts(void* optctx, const char* opt, const char* arg);

    /**
     * Print a listing containing all the sample formats supported by the
     * program.
     */
    int show_sample_fmts(void* optctx, const char* opt, const char* arg);
    void show_help_codec(const char* name, int encoder);
    void show_help_demuxer(const char* name);
    void show_help_protocol(const char* name);
    void show_help_muxer(const char* name);
#if CONFIG_AVFILTER
    void show_help_filter(const char* name);
#endif
    void show_help_bsf(const char* name);
    /**
     * Print a listing containing all the color names and values recognized
     * by the program.
     */
    int show_colors(void* optctx, const char* opt, const char* arg);

    /**
     * Return a positive value if a line read from standard input
     * starts with [yY], otherwise return 0.
     */
    int read_yesno(void);

    /**
     * Get a file corresponding to a preset file.
     *
     * If is_path is non-zero, look for the file in the path preset_name.
     * Otherwise search for a file named arg.ffpreset in the directories
     * $FFMPEG_DATADIR (if set), $HOME/.ffmpeg, and in the datadir defined
     * at configuration time or in a "ffpresets" folder along the executable
     * on win32, in that order. If no such file is found and
     * codec_name is defined, then search for a file named
     * codec_name-preset_name.avpreset in the above-mentioned directories.
     *
     * @param filename buffer where the name of the found filename is written
     * @param filename_size size in bytes of the filename buffer
     * @param preset_name name of the preset to search
     * @param is_path tell if preset_name is a filename path
     * @param codec_name name of the codec for which to look for the
     * preset, may be NULL
     */
    FILE* get_preset_file(char* filename, size_t filename_size,
        const char* preset_name, int is_path, const char* codec_name);

    /**
     * Realloc array to hold new_size elements of elem_size.
     * Calls exit() on failure.
     *
     * @param array array to reallocate
     * @param elem_size size in bytes of each element
     * @param size new element count will be written here
     * @param new_size number of elements to place in reallocated array
     * @return reallocated array
     */
    void* grow_array(void* array, int elem_size, int* size, int new_size);

#define media_type_string av_get_media_type_string

#define GROW_ARRAY(array, nb_elems)\
    array = (decltype(array))grow_array(array, sizeof(*array), &nb_elems, nb_elems + 1)

#define GET_PIX_FMT_NAME(pix_fmt)\
    const char *name = av_get_pix_fmt_name(pix_fmt);

#define GET_CODEC_NAME(id)\
    const char *name = avcodec_descriptor_get(id)->name;

#define GET_SAMPLE_FMT_NAME(sample_fmt)\
    const char *name = av_get_sample_fmt_name(sample_fmt)

#define GET_SAMPLE_RATE_NAME(rate)\
    char name[16];\
    snprintf(name, sizeof(name), "%d", rate);

#define GET_CH_LAYOUT_NAME(ch_layout)\
    char name[16];\
    snprintf(name, sizeof(name), "0x%"PRIx64, ch_layout);

#define GET_CH_LAYOUT_DESC(ch_layout)\
    char name[128];\
    av_get_channel_layout_string(name, sizeof(name), 0, ch_layout);

    double get_rotation(AVStream* st);


    AVDictionary* sws_dict = nullptr;;
    AVDictionary* swr_opts = nullptr;
    AVDictionary* format_opts = nullptr, * codec_opts = nullptr, * resample_opts = nullptr;

     FILE* report_file = nullptr;
     int report_file_level = AV_LOG_DEBUG;
    int hide_banner = 0;
    static void log_callback_report(void* ptr, int level, const char* fmt, va_list vl);

    void  print_all_libs_info(int flags, int level);


    void  print_program_info(int flags, int level);
    void  print_buildconf(int flags, int level);
    int  is_device(const AVClass* avclass);
    int  show_formats_devices(void* optctx, const char* opt, const char* arg, int device_only, int muxdemuxers);
    enum show_muxdemuxers {
        SHOW_DEFAULT,
        SHOW_DEMUXERS,
        SHOW_MUXERS,
    };

#if HAVE_COMMANDLINETOARGVW && defined(_WIN32)
    /* Will be leaked on exit */
    static char** win32_argv_utf8 = NULL;
    static int win32_argc = 0;

    /**
     * Prepare command line arguments for executable.
     * For Windows - perform wide-char to UTF-8 conversion.
     * Input arguments should be main() function arguments.
     * @param argc_ptr Arguments number (including executable)
     * @param argv_ptr Arguments list.
     */
    void prepare_app_arguments(int* argc_ptr, char*** argv_ptr);
#else
    void prepare_app_arguments(int* argc_ptr, char*** argv_ptr);
#endif /* HAVE_COMMANDLINETOARGVW */
    int write_option(void* optctx, const OptionDef* po, const char* opt,const char* arg);
     int warned_cfg = 0;

#define INDENT        1
#define SHOW_VERSION  2
#define SHOW_CONFIG   4
#define SHOW_COPYRIGHT 8

#define PRINT_LIB_INFO(libname, LIBNAME, flags, level)                  \
    if (CONFIG_##LIBNAME) {                                             \
        const char *indent = flags & INDENT? "  " : "";                 \
        if (flags & SHOW_VERSION) {                                     \
            unsigned int version = libname##_version();                 \
            av_log(NULL, level,                                         \
                   "%slib%-11s %2d.%3d.%3d / %2d.%3d.%3d\n",            \
                   indent, #libname,                                    \
                   LIB##LIBNAME##_VERSION_MAJOR,                        \
                   LIB##LIBNAME##_VERSION_MINOR,                        \
                   LIB##LIBNAME##_VERSION_MICRO,                        \
                   AV_VERSION_MAJOR(version), AV_VERSION_MINOR(version),\
                   AV_VERSION_MICRO(version));                          \
        }                                                               \
        if (flags & SHOW_CONFIG) {                                      \
            const char *cfg = libname##_configuration();                \
            if (strcmp(FFMPEG_CONFIGURATION, cfg)) {                    \
                if (!warned_cfg) {                                      \
                    av_log(NULL, level,                                 \
                            "%sWARNING: library configuration mismatch\n", \
                            indent);                                    \
                    warned_cfg = 1;                                     \
                }                                                       \
                av_log(NULL, level, "%s%-11s configuration: %s\n",      \
                        indent, #libname, cfg);                         \
            }                                                           \
        }                                                               \
    }                  

#define PRINT_CODEC_SUPPORTED(codec, field, type, list_name, term, get_name) \
    if (codec->field) {                                                      \
        const type *p = codec->field;                                        \
                                                                             \
        printf("    Supported " list_name ":");                              \
        while (*p != term) {                                                 \
            get_name(*p);                                                    \
            printf(" %s", name);                                             \
            p++;                                                             \
        }                                                                    \
        printf("\n");                                                        \
    }                                                                      
    void (*program_exit)(int ret) = nullptr;
};