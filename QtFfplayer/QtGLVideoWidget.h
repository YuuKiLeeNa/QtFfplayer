#pragma once
#include<QOpenGLFunctions>
#include<QOpenGLWidget>
#include<thread>
#include <QOpenGLVertexArrayObject>
#include <QSurfaceFormat>
#include<atomic>
#include<mutex>
#ifdef __cplusplus
extern "C"
{
#endif 
#include "libavutil/pixfmt.h"
#include "libavutil/rational.h"
#ifdef __cplusplus
}
#endif 
#include<QOpenGLShaderProgram>
#include<QOpenGLTexture>
#include<glm/glm.hpp>
#include "HDRMetadataParser.h"

struct AVFrame;
class QtGLVideoWidget:public QOpenGLWidget,protected QOpenGLFunctions
{
protected:
	// Uniform locations缓存
	struct UniformLocations {
		GLint tex_y, tex_u, tex_v, tex_a;
		GLint pixel_format, /*color_space,*/ color_range;
		GLint eotf_type, swap_uv;
		GLint display_peak, system_gamma, hdr_metadata;
		GLint yuv_to_rgb_matrix, yuv_offset, y_scale, uv_scale;
		GLint /*chroma_scale,*/ chroma_offset, ten_bit_scale;
		//GLint bit_depth_params;
	} uniforms;

	// 缓存的值，避免重复上传
	struct UniformCache {
		int last_pixel_format = -1;
		int last_color_space = -1;
		int last_color_range = -1;
		int last_eotf_type = -1;
		int last_swap_uv = -1;
		float last_display_peak = -1.0f;
		float last_system_gamma = -1.0f;
		float last_y_scale = -1.0f;
		float last_uv_scale = -1.0f;
		float last_ten_bit_scale = -1.0f;
		glm::vec2 last_chroma_offset;
		glm::mat3 last_matrix;
		glm::vec3 last_offset;
		glm::vec3 last_bit_params;
		
	} cache;

	void resetUniformCache() 
	{
		cache.last_pixel_format = -1;
		cache.last_color_space = -1;
		cache.last_color_range = -1;
		cache.last_eotf_type = -1;
		cache.last_swap_uv = -1;
		cache.last_display_peak = -1.0f;
		cache.last_system_gamma = -1.0f;
		cache.last_y_scale = -1.0f;
		cache.last_uv_scale = -1.0f;
		cache.last_ten_bit_scale = -1.0f;
		cache.last_chroma_offset = decltype(cache.last_chroma_offset){};
		cache.last_matrix = decltype(cache.last_matrix){};
		cache.last_offset = decltype(cache.last_offset){};
		cache.last_bit_params = decltype(cache.last_bit_params){};
	}

public:
	QtGLVideoWidget(QWidget*pParentWidget = nullptr);
	virtual ~QtGLVideoWidget();
	void renderFrame(AVFrame* f);
	bool isSupportAVPixelFormat(AVPixelFormat fmt);
	static std::vector<AVPixelFormat>getSupportAVPixelFormat();
protected:
	QRect calculate_display_rect(AVFrame* f);
	QRect calculate_display_rect(
		int scr_xleft, int scr_ytop, int scr_width, int scr_height,
		int pic_width, int pic_height, AVRational pic_sar);
	void deleteTex();
	void initConnect();
	QOpenGLShaderProgram program;
	//QOpenGLTexture* texture = nullptr;
	//QMatrix4x4 projection;
	//const AVPixelFormat m_pixNeeded = AV_PIX_FMT_YUV420P;

protected:
	//bool m_bMousePressed = false;
	//QPoint StartPosStartPos;
	
	//std::pair<QString, int> m_strSaveFile;
	AVFrame* m_frameSave;
	AVFrame* m_frame_tmp;

	bool m_bUploadTex = false;

	int m_iPreFrameXOffset = 0;
	int m_iPreFrameYOffset = 0;
	std::atomic_int m_iPreFrameWidth = 1920;
	std::atomic_int m_iPreFrameHeight = 1080;
	int m_iPreFrameWinWidth = 0;
	int m_iPreFrameWinHeight = 0;
	AVRational m_preFrameRatio;
	int64_t m_i64VideoLength;
	std::mutex m_mutex;
	AVPixelFormat m_frameFormat = AV_PIX_FMT_NONE;
	void setFrameSize(int width, int height);


protected:
	void initializeGL() override;
	//GLuint compileShaderDirectly(GLenum type, const char* source);
	void paintGL() override;
	void resizeGL(int width, int height) override;
	QRect m_drawRect;

protected:
    // 初始化时获取所有uniform位置
    void initUniformLocations();

    // 智能设置uniform，只更新变化的值
    void smartSetUniforms(AVFrame* frame, float display_peak_nits = 1000.0f);
	void updateConversionMatrix(AVFrame* frame, int pixel_format, bool is_10bit);
protected:

    void updateChromaPosition(AVFrame* frame);
    glm::mat3 calculateYUVtoRGBMatrix(AVColorSpace cs, bool is_full_range, bool is_10bit);
	glm::mat3 fullRangeMatrix(AVColorSpace cs);
	glm::vec3 calculateYUVOffset(AVColorRange range, bool is_10bit);


	int mapPixelFormat(AVPixelFormat fmt);

	bool is10BitFormat(AVPixelFormat fmt);

	int mapColorSpace(AVColorSpace cs);

	AVColorSpace guessColorSpace(AVFrame* frame);

	int getEOTFType(AVColorTransferCharacteristic transfer);
protected:
	void initializeTextures(AVPixelFormat pixfmt, int w, int h);

	void bindPixelTexture(GLuint texture, int texW, int texH, GLenum format, GLenum type, uint8_t* pixels, int stride);

	QOpenGLShaderProgram m_program;
	QOpenGLVertexArrayObject m_vao;
	GLuint yuva_tex[4]{0};
	GLint u_pos{ 0 };
	std::atomic_bool m_isOpenGLInit = false;
protected:
	// 在你的渲染器中使用
	void setup_hdr_uniforms(AVFrame* frame,float display_peak_nits);

	// 辅助函数：设置原色矩阵
	//void setup_color_primaries_matrix(const HDRMetadataParser::HDRInfo& hdr_info);

	// 颜色转换矩阵计算（简化）
	glm::mat3 calculate_color_conversion_matrix(const float (&src_red)[2], const float(&src_green)[2],
		const float (& src_blue)[2], const float(& src_white)[2]);

};

