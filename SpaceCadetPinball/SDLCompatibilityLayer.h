#pragma once
#include "SDL.h"

#define Mix_LoadMUS_RW(rw, freesrc) Mix_LoadMUSType_RW(rw, MUS_NONE, freesrc)

#define SDL_TICKS_PASSED(A, B)  ((Sint32)((B) - (A)) <= 0)

typedef int (SDLCALL * SDLCompat_SDL2EventFilter) (void *userdata, SDL_Event * event);

#define SDL_INIT_EVENTS 0
#define SDL_INIT_GAMECONTROLLER 0
// Don't. We don't have support for joysticks nor for controllers.
#undef SDL_INIT_JOYSTICK
#define SDL_INIT_JOYSTICK 0

#define SDL_RENDERER_ACCELERATED 0
#define SDL_RENDERER_SOFTWARE 0

#define SDL_WINDOWPOS_UNDEFINED_MASK    0x1FFF0000u
#define SDL_WINDOWPOS_UNDEFINED_DISPLAY(X)  (SDL_WINDOWPOS_UNDEFINED_MASK|(X))
#define SDL_WINDOWPOS_UNDEFINED         SDL_WINDOWPOS_UNDEFINED_DISPLAY(0)
#define SDL_WINDOW_HIDDEN 0
#define SDL_WINDOW_RESIZABLE SDL_RESIZABLE
#define SDL_WINDOW_FULLSCREEN_DESKTOP 0

#define SDLK_SCROLLLOCK SDLK_SCROLLOCK
#define SDLK_NUMLOCKCLEAR SDLK_NUMLOCK
#define SDLK_PRINTSCREEN SDLK_PRINT
#define SDLK_KP_0 SDLK_KP0
#define SDLK_KP_1 SDLK_KP1
#define SDLK_KP_2 SDLK_KP2
#define SDLK_KP_3 SDLK_KP3
#define SDLK_KP_4 SDLK_KP4
#define SDLK_KP_5 SDLK_KP5
#define SDLK_KP_6 SDLK_KP6
#define SDLK_KP_7 SDLK_KP7
#define SDLK_KP_8 SDLK_KP8
#define SDLK_KP_9 SDLK_KP9
#define SDLK_LGUI SDLK_LSUPER
#define SDLK_RGUI SDLK_RSUPER
#define SDLK_APPLICATION SDLK_MENU  // !! Not sure.
#define SDL_Keymod SDLMod
#define KMOD_GUI KMOD_META  // !! Unfortunately Super (Windows logo) keys do not work as modifier keys in SDL 1.2.

#undef SDL_TEXTINPUT  // No IME support for you.
#define SDL_MOUSEWHEEL SDL_EVENT_RESERVED2
/*
    Reimplement mouse wheel event handler like this:

    > switch (event->type)
    > {
    >   case SDL_MOUSEBUTTONDOWN:
    >   case SDL_MOUSEBUTTONUP:
    >   {
    >       if (event->button.button == SDL_BUTTON_WHEELUP || event->button.button == SDL_BUTTON_WHEELDOWN) {
    >           float wheel_x = 0.0f;
    >           float wheel_y = (event->button.button == SDL_BUTTON_WHEELUP) ? 1.0f : -1.0f;
    >           io.AddMouseWheelEvent(wheel_x, wheel_y);
    >       } else {
    >           // Handle other mouse buttons here
    >       }
    > ...
*/

typedef enum
{
    SDL_CONTROLLER_BUTTON_INVALID = -1,
    SDL_CONTROLLER_BUTTON_A,
    SDL_CONTROLLER_BUTTON_B,
    SDL_CONTROLLER_BUTTON_X,
    SDL_CONTROLLER_BUTTON_Y,
    SDL_CONTROLLER_BUTTON_BACK,
    SDL_CONTROLLER_BUTTON_GUIDE,
    SDL_CONTROLLER_BUTTON_START,
    SDL_CONTROLLER_BUTTON_LEFTSTICK,
    SDL_CONTROLLER_BUTTON_RIGHTSTICK,
    SDL_CONTROLLER_BUTTON_LEFTSHOULDER,
    SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,
    SDL_CONTROLLER_BUTTON_DPAD_UP,
    SDL_CONTROLLER_BUTTON_DPAD_DOWN,
    SDL_CONTROLLER_BUTTON_DPAD_LEFT,
    SDL_CONTROLLER_BUTTON_DPAD_RIGHT,
    SDL_CONTROLLER_BUTTON_MISC1,    /* Xbox Series X share button, PS5 microphone button, Nintendo Switch Pro capture button, Amazon Luna microphone button */
    SDL_CONTROLLER_BUTTON_PADDLE1,  /* Xbox Elite paddle P1 (upper left, facing the back) */
    SDL_CONTROLLER_BUTTON_PADDLE2,  /* Xbox Elite paddle P3 (upper right, facing the back) */
    SDL_CONTROLLER_BUTTON_PADDLE3,  /* Xbox Elite paddle P2 (lower left, facing the back) */
    SDL_CONTROLLER_BUTTON_PADDLE4,  /* Xbox Elite paddle P4 (lower right, facing the back) */
    SDL_CONTROLLER_BUTTON_TOUCHPAD, /* PS4/PS5 touchpad button */
    SDL_CONTROLLER_BUTTON_MAX
} SDL_GameControllerButton;

#define SDL_WINDOWEVENT SDL_ACTIVEEVENT
#define SDL_RENDER_TARGETS_RESET SDL_EVENT_RESERVED2  // This should never be triggered since "Events SDL_USEREVENT through SDL_MAXEVENTS-1 are for your use".
#define SDL_CONTROLLERBUTTONDOWN SDL_EVENT_RESERVED2
#define SDL_CONTROLLERBUTTONUP SDL_EVENT_RESERVED3
#define SDL_JOYDEVICEADDED SDL_EVENT_RESERVED2
#define SDL_JOYDEVICEREMOVED SDL_EVENT_RESERVED3
typedef enum
{
    SDL_WINDOWEVENT_NONE,           /**< Never used */
    SDL_WINDOWEVENT_SHOWN,          /**< Window has been shown */
    SDL_WINDOWEVENT_HIDDEN,         /**< Window has been hidden */
    SDL_WINDOWEVENT_EXPOSED,        /**< Window has been exposed and should be
                                         redrawn */
    SDL_WINDOWEVENT_MOVED,          /**< Window has been moved to data1, data2
                                     */
    SDL_WINDOWEVENT_RESIZED,        /**< Window has been resized to data1xdata2 */
    SDL_WINDOWEVENT_SIZE_CHANGED,   /**< The window size has changed, either as
                                         a result of an API call or through the
                                         system or user changing the window size. */
    SDL_WINDOWEVENT_MINIMIZED,      /**< Window has been minimized */
    SDL_WINDOWEVENT_MAXIMIZED,      /**< Window has been maximized */
    SDL_WINDOWEVENT_RESTORED,       /**< Window has been restored to normal size
                                         and position */
    SDL_WINDOWEVENT_ENTER,          /**< Window has gained mouse focus */
    SDL_WINDOWEVENT_LEAVE,          /**< Window has lost mouse focus */
    SDL_WINDOWEVENT_FOCUS_GAINED,   /**< Window has gained keyboard focus */
    SDL_WINDOWEVENT_FOCUS_LOST,     /**< Window has lost keyboard focus */
    SDL_WINDOWEVENT_CLOSE,          /**< The window manager requests that the window be closed */
    SDL_WINDOWEVENT_TAKE_FOCUS,     /**< Window is being offered a focus (should SetWindowInputFocus() on itself or a subwindow, or ignore) */
    SDL_WINDOWEVENT_HIT_TEST,       /**< Window had a hit test that wasn't SDL_HITTEST_NORMAL. */
    SDL_WINDOWEVENT_ICCPROF_CHANGED,/**< The ICC profile of the window's display has changed. */
    SDL_WINDOWEVENT_DISPLAY_CHANGED /**< Window has been moved to display data1. */
} SDL_WindowEventID;
/*
    Do something like this:

    > switch (event->type)
    > {
    >   case SDL_WINDOWEVENT:
    >   {
    >     Uint8 window_event;
    >     if (event->active.state & SDL_APPINPUTFOCUS)
    >         window_event = event->active.gain ? SDL_WINDOWEVENT_FOCUS_GAINED : SDL_WINDOWEVENT_FOCUS_LOST;
    >     if (event->active.state & SDL_APPMOUSEFOCUS)
    >         window_event = event->active.gain ? SDL_WINDOWEVENT_ENTER : SDL_WINDOWEVENT_LEAVE;
    > ...
*/

inline char *SDL_GetClipboardText() { return ""; };
inline int SDL_SetClipboardText(const char *) { return 0; };
inline void SDL_SetMainReady() {};

typedef enum
{
    SDL_SYSTEM_CURSOR_ARROW,     /**< Arrow */
    SDL_SYSTEM_CURSOR_IBEAM,     /**< I-beam */
    SDL_SYSTEM_CURSOR_WAIT,      /**< Wait */
    SDL_SYSTEM_CURSOR_CROSSHAIR, /**< Crosshair */
    SDL_SYSTEM_CURSOR_WAITARROW, /**< Small wait cursor (or Wait if not available) */
    SDL_SYSTEM_CURSOR_SIZENWSE,  /**< Double arrow pointing northwest and southeast */
    SDL_SYSTEM_CURSOR_SIZENESW,  /**< Double arrow pointing northeast and southwest */
    SDL_SYSTEM_CURSOR_SIZEWE,    /**< Double arrow pointing west and east */
    SDL_SYSTEM_CURSOR_SIZENS,    /**< Double arrow pointing north and south */
    SDL_SYSTEM_CURSOR_SIZEALL,   /**< Four pointed arrow pointing north, south, east, and west */
    SDL_SYSTEM_CURSOR_NO,        /**< Slashed circle or crossbones */
    SDL_SYSTEM_CURSOR_HAND,      /**< Hand */
    SDL_NUM_SYSTEM_CURSORS
} SDL_SystemCursor;

// Always returns a new default cursor.
SDL_Cursor *SDL_CreateSystemCursor(SDL_SystemCursor);

typedef enum
{
    SDL_MESSAGEBOX_ERROR                 = 0x00000010,   /**< error dialog */
    SDL_MESSAGEBOX_WARNING               = 0x00000020,   /**< warning dialog */
    SDL_MESSAGEBOX_INFORMATION           = 0x00000040,   /**< informational dialog */
    SDL_MESSAGEBOX_BUTTONS_LEFT_TO_RIGHT = 0x00000080,   /**< buttons placed left to right */
    SDL_MESSAGEBOX_BUTTONS_RIGHT_TO_LEFT = 0x00000100    /**< buttons placed right to left */
} SDL_MessageBoxFlags;

typedef SDL_Surface SDL_Texture;

typedef struct SDL_RendererInfo
{
    const char *name;           /**< The name of the renderer */
} SDL_RendererInfo;

typedef enum
{
    SDL_BLENDMODE_NONE = 0x00000000,     /**< no blending
                                              dstRGBA = srcRGBA */
    SDL_BLENDMODE_BLEND = 0x00000001,    /**< alpha blending
                                              dstRGB = (srcRGB * srcA) + (dstRGB * (1-srcA))
                                              dstA = srcA + (dstA * (1-srcA)) */
    SDL_BLENDMODE_ADD = 0x00000002,      /**< additive blending
                                              dstRGB = (srcRGB * srcA) + dstRGB
                                              dstA = dstA */
    SDL_BLENDMODE_MOD = 0x00000004,      /**< color modulate
                                              dstRGB = srcRGB * dstRGB
                                              dstA = dstA */
    SDL_BLENDMODE_MUL = 0x00000008,      /**< color multiply
                                              dstRGB = (srcRGB * dstRGB) + (dstRGB * (1-srcA))
                                              dstA = dstA */
    SDL_BLENDMODE_INVALID = 0x7FFFFFFF

    /* Additional custom blend modes can be returned by SDL_ComposeCustomBlendMode() */

} SDL_BlendMode;

typedef enum {
    SDL_PIXELFORMAT_RGBA8888,
    SDL_PIXELFORMAT_ABGR8888,
    SDL_PIXELFORMAT_BGRA8888,
    SDL_PIXELFORMAT_ARGB8888,

    SDL_PIXELFORMAT_INDEX8,

// ____32 formats' memory layout is exactly as its name and platform-independent,
// while ____8888 ones have their byte order that the name implies reversed on little-endian systems.
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    SDL_PIXELFORMAT_RGBA32 = SDL_PIXELFORMAT_RGBA8888,
    SDL_PIXELFORMAT_BGRA32 = SDL_PIXELFORMAT_BGRA8888,
    SDL_PIXELFORMAT_ARGB32 = SDL_PIXELFORMAT_ARGB8888
#else
    SDL_PIXELFORMAT_RGBA32 = SDL_PIXELFORMAT_ABGR8888,
    SDL_PIXELFORMAT_BGRA32 = SDL_PIXELFORMAT_ARGB8888,
    SDL_PIXELFORMAT_ARGB32 = SDL_PIXELFORMAT_BGRA8888
#endif
} SDLCompat_PixelFormatEnum;

typedef struct SDL_Renderer {
    SDL_Color DrawColor;
    SDL_Surface *InitialRenderTarget;
    SDL_Surface *RenderTarget;
    SDL_bool ClipEnabled;
    SDL_bool ClipEnabledBackup;
    SDL_Rect ClipRect;
    SDL_Rect ClipRectBackup;
    SDL_Surface *ClipSource;  // This is what we actaully draw on with clipping enabled, then the desired clipping area is transferred to RenderTarget.
    SDL_BlendMode BlendMode;
} SDL_Renderer;

typedef struct SDL_Window {
    SDL_Renderer *Renderer;
    SDL_Rect tempSize;  // DO NOT USE, only used to temporarily store size before Renderer creation.
    Uint32 tempFlags;   // Same as above. They do not represent the actual supported video mode.
} SDL_Window;

typedef enum
{
    SDL_TEXTUREACCESS_STATIC,    /**< Changes rarely, not lockable */
    SDL_TEXTUREACCESS_STREAMING, /**< Changes frequently, lockable */
    SDL_TEXTUREACCESS_TARGET     /**< Texture can be used as a render target */
} SDL_TextureAccess;

typedef enum
{
    SDL_FLIP_NONE = 0x00000000,     /**< Do not flip */
    SDL_FLIP_HORIZONTAL = 0x00000001,    /**< flip horizontally */
    SDL_FLIP_VERTICAL = 0x00000002     /**< flip vertically */
} SDL_RendererFlip;

typedef struct SDL_Point
{
    int x;
    int y;
} SDL_Point;

#define SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH "SDL_MOUSE_FOCUS_CLICKTHROUGH"
#define SDL_HINT_MOUSE_AUTO_CAPTURE    "SDL_MOUSE_AUTO_CAPTURE"
#define SDL_HINT_RENDER_SCALE_QUALITY       "SDL_RENDER_SCALE_QUALITY"

const char *SDL_GetHint(const char *);
SDL_bool SDL_SetHint(const char *, const char *);

int SDL_WaitEventTimeout(SDL_Event *, int);
void SDL_AddEventWatch(SDLCompat_SDL2EventFilter, void *);
void SDL_DelEventWatch(SDLCompat_SDL2EventFilter, void *);

SDL_Window *SDL_CreateWindow(const char *, int, int, int, int, Uint32);
void SDL_SetWindowTitle(SDL_Window *, const char *);
void SDL_ShowWindow(SDL_Window *);
int SDL_SetWindowFullscreen(SDL_Window *, Uint32);
void SDL_GetWindowSize(SDL_Window *, int *, int *);
void SDL_SetWindowSize(SDL_Window *, int, int);
void SDL_WarpMouseInWindow(SDL_Window *, int, int);
void SDL_SetWindowGrab(SDL_Window *, SDL_bool);

void SDL_DestroyTexture(SDL_Texture *);
void SDL_DestroyRenderer(SDL_Renderer *);
void SDL_DestroyWindow(SDL_Window *);

SDL_Renderer *SDL_CreateRenderer(SDL_Window *, int, Uint32);
int SDL_GetRendererInfo(SDL_Renderer *, SDL_RendererInfo *);
int SDL_GetRendererOutputSize(SDL_Renderer *, int *, int *);
int SDL_RenderClear(SDL_Renderer *);
int SDL_RenderDrawLine(SDL_Renderer *, int, int, int, int);
int SDL_RenderDrawRect(SDL_Renderer *, const SDL_Rect *);
int SDL_RenderFillRect(SDL_Renderer *, const SDL_Rect *);
void SDL_RenderPresent(SDL_Renderer *);
int SDL_GetRenderDrawColor(SDL_Renderer *, Uint8 *, Uint8 *, Uint8 *, Uint8 *);
int SDL_SetRenderDrawColor(SDL_Renderer *, Uint8, Uint8, Uint8, Uint8);
int SDL_GetRenderDrawBlendMode(SDL_Renderer *, SDL_BlendMode *);
int SDL_SetRenderDrawBlendMode(SDL_Renderer *, SDL_BlendMode);
SDL_bool SDL_RenderIsClipEnabled(SDL_Renderer *);
void SDL_RenderGetClipRect(SDL_Renderer *, SDL_Rect *);
int SDL_RenderSetClipRect(SDL_Renderer *, const SDL_Rect *);
SDL_Texture *SDL_GetRenderTarget(SDL_Renderer *);
int SDL_SetRenderTarget(SDL_Renderer *, SDL_Texture *);
int SDL_RenderDrawPoint(SDL_Renderer *, int, int);
int SDL_RenderDrawPoints(SDL_Renderer *, const SDL_Point *, int);
int SDL_RenderCopy(SDL_Renderer *, SDL_Texture *, const SDL_Rect *, const SDL_Rect *);
int SDL_RenderCopyEx(SDL_Renderer *, SDL_Texture *, const SDL_Rect *, const SDL_Rect *, const double, const SDL_Point *, const SDL_RendererFlip);

SDL_Texture *SDL_CreateTexture(SDL_Renderer *, Uint32, int, int, int);
SDL_Texture *SDL_CreateTextureFromSurface(SDL_Renderer *, SDL_Surface *);
int SDL_GetTextureBlendMode(SDL_Texture *, SDL_BlendMode *);
int SDL_SetTextureBlendMode(SDL_Texture *, SDL_BlendMode);
int SDL_QueryTexture(SDL_Texture *, Uint32 *, int *, int *, int *);
int SDL_LockTexture(SDL_Texture *, const SDL_Rect *, void **, int *);
void SDL_UnlockTexture(SDL_Texture *);
int SDL_UpdateTexture(SDL_Texture *, const SDL_Rect *, const void *, int);
int SDL_SetTextureColorMod(SDL_Texture *, Uint8, Uint8, Uint8);
int SDL_SetPaletteColors(SDL_Palette *, const SDL_Color *, int, int);

char *SDL_GetPrefPath(const char *, const char *);
char *SDL_GetBasePath(void);

Uint64 SDL_GetPerformanceFrequency(void);
Uint64 SDL_GetPerformanceCounter(void);
