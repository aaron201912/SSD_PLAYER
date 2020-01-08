/*******************************************************************************
 * player.c
 *
 * history:
 *   2018-11-27 - [lei]     Create file: a simplest ffmpeg player
 *   2018-12-01 - [lei]     Playing audio
 *   2018-12-06 - [lei]     Playing audio&vidio
 *   2019-01-06 - [lei]     Add audio resampling, fix bug of unsupported audio 
 *                          format(such as planar)
 *   2019-01-16 - [lei]     Sync video to audio.
 *
 * details:
 *   A simple ffmpeg player.
 *
 * refrence:
 *   ffplay.c in FFmpeg 4.1 project.
 *******************************************************************************/
#ifdef SUPPORT_PLAYER_MODULE
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

#include "player.h"
#include "frame.h"
#include "packet.h"
#include "demux.h"
#include "videostream.h"
#include "audiostream.h"

#include <dlfcn.h>

AVPacket a_flush_pkt, v_flush_pkt;
static int av_sync_type = AV_SYNC_AUDIO_MASTER;
static float seek_interval = 10;

//ffmpeg lib
AvCodec_LibInfo_t AvCodecLibInfo;
AvFormat_LibInfo_t AvFormatLibInfo;
AvUtil_LibInfo_t AvUtilLibInfo;
Swr_LibInfo_t SwrLibInfo;
Sws_LibInfo_t SwsLibInfo;


//player_stat_t *player_init(const char *p_input_file);
//int player_deinit(player_stat_t *is);
static int get_master_sync_type(player_stat_t *is) {
    if (is->av_sync_type == AV_SYNC_VIDEO_MASTER) {
        if (is->p_video_stream)
            return AV_SYNC_VIDEO_MASTER;
        else
            return AV_SYNC_AUDIO_MASTER;
    } else if (is->av_sync_type == AV_SYNC_AUDIO_MASTER) {
        if (is->p_audio_stream)
            return AV_SYNC_AUDIO_MASTER;
        else
            return AV_SYNC_EXTERNAL_CLOCK;
    } else {
        return AV_SYNC_EXTERNAL_CLOCK;
    }
}


/* get the current master clock value */
static double get_master_clock(player_stat_t *is)
{
    double val;

    switch (get_master_sync_type(is)) {
        case AV_SYNC_VIDEO_MASTER:
            val = get_clock(&is->video_clk);
            break;
        case AV_SYNC_AUDIO_MASTER:
            val = get_clock(&is->audio_clk);
            break;
        default:
            val = get_clock(&is->extclk);
            break;
    }
    return val;
}


// 返回值：返回上一帧的pts更新值(上一帧pts+流逝的时间)
double get_clock(play_clock_t *c)
{
    if (*c->queue_serial != c->serial)
    {
        return NAN;
    }
    if (c->paused)
    {
        return c->pts;
    }
    else
    {
        double time = AvUtilLibInfo.av_gettime_relative() / 1000000.0;
        double ret = c->pts_drift + time;   // 展开得： c->pts + (time - c->last_updated)
        return ret;
    }
}

void set_clock_at(play_clock_t *c, double pts, int serial, double time)
{
    c->pts = pts;
    c->last_updated = time;
    c->pts_drift = c->pts - time;
    c->serial = serial;
}

void set_clock(play_clock_t *c, double pts, int serial)
{
    double time = AvUtilLibInfo.av_gettime_relative() / 1000000.0;
    set_clock_at(c, pts, serial, time);
}

static void set_clock_speed(play_clock_t *c, double speed)
{
    set_clock(c, get_clock(c), c->serial);
    c->speed = speed;
}

void init_clock(play_clock_t *c, int *queue_serial)
{
    c->speed = 1.0;
    c->paused = 0;
    c->queue_serial = queue_serial;
    set_clock(c, NAN, -1);
}

static void sync_play_clock_to_slave(play_clock_t *c, play_clock_t *slave)
{
    double clock = get_clock(c);
    double slave_clock = get_clock(slave);
    if (!isnan(slave_clock) && (isnan(clock) || fabs(clock - slave_clock) > AV_NOSYNC_THRESHOLD))
        set_clock(c, slave_clock, slave->serial);
}

static void audio_decoder_abort(player_stat_t *is)
{
    packet_queue_abort(&is->audio_pkt_queue);
    frame_queue_signal(&is->audio_frm_queue);

    pthread_join(is->audioDecode_tid, NULL);
    pthread_join(is->audioPlay_tid, NULL);

    packet_queue_flush(&is->audio_pkt_queue);
    printf("audio packet flush done!\n");
    AvCodecLibInfo.avcodec_free_context(&is->p_acodec_ctx);
}

static void video_decoder_abort(player_stat_t *is)
{
    packet_queue_abort(&is->video_pkt_queue);
    frame_queue_signal(&is->video_frm_queue);
    #if (!ENABLE_SMALL)
    pthread_join(is->videoDecode_tid, NULL);
    #endif
    pthread_join(is->videoPlay_tid, NULL);

    packet_queue_flush(&is->video_pkt_queue);
    printf("video packet flush done!\n");
    AvCodecLibInfo.avcodec_free_context(&is->p_vcodec_ctx);
}

static void stream_component_close(player_stat_t *is, int stream_index)
{
    AVFormatContext *ic = is->p_fmt_ctx;
    AVCodecParameters *codecpar;
    printf("stream_index: %d,nb_streams: %d\n",stream_index,ic->nb_streams);
    if (stream_index < 0 || stream_index >= ic->nb_streams)
        return;
    codecpar = ic->streams[stream_index]->codecpar;

    switch (codecpar->codec_type) {
    case AVMEDIA_TYPE_AUDIO:
        audio_decoder_abort(is);
        SwrLibInfo.swr_free(&is->audio_swr_ctx);
        AvUtilLibInfo.av_freep(&is->p_audio_frm);
        is->audio_frm_size = 0;
        is->audio_frm_rwr_size = 0;
        is->audio_frm_rwr = NULL;
        break;
    case AVMEDIA_TYPE_VIDEO:
        video_decoder_abort(is);
        break;

    default:
        break;
    }

    ic->streams[stream_index]->discard = AVDISCARD_ALL;
    switch (codecpar->codec_type) {
    case AVMEDIA_TYPE_AUDIO:
        is->p_audio_stream = NULL;
        is->audio_idx = -1;
        break;
    case AVMEDIA_TYPE_VIDEO:
        is->p_video_stream = NULL;
        is->video_idx = -1;
        break;
    default:
        break;
    }
}

static void* idle_thread(void *arg)
{
    player_stat_t *is = (player_stat_t *)arg;

    printf("get in idle pthread!\n");

    while(is)
    {
        if(is->abort_request)
        {
            break;
        }

        AvUtilLibInfo.av_usleep((unsigned)(50 * 1000));   //阻塞50ms

        if (is->play_error < 0)
        {
            if (is->playerController.fpPlayError)
            {
                is->playerController.fpPlayError(is->play_error);
                if (is)
                    is->play_error = 0;
            }
        }
        else
        {
            if (is->audio_complete && is->video_complete)
            {
                if (is->playerController.fpPlayComplete)
                {
                    is->playerController.fpPlayComplete();
                    break;
                }
            }
        }
    }
    
    printf("idle pthread exit\n");

    return NULL;
}

bool CloseFFmpegLib(void)
{
    printf("CloseFFmpegLib\n");
    if(AvCodecLibInfo.pAvCodecLibHandle)
    {
        dlclose(AvCodecLibInfo.pAvCodecLibHandle);
        AvCodecLibInfo.pAvCodecLibHandle = NULL;
    }
    memset(&AvCodecLibInfo, 0, sizeof(AvCodecLibInfo));
    printf("11\n");
    if(AvFormatLibInfo.pAvFormatLibHandle)
    {
        dlclose(AvFormatLibInfo.pAvFormatLibHandle);
        AvFormatLibInfo.pAvFormatLibHandle = NULL;
    }
    memset(&AvFormatLibInfo, 0, sizeof(AvFormatLibInfo));

    if(AvUtilLibInfo.pAvUtilLibHandle)
    {
        dlclose(AvUtilLibInfo.pAvUtilLibHandle);
        AvUtilLibInfo.pAvUtilLibHandle = NULL;
    }
    memset(&AvUtilLibInfo, 0, sizeof(AvUtilLibInfo));

    if(SwrLibInfo.pSwrLibHandle)
    {
        dlclose(SwrLibInfo.pSwrLibHandle);
        SwrLibInfo.pSwrLibHandle = NULL;
    }
    memset(&SwrLibInfo, 0, sizeof(SwrLibInfo));

    if(SwsLibInfo.pSwsLibHandle)
    {
        dlclose(SwsLibInfo.pSwsLibHandle);
        SwsLibInfo.pSwsLibHandle = NULL;
    }
    memset(&SwsLibInfo, 0, sizeof(SwsLibInfo));
    
    printf("CloseFFmpegLib finished\n");
    
    return 0;
}


bool OpenFFmpegLib(void)
{
    printf("OpenFFmpegLib\n");
    //open libavcodec
    AvCodecLibInfo.pAvCodecLibHandle = dlopen("libavcodec.so", RTLD_NOW);
    if(NULL == AvCodecLibInfo.pAvCodecLibHandle)
    {
        printf(" %s: Can not load libavcodec.so!\n", __func__);
        return FALSE;
    }

    AvCodecLibInfo.avcodec_alloc_context3 = (AVCodecContext *(*)(const AVCodec *codec))dlsym(AvCodecLibInfo.pAvCodecLibHandle, "avcodec_alloc_context3");
    if(NULL == AvCodecLibInfo.avcodec_alloc_context3)
    {
        printf(" %s: dlsym avcodec_alloc_context3 failed, %s\n", __func__, dlerror());
        return FALSE;
    }

    AvCodecLibInfo.avcodec_find_decoder_by_name = (AVCodec *(*)(const char *name))dlsym(AvCodecLibInfo.pAvCodecLibHandle, "avcodec_find_decoder_by_name");
    if(NULL == AvCodecLibInfo.avcodec_find_decoder_by_name)
    {
        printf(" %s: dlsym avcodec_find_decoder_by_name failed, %s\n", __func__, dlerror());
        return FALSE;
    }

    AvCodecLibInfo.avcodec_find_decoder = (AVCodec *(*)(enum AVCodecID id))dlsym(AvCodecLibInfo.pAvCodecLibHandle, "avcodec_find_decoder");
    if(NULL == AvCodecLibInfo.avcodec_find_decoder)
    {
        printf(" %s: dlsym avcodec_find_decoder failed, %s\n", __func__, dlerror());
        return FALSE;
    }

    AvCodecLibInfo.avcodec_flush_buffers = (void (*)(AVCodecContext *avctx))dlsym(AvCodecLibInfo.pAvCodecLibHandle, "avcodec_flush_buffers");
    if(NULL == AvCodecLibInfo.avcodec_flush_buffers)
    {
        printf(" %s: dlsym avcodec_flush_buffers failed, %s\n", __func__, dlerror());
        return FALSE;
    }

    AvCodecLibInfo.avcodec_free_context = (void (*)(AVCodecContext **avctx))dlsym(AvCodecLibInfo.pAvCodecLibHandle, "avcodec_free_context");
    if(NULL == AvCodecLibInfo.avcodec_free_context)
    {
        printf(" %s: dlsym avcodec_free_context failed, %s\n", __func__, dlerror());
        return FALSE;
    }

    AvCodecLibInfo.avcodec_open2 = (int (*)(AVCodecContext *avctx, const AVCodec *codec, AVDictionary **options))dlsym(AvCodecLibInfo.pAvCodecLibHandle, "avcodec_open2");
    if(NULL == AvCodecLibInfo.avcodec_open2)
    {
        printf(" %s: dlsym avcodec_open2 failed, %s\n", __func__, dlerror());
        return FALSE;
    }

    AvCodecLibInfo.avcodec_parameters_to_context = (int (*)(AVCodecContext *codec, const AVCodecParameters *par))dlsym(AvCodecLibInfo.pAvCodecLibHandle, "avcodec_parameters_to_context");
    if(NULL == AvCodecLibInfo.avcodec_parameters_to_context)
    {
        printf(" %s: dlsym avcodec_parameters_to_context failed, %s\n", __func__, dlerror());
        return FALSE;
    }

    AvCodecLibInfo.avcodec_receive_frame = (int (*)(AVCodecContext *avctx, AVFrame *frame))dlsym(AvCodecLibInfo.pAvCodecLibHandle, "avcodec_receive_frame");
    if(NULL == AvCodecLibInfo.avcodec_receive_frame)
    {
        printf(" %s: dlsym avcodec_receive_frame failed, %s\n", __func__, dlerror());
        return FALSE;
    }

    AvCodecLibInfo.avcodec_send_packet = (int (*)(AVCodecContext *avctx, const AVPacket *avpkt))dlsym(AvCodecLibInfo.pAvCodecLibHandle, "avcodec_send_packet");
    if(NULL == AvCodecLibInfo.avcodec_send_packet)
    {
        printf(" %s: dlsym avcodec_send_packet failed, %s\n", __func__, dlerror());
        return FALSE;
    }

    AvCodecLibInfo.av_init_packet = (void (*)(AVPacket *pkt))dlsym(AvCodecLibInfo.pAvCodecLibHandle, "av_init_packet");
    if(NULL == AvCodecLibInfo.av_init_packet)
    {
        printf(" %s: dlsym av_init_packet failed, %s\n", __func__, dlerror());
        return FALSE;
    }

    AvCodecLibInfo.av_packet_make_refcounted = (int (*)(AVPacket *pkt))dlsym(AvCodecLibInfo.pAvCodecLibHandle, "av_packet_make_refcounted");
    if(NULL == AvCodecLibInfo.av_packet_make_refcounted)
    {
        printf(" %s: dlsym av_packet_make_refcounted failed, %s\n", __func__, dlerror());
        return FALSE;
    }

    AvCodecLibInfo.av_packet_unref = (void (*)(AVPacket *pkt))dlsym(AvCodecLibInfo.pAvCodecLibHandle, "av_packet_unref");
    if(NULL == AvCodecLibInfo.av_packet_unref)
    {
        printf(" %s: dlsym av_packet_unref failed, %s\n", __func__, dlerror());
        return FALSE;
    }

    //open libavformat
    AvFormatLibInfo.pAvFormatLibHandle = dlopen("libavformat.so", RTLD_NOW);
    if(NULL == AvFormatLibInfo.pAvFormatLibHandle)
    {
        printf(" %s: Can not load libavformat.so!\n", __func__);
        return FALSE;
    }

    AvFormatLibInfo.avformat_alloc_context = (AVFormatContext *(*)(void))dlsym(AvFormatLibInfo.pAvFormatLibHandle, "avformat_alloc_context");
    if(NULL == AvFormatLibInfo.avformat_alloc_context)
    {
        printf(" %s: dlsym avformat_alloc_context failed, %s\n", __func__, dlerror());
        return FALSE;
    }

    AvFormatLibInfo.avformat_close_input = (void (*)(AVFormatContext **s))dlsym(AvFormatLibInfo.pAvFormatLibHandle, "avformat_close_input");
    if(NULL == AvFormatLibInfo.avformat_close_input)
    {
        printf(" %s: dlsym avformat_close_input failed, %s\n", __func__, dlerror());
        return FALSE;
    }

    AvFormatLibInfo.avformat_find_stream_info = (int (*)(AVFormatContext *ic, AVDictionary **options))dlsym(AvFormatLibInfo.pAvFormatLibHandle, "avformat_find_stream_info");
    if(NULL == AvFormatLibInfo.avformat_find_stream_info)
    {
        printf(" %s: dlsym avformat_find_stream_info failed, %s\n", __func__, dlerror());
        return FALSE;
    }

    AvFormatLibInfo.avformat_open_input = (int (*)(AVFormatContext **ps, const char *url, AVInputFormat *fmt, AVDictionary **options))dlsym(AvFormatLibInfo.pAvFormatLibHandle, "avformat_open_input");
    if(NULL == AvFormatLibInfo.avformat_open_input)
    {
        printf(" %s: dlsym avformat_open_input failed, %s\n", __func__, dlerror());
        return FALSE;
    }

    AvFormatLibInfo.avformat_seek_file = (int (*)(AVFormatContext *s, int stream_index, int64_t min_ts, int64_t ts, int64_t max_ts, int flags))dlsym(AvFormatLibInfo.pAvFormatLibHandle, "avformat_seek_file");
    if(NULL == AvFormatLibInfo.avformat_seek_file)
    {
        printf(" %s: dlsym avformat_seek_file failed, %s\n", __func__, dlerror());
        return FALSE;
    }

    AvFormatLibInfo.av_guess_frame_rate = (AVRational (*)(AVFormatContext *ctx, AVStream *stream, AVFrame *frame))dlsym(AvFormatLibInfo.pAvFormatLibHandle, "av_guess_frame_rate");
    if(NULL == AvFormatLibInfo.av_guess_frame_rate)
    {
        printf(" %s: dlsym av_guess_frame_rate failed, %s\n", __func__, dlerror());
        return FALSE;
    }

    AvFormatLibInfo.avio_feof = (int (*)(AVIOContext *s))dlsym(AvFormatLibInfo.pAvFormatLibHandle, "avio_feof");
    if(NULL == AvFormatLibInfo.avio_feof)
    {
        printf(" %s: dlsym avio_feof failed, %s\n", __func__, dlerror());
        return FALSE;
    }

    AvFormatLibInfo.av_read_frame = (int (*)(AVFormatContext *s, AVPacket *pkt))dlsym(AvFormatLibInfo.pAvFormatLibHandle, "av_read_frame");
    if(NULL == AvFormatLibInfo.av_read_frame)
    {
        printf(" %s: dlsym av_read_frame failed, %s\n", __func__, dlerror());
        return FALSE;
    }

    AvFormatLibInfo.av_read_pause = (int (*)(AVFormatContext *s))dlsym(AvFormatLibInfo.pAvFormatLibHandle, "av_read_pause");
    if(NULL == AvFormatLibInfo.av_read_pause)
    {
        printf(" %s: dlsym av_read_pause failed, %s\n", __func__, dlerror());
        return FALSE;
    }

    AvFormatLibInfo.av_read_play = (int (*)(AVFormatContext *s))dlsym(AvFormatLibInfo.pAvFormatLibHandle, "av_read_play");
    if(NULL == AvFormatLibInfo.av_read_play)
    {
        printf(" %s: dlsym av_read_play failed, %s\n", __func__, dlerror());
        return FALSE;
    }

    //open libavutil.so
    AvUtilLibInfo.pAvUtilLibHandle = dlopen("libavutil.so", RTLD_NOW);
    if(NULL == AvUtilLibInfo.pAvUtilLibHandle)
    {
        printf(" %s: Can not load libavutil.so!\n", __func__);
        return FALSE;
    }

    AvUtilLibInfo.av_fast_mallocz = (void (*)(void *ptr, unsigned int *size, size_t min_size))dlsym(AvUtilLibInfo.pAvUtilLibHandle, "av_fast_mallocz");
    if(NULL == AvUtilLibInfo.av_fast_mallocz)
    {
        printf(" %s: dlsym av_fast_mallocz failed, %s\n", __func__, dlerror());
        return FALSE;
    }

    AvUtilLibInfo.av_frame_alloc = (AVFrame *(*)(void))dlsym(AvUtilLibInfo.pAvUtilLibHandle, "av_frame_alloc");
    if(NULL == AvUtilLibInfo.av_frame_alloc)
    {
        printf(" %s: dlsym av_frame_alloc failed, %s\n", __func__, dlerror());
        return FALSE;
    }

    AvUtilLibInfo.av_frame_free = (void (*)(AVFrame **frame))dlsym(AvUtilLibInfo.pAvUtilLibHandle, "av_frame_free");
    if(NULL == AvUtilLibInfo.av_frame_free)
    {
        printf(" %s: dlsym av_frame_free failed, %s\n", __func__, dlerror());
        return FALSE;
    }

    AvUtilLibInfo.av_frame_move_ref = (void (*)(AVFrame *dst, AVFrame *src))dlsym(AvUtilLibInfo.pAvUtilLibHandle, "av_frame_move_ref");
    if(NULL == AvUtilLibInfo.av_frame_move_ref)
    {
        printf(" %s: dlsym av_frame_move_ref failed, %s\n", __func__, dlerror());
        return FALSE;
    }

    AvUtilLibInfo.av_frame_unref = (void (*)(AVFrame *frame))dlsym(AvUtilLibInfo.pAvUtilLibHandle, "av_frame_unref");
    if(NULL == AvUtilLibInfo.av_frame_unref)
    {
        printf(" %s: dlsym av_frame_unref failed, %s\n", __func__, dlerror());
        return FALSE;
    }

    AvUtilLibInfo.av_free = (void (*)(void *ptr))dlsym(AvUtilLibInfo.pAvUtilLibHandle, "av_free");
    if(NULL == AvUtilLibInfo.av_free)
    {
        printf(" %s: dlsym av_free failed, %s\n", __func__, dlerror());
        return FALSE;
    }

    AvUtilLibInfo.av_freep = (void (*)(void *ptr))dlsym(AvUtilLibInfo.pAvUtilLibHandle, "av_freep");
    if(NULL == AvUtilLibInfo.av_freep)
    {
        printf(" %s: dlsym av_freep failed, %s\n", __func__, dlerror());
        return FALSE;
    }

    AvUtilLibInfo.av_get_bytes_per_sample = (int (*)(enum AVSampleFormat sample_fmt))dlsym(AvUtilLibInfo.pAvUtilLibHandle, "av_get_bytes_per_sample");
    if(NULL == AvUtilLibInfo.av_get_bytes_per_sample)
    {
        printf(" %s: dlsym av_get_bytes_per_sample failed, %s\n", __func__, dlerror());
        return FALSE;
    }

    AvUtilLibInfo.av_get_channel_layout_nb_channels = (int (*)(uint64_t channel_layout))dlsym(AvUtilLibInfo.pAvUtilLibHandle, "av_get_channel_layout_nb_channels");
    if(NULL == AvUtilLibInfo.av_get_channel_layout_nb_channels)
    {
        printf(" %s: dlsym av_get_channel_layout_nb_channels failed, %s\n", __func__, dlerror());
        return FALSE;
    }

    AvUtilLibInfo.av_get_default_channel_layout = (int64_t (*)(int nb_channels))dlsym(AvUtilLibInfo.pAvUtilLibHandle, "av_get_default_channel_layout");
    if(NULL == AvUtilLibInfo.av_get_default_channel_layout)
    {
        printf(" %s: dlsym av_get_default_channel_layout failed, %s\n", __func__, dlerror());
        return FALSE;
    }

    AvUtilLibInfo.av_get_sample_fmt_name = (const char *(*)(enum AVSampleFormat sample_fmt))dlsym(AvUtilLibInfo.pAvUtilLibHandle, "av_get_sample_fmt_name");
    if(NULL == AvUtilLibInfo.av_get_sample_fmt_name)
    {
        printf(" %s: dlsym av_get_sample_fmt_name failed, %s\n", __func__, dlerror());
        return FALSE;
    }

    AvUtilLibInfo.av_gettime_relative = (int64_t (*)(void))dlsym(AvUtilLibInfo.pAvUtilLibHandle, "av_gettime_relative");
    if(NULL == AvUtilLibInfo.av_gettime_relative)
    {
        printf(" %s: dlsym av_gettime_relative failed, %s\n", __func__, dlerror());
        return FALSE;
    }

    AvUtilLibInfo.av_image_fill_arrays = (int (*)(uint8_t *dst_data[4], int dst_linesize[4], const uint8_t *src, enum AVPixelFormat pix_fmt, int width, int height, int align))dlsym(AvUtilLibInfo.pAvUtilLibHandle, "av_image_fill_arrays");
    if(NULL == AvUtilLibInfo.av_image_fill_arrays)
    {
        printf(" %s: dlsym av_image_fill_arrays failed, %s\n", __func__, dlerror());
        return FALSE;
    }

    AvUtilLibInfo.av_image_get_buffer_size = (int (*)(enum AVPixelFormat pix_fmt, int width, int height, int align))dlsym(AvUtilLibInfo.pAvUtilLibHandle, "av_image_get_buffer_size");
    if(NULL == AvUtilLibInfo.av_image_get_buffer_size)
    {
        printf(" %s: dlsym av_image_get_buffer_size failed, %s\n", __func__, dlerror());
        return FALSE;
    }

    AvUtilLibInfo.av_log = (void (*)(void *avcl, int level, const char *fmt, ...))dlsym(AvUtilLibInfo.pAvUtilLibHandle, "av_log");
    if(NULL == AvUtilLibInfo.av_log)
    {
        printf(" %s: dlsym av_log failed, %s\n", __func__, dlerror());
        return FALSE;
    }

    AvUtilLibInfo.av_malloc = (void *(*)(size_t size) )dlsym(AvUtilLibInfo.pAvUtilLibHandle, "av_malloc");
    if(NULL == AvUtilLibInfo.av_malloc)
    {
        printf(" %s: dlsym av_malloc failed, %s\n", __func__, dlerror());
        return FALSE;
    }

    AvUtilLibInfo.av_mallocz = (void *(*)(size_t size) )dlsym(AvUtilLibInfo.pAvUtilLibHandle, "av_mallocz");
    if(NULL == AvUtilLibInfo.av_mallocz)
    {
        printf(" %s: dlsym av_mallocz failed, %s\n", __func__, dlerror());
        return FALSE;
    }

    AvUtilLibInfo.av_pix_fmt_desc_get = (const AVPixFmtDescriptor *(*)(enum AVPixelFormat pix_fmt))dlsym(AvUtilLibInfo.pAvUtilLibHandle, "av_pix_fmt_desc_get");
    if(NULL == AvUtilLibInfo.av_pix_fmt_desc_get)
    {
        printf(" %s: dlsym av_pix_fmt_desc_get failed, %s\n", __func__, dlerror());
        return FALSE;
    }

    AvUtilLibInfo.av_rescale_q = (int64_t (*)(int64_t a, AVRational bq, AVRational cq) )dlsym(AvUtilLibInfo.pAvUtilLibHandle, "av_rescale_q");
    if(NULL == AvUtilLibInfo.av_rescale_q)
    {
        printf(" %s: dlsym av_rescale_q failed, %s\n", __func__, dlerror());
        return FALSE;
    }

    AvUtilLibInfo.av_samples_get_buffer_size = (int (*)(int *linesize, int nb_channels, int nb_samples,enum AVSampleFormat sample_fmt, int align))dlsym(AvUtilLibInfo.pAvUtilLibHandle, "av_samples_get_buffer_size");
    if(NULL == AvUtilLibInfo.av_samples_get_buffer_size)
    {
        printf(" %s: dlsym av_samples_get_buffer_size failed, %s\n", __func__, dlerror());
        return FALSE;
    }

    AvUtilLibInfo.av_strdup = (char *(*)(const char *s) )dlsym(AvUtilLibInfo.pAvUtilLibHandle, "av_strdup");
    if(NULL == AvUtilLibInfo.av_strdup)
    {
        printf(" %s: dlsym av_strdup failed, %s\n", __func__, dlerror());
        return FALSE;
    }

    AvUtilLibInfo.av_usleep = (int (*)(unsigned usec))dlsym(AvUtilLibInfo.pAvUtilLibHandle, "av_usleep");
    if(NULL == AvUtilLibInfo.av_usleep)
    {
        printf(" %s: dlsym av_usleep failed, %s\n", __func__, dlerror());
        return FALSE;
    }

    //open libswresample.so
    SwrLibInfo.pSwrLibHandle = dlopen("libswresample.so", RTLD_NOW);
    if(NULL == SwrLibInfo.pSwrLibHandle)
    {
        printf(" %s: Can not load libswresample.so!\n", __func__);
        return FALSE;
    }

    SwrLibInfo.swr_alloc_set_opts = (struct SwrContext *(*)(struct SwrContext *s, \
                                          int64_t out_ch_layout, enum AVSampleFormat out_sample_fmt, int out_sample_rate, \
                                          int64_t  in_ch_layout, enum AVSampleFormat  in_sample_fmt, int  in_sample_rate, \
                                          int log_offset, void *log_ctx))dlsym(SwrLibInfo.pSwrLibHandle, "swr_alloc_set_opts");
    if(NULL == SwrLibInfo.swr_alloc_set_opts)
    {
        printf(" %s: dlsym swr_alloc_set_opts failed, %s\n", __func__, dlerror());
        return FALSE;
    }

    SwrLibInfo.swr_convert = (int (*)(struct SwrContext *s, uint8_t **out, int out_count,const uint8_t **in , int in_count))dlsym(SwrLibInfo.pSwrLibHandle, "swr_convert");
    if(NULL == SwrLibInfo.swr_convert)
    {
        printf(" %s: dlsym swr_convert failed, %s\n", __func__, dlerror());
        return FALSE;
    }

    SwrLibInfo.swr_free = (void (*)(struct SwrContext **s))dlsym(SwrLibInfo.pSwrLibHandle, "swr_free");
    if(NULL == SwrLibInfo.swr_free)
    {
        printf(" %s: dlsym swr_free failed, %s\n", __func__, dlerror());
        return FALSE;
    }

    SwrLibInfo.swr_init = (int (*)(struct SwrContext *s))dlsym(SwrLibInfo.pSwrLibHandle, "swr_init");
    if(NULL == SwrLibInfo.swr_init)
    {
        printf(" %s: dlsym swr_init failed, %s\n", __func__, dlerror());
        return FALSE;
    }

    //open libswresample.so
    SwsLibInfo.pSwsLibHandle = dlopen("libswscale.so", RTLD_NOW);
    if(NULL == SwsLibInfo.pSwsLibHandle)
    {
        printf(" %s: Can not load libswscale.so!\n", __func__);
        return FALSE;
    }

    SwsLibInfo.sws_freeContext = (void (*)(struct SwsContext *swsContext))dlsym(SwsLibInfo.pSwsLibHandle, "sws_freeContext");
    if(NULL == SwsLibInfo.sws_freeContext)
    {
        printf(" %s: dlsym sws_freeContext failed, %s\n", __func__, dlerror());
        return FALSE;
    }

    SwsLibInfo.sws_getContext = ( struct SwsContext *(*)(int srcW, int srcH, enum AVPixelFormat srcFormat, \
                                  int dstW, int dstH, enum AVPixelFormat dstFormat, \
                                  int flags, SwsFilter *srcFilter, \
                                  SwsFilter *dstFilter, const double *param))dlsym(SwsLibInfo.pSwsLibHandle, "sws_getContext");
    if(NULL == SwsLibInfo.sws_getContext)
    {
        printf(" %s: dlsym sws_getContext failed, %s\n", __func__, dlerror());
        return FALSE;
    }

    SwsLibInfo.sws_scale = (int (*)(struct SwsContext *c, const uint8_t *const srcSlice[], \
              const int srcStride[], int srcSliceY, int srcSliceH, \
              uint8_t *const dst[], const int dstStride[]))dlsym(SwsLibInfo.pSwsLibHandle, "sws_scale");
    if(NULL == SwsLibInfo.sws_scale)
    {
        printf(" %s: dlsym sws_scale failed, %s\n", __func__, dlerror());
        return FALSE;
    }
    
    return 0;
}

player_stat_t *player_init(const char *p_input_file)
{
    player_stat_t *is;

    //open ffmpeg lib
    OpenFFmpegLib();
    
    printf("player_init start\n");
    is = (player_stat_t *)AvUtilLibInfo.av_mallocz(sizeof(player_stat_t));
    if (!is)
    {
    	printf("av_mallocz null\n");
        return NULL;
    }
    printf("av_mallocz success\n");
    is->filename = AvUtilLibInfo.av_strdup(p_input_file);
    if (is->filename == NULL)
    {
        goto fail;
    }

    /* start video display */
    if (frame_queue_init(&is->video_frm_queue, &is->video_pkt_queue, VIDEO_PICTURE_QUEUE_SIZE, 1) < 0 ||
        frame_queue_init(&is->audio_frm_queue, &is->audio_pkt_queue, SAMPLE_QUEUE_SIZE, 1) < 0)
    {
        goto fail;
    }

    if (packet_queue_init(&is->video_pkt_queue) < 0 ||
        packet_queue_init(&is->audio_pkt_queue) < 0)
    {
        goto fail;
    }

    AvCodecLibInfo.av_init_packet(&a_flush_pkt);
    AvCodecLibInfo.av_init_packet(&v_flush_pkt);
    a_flush_pkt.data = (uint8_t *)&a_flush_pkt;
    v_flush_pkt.data = (uint8_t *)&v_flush_pkt;
    
    packet_queue_put(&is->video_pkt_queue, &a_flush_pkt);
    packet_queue_put(&is->audio_pkt_queue, &v_flush_pkt);

    if (pthread_cond_init(&is->continue_read_thread,NULL) != SUCCESS)
    {
        printf("[%s %d]exec function failed\n", __FUNCTION__, __LINE__);
        goto fail;
    }

    init_clock(&is->video_clk, &is->video_pkt_queue.serial);
    init_clock(&is->audio_clk, &is->audio_pkt_queue.serial);

    is->abort_request = 0;
    is->p_frm_yuv = AvUtilLibInfo.av_frame_alloc();
    if (is->p_frm_yuv == NULL)
    {
        printf("av_frame_alloc() for p_frm_raw failed\n");
        goto fail;
    }
    is->av_sync_type = av_sync_type;
    is->play_error = 0;

    pthread_create(&is->idle_tid, NULL, idle_thread, is);

    return is;
fail:
    AvUtilLibInfo.av_free(is->filename);
    frame_queue_destory(&is->video_frm_queue);
    frame_queue_destory(&is->audio_frm_queue);
    packet_queue_flush(&is->video_pkt_queue);
    packet_queue_flush(&is->audio_pkt_queue);
    AvUtilLibInfo.av_frame_free(&is->p_frm_yuv);
    AvUtilLibInfo.av_freep(&is);

    return NULL;
}


int player_deinit(player_stat_t *is)
{
    printf("player_deinit start\n");
    
    /* XXX: use a special url_shutdown call to abort parse cleanly */
    is->abort_request = 1;
    
    // 如果运行期间没有错误或者解码速度不够,按正常流程退出
    if (!is->play_error || is->play_error == -3)
    {
        pthread_join(is->read_tid, NULL);

        /* close each stream */
        if (is->audio_idx >= 0)
        {
            stream_component_close(is, is->audio_idx);
            printf("audio stream_component_close finish\n");
        }
        if (is->video_idx >= 0)
        {
            stream_component_close(is, is->video_idx);
            printf("video stream_component_close finish\n");
        }

        AvFormatLibInfo.avformat_close_input(&is->p_fmt_ctx);

        SwsLibInfo.sws_freeContext(is->img_convert_ctx);
    }
    /* free all pictures */
    packet_queue_destroy(&is->video_pkt_queue);
    
    packet_queue_destroy(&is->audio_pkt_queue);

    frame_queue_destory(&is->video_frm_queue);

    frame_queue_destory(&is->audio_frm_queue);

    pthread_cond_destroy(&is->continue_read_thread);

    AvUtilLibInfo.av_frame_free(&is->p_frm_yuv);

    AvUtilLibInfo.av_free(is->filename);

    AvUtilLibInfo.av_freep(&is);
    
    CloseFFmpegLib();

    return 0;
}

/* pause or resume the video */
void stream_toggle_pause(player_stat_t *is)
{
    if (is->paused)
    {
        // 这里表示当前是暂停状态，将切换到继续播放状态。在继续播放之前，先将暂停期间流逝的时间加到frame_timer中
        is->frame_timer += AvUtilLibInfo.av_gettime_relative() / 1000000.0 - is->video_clk.last_updated;
        if (is->read_pause_return != AVERROR(ENOSYS)) {
            is->video_clk.paused = 0;
        }
        set_clock(&is->video_clk, get_clock(&is->video_clk), is->video_clk.serial);
    }
    is->paused = is->audio_clk.paused = is->video_clk.paused = !is->paused;
}

void toggle_pause(player_stat_t *is)
{
    stream_toggle_pause(is);
    is->step = 0;
}

// 此处输入的pos定义为相对总时长的相对位置
void stream_seek(player_stat_t *is, int64_t pos, int64_t rel, int seek_by_bytes)
{
    if (!is->seek_req) {
        is->seek_pos = pos;
        is->seek_rel = rel;
        is->seek_flags &= ~AVSEEK_FLAG_BYTE;
        if (seek_by_bytes)
            is->seek_flags |= AVSEEK_FLAG_BYTE;
        is->seek_req = 1;
        pthread_cond_signal(&is->continue_read_thread);
    }
}
#endif

