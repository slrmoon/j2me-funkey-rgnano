/*
 * FunKey audio engine: content detection and audio decoders.
 *
 * Detection looks at the bytes, not at the MIME type the game declared:
 * J2ME games routinely create "audio/midi" players for WAV data, "audio/x-wav"
 * for AMR, or pass null. Supported:
 *
 *   WAV  PCM 8/16/24/32-bit, IEEE float, A-law, mu-law, IMA ADPCM, MS ADPCM
 *   AU   (audio/basic) mu-law, A-law, 8/16-bit PCM
 *   AMR-NB (opencore, when FKA_HAVE_AMR)
 *   MP3  (minimp3, decoded while playing)
 *   SMF / RMID, SMAF, iMelody, Nokia OTA, RTTTL, JSR-135 tone sequences
 *   MLD  (i-mode / DoJa melody: Yamaha MA-3 FM synthesis, fka_mld.c)
 */
#include "fka_decode.h"
#include "fka.h"
#include "fka_mld.h"

#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define MINIMP3_ONLY_MP3
#define MINIMP3_NO_SIMD
#define MINIMP3_IMPLEMENTATION
#include "minimp3.h"

#ifdef FKA_HAVE_AMR
#include "interf_dec.h"
#endif

static uint16_t le16(const uint8_t *p) { return (uint16_t) (p[0] | (p[1] << 8)); }
static uint32_t le32(const uint8_t *p) { return p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t) p[3] << 24); }
static uint32_t be32(const uint8_t *p) { return ((uint32_t) p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3]; }

static int16_t ulaw(uint8_t u)
{
    int t;
    u = (uint8_t) ~u;
    t = ((u & 0x0f) << 3) + 0x84;
    t <<= (u & 0x70) >> 4;
    return (int16_t) ((u & 0x80) ? (0x84 - t) : (t - 0x84));
}

static int16_t alaw(uint8_t a)
{
    int t, seg;
    a ^= 0x55;
    t = (a & 0x0f) << 4;
    seg = (a & 0x70) >> 4;
    if (seg == 0) t += 8;
    else if (seg == 1) t += 0x108;
    else t = (t + 0x108) << (seg - 1);
    return (int16_t) ((a & 0x80) ? t : -t);
}

static int set_pcm(FkaPcm *p, int16_t *buf, int len, int rate)
{
    memset(p, 0, sizeof(*p));
    if (buf == NULL || len <= 0 || rate < 1000 || rate > 96000) {
        free(buf);
        return -1;
    }
    p->rate = rate;
    p->length = len;
    p->win = buf;
    p->win_len = len;
    p->whole = 1;
    return 0;
}

/* ------------------------------------------------------------------ */
/* WAV                                                                 */
/* ------------------------------------------------------------------ */

static const int ima_index[16] = { -1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8 };
static const int ima_step[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230,
    253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876, 963,
    1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327,
    3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442,
    11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794,
    32767
};

static int ima_nibble(int *pred, int *idx, int nib)
{
    int step = ima_step[*idx], diff = step >> 3;
    if (nib & 1) diff += step >> 2;
    if (nib & 2) diff += step >> 1;
    if (nib & 4) diff += step;
    if (nib & 8) diff = -diff;
    *pred += diff;
    if (*pred > 32767) *pred = 32767;
    if (*pred < -32768) *pred = -32768;
    *idx += ima_index[nib];
    if (*idx < 0) *idx = 0;
    if (*idx > 88) *idx = 88;
    return *pred;
}

static int16_t *wav_ima(const uint8_t *d, int n, int ch, int block, int spb, int *out_len)
{
    int blocks, b, k = 0;
    int16_t *out, *tmp;
    if (ch < 1 || ch > 2 || block < 4 * ch) return NULL;
    if (spb <= 0) spb = (block - 4 * ch) * 8 / (4 * ch) + 1;
    blocks = (n + block - 1) / block;
    out = (int16_t *) malloc((size_t) blocks * spb * sizeof(int16_t));
    tmp = (int16_t *) malloc((size_t) spb * 2 * sizeof(int16_t) + 16 * sizeof(int16_t));
    if (out == NULL || tmp == NULL) {
        free(out);
        free(tmp);
        return NULL;
    }
    for (b = 0; b < blocks; ++b) {
        const uint8_t *p = d + b * block;
        int avail = n - b * block, pred[2], idx[2], i, c, got;
        int16_t *t[2];
        t[0] = tmp;
        t[1] = tmp + spb + 8;
        if (avail < 4 * ch) break;
        if (avail > block) avail = block;
        for (c = 0; c < ch; ++c) {
            pred[c] = (int16_t) le16(p + 4 * c);
            idx[c] = p[4 * c + 2] > 88 ? 88 : p[4 * c + 2];
            t[c][0] = (int16_t) pred[c];
        }
        got = 1;
        p += 4 * ch;
        avail -= 4 * ch;
        /* data: 4-byte words per channel, 8 samples each, low nibble first */
        while (avail >= 4 * ch && got + 8 <= spb + 7) {
            for (c = 0; c < ch; ++c) {
                for (i = 0; i < 4; ++i) {
                    uint8_t byte = p[c * 4 + i];
                    t[c][got + i * 2] = (int16_t) ima_nibble(&pred[c], &idx[c], byte & 15);
                    t[c][got + i * 2 + 1] = (int16_t) ima_nibble(&pred[c], &idx[c], byte >> 4);
                }
            }
            got += 8;
            p += 4 * ch;
            avail -= 4 * ch;
        }
        if (got > spb) got = spb;
        for (i = 0; i < got; ++i)
            out[k++] = ch == 2 ? (int16_t) ((t[0][i] + t[1][i]) >> 1) : t[0][i];
    }
    free(tmp);
    *out_len = k;
    return out;
}

static int16_t *wav_msadpcm(const uint8_t *d, int n, int ch, int block, int spb,
                            const int16_t (*coef)[2], int ncoef, int *out_len)
{
    static const int adapt[16] = { 230, 230, 230, 230, 307, 409, 512, 614,
                                   768, 614, 512, 409, 307, 230, 230, 230 };
    int blocks, b, k = 0;
    int16_t *out;
    if (ch < 1 || ch > 2 || block < 7 * ch) return NULL;
    if (spb <= 0) spb = (block - 7 * ch) * 2 / ch + 2;
    blocks = (n + block - 1) / block;
    out = (int16_t *) malloc((size_t) blocks * spb * sizeof(int16_t));
    if (out == NULL) return NULL;
    for (b = 0; b < blocks; ++b) {
        const uint8_t *p = d + b * block;
        int avail = n - b * block, c, i, nib_count;
        int pr[2], delta[2], s1[2], s2[2];
        if (avail < 7 * ch) break;
        if (avail > block) avail = block;
        for (c = 0; c < ch; ++c) {
            pr[c] = p[c] < ncoef ? p[c] : 0;
            delta[c] = (int16_t) le16(p + ch + 2 * c);
            s1[c] = (int16_t) le16(p + 3 * ch + 2 * c);
            s2[c] = (int16_t) le16(p + 5 * ch + 2 * c);
        }
        p += 7 * ch;
        avail -= 7 * ch;
        out[k++] = ch == 2 ? (int16_t) ((s2[0] + s2[1]) >> 1) : (int16_t) s2[0];
        out[k++] = ch == 2 ? (int16_t) ((s1[0] + s1[1]) >> 1) : (int16_t) s1[0];
        nib_count = avail * 2;
        if (nib_count > (spb - 2) * ch) nib_count = (spb - 2) * ch;
        {
            int acc = 0;
            for (i = 0; i < nib_count; ++i) {
                int nib = (i & 1) ? (p[i >> 1] & 15) : (p[i >> 1] >> 4);
                int sn = nib & 8 ? nib - 16 : nib;
                int pred;
                c = ch == 2 ? (i & 1) : 0;
                pred = (s1[c] * coef[pr[c]][0] + s2[c] * coef[pr[c]][1]) >> 8;
                pred += sn * delta[c];
                if (pred > 32767) pred = 32767;
                if (pred < -32768) pred = -32768;
                s2[c] = s1[c];
                s1[c] = pred;
                delta[c] = (adapt[nib] * delta[c]) >> 8;
                if (delta[c] < 16) delta[c] = 16;
                if (ch == 2) {
                    if (c == 0) acc = pred;
                    else out[k++] = (int16_t) ((acc + pred) >> 1);
                } else {
                    out[k++] = (int16_t) pred;
                }
            }
        }
    }
    *out_len = k;
    return out;
}

static int decode_wav(const uint8_t *d, int n, FkaPcm *out)
{
    int pos = 12, tag = -1, ch = 1, rate = 0, block = 0, bits = 16, spb = 0;
    const uint8_t *data = NULL, *fmt = NULL;
    int data_len = 0, fmt_len = 0, len = 0, i;
    int16_t *pcm = NULL;
    while (pos + 8 <= n) {
        uint32_t sz = le32(d + pos + 4);
        int body = pos + 8;
        if (sz > (uint32_t) (n - body)) sz = (uint32_t) (n - body);
        if (memcmp(d + pos, "fmt ", 4) == 0 && sz >= 16) {
            fmt = d + body;
            fmt_len = (int) sz;
        } else if (memcmp(d + pos, "data", 4) == 0) {
            data = d + body;
            data_len = (int) sz;
            if (data_len == 0) data_len = n - body;  /* streamed WAV writers */
            break;
        }
        pos = body + (int) sz + (int) (sz & 1);
    }
    if (fmt == NULL || data == NULL) return -1;
    tag = le16(fmt);
    ch = le16(fmt + 2);
    rate = (int) le32(fmt + 4);
    block = le16(fmt + 12);
    bits = le16(fmt + 14);
    if (tag == 0xfffe && fmt_len >= 26) tag = le16(fmt + 24);
    if ((tag == 0x11 || tag == 2) && fmt_len >= 20) spb = le16(fmt + 18);
    if (ch < 1 || ch > 8) return -2;

    if (tag == 1 || tag == 3) {
        int bps = bits / 8, frames;
        if (bps < 1 || bps > 4) return -3;
        frames = data_len / (bps * ch);
        pcm = (int16_t *) malloc((size_t) (frames > 0 ? frames : 1) * sizeof(int16_t));
        if (pcm == NULL) return -4;
        for (i = 0; i < frames; ++i) {
            int c, acc = 0;
            for (c = 0; c < ch; ++c) {
                const uint8_t *s = data + (i * ch + c) * bps;
                int v;
                if (tag == 3 && bps == 4) {
                    float f;
                    memcpy(&f, s, 4);
                    if (f > 1.0f) f = 1.0f;
                    if (f < -1.0f) f = -1.0f;
                    v = (int) (f * 32767.0f);
                } else if (bps == 1) v = (s[0] - 128) << 8;
                else if (bps == 2) v = (int16_t) le16(s);
                else v = (int16_t) le16(s + bps - 2);
                acc += v;
            }
            pcm[i] = (int16_t) (acc / ch);
        }
        len = frames;
    } else if (tag == 6 || tag == 7) {
        int frames = data_len / ch;
        pcm = (int16_t *) malloc((size_t) (frames > 0 ? frames : 1) * sizeof(int16_t));
        if (pcm == NULL) return -4;
        for (i = 0; i < frames; ++i) {
            int c, acc = 0;
            for (c = 0; c < ch; ++c) {
                uint8_t b = data[i * ch + c];
                acc += tag == 7 ? ulaw(b) : alaw(b);
            }
            pcm[i] = (int16_t) (acc / ch);
        }
        len = frames;
    } else if (tag == 0x11) {
        pcm = wav_ima(data, data_len, ch, block, spb, &len);
    } else if (tag == 2) {
        static const int16_t def[7][2] = { { 256, 0 }, { 512, -256 }, { 0, 0 }, { 192, 64 },
                                           { 240, 0 }, { 460, -208 }, { 392, -232 } };
        int16_t coef[32][2];
        int ncoef = 7;
        memcpy(coef, def, sizeof(def));
        if (fmt_len >= 22) {
            int nc = le16(fmt + 20);
            if (nc >= 1 && nc <= 32 && fmt_len >= 22 + nc * 4) {
                ncoef = nc;
                for (i = 0; i < nc; ++i) {
                    coef[i][0] = (int16_t) le16(fmt + 22 + i * 4);
                    coef[i][1] = (int16_t) le16(fmt + 24 + i * 4);
                }
            }
        }
        pcm = wav_msadpcm(data, data_len, ch, block, spb, (const int16_t (*)[2]) coef, ncoef, &len);
    } else {
        return -5;
    }
    return set_pcm(out, pcm, len, rate);
}

/* ------------------------------------------------------------------ */
/* AU / Sun audio                                                      */
/* ------------------------------------------------------------------ */

static int decode_au(const uint8_t *d, int n, FkaPcm *out)
{
    uint32_t off = be32(d + 4), size = be32(d + 8), enc = be32(d + 12), rate = be32(d + 16);
    int ch = (int) be32(d + 20), bps, frames, i;
    int16_t *pcm;
    if (off < 24 || off > (uint32_t) n || ch < 1 || ch > 8) return -1;
    if (size == 0xffffffffu || size > (uint32_t) n - off) size = (uint32_t) n - off;
    bps = (enc == 3) ? 2 : 1;
    if (enc != 1 && enc != 2 && enc != 3 && enc != 27) return -2;
    frames = (int) size / (bps * ch);
    pcm = (int16_t *) malloc((size_t) (frames > 0 ? frames : 1) * sizeof(int16_t));
    if (pcm == NULL) return -3;
    for (i = 0; i < frames; ++i) {
        int c, acc = 0;
        for (c = 0; c < ch; ++c) {
            const uint8_t *s = d + off + (uint32_t) ((i * ch + c) * bps);
            if (enc == 1) acc += ulaw(s[0]);
            else if (enc == 27) acc += alaw(s[0]);
            else if (enc == 2) acc += (int8_t) s[0] * 256;
            else acc += (int16_t) ((s[0] << 8) | s[1]);
        }
        pcm[i] = (int16_t) (acc / ch);
    }
    return set_pcm(out, pcm, frames, (int) rate);
}

/* ------------------------------------------------------------------ */
/* AMR-NB                                                              */
/* ------------------------------------------------------------------ */

static int decode_amr(const uint8_t *d, int n, FkaPcm *out)
{
#ifdef FKA_HAVE_AMR
    static const int frame_size[16] = { 12, 13, 15, 17, 19, 20, 26, 31, 5, 6, 5, 5, 0, 0, 0, 0 };
    void *dec;
    int pos = 6, frames = 0, k = 0;
    int16_t *pcm;
    /* count frames first so the buffer is allocated once */
    while (pos < n) {
        int sz = frame_size[(d[pos] >> 3) & 15];
        if (pos + 1 + sz > n) break;
        pos += 1 + sz;
        ++frames;
    }
    if (frames == 0) return -1;
    pcm = (int16_t *) malloc((size_t) frames * 160 * sizeof(int16_t));
    if (pcm == NULL) return -2;
    dec = Decoder_Interface_init();
    if (dec == NULL) {
        free(pcm);
        return -3;
    }
    pos = 6;
    while (pos < n && k < frames * 160) {
        int sz = frame_size[(d[pos] >> 3) & 15];
        if (pos + 1 + sz > n) break;
        /* NO_DATA / SID frames still yield 160 samples (comfort noise) */
        Decoder_Interface_Decode(dec, (unsigned char *) d + pos, pcm + k, 0);
        k += 160;
        pos += 1 + sz;
    }
    Decoder_Interface_exit(dec);
    return set_pcm(out, pcm, k, 8000);
#else
    (void) d; (void) n; (void) out;
    return -10;
#endif
}

/* ------------------------------------------------------------------ */
/* MP3 (streaming)                                                     */
/* ------------------------------------------------------------------ */

static int mp3_skip_id3(const uint8_t *d, int n)
{
    if (n >= 10 && memcmp(d, "ID3", 3) == 0) {
        int sz = ((d[6] & 0x7f) << 21) | ((d[7] & 0x7f) << 14) | ((d[8] & 0x7f) << 7) | (d[9] & 0x7f);
        return 10 + sz + ((d[5] & 0x10) ? 10 : 0);
    }
    return 0;
}

/* Xing/Info + LAME tag (from minimp3_ex.h, CC0): total frames and the
 * encoder delay/padding, so looping MP3 music has no gap at the seam. */
static int mp3_vbrtag(const uint8_t *frame, int frame_size, uint32_t *frames, int *delay, int *padding)
{
    bs_t bs[1];
    L3_gr_info_t gr_info[4];
    int flags, off;
    const uint8_t *tag;
    if (frame_size < HDR_SIZE + 8) return 0;
    bs_init(bs, frame + HDR_SIZE, frame_size - HDR_SIZE);
    if (HDR_IS_CRC(frame)) get_bits(bs, 16);
    if (L3_read_side_info(bs, gr_info, frame) < 0) return 0;
    off = HDR_SIZE + bs->pos / 8;
    if (off > frame_size || frame_size - off < 12) return 0;
    tag = frame + off;
    if (memcmp(tag, "Xing", 4) != 0 && memcmp(tag, "Info", 4) != 0) return 0;
    flags = tag[7];
    if (!(flags & 1)) return 0;
    off += 8;
    tag = frame + off;
    *frames = ((uint32_t) tag[0] << 24) | ((uint32_t) tag[1] << 16) | ((uint32_t) tag[2] << 8) | tag[3];
    off += 4;
    if (flags & 2) off += 4;
    if (flags & 4) off += 100;
    if (flags & 8) off += 4;
    *delay = *padding = 0;
    if (frame_size - off < 1) return 1;
    if (frame[off] != 0 && frame_size - off > 35) {
        tag = frame + off + 21;
        *delay = ((tag[0] << 4) | (tag[1] >> 4)) + (528 + 1);
        *padding = (((tag[1] & 0xf) << 8) | tag[2]) - (528 + 1);
    }
    return 1;
}

static int decode_mp3(const uint8_t *d, int n, FkaPcm *out)
{
    mp3dec_t *dec;
    mp3dec_frame_info_t info;
    int pos = mp3_skip_id3(d, n), first = 1, rate = 0, guard = 0, start, delay = 0;
    int64_t total = 0, tagged = -1;
    if (pos >= n) return -1;
    dec = (mp3dec_t *) malloc(sizeof(mp3dec_t));
    if (dec == NULL) return -2;
    mp3dec_init(dec);
    start = pos;
    /* scan headers (no synthesis) for the rate and the exact length */
    while (pos < n && ++guard < 1000000) {
        int samples = mp3dec_decode_frame(dec, d + pos, n - pos, NULL, &info);
        if (info.frame_bytes <= 0) break;
        if (samples > 0 && first) {
            uint32_t frames = 0;
            int padding = 0;
            rate = info.hz;
            first = 0;
            /* the frame we got may start after junk: locate it */
            {
                int fpos = pos, k;
                for (k = pos; k + 4 <= n && k < pos + info.frame_bytes; ++k) {
                    if (d[k] == 0xff && (d[k + 1] & 0xe0) == 0xe0) {
                        fpos = k;
                        break;
                    }
                }
                if (mp3_vbrtag(d + fpos, n - fpos, &frames, &delay, &padding) > 0) {
                    int64_t t = (int64_t) frames * samples - delay - (padding > 0 ? padding : 0);
                    if (delay < 0) delay = 0;
                    if (t > 0) tagged = t;
                    pos += info.frame_bytes;
                    start = pos;       /* the Info frame itself is not audio */
                    mp3dec_init(dec);
                    continue;
                }
                delay = 0;
            }
        }
        pos += info.frame_bytes;
        if (samples <= 0) continue;
        total += samples;
    }
    if (tagged > 0 && tagged <= total) total = tagged;
    else if (delay > 0 && total > delay) total -= delay;
    if (rate == 0 || total <= 0) {
        free(dec);
        return -3;
    }
    memset(out, 0, sizeof(*out));
    out->rate = rate;
    out->length = total;
    out->mp3 = dec;
    out->data = (uint8_t *) malloc((size_t) n);
    out->win_cap = MINIMP3_MAX_SAMPLES_PER_FRAME * 3;
    out->win = (int16_t *) malloc((size_t) out->win_cap * sizeof(int16_t));
    if (out->data == NULL || out->win == NULL) {
        fka_pcm_free(out);
        return -4;
    }
    memcpy(out->data, d, (size_t) n);
    out->data_len = n;
    out->mp3_start = start;
    out->mp3_delay = delay;
    out->data_pos = start;
    out->mp3_to_skip = delay;
    mp3dec_init(dec);
    return 0;
}

static void mp3_rewind(FkaPcm *p)
{
    mp3dec_init((mp3dec_t *) p->mp3);
    p->data_pos = p->mp3_start;
    p->mp3_to_skip = p->mp3_delay;
    p->win_start = 0;
    p->win_len = 0;
}

/* Appends one decoded frame to the window; returns samples added. */
static int mp3_next(FkaPcm *p, int keep_from)
{
    mp3dec_frame_info_t info;
    int16_t frame[MINIMP3_MAX_SAMPLES_PER_FRAME];
    int guard = 0;
    while (p->data_pos < p->data_len && guard++ < 64) {
        int samples = mp3dec_decode_frame((mp3dec_t *) p->mp3, p->data + p->data_pos,
                                          p->data_len - p->data_pos, frame, &info);
        int i, drop;
        if (info.frame_bytes <= 0) {
            p->data_pos = p->data_len;
            return 0;
        }
        p->data_pos += info.frame_bytes;
        if (samples <= 0) continue;
        /* drop what the caller no longer needs */
        drop = keep_from - (int) (p->win_start);
        if (drop < 0) drop = 0;
        if (drop > p->win_len) drop = p->win_len;
        if (p->win_len - drop + samples > p->win_cap) drop = p->win_len + samples - p->win_cap;
        if (drop > 0) {
            memmove(p->win, p->win + drop, (size_t) (p->win_len - drop) * sizeof(int16_t));
            p->win_len -= drop;
            p->win_start += drop;
        }
        i = 0;
        if (p->mp3_to_skip > 0) {
            i = p->mp3_to_skip < samples ? p->mp3_to_skip : samples;
            p->mp3_to_skip -= i;
            if (i == samples) continue;
        }
        drop = i;
        for (; i < samples; ++i) {
            int v = info.channels == 2 ? (frame[2 * i] + frame[2 * i + 1]) >> 1 : frame[i];
            p->win[p->win_len++] = (int16_t) v;
        }
        return samples - drop;
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* MLD: rendered by the MA-3 synth while playing                       */
/* ------------------------------------------------------------------ */

#define MLD_CHUNK 1024

static int decode_mld(const uint8_t *d, int n, FkaPcm *out)
{
    long long frames;
    memset(out, 0, sizeof(*out));
    frames = fka_mld_count_frames(d, n, (float) FKA_RATE);
    if (frames <= 0) return -1;
    out->mld = fka_mld_open(d, n, (float) FKA_RATE);
    out->win_cap = 4 * MLD_CHUNK;
    out->win = (int16_t *) malloc((size_t) out->win_cap * sizeof(int16_t));
    out->mld_buf = (float *) malloc(2 * MLD_CHUNK * sizeof(float));
    if (out->mld == NULL || out->win == NULL || out->mld_buf == NULL) {
        fka_mld_close((FkaMld *) out->mld);
        free(out->win);
        free(out->mld_buf);
        memset(out, 0, sizeof(*out));
        return -1;
    }
    out->rate = FKA_RATE;
    out->length = frames;
    return 0;
}

static void mld_rewind(FkaPcm *p)
{
    fka_mld_reset((FkaMld *) p->mld);
    p->win_start = 0;
    p->win_len = 0;
    p->rebase = 0;
}

static int mld_next(FkaPcm *p, int64_t keep_from)
{
    int64_t drop = keep_from - p->win_start;
    int got, i, tries;
    long long shift = 0;
    if (drop < 0) drop = 0;
    if (drop > p->win_len) drop = p->win_len;
    if (p->win_len - drop + MLD_CHUNK > p->win_cap) drop = p->win_len + MLD_CHUNK - p->win_cap;
    if (drop > 0) {
        memmove(p->win, p->win + drop, (size_t) (p->win_len - drop) * sizeof(int16_t));
        p->win_len -= (int) drop;
        p->win_start += drop;
    }
    fka_mld_set_loop((FkaMld *) p->mld, p->mld_loop);
    for (tries = 0; tries < 4; ++tries) {
        got = fka_mld_render((FkaMld *) p->mld, p->mld_buf, MLD_CHUNK);
        shift = fka_mld_take_shift((FkaMld *) p->mld);
        if (got != 0 || shift == 0) break;
        /* jumped back before rendering anything */
        p->win_start -= shift;
        p->rebase += shift;
    }
    if (got <= 0) return 0;   /* finished, release tails included */
    for (i = 0; i < got; ++i) {
        /* mono: average of the MA-3 stereo pair, rounded half up */
        float v = (p->mld_buf[2 * i] + p->mld_buf[2 * i + 1]) * 0.5f * 32767.0f;
        int s = (int) (v >= 0 ? v + 0.5f : v - 0.5f);
        if (s > 32767) s = 32767;
        if (s < -32768) s = -32768;
        p->win[p->win_len++] = (int16_t) s;
    }
    if (shift != 0) {
        /* the frames after this chunk continue from the loop start */
        p->win_start -= shift;
        p->rebase += shift;
    }
    return got;
}

const int16_t *fka_pcm_at(FkaPcm *p, int64_t idx)
{
    if (idx < 0) return NULL;
    if (p->mld != NULL) {
        /* no length limit: the release tail runs past the last event */
        if (idx < p->win_start) mld_rewind(p);
        while (idx + 1 >= p->win_start + p->win_len) {
            int64_t before = p->rebase;
            if (mld_next(p, idx > 0 ? idx - 1 : 0) == 0) break;
            idx -= p->rebase - before;
        }
        if (idx >= p->win_start + p->win_len) return NULL;
        return p->win + (idx - p->win_start);
    }
    if (p->length >= 0 && idx >= p->length) return NULL;
    if (p->whole) return p->win + idx;
    if (idx < p->win_start) mp3_rewind(p);
    while (idx + 1 >= p->win_start + p->win_len) {
        if (mp3_next(p, (int) (idx > 0 ? idx - 1 : 0)) == 0) break;
    }
    if (idx >= p->win_start + p->win_len) return NULL;
    return p->win + (idx - p->win_start);
}

void fka_pcm_free(FkaPcm *p)
{
    if (p->mld != NULL) fka_mld_close((FkaMld *) p->mld);
    free(p->mld_buf);
    free(p->win);
    free(p->mp3);
    free(p->data);
    memset(p, 0, sizeof(*p));
}

/* ------------------------------------------------------------------ */
/* detection                                                           */
/* ------------------------------------------------------------------ */

static int is_mp3_frame(const uint8_t *d, int n)
{
    mp3dec_t dec;
    mp3dec_frame_info_t info;
    int pos = mp3_skip_id3(d, n);
    if (pos >= n - 4) return pos > 0 && pos < n;
    if (d[pos] != 0xff || (d[pos + 1] & 0xe0) != 0xe0) return 0;
    mp3dec_init(&dec);
    mp3dec_decode_frame(&dec, d + pos, n - pos, NULL, &info);
    return info.frame_bytes > 0 && info.hz > 0;
}

static const uint8_t *skip_text_ws(const uint8_t *p, const uint8_t *end)
{
    if (end - p >= 3 && p[0] == 0xef && p[1] == 0xbb && p[2] == 0xbf) p += 3;
    while (p < end && (*p == ' ' || *p == '\r' || *p == '\n' || *p == '\t')) ++p;
    return p;
}

static int looks_rtttl(const uint8_t *d, int n)
{
    int i, colons = 0, eq = 0;
    for (i = 0; i < n && i < 256; ++i) {
        if (d[i] == ':') ++colons;
        else if (d[i] == '=') ++eq;
        else if (d[i] < 0x20 && d[i] != '\r' && d[i] != '\n' && d[i] != '\t') return 0;
    }
    return colons >= 2 && eq >= 1;
}

int fka_decode(const uint8_t *data, int len, const char *hint, FkaContent *out)
{
    const uint8_t *t;
    int i, r = -1;
    memset(out, 0, sizeof(*out));
    if (data == NULL || len < 4) return -1;

    /* MIDI first: SMF, possibly inside RIFF RMID or behind junk */
    if (memcmp(data, "MThd", 4) == 0 ||
        (len > 20 && memcmp(data, "RIFF", 4) == 0 && memcmp(data + 8, "RMID", 4) == 0)) {
        out->kind = FKA_KIND_SONG;
        out->type = "audio/midi";
        return fka_parse_smf(data, len, &out->song);
    }
    if (len > 8 && memcmp(data, "melo", 4) == 0) {
        out->kind = FKA_KIND_PCM;
        out->type = "audio/x-mld";
        return decode_mld(data, len, &out->pcm);
    }
    if (memcmp(data, "MMMD", 4) == 0) {
        out->kind = FKA_KIND_SONG;
        out->type = "application/vnd.smaf";
        return fka_parse_smaf(data, len, &out->song);
    }
    if (len > 12 && memcmp(data, "RIFF", 4) == 0 && memcmp(data + 8, "WAVE", 4) == 0) {
        out->kind = FKA_KIND_PCM;
        out->type = "audio/x-wav";
        return decode_wav(data, len, &out->pcm);
    }
    if (len > 24 && memcmp(data, ".snd", 4) == 0) {
        out->kind = FKA_KIND_PCM;
        out->type = "audio/basic";
        return decode_au(data, len, &out->pcm);
    }
    if (len > 6 && memcmp(data, "#!AMR\n", 6) == 0) {
        out->kind = FKA_KIND_PCM;
        out->type = "audio/amr";
        return decode_amr(data, len, &out->pcm);
    }
    if (data[0] == 0xfe && data[1] == 0x01) {
        out->kind = FKA_KIND_SONG;
        out->type = "audio/x-tone-seq";
        return fka_parse_tone_sequence(data, len, &out->song);
    }
    t = skip_text_ws(data, data + len);
    if (data + len - t > 13 && strncasecmp((const char *) t, "BEGIN:IMELODY", 13) == 0) {
        out->kind = FKA_KIND_SONG;
        out->type = "audio/imelody";
        return fka_parse_imelody(t, (int) (data + len - t), &out->song);
    }
    if (is_mp3_frame(data, len)) {
        out->kind = FKA_KIND_PCM;
        out->type = "audio/mpeg";
        return decode_mp3(data, len, &out->pcm);
    }
    if (data[0] >= 1 && data[0] <= 8 && (data[1] >> 1) == 0x25) {
        out->kind = FKA_KIND_SONG;
        out->type = "audio/x-nokia-ota";
        if (fka_parse_ota(data, len, &out->song) == 0) return 0;
    }
    /* OTA without the leading command count (starts at "ringing tone
     * programming", 0x25 << 1), as some Nokia games store it */
    if ((data[0] >> 1) == 0x25 && len < 65536) {
        uint8_t *tmp = (uint8_t *) malloc((size_t) len + 1);
        if (tmp != NULL) {
            tmp[0] = 2;
            memcpy(tmp + 1, data, (size_t) len);
            r = fka_parse_ota(tmp, len + 1, &out->song);
            free(tmp);
            if (r == 0) {
                out->kind = FKA_KIND_SONG;
                out->type = "audio/x-nokia-ota";
                return 0;
            }
        }
    }
    if (looks_rtttl(t, (int) (data + len - t))) {
        out->kind = FKA_KIND_SONG;
        out->type = "audio/x-rtttl";
        if (fka_parse_rtttl(t, (int) (data + len - t), &out->song) == 0) return 0;
    }
    /* last resort: some files hide an SMF behind a vendor header */
    for (i = 0; i + 14 < len && i < 4096; ++i) {
        if (data[i] == 'M' && memcmp(data + i, "MThd", 4) == 0) {
            out->kind = FKA_KIND_SONG;
            out->type = "audio/midi";
            return fka_parse_smf(data + i, len - i, &out->song);
        }
    }
    (void) hint;
    out->kind = FKA_KIND_NONE;
    return r;
}

void fka_content_free(FkaContent *c)
{
    fka_pcm_free(&c->pcm);
    fka_song_free(&c->song);
    c->kind = FKA_KIND_NONE;
}
