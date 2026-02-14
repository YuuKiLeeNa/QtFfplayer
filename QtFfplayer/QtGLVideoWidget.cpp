#include "QtGLVideoWidget.h"
#include <QOpenGLShader>
#include<memory>
#include<algorithm>
#include<numeric>
#ifdef __cplusplus
extern "C" {
#endif
#include"libavdevice/avdevice.h"
#include "libavutil/frame.h"
#include "libavutil/mastering_display_metadata.h"
#include "libswscale/swscale.h"
#ifdef __cplusplus
}
#endif
#include <glm/ext.hpp>

// 像素格式枚举（与FFmpeg对应）
#define PIX_FMT_NONE       -1
#define PIX_FMT_YUV420P    0   // YUV420平面
#define PIX_FMT_YUV422P    1   // YUV422平面
#define PIX_FMT_YUV444P    2   // YUV444平面
#define PIX_FMT_NV12       3   // YUV420半平面 (UV交错)
#define PIX_FMT_NV21       4   // YUV420半平面 (VU交错)
#define PIX_FMT_YUYV422    5   // YUYV打包
#define PIX_FMT_UYVY422    6   // UYVY打包
#define PIX_FMT_YUV420P10LE 7  // 10位YUV420
#define PIX_FMT_YUVA420P   8   // YUVA420平面
#define PIX_FMT_RGB24      9   // RGB24
#define PIX_FMT_RGBA       10  // RGBA
#define PIX_FMT_BGR24      11  // BGR24
#define PIX_FMT_BGRA       12  // BGRA
#define PIX_FMT_GRAY8      13  // 灰度
#define PIX_FMT_P010LE      14 // P010 (10位半平面)
#define PIX_FMT_P010       PIX_FMT_P010LE
#define PIX_FMT_YUV422P10LE 15 // 10位YUV422
#define PIX_FMT_YUV444P10LE 16 // 10位YUV444

// EOTF类型枚举
#define EOTF_SDR_GAMMA     0
#define EOTF_HLG           1
#define EOTF_PQ            2

// 颜色空间枚举
#define COLOR_SPACE_BT601  0
#define COLOR_SPACE_BT709  1
#define COLOR_SPACE_BT2020 2
#define COLOR_SPACE_RGB    3

#define FRAG_SHADER_YUV_UNIVERSAL_OPTIMIZED R"(
#version 130

// 像素格式枚举（与C++端匹配）
#define PIX_FMT_YUV420P    0   // YUV420平面
#define PIX_FMT_YUV422P    1   // YUV422平面
#define PIX_FMT_YUV444P    2   // YUV444平面
#define PIX_FMT_NV12       3   // YUV420半平面 (UV交错)
#define PIX_FMT_NV21       4   // YUV420半平面 (VU交错)
#define PIX_FMT_YUYV422    5   // YUYV打包
#define PIX_FMT_UYVY422    6   // UYVY打包
#define PIX_FMT_YUV420P10LE 7  // 10位YUV420
#define PIX_FMT_YUVA420P   8   // YUVA420平面
#define PIX_FMT_RGB24      9   // RGB24
#define PIX_FMT_RGBA       10  // RGBA
#define PIX_FMT_BGR24      11  // BGR24
#define PIX_FMT_BGRA       12  // BGRA
#define PIX_FMT_GRAY8      13  // 灰度
#define PIX_FMT_P010LE     14
#define PIX_FMT_P010       PIX_FMT_P010LE
#define PIX_FMT_YUV422P10LE 15 // 10位YUV422
#define PIX_FMT_YUV444P10LE 16 // 10位YUV444


// EOTF类型枚举
#define EOTF_SDR_GAMMA     0
#define EOTF_HLG           1
#define EOTF_PQ            2

// ============================================
// Uniform变量
// ============================================

// 纹理采样器
uniform sampler2D tex_y;        // Y平面或主纹理
uniform sampler2D tex_u;        // U平面或UV平面
uniform sampler2D tex_v;        // V平面或VU平面（可选）
uniform sampler2D tex_a;        // Alpha平面（可选）

// 格式控制
uniform int pixel_format;       // 像素格式枚举
uniform int color_range;        // 范围类型 (0:有限范围, 1:全范围)
uniform int eotf_type;          // EOTF类型
uniform int swap_uv;           // 是否交换UV

// HDR参数
uniform float display_peak_nits;  // 显示器峰值亮度
uniform float system_gamma;       // 系统Gamma值（HLG用）
uniform vec2  hdr_metadata;       // HDR元数据（最大内容亮度等）

// 转换矩阵（预计算，避免着色器分支）
uniform mat3  yuv_to_rgb_matrix;  // YUV->RGB转换矩阵
uniform vec3  yuv_offset;         // YUV偏移量
uniform float y_scale;            // Y 缩放因子（有限范围需要）
uniform float uv_scale;           // UV 缩放因子（有限范围需要）


// 色度位置
uniform vec2  chroma_offset;      // 色度偏移（注意：chroma_scale 已被移除）

// 10位数据参数
//uniform vec3  bit_depth_params;   // x:10位缩放, y:10位偏移, z:位深

uniform float ten_bit_scale;

// ============================================
// 输入/输出变量
// ============================================
varying vec2 v_coord;  // 从顶点着色器传入的纹理坐标

// ============================================
// 常量定义（使用宏定义，兼容GLSL 120/130）
// ============================================

// EOTF常数
#define PQ_M1 0.1593017578125
#define PQ_M2 78.84375
#define PQ_C1 0.8359375
#define PQ_C2 18.8515625
#define PQ_C3 18.6875

#define HLG_A 0.17883277
#define HLG_B 0.28466892
#define HLG_C 0.55991073

#define GAMMA_2_2 2.2
#define GAMMA_2_4 2.4

// YUV到RGB转换
vec3 yuv_to_rgb(vec3 yuv) {
    return yuv_to_rgb_matrix * yuv;
}

// ============================================
// EOTF转换函数
// ============================================

// HLG逆OETF
float hlg_inverse_eotf_fast(float x) {
    bool is_low = x <= 0.5;
    float low_result = (x * x) * 0.33333333;
    float high_result = (exp((x - HLG_C) * (1.0/HLG_A)) + HLG_B) * 0.08333333;
    return is_low ? low_result : high_result;
}

// PQ逆EOTF
float pq_inverse_eotf_fast(float x) {
    float x_pow = pow(x, 1.0 / PQ_M2);
    float num = max(x_pow - PQ_C1, 0.0);
    float den = PQ_C2 - PQ_C3 * x_pow;
    return pow(num / den, 1.0 / PQ_M1);
}

// SDR Gamma逆变换
float gamma_inverse_eotf_fast(float x, int range_type) {
    if (range_type == 0) {  // 有限范围
        x = (x - 0.062745) * 1.164383;
    }
    return pow(x, GAMMA_2_4);
}

// 应用EOTF到RGB向量
vec3 apply_eotf_fast(vec3 rgb, int eotf, int range) {
    vec3 result = rgb;
    
    if (eotf == EOTF_HLG) {
        float sys_gamma = max(system_gamma, 0.001);
        result.r = pow(hlg_inverse_eotf_fast(result.r), sys_gamma);
        result.g = pow(hlg_inverse_eotf_fast(result.g), sys_gamma);
        result.b = pow(hlg_inverse_eotf_fast(result.b), sys_gamma);
        result *= 1000.0;
    } 
    else if (eotf == EOTF_PQ) {
        result.r = pq_inverse_eotf_fast(result.r);
        result.g = pq_inverse_eotf_fast(result.g);
        result.b = pq_inverse_eotf_fast(result.b);
        result *= 10000.0;
    } 
    else {
        result.r = gamma_inverse_eotf_fast(result.r, range);
        result.g = gamma_inverse_eotf_fast(result.g, range);
        result.b = gamma_inverse_eotf_fast(result.b, range);
        result *= 100.0;
    }
    
    return result;
}

// ============================================
// 色调映射函数
// ============================================

// Reinhard色调映射
vec3 tonemap_reinhard_fast(vec3 color, float max_white) {
    float max_white_safe = max(max_white, 0.001);
    return color / (color + max_white_safe);
}

// ACES近似色调映射
vec3 tonemap_aces_fast(vec3 color) {
    const vec3 a = vec3(2.51);
    const vec3 b = vec3(0.03);
    const vec3 c = vec3(2.43);
    const vec3 d = vec3(0.59);
    const vec3 e = vec3(0.14);
    
    vec3 numerator = color * (a * color + b);
    vec3 denominator = color * (c * color + d) + e;
    return clamp(numerator / denominator, 0.0, 1.0);
}

// 自适应色调映射
vec3 tonemap_adaptive_fast(vec3 linear_hdr, float display_peak, vec2 metadata) {
    float max_content = metadata.x > 0.0 ? metadata.x : 1000.0;
    float ratio = display_peak / max_content;
    
    vec3 result;
    if (ratio >= 1.0) {
        result = linear_hdr / max_content;
    } else if (ratio > 0.5) {
        result = tonemap_reinhard_fast(linear_hdr, max_content) * ratio;
    } else {
        result = tonemap_aces_fast(linear_hdr / max_content);
    }
    return clamp(result, 0.0, 1.0);
}

// ============================================
// 采样函数（使用 texture2D）
// ============================================

// 获取平面/半平面 YUV 数据（修正 UV 缩放逻辑）
vec3 get_yuv_from_planar_optimized(vec2 coord) {
    vec3 yuv;
    yuv.x = texture2D(tex_y, coord).r;
    
    // ---------- 半平面格式（NV12 / P010）----------
    if (pixel_format == PIX_FMT_NV12 || 
        pixel_format == PIX_FMT_P010 ||      
        pixel_format == PIX_FMT_NV21) {
        
        vec2 uv = texture2D(tex_u, coord).rg;
        yuv.y = uv.r;
        yuv.z = uv.g;
        if (swap_uv == 1) {
            yuv.yz = yuv.zy;   // 交换 U、V
        }
    } 
    else {
        yuv.y = texture2D(tex_u, coord).r;
        yuv.z = texture2D(tex_v, coord).r;
    }
    return yuv;
}

// 获取打包YUV数据
vec3 get_yuv_from_packed_optimized(vec2 coord) {
    vec4 packed = texture2D(tex_y, coord);
    vec3 yuv = vec3(0.0);
    
    if (pixel_format == PIX_FMT_YUYV422) {
        float pixel_pos = fract(coord.x * 2.0);
        yuv.x = mix(packed.r, packed.b, step(0.5, pixel_pos)); // Y0=byte0, Y1=byte2
        yuv.y = packed.g; // U
        yuv.z = packed.a; // V
    }
    else if (pixel_format == PIX_FMT_UYVY422) {
        float pixel_pos = fract(coord.x * 2.0);
        yuv.x = mix(packed.g, packed.a, step(0.5, pixel_pos)); // Y0=byte1, Y1=byte3
        yuv.y = packed.r; // U
        yuv.z = packed.b; // V
    }
    return yuv;
}

// 获取Alpha分量
float get_alpha_optimized(vec2 coord) {
    if (pixel_format != PIX_FMT_YUVA420P && 
        pixel_format != PIX_FMT_RGBA && 
        pixel_format != PIX_FMT_BGRA) {
        return 1.0;
    }
    if (pixel_format == PIX_FMT_YUVA420P) {
        return texture2D(tex_a, coord).r;
    } else {
        return texture2D(tex_y, coord).a;
    }
}

// ============================================
// RGB格式处理
// ============================================

vec4 process_rgb_format_optimized(vec2 coord) {
    vec4 color = texture2D(tex_y, coord);
    return color;
}

// ============================================
// 主函数
// ============================================
void main() {

    vec4 final_color;

    // ---------- RGB / BGR / RGBA / BGRA ----------
    if (pixel_format >= PIX_FMT_RGB24 && pixel_format <= PIX_FMT_BGRA) {
        final_color = process_rgb_format_optimized(v_coord);
        // 若需要 sRGB Gamma 校正（SDR 通常不需要，可注释）
        // if (eotf_type == EOTF_SDR_GAMMA) {
        //     final_color.rgb = pow(final_color.rgb, vec3(1.0 / GAMMA_2_2));
        //     final_color.rgb *= 100.0;
        // }
    }
    // ---------- 灰度 ----------
    else if (pixel_format == PIX_FMT_GRAY8) {
        float gray = texture2D(tex_y, v_coord).r;
        final_color = vec4(gray, gray, gray, 1.0);
        // GRAY8 是全范围，直接输出，禁止任何偏移/缩放！
    }
    // ---------- YUV 家族（平面/半平面/打包）----------
    else {
        vec3 yuv;
        if (pixel_format == PIX_FMT_YUYV422 || pixel_format == PIX_FMT_UYVY422)
            yuv = get_yuv_from_packed_optimized(v_coord);
        else
            yuv = get_yuv_from_planar_optimized(v_coord);

        // ---------- 黄金三步（统一处理 8/10/12/16-bit）----------
        yuv *= ten_bit_scale;       // 10-bit 左对齐=1，右对齐=64
        yuv += yuv_offset;          // 有限/全范围偏移
        yuv.x *= y_scale;           // Y 范围缩放（有限范围需要）
        yuv.yz *= uv_scale;         // UV 范围缩放（有限范围需要）

        vec3 rgb = yuv_to_rgb_matrix * yuv;
        rgb = clamp(rgb, 0.0, 1.0);

        /*// ---------- EOTF / 色调映射（SDR 可跳过）----------
        rgb = apply_eotf_fast(rgb, eotf_type, color_range);
        if (eotf_type != EOTF_SDR_GAMMA) {
            rgb = tonemap_adaptive_fast(rgb, display_peak_nits, hdr_metadata);
        } else {
            rgb /= 100.0;   // 将 100-nit SDR 缩放到 [0,1]
        }*/
        //////////////////////////////////////////////////////
         // ---------- 处理 HDR 内容（PQ/HLG），SDR 直接输出 ----------
        if (eotf_type == EOTF_HLG || eotf_type == EOTF_PQ) {
            rgb = apply_eotf_fast(rgb, eotf_type, color_range);
            rgb = tonemap_adaptive_fast(rgb, display_peak_nits, hdr_metadata);
        }
        ////////////////////////////////////////////////////////////

        float alpha = get_alpha_optimized(v_coord);
        rgb = clamp(rgb, 0.0, 1.0);
        final_color = vec4(rgb, alpha);
    }
    gl_FragColor = clamp(final_color, 0.0, 1.0);
})"


#define VERT_SHADER R"(
#version 130
uniform mat4 u_pm;
uniform vec4 draw_pos;

const vec2 verts[4] = vec2[] (
  vec2(-0.5,  0.5),
  vec2(-0.5, -0.5),
  vec2( 0.5,  0.5),
  vec2( 0.5, -0.5)
);

const vec2 texcoords[4] = vec2[] (
  vec2(0.0, 1.0),
  vec2(0.0, 0.0),
  vec2(1.0, 1.0),
  vec2(1.0, 0.0)
);

out vec2 v_coord;

void main() {
   vec2 vert = verts[gl_VertexID];
   vec4 p = vec4((0.5 * draw_pos.z) + draw_pos.x + (vert.x * draw_pos.z),
                 (0.5 * draw_pos.w) + draw_pos.y + (vert.y * draw_pos.w),
                 0, 1);
   gl_Position = u_pm * p;
   v_coord = texcoords[gl_VertexID];
})"

bool operator==(const AVRational& r1, const AVRational& r2) 
{
	return memcmp(&r1, &r2, sizeof(AVRational)) == 0;
}

QtGLVideoWidget::QtGLVideoWidget(QWidget* pParentWidget) :QOpenGLWidget(pParentWidget)
{
	//resize(1280, 720);
	//QSurfaceFormat defaultFormat = QSurfaceFormat::defaultFormat();
	//defaultFormat.setProfile(QSurfaceFormat::CoreProfile);
	//defaultFormat.setVersion(3, 3); // Adapt to your system
	//QSurfaceFormat::setDefaultFormat(defaultFormat);
	//setFormat(defaultFormat);

    // 设置OpenGL版本和配置文件
    QSurfaceFormat format;
   // format.setVersion(3, 3);  // 或更高版本，如4.0, 4.5等
    format.setVersion(1, 3);  // 或更高版本，如4.0, 4.5等
    format.setProfile(QSurfaceFormat::CoreProfile);  // 核心配置文件
    //format.setOption(QSurfaceFormat::DeprecatedFunctions, false);  // 禁用已弃用函数

    // 可选：设置深度缓冲、多重采样等
    format.setDepthBufferSize(24);
    format.setSamples(4);  // 4x多重采样

    setFormat(format);



	m_frameSave = nullptr;
	initConnect();
	setMouseTracking(true);
	setAcceptDrops(true);

	m_frame_tmp = av_frame_alloc();
}

QtGLVideoWidget::~QtGLVideoWidget()
{
	deleteTex();
	if (m_frameSave)
	{
		//av_frame_unref(m_frameSave);
		av_frame_free(&m_frameSave);
	}
	if (m_frame_tmp) 
	{
		//av_frame_unref(m_frame_tmp);
		av_frame_free(&m_frame_tmp);
	}
}

void QtGLVideoWidget::renderFrame(AVFrame* f)
{
	if (f == nullptr || !isSupportAVPixelFormat((AVPixelFormat)f->format)/*f->format != m_pixNeeded*/)
	{
		//if (!m_isOpenGLInit) 
		{
			std::lock_guard<std::mutex>lock(m_mutex);
			if (m_frameSave)
			{
				//av_frame_unref(m_frameSave);
				av_frame_free(&m_frameSave);
			}
			m_frameSave = nullptr;
		}
		if (f) 
		{
			//av_frame_unref(f);
			av_frame_free(&f);
		}
		//return;
	}
    else{
		std::lock_guard<std::mutex>lock(m_mutex);
		if (m_frameSave)
		{
			//av_frame_unref(m_frameSave);
			av_frame_free(&m_frameSave);
		}
		m_frameSave = f;
		m_bUploadTex = true;
	}
	update();
	//emit signalsGetFrame();
}

bool QtGLVideoWidget::isSupportAVPixelFormat(AVPixelFormat fmt)
{
    return mapPixelFormat(fmt) != PIX_FMT_NONE;
}
std::vector<AVPixelFormat> QtGLVideoWidget::getSupportAVPixelFormat()
{

        return std::vector<AVPixelFormat>{AV_PIX_FMT_YUV420P,
        AV_PIX_FMT_YUV422P,
        AV_PIX_FMT_YUV444P,
        AV_PIX_FMT_NV12,
        AV_PIX_FMT_NV21,
        AV_PIX_FMT_YUYV422,
        AV_PIX_FMT_UYVY422,
        AV_PIX_FMT_YUV420P10LE,
        AV_PIX_FMT_YUVA420P,
        AV_PIX_FMT_RGB24,
        AV_PIX_FMT_RGBA,
        AV_PIX_FMT_BGR24,
        AV_PIX_FMT_BGRA,
        AV_PIX_FMT_GRAY8,
        AV_PIX_FMT_P010LE,
        AV_PIX_FMT_YUV422P10LE,
        AV_PIX_FMT_YUV444P10LE
        };
}

QRect QtGLVideoWidget::calculate_display_rect(AVFrame* f)
{
	if(f)
		return calculate_display_rect(0, 0, width(), height(), f->width, f->height, f->sample_aspect_ratio);
	return QRect();
}

QRect QtGLVideoWidget::calculate_display_rect(
	int scr_xleft, int scr_ytop, int scr_width, int scr_height,
	int pic_width, int pic_height, AVRational pic_sar)
{
	/*if (m_iPreFrameXOffset == scr_xleft
		&& m_iPreFrameYOffset == scr_ytop
		&& m_iPreFrameWidth == pic_width
		&& m_iPreFrameHeight == pic_height
		&& m_iPreFrameWinWidth == scr_width
		&& m_iPreFrameWinHeight == scr_height
		&& m_preFrameRatio == pic_sar)
		return;*/

	if (m_iPreFrameWidth != pic_width
		|| m_iPreFrameHeight != pic_height)
		m_isOpenGLInit = false;

	m_iPreFrameXOffset = scr_xleft;
	m_iPreFrameYOffset = scr_ytop;
	m_iPreFrameWidth = pic_width;
	m_iPreFrameHeight = pic_height;
	m_iPreFrameWinWidth = scr_width;
	m_iPreFrameWinHeight = scr_height;
	m_preFrameRatio = pic_sar;
	AVRational aspect_ratio = pic_sar;
	int64_t width, height, x, y;

	if (av_cmp_q(aspect_ratio, av_make_q(0, 1)) <= 0)
		aspect_ratio = av_make_q(1, 1);
	aspect_ratio = av_mul_q(aspect_ratio, av_make_q(pic_width, pic_height));

	/* XXX: we suppose the screen has a 1.0 pixel ratio */
	height = scr_height;
	width = av_rescale(height, aspect_ratio.num, aspect_ratio.den) & ~1;
	if (width > scr_width) {
		width = scr_width;
		height = av_rescale(width, aspect_ratio.den, aspect_ratio.num) & ~1;
	}
	x = (scr_width - width) / 2;
	y = (scr_height - height) / 2;
	return QRect(scr_xleft + x, scr_ytop + y, FFMAX((int)width, 1), FFMAX((int)height, 1));
	/*m_drawRect.setX(scr_xleft + x);
	m_drawRect.setY(scr_ytop + y);
	m_drawRect.setWidth(FFMAX((int)width, 1));
	m_drawRect.setHeight(FFMAX((int)height, 1));*/
	//qDebug() << "calc width:" << this->width() << "\theight:" << this->height();
}

void QtGLVideoWidget::deleteTex()
{
    for (auto& tex : yuva_tex) 
        if(tex)
            glDeleteTextures(1, &tex);
}

void QtGLVideoWidget::initConnect()
{
}

void QtGLVideoWidget::setFrameSize(int width, int height)
{
	m_iPreFrameWidth = width;
	m_iPreFrameHeight = height;
	m_isOpenGLInit = false;
}

void QtGLVideoWidget::initializeGL()
{
	initializeOpenGLFunctions();
	m_program.removeAllShaders();
	m_program.destroyed();
	if (!m_program.create())
	{
		qDebug() << "m_program.create() failed";
	}
	glDisable(GL_DEPTH_TEST);

    if (!m_program.addShaderFromSourceCode(QOpenGLShader::Vertex, VERT_SHADER)) 
    {
        qDebug() << "VERT shader compile error:" << m_program.log();
    }

    if (!m_program.addShaderFromSourceCode(QOpenGLShader::Fragment, FRAG_SHADER_YUV_UNIVERSAL_OPTIMIZED)) 
    {
        qDebug() << "Fragment shader compile error:" << m_program.log();
    }
   
    if (!m_program.link()) 
    {
        qDebug() << "Shader program link failed:" << m_program.log();
    }
    else {
        qDebug() << "Shader program link success!";
    }

    initUniformLocations();

	u_pos = m_program.uniformLocation("draw_pos");
	m_vao.destroy();
	m_vao.create();
}

void QtGLVideoWidget::paintGL()
{
    if (!m_frameSave)
    {
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        return;
    }
    bool bUploadTex;
    {
        std::lock_guard<std::mutex>lock(m_mutex);
        if (bUploadTex = m_bUploadTex)
        {
            av_frame_unref(m_frame_tmp);
            if (m_frameSave)
                av_frame_ref(m_frame_tmp, m_frameSave);
        }
    }
    QRect rect = calculate_display_rect(m_frame_tmp);
    if (!m_isOpenGLInit)
    {
        initializeTextures((AVPixelFormat)m_frame_tmp->format, m_frame_tmp->width, m_frame_tmp->height);
        m_isOpenGLInit = true;
    }

    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    m_program.bind();
    smartSetUniforms(m_frame_tmp,2000);

    QMatrix4x4 m;
    m.ortho(0, width(), height(), 0.0, 0.0, 100.0f);
    m_program.setUniformValue("u_pm", m);
    glUniform4f(u_pos, rect.left(), rect.top(), rect.width(), rect.height());
    //setup_hdr_uniforms(m_frame_tmp);
    int w = m_frame_tmp->width;
    int h = m_frame_tmp->height;
    switch ((AVPixelFormat)m_frame_tmp->format)
    {
    case AV_PIX_FMT_YUV420P:      // YUV420平面
        glActiveTexture(GL_TEXTURE0);
        if (bUploadTex)
            bindPixelTexture(yuva_tex[0], w, h, GL_RED, GL_UNSIGNED_BYTE, m_frame_tmp->data[0], m_frame_tmp->linesize[0]);
        glActiveTexture(GL_TEXTURE1);
        if (bUploadTex)
            bindPixelTexture(yuva_tex[1], w / 2, h / 2, GL_RED, GL_UNSIGNED_BYTE, m_frame_tmp->data[1], m_frame_tmp->linesize[1]);
        glActiveTexture(GL_TEXTURE2);
        if (bUploadTex)
            bindPixelTexture(yuva_tex[2], w / 2, h / 2, GL_RED, GL_UNSIGNED_BYTE, m_frame_tmp->data[2], m_frame_tmp->linesize[2]);
        break;

    case AV_PIX_FMT_YUV422P:      // YUV422平面
        glActiveTexture(GL_TEXTURE0);
        if (bUploadTex)
            bindPixelTexture(yuva_tex[0], w, h, GL_RED, GL_UNSIGNED_BYTE, m_frame_tmp->data[0], m_frame_tmp->linesize[0]);
        glActiveTexture(GL_TEXTURE1);
        if (bUploadTex)
            bindPixelTexture(yuva_tex[1], w / 2, h, GL_RED, GL_UNSIGNED_BYTE, m_frame_tmp->data[1], m_frame_tmp->linesize[1]);
        glActiveTexture(GL_TEXTURE2);
        if (bUploadTex)
            bindPixelTexture(yuva_tex[2], w / 2, h, GL_RED, GL_UNSIGNED_BYTE, m_frame_tmp->data[2], m_frame_tmp->linesize[2]);
        break;

    case AV_PIX_FMT_YUV444P:      // YUV444平面
        glActiveTexture(GL_TEXTURE0);
        if (bUploadTex)
            bindPixelTexture(yuva_tex[0], w, h, GL_RED, GL_UNSIGNED_BYTE, m_frame_tmp->data[0], m_frame_tmp->linesize[0]);
        glActiveTexture(GL_TEXTURE1);
        if (bUploadTex)
            bindPixelTexture(yuva_tex[1], w, h, GL_RED, GL_UNSIGNED_BYTE, m_frame_tmp->data[1], m_frame_tmp->linesize[1]);
        glActiveTexture(GL_TEXTURE2);
        if (bUploadTex)
            bindPixelTexture(yuva_tex[2], w, h, GL_RED, GL_UNSIGNED_BYTE, m_frame_tmp->data[2], m_frame_tmp->linesize[2]);
        break;

    case AV_PIX_FMT_NV12:         // YUV420半平面 (UV交错)
    case AV_PIX_FMT_NV21:         // YUV420半平面 (VU交错)
        glActiveTexture(GL_TEXTURE0);
        if (bUploadTex)
            bindPixelTexture(yuva_tex[0], w, h, GL_RED, GL_UNSIGNED_BYTE, m_frame_tmp->data[0], m_frame_tmp->linesize[0]);
        glActiveTexture(GL_TEXTURE1);
        if (bUploadTex)
            bindPixelTexture(yuva_tex[1], w / 2, h / 2, GL_RG, GL_UNSIGNED_BYTE, m_frame_tmp->data[1], m_frame_tmp->linesize[1] / 2);
        //bindPixelTexture(yuva_tex[1], w/2, h / 2, GL_RG, GL_UNSIGNED_BYTE, m_frame_tmp->data[1], m_frame_tmp->linesize[1]);
        break;

    case AV_PIX_FMT_YUYV422:      // YUYV打包
    case AV_PIX_FMT_UYVY422:      // UYVY打包
        glActiveTexture(GL_TEXTURE0);
        if (bUploadTex)
            bindPixelTexture(yuva_tex[0], w / 2, h, GL_RGBA, GL_UNSIGNED_BYTE, m_frame_tmp->data[0], m_frame_tmp->linesize[0] / 4);
        break;

    case AV_PIX_FMT_YUV420P10LE:  // 10位YUV420
        glActiveTexture(GL_TEXTURE0);
        if (bUploadTex)
            bindPixelTexture(yuva_tex[0], w, h, GL_RED, GL_UNSIGNED_SHORT, m_frame_tmp->data[0], m_frame_tmp->linesize[0] / 2);
        glActiveTexture(GL_TEXTURE1);
        if (bUploadTex)
            bindPixelTexture(yuva_tex[1], w / 2, h / 2, GL_RED, GL_UNSIGNED_SHORT, m_frame_tmp->data[1], m_frame_tmp->linesize[1] / 2);
        glActiveTexture(GL_TEXTURE2);
        if (bUploadTex)
            bindPixelTexture(yuva_tex[2], w / 2, h / 2, GL_RED, GL_UNSIGNED_SHORT, m_frame_tmp->data[2], m_frame_tmp->linesize[2] / 2);
        break;

    case AV_PIX_FMT_YUVA420P:     // YUVA420平面
        glActiveTexture(GL_TEXTURE0);
        if (bUploadTex)
            bindPixelTexture(yuva_tex[0], w, h, GL_RED, GL_UNSIGNED_BYTE, m_frame_tmp->data[0], m_frame_tmp->linesize[0]);
        glActiveTexture(GL_TEXTURE1);
        if (bUploadTex)
            bindPixelTexture(yuva_tex[1], w / 2, h / 2, GL_RED, GL_UNSIGNED_BYTE, m_frame_tmp->data[1], m_frame_tmp->linesize[1]);
        glActiveTexture(GL_TEXTURE2);
        if (bUploadTex)
            bindPixelTexture(yuva_tex[2], w / 2, h / 2, GL_RED, GL_UNSIGNED_BYTE, m_frame_tmp->data[2], m_frame_tmp->linesize[2]);
        glActiveTexture(GL_TEXTURE3);
        if (bUploadTex)
            bindPixelTexture(yuva_tex[3], w, h, GL_RED, GL_UNSIGNED_BYTE, m_frame_tmp->data[3], m_frame_tmp->linesize[3]);
        break;

    case AV_PIX_FMT_RGB24:        // RGB24
        glActiveTexture(GL_TEXTURE0);
        if (bUploadTex)
            bindPixelTexture(yuva_tex[0], w, h, GL_RGB, GL_UNSIGNED_BYTE, m_frame_tmp->data[0], m_frame_tmp->linesize[0] / 3);
        break;

    case AV_PIX_FMT_RGBA:         // RGBA

        glActiveTexture(GL_TEXTURE0);
        if (bUploadTex)
            bindPixelTexture(yuva_tex[0], w, h, GL_RGBA, GL_UNSIGNED_BYTE, m_frame_tmp->data[0], m_frame_tmp->linesize[0] / 4);
        break;

    case AV_PIX_FMT_BGR24:       // BGR24

        glActiveTexture(GL_TEXTURE0);
        if (bUploadTex)
            bindPixelTexture(yuva_tex[0], w, h, GL_BGR, GL_UNSIGNED_BYTE, m_frame_tmp->data[0], m_frame_tmp->linesize[0] / 3);
        break;

    case AV_PIX_FMT_BGRA:        // BGRA
        glActiveTexture(GL_TEXTURE0);
        if (bUploadTex)
            bindPixelTexture(yuva_tex[0], w, h, GL_BGRA, GL_UNSIGNED_BYTE, m_frame_tmp->data[0], m_frame_tmp->linesize[0] / 4);
        break;

    case AV_PIX_FMT_GRAY8:       // 灰度
        glActiveTexture(GL_TEXTURE0);
        if (bUploadTex)
            bindPixelTexture(yuva_tex[0], w, h, GL_RED, GL_UNSIGNED_BYTE, m_frame_tmp->data[0], m_frame_tmp->linesize[0]);
        break;

    case AV_PIX_FMT_P010LE:      // P010 (10位半平面，小端)
        glActiveTexture(GL_TEXTURE0);
        if (bUploadTex)
            bindPixelTexture(yuva_tex[0], w, h, GL_RED, GL_UNSIGNED_SHORT, m_frame_tmp->data[0], m_frame_tmp->linesize[0] / 2);
        glActiveTexture(GL_TEXTURE1);
        if (bUploadTex)
            bindPixelTexture(yuva_tex[1], w / 2, h / 2, GL_RG, GL_UNSIGNED_SHORT, m_frame_tmp->data[1], m_frame_tmp->linesize[1] / 4);
        break;

    case AV_PIX_FMT_YUV422P10LE: // 10位YUV422
        glActiveTexture(GL_TEXTURE0);
        if (bUploadTex)
            bindPixelTexture(yuva_tex[0], w, h, GL_RED, GL_UNSIGNED_SHORT, m_frame_tmp->data[0], m_frame_tmp->linesize[0] / 2);
        glActiveTexture(GL_TEXTURE1);
        if (bUploadTex)
            bindPixelTexture(yuva_tex[1], w / 2, h, GL_RED, GL_UNSIGNED_SHORT, m_frame_tmp->data[1], m_frame_tmp->linesize[1] / 2);
        glActiveTexture(GL_TEXTURE2);
        if (bUploadTex)
            bindPixelTexture(yuva_tex[2], w / 2, h, GL_RED, GL_UNSIGNED_SHORT, m_frame_tmp->data[2], m_frame_tmp->linesize[2] / 2);
        break;

    case AV_PIX_FMT_YUV444P10LE: // 10位YUV444
        glActiveTexture(GL_TEXTURE0);
        if (bUploadTex)
            bindPixelTexture(yuva_tex[0], w, h, GL_RED, GL_UNSIGNED_SHORT, m_frame_tmp->data[0], m_frame_tmp->linesize[0] / 2);
        glActiveTexture(GL_TEXTURE1);
        if (bUploadTex)
            bindPixelTexture(yuva_tex[1], w, h, GL_RED, GL_UNSIGNED_SHORT, m_frame_tmp->data[1], m_frame_tmp->linesize[1] / 2);
        glActiveTexture(GL_TEXTURE2);
        if (bUploadTex)
            bindPixelTexture(yuva_tex[2], w, h, GL_RED, GL_UNSIGNED_SHORT, m_frame_tmp->data[2], m_frame_tmp->linesize[2] / 2);
        break;

    case AV_PIX_FMT_YUVJ420P:    // JPEG范围YUV420P
    case AV_PIX_FMT_YUVJ422P:    // JPEG范围YUV422P
    case AV_PIX_FMT_YUVJ444P:    // JPEG范围YUV444P
        glActiveTexture(GL_TEXTURE0);
        if (bUploadTex)
            bindPixelTexture(yuva_tex[0], w, h, GL_RED, GL_UNSIGNED_BYTE, m_frame_tmp->data[0], m_frame_tmp->linesize[0]);
        glActiveTexture(GL_TEXTURE1);
        if (bUploadTex)
            bindPixelTexture(yuva_tex[1], w / 2, h / 2, GL_RED, GL_UNSIGNED_BYTE, m_frame_tmp->data[1], m_frame_tmp->linesize[1]);
        glActiveTexture(GL_TEXTURE2);
        if (bUploadTex)
            bindPixelTexture(yuva_tex[2], w / 2, h / 2, GL_RED, GL_UNSIGNED_BYTE, m_frame_tmp->data[2], m_frame_tmp->linesize[2]);
        break;

    case AV_PIX_FMT_NONE:
    default:
        break;
    }
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindTexture(GL_TEXTURE_2D, 0);
    m_program.release();
    m_vao.release();


}

void QtGLVideoWidget::resizeGL(int width, int height)
{
	if(m_frameSave)
		calculate_display_rect(0, 0, width, height, m_frameSave->width, m_frameSave->height, m_frameSave->sample_aspect_ratio);
    update();
}
// 初始化时获取所有uniform位置
void QtGLVideoWidget::initUniformLocations() {
    uniforms.tex_y = m_program.uniformLocation("tex_y");//glGetUniformLocation(program, "tex_y");
    uniforms.tex_u = m_program.uniformLocation("tex_u");//glGetUniformLocation(program, "tex_u");
    uniforms.tex_v = m_program.uniformLocation("tex_v");//glGetUniformLocation(program, "tex_v");
    uniforms.tex_a = m_program.uniformLocation("tex_a");//glGetUniformLocation(program, "tex_a");

    uniforms.pixel_format = m_program.uniformLocation("pixel_format"); //glGetUniformLocation(program, "pixel_format");
    //uniforms.color_space = m_program.uniformLocation("color_space");//glGetUniformLocation(program, "color_space");
    uniforms.color_range = m_program.uniformLocation("color_range");//glGetUniformLocation(program, "color_range");
    uniforms.eotf_type = m_program.uniformLocation("eotf_type");//glGetUniformLocation(program, "eotf_type");
    uniforms.swap_uv = m_program.uniformLocation("swap_uv");//glGetUniformLocation(program, "is_10bit");

    uniforms.display_peak = m_program.uniformLocation("display_peak_nits");//glGetUniformLocation(program, "display_peak_nits");
    uniforms.system_gamma = m_program.uniformLocation("system_gamma");//glGetUniformLocation(program, "system_gamma");
    uniforms.hdr_metadata = m_program.uniformLocation("hdr_metadata");//glGetUniformLocation(program, "hdr_metadata");

    uniforms.yuv_to_rgb_matrix = m_program.uniformLocation("yuv_to_rgb_matrix");//glGetUniformLocation(program, "yuv_to_rgb_matrix");
    uniforms.yuv_offset = m_program.uniformLocation("yuv_offset");//glGetUniformLocation(program, "yuv_offset");
    uniforms.y_scale = m_program.uniformLocation("y_scale");
    uniforms.uv_scale = m_program.uniformLocation("uv_scale");
    uniforms.chroma_offset = m_program.uniformLocation("chroma_offset");//glGetUniformLocation(program, "chroma_offset");
    uniforms.ten_bit_scale = m_program.uniformLocation("ten_bit_scale");

}

// 智能设置uniform，只更新变化的值
void QtGLVideoWidget::smartSetUniforms(AVFrame* frame, float display_peak_nits) {
    //glUseProgram(m_program.programId());
    // 1. 设置纹理单元（总是设置，因为纹理可能变化）
    glUniform1i(uniforms.tex_y, 0);
    glUniform1i(uniforms.tex_u, 1);
    glUniform1i(uniforms.tex_v, 2);
    glUniform1i(uniforms.tex_a, 3);

    // 2. 计算格式相关参数
    AVPixelFormat pixfmt = (AVPixelFormat)frame->format;
    int pixel_format = mapPixelFormat(pixfmt);
    bool is_10bit = is10BitFormat(pixfmt);

    // 3. 智能更新pixel_format
    if (pixel_format != cache.last_pixel_format) {
        glUniform1i(uniforms.pixel_format, pixel_format);
        cache.last_pixel_format = pixel_format;
    }

    // 关键：更新矩阵和偏移，传入 frame 和 pixel_format
    updateConversionMatrix(frame, pixel_format, is_10bit);
  
    // 7. 计算EOTF类型
    AVColorTransferCharacteristic transfer = frame->color_trc;
    int eotf_type = getEOTFType(transfer);

    // 8. 智能更新EOTF类型
    if (eotf_type != cache.last_eotf_type) {
        glUniform1i(uniforms.eotf_type, eotf_type);
        cache.last_eotf_type = eotf_type;
    }

    // 9. 智能更新显示峰值亮度
    if (display_peak_nits != cache.last_display_peak) {
        glUniform1f(uniforms.display_peak, display_peak_nits);
        cache.last_display_peak = display_peak_nits;
    }

    // 10. 设置系统Gamma（HLG用）
    float sys_gamma = 1.2f;
    if (sys_gamma != cache.last_system_gamma) {
        glUniform1f(uniforms.system_gamma, sys_gamma);
        cache.last_system_gamma = sys_gamma;
    }

    //// 11. 设置HDR元数据
    setup_hdr_uniforms(m_frame_tmp, display_peak_nits);
    //float max_content = 1000.0f;
    //if (frame->mastering_display_metadata) {
    //    max_content = frame->mastering_display_metadata->max_luminance / 10000.0f;
    //}
    //glUniform2f(uniforms.hdr_metadata, max_content, 0.0f);

    // 12. 设置色度位置
    updateChromaPosition(frame);
}

void QtGLVideoWidget::updateConversionMatrix(AVFrame* frame, int pixel_format, bool is_10bit) {

    // --- 默认值（安全回退）---
    glm::mat3 matrix = glm::mat3(1.0f);
    glm::vec3 offset = glm::vec3(0.0f);
    float y_scale = 1.0f, uv_scale = 1.0f, ten_bit_scale = 1.0f;
    int swap_uv = 0;


    // 1. 获取原始元数据（可能 UNSPECIFIED）
    AVColorSpace cs = frame->colorspace;
    AVColorRange range = frame->color_range;

    // 2. 处理未指定值
    if (cs == AVCOL_SPC_UNSPECIFIED) {
        // 根据分辨率猜测（您原有的函数）
        cs = guessColorSpace(frame);
    }
    if (range == AVCOL_RANGE_UNSPECIFIED) {
        range = AVCOL_RANGE_MPEG;  // 默认有限范围
    }
    // 4. 计算矩阵和偏移
    bool is_full_range = (range == AVCOL_RANGE_JPEG);

    // --- 根据像素格式和元数据设置精确参数 ---
    switch (pixel_format) {
        // ----- 8-bit YUV 独立平面（经 sws_scale 转换）-----
    case PIX_FMT_YUV420P:
    case PIX_FMT_YUV422P:
    case PIX_FMT_YUV444P:
        matrix = fullRangeMatrix(cs); // BT.601 全范围矩阵
        offset = glm::vec3(-16.0f / 255.0f, -128.0f / 255.0f, -128.0f / 255.0f);
        y_scale = 255.0f / 219.0f;
        uv_scale = 255.0f / 224.0f;
        ten_bit_scale = 1.0f;
        break;

        // ----- 10-bit YUV 独立平面（右对齐）-----
    case PIX_FMT_YUV420P10LE:
        matrix = fullRangeMatrix(cs);
        offset = glm::vec3(-64.0f / 1023.0f, -512.0f / 1023.0f, -512.0f / 1023.0f);
        y_scale = 1023.0f / 876.0f;
        uv_scale = 1023.0f / 896.0f;
        ten_bit_scale = 65535.0f / 1023.0f;   // 右对齐必须乘 64
        break;

        // ----- NV12（硬解码，通常 BT.709 有限/全）-----
    case PIX_FMT_NV12:
        matrix = fullRangeMatrix(cs);
        if (frame->color_range == AVCOL_RANGE_MPEG) {
            offset = glm::vec3(-16.0f / 255.0f, -128.0f / 255.0f, -128.0f / 255.0f);
            y_scale = 255.0f / 219.0f;
            uv_scale = 255.0f / 224.0f;
        }
        else {
            offset = glm::vec3(0.0f, -0.5f, -0.5f);
            y_scale = uv_scale = 1.0f;
        }
        ten_bit_scale = 1.0f;
        break;
        // NV21（VU 顺序）
    case PIX_FMT_NV21:
        matrix = fullRangeMatrix(cs);
        if (frame->color_range == AVCOL_RANGE_MPEG) {
            offset = glm::vec3(-16.0f / 255.0f, -128.0f / 255.0f, -128.0f / 255.0f);
            y_scale = 255.0f / 219.0f;
            uv_scale = 255.0f / 224.0f;
        }
        else {
            offset = glm::vec3(0.0f, -0.5f, -0.5f);
            y_scale = uv_scale = 1.0f;
        }
        ten_bit_scale = 1.0f;
        swap_uv = 1;
        break;
        // ----- P010（经 sws_scale 转换，左对齐 BT.601 有限）-----
    case PIX_FMT_P010LE:
        matrix = fullRangeMatrix(cs);
        offset = glm::vec3(-64.0f / 1023.0f, -512.0f / 1023.0f, -512.0f / 1023.0f);
        y_scale = 1023.0f / 876.0f;
        uv_scale = 1023.0f / 896.0f;
        ten_bit_scale = 1.0f;
        break;

        // ----- 打包格式（YUYV/UYVY）-----
    case PIX_FMT_YUYV422:
    case PIX_FMT_UYVY422:
        matrix = fullRangeMatrix(cs);
        offset = glm::vec3(-16.0f / 255.0f, -128.0f / 255.0f, -128.0f / 255.0f);
        y_scale = 255.0f / 219.0f;
        uv_scale = 255.0f / 224.0f;
        ten_bit_scale = 1.0f;
        break;

        // ----- 灰度 -----
    case PIX_FMT_GRAY8:
        // 无需任何转换，直接输出
        break;

        // ----- RGB 家族 -----
    case PIX_FMT_RGB24:
    case PIX_FMT_BGR24:
    case PIX_FMT_RGBA:
    case PIX_FMT_BGRA:
        // 无需任何 YUV uniform
        break;
    }

    // 5. 更新 uniform
    if (matrix != cache.last_matrix) {
        glUniformMatrix3fv(uniforms.yuv_to_rgb_matrix, 1, GL_FALSE, QMatrix3x3(glm::value_ptr(matrix)).constData());
        cache.last_matrix = matrix;
    }
    if (offset != cache.last_offset) {
        glUniform3fv(uniforms.yuv_offset, 1, glm::value_ptr(offset));
        cache.last_offset = offset;
    }

    if (y_scale != cache.last_y_scale)
    {
        glUniform1f(uniforms.y_scale, y_scale);
        cache.last_y_scale = y_scale;
    }
    if (uv_scale != cache.last_uv_scale) {
        glUniform1f(uniforms.uv_scale, uv_scale);
        cache.last_uv_scale = uv_scale;
    }

    if (ten_bit_scale != cache.last_ten_bit_scale)
    {
        glUniform1f(uniforms.ten_bit_scale, ten_bit_scale);
        cache.last_ten_bit_scale = ten_bit_scale;
    }
    if (swap_uv != cache.last_swap_uv)
    {
        glUniform1i(uniforms.swap_uv, swap_uv);
        cache.last_swap_uv = swap_uv;
    }
}

void QtGLVideoWidget::updateChromaPosition(AVFrame* frame) {
    glm::vec2 offset(0.0f, 0.0f);   // 强制为 (0,0)
    if (cache.last_chroma_offset != offset) {
        glUniform2fv(uniforms.chroma_offset, 1, glm::value_ptr(offset));
        cache.last_chroma_offset = offset;  // 更新缓存
    }
}

glm::mat3 QtGLVideoWidget::calculateYUVtoRGBMatrix(AVColorSpace cs, bool is_full_range, bool is_10bit) {
    // BT.601
    if (cs == AVCOL_SPC_BT470BG || cs == AVCOL_SPC_SMPTE170M) {
        if (is_full_range) {
            return glm::mat3(
                1.000000f, 0.000000f, 1.402000f,
                1.000000f, -0.344136f, -0.714136f,
                1.000000f, 1.772000f, 0.000000f
            );
        }
        else {
            return glm::mat3(
                1.164384f, 0.000000f, 1.596027f,
                1.164384f, -0.391762f, -0.812968f,
                1.164384f, 2.017232f, 0.000000f
            );
        }
    }
    // BT.709
    else if (cs == AVCOL_SPC_BT709) {
        if (is_full_range) {
            return glm::mat3(
                1.000000f, 0.000000f, 1.574800f,
                1.000000f, -0.187324f, -0.468124f,
                1.000000f, 1.855600f, 0.000000f
            );
        }
        else {
            return glm::mat3(
                1.164384f, 0.000000f, 1.792741f,
                1.164384f, -0.213249f, -0.532909f,
                1.164384f, 2.112402f, 0.000000f
            );
        }
    }
    // BT.2020
    else if (cs == AVCOL_SPC_BT2020_NCL) {
        if (is_full_range) {
            return glm::mat3(
                1.000000f, 0.000000f, 1.474600f,
                1.000000f, -0.164553f, -0.571353f,
                1.000000f, 1.881400f, 0.000000f
            );
        }
        else {
            return glm::mat3(
                1.164384f, 0.000000f, 1.678674f,
                1.164384f, -0.187326f, -0.650424f,
                1.164384f, 2.141772f, 0.000000f
            );
        }
    }
    // 默认：BT.601 有限范围
    else {
        return glm::mat3(
            1.164384f, 0.000000f, 1.596027f,
            1.164384f, -0.391762f, -0.812968f,
            1.164384f, 2.017232f, 0.000000f
        );
    }
}
glm::mat3 QtGLVideoWidget::fullRangeMatrix(AVColorSpace cs)
{
    switch (cs)
    {
    case AVCOL_SPC_BT709:
        return glm::mat3(
            1.000000f, 0.000000f, 1.574800f,
            1.000000f, -0.187324f, -0.468124f,
            1.000000f, 1.855600f, 0.000000f
        );
    case AVCOL_SPC_BT2020_NCL:
        return glm::mat3(
            1.000000f, 0.000000f, 1.474600f,
            1.000000f, -0.164553f, -0.571353f,
            1.000000f, 1.881400f, 0.000000f
        );
    case AVCOL_SPC_BT470BG:
    case AVCOL_SPC_SMPTE170M:
    default:
        return glm::mat3(
            1.000000f, 0.000000f, 1.402000f,
            1.000000f, -0.344136f, -0.714136f,
            1.000000f, 1.772000f, 0.000000f
        );
    }
}

glm::vec3 QtGLVideoWidget::calculateYUVOffset(AVColorRange range, bool is_10bit) {
    bool is_full_range = (range == AVCOL_RANGE_JPEG);
    if (is_full_range) {
        return glm::vec3(0.0f, -0.5f, -0.5f);
    }
    else {
        if (is_10bit) {
            return glm::vec3(-64.0f / 1023.0f, -512.0f / 1023.0f, -512.0f / 1023.0f);
        }
        else {
            return glm::vec3(-16.0f / 255.0f, -128.0f / 255.0f, -128.0f / 255.0f);
        }
    }
}

int QtGLVideoWidget::mapPixelFormat(AVPixelFormat fmt) {
    // 映射FFmpeg格式到我们的枚举
    switch (fmt) {
    case AV_PIX_FMT_YUV420P: return PIX_FMT_YUV420P;
    case AV_PIX_FMT_YUV422P: return PIX_FMT_YUV422P;
    case AV_PIX_FMT_YUV444P: return PIX_FMT_YUV444P;
    case AV_PIX_FMT_NV12: return PIX_FMT_NV12;
    case AV_PIX_FMT_NV21: return PIX_FMT_NV21;
    case AV_PIX_FMT_YUYV422: return PIX_FMT_YUYV422;
    case AV_PIX_FMT_UYVY422: return PIX_FMT_UYVY422;
    case AV_PIX_FMT_YUV420P10LE: return PIX_FMT_YUV420P10LE;
    case AV_PIX_FMT_YUVA420P: return PIX_FMT_YUVA420P;
    case AV_PIX_FMT_RGB24: return PIX_FMT_RGB24;
    case AV_PIX_FMT_RGBA: return PIX_FMT_RGBA;
    case AV_PIX_FMT_BGR24: return PIX_FMT_BGR24;
    case AV_PIX_FMT_BGRA: return PIX_FMT_BGRA;
    case AV_PIX_FMT_GRAY8: return PIX_FMT_GRAY8;
    case AV_PIX_FMT_P010LE: return PIX_FMT_P010;
    case AV_PIX_FMT_YUV422P10LE: return PIX_FMT_YUV422P10LE;
    case AV_PIX_FMT_YUV444P10LE: return PIX_FMT_YUV444P10LE;
    default: return PIX_FMT_NONE;
    }
}

bool QtGLVideoWidget::is10BitFormat(AVPixelFormat fmt) {
    switch (fmt) {
    case AV_PIX_FMT_YUV420P10LE:
    case AV_PIX_FMT_YUV422P10LE:
    case AV_PIX_FMT_YUV444P10LE:
    case AV_PIX_FMT_P010LE:
        return true;
    default:
        return false;
    }
}

int QtGLVideoWidget::mapColorSpace(AVColorSpace cs) {
    switch (cs) {
    case AVCOL_SPC_BT709: return COLOR_SPACE_BT709;
    case AVCOL_SPC_BT2020_NCL:
    case AVCOL_SPC_BT2020_CL: return COLOR_SPACE_BT2020;
    case AVCOL_SPC_RGB: return COLOR_SPACE_RGB;
    default: return COLOR_SPACE_BT601;
    }
}

AVColorSpace QtGLVideoWidget::guessColorSpace(AVFrame* frame) {
    //根据分辨率猜测颜色空间
    return frame->colorspace == AVCOL_SPC_UNSPECIFIED ? AVCOL_SPC_BT470BG: frame->colorspace;


    if (frame->width >= 1280 || frame->height >= 720) {
        return AVCOL_SPC_BT709;  // 高清用BT.709
    }
    else {
        return AVCOL_SPC_BT470BG;  // 标清用BT.601
    }
    //return frame->colorspace == AVCOL_SPC_UNSPECIFIED ? AVCOL_SPC_BT470BG : frame->colorspace;
}

int QtGLVideoWidget::getEOTFType(AVColorTransferCharacteristic transfer) {
    switch (transfer) {
    case AVCOL_TRC_ARIB_STD_B67: return EOTF_HLG;
    case AVCOL_TRC_SMPTE2084: return EOTF_PQ;
    default: return EOTF_SDR_GAMMA;
    }
}


void QtGLVideoWidget::initializeTextures(AVPixelFormat pixfmt, int w, int h)
{
    deleteTex();

    glGenTextures(sizeof(yuva_tex) / sizeof(std::remove_pointer_t<std::decay_t<decltype(yuva_tex)>>), yuva_tex);

    for (auto& tex : yuva_tex) 
    {
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    if (pixfmt != AV_PIX_FMT_NONE)
    {
        // 根据像素格式初始化纹理
        switch (pixfmt)
        {
        case AV_PIX_FMT_YUV420P:      // YUV420平面
            glBindTexture(GL_TEXTURE_2D, yuva_tex[0]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
            glBindTexture(GL_TEXTURE_2D, yuva_tex[1]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, w / 2, h / 2, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
            glBindTexture(GL_TEXTURE_2D, yuva_tex[2]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, w / 2, h / 2, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
            break;

        case AV_PIX_FMT_YUV422P:      // YUV422平面
            glBindTexture(GL_TEXTURE_2D, yuva_tex[0]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
            glBindTexture(GL_TEXTURE_2D, yuva_tex[1]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, w / 2, h, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
            glBindTexture(GL_TEXTURE_2D, yuva_tex[2]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, w / 2, h, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
            break;

        case AV_PIX_FMT_YUV444P:      // YUV444平面
            glBindTexture(GL_TEXTURE_2D, yuva_tex[0]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
            glBindTexture(GL_TEXTURE_2D, yuva_tex[1]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
            glBindTexture(GL_TEXTURE_2D, yuva_tex[2]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
            break;

        case AV_PIX_FMT_NV12:         // YUV420半平面 (UV交错)
        case AV_PIX_FMT_NV21:         // YUV420半平面 (VU交错)
            glBindTexture(GL_TEXTURE_2D, yuva_tex[0]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
            glBindTexture(GL_TEXTURE_2D, yuva_tex[1]);
            // NV12/NV21的UV平面是交错存储的，所以需要RG格式，高度减半
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RG8, w / 2, h / 2, 0, GL_RG, GL_UNSIGNED_BYTE, NULL);
            break;

        case AV_PIX_FMT_YUYV422:      // YUYV打包
        case AV_PIX_FMT_UYVY422:      // UYVY打包
            // YUYV422：每两个像素共享一对UV，纹理宽度为原宽度的一半
            glBindTexture(GL_TEXTURE_2D, yuva_tex[0]);
            // 使用RGBA纹理存储Y0 U Y1 V
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w / 2, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
            break;

        case AV_PIX_FMT_YUV420P10LE:  // 10位YUV420
            glBindTexture(GL_TEXTURE_2D, yuva_tex[0]);
            // 10位数据存储在16位纹理中
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R16, w, h, 0, GL_RED, GL_UNSIGNED_SHORT, NULL);
            glBindTexture(GL_TEXTURE_2D, yuva_tex[1]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R16, w / 2, h / 2, 0, GL_RED, GL_UNSIGNED_SHORT, NULL);
            glBindTexture(GL_TEXTURE_2D, yuva_tex[2]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R16, w / 2, h / 2, 0, GL_RED, GL_UNSIGNED_SHORT, NULL);
            break;

        case AV_PIX_FMT_YUVA420P:     // YUVA420平面
            glBindTexture(GL_TEXTURE_2D, yuva_tex[0]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
            glBindTexture(GL_TEXTURE_2D, yuva_tex[1]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, w / 2, h / 2, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
            glBindTexture(GL_TEXTURE_2D, yuva_tex[2]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, w / 2, h / 2, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
            glBindTexture(GL_TEXTURE_2D, yuva_tex[3]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, NULL); // Alpha平面
            break;

        case AV_PIX_FMT_RGB24:        // RGB24
            glBindTexture(GL_TEXTURE_2D, yuva_tex[0]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
            //glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
            break;

        case AV_PIX_FMT_RGBA:         // RGBA
            glBindTexture(GL_TEXTURE_2D, yuva_tex[0]);
            //glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
            break;

        case AV_PIX_FMT_BGR24:       // BGR24
            glBindTexture(GL_TEXTURE_2D, yuva_tex[0]);
            // BGR通道顺序，可以用GL_BGR格式，或者用GL_RGB然后在着色器中交换通道
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, w, h, 0, GL_BGR, GL_UNSIGNED_BYTE, NULL);
            //glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8, w, h, 0, GL_BGR, GL_UNSIGNED_BYTE, nullptr);
            break;

        case AV_PIX_FMT_BGRA:        // BGRA
            glBindTexture(GL_TEXTURE_2D, yuva_tex[0]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
            //glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, w, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
            break;

        case AV_PIX_FMT_GRAY8:       // 灰度
            glBindTexture(GL_TEXTURE_2D, yuva_tex[0]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
            break;

        case AV_PIX_FMT_P010LE:      // P010 (10位半平面，小端)
            glBindTexture(GL_TEXTURE_2D, yuva_tex[0]);
            // Y平面，16位单通道
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R16, w, h, 0, GL_RED, GL_UNSIGNED_SHORT, NULL);
            glBindTexture(GL_TEXTURE_2D, yuva_tex[1]);
            // UV平面，16位双通道（交错存储）
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16, w / 2, h / 2, 0, GL_RG, GL_UNSIGNED_SHORT, NULL);
            break;

        case AV_PIX_FMT_YUV422P10LE: // 10位YUV422
            glBindTexture(GL_TEXTURE_2D, yuva_tex[0]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R16, w, h, 0, GL_RED, GL_UNSIGNED_SHORT, NULL);
            glBindTexture(GL_TEXTURE_2D, yuva_tex[1]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R16, w / 2, h, 0, GL_RED, GL_UNSIGNED_SHORT, NULL);
            glBindTexture(GL_TEXTURE_2D, yuva_tex[2]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R16, w / 2, h, 0, GL_RED, GL_UNSIGNED_SHORT, NULL);
            break;

        case AV_PIX_FMT_YUV444P10LE: // 10位YUV444
            glBindTexture(GL_TEXTURE_2D, yuva_tex[0]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R16, w, h, 0, GL_RED, GL_UNSIGNED_SHORT, NULL);
            glBindTexture(GL_TEXTURE_2D, yuva_tex[1]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R16, w, h, 0, GL_RED, GL_UNSIGNED_SHORT, NULL);
            glBindTexture(GL_TEXTURE_2D, yuva_tex[2]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R16, w, h, 0, GL_RED, GL_UNSIGNED_SHORT, NULL);
            break;

        case AV_PIX_FMT_YUVJ420P:    // JPEG范围YUV420P
        case AV_PIX_FMT_YUVJ422P:    // JPEG范围YUV422P
        case AV_PIX_FMT_YUVJ444P:    // JPEG范围YUV444P
            // 使用与普通YUV相同的纹理格式，只是着色器处理不同
            glBindTexture(GL_TEXTURE_2D, yuva_tex[0]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
            glBindTexture(GL_TEXTURE_2D, yuva_tex[1]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, w / 2, h / 2, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
            glBindTexture(GL_TEXTURE_2D, yuva_tex[2]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, w / 2, h / 2, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
            break;

        case AV_PIX_FMT_NONE:
        default:
            // 默认使用RGB格式作为后备
            //glBindTexture(GL_TEXTURE_2D, yuva_tex[0]);
            //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
            break;
        }

        // 确保解绑纹理
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

//void QtGLVideoWidget::bindPixelTexture(GLuint texture, QtGLVideoWidget::YUVTextureType textureType, uint8_t* pixels, int stride)
//{
//	if (!pixels)
//		return;
//
//	int frameW = m_iPreFrameWidth;
//	int frameH = m_iPreFrameHeight;
//
//	int const width = textureType == YTexture ? frameW : frameW / 2;
//	int const height = textureType == YTexture ? frameH : frameH / 2;
//	glBindTexture(GL_TEXTURE_2D, texture);
//	glPixelStorei(GL_UNPACK_ROW_LENGTH, stride);
//	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RED, GL_UNSIGNED_BYTE, pixels);
//}

void QtGLVideoWidget::bindPixelTexture(GLuint texture, int texW, int texH, GLenum format, GLenum type,uint8_t* pixels, int stride)
{
    if (!pixels)
        return;
    glBindTexture(GL_TEXTURE_2D, texture);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, stride);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texW, texH, format, type, pixels);
}



// 在你的渲染器中使用
void QtGLVideoWidget::setup_hdr_uniforms(AVFrame* frame, float display_peak_nits) {
    // 1. 解析HDR元数据
    HDRMetadataParser::HDRInfo hdr_info =
        HDRMetadataParser::parse_frame(frame);

    // 2. 获取显示器能力
    //float display_peak_nits = 2000;//get_display_peak_brightness();

    // 3. 如果显示器能力未知，使用安全值
    if (display_peak_nits <= 0.0f) {
        // 根据HDR内容调整默认值
        if (hdr_info.is_hdr) {
            display_peak_nits = 1000.0f;  // HDR显示器默认值
        }
        else {
            display_peak_nits = 300.0f;   // SDR显示器默认值
        }
    }

    // 4. 获取着色器uniform位置
    //GLint loc_display_peak = m_program.uniformLocation("display_peak_nits");//glGetUniformLocation(m_program, "display_peak_nits");
    //GLint loc_hdr_metadata = m_program.uniformLocation("hdr_metadata");//glGetUniformLocation(m_program, "hdr_metadata");
    //GLint loc_system_gamma = m_program.uniformLocation("system_gamma");//glGetUniformLocation(m_program, "system_gamma");
    //GLint loc_hdr_info = m_program.uniformLocation("hdr_info");//glGetUniformLocation(m_program, "hdr_info");  // 可能需要

    // 5. 设置uniform

    // 显示峰值亮度
    if (uniforms.display_peak != -1) {
        glUniform1f(uniforms.display_peak, display_peak_nits);
    }

    // HDR元数据（最大内容亮度和平均亮度）
    if (uniforms.hdr_metadata != -1) {
        float max_content = hdr_info.content.has_data ?
            hdr_info.content.max_content_light_level :
            hdr_info.mastering.display_peak_luminance;

        float max_average = hdr_info.content.has_data ?
            hdr_info.content.max_frame_average_light_level :
            max_content * 0.7f;  // 估计值

        glUniform2f(uniforms.hdr_metadata, max_content, max_average);
    }

    // 系统Gamma（HLG需要）
    if (uniforms.system_gamma != -1) {
        float system_gamma = 1.2f;  // HLG默认

        // 如果是SDR，使用sRGB Gamma
        if (!hdr_info.is_hlg && !hdr_info.is_pq) {
            system_gamma = 2.2f;
        }
        glUniform1f(uniforms.system_gamma, system_gamma);
    }

    //// 额外HDR信息（如果需要）
    //if (loc_hdr_info != -1) {
    //    // 可以用vec4传递更多信息
    //    // x: 是否HDR, y: 是否PQ, z: 是否HLG, w: 是否有HDR10+
    //    glUniform4f(loc_hdr_info,
    //        hdr_info.is_hdr ? 1.0f : 0.0f,
    //        hdr_info.is_pq ? 1.0f : 0.0f,
    //        hdr_info.is_hlg ? 1.0f : 0.0f,
    //        hdr_info.has_hdr10_plus ? 1.0f : 0.0f);
    //}

    // 6. 设置原色矩阵（如果需要色域转换）
   /* if (hdr_info.mastering.has_primaries) {
        setup_color_primaries_matrix(hdr_info);
    }*/
}

// 辅助函数：设置原色矩阵
//void QtGLVideoWidget::setup_color_primaries_matrix(const HDRMetadataParser::HDRInfo& hdr_info) {
//    // 如果内容使用的原色与显示器不同，需要进行色域转换
//    // 这里简化处理，实际需要计算3x3的转换矩阵
//
//    // 获取uniform位置
//    GLint loc_color_matrix = m_program.uniformLocation("color_primaries_matrix");//glGetUniformLocation(m_program, "color_primaries_matrix");
//
//    if (loc_color_matrix != -1 && hdr_info.mastering.has_primaries) {
//        // 计算从内容原色到显示器原色的转换矩阵
//        // 这里简化，假设显示器是sRGB/BT.709
//        glm::mat3 matrix = calculate_color_conversion_matrix(
//           hdr_info.mastering.red_primary,
//            hdr_info.mastering.green_primary,
//            hdr_info.mastering.blue_primary,
//            hdr_info.mastering.white_point
//        );
//
//        glUniformMatrix3fv(loc_color_matrix, 1, GL_FALSE, glm::value_ptr(matrix));
//    }
//}

// 颜色转换矩阵计算（简化）
glm::mat3 QtGLVideoWidget::calculate_color_conversion_matrix(const float (& src_red)[2], const float(& src_green)[2],
    const float (& src_blue)[2], const float(& src_white)[2]) {
    // 这里应该实现完整的颜色空间转换矩阵计算
    // 实际应用可能使用预定义矩阵

    // 默认单位矩阵（不转换）
    return glm::mat3(1.0f);
}