
// MFC_FFmpegDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "MFC_FFmpeg.h"
#include "MFC_FFmpegDlg.h"
#include "afxdialogex.h"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "SDL2/SDL.h"
#include "libavutil/pixfmt.h"
#include "libavutil/imgutils.h"
}

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CMFCFFmpegDlg dialog



CMFCFFmpegDlg::CMFCFFmpegDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_MFC_FFMPEG_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMFCFFmpegDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_FILEPATH, m_url);
	DDX_Control(pDX, IDC_SLIDER_PROGRESS, m_slider);
	DDX_Control(pDX, IDC_STATIC_DURATION, m_duration);
	DDX_Control(pDX, IDC_STATIC_PROGRESS, m_progress);
}

BEGIN_MESSAGE_MAP(CMFCFFmpegDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_GETMINMAXINFO()
	ON_BN_CLICKED(IDC_BUTTON_PLAY, &CMFCFFmpegDlg::OnBnClickedButtonPlay)
	ON_BN_CLICKED(IDC_BUTTON_PAUSE, &CMFCFFmpegDlg::OnBnClickedButtonPause)
	ON_BN_CLICKED(IDC_BUTTON_STOP, &CMFCFFmpegDlg::OnBnClickedButtonStop)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE, &CMFCFFmpegDlg::OnBnClickedButtonBrowse)
	ON_WM_DROPFILES()
	ON_WM_HSCROLL()
END_MESSAGE_MAP()


// CMFCFFmpegDlg message handlers

BOOL CMFCFFmpegDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	CRect r;
	GetWindowRect(&r);
	dlgMinWidth = r.right - r.left;
	dlgMinHeight = r.bottom - r.top;

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CMFCFFmpegDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CMFCFFmpegDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CMFCFFmpegDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CMFCFFmpegDlg::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
	// TODO: Add your message handler code here and/or call default
	// CRect r;
	// GetClientRect(&r);
	// lpMMI->ptMinTrackSize.x = r.Width();
	// lpMMI->ptMinTrackSize.y = r.Height();
	lpMMI->ptMinTrackSize.x = dlgMinWidth;
	lpMMI->ptMinTrackSize.y = dlgMinHeight;
	CDialogEx::OnGetMinMaxInfo(lpMMI);
}


//FFmpeg+SDL related code

//Refresh Event
#define SFM_REFRESH_EVENT  (SDL_USEREVENT + 1)
#define SFM_BREAK_EVENT  (SDL_USEREVENT + 2)

#define SDL_HINT_RENDER_SCALE_QUALITY 1

int thread_exit = 0;
int thread_pause = 0;

int sfp_refresh_thread(void* opaque) {

	thread_exit = 0;
	thread_pause = 0;

	while (thread_exit == 0) {
		SDL_Event event;
		event.type = SFM_REFRESH_EVENT;
		SDL_PushEvent(&event);
		SDL_Delay(30);
	}
	//Quit
	SDL_Event event;
	event.type = SFM_BREAK_EVENT;
	SDL_PushEvent(&event);
	thread_exit = 0;
	thread_pause = 0;
	return 0;
}

//main function is changed to ffmpegplayer 
int ffmpegplayer(LPVOID lpParam)
{
	AVFormatContext* v_pFormatCtx = NULL;

	AVStream* video_st = NULL, * audio_st = NULL;
	AVCodec* video_dec = NULL, * audio_dec = NULL;
	AVCodecContext* video_dec_ctx = NULL, * audio_dec_ctx = NULL;

	unsigned char* frame_out_buffer;
	AVFrame* p_frame = NULL, * p_frame_yuv = NULL;
	AVPacket* p_pkt = NULL;

	struct SwsContext* video_sws_ctx;
	int st_index[AVMEDIA_TYPE_NB];
	int video_frame_cout = 0, audio_frame_cout = 0;

	char frame_type[][3] = { "N", "I", "P", "B", "S" ,"SI", "SP", "BI" };

	CString time_length, time_progress;
	int tns, thh, tmm, tss;


	//------------SDL----------------
	int screen_w, screen_h;
	SDL_Window* screen;
	SDL_Renderer* sdlRenderer;
	SDL_Texture* sdlTexture;
	SDL_Rect sdlRect;
	SDL_Thread* video_tid;
	SDL_Event event;
	CRect rcDisplay;

	//===========================================
	 //The file path is changed as follows:
	CMFCFFmpegDlg* dlg = (CMFCFFmpegDlg*)lpParam;
	char filepath[250] = { 0 };
	GetWindowTextA(dlg->m_url, (LPSTR)filepath, 250);
	//===========================================

	// 1. 打开文件，分配AVFormatContext	结构体上下文
	v_pFormatCtx = avformat_alloc_context();	// 初始化上下文
	if (avformat_open_input(&v_pFormatCtx, filepath, NULL, NULL) < 0) {
		AfxMessageBox(_T("Open file failed!"));
		return -1;
	}
	// 2. 查找文件中的流信息
	if (avformat_find_stream_info(v_pFormatCtx, NULL) < 0) {
		AfxMessageBox(_T("avformat_alloc_context Faild!"));
		return -1;
	}

	// 3. 打印流信息
	// av_dump_format(v_pFormatCtx, 0, filepath, 0);
	tns = v_pFormatCtx->duration / 1000000;
	thh = tns / 3600;
	tmm = (tns % 3600) / 60;
	tss = (tns % 60);
	time_length.Format(L"/ %02d:%02d:%02d", thh, tmm, tss);
	dlg->m_duration.SetWindowTextW(time_length);

	memset(st_index, -1, sizeof(st_index));

	// 4. 找到视频流 与 音频流的 index索引
	for (unsigned int i = 0; i < v_pFormatCtx->nb_streams; i++)
	{
		AVMediaType type = v_pFormatCtx->streams[i]->codecpar->codec_type;
		if (type >= 0 && st_index[type] == -1) {
			st_index[type] = av_find_best_stream(v_pFormatCtx, type, -1, -1, NULL, 0);
			// AfxMessageBox(_T("找到 st_index[") + type + _T("] = ") + st_index[type]);
		}
	}

	// 5. 检查是否存在流信息
	for (unsigned int i = 0; ; i++) {
		if (i < v_pFormatCtx->nb_streams && st_index[i] >= 0) {
			break;
		}
		else {
			AfxMessageBox(_T("Stream infomation undetected!"));
		}
	}

	// 6. 找到视频流的 codec, 初始化并打开解码器
	if (st_index[AVMEDIA_TYPE_VIDEO] >= 0) {
		// 获得视频流
		video_st = v_pFormatCtx->streams[st_index[AVMEDIA_TYPE_VIDEO]];
		dlg->m_slider.SetRange(0, video_st->duration);
		// 找到视频解码器
		video_dec = avcodec_find_decoder(video_st->codecpar->codec_id);
		// 初始化视频解码器上下文
		video_dec_ctx = avcodec_alloc_context3(video_dec);
		// 拷贝解码器信息到上下文中
		avcodec_parameters_to_context(video_dec_ctx, video_st->codecpar);
		// 打开视频解码器
		if (avcodec_open2(video_dec_ctx, video_dec, NULL) < 0) {
			AfxMessageBox(_T("Open Codec Faild"));
		}
		// AfxMessageBox(_T("打开视频解码器： ") + video_dec->name + _T(" (") + video_dec->long_name + _T(")"));
	}

	// 7. 找到音频流的 codec, 初始化并打开解码器
	if (st_index[AVMEDIA_TYPE_AUDIO] >= 0) {
		// 获得音频流
		audio_st = v_pFormatCtx->streams[st_index[AVMEDIA_TYPE_AUDIO]];
		// 找到音频解码器
		audio_dec = avcodec_find_decoder(audio_st->codecpar->codec_id);
		// 初始化音频解码器上下文
		audio_dec_ctx = avcodec_alloc_context3(audio_dec);
		// 拷贝解码器信息到上下文中
		avcodec_parameters_to_context(audio_dec_ctx, audio_st->codecpar);
		// 打开音频解码器
		if (avcodec_open2(audio_dec_ctx, audio_dec, NULL) < 0) {
			AfxMessageBox(_T("Open Codec Faild"));
		}
		// AfxMessageBox(_T("打开音频解码器： ") + audio_dec->name + _T(" (") + audio_dec->long_name + _T(")"));
	}

	// 8. 分配AVFrame 和 AVPacket 内存
	p_frame = av_frame_alloc();	// 初始化 AVFrame 帧内存
	
	// 申请 视频 buffer
	p_frame_yuv = av_frame_alloc();
	frame_out_buffer = (unsigned char*)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, video_dec_ctx->width, video_dec_ctx->height, 1));
	av_image_fill_arrays(p_frame_yuv->data, p_frame_yuv->linesize, frame_out_buffer, AV_PIX_FMT_YUV420P, video_dec_ctx->width, video_dec_ctx->height, 1);
	
	video_sws_ctx = sws_getContext(video_dec_ctx->width, video_dec_ctx->height, video_dec_ctx->pix_fmt, video_dec_ctx->width, video_dec_ctx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

	// 初始化 AVPacket 包内存
	p_pkt = av_packet_alloc();
	av_init_packet(p_pkt);
	p_pkt->data = NULL;
	p_pkt->size = 0;

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
		AfxMessageBox(_T("Could not initialize SDL\n"));
		return -1;
	}

	// 8. 初始化 SDL， 支持音频、视频、定时器
	screen_w = video_dec_ctx->width;
	screen_h = video_dec_ctx->height;

	
	// 9. 创建SDL 窗口(显示位置由系统决定，窗口大小为视频大小)
	// 显示在弹出窗口
	// screen = SDL_CreateWindow("Simplest ffmpeg player's Window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screen_w, screen_h, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
	//===========================================
	// 显示在MFC控件上
	screen = SDL_CreateWindowFrom(dlg->GetDlgItem(IDC_STATIC_DISPLAY)->GetSafeHwnd());
	//===========================================
	if (!screen) {
		AfxMessageBox(_T("SDL: could not create window - exiting\n"));
		return -1;
	}

	// 10. 初始化 SDL_Renderer 渲染器, 用于渲染 Texture 到 SDL_Windows 中
	sdlRenderer = SDL_CreateRenderer(screen, -1, SDL_RENDERER_TARGETTEXTURE);
	// dlg->GetDlgItem(IDC_STATIC_DISPLAY)->GetClientRect(rcDisplay);
	SDL_RenderSetLogicalSize(sdlRenderer, screen_w, screen_h);

	// 11. 初始化SDL_Texture， 用于显示YUV数据
	// SDL_PIXELFORMAT_IYUV: 格式为 Y + U + V
	// SDL_PIXELFORMAT_YV12: 格式为 Y + V + U
	sdlTexture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, screen_w, screen_h);

	sdlRect.x = 0;
	sdlRect.y = 0;
	sdlRect.w = screen_w;
	sdlRect.h = screen_h;
	int ret = 0;

	video_tid = SDL_CreateThread(sfp_refresh_thread, NULL, NULL);

	//Event Loop
	while (1) {
		//Wait
		SDL_WaitEvent(&event);
		if (event.type == SFM_REFRESH_EVENT) {
			if (thread_pause) {
				// 复制解码后的YUV数据到 p_frame_yuv 中
				sws_scale(video_sws_ctx, (const unsigned char* const*)p_frame->data, p_frame->linesize, 0, video_dec_ctx->height, p_frame_yuv->data, p_frame_yuv->linesize);

				// 更新 YUV数据到 Texture 中
				SDL_UpdateTexture(sdlTexture, NULL, p_frame_yuv->data[0], p_frame_yuv->linesize[0]);

				// 清除 Rendder
				SDL_RenderClear(sdlRenderer);

				// 复制Texture 到 renderer渲染器中
				SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, NULL);

				// 显示
				SDL_RenderPresent(sdlRenderer);
			}
			else {
				// 读取一帧数据
				if (av_read_frame(v_pFormatCtx, p_pkt) >= 0) {
					if (p_pkt->stream_index == st_index[AVMEDIA_TYPE_VIDEO]) {
						// 解码视频帧
						ret = avcodec_send_packet(video_dec_ctx, p_pkt);
						if (ret >= 0) {
							ret = avcodec_receive_frame(video_dec_ctx, p_frame);
							dlg->m_slider.SetPos(p_frame->best_effort_timestamp);
							tns = v_pFormatCtx->duration * p_frame->best_effort_timestamp / video_st->duration / 1000000;
							thh = tns / 3600;
							tmm = (tns % 3600) / 60;
							tss = (tns % 60);
							time_progress.Format(L"%02d:%02d:%02d", thh, tmm, tss);
							dlg->m_progress.SetWindowTextW(time_progress);
							if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN)) {
								ret = 0;
								continue;
							}
							// 复制解码后的YUV数据到 p_frame_yuv 中
							sws_scale(video_sws_ctx, (const unsigned char* const*)p_frame->data, p_frame->linesize, 0, video_dec_ctx->height, p_frame_yuv->data, p_frame_yuv->linesize);

							// 更新 YUV数据到 Texture 中
							SDL_UpdateTexture(sdlTexture, NULL, p_frame_yuv->data[0], p_frame_yuv->linesize[0]);

							// 清除 Rendder
							SDL_RenderClear(sdlRenderer);

							// 复制Texture 到 renderer渲染器中
							SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, NULL);

							// 显示
							SDL_RenderPresent(sdlRenderer);

							// TRACE("Decode 1 frame\n");
						}
						else {
							AfxMessageBox(_T("Decode Error.\n"));
							return -1;
						}
					}
					else if (p_pkt->stream_index == st_index[AVMEDIA_TYPE_AUDIO]) {
						// 解码音频帧
						ret = avcodec_send_packet(audio_dec_ctx, p_pkt);
						if (ret >= 0) {
							ret = avcodec_receive_frame(audio_dec_ctx, p_frame);
							if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
							{
								ret = 0;
								continue;
							}
							//ret = (int)fwrite(p_frame->extended_data[0], 1, p_frame->nb_samples * av_get_bytes_per_sample(audio_dec_ctx->sample_fmt), audio_dst_filename); 

							// cout << "音频：\t第 " << audio_frame_cout++ << " 帧   \tpkt_size = " << p_frame->pkt_size << "  \t\t\taudio_size   = " << p_frame->nb_samples * av_get_bytes_per_sample(audio_dec_ctx->sample_fmt) << endl;
						}
						else {
							AfxMessageBox(_T("Decode Error.\n"));
							return -1;
						}
					}
				}
				else {
					//Exit Thread
					thread_exit = 1;
				}
			}
		}
		else if (event.type == SDL_QUIT) {
			thread_exit = 1;
		}
		else if (event.type == SFM_BREAK_EVENT) {
			break;
		}

	}

	SDL_DestroyTexture(sdlTexture);
	SDL_DestroyRenderer(sdlRenderer);
	SDL_DestroyWindow(screen);
	SDL_Quit();
	//FIX Small Bug
	//SDL Hide Window When it finished
	dlg->GetDlgItem(IDC_STATIC_DISPLAY)->ShowWindow(SW_SHOWNORMAL);
	av_frame_unref(p_frame);
	av_frame_unref(p_frame_yuv);
	av_packet_unref(p_pkt);
	avcodec_close(video_dec_ctx);
	avcodec_close(audio_dec_ctx);
	avformat_close_input(&v_pFormatCtx);

	return 0;
}


//Playing thread
UINT Thread_Play(LPVOID lpParam) {
	CMFCFFmpegDlg* dlg = (CMFCFFmpegDlg*)lpParam;
	dlg->GetDlgItem(IDC_BUTTON_STOP)->EnableWindow(TRUE);
	dlg->GetDlgItem(IDC_BUTTON_PAUSE)->EnableWindow(TRUE);
	dlg->m_slider.EnableWindow(TRUE);
	ffmpegplayer(lpParam);
	dlg->GetDlgItem(IDC_BUTTON_STOP)->EnableWindow(FALSE);
	dlg->GetDlgItem(IDC_BUTTON_PAUSE)->EnableWindow(FALSE);
	dlg->m_slider.SetPos(0);
	dlg->m_slider.EnableWindow(FALSE);
	dlg->m_progress.SetWindowTextW(L"00:00:00");
	dlg->m_duration.SetWindowTextW(L"/ 00:00:00");
	return 0;
}


void CMFCFFmpegDlg::OnBnClickedButtonPlay()
{
	// TODO: Add your control notification handler code here
	pThreadPlay = AfxBeginThread(Thread_Play, this);//Start thread
}


void CMFCFFmpegDlg::OnBnClickedButtonPause()
{
	// TODO: Add your control notification handler code here
	thread_pause = !thread_pause;
	if (thread_pause == 0) {
		GetDlgItem(IDC_BUTTON_PAUSE)->SetWindowTextW(L"Pause");
	}
	else {
		GetDlgItem(IDC_BUTTON_PAUSE)->SetWindowTextW(L"Resume");
	}
}


void CMFCFFmpegDlg::OnBnClickedButtonStop()
{
	// TODO: Add your control notification handler code here
	thread_exit = 1;
}


void CMFCFFmpegDlg::OnBnClickedButtonBrowse()
{
	// TODO: Add your control notification handler code here
	CString FilePathName;
	CFileDialog dlg(TRUE, NULL, NULL, NULL, NULL);///TRUE is OPEN dialog box, FALSE is SAVE AS dialog box 
	if (dlg.DoModal() == IDOK) {
		FilePathName = dlg.GetPathName();
		m_url.SetWindowText(FilePathName);
	}
}


void CMFCFFmpegDlg::OnDropFiles(HDROP hDropInfo)
{
	// TODO: Add your message handler code here and/or call default
	//获取文件路径
	TCHAR szPath[MAX_PATH] = { 0 };
	if (DragQueryFile(hDropInfo, 0, szPath, MAX_PATH) > 0) {
		//显示到控件上
		m_url.SetWindowText(szPath);
	}

	CDialogEx::OnDropFiles(hDropInfo);
}


BOOL CMFCFFmpegDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: Add your specialized code here and/or call the base class
	// 屏蔽Enter关闭窗口
	// if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN)
	// 	return TRUE;
	// 屏蔽ESC关闭窗口
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_ESCAPE)
		return TRUE;

	return CDialogEx::PreTranslateMessage(pMsg);
}


void CMFCFFmpegDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO: Add your message handler code here and/or call default

	CDialogEx::OnHScroll(nSBCode, nPos, pScrollBar);
}
