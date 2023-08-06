#include "SDLCompatibilityLayer.h"
#include "default_cursor.h"
#include "SDL_gfxPrimitives.h"
#include "SDL_rotozoom.h"
#include <cassert>
#include <map>
#include <string>

/*Use (void) to silent unused warnings.*/
#define assertm(exp, msg) assert(((void)msg, exp))

// Code dependencies copied from SDL 1.2.15, please collapse this!
#pragma region original_stuff

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
static SDL_Surface *SDLCompat_CreateSurfaceDuplicateFormat(Uint32 flags, SDL_Rect *size, SDL_Surface *source) {
    flags = (flags == SDLCOMPAT_INVALID_FLAG) ? source->flags : flags;
    int w = size ? size->w : source->w;
    int h = size ? size->h : source->h;
    SDL_Surface *newSurface = SDL_CreateRGBSurface(flags, \
                                                   w, \
                                                   h, \
                                                   source->format->BitsPerPixel, \
                                                   source->format->Rmask, \
                                                   source->format->Gmask, \
                                                   source->format->Bmask, \
                                                   source->format->Amask);
    newSurface->unused1 = source->unused1;
    return newSurface;
}

char *SDL_GetClipboardText() { return nullptr; };
int SDL_SetClipboardText(const char *) { return 0; };
void SDL_SetMainReady() {};

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
    assertm(!windowCount, "Creating more than one window");
    windowCount++;
    SDL_Window *newWindow = new SDL_Window{0};
    SDL_Rect size = {0, 0, w, h};
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
    newRenderer->RenderTarget = newRenderer->InitialRenderTarget = SDL_SetVideoMode(window->tempSize.w, window->tempSize.h, 16, window->tempFlags);  // !! Hardcoded bpp?
    window->Renderer = newRenderer;
    return newRenderer;
}

int SDL_GetRendererInfo(SDL_Renderer *, SDL_RendererInfo *info) {
    static const char name[] = "SDLegacy Alpha";
    info->name = name;
    return 0;
}

int SDL_RenderClear(SDL_Renderer *renderer) {
    int rtn = 0;
    rtn = !rtn ? SDLCompat_UpdateClipSource(renderer) : rtn;

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
    SDL_Flip(renderer->InitialRenderTarget);
}
void SDL_WarpMouseInWindow(SDL_Window *, int x, int y) {
    SDL_WarpMouse(x, y);
    // !! Problem is, SDL_WarpMouse uses global coordinates instead of ones relative to current window.
}
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
    } else {
        // Other non-bpp-32 format goes here
        return -1;  // But they are not implemented yet
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
        newTexture = SDL_CreateRGBSurface(SDL_SWSURFACE | SDL_RLEACCEL, w, h, bpp, Rmask, Gmask, Bmask, Amask);
        newTexture->unused1 = format;
    }

    return newTexture;
}

SDL_Texture *SDL_CreateTextureFromSurface(SDL_Renderer *, SDL_Surface *surface) {
    SDL_Texture *newTexture = SDL_ConvertSurface(surface, surface->format, SDL_SWSURFACE | SDL_SRCALPHA | SDL_RLEACCEL);  // Duplicate surface  // !! SRCALPHA?
    newTexture->unused1 = surface->unused1;
    return newTexture;
}

int SDL_GetTextureBlendMode(SDL_Texture *texture, SDL_BlendMode *blendMode) {
    *blendMode = (texture->flags & SDL_BLENDMODE_BLEND) ? SDL_BLENDMODE_BLEND : SDL_BLENDMODE_NONE;
    return 0;
}

int SDL_SetTextureBlendMode(SDL_Texture *texture, SDL_BlendMode blendMode) {
    int rtn = 0;
    rtn = !rtn ? SDL_SetAlpha(texture, SDL_RLEACCEL | ((blendMode & SDL_BLENDMODE_BLEND) ? SDL_SRCALPHA : 0), SDL_ALPHA_OPAQUE) : rtn;
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
        if (renderer->ClipEnabled) {
            // Disable and clean up
            renderer->ClipEnabled = SDL_FALSE;
            SDL_FreeSurface(renderer->ClipSource);
            renderer->ClipSource = renderer->RenderTarget;  // So that anything drawn onto ClipSource is passed down to RenderTarget, effectively bypassing the clip feature.
        }
        return 0;
    }

    if (renderer->ClipEnabled)
        SDL_RenderSetClipRect(renderer, nullptr);

    renderer->ClipEnabled = SDL_TRUE;
    renderer->ClipRect = *rect;
    // renderer->ClipSource = SDL_ConvertSurface(renderer->RenderTarget, renderer->RenderTarget->format, SDL_SWSURFACE);  // Duplicate surface
    renderer->ClipSource = SDLCompat_CreateSurfaceDuplicateFormat(SDL_SWSURFACE | SDL_SRCALPHA | SDL_RLEACCEL, nullptr, renderer->RenderTarget);  // Faster because no blitting required (blank surface)
    return renderer->ClipSource ? 0 : -1;
}

// Call before any drawing
static int SDLCompat_UpdateClipSource(SDL_Renderer *renderer) {
    if (!renderer->ClipEnabled)
        return 0;

    return SDL_BlitSurface(renderer->RenderTarget, nullptr, renderer->ClipSource, nullptr);
}

// Call after any drawing
static int SDLCompat_BlitClipSource(SDL_Renderer *renderer) {
    if (!renderer->ClipEnabled)
        return 0;

    SDL_Rect dstrect = renderer->ClipRect;  // Temporary copy to be modified by SDL_BlitSurface
    return SDL_BlitSurface(renderer->ClipSource, &renderer->ClipRect, renderer->RenderTarget, &dstrect);
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

    SDL_BlendMode prevBlendMode;
    rtn = !rtn ? SDL_GetTextureBlendMode(texture, &prevBlendMode) : rtn;
    rtn = !rtn ? SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_NONE) : rtn;

    rtn = !rtn ? SDL_BlitSurface(sprite, nullptr, texture, (SDL_Rect *) rect) : rtn;

    rtn = !rtn ? SDL_SetTextureBlendMode(texture, prevBlendMode) : rtn;
    return rtn;
}

int SDL_SetTextureColorMod(SDL_Texture *texture, Uint8 r, Uint8 g, Uint8 b) {
    // !! Not implemented yet
    return 0;
}
