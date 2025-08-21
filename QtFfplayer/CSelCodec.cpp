#include "CSelCodec.h"
#ifdef __cplusplus
extern "C" 
{
#endif
#include "libavcodec/avcodec.h"
#include "libavcodec/codec.h"
#include "libavdevice/avdevice.h"
#ifdef __cplusplus
}
#endif
#include<set>
#include<assert.h>
#include<algorithm>
#include<numeric>
#include<iterator>


std::vector<AVHWDeviceType> CSelCodec::getHWDeviceType() 
{
	if (!m_isInitHWDeviceType)
	{
		m_isInitHWDeviceType = true;
		AVHWDeviceType type = AVHWDeviceType::AV_HWDEVICE_TYPE_NONE;
		while ((type = av_hwdevice_iterate_types(type)) != AVHWDeviceType::AV_HWDEVICE_TYPE_NONE)
			m_vecHWDeviceType.push_back(type);
	}
	return m_vecHWDeviceType;
}

bool  CSelCodec::check_hw_device_type(AVHWDeviceType type) {
	const auto vecDeviceSets = getHWDeviceType();
	return std::find(vecDeviceSets.cbegin(), vecDeviceSets.cend(), type) != vecDeviceSets.cend();
}

//void CSelCodec::initCodec() 
//{
//	s_codecOfEncode.clear();
//	s_codecOfDecode.clear();
//	void* opaque = nullptr;
//	const AVCodec* tmp;
//	while (tmp = av_codec_iterate(&opaque))
//	{
//		if (av_codec_is_encoder(tmp))
//			s_codecOfEncode.push_back(tmp);
//		else if (av_codec_is_decoder(tmp))
//			s_codecOfDecode.push_back(tmp);
//	}
//}


std::vector<const AVCodecHWConfig*>CSelCodec::getAVCodecHWConfig(const AVCodec* codec, int flag)
{
	std::vector<const AVCodecHWConfig*> sets;
	const AVCodecHWConfig* codecHWConfig, * poor = nullptr;
	for (int i = 0;; ++i)
	{
		if (!(codecHWConfig = avcodec_get_hw_config(codec, i)))
			break;
		if ((codecHWConfig->methods & flag) == flag && check_hw_device_type(codecHWConfig->device_type))
			sets.push_back(codecHWConfig);
	}
	return sets;
}

std::vector<const AVCodecHWConfig*>CSelCodec::getDecodeHWConfig(const AVCodec* pDecodeCodec) 
{
	return getAVCodecHWConfig(pDecodeCodec, AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX);

	/*std::vector<const AVCodecHWConfig*> sets;
	const AVCodecHWConfig* tmp, * poor = nullptr;
	for (int i = 0;; ++i)
	{
		if (!(tmp = avcodec_get_hw_config(pDecodeCodec, i)))
			break;
		if (tmp->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX)
			sets.push_back(tmp);
	}
	return sets;*/
}

std::vector<const AVCodecHWConfig*>CSelCodec::getEncodeHWConfig(const AVCodec* pEncodeCodec) 
{
	return getAVCodecHWConfig(pEncodeCodec, AV_CODEC_HW_CONFIG_METHOD_HW_FRAMES_CTX);
}


//std::pair<const AVCodecHWConfig*, std::pair<const AVCodec*, const AVCodecHWConfig*>>CSelCodec::getDecodeHWConfigAndHWEncodeCodec(const AVCodec* pDecodeCodec, AVCodecID encodeStream)
//{
//	const AVCodecHWConfig* tmp,  *poor = nullptr;
//	for (int i = 0;; ++i)
//	{
//		if (!(tmp = avcodec_get_hw_config(pDecodeCodec, i)))
//			break;
//		if (tmp->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX)
//		{
//			if (encodeStream != AV_CODEC_ID_NONE)
//			{
//				std::vector<std::pair<const AVCodec*, const AVCodecHWConfig*>>per = getHWEncodeCodecByStreamIdAndAcceptPixFmt(encodeStream, tmp->pix_fmt);
//				if (per.first)
//					return std::make_pair(tmp, std::move(per));
//				else if (!poor)
//					poor = tmp;
//			}
//			else
//				return std::make_pair(tmp, std::make_pair(nullptr, nullptr));
//		}
//	}
//	return std::make_pair(poor, std::make_pair(nullptr, nullptr));
//}



//const AVCodecHWConfig* CSelCodec::getHWConfigOfDecoder(const AVCodec* pDecodeCodec)
//{
//	const AVCodecHWConfig* tmp;
//	for (int i = 0;; ++i)
//	{
//		tmp = avcodec_get_hw_config(pDecodeCodec, i);
//		if (tmp->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX)
//		{
//			std::find_if(s_codecOfDecode.cbegin(), s_codecOfDecode.cend(), [pDecodeCodec, tmp](const AVCodec*de_codec)
//				{
//					if (de_codec->id == pDecodeCodec->id)
//						return true;
//					else
//						return false;
//				});
//		}
//	}
//}
//
//std::pair<const AVCodec*, const AVCodecHWConfig*> CSelCodec::getHWEncodeCodecByStreamId(AVCodecID id)
//{
//	for (auto ele : s_codecOfEncode)
//	{
//		if (ele->id != id)
//			continue;
//		for (int index = 0;; ++index)
//		{
//			const AVCodecHWConfig* p = avcodec_get_hw_config(ele, index);
//			if (p && (p->methods & AV_CODEC_HW_CONFIG_METHOD_HW_FRAMES_CTX))
//				return std::make_pair(ele, p);
//		}
//	}
//	return std::make_pair(nullptr, nullptr);
//}


std::vector<std::pair<const AVCodec*, const AVCodecHWConfig*>> CSelCodec::getHWEncodeCodecByStreamIdAndAcceptPixFmt(AVCodecID id, bool bSoftEncoder, AVPixelFormat hwPixFmt, AVHWDeviceType deviceType)
{
	void* opaque = nullptr;
	const AVCodec* ele;
	std::vector<std::pair<const AVCodec*, const AVCodecHWConfig*>>sets;
	std::vector<std::pair<const AVCodec*, const AVCodecHWConfig*>>hardWareEncoderSets;
	std::vector<std::pair<const AVCodec*, const AVCodecHWConfig*>>softEncoderSets;

	if (deviceType != AV_HWDEVICE_TYPE_NONE && !check_hw_device_type(deviceType))
		deviceType = AV_HWDEVICE_TYPE_NONE;

	while (ele = av_codec_iterate(&opaque))
	{
		if (!av_codec_is_encoder(ele) || ele->id != id)
			continue;
		for (int index = 0;; ++index)
		{
			const AVCodecHWConfig* pHWConfig = avcodec_get_hw_config(ele, index);
			if (!pHWConfig)
			{
				if (index == 0 && bSoftEncoder)
					softEncoderSets.emplace_back(ele, nullptr);
				break;
			}
			if ((deviceType == AV_HWDEVICE_TYPE_NONE || deviceType == pHWConfig->device_type)
				&& (pHWConfig->methods & AV_CODEC_HW_CONFIG_METHOD_HW_FRAMES_CTX)
				&& check_hw_device_type(pHWConfig->device_type))
			{
				if (hwPixFmt == AV_PIX_FMT_NONE)
					hardWareEncoderSets.push_back({ ele, pHWConfig });
					//return std::make_pair(ele, pHWConfig);
				else if (pHWConfig->pix_fmt == AV_PIX_FMT_NONE || pHWConfig->pix_fmt == hwPixFmt)
					hardWareEncoderSets.push_back({ ele, pHWConfig });
					//return std::make_pair(ele, pHWConfig);
			}
		}
	}
	sets.swap(hardWareEncoderSets);
	if (!softEncoderSets.empty()) 
	{
		sets.reserve(softEncoderSets.size());
		std::move(std::make_move_iterator(softEncoderSets.begin()), std::make_move_iterator(softEncoderSets.end()), std::back_insert_iterator<decltype(sets)>(sets));
	}
	return sets;
}

std::vector<AVPixelFormat>CSelCodec::getHWPixelFormat() 
{
	if (!m_hwPixelFormat.empty())
		return m_hwPixelFormat;
	std::set<AVPixelFormat>sets;
	void* opaque = nullptr;
	const AVCodec* pCodec;
	while (pCodec = av_codec_iterate(&opaque)) 
	{
		for (int index = 0;; ++index)
		{
			const AVCodecHWConfig*pAVCodecHWConfig = avcodec_get_hw_config(pCodec, index);
			if (!pAVCodecHWConfig)
				break;
			if (pAVCodecHWConfig->methods & AV_CODEC_HW_CONFIG_METHOD_HW_FRAMES_CTX)
			{
				assert(pAVCodecHWConfig->pix_fmt != AV_PIX_FMT_NONE);
				sets.insert(pAVCodecHWConfig->pix_fmt);
			}
		}
	}
	//m_hwPixelFormat.reserve(sets.size());
	m_hwPixelFormat.assign(sets.begin(), sets.end());
	return m_hwPixelFormat;
}

bool CSelCodec::isSoftEncoder(const AVCodec* codec)
{
	bool b = av_codec_is_encoder(codec);
	if (!b)
		return false;
	return avcodec_get_hw_config(codec, 0) == NULL;
}

std::vector<AVPixelFormat>CSelCodec::m_hwPixelFormat;

std::vector<AVHWDeviceType>CSelCodec::m_vecHWDeviceType;

bool CSelCodec::m_isInitHWDeviceType = false;