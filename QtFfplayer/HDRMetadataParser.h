#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <libavutil/mastering_display_metadata.h>

#ifdef __cplusplus
}
#endif

#include<algorithm>

class HDRMetadataParser {
public:
    struct HDRInfo {
        // 主显示元数据
        struct {
            float display_peak_luminance;      // 最大亮度（尼特）
            float min_luminance;                // 最小亮度（尼特）

            // 显示原色（CIE 1931 xy坐标）
            float red_primary[2];    // 红色原色 (x, y)
            float green_primary[2];  // 绿色原色 (x, y)
            float blue_primary[2];   // 蓝色原色 (x, y)
            float white_point[2];    // 白点 (x, y)

            bool has_primaries;      // 是否有原色信息
            bool has_luminance;      // 是否有亮度信息
        } mastering;

        // 内容亮度级别
        struct {
            float max_content_light_level;      // MaxCLL (尼特)
            float max_frame_average_light_level; // MaxFALL (尼特)
            bool has_data;                      // 是否有数据
        } content;

        // 推断信息
        bool is_hdr = false;
        bool is_pq = false;
        bool is_hlg = false;
        bool has_hdr10_plus = false;

        HDRInfo() {
            reset();
        }

        void reset() {
            mastering.display_peak_luminance = 100.0f;  // SDR默认值
            mastering.min_luminance = 0.001f;

            // 默认BT.709原色
            mastering.red_primary[0] = 0.640f; mastering.red_primary[1] = 0.330f;
            mastering.green_primary[0] = 0.300f; mastering.green_primary[1] = 0.600f;
            mastering.blue_primary[0] = 0.150f; mastering.blue_primary[1] = 0.060f;
            mastering.white_point[0] = 0.3127f; mastering.white_point[1] = 0.3290f; // D65

            mastering.has_primaries = false;
            mastering.has_luminance = false;

            content.max_content_light_level = 0.0f;
            content.max_frame_average_light_level = 0.0f;
            content.has_data = false;

            is_hdr = false;
            is_pq = false;
            is_hlg = false;
            has_hdr10_plus = false;
        }
    };

    static HDRInfo parse_frame(AVFrame* frame) {
        HDRInfo info;

        if (!frame) return info;

        // 1. 根据传输特性设置基本HDR类型
        detect_hdr_type_from_trc(frame->color_trc, info);

        // 2. 解析side_data中的HDR元数据
        parse_side_data(frame, info);

        // 3. 如果没找到元数据，尝试根据其他信息推断
        if (!info.mastering.has_luminance && !info.content.has_data) {
            infer_hdr_info_from_other_properties(frame, info);
        }

        // 4. 验证数据的合理性
        validate_and_adjust_hdr_info(info);

        return info;
    }

private:
    static void detect_hdr_type_from_trc(enum AVColorTransferCharacteristic trc, HDRInfo& info) {
        switch (trc) {
        case AVCOL_TRC_SMPTE2084:  // Perceptual Quantizer (PQ)
            info.is_hdr = true;
            info.is_pq = true;
            info.mastering.display_peak_luminance = 10000.0f;  // PQ默认最大值
            break;

        case AVCOL_TRC_ARIB_STD_B67:  // Hybrid Log-Gamma (HLG)
            info.is_hdr = true;
            info.is_hlg = true;
            info.mastering.display_peak_luminance = 1000.0f;   // HLG默认最大值
            break;

        case AVCOL_TRC_BT2020_10:
        case AVCOL_TRC_BT2020_12:
            // BT.2020传输特性，可能是HDR也可能是SDR
            info.is_hdr = true;  // 暂时标记为HDR
            info.mastering.display_peak_luminance = 1000.0f;
            break;

        default:
            // SDR传输特性
            info.is_hdr = false;
            info.mastering.display_peak_luminance = 100.0f;
            break;
        }
    }

    static void parse_side_data(AVFrame* frame, HDRInfo& info) {
        if (!frame->side_data || frame->nb_side_data <= 0) {
            return;
        }

        for (int i = 0; i < frame->nb_side_data; i++) {
            AVFrameSideData* side_data = frame->side_data[i];

            switch (side_data->type) {
            case AV_FRAME_DATA_MASTERING_DISPLAY_METADATA:
                parse_mastering_display_metadata(side_data, info);
                break;

            case AV_FRAME_DATA_CONTENT_LIGHT_LEVEL:
                parse_content_light_level(side_data, info);
                break;

            case AV_FRAME_DATA_DYNAMIC_HDR_PLUS:
                info.has_hdr10_plus = true;
                // 注意：HDR10+需要专门的解析，这里简化处理
                break;

            case AV_FRAME_DATA_DYNAMIC_HDR_VIVID:
                // HDR Vivid动态元数据
                info.is_hdr = true;
                info.is_pq = true;
                break;
            }
        }
    }

    static void parse_mastering_display_metadata(AVFrameSideData* side_data, HDRInfo& info) {
        if (!side_data || side_data->size < sizeof(AVMasteringDisplayMetadata)) {
            return;
        }

        AVMasteringDisplayMetadata* metadata = (AVMasteringDisplayMetadata*)side_data->data;

        // 解析亮度信息
        if (metadata->has_luminance) {
            info.mastering.has_luminance = true;

            // max_luminance 是 AVRational，需要正确解析
            if (metadata->max_luminance.den != 0) {
                // 注意：这里是直接转换为尼特，不是尼特*10000
                info.mastering.display_peak_luminance =
                    (float)metadata->max_luminance.num / (float)metadata->max_luminance.den;
            }

            // min_luminance 也是 AVRational
            if (metadata->min_luminance.den != 0) {
                info.mastering.min_luminance =
                    (float)metadata->min_luminance.num / (float)metadata->min_luminance.den;
            }

            // 确保最小值合理
            if (info.mastering.min_luminance <= 0.0f) {
                info.mastering.min_luminance = 0.001f;  // 合理的最小值
            }
        }

        // 解析原色信息
        if (metadata->has_primaries) {
            info.mastering.has_primaries = true;

            // 解析红色原色 (r)
            if (metadata->display_primaries[0][0].den != 0 &&
                metadata->display_primaries[0][1].den != 0) {
                info.mastering.red_primary[0] =
                    (float)metadata->display_primaries[0][0].num /
                    (float)metadata->display_primaries[0][0].den;
                info.mastering.red_primary[1] =
                    (float)metadata->display_primaries[0][1].num /
                    (float)metadata->display_primaries[0][1].den;
            }

            // 解析绿色原色 (g)
            if (metadata->display_primaries[1][0].den != 0 &&
                metadata->display_primaries[1][1].den != 0) {
                info.mastering.green_primary[0] =
                    (float)metadata->display_primaries[1][0].num /
                    (float)metadata->display_primaries[1][0].den;
                info.mastering.green_primary[1] =
                    (float)metadata->display_primaries[1][1].num /
                    (float)metadata->display_primaries[1][1].den;
            }

            // 解析蓝色原色 (b)
            if (metadata->display_primaries[2][0].den != 0 &&
                metadata->display_primaries[2][1].den != 0) {
                info.mastering.blue_primary[0] =
                    (float)metadata->display_primaries[2][0].num /
                    (float)metadata->display_primaries[2][0].den;
                info.mastering.blue_primary[1] =
                    (float)metadata->display_primaries[2][1].num /
                    (float)metadata->display_primaries[2][1].den;
            }

            // 解析白点
            if (metadata->white_point[0].den != 0 &&
                metadata->white_point[1].den != 0) {
                info.mastering.white_point[0] =
                    (float)metadata->white_point[0].num /
                    (float)metadata->white_point[0].den;
                info.mastering.white_point[1] =
                    (float)metadata->white_point[1].num /
                    (float)metadata->white_point[1].den;
            }
        }
    }

    static void parse_content_light_level(AVFrameSideData* side_data, HDRInfo& info) {
        if (!side_data || side_data->size < sizeof(AVContentLightMetadata)) {
            return;
        }

        AVContentLightMetadata* metadata = (AVContentLightMetadata*)side_data->data;

        info.content.has_data = true;
        info.content.max_content_light_level = metadata->MaxCLL;
        info.content.max_frame_average_light_level = metadata->MaxFALL;

        // 如果最大亮度未知，使用内容亮度作为参考
        if (!info.mastering.has_luminance && info.content.max_content_light_level > 0) {
            info.mastering.display_peak_luminance =
                std::max(info.mastering.display_peak_luminance,
                    info.content.max_content_light_level);
        }
    }

    static void infer_hdr_info_from_other_properties(AVFrame* frame, HDRInfo& info) {
        // 如果颜色空间是BT.2020，很可能是HDR
        if (frame->colorspace == AVCOL_SPC_BT2020_NCL ||
            frame->colorspace == AVCOL_SPC_BT2020_CL) {
            info.is_hdr = true;

            // 根据分辨率猜测亮度范围
            if (frame->width >= 3840 || frame->height >= 2160) {  // 4K
                info.mastering.display_peak_luminance = 4000.0f;  // 4K HDR通常较亮
            }
            else {
                info.mastering.display_peak_luminance = 1000.0f;  // 1080p HDR
            }
        }

        // 根据位深猜测
        switch (frame->format) {
        case AV_PIX_FMT_YUV420P10LE:
        case AV_PIX_FMT_YUV422P10LE:
        case AV_PIX_FMT_YUV444P10LE:
        case AV_PIX_FMT_P010LE:
            // 10位可能是HDR
            if (!info.is_hdr) {
                info.is_hdr = true;
                info.mastering.display_peak_luminance = 1000.0f;
            }
            break;
        }
    }

    static void validate_and_adjust_hdr_info(HDRInfo& info) {
        // 确保亮度值合理
        if (info.mastering.display_peak_luminance <= 0.0f) {
            if (info.is_hdr) {
                info.mastering.display_peak_luminance = 1000.0f;  // HDR默认值
            }
            else {
                info.mastering.display_peak_luminance = 100.0f;   // SDR默认值
            }
        }

        // 确保最小亮度合理
        if (info.mastering.min_luminance <= 0.0f ||
            info.mastering.min_luminance >= info.mastering.display_peak_luminance) {
            info.mastering.min_luminance = info.mastering.display_peak_luminance * 0.0001f;
        }

        // 如果内容亮度数据存在但主亮度未知，使用内容亮度
        if (info.content.has_data && !info.mastering.has_luminance) {
            if (info.content.max_content_light_level > 0) {
                // 通常主亮度略高于最大内容亮度
                info.mastering.display_peak_luminance =
                    std::max(info.mastering.display_peak_luminance,
                        info.content.max_content_light_level * 1.1f);
            }
        }

        // 限制最大值
        const float MAX_REASONABLE_NITS = 10000.0f;
        if (info.mastering.display_peak_luminance > MAX_REASONABLE_NITS) {
            info.mastering.display_peak_luminance = MAX_REASONABLE_NITS;
        }
    }
};