#include "QtFfplayer.h"
#include <QOpenGLShader>
#include<memory>
#include<algorithm>
#include<numeric>
extern "C"{
#include"libavdevice/avdevice.h"
#include "libavutil/frame.h"
#include "libswscale/swscale.h"
}
#include<QDebug>
#include "ffplay.h"
#include<QApplication>
#include<QLabel>
#include<qbuttongroup.h>
#include<QSlider>
#include<qmenubar.h>
#include<qspinbox.h>
#include<QPushButton>
#include<qboxlayout.h>
#include<QFileDialog>
#include<QMouseEvent>
#include"QtPlayList.h"
#include<QMessageBox>
#include<QMimeData>
#include "QtPlayListModel.h"
#include "QtOpenUrlDlg.h"
#include"QtClockAction.h"
#include<QLineEdit>
#include"CConfig.h"
#include<Windows.h>
#include "define_type.h"

#ifdef DELAY_ANALYSIS
int CONTROL_WID_HEIGHT = 110;
const int CONTROL_TEST_WID_HEIGHT = 60;
#else
#define CONTROL_WID_HEIGHT 50
#endif

#define LIST_WID_WIDTH 200


QtFfplayer::QtFfplayer(QWidget* pParentWidget) :QOpenGLWidget(pParentWidget)
{
	resize(1280, 720);
	//m_vhdFmt = CSelCodec::getHWPixelFormat();
	QSurfaceFormat defaultFormat = QSurfaceFormat::defaultFormat();
	defaultFormat.setProfile(QSurfaceFormat::CoreProfile);
	defaultFormat.setVersion(3, 3); // Adapt to your system
	QSurfaceFormat::setDefaultFormat(defaultFormat);
	setFormat(defaultFormat);
	initUI(this);
	m_frameSave.frame = nullptr;
	initConnect();
	m_widControl->hide();//setVisible(false);
	setMouseTracking(true);
	setAcceptDrops(true);
	//QObject::connect(this, SIGNAL(signalsFrameSizeChanged()), this, SLOT(slotFrameSizeChanged()));

	memset(&m_frameCopy, 0, sizeof(m_frameCopy));
	m_frame_tmp = av_frame_alloc();
}

QtFfplayer::~QtFfplayer()
{
	QObject::disconnect(this, SIGNAL(signalsVideoBegin()), this, SLOT(slotVideoBegin()));
	QObject::disconnect(this, SIGNAL(signalsVideoEnd()), this, SLOT(slotVideoEnd()));
	stop();
	deleteTex();
	if (m_frameSave.frame) 
	{
		av_frame_unref(m_frameSave.frame);
		av_frame_free(&m_frameSave.frame);
	}
	if (m_frame_tmp)
	{
		av_frame_unref(m_frame_tmp);
		av_frame_free(&m_frame_tmp);
	}

}

void QtFfplayer::Render(ffplay::Frame* f1)
{
	UPTR_FME rel_f(av_frame_alloc());
	AVFrame* frame = f1->frame;
	if (!frame)
		return;

	av_frame_ref(rel_f.get(), frame);

	if (rel_f->format != m_pixNeeded)
	{
		UPTR_FME f(av_frame_alloc());
		f->format = m_pixNeeded;
		f->width = rel_f->width;
		f->height = rel_f->height;
		int ret = av_frame_get_buffer(f.get(), 0);
		if (ret != 0)
		{
			char szErr[128];
			qDebug() << "av_frame_get_buffer error:" << av_make_error_string(szErr, sizeof(szErr), ret);
			return;
		}
		if ((ret = av_frame_make_writable(f.get())) != 0)
		{
			char szErr[128];
			qDebug() << "av_frame_make_writable error:" << av_make_error_string(szErr, sizeof(szErr), ret);
			return;
		}
			
		if (m_frameFormat != rel_f->format || rel_f->width != m_frameWidth || rel_f->height != m_frameHeight)
		{
			m_frameFormat = (AVPixelFormat)rel_f->format;
			if (m_SwsContext)
			{
				sws_freeContext(m_SwsContext);
				m_SwsContext = nullptr;
			}
			if (rel_f->width != m_frameWidth || rel_f->height != m_frameHeight)
				setFrameSize(rel_f->width, rel_f->height);

			m_SwsContext = sws_getContext(rel_f->width, rel_f->height, (AVPixelFormat)rel_f->format, m_frameWidth, m_frameHeight, (AVPixelFormat)f->format, SWS_BILINEAR, nullptr, nullptr, nullptr);
				
			if (!m_SwsContext)
			{
				qDebug() << "sws_getContext error\n";
				return;
			}
		}
		if ((ret = sws_scale(m_SwsContext, rel_f->data, rel_f->linesize, 0, f->height, f->data, f->linesize)) < 0)
		{
			char szErr[128];
			qDebug() << "sws_scale error:" << av_make_error_string(szErr, sizeof(szErr), ret);
			sws_freeContext(m_SwsContext);
			return;
		}
		rel_f.reset(f.release());
	}

	else if (m_frameWidth != rel_f->width || m_frameHeight != rel_f->height)
		setFrameSize(rel_f->width, rel_f->height);

	{
		std::lock_guard<std::mutex>lock(m_mutex);
		if (m_frameSave.frame) 
		{
			av_frame_unref(m_frameSave.frame);
			av_frame_free(&m_frameSave.frame);
		}
		memcpy(&m_frameSave, f1, sizeof(ffplay::Frame));
		m_frameSave.frame = rel_f.release();
		m_bUploadTex = true;
	}
	update();
	
}

AVPixelFormat QtFfplayer::getNeededPixeFormat()
{
	return m_pixNeeded;
}

void QtFfplayer::start(const std::pair<QString, int>& pathFile)
{

	stop();
	m_bUnderPlayMode = true;
	m_strSaveFile = pathFile;



	if (m_strSaveFile.first.isEmpty())
		return;


	std::function<void(ffplay::Frame*)>callBackGetAVFrame = [this](ffplay::Frame* f)
	{
		Render(f);
	};

	m_play.setCallBackGetFrame(callBackGetAVFrame);
	m_play.setExitThread(false);
	m_play.setGetVideoLength([this](int64_t t)
		{
			emit this->signalsVideoLength(t);
		});
	m_play.setGetCurTime([this](int64_t t)
		{
			emit this->signalsCurTime(t);
		});
	m_play.setNoticeBeginAndEnd([this]() {emit this->signalsVideoBegin(); }, [this]() {emit this->signalsVideoEnd(); });

	m_play.setCallBackGetVolume([this]() 
		{
			return m_sliderVoice->value();
		});
	m_play.setGetSpeedCallBack([this]()
		{
			return m_spinPlaySpeed->value();
		});
#ifdef DELAY_ANALYSIS
	m_play.setDelayCallBack([this](double t) {emit this->signalsDelay(t); });
	m_play.setUndulateCallBack([this](double t) {emit this->signalsUndulate(t); });
	m_play.setAudioDataCallBack([this](double t) {emit this->signalsAudioData(t); });
	m_play.setVideoDataCallBack([this](double t) {emit this->signalsVideoData(t); });
	m_play.setAudioDataFrameCallBack([this](double t) {emit this->signalsAudioFrameData(t); });
	m_play.setVideoDataFrameCallBack([this](double t) {emit this->signalsVideoFrameData(t); });
	m_play.setBufferCallBack([this](double t) {emit this->signalsBuffer(t); });
	m_play.setSDLFeedBufferCallBack([this](double t) {emit this->signalsSDLFeedBuffer(t); });
	m_play.setSoundTouchTranslateBufferCallBack([this](double t) {emit this->signalsSoundTouchTranslateBuffer(t); });
	m_play.setSoundTouchUnprocessedBufferCallBack([this](double t) {emit this->signalsSoundTouchUnprocessedBuffer(t); });
	m_play.setTotalDelayCallBack([this](double t) {emit this->signalsTotalDelay(t); });
	m_play.setSyncTypeCallBack([this](int type) {emit this->signalsSyncType(type); });
#endif

	memset(m_szCmdLine, 0, sizeof(m_szCmdLine));

	memcpy(m_szCmdLine[1], m_strSaveFile.first.toUtf8(), /*m_strSaveFile.size() */ m_strSaveFile.first.toUtf8().length());
	m_pp[0] = m_szCmdLine[0];
	m_pp[1] = m_szCmdLine[1];
	setBtnPlayingState(true);
	//QObject::connect(this, SIGNAL(signalsGetFrame()), this, SLOT(slotGetFrame()));// , Qt::BlockingQueuedConnection);
	QObject::connect(this, SIGNAL(signalsVideoBegin()), this, SLOT(slotVideoBegin()));
	QObject::connect(this, SIGNAL(signalsVideoEnd()), this, SLOT(slotVideoEnd()));
	QObject::connect(this, SIGNAL(signalsGetFrame()), this, SLOT(slotGetFrame()));//, Qt::ConnectionType::BlockingQueuedConnection);
	
	m_thread.reset();
	{
		std::unique_lock<std::mutex>lock(m_play.m_mutexProtectedRunStart);
		m_play.m_bIsRunStart = true;
	}
	m_thread = std::unique_ptr<std::thread, std::function<void(std::thread*&)>>(new std::thread(&ffplay::start, &m_play, 2, m_pp), [this](std::thread*& t)
		{
			std::unique_lock<std::mutex>lock1(m_play.m_mutexProtectedRunStart);
			m_play.m_cond.wait(lock1, [this]()
				{
					return !m_play.m_bIsRunStart;
				});

			m_play.m_isExitThread = true;
			//m_play.isss->abort_request = 1;
			if(m_play.isss)
				m_play.isss->abort_request = 1;
			if (t->joinable())
				t->join();
			delete t;
			m_play.resetData();
#ifdef DELAY_ANALYSIS
			int isize = m_vDelayAnalysis.size();
			for (int i = 2; i < isize; ++i) 
				(this->*std::get<2>(m_vDelayAnalysis[i]))->setText("");
			m_iSyncType = -2;
#endif
		});
}
void QtFfplayer::stop()
{
	QObject::disconnect(this, SIGNAL(signalsVideoBegin()), this, SLOT(slotVideoBegin()));
	QObject::disconnect(this, SIGNAL(signalsVideoEnd()), this, SLOT(slotVideoEnd()));
	QObject::disconnect(this, SIGNAL(signalsGetFrame()), nullptr, nullptr);


	setBtnPlayingState(false);
	if (m_thread)
	{
		m_thread.reset();
		assert(!m_thread);
	}

	{
		std::lock_guard<std::mutex>lock(m_mutex);
		if (m_frameSave.frame)
		{
			av_frame_unref(m_frameSave.frame);
			av_frame_free(&m_frameSave.frame);
		}
		m_bUploadTex = true;
	}
	update();
}

QtPlayList::PLAY_MODE QtFfplayer::getPlayMode()
{
	return (QtPlayList::PLAY_MODE)m_btnGroupPlayMode->checkedId();
}

QStringList QtFfplayer::getFilesWhenHaveDirs(QStringList&& files)
{
	QStringList strDirFiles;
	QStringList strdir;
	files.erase(std::remove_if(files.begin(), files.end(), [&strdir](QString& str)
		{
			if (QFileInfo(str).isDir())
			{
				strdir << std::move(str);
				return true;
			}
			else return false;

		}), files.end());

	for (auto& ele : strdir)
	{
		QFileInfoList files = GetFileList(ele);
		std::move(files.begin(), files.end(), std::back_inserter(files));
	}
	return std::move(files);
}

QFileInfoList QtFfplayer::GetFileList(const QString& path)
{
	QDir dir(path);
	QFileInfoList file_list = dir.entryInfoList(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
	QFileInfoList folder_list = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);

	for (int i = 0; i != folder_list.size(); i++)
	{
		QString name = folder_list.at(i).absoluteFilePath();
		QFileInfoList child_file_list = GetFileList(name);
		file_list.append(child_file_list);
	}
	return file_list;
}

#ifdef DELAY_ANALYSIS

template<typename ConType>
	QHBoxLayout* QtFfplayer::makeLay(const QString& strLabel, QLabel** label, ConType** edit, const QString& strLabelUnit, QLabel** labelUnit, QWidget* parentWid)
{
	QHBoxLayout* l = new QHBoxLayout;
	(*label) = new QLabel(strLabel, parentWid);
	(*edit) = new ConType(parentWid);
	(*edit)->setValidator(new QRegExpValidator(QRegExp(R"(^[1-9]\d*\.\d*|0\.\d*[1-9]\d*$)"), this));
	l->addWidget(*label);
	l->addWidget(*edit);
	if (labelUnit)
	{
		(*labelUnit) = new QLabel(strLabelUnit, parentWid);
		l->addWidget(*labelUnit);
	}
	return l;
}

QGridLayout* QtFfplayer::makeGridLay(QWidget* parentWid) 
{
	QGridLayout* gridLay = new QGridLayout;
	gridLay->setContentsMargins(0,0,0,0);
	gridLay->setSpacing(2);
	int size = m_vDelayAnalysis.size();
	
	for (int i = 0; i < size; ++i)
	{
		auto l = makeLay(std::get<0>(m_vDelayAnalysis[i]), &(this->*std::get<1>(m_vDelayAnalysis[i])), &(this->*std::get<2>(m_vDelayAnalysis[i])),std::get<3>(m_vDelayAnalysis[i]), &(this->*std::get<4>(m_vDelayAnalysis[i])),parentWid);
		l->setContentsMargins(8,0,8,0);
		if(i > 1)
			(this->*std::get<2>(m_vDelayAnalysis[i]))->setEnabled(false);
		gridLay->addLayout(l,i%2, i/2, Qt::AlignLeft);
	}
	//auto l = makeLay(QString::fromWCharArray(L"速度"), &m_labelSpin,&m_spinEnableSpeed, "", nullptr, parentWid);
	QHBoxLayout* l = new QHBoxLayout;
	m_labelSpin = new QLabel(QString::fromWCharArray(L"速度:"), parentWid);
	m_spinEnableSpeed = new QDoubleSpinBox(parentWid);
	m_spinEnableSpeed->setRange(0,100);
	m_spinEnableSpeed->setValue(1.5);
	l->addWidget(m_labelSpin);
	l->addWidget(m_spinEnableSpeed);
	gridLay->addLayout(l, size%2, size/2, Qt::AlignLeft);
	return gridLay;
}

//template <class T>
//typename std::enable_if<std::is_same<T, QLineEdit>::value, QLineEdit>::type
//QtFfplayer::is_edit(T* i) 
//{
//	i->setValidator(new QRegExpValidator(QRegExp(R"(^[1-9]\d*\.\d*|0\.\d*[1-9]\d*$)"), this));
//	//return i;
//}

#endif


void QtFfplayer::slotBtnOpenClick()
{
	QStringList fileNames = QFileDialog::getOpenFileNames(this,
		"open",
		"E:\\",
		tr("(*.*);;"));
	if (!fileNames.isEmpty())
	{
		fileNames = getFilesWhenHaveDirs(std::move(fileNames));
		appendToListAndPlayFirstFile(std::move(fileNames));
	}
}

void QtFfplayer::slotVideoLength(long long i)
{
	m_i64VideoLength = i;
	char szMediaTime[64];
	snprintf(szMediaTime,sizeof(szMediaTime),"%02lld:%02lld:%02lld", (i/1000000/3600)%100, ((i/1000000)%3600)/60, (i / 1000000)%60);
	m_labelMediaTime->setText(szMediaTime);
	m_sliderProgress->setRange(0, m_i64VideoLength);
}

void QtFfplayer::slotCurTime(long long i)
{
	m_sliderProgress->setValue(i);
	m_listPlayList->slotProgressBarValueChange();
	if (i < 0)
		i = 0;
	char szMediaTime[64];
	snprintf(szMediaTime, sizeof(szMediaTime), "%02lld:%02lld:%02lld", (i / 1000000 / 3600) % 100, ((i / 1000000) % 3600) / 60, (i / 1000000) % 60);
	m_labelPlayTime->setText(szMediaTime);
}

void QtFfplayer::slotVoiceBtnClicked(bool checked)
{
	m_sliderVoice->setEnabled(!checked);
	if (checked) 
	{
		m_iVoiceValueSave = m_sliderVoice->value();
		m_sliderVoice->setValue(0);
	}
	else 
		m_sliderVoice->setValue(m_iVoiceValueSave);
}

void QtFfplayer::slotPlayBtnClicked(bool checked)
{
	if (m_play.isss)
		m_play.toggle_pause(m_play.isss.get());
	else
	{
		auto p = m_listPlayList->getCurPlayIndexFile();
		if (p.first.isEmpty() || p.second < -1)
			checked = false;
		else
		{
			m_listPlayList->slotRowDoubleClicked(m_listPlayList->getModel()->index(p.second, 0));
		}
	}
	setBtnPlayingState(checked);
}
void QtFfplayer::slotStopBtnClicked()
{
	stop();
	m_bUnderPlayMode = false;
}
void QtFfplayer::slotBackBtnClick()
{
	QKeyEvent leftKey(QEvent::KeyPress, Qt::Key_Left, Qt::NoModifier);
	keyPressEvent(&leftKey);
}
void QtFfplayer::slotAheadBtnClick()
{
	QKeyEvent rightKey(QEvent::KeyPress, Qt::Key_Right, Qt::NoModifier);
	keyPressEvent(&rightKey);
}

void QtFfplayer::slotListBtnClick()
{
	if (m_listPlayList->isHidden())
		m_listPlayList->show();
	else
		m_listPlayList->hide();
	calculate_display_rect(&m_frameSave);
	update();
}

void QtFfplayer::slotPlayReachCurEnd()
{
	stop();
}

void QtFfplayer::slotVideoBegin()
{
	//setBtnPlayingState(true);
}

void QtFfplayer::slotVideoEnd()
{
	QObject::disconnect(this, SIGNAL(signalsVideoEnd()), this, SLOT(slotVideoEnd()));
	QObject::disconnect(this, SIGNAL(signalsVideoBegin()), this, SLOT(slotVideoBegin()));
	setBtnPlayingState(false);
	if (!m_bUnderPlayMode)
	{
		stop();
		return;
	}
	auto p = m_listPlayList->NextPlayFileName();
	if (p.first.isEmpty() || p.second < -1)
	{
		stop();
		return;
	}
	start(p);
}

void QtFfplayer::slotSpinBoxValueChanged(double d)
{
	m_play.setAllClockSpeed(d);
}

void QtFfplayer::slotScreenBtnClick(bool checked)
{
	setBtnScreenState(checked);
	if (checked)
		showFullScreen();
	else
		showNormal();
}
void QtFfplayer::slotSyncModelChanged(int state)
{
	if (m_play.isss)
		m_play.isss->av_sync_type = state;
	m_play.av_sync_type = state;
}

#ifdef DELAY_ANALYSIS
void QtFfplayer::slotDelay(double t) 
{
	m_editDelay->setText(QString("%1").arg(t));
}
void QtFfplayer::slotUndulate(double t) 
{
	m_editUndulate->setText(QString("%1").arg(t));
}
void QtFfplayer::slotAudioData(double t) 
{
	m_editAudioData->setText(QString("%1").arg(t));
	if(m_iSyncType == ffplay::AV_SYNC_AUDIO_MASTER)
		calcTotalDelay();
}
void QtFfplayer::slotVideoData(double t) 
{
	m_editVideoData->setText(QString("%1").arg(t));
	if (m_iSyncType == ffplay::AV_SYNC_VIDEO_MASTER)
		calcTotalDelay();
}
void QtFfplayer::slotAudioFrameData(double t) 
{
	m_editAudioFrameData->setText(QString("%1").arg(t));
	if (m_iSyncType == ffplay::AV_SYNC_AUDIO_MASTER)
		calcTotalDelay();
}
void QtFfplayer::slotVideoFrameData(double t) 
{
	m_editVideoFrameData->setText(QString("%1").arg(t));
	if (m_iSyncType == ffplay::AV_SYNC_VIDEO_MASTER)
		calcTotalDelay();
}
void QtFfplayer::slotBuffer(double t) 
{
	m_editBuffer->setText(QString("%1").arg(t));
	if (m_iSyncType == ffplay::AV_SYNC_AUDIO_MASTER)
		calcTotalDelay();
}
void QtFfplayer::slotSDLFeedBuffer(double t) 
{
	m_editSDLFeedBuffer->setText(QString("%1").arg(t));
	if (m_iSyncType == ffplay::AV_SYNC_AUDIO_MASTER)
		calcTotalDelay();
}
void QtFfplayer::slotSoundTouchTranslateBuffer(double t) 
{
	m_editSoundTouchTranslateBuffer->setText(QString("%1").arg(t));
	if (m_iSyncType == ffplay::AV_SYNC_AUDIO_MASTER)
		calcTotalDelay();
}
void QtFfplayer::slotSoundTouchUnprocessedBuffer(double t) 
{
	m_editSoundTouchUnprocessedBuffer->setText(QString("%1").arg(t));
	if (m_iSyncType == ffplay::AV_SYNC_AUDIO_MASTER)
		calcTotalDelay();
}
void QtFfplayer::slotTotalDelay(double t) 
{
	m_editTotalDelay->setText(QString("%1").arg(t));
}
void QtFfplayer::slotSyncType(int type) 
{
	m_iSyncType = type;
}
#endif

void QtFfplayer::initUI(QWidget* parentWidget) 
{
	m_widControl = new std::remove_pointer<decltype(m_widControl)>::type(parentWidget);
	m_widControl->setFocusPolicy(Qt::NoFocus);
	m_widControl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	m_widControl->setFixedHeight(CONTROL_WID_HEIGHT);
	m_widControl->setObjectName("m_widControl");
	m_widControl->setStyleSheet("QFrame#m_widControl{background-color:transparent;}");
	this->setStyleSheet(R"(
QPushButton{border:none;}
QPushButton:hover{border:1px solid white;}
QLabel{color:white;}

QSlider::groove:horizontal {
	border: 1px solid #604A708B;
	background: #60C0C0C0;
	height: 6px;
	border-radius: 3px;
}

QSlider::sub-page:horizontal
{

background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 1,
    stop: 0 #805DCCFF, stop: 1 #801874CD);
	border: 1px solid #4A708B;
	height: 6px;
	border-radius: 3px;
}

QSlider::add-page:horizontal {
	background: #60575757;
	border: 0px solid #777;
	width:6px;
	height:6px;
	border-radius: 3px;
}

QSlider::handle:horizontal 
{
    background-color: #A0FFFFFF;
    width: 8px;
	height: 8px;
	margin-top: -1px;
    margin-bottom: -1px;
    border-radius: 4px;
}

QSlider::handle:horizontal:hover
{
    background-color: #A0FFFFFF;
    width: 8px;
	height: 8px;
	margin-top: -1px;
    margin-bottom: -1px;
    border-radius: 4px;
}

QSlider::sub-page:horizontal:disabled
{
	background: #60606060;
	border-color: #999;
}

QSlider::add-page:horizontal:disabled
{
	background: #60606060;
	border-color: #999;
}

QSlider::handle:horizontal:disabled {
	border:none;
	background-color: transparent;
    width: 8px;
	height: 8px;
	margin-top: -1px;
    margin-bottom: -1px;
    border-radius: 4px;
}
QMenu {
    border:1px solid #7EC0EE;
    background-color:transparent;
}
QMenu::item {
    background-color:transparent;
    color:white;
}
QMenu::item:selected {
    border:1px solid #98FB98;
    color:#7CCD7C;
}
QMenu::item:disabled {
    color:gray;
}
QHeaderView
{
    background:transparent;
}
QHeaderView::section
{
    font-size:14px;
    font-family:"Microsoft YaHei";
    color:#FFFFFF;
    background:transparent;
    border:none;
}
QTableView
{
    background:#5C5C5C;
}
QScrollBar:horizontal
{
    height:8px;
    background:rgba(0,0,0,0%);
    margin:0px,0px,0px,0px;
    padding-left:9px;
    padding-right:9px;
}
QScrollBar::handle:horizontal
{
    height:8px;
    background-color:#20FFFFFF;
    border-radius:4px;
    min-width:20;
}
QScrollBar::handle:horizontal:hover
{
    height:8px;
    background-color:#50FFFFFF;
    border-radius:4px;
    min-width:20;
}
QScrollBar::handle:horizontal:pressed
{
    height:8px;
    background-color:#70FFFFFF;
    border-radius:4px;
    min-width:20;
}
QScrollBar::add-line:horizontal
{
    height:8px;width:8px;
    border-image:url(:/icon/res/left.png);
    subcontrol-position:left;
}
QScrollBar::add-line:horizontal:hover
{
    height:8px;width:8px;
    border-image:url(:/icon/res/leftHover.png);
    subcontrol-position:left;
}
QScrollBar::add-line:horizontal:pressed
{
    height:8px;width:8px;
    border-image:url(:/icon/res/leftPress.png);
    subcontrol-position:left;
}
QScrollBar::sub-line:horizontal
{
    height:8px;width:8px;
    border-image:url(:/icon/res/right.png);
    subcontrol-position:right;
}
QScrollBar::sub-line:horizontal:hover
{
    height:8px;width:8px;
    border-image:url(:/icon/res/rightHover.png);
    subcontrol-position:right;
}
QScrollBar::sub-line:horizontal:pressed
{
    height:8px;width:8px;
    border-image:url(:/icon/res/rightPress.png);
    subcontrol-position:right;
}
QScrollBar::add-page:horizontal,QScrollBar::sub-page:horizontal
{
    background:rgba(0,0,0,0%);
    border-radius:4px;
}

QScrollBar:vertical
{
    width:8px;
    background-color:transparent;
    margin:0px,0px,0px,0px;
    padding-top:9px;
    padding-bottom:9px;
}
QScrollBar::handle:vertical
{
    width: 8px;
    background-color:#20FFFFFF;
    border-radius:4px;
    min-height:20;
}
QScrollBar::handle:vertical:hover
{
    width:8px;
    background-color:#50FFFFFF;
    border-radius:4px;
    min-height:20;
}
QScrollBar::handle:vertical:pressed
{
    width:8px;
    background-color:#70FFFFFF;
    border-radius:4px;
    min-height:20;
}
QScrollBar::sub-line:vertical
{
    height:8px;width:8px;
    border-image:url(:/icon/res/top.png);
    subcontrol-position:top;
}
QScrollBar::sub-line:vertical:hover
{
    height:8px;width:8px;
    border-image:url(:/icon/res/topHover.png);
    subcontrol-position:top;
}
QScrollBar::sub-line:vertical:pressed
{
    height:8px;width:8px;
    border-image:url(:/icon/res/topPress.png);
    subcontrol-position:top;
}
QScrollBar::add-line:vertical
{
    height:8px;width:8px;
    border-image:url(:/icon/res/bottom.png);
    subcontrol-position:bottom;
}
QScrollBar::add-line:vertical:hover
{
    height:8px;width:8px;
    border-image:url(:/icon/res/bottomHover.png);
    subcontrol-position:bottom;
}
QScrollBar::add-line:vertical:pressed
{
    height:8px;width:8px;
    border-image:url(:/icon/res/bottomPress.png);
    subcontrol-position:bottom;
}
QScrollBar::add-page:vertical,QScrollBar::sub-page:vertical
{
    background-color:#00FFFFFF;
    border-radius:4px;
}
)");

#ifdef DELAY_ANALYSIS
	m_widRealTimeInfo = new std::remove_pointer<decltype(m_widRealTimeInfo)>::type(m_widControl);
	m_widRealTimeInfo->setStyleSheet(R"(QLineEdit{background-color:transparent;border:1px solid #66CDAA;border-radius:4px;color:white;}
QLineEdit:disabled{background-color:transparent;border:1px solid gray;border-radius:4px;font-color:white;}
QDoubleSpinBox {
    background-color:transparent;color:white;
	font-size:18px;
}
QDoubleSpinBox::up-button {
    subcontrol-origin: border;
    subcontrol-position: top right; 
    border-image: url(:/icon/res/up.png) 1;
    border-width: 1px;
}
QDoubleSpinBox::up-button:hover {
    border-image: url(:/icon/res/upHover.png) 1;
}
QDoubleSpinBox::up-button:pressed {
    border-image: url(:/icon/res/upPress.png) 1;
}
QDoubleSpinBox::down-button {
    subcontrol-origin: border;
    subcontrol-position: bottom right; 
    border-image: url(:/icon/res/down.png) 1;
    border-width: 1px;
}
QDoubleSpinBox::down-button:hover {
    border-image: url(:/icon/res/downHover.png) 1;
}
QDoubleSpinBox::down-button:pressed {
    border-image: url(:/icon/res/downPress.png) 1;
}
)");
	m_widRealTimeInfo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	m_widRealTimeInfo->setFixedHeight(CONTROL_TEST_WID_HEIGHT);
	m_widRealTimeInfo->setLayout(makeGridLay(m_widRealTimeInfo));
	m_editDelay->setText("200");
	m_editUndulate->setText("100");

#endif

	m_sliderProgress = new QProgressSlider([this](long long v, long long incr) {m_play.setReqeustSeek(v, incr); }, m_widControl);
	m_sliderProgress->setStyleSheet(R"(QProgressSlider{background-color:white;})");
	m_labelPlayTime = new QLabel("00:00:00", m_widControl);
	m_sliderVoice = new QSlider(m_widControl);
	m_sliderVoice->setRange(0,128);
	m_sliderVoice->setValue(100);
	m_btnVoice = new QPushButton(m_widControl);
	m_btnVoice->setCheckable(true);
	m_btnVoice->setFixedSize(24,24);
	m_btnVoice->setStyleSheet(R"(QPushButton{border-image:url(:/icon/res/voice.png);}
QPushButton:hover{border-image:url(:/icon/res/voiceHover.png);}
QPushButton:pressed{border-image:url(:/icon/res/voicePress.png);}
QPushButton:checked{border-image:url(:/icon/res/voiceShut.png);})");
	m_sliderVoice->setOrientation(Qt::Orientation::Horizontal);
	m_sliderVoice->setFixedSize(120,8);
	
	std::vector<std::pair<QPushButton* QtFfplayer::*, QString>>btnSets{ {&QtFfplayer::m_btnPlay, R"(QPushButton{border-image:url(:/icon/res/play.png);}
QPushButton:hover{border-image:url(:/icon/res/playHover.png);}
QPushButton:pressed{border-image:url(:/icon/res/playPress.png);}
)"}
		,{&QtFfplayer::m_btnStop,R"(QPushButton{border-image:url(:/icon/res/stop.png);}
QPushButton:hover{border-image:url(:/icon/res/stopHover.png);}
QPushButton:pressed{border-image:url(:/icon/res/stopPress.png);})"}
		,{&QtFfplayer::m_btnBack,R"(QPushButton{border-image:url(:/icon/res/back.png);}
QPushButton:hover{border-image:url(:/icon/res/backHover.png);}
QPushButton:pressed{border-image:url(:/icon/res/backPress.png);})"}
		,{&QtFfplayer::m_btnAhead,R"(QPushButton{border-image:url(:/icon/res/ahead.png);}
QPushButton:hover{border-image:url(:/icon/res/aheadHover.png);}
QPushButton:pressed{border-image:url(:/icon/res/aheadPress.png);})"}
		,{&QtFfplayer::m_btnOpen,R"(QPushButton{border-image:url(:/icon/res/open.png);}
QPushButton:hover{border-image:url(:/icon/res/openHover.png);}
QPushButton:pressed{border-image:url(:/icon/res/openPress.png);})"}
		,{&QtFfplayer::m_btnList,R"(QPushButton{border-image:url(:/icon/res/file.png);}
QPushButton:hover{border-image:url(:/icon/res/fileHover.png);}
QPushButton:pressed{border-image:url(:/icon/res/filePress.png);})"}
	};

	for (auto& ele : btnSets) 
	{
		this->*ele.first = new QPushButton(m_widControl);
		(this->*ele.first)->setStyleSheet(ele.second);
		(this->*ele.first)->setFixedSize(24,24);
	}
	m_btnPlay->setCheckable(true);

	m_spinPlaySpeed = new QDoubleSpinBox(m_widControl);
	//m_spinPlaySpeed->setFocusPolicy(Qt::NoFocus);
	m_spinPlaySpeed->setStyleSheet(R"(
QDoubleSpinBox {
    background-color:transparent;color:white;
	font-size:18px;
}
QDoubleSpinBox::up-button {
    subcontrol-origin: border;
    subcontrol-position: top right; 
    border-image: url(:/icon/res/up.png) 1;
    border-width: 1px;
}
QDoubleSpinBox::up-button:hover {
    border-image: url(:/icon/res/upHover.png) 1;
}
QDoubleSpinBox::up-button:pressed {
    border-image: url(:/icon/res/upPress.png) 1;
}
QDoubleSpinBox::down-button {
    subcontrol-origin: border;
    subcontrol-position: bottom right; 
    border-image: url(:/icon/res/down.png) 1;
    border-width: 1px;
}
QDoubleSpinBox::down-button:hover {
    border-image: url(:/icon/res/downHover.png) 1;
}
QDoubleSpinBox::down-button:pressed {
    border-image: url(:/icon/res/downPress.png) 1;
}
)");

	m_spinPlaySpeed->setFixedWidth(60);
	m_spinPlaySpeed->setRange(0.01, 99);
	m_spinPlaySpeed->setDecimals(2);
	m_spinPlaySpeed->setSingleStep(0.01);
	m_spinPlaySpeed->setValue(1.00);
	m_labelMediaTime = new QLabel("00:00:00", m_widControl);

	QHBoxLayout* btnLay = new QHBoxLayout;
	btnLay->setAlignment(Qt::AlignCenter);
	btnLay->setContentsMargins(0,0,0,0);
	btnLay->setSpacing(2);
	btnLay->setAlignment(Qt::AlignCenter);
	btnLay->addWidget(m_labelPlayTime);
	btnLay->addSpacerItem(new QSpacerItem(30, 1, QSizePolicy::Fixed, QSizePolicy::Preferred));



	m_btnGroupPlayMode = new QButtonGroup(m_widControl);
	m_btnGroupPlayMode->setExclusive(true);
	for (auto& ele : PlayMode) 
	{
		QPushButton*btnTmp = new QPushButton(m_widControl);
		btnTmp->setFixedSize(24,24);
		btnTmp->setStyleSheet(makeStyleSheet(std::get<1>(ele), std::get<2>(ele), std::get<3>(ele), std::get<4>(ele)));
		this->*std::get<0>(ele) = btnTmp;
		btnLay->addWidget(btnTmp);
		btnTmp->setCheckable(true);
		m_btnGroupPlayMode->addButton(btnTmp, std::get<5>(ele));
	}

	m_btnPlayModeListCircle->setChecked(true);

	
	m_btnScreen = new QPushButton(m_widControl);
	m_btnScreen->setFixedSize(24, 24);
	m_btnScreen->setCheckable(true);
	setBtnScreenState(false);
	
	btnLay->addStretch();

	btnLay->addWidget(m_btnVoice);
	btnLay->addWidget(m_sliderVoice);
	btnLay->addWidget(m_btnPlay);
	btnLay->addWidget(m_btnStop);
	btnLay->addWidget(m_btnBack);
	btnLay->addWidget(m_btnAhead);
	btnLay->addWidget(m_btnOpen);
	btnLay->addWidget(m_spinPlaySpeed);
	btnLay->addWidget(m_btnList);
	btnLay->addWidget(m_btnScreen);

	m_btnOther = new QPushButton(m_widControl);
	m_btnOther->setFixedSize(24,24);
	m_btnOther->setStyleSheet(makeStyleSheetUncheckable(std::get<0>(m_vStateIconBtnOther[0])
		, std::get<1>(m_vStateIconBtnOther[0])
		, std::get<2>(m_vStateIconBtnOther[0])));



	btnLay->addWidget(m_btnOther);
	btnLay->addWidget(m_labelMediaTime);


	QVBoxLayout* controlWidL = new QVBoxLayout;
	controlWidL->setContentsMargins(0,2,0,2);
	controlWidL->setSpacing(2);
	controlWidL->setAlignment(Qt::AlignCenter);
#ifdef DELAY_ANALYSIS
	controlWidL->addWidget(m_widRealTimeInfo);
#endif

	controlWidL->addWidget(m_sliderProgress);
	controlWidL->addLayout(btnLay);
	m_widControl->setLayout(controlWidL);

	m_listPlayList = new QtPlayList(this,m_sliderProgress,this);

	m_listPlayList->setFixedWidth(LIST_WID_WIDTH);
	m_listPlayList->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
	m_listPlayList->setPlayModelCallBack(std::bind(&QtFfplayer::getPlayMode, this));
	m_listPlayList->hide();
	auto setsList = CConfig::readPlayList();
	m_listPlayList->appendList(std::move(setsList));

	
	QVBoxLayout* widControlInLay = new QVBoxLayout;
	widControlInLay->setContentsMargins(0,0,0,0);
	widControlInLay->setSpacing(0);
	widControlInLay->addStretch();
	widControlInLay->addWidget(m_widControl);

	QHBoxLayout* mainL = new QHBoxLayout;
	mainL->setContentsMargins(0,0,0,0);
	mainL->setSpacing(0);
	mainL->setAlignment(Qt::AlignRight);
	mainL->addLayout(widControlInLay);
	mainL->addWidget(m_listPlayList);
	setLayout(mainL);

	setContextMenuPolicy(Qt::ContextMenuPolicy::DefaultContextMenu);
	m_contextMenu = new QMenu(this);
	//m_initMenuInfo
	for (auto& ele : m_initMenuInfo) 
	{
		std::get<1>(ele) = new QAction(std::get<0>(ele),this);
		m_contextMenu->addAction(std::get<1>(ele));
	}
	setContextMenuPolicy(Qt::ContextMenuPolicy::DefaultContextMenu);
	setMenuTransparent(m_contextMenu);
}

QRect QtFfplayer::calculate_display_rect(ffplay::Frame* f)
{
	return calculate_display_rect(0, 0, width() - (m_listPlayList->isHidden() ? 0:LIST_WID_WIDTH), height() - (m_widControl->isHidden() ? 0:CONTROL_WID_HEIGHT), f->width, f->height, f->sar);
}

QRect QtFfplayer::calculate_display_rect(
	int scr_xleft, int scr_ytop, int scr_width, int scr_height,
	int pic_width, int pic_height, AVRational pic_sar)
{
	/*if (m_iPreFrameXOffset == scr_xleft
		&& m_iPreFrameYOffset == scr_ytop
		&& m_iPreFrameWidth == pic_width
		&& m_iPreFrameHeight == pic_height
		&& m_iPreFrameWinWidth == scr_width
		&& m_iPreFrameWinHeight == scr_height)
		return;*/
	m_iPreFrameXOffset = scr_xleft;
	m_iPreFrameYOffset = scr_ytop;
	m_iPreFrameWidth = pic_width;
	m_iPreFrameHeight = pic_height;
	m_iPreFrameWinWidth = scr_width;
	m_iPreFrameWinHeight = scr_height;

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
}

void QtFfplayer::deleteTex() 
{
	makeCurrent();
	if (y_tex) glDeleteTextures(1, &y_tex);
	if (u_tex) glDeleteTextures(1, &u_tex);
	if (v_tex) glDeleteTextures(1, &v_tex);
	doneCurrent();
}

void QtFfplayer::initConnect() 
{
	QObject::connect(m_btnOpen, SIGNAL(clicked(bool)), this,SLOT(slotBtnOpenClick()));
	QObject::connect(this, SIGNAL(signalsVideoLength(long long)), this, SLOT(slotVideoLength(long long)));
	QObject::connect(this, SIGNAL(signalsCurTime(long long)), this, SLOT(slotCurTime(long long)));
	//QObject::connect(this, SIGNAL(signalsGetFrame()), this, SLOT(slotGetFrame()));
	

	QObject::connect(m_btnVoice, SIGNAL(clicked(bool)), this, SLOT(slotVoiceBtnClicked(bool)));
	QObject::connect(m_btnPlay, SIGNAL(clicked(bool)), this, SLOT(slotPlayBtnClicked(bool)));
	QObject::connect(m_btnStop, SIGNAL(clicked(bool)), this, SLOT(slotStopBtnClicked()));
	QObject::connect(m_btnBack, SIGNAL(clicked(bool)), this, SLOT(slotBackBtnClick()));
	QObject::connect(m_btnAhead, SIGNAL(clicked(bool)), this, SLOT(slotAheadBtnClick()));
	QObject::connect(m_btnList, SIGNAL(clicked(bool)), this, SLOT(slotListBtnClick()));

	QObject::connect(m_spinPlaySpeed, SIGNAL(valueChanged(double)), this, SLOT(slotSpinBoxValueChanged(double)));

	QObject::connect(m_btnScreen, SIGNAL(clicked(bool)), this, SLOT(slotScreenBtnClick(bool)));

	for (auto& ele : m_initMenuInfo) 
		QObject::connect(std::get<1>(ele), std::get<2>(ele), this, std::get<3>(ele));

	QObject::connect(m_actionClock, SIGNAL(stateChanged(int)), this, SLOT(slotSyncModelChanged(int)));

#ifdef DELAY_ANALYSIS
	for (auto& ele : m_vDelayAnalysis)
		QObject::connect(this,std::get<5>(ele),this,std::get<6>(ele));
	QObject::connect(this, SIGNAL(signalsSyncType(int)), this, SLOT(slotSyncType(int)));
#endif

}

void QtFfplayer::mouseMoveEvent(QMouseEvent* event)
{
	QPoint EndPos;
	if (event->buttons() == Qt::LeftButton && m_bMousePressed)
	{
		EndPos = event->globalPos() - StartPos;
		this->move(EndPos);
	}
	else
	{
		QPoint mousePt = event->globalPos();
		QPoint leftTop = pos();

		int titleHeight = isFullScreen() ? 0:frameGeometry().height() - geometry().height();

		QRect controlWidgetRect{ QPoint(leftTop.x(), leftTop.y() + height() - m_widControl->height() + titleHeight), QPoint(leftTop.x() + width() - (m_listPlayList->isHidden() ? 0:LIST_WID_WIDTH),leftTop.y() + height() + titleHeight) };
		bool b = controlWidgetRect.contains(mousePt);

		if (m_widControl->isHidden() == b)
		{
			m_widControl->setVisible(b);
			calculate_display_rect(&m_frameSave);
			update();
			std::vector<QWidget*>focusJudeg = {
			m_btnPlayModeSingle
			,m_btnPlayModeSingleCircle
			,m_btnPlayModeList
			,m_btnPlayModeListCircle
			,m_btnPlayModeRander
			,m_btnOther
			,m_btnScreen
			,m_btnPlay
			,m_btnStop
			,m_btnBack
			,m_btnAhead
			,m_btnOpen
			,m_btnList
			,m_spinPlaySpeed
			,m_sliderVoice
			,m_btnVoice
			,m_sliderProgress
			};
			bool b = false;
			for (auto& ele : focusJudeg)
				if (b = ele->hasFocus())
					break;
			setFocus();
		}
	}
}
void QtFfplayer::mousePressEvent(QMouseEvent* event)
{
	setFocus();
	if (event->buttons() == Qt::LeftButton && !isFullScreen())
	{
		StartPos = event->globalPos() - this->frameGeometry().topLeft();
		m_bMousePressed = true;
	}
	qDebug() << event->buttons();
}

void QtFfplayer::mouseReleaseEvent(QMouseEvent* event) 
{
	m_bMousePressed = false;
}

void QtFfplayer::leaveEvent(QEvent* event) 
{
	if (!m_widControl->isHidden())
	{
		m_widControl->setVisible(false);
		calculate_display_rect(&m_frameSave);
		update();
	}
}

void QtFfplayer::keyPressEvent(QKeyEvent* ev) 
{
	long long incr = 0;
	switch (ev->key())
	{
        //case SDL_KEYDOWN:
            /*if (exit_on_keydown || event.key.keysym.sym == SDLK_ESCAPE || event.key.keysym.sym == SDLK_q) {
                do_exit(cur_stream);
                break;
            }*/
			//break;
		case Qt::Key_Control:
			m_listPlayList->setControlKeyPressed(true);
			//m_listPlayList->keyPressEvent(ev);
		//toggle_full_screen(cur_stream);
		//cur_stream->force_refresh = 1;
			break;
		case Qt::Key_F:
             //toggle_full_screen(cur_stream);
             //cur_stream->force_refresh = 1;
             break;
        case Qt::Key_P:
		case Qt::Key_Space:
             //toggle_pause(cur_stream);
			slotPlayBtnClicked(!m_btnPlay->isChecked());
             break;
        case Qt::Key_M:
             //toggle_mute(cur_stream);
             break;
        //case SDLK_KP_MULTIPLY:
        case Qt::Key_0:
             //update_volume(cur_stream, 1, SDL_VOLUME_STEP);
             break;
        //case SDLK_KP_DIVIDE:
        case Qt::Key_9:
             //update_volume(cur_stream, -1, SDL_VOLUME_STEP);
             break;
        case Qt::Key_S: // S: Step to next frame
                //step_to_next_frame(cur_stream);
             break;
        case Qt::Key_A:
                //stream_cycle_channel(cur_stream, AVMEDIA_TYPE_AUDIO);
                break;
        case Qt::Key_V:
                //stream_cycle_channel(cur_stream, AVMEDIA_TYPE_VIDEO);
                break;
        case Qt::Key_C:
                //stream_cycle_channel(cur_stream, AVMEDIA_TYPE_VIDEO);
                //stream_cycle_channel(cur_stream, AVMEDIA_TYPE_AUDIO);
                //stream_cycle_channel(cur_stream, AVMEDIA_TYPE_SUBTITLE);
                break;
		case Qt::Key_T:
                //stream_cycle_channel(cur_stream, AVMEDIA_TYPE_SUBTITLE);
                break;
		case Qt::Key_Escape:
			if (isFullScreen())
			{
				emit m_btnScreen->clicked(false);
				m_btnScreen->setChecked(false);
			}
			break;
        case Qt::Key_W:
#if CONFIG_AVFILTER
                /*if (cur_stream->show_mode == SHOW_MODE_VIDEO && cur_stream->vfilter_idx < nb_vfilters - 1) {
                    if (++cur_stream->vfilter_idx >= nb_vfilters)
                        cur_stream->vfilter_idx = 0;
                } else {
                    cur_stream->vfilter_idx = 0;
                    toggle_audio_display(cur_stream);
                }*/
#else
                //toggle_audio_display(cur_stream);
#endif
                break;
            //case SDLK_PAGEUP:
                //if (cur_stream->ic->nb_chapters <= 1) {
                //    incr = 600.0;
                //    goto do_seek;
               // }
                //seek_chapter(cur_stream, 1);
               // break;
            //case SDLK_PAGEDOWN:
                //if (cur_stream->ic->nb_chapters <= 1) {
                //    incr = -600.0;
                //    goto do_seek;
                //}
                //seek_chapter(cur_stream, -1);
                //break;
            case Qt::Key_Left:
                incr = -10000000;
                goto do_seek;
            case Qt::Key_Right:
                incr = 10000000;
                goto do_seek;
            case Qt::Key_Up:
                incr = 60000000;
                goto do_seek;
            case Qt::Key_Down:
                incr = -60000000;
            do_seek:
				{
					//m_sliderProgress->setHandleOffSetPos(incr);
				long long pos = m_sliderProgress->value();//get_master_clock(cur_stream);
                        //if (isnan(pos))
                        //    pos = (double)cur_stream->seek_pos / AV_TIME_BASE;
               // pos += incr;
				m_play.setReqeustSeek(pos+ incr, incr);
                 //if (m_play.ic->start_time != AV_NOPTS_VALUE && pos < cur_stream->ic->start_time / (double)AV_TIME_BASE)
                 //           pos = cur_stream->ic->start_time / (double)AV_TIME_BASE;
                 //stream_seek(cur_stream, (int64_t)(pos * AV_TIME_BASE), (int64_t)(incr * AV_TIME_BASE), 0);

                }
                break;
        default:
            break;
        }
		QOpenGLWidget::keyPressEvent(ev);
}

void QtFfplayer::keyReleaseEvent(QKeyEvent* ev) 
{
	switch (ev->key())
	{
	case Qt::Key_Control:
		m_listPlayList->setControlKeyPressed(false);
		//m_listPlayList->keyPressEvent(ev);
	//toggle_full_screen(cur_stream);
	//cur_stream->force_refresh = 1;
		break;
	}
	QOpenGLWidget::keyReleaseEvent(ev);
}

void QtFfplayer::dragEnterEvent(QDragEnterEvent* event) 
{
	if (event->mimeData()->hasUrls() && event->source() == nullptr)
	{
		event->acceptProposedAction();
	}
	else
	{
		event->ignore();
	}
}

void QtFfplayer::dropEvent(QDropEvent* event) 
{
	const QMimeData* mimeData = event->mimeData();
	if (!mimeData->hasUrls() && event->source() != nullptr)
		return;

	QList<QUrl> urlList = mimeData->urls();
	QStringList sets;
	for (auto& ele : urlList)
	{
		QString fileName = ele.toLocalFile();
		if (fileName.isEmpty())
			continue;
		sets<<fileName;
	}
	auto firstType = m_listPlayList->appendList(std::move(sets));
	//m_listPlayList->setPlayIndex(firstType.second);
	m_listPlayList->slotRowDoubleClicked(m_listPlayList->getModel()->index(firstType.second, 0));
}

void QtFfplayer::setBtnPlayingState(bool b) 
{
	if (m_vStateIconBtnPlay.size() > 1)
	{
		int i = b ? 1:0;
		m_btnPlay->setStyleSheet(makeStyleSheetUncheckable(std::get<0>(m_vStateIconBtnPlay[i]), std::get<1>(m_vStateIconBtnPlay[i]), std::get<2>(m_vStateIconBtnPlay[i])));
		m_btnPlay->setChecked(b);
	}
}

void QtFfplayer::setBtnScreenState(bool b) 
{
	if (m_vStateIconBtnFullScreen.size() > 1)
	{
		int i = b ? 1 : 0;
		m_btnScreen->setStyleSheet(makeStyleSheetUncheckable(std::get<0>(m_vStateIconBtnFullScreen[i])
			, std::get<1>(m_vStateIconBtnFullScreen[i])
			, std::get<2>(m_vStateIconBtnFullScreen[i])));
	}
}


void QtFfplayer::slotOpen() 
{
	slotBtnOpenClick();
}

void QtFfplayer::slotOpenUrl() 
{
	QtOpenUrlDlg dlg(this);
	if (dlg.exec() == QDialog::Accepted)
	{
		QString str = dlg.getUrl();
		if (!str.isEmpty())
		{
			QStringList strList;
			strList << str;
			appendToListAndPlayFirstFile(std::move(strList));
		}
	}
}

void QtFfplayer::appendToListAndPlayFirstFile(QStringList&& strList) 
{
	auto p = m_listPlayList->appendList(std::move(strList));
	if (p.first.isEmpty() || p.second < -1)
		stop();
	else
	{
		setBtnPlayingState(true);
		m_listPlayList->slotRowDoubleClicked(m_listPlayList->getModel()->index(p.second, 0));
	}
}

void QtFfplayer::contextMenuEvent(QContextMenuEvent* event)
{
	m_contextMenu->exec(QCursor::pos());
}

void QtFfplayer::setMenuTransparent(QMenu* menu) 
{
	menu->setWindowFlags(menu->windowFlags() | Qt::FramelessWindowHint);
	menu->setAttribute(Qt::WA_TranslucentBackground);
	HWND hwnd = reinterpret_cast<HWND>(menu->winId());
	DWORD class_style = ::GetClassLong(hwnd, GCL_STYLE);
	class_style &= ~CS_DROPSHADOW;
	::SetClassLong(hwnd, GCL_STYLE, class_style);
}


void QtFfplayer::setFrameSize( int width,  int height)
{
	m_frameWidth = width;
	m_frameHeight = height;
	m_isOpenGLInit = false;

}

void QtFfplayer::initializeGL()
{
	initializeOpenGLFunctions();
	m_program.removeAllShaders();
	m_program.destroyed();
	if (!m_program.create())
	{
		qDebug() << "m_program.create() failed";
	}
	glDisable(GL_DEPTH_TEST);

	m_program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":shaders/shaders/yuv_core.vert");
	m_program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/shaders/yuv_core.frag");

	m_program.link();

	u_pos = m_program.uniformLocation("draw_pos");
	m_vao.destroy();
	m_vao.create();
}

void QtFfplayer::paintGL()
{
	if (!m_frameSave.frame)
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
			memcpy(&m_frameCopy, &m_frameSave, sizeof(m_frameCopy));
			av_frame_unref(m_frame_tmp);
			if (m_frameSave.frame)
				av_frame_ref(m_frame_tmp, m_frameSave.frame);
			m_frameCopy.frame = m_frame_tmp;
		}
	}

	if (!m_isOpenGLInit)
	{
			
			initializeTextures();
			m_isOpenGLInit = true;
			
	}
	
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	QRect rect = calculate_display_rect(&m_frameSave);
	
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
	if(bUploadTex)
		setYPixels(m_frameCopy.frame->data[0], m_frameCopy.frame->linesize[0]);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, u_tex);
	if (bUploadTex)
		setUPixels(m_frameCopy.frame->data[1], m_frameCopy.frame->linesize[1]);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, v_tex);
	if (bUploadTex)
		setVPixels(m_frameCopy.frame->data[2], m_frameCopy.frame->linesize[2]);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	m_program.release();
	m_vao.release();

}

void QtFfplayer::resizeGL(int width, int height) 
{
	update();
}

void QtFfplayer::setYPixels(uint8_t* pixels, int stride)
{
	bindPixelTexture(y_tex, YTexture, pixels, stride);
}

void QtFfplayer::setUPixels(uint8_t* pixels, int stride)
{
	bindPixelTexture(u_tex, UTexture, pixels, stride);
}

void QtFfplayer::setVPixels(uint8_t* pixels, int stride)
{
	bindPixelTexture(v_tex, VTexture, pixels, stride);
}

void QtFfplayer::initializeTextures()
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
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, m_frameWidth, m_frameHeight, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);

	glGenTextures(1, &u_tex);
	glBindTexture(GL_TEXTURE_2D, u_tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, m_frameWidth / 2, m_frameHeight / 2, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);

	glGenTextures(1, &v_tex);
	glBindTexture(GL_TEXTURE_2D, v_tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, m_frameWidth / 2, m_frameHeight / 2, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
}

void QtFfplayer::bindPixelTexture(GLuint texture, QtFfplayer::YUVTextureType textureType, uint8_t* pixels, int stride)
{
	if (!pixels)
		return;

	int frameW = m_frameWidth;
	int frameH = m_frameHeight;

	int const width = textureType == YTexture ? frameW : frameW / 2;
	int const height = textureType == YTexture ? frameH : frameH / 2;

	glBindTexture(GL_TEXTURE_2D, texture);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, stride);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RED, GL_UNSIGNED_BYTE, pixels);

}

#ifdef	DELAY_ANALYSIS
void QtFfplayer::calcTotalDelay() 
{
	//std::vector<QLineEdit*>vsets{ m_editAudioData , m_editAudioFrameData ,m_editBuffer ,m_editSDLFeedBuffer ,m_editSoundTouchTranslateBuffer ,m_editSoundTouchUnprocessedBuffer };
	double t = 0.0;

	for (auto& ele : m_vDelayAnalysis) 
	{
		if(std::get<7>(ele) == m_iSyncType)
			t += (this->*std::get<2>(ele))->text().toDouble();
	}
	m_editTotalDelay->setText(QString("%1").arg(t));

	double upT = m_editDelay->text().toDouble();
	double lowT = upT + m_editUndulate->text().toDouble();
	if (t > lowT && !m_bIsInAccelSpeed)
	{
		m_spinPlaySpeed->setValue(m_spinEnableSpeed->value());
		m_bIsInAccelSpeed = true;
	}
	else if (t < upT && m_bIsInAccelSpeed) 
	{
		m_spinPlaySpeed->setValue(1.0);
		m_bIsInAccelSpeed = false;
	}
}
#endif
