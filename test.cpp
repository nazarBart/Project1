//C:\Users\Nazar\Downloads\vcpkg - master\vcpkg > .\bootstrap - vcpkg.bat
//git clone https ://github.com/microsoft/vcpkg1 
#include <opencv2\opencv.hpp>
#include <iostream>
extern "C" // *C необходимо использовать для импорта здесь * /
{
#include "libavcodec/avcodec.h"
#include "libavdevice/avdevice.h"
#include "libavformat/avformat.h"
#include "libavfilter/avfilter.h"
#include "libavutil/avutil.h"
#include "libavutil/time.h"
#include "libswscale/swscale.h"
#include "libavutil/pixdesc.h"
}

#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avdevice.lib")
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "avfilter.lib")
#pragma comment(lib, "swscale.lib")
#pragma comment(lib, "swresample.lib")
#pragma comment(lib, "postproc.lib")

using namespace std;
using namespace cv;

void open_rtsp(const char* rtsp)
{
	unsigned int    i;
	int             ret;
	int             video_st_index = -1;
	int             audio_st_index = -1;
	AVFormatContext* ifmt_ctx = NULL;
	AVPacket        pkt;
	AVStream* st = NULL;
	char            errbuf[64];
	AVDictionary* optionsDict = NULL;
	av_register_all();                                                          // Register all codecs and formats so that they can be used.
	avformat_network_init();                                                    // Initialization of network components
	av_dict_set(&optionsDict, "rtsp_transport", "tcp", 0);                // Используя передачу tcp, если это не установлено, некоторые потоки rtsp будут зависать
	av_dict_set(&optionsDict, "stimeout", "2000000", 0);                  // Если stimeout не установлен

	av_init_packet(&pkt);                                                       // initialize packet.
	pkt.data = NULL;
	pkt.size = 0;
	bool nRestart = false;
	AVStream* pVst;
	uint8_t* buffer_rgb = NULL;
	AVCodecContext* pVideoCodecCtx = NULL;
	AVFrame* pFrame = av_frame_alloc();
	AVFrame* pFrameRGB = av_frame_alloc();
	int got_picture;
	SwsContext* img_convert_ctx = NULL;
	AVCodec* pVideoCodec = NULL;

	if ((ret = avformat_open_input(&ifmt_ctx, rtsp, 0, &optionsDict)) < 0) {            // Open the input file for reading.
		printf("Could not open input file '%s' (error '%s')\n", rtsp, av_make_error_string(errbuf, sizeof(errbuf), ret));
		goto EXIT;
	}

	if ((ret = avformat_find_stream_info(ifmt_ctx, NULL)) < 0) {                // Get information on the input file (number of streams etc.).
		printf("Could not open find stream info (error '%s')\n", av_make_error_string(errbuf, sizeof(errbuf), ret));
		goto EXIT;
	}

	for (i = 0; i < ifmt_ctx->nb_streams; i++) {                                // dump information
		av_dump_format(ifmt_ctx, i, rtsp, 0);
	}

	for (i = 0; i < ifmt_ctx->nb_streams; i++) {                                // find video stream index
		st = ifmt_ctx->streams[i];
		switch (st->codec->codec_type) {
		case AVMEDIA_TYPE_AUDIO: audio_st_index = i; break;
		case AVMEDIA_TYPE_VIDEO: video_st_index = i; break;
		default: break;
		}
	}
	if (-1 == video_st_index) {
		printf("No H.264 video stream in the input file\n");
		goto EXIT;
	}
	while (1)
	{
		do {
			ret = av_read_frame(ifmt_ctx, &pkt);                                // read frames
#if 1
			cout << pkt.size << endl;
			//decode stream
			if (!nRestart)
			{
				pVst = ifmt_ctx->streams[video_st_index];
				pVideoCodecCtx = pVst->codec;
				pVideoCodec = avcodec_find_decoder(pVideoCodecCtx->codec_id);
				if (pVideoCodec == NULL)
					return;
				//pVideoCodecCtx = avcodec_alloc_context3(pVideoCodec);

				if (avcodec_open2(pVideoCodecCtx, pVideoCodec, NULL) < 0)
					return;
				nRestart = true;
			}

			if (pkt.stream_index == video_st_index)
			{
				fprintf(stdout, "pkt.size=%d,pkt.pts=%lld, pkt.data=0x%x.", pkt.size, pkt.pts, (unsigned int)pkt.data);
				int av_result = avcodec_decode_video2(pVideoCodecCtx, pFrame, &got_picture, &pkt);

				if (got_picture)
				{
					fprintf(stdout, "decode one video frame!\n");
				}

				if (av_result < 0)
				{
					fprintf(stderr, "decode failed: inputbuf = 0x%x , input_framesize = %d\n", pkt.data, pkt.size);
					return;
				}
#if 1
				if (got_picture)
				{
					int bytes = avpicture_get_size(AV_PIX_FMT_RGB24, pVideoCodecCtx->width, pVideoCodecCtx->height);
					buffer_rgb = (uint8_t*)av_malloc(bytes);
					avpicture_fill((AVPicture*)pFrameRGB, buffer_rgb, AV_PIX_FMT_RGB24, pVideoCodecCtx->width, pVideoCodecCtx->height);

					img_convert_ctx = sws_getContext(pVideoCodecCtx->width, pVideoCodecCtx->height, pVideoCodecCtx->pix_fmt,
						pVideoCodecCtx->width, pVideoCodecCtx->height, AV_PIX_FMT_BGR24, SWS_FAST_BILINEAR, NULL, NULL, NULL);
					if (img_convert_ctx == NULL)
					{

						printf("can't init convert context!\n");
						return;
					}
					sws_scale(img_convert_ctx, pFrame->data, pFrame->linesize, 0, pVideoCodecCtx->height, pFrameRGB->data, pFrameRGB->linesize);
					IplImage* pRgbImg = cvCreateImage(cvSize(pVideoCodecCtx->width, pVideoCodecCtx->height), 8, 3);
					memcpy(pRgbImg->imageData, buffer_rgb, pVideoCodecCtx->width * 3 * pVideoCodecCtx->height);
					//Mat Img = cvarrToMat(pRgbImg, true);
					cvShowImage("GB28181Show", pRgbImg);
					//DetectFace(Img);
					cvWaitKey(10);
					cvReleaseImage(&pRgbImg);
					sws_freeContext(img_convert_ctx);
					av_free(buffer_rgb);
				}
#endif
			}

#endif
		} while (ret == AVERROR(EAGAIN));

		if (ret < 0) {
			printf("Could not read frame ---(error '%s')\n", av_make_error_string(errbuf, sizeof(errbuf), ret));
			goto EXIT;
		}

		if (pkt.stream_index == video_st_index) {                               // video frame
			printf("Video Packet size = %d\n", pkt.size);
		}
		else if (pkt.stream_index == audio_st_index) {                         // audio frame
			printf("Audio Packet size = %d\n", pkt.size);
		}
		else {
			printf("Unknow Packet size = %d\n", pkt.size);
		}
		av_packet_unref(&pkt);
	}

EXIT:

	if (NULL != ifmt_ctx) {
		avformat_close_input(&ifmt_ctx);
		ifmt_ctx = NULL;
	}

	return;
}

int main(int argc, char* argv[])
{
	while (1)
		open_rtsp("rtsp://192.168.1.86");
	return 0;
}

