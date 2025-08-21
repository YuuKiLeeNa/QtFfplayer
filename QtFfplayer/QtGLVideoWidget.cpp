#include "QtGLVideoWidget.h"
#include <QOpenGLShader>
#include<memory>
#include<algorithm>
#include<numeric>
extern "C" {
#include"libavdevice/avdevice.h"
#include "libavutil/frame.h"
#include "libswscale/swscale.h"
}

#define FRAG_SHADER R"(#version 150 core
uniform sampler2D y_tex;
uniform sampler2D u_tex;
uniform sampler2D v_tex;
in vec2 v_coord;
//layout( location = 0 ) out vec4 fragcolor;
out vec4 fragcolor;

const vec3 R_cf = vec3(1.164383,  0.000000,  1.596027);
const vec3 G_cf = vec3(1.164383, -0.391762, -0.812968);
const vec3 B_cf = vec3(1.164383,  2.017232,  0.000000);
const vec3 offset = vec3(-0.0625, -0.5, -0.5);

void main() {
  float y = texture(y_tex, v_coord).r;
  float u = texture(u_tex, v_coord).r;
  float v = texture(v_tex, v_coord).r;
  vec3 yuv = vec3(y,u,v);
  yuv += offset;
  fragcolor = vec4(0.0, 0.0, 0.0, 1.0);
  fragcolor.r = dot(yuv, R_cf);
  fragcolor.g = dot(yuv, G_cf);
  fragcolor.b = dot(yuv, B_cf);
})"


#define VERT_SHADER R"(#version 150 core
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
	QSurfaceFormat defaultFormat = QSurfaceFormat::defaultFormat();
	defaultFormat.setProfile(QSurfaceFormat::CoreProfile);
	defaultFormat.setVersion(3, 3); // Adapt to your system
	QSurfaceFormat::setDefaultFormat(defaultFormat);
	setFormat(defaultFormat);
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
		av_frame_unref(m_frameSave);
		av_frame_free(&m_frameSave);
	}
	if (m_frame_tmp) 
	{
		av_frame_unref(m_frame_tmp);
		av_frame_free(&m_frame_tmp);
	}
}

void QtGLVideoWidget::renderFrame(AVFrame* f)
{
	if (f == nullptr || f->format != m_pixNeeded)
	{
		if (!m_isOpenGLInit) 
		{
			std::lock_guard<std::mutex>lock(m_mutex);
			if (m_frameSave)
			{
				av_frame_unref(m_frameSave);
				av_frame_free(&m_frameSave);
			}
			m_frameSave = nullptr;
		}
		if (f) 
		{
			av_frame_unref(f);
			av_frame_free(&f);
		}
		return;
	}
	{
		std::lock_guard<std::mutex>lock(m_mutex);
		if (m_frameSave)
		{
			av_frame_unref(m_frameSave);
			av_frame_free(&m_frameSave);
		}
		m_frameSave = f;
		m_bUploadTex = true;
	}
	update();
	//emit signalsGetFrame();
}

AVPixelFormat QtGLVideoWidget::getNeededPixeFormat()
{
	return m_pixNeeded;
}

//void QtGLVideoWidget::slotGetFrame()
//{
//	std::lock_guard<std::mutex>lock(m_mutex);
//	if (m_frameSave)
//	{
//		calculate_display_rect(m_frameSave);
//		if (m_isOpenGLInit)
//		{
//			setYPixels(m_frameSave->data[0], m_frameSave->linesize[0]);
//			setUPixels(m_frameSave->data[1], m_frameSave->linesize[1]);
//			setVPixels(m_frameSave->data[2], m_frameSave->linesize[2]);
//		}
//		update();
//	}
//}

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
	//makeCurrent();
	if (y_tex) glDeleteTextures(1, &y_tex);
	if (u_tex) glDeleteTextures(1, &u_tex);
	if (v_tex) glDeleteTextures(1, &v_tex);
	//doneCurrent();
}

void QtGLVideoWidget::initConnect()
{
	//QObject::connect(this, SIGNAL(signalsGetFrame()), this, SLOT(slotGetFrame()));
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

	m_program.addShaderFromSourceCode(QOpenGLShader::Vertex, VERT_SHADER);
	m_program.addShaderFromSourceCode(QOpenGLShader::Fragment, FRAG_SHADER);

	m_program.link();
	m_program.setUniformValue("y_tex", 0);
	m_program.setUniformValue("u_tex", 1);
	m_program.setUniformValue("v_tex", 2);
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
		initializeTextures();
		m_isOpenGLInit = true;
	}
	
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	m_program.bind();
	m_vao.bind();
	m_program.setUniformValue("y_tex", 0);
	m_program.setUniformValue("u_tex", 1);
	m_program.setUniformValue("v_tex", 2);

	QMatrix4x4 m;
	m.ortho(0, width(), height(), 0.0, 0.0, 100.0f);
	m_program.setUniformValue("u_pm", m);
	glUniform4f(u_pos, rect.left(), rect.top(), rect.width(), rect.height());

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, y_tex);
	if (bUploadTex)
		setYPixels(m_frame_tmp->data[0], m_frame_tmp->linesize[0]);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, u_tex);
	if (bUploadTex)
		setUPixels(m_frame_tmp->data[1], m_frame_tmp->linesize[1]);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, v_tex);
	if (bUploadTex)
		setVPixels(m_frame_tmp->data[2], m_frame_tmp->linesize[2]);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	m_program.release();
	m_vao.release();


}

void QtGLVideoWidget::resizeGL(int width, int height)
{
	if(m_frameSave)
		calculate_display_rect(0, 0, width, height, m_frameSave->width, m_frameSave->height, m_frameSave->sample_aspect_ratio);
}

void QtGLVideoWidget::setYPixels(uint8_t* pixels, int stride)
{
	bindPixelTexture(y_tex, YTexture, pixels, stride);
}

void QtGLVideoWidget::setUPixels(uint8_t* pixels, int stride)
{
	bindPixelTexture(u_tex, UTexture, pixels, stride);
}

void QtGLVideoWidget::setVPixels(uint8_t* pixels, int stride)
{
	bindPixelTexture(v_tex, VTexture, pixels, stride);
}

void QtGLVideoWidget::initializeTextures()
{
	if (y_tex) glDeleteTextures(1, &y_tex);
	if (u_tex) glDeleteTextures(1, &u_tex);
	if (v_tex) glDeleteTextures(1, &v_tex);

	glGenTextures(1, &y_tex);
	glBindTexture(GL_TEXTURE_2D, y_tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, m_iPreFrameWidth, m_iPreFrameHeight, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);

	glGenTextures(1, &u_tex);
	glBindTexture(GL_TEXTURE_2D, u_tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, m_iPreFrameWidth / 2, m_iPreFrameHeight / 2, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);

	glGenTextures(1, &v_tex);
	glBindTexture(GL_TEXTURE_2D, v_tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, m_iPreFrameWidth / 2, m_iPreFrameHeight / 2, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
}

void QtGLVideoWidget::bindPixelTexture(GLuint texture, QtGLVideoWidget::YUVTextureType textureType, uint8_t* pixels, int stride)
{
	if (!pixels)
		return;

	int frameW = m_iPreFrameWidth;
	int frameH = m_iPreFrameHeight;

	int const width = textureType == YTexture ? frameW : frameW / 2;
	int const height = textureType == YTexture ? frameH : frameH / 2;


	glBindTexture(GL_TEXTURE_2D, texture);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, stride);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RED, GL_UNSIGNED_BYTE, pixels);

}

