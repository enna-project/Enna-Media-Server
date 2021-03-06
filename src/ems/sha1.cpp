/*
 * Copyright (C) 2007 Michael Niedermayer <michaelni@gmx.at>
 * based on public domain SHA-1 code by Steve Reid <steve@edmweb.com>
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * Modified by Joakim Plate <elupus@ecce.se> to be stand alone from libavutil
 */

#include "sha1.h"
#include <string.h>


/* static uint16_t bswap_16(uint16_t x) */
/* { */
/*     x= (x>>8) | (x<<8); */
/*     return x; */
/* } */

static uint32_t bswap_32(uint32_t x)
{
    x= ((x<<8)&0xFF00FF00) | ((x>>8)&0x00FF00FF);
    x= (x>>16) | (x<<16);
    return x;
}

static uint64_t bswap_64(uint64_t x)
{
    union {
        uint64_t ll;
        uint32_t l[2];
    } w, r;
    w.ll = x;
    r.l[0] = bswap_32 (w.l[1]);
    r.l[1] = bswap_32 (w.l[0]);
    return r.ll;
}

// be2me ... big-endian to machine-endian
// le2me ... little-endian to machine-endian

#ifdef WORDS_BIGENDIAN
#define be2me_16(x) (x)
#define be2me_32(x) (x)
#define be2me_64(x) (x)
#define le2me_16(x) bswap_16(x)
#define le2me_32(x) bswap_32(x)
#define le2me_64(x) bswap_64(x)
#else
#define be2me_16(x) bswap_16(x)
#define be2me_32(x) bswap_32(x)
#define be2me_64(x) bswap_64(x)
#define le2me_16(x) (x)
#define le2me_32(x) (x)
#define le2me_64(x) (x)
#endif

const int sha1_size = sizeof(SHA1);

#define rol(value, bits) (((value) << (bits)) | ((value) >> (32 - (bits))))

/* (R0+R1), R2, R3, R4 are the different operations used in SHA1 */
#define blk0(i) (block[i] = be2me_32(((const uint32_t*)buffer)[i]))
#define blk(i) (block[i] = rol(block[i-3]^block[i-8]^block[i-14]^block[i-16],1))

#define R0(v,w,x,y,z,i) z+=((w&(x^y))^y)    +blk0(i)+0x5A827999+rol(v,5);w=rol(w,30);
#define R1(v,w,x,y,z,i) z+=((w&(x^y))^y)    +blk (i)+0x5A827999+rol(v,5);w=rol(w,30);
#define R2(v,w,x,y,z,i) z+=( w^x     ^y)    +blk (i)+0x6ED9EBA1+rol(v,5);w=rol(w,30);
#define R3(v,w,x,y,z,i) z+=(((w|x)&y)|(w&x))+blk (i)+0x8F1BBCDC+rol(v,5);w=rol(w,30);
#define R4(v,w,x,y,z,i) z+=( w^x     ^y)    +blk (i)+0xCA62C1D6+rol(v,5);w=rol(w,30);

/* Hash a single 512-bit block. This is the core of the algorithm. */

static void transform(uint32_t state[5], const uint8_t buffer[64]){
    uint32_t block[80];
    unsigned int i, a, b, c, d, e;

    a = state[0];
    b = state[1];
    c = state[2];
    d = state[3];
    e = state[4];
#if CONFIG_SMALL
    for(i=0; i<80; i++){
        int t;
        if(i<16) t= be2me_32(((uint32_t*)buffer)[i]);
        else     t= rol(block[i-3]^block[i-8]^block[i-14]^block[i-16],1);
        block[i]= t;
        t+= e+rol(a,5);
        if(i<40){
            if(i<20)    t+= ((b&(c^d))^d)    +0x5A827999;
            else        t+= ( b^c     ^d)    +0x6ED9EBA1;
        }else{
            if(i<60)    t+= (((b|c)&d)|(b&c))+0x8F1BBCDC;
            else        t+= ( b^c     ^d)    +0xCA62C1D6;
        }
        e= d;
        d= c;
        c= rol(b,30);
        b= a;
        a= t;
    }
#else
    for(i=0; i<15; i+=5){
        R0(a,b,c,d,e,0+i); R0(e,a,b,c,d,1+i); R0(d,e,a,b,c,2+i); R0(c,d,e,a,b,3+i); R0(b,c,d,e,a,4+i);
    }
    R0(a,b,c,d,e,15); R1(e,a,b,c,d,16); R1(d,e,a,b,c,17); R1(c,d,e,a,b,18); R1(b,c,d,e,a,19);
    for(i=20; i<40; i+=5){
        R2(a,b,c,d,e,0+i); R2(e,a,b,c,d,1+i); R2(d,e,a,b,c,2+i); R2(c,d,e,a,b,3+i); R2(b,c,d,e,a,4+i);
    }
    for(; i<60; i+=5){
        R3(a,b,c,d,e,0+i); R3(e,a,b,c,d,1+i); R3(d,e,a,b,c,2+i); R3(c,d,e,a,b,3+i); R3(b,c,d,e,a,4+i);
    }
    for(; i<80; i+=5){
        R4(a,b,c,d,e,0+i); R4(e,a,b,c,d,1+i); R4(d,e,a,b,c,2+i); R4(c,d,e,a,b,3+i); R4(b,c,d,e,a,4+i);
    }
#endif
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
}

void sha1_init(SHA1* ctx){
    ctx->state[0] = 0x67452301;
    ctx->state[1] = 0xEFCDAB89;
    ctx->state[2] = 0x98BADCFE;
    ctx->state[3] = 0x10325476;
    ctx->state[4] = 0xC3D2E1F0;
    ctx->count    = 0;
}

void sha1_update(SHA1* ctx, const uint8_t* data, unsigned int len){
    unsigned int i, j;

    j = ctx->count & 63;
    ctx->count += len;
#if CONFIG_SMALL
    for( i = 0; i < len; i++ ){
        ctx->buffer[ j++ ] = data[i];
        if( 64 == j ){
            transform(ctx->state, ctx->buffer);
            j = 0;
        }
    }
#else
    if ((j + len) > 63) {
        memcpy(&ctx->buffer[j], data, (i = 64-j));
        transform(ctx->state, ctx->buffer);
        for ( ; i + 63 < len; i += 64) {
            transform(ctx->state, &data[i]);
        }
        j=0;
    }
    else i = 0;
    memcpy(&ctx->buffer[j], &data[i], len - i);
#endif
}

void sha1_final(SHA1* ctx, uint8_t digest[20]){
    int i;
    uint64_t finalcount= be2me_64(ctx->count<<3);

    sha1_update(ctx, (const uint8_t*)"\200", 1);
    while ((ctx->count & 63) != 56) {
        sha1_update(ctx, (const uint8_t*)"", 1);
    }
    sha1_update(ctx, (uint8_t *)&finalcount, 8); /* Should cause a transform() */
    for(i=0; i<5; i++)
        ((uint32_t*)digest)[i]= be2me_32(ctx->state[i]);
}

