
#include "mp3.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/soundcard.h>

#define MINIMP3_IMPLEMENTATION
#include "minimp3.h"

#define INBUF_SIZE 32768
#define PCM_BUF_SIZE (MINIMP3_MAX_SAMPLES_PER_FRAME*2)
#define DSP_RATE 32000
#define DSP_CH   2

static int open_dsp(int rate,int ch){
    int fd = open("/dev/snd/dsp",O_WRONLY);
    if(fd<0) fd=open("/dev/dsp",O_WRONLY);
    if(fd<0){
        return -1;
    }

    int fmt = AFMT_S16_LE;
    ioctl(fd, SOUND_PCM_WRITE_BITS, &fmt);
    ioctl(fd, SOUND_PCM_WRITE_CHANNELS, &ch);

    int rr=rate;
    ioctl(fd, SOUND_PCM_WRITE_RATE, &rr);

    return fd;
}

static void write_all(int fd,const short *buf,int frames){
    write(fd, buf, frames*DSP_CH*sizeof(short));
}

static int resample_48k_to_32k(const short *in,int frames,short *out){
    double step=48000.0/32000.0;
    double pos=0.0;
    int out_frames=0;

    while((int)pos<frames){
        int i=(int)pos;
        out[out_frames*2+0]=in[i*2+0];
        out[out_frames*2+1]=in[i*2+1];
        out_frames++;
        pos+=step;
    }
    return out_frames;
}

/**
 * Reads a single key from the TTY file descriptor. Returns the character read, or -1 on error or if no key is available.
 *
 * Do not use kbd_read_char here because it may block waiting for a key, while we want to check for a key press without blocking.
 */
static inline char pax_getkey(const int tty_fd) {
    char ch = 0;
    ssize_t r = read(tty_fd, &ch, 1);
    if (r <= 0) return -1;
    return ch;
}

int play_mp3(const char *filename, const int tty_fd){
    FILE *f=fopen(filename,"rb");
    if(!f) return -1;

    uint8_t inbuf[INBUF_SIZE];
    size_t buf_len = fread(inbuf,1,INBUF_SIZE,f);

    mp3dec_t dec;
    mp3dec_init(&dec);

    short pcm[PCM_BUF_SIZE];
    short outbuf[PCM_BUF_SIZE];

    int dsp=-1;
    int exit_flag=0;

    printf("Playing %s\n", filename);
    fflush(stdout);

    while(buf_len>0){
        mp3dec_frame_info_t fi;
        int samples=mp3dec_decode_frame(&dec,inbuf,buf_len,pcm,&fi);

        if(fi.frame_bytes==0){
            buf_len=0;
            break;
        }

        memmove(inbuf,inbuf+fi.frame_bytes,buf_len-fi.frame_bytes);
        buf_len-=fi.frame_bytes;

        if(samples<=0) continue;
        if(dsp<0){
            dsp=open_dsp(DSP_RATE,DSP_CH);
            if(dsp<0) break;
        }

        int out_frames = (fi.hz==48000) ? resample_48k_to_32k(pcm,samples,outbuf) : samples;
        if(fi.hz!=48000) memcpy(outbuf,pcm,samples*DSP_CH*sizeof(short));

        write_all(dsp,outbuf,out_frames);

        // doplnění bufferu
        if(buf_len<1024){
            size_t n=fread(inbuf+buf_len,1,INBUF_SIZE-buf_len,f);
            if(n==0) break;
            buf_len+=n;
        }

        // kontrola tlačítka pro exit
        char ch = pax_getkey(tty_fd);
        if(ch==0) { exit_flag=1; break; }
    }

    printf("Stopping %s\n", filename);
    fflush(stdout);

    if(dsp>=0) close(dsp);
    fclose(f);
    return exit_flag;
}
