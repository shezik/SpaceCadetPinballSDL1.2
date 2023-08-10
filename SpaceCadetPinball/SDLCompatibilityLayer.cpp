#include "SDLCompatibilityLayer.h"
#include "default_cursor.h"
#include "SDL_gfxPrimitives.h"
#include "SDL_rotozoom.h"
#include <cassert>
#include <map>
#include <string>

#include <stdlib.h>
// Unix stuff
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>

/*Use (void) to silent unused warnings.*/
#define assertm(exp, msg) assert(((void)msg, exp))

// Code dependencies copied from SDL 1.2.15, please collapse this!
#pragma region original_stuff

#if 1
    #define DEFAULT_ALPHA_MODE (SDL_SRCALPHA | SDL_RLEACCEL)
#else
    #define DEFAULT_ALPHA_MODE 0
#endif

typedef struct SDL_BlitMap {
	SDL_Surface *dst;
	int identity;
	Uint8 *table;
	SDL_blit hw_blit;
	SDL_blit sw_blit;
	struct private_hwaccel *hw_data;
	struct private_swaccel *sw_data;

	/* the version count matches the destination; mismatch indicates
	   an invalid mapping */
        unsigned int format_version;
} SDL_BlitMap;

static void SDL_InvalidateMap(SDL_BlitMap *map)
{
	if ( ! map ) {
		return;
	}
	map->dst = NULL;
	map->format_version = (unsigned int)-1;
	if ( map->table ) {
		SDL_free(map->table);
		map->table = NULL;
	}
}

static void SDL_FormatChanged(SDL_Surface *surface)
{
	static int format_version = 0;
	++format_version;
	if ( format_version < 0 ) { /* It wrapped... */
		format_version = 1;
	}
	surface->format_version = format_version;
	SDL_InvalidateMap(surface->map);
}

static void SDL_FreeFormat(SDL_PixelFormat *format)
{
	if ( format ) {
		if ( format->palette ) {
			if ( format->palette->colors ) {
				SDL_free(format->palette->colors);
			}
			SDL_free(format->palette);
		}
		SDL_free(format);
	}
}

SDL_PixelFormat *SDL_AllocFormat(int bpp,
			Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask)
{
	SDL_PixelFormat *format;
	Uint32 mask;

	/* Allocate an empty pixel format structure */
	format = (SDL_PixelFormat *) SDL_malloc(sizeof(*format));
	if ( format == NULL ) {
		SDL_OutOfMemory();
		return(NULL);
	}
	SDL_memset(format, 0, sizeof(*format));
	format->alpha = SDL_ALPHA_OPAQUE;

	/* Set up the format */
	format->BitsPerPixel = bpp;
	format->BytesPerPixel = (bpp+7)/8;
	if ( Rmask || Bmask || Gmask ) { /* Packed pixels with custom mask */
		format->palette = NULL;
		format->Rshift = 0;
		format->Rloss = 8;
		if ( Rmask ) {
			for ( mask = Rmask; !(mask&0x01); mask >>= 1 )
				++format->Rshift;
			for ( ; (mask&0x01); mask >>= 1 )
				--format->Rloss;
		}
		format->Gshift = 0;
		format->Gloss = 8;
		if ( Gmask ) {
			for ( mask = Gmask; !(mask&0x01); mask >>= 1 )
				++format->Gshift;
			for ( ; (mask&0x01); mask >>= 1 )
				--format->Gloss;
		}
		format->Bshift = 0;
		format->Bloss = 8;
		if ( Bmask ) {
			for ( mask = Bmask; !(mask&0x01); mask >>= 1 )
				++format->Bshift;
			for ( ; (mask&0x01); mask >>= 1 )
				--format->Bloss;
		}
		format->Ashift = 0;
		format->Aloss = 8;
		if ( Amask ) {
			for ( mask = Amask; !(mask&0x01); mask >>= 1 )
				++format->Ashift;
			for ( ; (mask&0x01); mask >>= 1 )
				--format->Aloss;
		}
		format->Rmask = Rmask;
		format->Gmask = Gmask;
		format->Bmask = Bmask;
		format->Amask = Amask;
	} else if ( bpp > 8 ) {		/* Packed pixels with standard mask */
		/* R-G-B */
		if ( bpp > 24 )
			bpp = 24;
		format->Rloss = 8-(bpp/3);
		format->Gloss = 8-(bpp/3)-(bpp%3);
		format->Bloss = 8-(bpp/3);
		format->Rshift = ((bpp/3)+(bpp%3))+(bpp/3);
		format->Gshift = (bpp/3);
		format->Bshift = 0;
		format->Rmask = ((0xFF>>format->Rloss)<<format->Rshift);
		format->Gmask = ((0xFF>>format->Gloss)<<format->Gshift);
		format->Bmask = ((0xFF>>format->Bloss)<<format->Bshift);
	} else {
		/* Palettized formats have no mask info */
		format->Rloss = 8;
		format->Gloss = 8;
		format->Bloss = 8;
		format->Aloss = 8;
		format->Rshift = 0;
		format->Gshift = 0;
		format->Bshift = 0;
		format->Ashift = 0;
		format->Rmask = 0;
		format->Gmask = 0;
		format->Bmask = 0;
		format->Amask = 0;
	}
	if ( bpp <= 8 ) {			/* Palettized mode */
		int ncolors = 1<<bpp;
#ifdef DEBUG_PALETTE
		fprintf(stderr,"bpp=%d ncolors=%d\n",bpp,ncolors);
#endif
		format->palette = (SDL_Palette *)SDL_malloc(sizeof(SDL_Palette));
		if ( format->palette == NULL ) {
			SDL_FreeFormat(format);
			SDL_OutOfMemory();
			return(NULL);
		}
		(format->palette)->ncolors = ncolors;
		(format->palette)->colors = (SDL_Color *)SDL_malloc(
				(format->palette)->ncolors*sizeof(SDL_Color));
		if ( (format->palette)->colors == NULL ) {
			SDL_FreeFormat(format);
			SDL_OutOfMemory();
			return(NULL);
		}
		if ( Rmask || Bmask || Gmask ) {
			/* create palette according to masks */
			int i;
			int Rm=0,Gm=0,Bm=0;
			int Rw=0,Gw=0,Bw=0;
#ifdef ENABLE_PALETTE_ALPHA
			int Am=0,Aw=0;
#endif
			if(Rmask)
			{
				Rw=8-format->Rloss;
				for(i=format->Rloss;i>0;i-=Rw)
					Rm|=1<<i;
			}
#ifdef DEBUG_PALETTE
			fprintf(stderr,"Rw=%d Rm=0x%02X\n",Rw,Rm);
#endif
			if(Gmask)
			{
				Gw=8-format->Gloss;
				for(i=format->Gloss;i>0;i-=Gw)
					Gm|=1<<i;
			}
#ifdef DEBUG_PALETTE
			fprintf(stderr,"Gw=%d Gm=0x%02X\n",Gw,Gm);
#endif
			if(Bmask)
			{
				Bw=8-format->Bloss;
				for(i=format->Bloss;i>0;i-=Bw)
					Bm|=1<<i;
			}
#ifdef DEBUG_PALETTE
			fprintf(stderr,"Bw=%d Bm=0x%02X\n",Bw,Bm);
#endif
#ifdef ENABLE_PALETTE_ALPHA
			if(Amask)
			{
				Aw=8-format->Aloss;
				for(i=format->Aloss;i>0;i-=Aw)
					Am|=1<<i;
			}
# ifdef DEBUG_PALETTE
			fprintf(stderr,"Aw=%d Am=0x%02X\n",Aw,Am);
# endif
#endif
			for(i=0; i < ncolors; ++i) {
				int r,g,b;
				r=(i&Rmask)>>format->Rshift;
				r=(r<<format->Rloss)|((r*Rm)>>Rw);
				format->palette->colors[i].r=r;

				g=(i&Gmask)>>format->Gshift;
				g=(g<<format->Gloss)|((g*Gm)>>Gw);
				format->palette->colors[i].g=g;

				b=(i&Bmask)>>format->Bshift;
				b=(b<<format->Bloss)|((b*Bm)>>Bw);
				format->palette->colors[i].b=b;

#ifdef ENABLE_PALETTE_ALPHA
				a=(i&Amask)>>format->Ashift;
				a=(a<<format->Aloss)|((a*Am)>>Aw);
				format->palette->colors[i].unused=a;
#else
				format->palette->colors[i].unused=0;
#endif
			}
		} else if ( ncolors == 2 ) {
			/* Create a black and white bitmap palette */
			format->palette->colors[0].r = 0xFF;
			format->palette->colors[0].g = 0xFF;
			format->palette->colors[0].b = 0xFF;
			format->palette->colors[1].r = 0x00;
			format->palette->colors[1].g = 0x00;
			format->palette->colors[1].b = 0x00;
		} else {
			/* Create an empty palette */
			SDL_memset((format->palette)->colors, 0,
				(format->palette)->ncolors*sizeof(SDL_Color));
		}
	}
	return(format);
}

SDL_PixelFormat *SDL_ReallocFormat(SDL_Surface *surface, int bpp,
			Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask)
{
	if ( surface->format ) {
		SDL_FreeFormat(surface->format);
		SDL_FormatChanged(surface);
	}
	surface->format = SDL_AllocFormat(bpp, Rmask, Gmask, Bmask, Amask);
	return surface->format;
}

#pragma endregion

#define SDLCOMPAT_INVALID_FLAG 0x7FFFFFFF
// Create empty surface with identical format, pass SDLCOMPAT_INVALID_FLAG as flags to use flags from source, pass nullptr as rect to use size from source
// OR both SDL_CreateRGBSurface and SDL_SetAlpha flags together. It'll take care of it for you.
static SDL_Surface *SDLCompat_CreateSurfaceDuplicateFormat(Uint32 flags, SDL_Rect *size, SDL_Surface *source) {
    flags = (flags == SDLCOMPAT_INVALID_FLAG) ? source->flags : flags;
    int w = size ? size->w : source->w;
    int h = size ? size->h : source->h;
    SDL_Surface *newSurface = SDL_CreateRGBSurface(flags & (SDL_SWSURFACE | SDL_HWSURFACE | SDL_ASYNCBLIT), \
                                                   w, \
                                                   h, \
                                                   source->format->BitsPerPixel, \
                                                   source->format->Rmask, \
                                                   source->format->Gmask, \
                                                   source->format->Bmask, \
                                                   source->format->Amask);
    newSurface->unused1 = source->unused1;
    SDL_SetAlpha(newSurface, (flags & SDL_SRCALPHA) ? (SDL_SRCALPHA | SDL_RLEACCEL) : 0, SDL_ALPHA_OPAQUE);

    // Copy palette if required
    if (source->format->BitsPerPixel <= 8) {
        int ncolors = source->format->palette->ncolors;
        newSurface->format->palette->ncolors = ncolors;
        SDL_memcpy(newSurface->format->palette->colors, source->format->palette->colors, ncolors * sizeof(SDL_Color));
    }

    return newSurface;
}

SDL_Cursor *SDL_CreateSystemCursor(SDL_SystemCursor) {
    return SDL_CreateCursor(default_cdata, default_cmask,
					        DEFAULT_CWIDTH, DEFAULT_CHEIGHT,
					        DEFAULT_CHOTX, DEFAULT_CHOTY);
}

std::map<std::string, std::string> hints;

const char *SDL_GetHint(const char *name) {
    std::map<std::string, std::string>::iterator iter = hints.find(name);
    if (iter == hints.end())
        return nullptr;
    return hints[name].c_str();
}

SDL_bool SDL_SetHint(const char *name, const char *value) {
    hints[name] = value;
    return SDL_TRUE;
}

static int SDLCompat_GetRotozoomSmoothFlag() {
    const char *hint = SDL_GetHint(SDL_HINT_RENDER_SCALE_QUALITY);
    int defaultVal = SMOOTHING_OFF;

    if (!hint) {
        return defaultVal;
    }
    if (!strcmp(hint, "nearest") || !strcmp(hint, "0")) {
        return SMOOTHING_OFF;
    }
    if (!strcmp(hint, "linear") || !strcmp(hint, "1")) {
        return SMOOTHING_ON;
    }
    if (!strcmp(hint, "best") || !strcmp(hint, "2")) {
        return SMOOTHING_ON;
    }
    return defaultVal;
}

static int windowCount = 0;

SDL_Window *SDL_CreateWindow(const char *title, int, int, int w, int h, Uint32 flags) {
    // !! title??
    assertm(!windowCount, "Creating more than one window");
    windowCount++;
    SDL_Window *newWindow = new SDL_Window{0};
    // !! SDL_Rect size = {0, 0, w, h};
    SDL_Rect size = {0, 0, 480, 272};
    newWindow->tempSize = size;
    newWindow->tempFlags = flags;
    return newWindow;
}

void SDL_SetWindowTitle(SDL_Window *, const char *title) {
    SDL_WM_SetCaption(title, nullptr);
}

void SDL_ShowWindow(SDL_Window *) {};

// Update video mode and InitialRenderTarget
void SDL_SetWindowSize(SDL_Window *window, int w, int h) {
    return;  // !!
    SDL_Renderer *renderer = window->Renderer;

    // If currently on InitialRenderTarget and clipping is enabled, update ClipSource
    bool updateClipSource = renderer->ClipEnabled && (renderer->RenderTarget == renderer->InitialRenderTarget);

    // No need to free InitialRenderTarget
    renderer->InitialRenderTarget = SDL_SetVideoMode(w, h, renderer->InitialRenderTarget->format->BitsPerPixel, renderer->InitialRenderTarget->flags);
    
    if (updateClipSource)
        SDL_RenderSetClipRect(renderer, &renderer->ClipRect);
}

void SDL_DestroyTexture(SDL_Texture *texture) {
    SDL_FreeSurface(texture);
}
void SDL_DestroyRenderer(SDL_Renderer *renderer) {  // SDL_Window still keeps a copy of the free'd renderer!
    // !! Should we free InitialRenderTarget or let SDL do the job?
    SDL_FreeSurface(renderer->InitialRenderTarget); renderer->InitialRenderTarget = nullptr;
    SDL_FreeSurface(renderer->RenderTarget); renderer->RenderTarget = nullptr;
    SDL_RenderSetClipRect(renderer, nullptr);  // Free ClipSource if required.
    renderer->ClipSource = renderer->InitialRenderTarget;  // In case clip was disabled.
}
void SDL_DestroyWindow(SDL_Window *window) {
    if (window) {
        delete window;
        assertm(windowCount, "Too few windows");
        windowCount--;
    }
}

SDL_Renderer *SDL_CreateRenderer(SDL_Window *window, int, Uint32) {
    SDL_Renderer *newRenderer = new SDL_Renderer{0};
    assertm(!window->Renderer, "Renderer already exists for this window");
    // printf("newRenderer pointer: %p\n", newRenderer);
    // printf("Window pointer: %p\n", window);
    // printf("Window width: %d\n", window->tempSize.w);
    // printf("Window height: %d\n", window->tempSize.h);
    // printf("Window flags: %x\n", window->tempFlags);
    newRenderer->ClipSource = newRenderer->RenderTarget = newRenderer->InitialRenderTarget = SDL_SetVideoMode(window->tempSize.w, window->tempSize.h, 16, window->tempFlags);  // !! Hardcoded bpp?
    window->Renderer = newRenderer;
    return newRenderer;
}

int SDL_GetRendererInfo(SDL_Renderer *, SDL_RendererInfo *info) {
    static const char name[] = "SDLegacy Alpha";
    info->name = name;
    return 0;
}

int SDL_GetRendererOutputSize(SDL_Renderer *renderer, int *w, int *h) {
    // printf("Renderer output size: %d x %d\n", renderer->InitialRenderTarget->w, renderer->InitialRenderTarget->h);
    if (w)
        *w = renderer->InitialRenderTarget->w;
    if (h)
        *h = renderer->InitialRenderTarget->h;
    return 0;
}

// Call before any drawing
static int SDLCompat_UpdateClipSource(SDL_Renderer *renderer) {
    if (!renderer->ClipEnabled) {
        assertm(renderer->ClipSource == renderer->RenderTarget, "ClipSource not reset after disabling clipping");
        return 0;
    }

    return SDL_BlitSurface(renderer->RenderTarget, nullptr, renderer->ClipSource, nullptr);
}

// Call after any drawing
static int SDLCompat_BlitClipSource(SDL_Renderer *renderer) {
    if (!renderer->ClipEnabled) {
        assertm(renderer->ClipSource == renderer->RenderTarget, "ClipSource not reset after disabling clipping");
        return 0;
    }

    SDL_Rect dstrect = renderer->ClipRect;  // Temporary copy to be modified by SDL_BlitSurface
    return SDL_BlitSurface(renderer->ClipSource, &renderer->ClipRect, renderer->RenderTarget, &dstrect);  // !! Disable alpha blending?
}

int SDL_RenderClear(SDL_Renderer *renderer) {
    int rtn = 0;
    // printf("RenderTarget: %p\n", renderer->RenderTarget);
    rtn = !rtn ? SDLCompat_UpdateClipSource(renderer) : rtn;
    // printf("ClipSource: %p\n", renderer->ClipSource);
    // printf("ClipSource->format: %p\n", renderer->ClipSource->format);
    // printf("ClipSource->format->BitsPerPixel: %d\n", renderer->ClipSource->format->BitsPerPixel);

    rtn = !rtn ? SDL_FillRect(renderer->ClipSource, \
                              nullptr, \
                              SDL_MapRGBA(renderer->ClipSource->format, \
                                          renderer->DrawColor.r, \
                                          renderer->DrawColor.g, \
                                          renderer->DrawColor.b, \
                                          renderer->DrawColor.unused) \
                              )
               : rtn;

    rtn = !rtn ? SDLCompat_BlitClipSource(renderer) : rtn;
    return rtn;
}
int SDL_RenderDrawLine(SDL_Renderer *renderer, int x1, int y1, int x2, int y2) {
    int rtn = 0;
    SDL_BlendMode blendMode;
    rtn = !rtn ? SDLCompat_UpdateClipSource(renderer) : rtn;
    
    rtn = !rtn ? SDL_GetRenderDrawBlendMode(renderer, &blendMode) : rtn;
    if (SDLCompat_GetRotozoomSmoothFlag() == SMOOTHING_ON)  // !! Is there another hint for this purpose?
        rtn = !rtn ? aalineRGBA(renderer->ClipSource, \
                                x1, y1, \
                                x2, y2, \
                                renderer->DrawColor.r, \
                                renderer->DrawColor.g, \
                                renderer->DrawColor.b, \
                                (blendMode == SDL_BLENDMODE_BLEND) ? renderer->DrawColor.unused : SDL_ALPHA_OPAQUE) \
                   : rtn;  // !! Is this how we deal with the alpha channel?
    else
        rtn = !rtn ? lineRGBA(renderer->ClipSource, \
                              x1, y1, \
                              x2, y2, \
                              renderer->DrawColor.r, \
                              renderer->DrawColor.g, \
                              renderer->DrawColor.b, \
                              (blendMode == SDL_BLENDMODE_BLEND) ? renderer->DrawColor.unused : SDL_ALPHA_OPAQUE) \
                   : rtn;  // !! Is this how we deal with the alpha channel?
    
    rtn = !rtn ? SDLCompat_BlitClipSource(renderer) : rtn;
    return rtn;
}
int SDL_RenderDrawRect(SDL_Renderer *renderer, const SDL_Rect *rect) {
    int rtn = 0;
    SDL_BlendMode blendMode;
    rtn = !rtn ? SDLCompat_UpdateClipSource(renderer) : rtn;
    SDL_Rect rectFull = {0, 0, renderer->ClipSource->w, renderer->ClipSource->h};
    
    if (!rect)
        rect = &rectFull;

    rtn = !rtn ? SDL_GetRenderDrawBlendMode(renderer, &blendMode) : rtn;
    rtn = !rtn ? rectangleRGBA(renderer->ClipSource, \
                               rect->x, \
                               rect->y, \
                               rect->x + rect->w, \
                               rect->y + rect->h, \
                               renderer->DrawColor.r, \
                               renderer->DrawColor.g, \
                               renderer->DrawColor.b, \
                               (blendMode == SDL_BLENDMODE_BLEND) ? renderer->DrawColor.unused : SDL_ALPHA_OPAQUE) \
               : rtn;  // !! Is this how we deal with the alpha channel?
    
    rtn = !rtn ? SDLCompat_BlitClipSource(renderer) : rtn;
    return rtn;
}
int SDL_RenderFillRect(SDL_Renderer *renderer, const SDL_Rect *rect) {
    int rtn = 0;
    SDL_BlendMode blendMode;
    rtn = !rtn ? SDLCompat_UpdateClipSource(renderer) : rtn;
    SDL_Rect rectFull = {0, 0, renderer->ClipSource->w, renderer->ClipSource->h};
    
    if (!rect)
        rect = &rectFull;

    rtn = !rtn ? SDL_GetRenderDrawBlendMode(renderer, &blendMode) : rtn;
    rtn = !rtn ? boxRGBA(renderer->ClipSource, \
                         rect->x, \
                         rect->y, \
                         rect->x + rect->w, \
                         rect->y + rect->h, \
                         renderer->DrawColor.r, \
                         renderer->DrawColor.g, \
                         renderer->DrawColor.b, \
                         (blendMode == SDL_BLENDMODE_BLEND) ? renderer->DrawColor.unused : SDL_ALPHA_OPAQUE) \
               : rtn;  // !! Is this how we deal with the alpha channel?
    
    rtn = !rtn ? SDLCompat_BlitClipSource(renderer) : rtn;
    return rtn;
}
void SDL_RenderPresent(SDL_Renderer *renderer) {
    SDL_Flip(renderer->InitialRenderTarget);  // We could scale stuff here.
}
void SDL_WarpMouseInWindow(SDL_Window *, int x, int y) {
    SDL_WarpMouse(x, y);
    // !! Problem is, SDL_WarpMouse uses global coordinates instead of ones relative to current window.
}
int SDL_SetWindowFullscreen(SDL_Window *, Uint32) {
    // !! Not implemented
    return 1;
};
void SDL_GetWindowSize(SDL_Window *window, int *w, int *h) {
    if (w)
        *w = window->Renderer->InitialRenderTarget->w;
    if (h)
        *h = window->Renderer->InitialRenderTarget->h;
}
void SDL_SetWindowGrab(SDL_Window *, SDL_bool grabbed) {
    SDL_WM_GrabInput(grabbed ? SDL_GRAB_ON : SDL_GRAB_OFF);
}
int SDL_WaitEventTimeout(SDL_Event *event, int timeout) {
    Uint32 start, expiration;
    if (timeout > 0) {
        start = SDL_GetTicks();
        expiration = start + timeout;
    } else {
        start = 0;
        expiration = 0;
    }
    while ( 1 ) {
        SDL_PumpEvents();
        switch(SDL_PeepEvents(event, 1, SDL_GETEVENT, SDL_ALLEVENTS)) {
            case -1: return 0;
            case 1: return 1;
            case 0: break;
        }
        if (timeout <= 0 || SDL_TICKS_PASSED(SDL_GetTicks(), expiration)) {
            return 0;
        }
        SDL_Delay(10);
    }
}

static SDLCompat_SDL2EventFilter SDLCompat_UserEventFilter;
static void *SDLCompat_EventFilterUserdata;
static int SDLCompat_UserEventFilterWrapper(const SDL_Event *event) {
    SDLCompat_UserEventFilter(SDLCompat_EventFilterUserdata, (SDL_Event *) event);
    return 1;  // MUST RETURN TRUE otherwise the event is dropped.
}
void SDL_AddEventWatch(SDLCompat_SDL2EventFilter filter, void *userdata) {
    SDLCompat_UserEventFilter = filter;
    SDLCompat_EventFilterUserdata = userdata;
    SDL_SetEventFilter(filter ? SDLCompat_UserEventFilterWrapper : nullptr);
}

void SDL_DelEventWatch(SDLCompat_SDL2EventFilter, void *) {
    SDL_SetEventFilter(nullptr);
}

static int SDLCompat_ParsePixelFormat(SDLCompat_PixelFormatEnum fmt, int *bpp, Uint32 *Rmask, Uint32 *Gmask, Uint32 *Bmask, Uint32 *Amask) {
    int _bpp;
    Uint32 _Rmask, _Gmask, _Bmask, _Amask;

    if (fmt >= SDL_PIXELFORMAT_RGBA8888 && fmt <= SDL_PIXELFORMAT_ARGB8888) {
        _bpp = 32;
        switch (fmt) {
            case SDL_PIXELFORMAT_RGBA8888:
                _Rmask = 0xff000000;
                _Gmask = 0x00ff0000;
                _Bmask = 0x0000ff00;
                _Amask = 0x000000ff;
                break;
            case SDL_PIXELFORMAT_ABGR8888:
                _Amask = 0xff000000;
                _Bmask = 0x00ff0000;
                _Gmask = 0x0000ff00;
                _Rmask = 0x000000ff;
                break;
            case SDL_PIXELFORMAT_BGRA8888:
                _Bmask = 0xff000000;
                _Gmask = 0x00ff0000;
                _Rmask = 0x0000ff00;
                _Amask = 0x000000ff;
                break;
            case SDL_PIXELFORMAT_ARGB8888:
                _Amask = 0xff000000;
                _Rmask = 0x00ff0000;
                _Gmask = 0x0000ff00;
                _Bmask = 0x000000ff;
                break;
        }
    } else if (fmt == SDL_PIXELFORMAT_INDEX8) {
        _bpp = 8;
        _Rmask = _Gmask = _Bmask = _Amask = 0;
    } else {
        return -1;
    }

    if (bpp)
        *bpp = _bpp;
    if (Rmask)
        *Rmask = _Rmask;
    if (Gmask)
        *Gmask = _Gmask;
    if (Bmask)
        *Bmask = _Bmask;
    if (Amask)
        *Amask = _Amask;

    return 0;
}

// Pixel data of surface remains intact. Better clear the surface first.
static int SDLCompat_ConvertSurfaceFormat(SDL_Surface *surface, SDLCompat_PixelFormatEnum destfmt) {
    // assertm(surface->format->BytesPerPixel > 1, "Cannot work on indexed color surfaces");
    int rtn = 0;

    int bpp;
    Uint32 Rmask, Gmask, Bmask, Amask;

    rtn = !rtn ? SDLCompat_ParsePixelFormat(destfmt, &bpp, &Rmask, &Gmask, &Bmask, &Amask) : rtn;

    if (surface->flags & SDL_HWSURFACE) {
        assertm(surface->format->BitsPerPixel == bpp, "Cannot change bpp on hardware surfaces.");  // Because I don't know how to.
    }

    rtn = !rtn ? -!SDL_ReallocFormat(surface, bpp, Rmask, Gmask, Bmask, Amask) : rtn;
    if (!rtn)
        surface->unused1 = destfmt;
    return rtn;
}

SDL_Texture *SDL_CreateTexture(SDL_Renderer *renderer, Uint32 format, int access, int w, int h) {
    int bpp;
    Uint32 Rmask, Gmask, Bmask, Amask;
    SDL_Texture *newTexture = nullptr;

    int rtn = SDLCompat_ParsePixelFormat((SDLCompat_PixelFormatEnum) format, &bpp, &Rmask, &Gmask, &Bmask, &Amask);
    if (!rtn) {
        newTexture = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, bpp, Rmask, Gmask, Bmask, Amask);
        newTexture->unused1 = format;
        SDL_SetAlpha(newTexture, DEFAULT_ALPHA_MODE, SDL_ALPHA_OPAQUE);
    }

    return newTexture;
}

SDL_Texture *SDL_CreateTextureFromSurface(SDL_Renderer *, SDL_Surface *surface) {
    SDL_Texture *newTexture = SDL_ConvertSurface(surface, surface->format, SDL_SWSURFACE);  // Duplicate surface  // !! SRCALPHA?
    SDL_SetAlpha(newTexture, DEFAULT_ALPHA_MODE, SDL_ALPHA_OPAQUE);  // !! Follow surface?
    newTexture->unused1 = surface->unused1;
    return newTexture;
}

int SDL_GetTextureBlendMode(SDL_Texture *texture, SDL_BlendMode *blendMode) {
    *blendMode = (texture->flags & SDL_SRCALPHA) ? SDL_BLENDMODE_BLEND : SDL_BLENDMODE_NONE;
    return 0;
}

int SDL_SetTextureBlendMode(SDL_Texture *texture, SDL_BlendMode blendMode) {
    int rtn = 0;
    rtn = !rtn ? SDL_SetAlpha(texture, (blendMode & SDL_BLENDMODE_BLEND) ? (SDL_SRCALPHA | SDL_RLEACCEL) : 0, SDL_ALPHA_OPAQUE) : rtn;
    if (blendMode & ~SDL_BLENDMODE_BLEND)
        rtn = !rtn ? -1 : rtn;  // Only alpha blending is supported.
    return rtn;
}

int SDL_GetRenderDrawColor(SDL_Renderer *renderer, Uint8 *r, Uint8 *g, Uint8 *b, Uint8 *a) {
    if (r)
        *r = renderer->DrawColor.r;
    if (g)
        *g = renderer->DrawColor.g;
    if (b)
        *b = renderer->DrawColor.b;
    if (a)
        *a = renderer->DrawColor.unused;
    return 0;
}

int SDL_SetRenderDrawColor(SDL_Renderer *renderer, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    renderer->DrawColor.r = r;
    renderer->DrawColor.g = g;
    renderer->DrawColor.b = b;
    renderer->DrawColor.unused = a;
    return 0;
}

int SDL_GetRenderDrawBlendMode(SDL_Renderer *renderer, SDL_BlendMode *blendMode) {
    if (blendMode)
        *blendMode = renderer->BlendMode;
    return 0;
}

int SDL_SetRenderDrawBlendMode(SDL_Renderer *renderer, SDL_BlendMode blendMode) {
    renderer->BlendMode = (blendMode & SDL_BLENDMODE_BLEND) ? SDL_BLENDMODE_BLEND : SDL_BLENDMODE_NONE;
    if (blendMode & ~SDL_BLENDMODE_BLEND)
        return -1;  // Only alpha blending is supported.
    return 0;
}

SDL_bool SDL_RenderIsClipEnabled(SDL_Renderer *renderer) {
    return renderer->ClipEnabled;
}

void SDL_RenderGetClipRect(SDL_Renderer *renderer, SDL_Rect *rect) {
    if (!rect)
        return;

    if (!renderer->ClipEnabled) {
        *rect = SDL_Rect{0};
        return;
    }
    *rect = renderer->ClipRect;
}

int SDL_RenderSetClipRect(SDL_Renderer *renderer, const SDL_Rect *rect) {
    if (!rect) {
        // Disable and clean up
        if (renderer->ClipEnabled)
            SDL_FreeSurface(renderer->ClipSource);

        renderer->ClipEnabled = SDL_FALSE;
        renderer->ClipSource = renderer->RenderTarget;  // So that anything drawn onto ClipSource is passed down to RenderTarget, effectively bypassing the clip feature.
        return 0;
    }

    if (renderer->ClipEnabled)
        SDL_RenderSetClipRect(renderer, nullptr);

    renderer->ClipEnabled = SDL_TRUE;
    renderer->ClipRect = *rect;
    // renderer->ClipSource = SDL_ConvertSurface(renderer->RenderTarget, renderer->RenderTarget->format, SDL_SWSURFACE);  // Duplicate surface
    renderer->ClipSource = SDLCompat_CreateSurfaceDuplicateFormat(SDL_SWSURFACE, nullptr, renderer->RenderTarget);  // Faster because no blitting required (blank surface)
    return renderer->ClipSource ? 0 : -1;
}

SDL_Texture *SDL_GetRenderTarget(SDL_Renderer *renderer) {
    return (renderer->RenderTarget == renderer->InitialRenderTarget) ? nullptr : renderer->RenderTarget;
}

int SDL_SetRenderTarget(SDL_Renderer *renderer, SDL_Texture *texture) {
    if (renderer->RenderTarget == texture || (!texture && renderer->RenderTarget == renderer->InitialRenderTarget))
        return 0;

    if (texture && renderer->RenderTarget == renderer->InitialRenderTarget) {
        renderer->ClipEnabledBackup = renderer->ClipEnabled;
        renderer->ClipRectBackup = renderer->ClipRect;
    }

    renderer->RenderTarget = texture ? texture : renderer->InitialRenderTarget;
    SDL_RenderSetClipRect(renderer, nullptr);

    if (!texture && renderer->ClipEnabledBackup) {
        SDL_RenderSetClipRect(renderer, &renderer->ClipRectBackup);
    }

    return 0;
}

// !! Check color format?
int SDL_RenderDrawPoint(SDL_Renderer *renderer, int x, int y) {
    int rtn = 0;

    // SDL_BlendMode blendMode;
    // rtn = !rtn ? SDL_GetTextureBlendMode(renderer->RenderTarget, &blendMode) : rtn;  // Follow blend mode of RenderTarget
    Uint8 a = (renderer->BlendMode == SDL_BLENDMODE_BLEND) ? renderer->DrawColor.unused : SDL_ALPHA_OPAQUE;

    rtn = !rtn ? SDLCompat_UpdateClipSource(renderer) : rtn;
    rtn = !rtn ? pixelRGBA(renderer->ClipSource, x, y, renderer->DrawColor.r, renderer->DrawColor.g, renderer->DrawColor.b, a) : rtn;
    rtn = !rtn ? SDLCompat_BlitClipSource(renderer) : rtn;
    return rtn;
}

int SDL_RenderDrawPoints(SDL_Renderer *renderer, const SDL_Point *points, int count) {
    int rtn = 0;
    Uint8 a = (renderer->BlendMode == SDL_BLENDMODE_BLEND) ? renderer->DrawColor.unused : SDL_ALPHA_OPAQUE;
    rtn = !rtn ? SDLCompat_UpdateClipSource(renderer) : rtn;

    for (; count > 0; count--)
        rtn = !rtn ? pixelRGBA(renderer->ClipSource, points[count].x, points[count].y, renderer->DrawColor.r, renderer->DrawColor.g, renderer->DrawColor.b, a) : rtn;

    rtn = !rtn ? SDLCompat_BlitClipSource(renderer) : rtn;
    return rtn;
}

int SDL_RenderCopy(SDL_Renderer *renderer, SDL_Texture *texture, const SDL_Rect *srcrect, const SDL_Rect *dstrect) {
    return SDL_RenderCopyEx(renderer, texture, srcrect, dstrect, 0, nullptr, SDL_FLIP_NONE);
}

int SDL_RenderCopyEx(SDL_Renderer *renderer, SDL_Texture *texture, const SDL_Rect *_srcrect, const SDL_Rect *_dstrect, const double angle, const SDL_Point *, const SDL_RendererFlip flip) {  // !! Cannot specify rotation center yet
    int rtn = 0;
    rtn = !rtn ? SDLCompat_UpdateClipSource(renderer) : rtn;

    SDL_Surface *src, *mid;
    double zoomx, zoomy;
    SDL_Rect srcrectLocal, dstrectLocal, *srcrect, *dstrect;
    SDL_Rect srcrectFull = {0, 0, texture->w, texture->h};
    SDL_Rect dstrectFull = {0, 0, renderer->ClipSource->w, renderer->ClipSource->h};

    srcrectLocal = _srcrect ? *_srcrect : srcrectFull;
    dstrectLocal = _dstrect ? *_dstrect : dstrectFull;
    srcrect = &srcrectLocal;
    dstrect = &dstrectLocal;

    printf("SDL_RenderCopyEx called! srcrect: %d x %d, dstrect: %d x %d, angle: %f, flip: %d, clipping: %d, clipping rect: %d x %d at (%d, %d)", srcrect->w, srcrect->h, dstrect->w, dstrect->h, angle, flip, renderer->ClipEnabled, renderer->ClipRect.w, renderer->ClipRect.h, renderer->ClipRect.x, renderer->ClipRect.y);

    if (/*!angle && !flip && srcrect->w == dstrect->w && srcrect->h == dstrect->h*/true) {
        // Much simpler.
        printf(", taking shortcut\n");
        rtn = !rtn ? SDL_BlitSurface(texture, (SDL_Rect *) srcrect, renderer->ClipSource, (SDL_Rect *) dstrect) : rtn;
        goto end;
    }
    printf("\n");
    
    assertm(srcrect->w && srcrect->h, "Divide by zero caused by srcrect");
    zoomx = (double) dstrect->w / srcrect->w; zoomy = (double) dstrect->h / srcrect->h;
    if (flip & SDL_FLIP_HORIZONTAL)
        zoomx = -zoomx;
    if (flip & SDL_FLIP_VERTICAL)
        zoomy = -zoomy;
    printf("zoomx: %f, zoomy: %f\n", zoomx, zoomy);

    if (angle) {
        if (!rtn) {
            src = SDLCompat_CreateSurfaceDuplicateFormat(SDLCOMPAT_INVALID_FLAG, (SDL_Rect *) srcrect, texture);
            rtn = -!src;
            rtn = !rtn ? SDL_SetAlpha(src, DEFAULT_ALPHA_MODE, SDL_ALPHA_OPAQUE) : rtn;
        }
        rtn = !rtn ? SDL_BlitSurface(texture, (SDL_Rect *) srcrect, src, nullptr) : rtn;
        srcrect = nullptr;
    } else {
        // No rotation required, try to speed up a bit
        src = texture;
        SDL_Rect srcrectNew;
        srcrectNew.w = srcrect->w * abs(zoomx) + 0.5;
        srcrectNew.h = srcrect->h * abs(zoomy) + 0.5;
        srcrectNew.x = zoomx > 0 ? (srcrect->x * zoomx + 0.5) : ((srcrectNew.w - 1) - (srcrect->x * -zoomx) + 0.5);
        srcrectNew.y = zoomy > 0 ? (srcrect->y * zoomy + 0.5) : ((srcrectNew.h - 1) - (srcrect->y * -zoomy) + 0.5);
        srcrectLocal = srcrectNew;
    }

    if (!rtn) {
        if (angle) {
            mid = rotozoomSurfaceXY(src, angle, zoomx, zoomy, SDLCompat_GetRotozoomSmoothFlag());
        } else {
            mid = zoomSurface(src, zoomx, zoomy, SDLCompat_GetRotozoomSmoothFlag());
        }
        rtn = -!mid;
        SDL_SetAlpha(mid, 0, SDL_ALPHA_OPAQUE);  // weird
    }

    if (src != texture) {
        SDL_FreeSurface(src); src = nullptr;
    }
    printf("mid: %d x %d, ClipSource: %d x %d\n", mid->w, mid->h, renderer->ClipSource->w, renderer->ClipSource->h);
    rtn = !rtn ? SDL_BlitSurface(mid, srcrect, renderer->ClipSource, (SDL_Rect *) dstrect) : rtn;
    SDL_FreeSurface(mid); mid = nullptr;

    end:
    rtn = !rtn ? SDLCompat_BlitClipSource(renderer) : rtn;
    return rtn;
}

int SDL_QueryTexture(SDL_Texture *texture, Uint32 *format, int *access, int *w, int *h) {
    if (format)
        *format = texture->unused1;  // !! Need to make sure every surface (texture) this library creates has this set. Especially the ones returned by SDL_gfx.
    if (access)
        *access = 0;  // !! Not implemented
    if (w)
        *w = texture->w;
    if (h)
        *h = texture->h;
    return 0;
}

// Locking an area in the texture is not implemented.
int SDL_LockTexture(SDL_Texture *texture, const SDL_Rect *, void **pixels, int *pitch) {
    int rtn = 0;
    
    if (pitch)
        *pitch = texture->pitch;
    if (SDL_MUSTLOCK(texture))
        rtn = !rtn ? SDL_LockSurface(texture) : rtn;
    if (pixels)
        *pixels = texture->pixels;
    return rtn;
}

void SDL_UnlockTexture(SDL_Texture *texture) {
    if (SDL_MUSTLOCK(texture))
        SDL_UnlockSurface(texture);
}

int SDL_UpdateTexture(SDL_Texture *texture, const SDL_Rect *rect, const void *pixels, int pitch) {
    int rtn = 0;
    SDL_Rect fullRect = {0, 0, texture->w, texture->h};
    if (!rect)
        rect = &fullRect;

    SDL_Surface *sprite = SDL_CreateRGBSurfaceFrom((void *) pixels, \
                                                   rect->w, \
                                                   rect->h, \
                                                   texture->format->BitsPerPixel, \
                                                   pitch, \
                                                   texture->format->Rmask, \
                                                   texture->format->Gmask, \
                                                   texture->format->Bmask, \
                                                   texture->format->Amask);
    rtn = -!sprite;

    rtn = !rtn ? SDL_SetAlpha(sprite, DEFAULT_ALPHA_MODE, SDL_ALPHA_OPAQUE) : rtn;
    rtn = !rtn ? SDL_BlitSurface(sprite, nullptr, texture, (SDL_Rect *) rect) : rtn;
    SDL_FreeSurface(sprite); sprite = nullptr;

    return rtn;
}

int SDL_SetTextureColorMod(SDL_Texture *texture, Uint8 r, Uint8 g, Uint8 b) {
    // !! Not implemented yet
    return 0;
}

// Quite unsafe.
int SDL_SetPaletteColors(SDL_Palette *palette, const SDL_Color *colors, int firstcolor, int ncolors) {
    assertm(palette, "Palette not allocated");
    printf("Copying palette...\n");
    if ( colors != (palette->colors + firstcolor) ) {
		SDL_memcpy(palette->colors + firstcolor, colors,
		       ncolors * sizeof(*colors));
	}
    printf("done!\n");
}

#define SDL_InvalidParamError(param)    SDL_SetError("Parameter '%s' is invalid", (param))

// !! Unix only rn
#pragma region unix_path_stuff

char *SDL_GetPrefPath(const char *org, const char *app)
{
    /*
     * We use XDG's base directory spec, even if you're not on Linux.
     *  This isn't strictly correct, but the results are relatively sane
     *  in any case.
     *
     * http://standards.freedesktop.org/basedir-spec/basedir-spec-latest.html
     */
    const char *envr = SDL_getenv("XDG_DATA_HOME");
    const char *append;
    char *retval = NULL;
    char *ptr = NULL;
    size_t len = 0;

    if (app == NULL) {
        SDL_InvalidParamError("app");
        return NULL;
    }
    if (org == NULL) {
        org = "";
    }

    if (envr == NULL) {
        /* You end up with "$HOME/.local/share/Game Name 2" */
        envr = SDL_getenv("HOME");
        if (envr == NULL) {
            /* we could take heroic measures with /etc/passwd, but oh well. */
            SDL_SetError("neither XDG_DATA_HOME nor HOME environment is set");
            return NULL;
        }
        append = "/.local/share/";
    } else {
        append = "/";
    }

    len = SDL_strlen(envr);
    if (envr[len - 1] == '/') {
        append += 1;
    }

    len += SDL_strlen(append) + SDL_strlen(org) + SDL_strlen(app) + 3;
    retval = (char *)SDL_malloc(len);
    if (retval == NULL) {
        SDL_OutOfMemory();
        return NULL;
    }

    if (*org) {
        (void)SDL_snprintf(retval, len, "%s%s%s/%s/", envr, append, org, app);
    } else {
        (void)SDL_snprintf(retval, len, "%s%s%s/", envr, append, app);
    }

    for (ptr = retval + 1; *ptr; ptr++) {
        if (*ptr == '/') {
            *ptr = '\0';
            if (mkdir(retval, 0700) != 0 && errno != EEXIST) {
                goto error;
            }
            *ptr = '/';
        }
    }
    if (mkdir(retval, 0700) != 0 && errno != EEXIST) {
    error:
        SDL_SetError("Couldn't create directory '%s': '%s'", retval, strerror(errno));
        SDL_free(retval);
        return NULL;
    }

    return retval;
}

/* QNX's /proc/self/exefile is a text file and not a symlink. */
#if !defined(__QNXNTO__)
static char *readSymLink(const char *path)
{
    char *retval = NULL;
    ssize_t len = 64;
    ssize_t rc = -1;

    while (1) {
        char *ptr = (char *)SDL_realloc(retval, (size_t)len);
        if (ptr == NULL) {
            SDL_OutOfMemory();
            break;
        }

        retval = ptr;

        rc = readlink(path, retval, len);
        if (rc == -1) {
            break; /* not a symlink, i/o error, etc. */
        } else if (rc < len) {
            retval[rc] = '\0'; /* readlink doesn't null-terminate. */
            return retval;     /* we're good to go. */
        }

        len *= 2; /* grow buffer, try again. */
    }

    SDL_free(retval);
    return NULL;
}
#endif

char *SDL_GetBasePath(void)
{
    char *retval = NULL;

#if defined(__FREEBSD__)
    char fullpath[PATH_MAX];
    size_t buflen = sizeof(fullpath);
    const int mib[] = { CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1 };
    if (sysctl(mib, SDL_arraysize(mib), fullpath, &buflen, NULL, 0) != -1) {
        retval = SDL_strdup(fullpath);
        if (retval == NULL) {
            SDL_OutOfMemory();
            return NULL;
        }
    }
#endif
#if defined(__OPENBSD__)
    /* Please note that this will fail if the process was launched with a relative path and $PWD + the cwd have changed, or argv is altered. So don't do that. Or add a new sysctl to OpenBSD. */
    char **cmdline;
    size_t len;
    const int mib[] = { CTL_KERN, KERN_PROC_ARGS, getpid(), KERN_PROC_ARGV };
    if (sysctl(mib, 4, NULL, &len, NULL, 0) != -1) {
        char *exe, *pwddst;
        char *realpathbuf = (char *)SDL_malloc(PATH_MAX + 1);
        if (realpathbuf == NULL) {
            SDL_OutOfMemory();
            return NULL;
        }

        cmdline = SDL_malloc(len);
        if (cmdline == NULL) {
            SDL_free(realpathbuf);
            SDL_OutOfMemory();
            return NULL;
        }

        sysctl(mib, 4, cmdline, &len, NULL, 0);

        exe = cmdline[0];
        pwddst = NULL;
        if (SDL_strchr(exe, '/') == NULL) { /* not a relative or absolute path, check $PATH for it */
            exe = search_path_for_binary(cmdline[0]);
        } else {
            if (exe && *exe == '.') {
                const char *pwd = SDL_getenv("PWD");
                if (pwd && *pwd) {
                    SDL_asprintf(&pwddst, "%s/%s", pwd, exe);
                }
            }
        }

        if (exe) {
            if (pwddst == NULL) {
                if (realpath(exe, realpathbuf) != NULL) {
                    retval = realpathbuf;
                }
            } else {
                if (realpath(pwddst, realpathbuf) != NULL) {
                    retval = realpathbuf;
                }
                SDL_free(pwddst);
            }

            if (exe != cmdline[0]) {
                SDL_free(exe);
            }
        }

        if (retval == NULL) {
            SDL_free(realpathbuf);
        }

        SDL_free(cmdline);
    }
#endif

    /* is a Linux-style /proc filesystem available? */
    if (retval == NULL && (access("/proc", F_OK) == 0)) {
        /* !!! FIXME: after 2.0.6 ships, let's delete this code and just
                      use the /proc/%llu version. There's no reason to have
                      two copies of this plus all the #ifdefs. --ryan. */
#if defined(__FREEBSD__)
        retval = readSymLink("/proc/curproc/file");
#elif defined(__NETBSD__)
        retval = readSymLink("/proc/curproc/exe");
#elif defined(__SOLARIS__)
        retval = readSymLink("/proc/self/path/a.out");
#elif defined(__QNXNTO__)
        retval = SDL_LoadFile("/proc/self/exefile", NULL);
#else
        retval = readSymLink("/proc/self/exe"); /* linux. */
        if (retval == NULL) {
            /* older kernels don't have /proc/self ... try PID version... */
            char path[64];
            const int rc = SDL_snprintf(path, sizeof(path),
                                        "/proc/%llu/exe",
                                        (unsigned long long)getpid());
            if ((rc > 0) && (rc < sizeof(path))) {
                retval = readSymLink(path);
            }
        }
#endif
    }

#if defined(__SOLARIS__)  /* try this as a fallback if /proc didn't pan out */
    if (!retval) {
        const char *path = getexecname();
        if ((path != NULL) && (path[0] == '/')) { /* must be absolute path... */
            retval = SDL_strdup(path);
            if (!retval) {
                SDL_OutOfMemory();
                return NULL;
            }
        }
    }
#endif

    /* If we had access to argv[0] here, we could check it for a path,
        or troll through $PATH looking for it, too. */

    if (retval != NULL) { /* chop off filename. */
        char *ptr = SDL_strrchr(retval, '/');
        if (ptr != NULL) {
            *(ptr + 1) = '\0';
        } else { /* shouldn't happen, but just in case... */
            SDL_free(retval);
            retval = NULL;
        }
    }

    if (retval != NULL) {
        /* try to shrink buffer... */
        char *ptr = (char *)SDL_realloc(retval, SDL_strlen(retval) + 1);
        if (ptr != NULL) {
            retval = ptr; /* oh well if it failed. */
        }
    }

    return retval;
}

#pragma endregion

#pragma region time_stuff

#if HAVE_CLOCK_GETTIME
static struct timespec start_ts;
#ifdef CLOCK_MONOTONIC_RAW
#define SDL_MONOTONIC_CLOCK CLOCK_MONOTONIC_RAW
#else
#define SDL_MONOTONIC_CLOCK CLOCK_MONOTONIC
#endif
#endif

static SDL_bool ticks_started = SDL_FALSE;
static SDL_bool has_monotonic_time = SDL_FALSE;
static void SDL_TicksInit() {
    if (ticks_started) {
        return;
    }
    ticks_started = SDL_TRUE;
    #if HAVE_CLOCK_GETTIME
    if (clock_gettime(SDL_MONOTONIC_CLOCK, &start_ts) == 0) {
        has_monotonic_time = SDL_TRUE;
    }
    #endif
}

Uint64 SDL_GetPerformanceFrequency(void)
{
    if (!ticks_started) {
        SDL_TicksInit();
    }

    if (has_monotonic_time) {
#if HAVE_CLOCK_GETTIME
        return 1000000000;
#elif defined(__APPLE__)
        Uint64 freq = mach_base_info.denom;
        freq *= 1000000000;
        freq /= mach_base_info.numer;
        return freq;
#endif
    }

    return 1000000;
}

Uint64 SDL_GetPerformanceCounter(void)
{
    Uint64 ticks;
    if (!ticks_started) {
        SDL_TicksInit();
    }

    if (has_monotonic_time) {
#if HAVE_CLOCK_GETTIME
        struct timespec now;

        clock_gettime(SDL_MONOTONIC_CLOCK, &now);
        ticks = now.tv_sec;
        ticks *= 1000000000;
        ticks += now.tv_nsec;
#elif defined(__APPLE__)
        ticks = mach_absolute_time();
#else
        /*SDL_assert*/assert(SDL_FALSE);
        ticks = 0;
#endif
    } else {
        struct timeval now;

        gettimeofday(&now, NULL);
        ticks = now.tv_sec;
        ticks *= 1000000;
        ticks += now.tv_usec;
    }
    return ticks;
}

#pragma endregion
