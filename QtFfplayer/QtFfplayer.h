#pragma once
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLTexture>
#include <QVector>
#include <QVector3D>
#include <QVector2D>
#include <QMatrix4x4>
#include <QOpenGLShaderProgram>
#include "CSelCodec.h"
#include<thread>
#include<condition_variable>
#include<string>
#include<mutex>
#include "ffplay.h"
extern "C"
{
#include "libavutil/pixfmt.h"
}
#include<thread>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWidget>
#include <QSurfaceFormat>
#include<atomic>
#include<mutex>
#include"QProgressSlider.h"
#include<QButtonGroup>
#include "QtPlayList.h"
#include <QDragEnterEvent>
#include <QDropEvent>
#include<QFileInfoList>
#include<QStringList>
#include<QGridLayout>
#include<memory>

struct AVFrame;
class QLabel;
class QButtonGroup;
class QPushButton;
class QSlider;
class QMenuBar;
class QDoubleSpinBox;
class QtClockAction;
class QLineEdit;
class QHBoxLayout;

class QtFfplayer :public QOpenGLWidget ,protected QOpenGLFunctions
{
	Q_OBJECT
public:
	QtFfplayer(QWidget*pParentWidget = nullptr);
	~QtFfplayer();
	void Render(ffplay::Frame *f1);
	AVPixelFormat getNeededPixeFormat();
	void start(const std::pair<QString, int>&pathFile);
	void stop();
	QtPlayList::PLAY_MODE getPlayMode();
	void setUnderPlayMode(bool b) 
	{
		m_bUnderPlayMode = b;
	}
	bool getUnderPlayMode() const
	{
		return m_bUnderPlayMode;
	}
	static QStringList getFilesWhenHaveDirs(QStringList&&files);
	static QFileInfoList GetFileList(const QString &path);
#ifdef DELAY_ANALYSIS
	template<typename ConType>QHBoxLayout* makeLay(const QString&strLabel, QLabel**label, ConType**edit,const QString&strLabelUnit, QLabel**labelUnit, QWidget*parentWid);
	QGridLayout* makeGridLay(QWidget* parentWid);
//template <class T>
//typename std::enable_if<std::is_same<T, QLineEdit>::value, QLineEdit>::type
//is_edit(T* i);
#endif
protected:
	enum ACT_TYPE {ACT_VOICE_SHOW_HIDE, ACT_BEGIN_SUSPEND, ACT_STOP, ACT_BACK, ACT_AHEAD, ACT_OPEN, ACT_SPEED};

signals:
	//void signalsGetFrame();
	void signalsFrameSizeChanged();
	void signalsVideoLength(long long);
	void signalsCurTime(long long);
	void signalsEndOfFile();
	void signalsVideoBegin();
	void signalsVideoEnd();
	//void signalsJoinThreadEnd();
#ifdef DELAY_ANALYSIS
	void signalsDelay(double);
	void signalsUndulate(double);
	void signalsAudioData(double);
	void signalsVideoData(double);
	void signalsAudioFrameData(double);
	void signalsVideoFrameData(double);
	void signalsBuffer(double);
	void signalsSDLFeedBuffer(double);
	void signalsSoundTouchTranslateBuffer(double);
	void signalsSoundTouchUnprocessedBuffer(double);
	void signalsTotalDelay(double);
	void signalsSyncType(int);
#endif
public slots:
	//void slotGetFrame();
	void slotBtnOpenClick();
	void slotVideoLength(long long i);
	void slotCurTime(long long i);
	void slotVoiceBtnClicked(bool checked);
	void slotPlayBtnClicked(bool checked);
	void slotStopBtnClicked();
	void slotBackBtnClick();
	void slotAheadBtnClick();
	void slotListBtnClick();
	void slotPlayReachCurEnd();
	void slotVideoBegin();
	void slotVideoEnd();
	void slotSpinBoxValueChanged(double d);
	void slotScreenBtnClick(bool checked);
	void slotSyncModelChanged(int state);
	//void slotOtherBtnClick();
#ifdef DELAY_ANALYSIS
	void slotDelay(double t);
	void slotUndulate(double t);
	void slotAudioData(double t);
	void slotVideoData(double t);
	void slotAudioFrameData(double t);
	void slotVideoFrameData(double t);
	void slotBuffer(double t);
	void slotSDLFeedBuffer(double t);
	void slotSoundTouchTranslateBuffer(double t);
	void slotSoundTouchUnprocessedBuffer(double t);
	void slotTotalDelay(double t);
	void slotSyncType(int type);
#endif


protected:
	void initUI(QWidget*parentWidget);
	QRect calculate_display_rect(ffplay::Frame*f);
	QRect calculate_display_rect(
		int scr_xleft, int scr_ytop, int scr_width, int scr_height,
		int pic_width, int pic_height, AVRational pic_sar);
	void deleteTex();
	void initConnect();
	struct SwsContext* m_SwsContext = nullptr;
	QVector<QVector3D> vertices;
	QVector<QVector2D> texCoords;
	QOpenGLShaderProgram program;
	QOpenGLTexture* texture = nullptr;
	QMatrix4x4 projection;
	const AVPixelFormat m_pixNeeded = AV_PIX_FMT_YUV420P;

protected:
	//void thdFunVideo();
	//void thdFunAudio();
	void mouseMoveEvent(QMouseEvent* event)override;
	void mousePressEvent(QMouseEvent* event)override;
	void mouseReleaseEvent(QMouseEvent* event)override;
	void leaveEvent(QEvent* event)override;
	void keyPressEvent(QKeyEvent* ev)override;
	void keyReleaseEvent(QKeyEvent* ev)override;
	void dragEnterEvent(QDragEnterEvent* event)override;
	void dropEvent(QDropEvent* event)override;
	void setBtnPlayingState(bool b);
	void setBtnScreenState(bool b);
protected:
	void slotOpen();
	void slotOpenUrl();
	void appendToListAndPlayFirstFile(QStringList&&strList);
	void contextMenuEvent(QContextMenuEvent* event)override;
	void setMenuTransparent(QMenu*menu);
protected:
	bool m_bMousePressed = false;
	QPoint StartPos;
	/*std::thread m_thdGathVideo;
	std::thread m_thdGathAudio;
	std::thread m_thdEnc;

	std::condition_variable m_conVarVideoThd;
	std::condition_variable m_conVarAudioThd;
	std::mutex m_muxVideo;
	bool m_bVideoCodecReady;

	std::mutex m_muxAudio;
	bool m_bAudioCodecReady;
	std::once_flag m_onceFlag;

	COpenStream m_video;
	COpenStream m_audio;
	CEncode m_enc;*/
	
	std::pair<QString,int> m_strSaveFile;
	ffplay m_play;
	ffplay::Frame m_frameSave;
	AVFrame* m_frame_tmp;
	decltype(m_frameSave) m_frameCopy;

	bool m_bUploadTex = false;

	int m_iPreFrameXOffset = 0;
	int m_iPreFrameYOffset = 0;
	int m_iPreFrameWidth = 0;
	int m_iPreFrameHeight = 0;
	int m_iPreFrameWinWidth = 0;
	int m_iPreFrameWinHeight = 0;
	AVRational m_preFrameRatio;
	int64_t m_i64VideoLength;

	std::mutex m_mutex;
	//int m_recordFrameW = 0;
	//int m_recordFrameH = 0;
	//int m_castFrameW = 0;
	//int m_caseFrameH = 0;
	AVPixelFormat m_frameFormat = AV_PIX_FMT_NONE;
	std::unique_ptr<std::thread, std::function<void(std::thread*&)>> m_thread;
	char m_szCmdLine[2][512];
	char* m_pp[2];


	//////////////////////////////////
	////////////////////////////////
	//QSize minimumSizeHint() const override;
	//QSize sizeHint() const override;

	void setFrameSize(int width,  int height);

	void setYPixels(uint8_t* pixels, int stride);
	void setUPixels(uint8_t* pixels, int stride);
	void setVPixels(uint8_t* pixels, int stride);

protected:
	void initializeGL() override;
	void paintGL() override;
	void resizeGL(int width, int height) override;
	//QRect m_drawRect;
private:
	enum YUVTextureType {
		YTexture,
		UTexture,
		VTexture
	};

	void initializeTextures();
	void bindPixelTexture(GLuint texture, YUVTextureType textureType, uint8_t* pixels, int stride);

	QOpenGLShaderProgram m_program;
	QOpenGLVertexArrayObject m_vao;

	std::atomic_int m_frameWidth = 1920;
	std::atomic_int m_frameHeight = 1080;

	GLuint y_tex{ 0 };
	GLuint u_tex{ 0 };
	GLuint v_tex{ 0 };
	GLint u_pos{ 0 };
	std::atomic_bool m_isOpenGLInit = false;
protected:

	std::function<QString(const QString&, const QString&, const QString&, const QString&)> makeStyleSheet = [](const QString&s1, const QString&s2, const QString&s3, const QString&s4)->QString
	{
		return QString("QPushButton{border-image:url(%1);}"
			"QPushButton:hover{border-image:url(%2);}"
			"QPushButton:pressed{border-image:url(%3);}"
			"QPushButton:checked{border-image:url(%4);}").arg(s1).arg(s2).arg(s3).arg(s4);
	};

	std::function<QString(const QString&, const QString&, const QString&)> makeStyleSheetUncheckable = [](const QString& s1, const QString& s2, const QString& s3)->QString
	{
		return QString("QPushButton{border-image:url(%1);}"
			"QPushButton:hover{border-image:url(%2);}"
			"QPushButton:pressed{border-image:url(%3);}").arg(s1).arg(s2).arg(s3);
	};


	int m_iVoiceValueSave = 0;
	QPushButton* m_btnVoice = nullptr;
	QProgressSlider* m_sliderProgress = nullptr;

	QLabel* m_labelPlayTime = nullptr;
	QSlider* m_sliderVoice = nullptr;
	QMenuBar* m_menuBar = nullptr;
	QDoubleSpinBox* m_spinPlaySpeed = nullptr;

	QLabel* m_labelMediaTime = nullptr;
	QButtonGroup* m_btnGrp = nullptr;

	QPushButton* m_btnPlay = nullptr;
	QPushButton* m_btnStop = nullptr;
	QPushButton* m_btnBack = nullptr;
	QPushButton* m_btnAhead = nullptr;
	QPushButton* m_btnOpen = nullptr;
	QPushButton* m_btnList = nullptr;
	QFrame* m_widControl = nullptr;
	QtPlayList* m_listPlayList = nullptr;
	std::atomic_bool m_bUnderPlayMode = true;


	QPushButton* m_btnScreen = nullptr;


	QButtonGroup* m_btnGroupPlayMode = nullptr;
	QPushButton* m_btnPlayModeSingle = nullptr;
	QPushButton* m_btnPlayModeSingleCircle = nullptr;
	QPushButton* m_btnPlayModeList = nullptr;
	QPushButton* m_btnPlayModeListCircle = nullptr;
	QPushButton* m_btnPlayModeRander = nullptr;
	QPushButton* m_btnOther = nullptr;

	std::vector<std::tuple<QPushButton* QtFfplayer::*, QString, QString, QString, QString, QtPlayList::PLAY_MODE>> PlayMode =
	{
		{&QtFfplayer::m_btnPlayModeSingle, ":/icon/res/single.png",":/icon/res/singleHover.png",":/icon/res/singlePress.png",":/icon/res/singleCheck.png", QtPlayList::PLAY_MODE::PLAY_MODEL_SINGLE}
		,{&QtFfplayer::m_btnPlayModeSingleCircle, ":/icon/res/singleCircle.png",":/icon/res/singleCircleHover.png",":/icon/res/singleCirclePress.png",":/icon/res/singleCircleCheck.png",QtPlayList::PLAY_MODE::PLAY_MODEL_SINGLE_CIRCLE}
		,{&QtFfplayer::m_btnPlayModeList, ":/icon/res/listOrder.png",":/icon/res/listOrderHover.png",":/icon/res/listOrderPress.png",":/icon/res/listOrderCheck.png",QtPlayList::PLAY_MODE::PLAY_MODEL_LIST_ORDER}
		,{&QtFfplayer::m_btnPlayModeListCircle, ":/icon/res/listCircle.png",":/icon/res/listCircleHover.png",":/icon/res/listCirclePress.png",":/icon/res/listCircleCheck.png",QtPlayList::PLAY_MODE::PLAY_MODEL_LIST_CIRCEL}
		,{&QtFfplayer::m_btnPlayModeRander, ":/icon/res/rand.png",":/icon/res/randHover.png",":/icon/res/randPress.png",":/icon/res/randCheck.png",QtPlayList::PLAY_MODE::PLAY_MODEL_LIST_RAND}
	};

	std::vector<std::tuple<QString, QString, QString>> m_vStateIconBtnPlay =
	{
		{":/icon/res/play.png",":/icon/res/playHover.png",":/icon/res/playPress.png"}
		,{":/icon/res/suspend.png",":/icon/res/suspendHover.png",":/icon/res/suspendPress.png"}
	};

	std::vector<std::tuple<QString, QAction*, void (QAction::*)(bool), void(QtFfplayer::*)()>>m_initMenuInfo =
	{
		{tr("open"), nullptr, &QAction::triggered, &QtFfplayer::slotOpen}
		,{tr("open url"), nullptr, &QAction::triggered, &QtFfplayer::slotOpenUrl}
	};

	QMenu* m_contextMenu;

	std::vector<std::tuple<QString, QString, QString>> m_vStateIconBtnFullScreen =
	{
		{":/icon/res/fullSrc.png",":/icon/res/fullSrcHover.png",":/icon/res/fullSrcPress.png"}
		,{":/icon/res/exitFulSrc.png",":/icon/res/exitFulSrcHover.png",":/icon/res/exitFulSrcPress.png"}
	};

	std::vector<std::tuple<QString, QString, QString>> m_vStateIconBtnOther =
	{
		{":/icon/res/other.png",":/icon/res/otherHover.png",":/icon/res/otherPress.png"}
	};

	QMenu* m_otherMenu = nullptr;
	QtClockAction* m_actionClock = nullptr;
#ifdef	DELAY_ANALYSIS
	void calcTotalDelay();
	QWidget* m_widRealTimeInfo = nullptr;
	QLabel* m_labelDelay = nullptr;
	QLineEdit* m_editDelay = nullptr;
	QLabel* m_labelDelayUnit = nullptr;

	QLabel* m_labelUndulate = nullptr;
	QLineEdit* m_editUndulate = nullptr;
	QLabel* m_labelUndulateUnit = nullptr;

	QLabel* m_labelAudioData = nullptr;
	QLineEdit* m_editAudioData = nullptr;
	QLabel* m_labelAudioDataUnit = nullptr;

	QLabel* m_labelVideoData = nullptr;
	QLineEdit* m_editVideoData = nullptr;
	QLabel* m_labelVideoDataUnit = nullptr;

	QLabel* m_labelAudioFrameData = nullptr;
	QLineEdit* m_editAudioFrameData = nullptr;
	QLabel* m_labelAudioFrameDataUnit = nullptr;

	QLabel* m_labelVideoFrameData = nullptr;
	QLineEdit* m_editVideoFrameData = nullptr;
	QLabel* m_labelVideoFrameDataUnit = nullptr;

	QLabel* m_labelBuffer = nullptr;
	QLineEdit* m_editBuffer = nullptr;
	QLabel* m_labelBufferUnit = nullptr;

	QLabel* m_labelSDLFeedBuffer = nullptr;
	QLineEdit* m_editSDLFeedBuffer = nullptr;
	QLabel* m_labelSDLFeedBufferUnit = nullptr;

	QLabel* m_labelSoundTouchTranslateBuffer = nullptr;
	QLineEdit* m_editSoundTouchTranslateBuffer = nullptr;
	QLabel* m_labelSoundTouchTranslateBufferUnit = nullptr;

	QLabel* m_labelSoundTouchUnprocessedBuffer = nullptr;
	QLineEdit* m_editSoundTouchUnprocessedBuffer = nullptr;
	QLabel* m_labelSoundTouchUnprocessedBufferUnit = nullptr;

	QLabel* m_labelTotalDelay = nullptr;
	QLineEdit* m_editTotalDelay = nullptr;
	QLabel* m_labelTotalDelayUnit = nullptr;

	QLabel* m_labelSpin = nullptr;
	QDoubleSpinBox* m_spinEnableSpeed = nullptr;
	bool m_bIsInAccelSpeed = false;
	std::vector<std::tuple<QString, QLabel* QtFfplayer::*, QLineEdit* QtFfplayer::*, QString, QLabel* QtFfplayer::*, void(QtFfplayer::*)(double), void(QtFfplayer::*)(double),int>>m_vDelayAnalysis =
	{
		std::make_tuple(QString::fromWCharArray(L"延迟上限:"), &QtFfplayer::m_labelDelay, &QtFfplayer::m_editDelay, QString::fromWCharArray(L"ms"),&QtFfplayer::m_labelDelayUnit, &QtFfplayer::signalsDelay, &QtFfplayer::slotDelay,-1)
		,std::make_tuple(QString::fromWCharArray(L"延迟范围:"), &QtFfplayer::m_labelUndulate, &QtFfplayer::m_editUndulate, QString::fromWCharArray(L"ms"),&QtFfplayer::m_labelUndulateUnit, &QtFfplayer::signalsUndulate, &QtFfplayer::slotUndulate,-1)
		,std::make_tuple(QString::fromWCharArray(L"音频pkt队列时长:"), &QtFfplayer::m_labelAudioData, &QtFfplayer::m_editAudioData, QString::fromWCharArray(L"ms"),&QtFfplayer::m_labelAudioDataUnit, &QtFfplayer::signalsAudioData, &QtFfplayer::slotAudioData, ffplay::AV_SYNC_AUDIO_MASTER)
		,std::make_tuple(QString::fromWCharArray(L"视频pkt队列时长:"), &QtFfplayer::m_labelVideoData, &QtFfplayer::m_editVideoData, QString::fromWCharArray(L"ms"),&QtFfplayer::m_labelVideoDataUnit, &QtFfplayer::signalsVideoData, &QtFfplayer::slotVideoData,ffplay::AV_SYNC_VIDEO_MASTER)
		,std::make_tuple(QString::fromWCharArray(L"音频frame队列时长:"), &QtFfplayer::m_labelAudioFrameData, &QtFfplayer::m_editAudioFrameData, QString::fromWCharArray(L"ms"),&QtFfplayer::m_labelAudioFrameDataUnit, &QtFfplayer::signalsAudioFrameData, &QtFfplayer::slotAudioFrameData,ffplay::AV_SYNC_AUDIO_MASTER)
		,std::make_tuple(QString::fromWCharArray(L"视频frame队列时长:"), &QtFfplayer::m_labelVideoFrameData,&QtFfplayer::m_editVideoFrameData, QString::fromWCharArray(L"ms"),&QtFfplayer::m_labelVideoFrameDataUnit, &QtFfplayer::signalsVideoFrameData, &QtFfplayer::slotVideoFrameData,ffplay::AV_SYNC_VIDEO_MASTER)
		,std::make_tuple(QString::fromWCharArray(L"音频samples未处理时长:"), &QtFfplayer::m_labelBuffer, &QtFfplayer::m_editBuffer, QString::fromWCharArray(L"ms"),&QtFfplayer::m_labelBufferUnit, &QtFfplayer::signalsBuffer, &QtFfplayer::slotBuffer,1)
		,std::make_tuple(QString::fromWCharArray(L"SDL feed缓冲区时长:"), &QtFfplayer::m_labelSDLFeedBuffer, &QtFfplayer::m_editSDLFeedBuffer, QString::fromWCharArray(L"ms"),&QtFfplayer::m_labelSDLFeedBufferUnit, &QtFfplayer::signalsSDLFeedBuffer, &QtFfplayer::slotSDLFeedBuffer,1)
		,std::make_tuple(QString::fromWCharArray(L"SoundTouch未读时长:"), &QtFfplayer::m_labelSoundTouchTranslateBuffer, &QtFfplayer::m_editSoundTouchTranslateBuffer, QString::fromWCharArray(L"ms"),&QtFfplayer::m_labelSoundTouchTranslateBufferUnit, &QtFfplayer::signalsSoundTouchTranslateBuffer, &QtFfplayer::slotSoundTouchTranslateBuffer,ffplay::AV_SYNC_AUDIO_MASTER)
		,std::make_tuple(QString::fromWCharArray(L"SoundTouch未处理时长:"), &QtFfplayer::m_labelSoundTouchUnprocessedBuffer, &QtFfplayer::m_editSoundTouchUnprocessedBuffer, QString::fromWCharArray(L"ms"),&QtFfplayer::m_labelSoundTouchUnprocessedBufferUnit, &QtFfplayer::signalsSoundTouchUnprocessedBuffer, &QtFfplayer::slotSoundTouchUnprocessedBuffer,ffplay::AV_SYNC_AUDIO_MASTER)
		,std::make_tuple(QString::fromWCharArray(L"总时长:"), &QtFfplayer::m_labelTotalDelay, &QtFfplayer::m_editTotalDelay, QString::fromWCharArray(L"ms"),&QtFfplayer::m_labelTotalDelayUnit, &QtFfplayer::signalsTotalDelay, &QtFfplayer::slotTotalDelay,-1)
	};
	
	int m_iSyncType = -2;
#endif
};