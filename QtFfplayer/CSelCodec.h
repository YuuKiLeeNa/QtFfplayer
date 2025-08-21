#pragma once
#include<vector>
#ifdef __cplusplus
extern "C"
{
#endif
#include "libavcodec/codec_id.h"
#include "libavutil/pixfmt.h"
#include "libavutil/hwcontext.h"
#ifdef __cplusplus
}
#endif
struct AVCodec;
struct AVCodecHWConfig;

class CSelCodec 
{
public:
	static std::vector<AVHWDeviceType> getHWDeviceType();
	static bool  check_hw_device_type(AVHWDeviceType type);


	/*****
	* flag sets to
	Bit set of AV_CODEC_HW_CONFIG_METHOD_* flags, describing the possible
     * setup methods which can be used with this configuration.
	***/
	static std::vector<const AVCodecHWConfig*>getAVCodecHWConfig(const AVCodec* codec, int flag);

	static std::vector<const AVCodecHWConfig*>getDecodeHWConfig(const AVCodec* pDecodeCodec);
	static std::vector<const AVCodecHWConfig*>getEncodeHWConfig(const AVCodec* pEncodeCodec);
	//static std::pair<const AVCodecHWConfig*, std::pair<const AVCodec*, const AVCodecHWConfig*>>getDecodeHWConfigAndHWEncodeCodec(const AVCodec* pDecodeCodec, AVCodecID encodeStream = AV_CODEC_ID_NONE);
	//static std::pair<const AVCodec*, const AVCodecHWConfig*> getHWEncodeCodecByStreamId(AVCodecID id);
	static std::vector<std::pair<const AVCodec*, const AVCodecHWConfig*>> getHWEncodeCodecByStreamIdAndAcceptPixFmt(AVCodecID id, bool bSoftEncoder = true,AVPixelFormat hwPixFmt = AV_PIX_FMT_NONE, AVHWDeviceType deviceType = AVHWDeviceType::AV_HWDEVICE_TYPE_NONE);
	static std::vector<AVPixelFormat>getHWPixelFormat();
	static bool isSoftEncoder(const AVCodec* codec);
protected:
	static std::vector<AVPixelFormat>m_hwPixelFormat;
	static std::vector<AVHWDeviceType>m_vecHWDeviceType;
	static bool m_isInitHWDeviceType;
};