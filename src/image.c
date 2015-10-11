#include "image.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <string.h>
#include "memset_pattern4.h"

#define ATHENA_MIN(A, B) (((A)>(B))?(B):(A))

void Athena_CreateImageArray(struct Athena_ImageArray *ia){
    ia->images = malloc(sizeof(struct Athena_Image) * 8);
    ia->num_images = 0;
    ia->images_capacity = 8;
}

static void athena_destroy_image_array(struct Athena_Image *images, unsigned length){
    if(!length)
        return;
    else{
        Athena_DestroyImage(images);
        athena_destroy_image_array(images + 1, length - 1);
    }
}

void Athena_DestroyImageArray(struct Athena_ImageArray *ia){
    athena_destroy_image_array(ia->images, ia->num_images);

    free(ia->images);
    ia->num_images = 0;
    ia->images_capacity = 0;
}

void Athena_DestroyImage(struct Athena_Image *that){
    free(that->pixels);
    that->pixels = NULL;
    that->w = that->h = 0;
}

static unsigned Athena_LowerBlitWidth(const struct Athena_Image *src, const struct Athena_Image *dst, unsigned x){
    const unsigned clip = dst->w - x;
    if(src->w<clip)
        return src->w;
    else
        return clip;
}

void Athena_LowerBlitScanLine(const struct Athena_Image *src, struct Athena_Image *dst, unsigned line, unsigned len, unsigned x, unsigned y){
    memcpy(dst->pixels + x + (y * dst->w), src->pixels + (line * src->w), len << 2);
}

/* #define ATHENA_DISABLE_OPT_BLEND 
    #pragma error
*/

#ifndef ATHENA_DISABLE_OPT_BLEND

static unsigned athena_find_solid_block(const struct Athena_Image *src, unsigned x, unsigned y){
/* the > 0xFA is a Slightly greedy fudge to improve performance. */
    if(x < src->w && Athena_RawToA( *Athena_PixelConst(src, x, y) ) > 0xFA)
        return athena_find_solid_block(src, x+1, y);

    return x;
}

static unsigned athena_find_empty_block(const struct Athena_Image *src, unsigned x, unsigned y){
/* the < 0x08 is a Slightly greedy fudge to improve performance. */
    if(x < src->w && Athena_RawToA( *Athena_PixelConst(src, x, y) ) < 0x08)
        return athena_find_empty_block(src, x+1, y);

    return x;
}

#endif

static int athena_blit_scanline_blended_iter(const struct Athena_Image *src, struct Athena_Viewport *to, unsigned laser_x, unsigned laser_y){
    if(laser_y >= to->h)
        return 0;
    else if(laser_x >= to->w)
        return athena_blit_scanline_blended_iter(src, to, 0, laser_y + 1);
    else{

        uint32_t *pixel_to = Athena_Pixel(to->image, to->x + laser_x, to->y + laser_y);
        const uint32_t *pixel_from = Athena_PixelConst(src, laser_x, laser_y);
#ifndef ATHENA_DISABLE_OPT_BLEND
        const unsigned empty_x = athena_find_empty_block(src, laser_x, laser_y),
            solid_x = athena_find_solid_block(src, laser_x, laser_y);
        if(solid_x > laser_x){
            const unsigned len = ATHENA_MIN(solid_x - laser_x, to->w - laser_x);
            memcpy(pixel_to, pixel_from, len << 2);
            return athena_blit_scanline_blended_iter(src, to, laser_x + len, laser_y);
        }
        else if(empty_x > laser_x){
            /* Just skip the zero alpha area */
            return athena_blit_scanline_blended_iter(src, to, empty_x, laser_y);
        }
        else
#endif
        {
            pixel_to[0] = Athena_RGBARawBlend(*pixel_from, *pixel_to);
            return athena_blit_scanline_blended_iter(src, to, laser_x + 1, laser_y);
        }
    }
}

void Athena_BlitScanLine(const struct Athena_Image *src, struct Athena_Image *dst, unsigned line, unsigned x, unsigned y){
    assert(src);
    assert(dst);
    Athena_LowerBlitScanLine(src, dst, line, Athena_LowerBlitWidth(src, dst, x), x, y);
}

static void Athena_LowerBlit(const struct Athena_Image *src, struct Athena_Image *dst, unsigned line, unsigned len, unsigned x, unsigned y){
    if((line < src->h) && (y < dst->h)){
        Athena_LowerBlitScanLine(src, dst, line, len, x, y);
        Athena_LowerBlit(src, dst, line+1, len, x, y+1);
    }
}

void Athena_Blit(const struct Athena_Image *src, struct Athena_Image *dst, int x, int y){
    assert(src);
    assert(dst);
    if(x < dst->w && y < dst->h && x + (long)src->w > 0 && y + (long)src->h > 0){
        const unsigned len = Athena_LowerBlitWidth(src, dst, x);
        if(len)
            Athena_LowerBlit(src, dst, 0, len, x, y);
    }
}

void Athena_BlitBlended(const struct Athena_Image *src, struct Athena_Image *dst, int x, int y){
    assert(src);
    assert(dst);
    if(x < dst->w && y < dst->h && x + (long)src->w > 0 && y + (long)src->h > 0){
        struct Athena_Viewport to;
        const unsigned len = Athena_LowerBlitWidth(src, dst, x);
        
        to.image = dst;
        to.x = x;
        to.y = y;
        to.w = len;
        to.h = ATHENA_MIN(src->h, dst->h - to.y);
        
        athena_blit_scanline_blended_iter(src, &to, 0, 0);
    }
}

void Athena_CloneImage(struct Athena_Image *to, const struct Athena_Image *from){
    const unsigned pix_size = from->w * from->h << 2;
    to->w = from->w;
    to->h = from->h;   
    
    to->pixels = malloc(pix_size);
    memcpy(to->pixels, from->pixels, pix_size);
}

void Athena_CreateImage(struct Athena_Image *that, unsigned w, unsigned h){
    that->w = w;
    that->h = h;

    that->pixels = calloc(w<<1, h<<1);
}

void Athena_SetPixel(struct Athena_Image *to, int x, int y, uint32_t color){
    if(x < 0 || y < 0 || x >= to->w || y >= to->h)
        return;
    Athena_Pixel(to, x, y)[0] = color;
}

uint32_t Athena_GetPixel(struct Athena_Image *to, int x, int y){
    if(x < 0 || y < 0 || x >= to->w || y >= to->h)
        return 0;
    return *Athena_Pixel(to, x, y);
}

void Athena_FillViewport(struct Athena_Viewport *v, uint32_t color){
    Athena_FillRect(v->image, v->x, v->y, v->w, v->h, color);
}

void Athena_FillRect(struct Athena_Image *to, int x, int y, unsigned w, unsigned h, uint32_t color){
    if(!w || !h)
        return;
    if(x < 0 || y < 0 || x >= to->w || y >= to->h)
        return;
    if(w == 1){
        Athena_SetPixel(to, x, y, color);
        if(h > 1){
            Athena_FillRect(to, x, y+1, 1, h-1, color); 
        }
    }
    else{
        memset_pattern4(Athena_Pixel(to, x, y), &color, ATHENA_MIN(w, to->w - x)<<2);

        Athena_FillRect(to, x, y+1, w, h-1, color);
    }
}

void Athena_FillViewport(struct Athena_Viewport *v, uint32_t color);

uint32_t *Athena_Pixel(struct Athena_Image *to, int x, int y){
    return (uint32_t *)Athena_PixelConst(to, x, y);
}

const uint32_t *Athena_PixelConst(const struct Athena_Image *to, int x, int y){
    return to->pixels + x + (y * to->w);
}

/* 0xFF00FFFF is yellow. That is all. */

uint32_t Athena_RGBAToRaw(uint8_t r, uint8_t g, uint8_t b, uint8_t a){
    return (a << 24) | (b << 16) | (g << 8) | (r);
}

void Athena_RawToRGBA(uint32_t rgba, uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *a){
    r[0] = rgba & 0xFF;
    rgba >>= 8;
    g[0] = rgba & 0xFF;
    rgba >>= 8;
    b[0] = rgba & 0xFF;
    rgba >>= 8;
    a[0] = rgba & 0xFF;
}

uint8_t Athena_RawToR(uint32_t rgba){
    return rgba & 0xFF;
}

uint8_t Athena_RawToG(uint32_t rgba){
    return (rgba >> 8) & 0xFF;
}

uint8_t Athena_RawToB(uint32_t rgba){
    return (rgba >> 16) & 0xFF;
}

uint8_t Athena_RawToA(uint32_t rgba){
    return (rgba >> 24) & 0xFF;
}

#define ATHENA_DECONSTRUCT_BLENDER(NAME)\
uint32_t Athena_RGBARaw ## NAME(uint32_t src, uint32_t dst){\
    uint8_t src_r, src_g, src_b, src_a, dst_r, dst_g, dst_b, dst_a;\
    Athena_RawToRGBA(src, &src_r, &src_g, &src_b, &src_a);\
    Athena_RawToRGBA(dst, &dst_r, &dst_g, &dst_b, &dst_a);\
    return Athena_RGBA ## NAME(src_r, src_g, src_b, src_a, dst_r, dst_g, dst_b, dst_a);\
}

ATHENA_DECONSTRUCT_BLENDER(Blend)
ATHENA_DECONSTRUCT_BLENDER(Multiply)

#undef ATHENA_DECONSTRUCT_BLENDER

uint32_t Athena_RGBABlend(uint8_t src_r, uint8_t src_g, uint8_t src_b, uint8_t src_a, uint8_t dst_r, uint8_t dst_g, uint8_t dst_b, uint8_t dst_a){
    float accum_r = ((float)dst_r)/255.0f, accum_g = ((float)dst_g)/255.0f, accum_b = ((float)dst_b)/255.0f;
    
    const float src_factor = ((float)src_a)/255.0f, dst_factor = 1.0f - src_factor;
    
    accum_r = (accum_r * dst_factor) + ((((float)src_r)/255.0f) * src_factor);
    accum_g = (accum_g * dst_factor) + ((((float)src_g)/255.0f) * src_factor);
    accum_b = (accum_b * dst_factor) + ((((float)src_b)/255.0f) * src_factor);

    return Athena_RGBAToRaw(accum_r * 255.0f, accum_g * 255.0f, accum_b * 255.0f, 0xFF);
}

uint32_t Athena_RGBAMultiply(uint8_t src_r, uint8_t src_g, uint8_t src_b, uint8_t src_a, uint8_t dst_r, uint8_t dst_g, uint8_t dst_b, uint8_t dst_a){
    const uint16_t mul_r = src_r * dst_r, 
        mul_g = src_g * dst_g, 
        mul_b = src_b * dst_b, 
        mul_a = src_a * dst_a;

    return Athena_RGBAToRaw(mul_r / 65025, mul_g / 65025, mul_b / 65025, mul_a / 65025);
}

unsigned Athena_LoadAuto(struct Athena_Image *to, const char *path){
    const char * const end = path + strlen(path), *str = end;
    while(str[0] != '.'){
        if(str==path)
            return ATHENA_LOADPNG_NFORMAT;
        str--;
    }
    
    if(end - str == 4){
        if(memcmp(str, ".png", 4)==0)
            return Athena_LoadPNG(to, path);
        else if(memcmp(str, ".tga", 4)==0)
            return Athena_LoadTGA(to, path);
    }
    return ATHENA_LOADPNG_NFORMAT;
}
