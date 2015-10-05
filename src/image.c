#include "image.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

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
    memcpy(dst->pixels + x + (y * dst->w), src->pixels + (line * src->w), len<<2);
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