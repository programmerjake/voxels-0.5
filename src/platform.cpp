/*
 * Voxels is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Voxels is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Voxels; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */
#include "platform.h"
#include <SDL.h>
#include <SDL_image.h>
#include <GL/gl.h>
#include "matrix.h"
#include "vector.h"
#include <cwchar>
#include <string>
#include <iostream>
#include <cstdlib>
#include <chrono>
#include <atomic>
#include <thread>
#include <mutex>

using namespace std;

double Display::realtimeTimer()
{
    return static_cast<double>(chrono::duration_cast<chrono::nanoseconds>(chrono::system_clock::now().time_since_epoch()).count()) * 1e-9;
}

static wstring ResourcePrefix;
static wstring getExecutablePath();

wstring getResourceFileName(wstring resource)
{
    return ResourcePrefix + resource;
}

#ifdef _WIN64
#error implement getExecutablePath for Win64
#elif _WIN32
#error implement getExecutablePath for Win32
#elif __ANDROID
#error implement getExecutablePath for Android
#elif __APPLE__
#include "TargetConditionals.h"
#if TARGET_OS_IPHONE && TARGET_IPHONE_SIMULATOR
#error implement getExecutablePath for iPhone simulator
#elif TARGET_OS_IPHONE
#error implement getExecutablePath for iPhone
#else
#error implement getExecutablePath for OS X
#endif
#elif __linux
#include <unistd.h>
#include <climits>
#include <cerrno>
#include <cstring>
#include <cwchar>
static wstring getExecutablePath()
{
    char buf[PATH_MAX + 1];
    ssize_t rv = readlink("/proc/self/exe", &buf[0], PATH_MAX);
    if(rv == -1)
    {
        throw new runtime_error(string("can't get executable path : ") + strerror(errno));
    }
    buf[rv] = '\0';
    return mbsrtowcs(&buf[0]);
}
#elif __unix
#error implement getExecutablePath for other unix
#elif __posix
#error implement getExecutablePath for other posix
#else
#error unknown platform in getExecutablePath
#endif

initializer initializer1([]()
{
    wstring p = getExecutablePath();
    size_t pos = p.find_last_of(L"/\\");
    if(pos == wstring::npos)
        p = L"";
    else
        p = p.substr(0, pos + 1);
    ResourcePrefix = p + L"res/";
});

static const auto ImageDecoderFlags = IMG_INIT_PNG;

static int xResInternal, yResInternal;

static SDL_Surface *videoSurface = nullptr;

initializer initializer2([]()
{
    if(0 != SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_AUDIO))
    {
        cerr << "error : can't start SDL : " << SDL_GetError() << endl;
        exit(1);
    }
    atexit(SDL_Quit);
    if(ImageDecoderFlags != (ImageDecoderFlags & IMG_Init(ImageDecoderFlags)))
    {
        cerr << "error : can't start SDLImage : " << IMG_GetError() << endl;
        exit(1);
    }
    atexit(IMG_Quit);
#if 0
    const(SDL_VideoInfo) * vidInfo = SDL_GetVideoInfo();
    if(vidInfo is null)
    {
        xResInternal = 800;
        yResInternal = 600;
    }
    else
    {
        xResInternal = vidInfo.current_w;
        yResInternal = vidInfo.current_h;
    }
#else
    xResInternal = 1024;
    yResInternal = 768;
#endif
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    videoSurface = SDL_SetVideoMode(xResInternal, yResInternal, 32, SDL_OPENGL);
    if(videoSurface == nullptr)
    {
        cerr << "error : can't set video mode : " << SDL_GetError();
        exit(1);
    }
    SDL_EnableUNICODE(1);
    SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
});

static volatile double lastFlipTime = 0;

static volatile double oldLastFlipTime = 0;

static recursive_mutex flipTimeLock;

static void lockFlipTime()
{
    flipTimeLock.lock();
}

static void unlockFlipTime()
{
    flipTimeLock.unlock();
}

struct FlipTimeLocker
{
    FlipTimeLocker()
    {
        lockFlipTime();
    }
    ~FlipTimeLocker()
    {
        unlockFlipTime();
    }
};

initializer initializer3([]()
{
    lastFlipTime = Display::realtimeTimer();
    const float fps = defaultFPS;
    oldLastFlipTime = lastFlipTime - static_cast<double>(1) / fps;
});

static volatile float averageFPSInternal = defaultFPS;

static double instantaneousFPS()
{
    FlipTimeLocker lock;
    double delta = lastFlipTime - oldLastFlipTime;
    if(delta <= eps || delta > 0.25)
    {
        return averageFPSInternal;
    }
    return 1 / delta;
}

static double frameDeltaTime()
{
    FlipTimeLocker lock;
    double delta = lastFlipTime - oldLastFlipTime;
    if(delta <= eps || delta > 0.25)
    {
        return 1 / averageFPSInternal;
    }
    return delta;
}

const float FPSUpdateFactor = 0.1f;

static float averageFPS()
{
    FlipTimeLocker lock;
    return averageFPSInternal;
}

static void flipDisplay(float fps = defaultFPS)
{
    double sleepTime;
    {
        FlipTimeLocker lock;
        double curTime = Display::realtimeTimer();
        sleepTime = 1 / fps - (curTime - lastFlipTime);
        if(sleepTime <= eps)
        {
            oldLastFlipTime = lastFlipTime;
            lastFlipTime = curTime;
            averageFPSInternal *= 1 - FPSUpdateFactor;
            averageFPSInternal += FPSUpdateFactor * instantaneousFPS();
        }
    }
    if(sleepTime > eps)
    {
        this_thread::sleep_for(chrono::nanoseconds(static_cast<int64_t>(sleepTime * 1e9)));
        {
            FlipTimeLocker lock;
            oldLastFlipTime = lastFlipTime;
            lastFlipTime = Display::realtimeTimer();
            averageFPSInternal *= 1 - FPSUpdateFactor;
            averageFPSInternal += FPSUpdateFactor * instantaneousFPS();
        }
    }
    SDL_GL_SwapBuffers();
}

static KeyboardKey translateKey(SDLKey input)
{
    switch(input)
    {
    case SDLK_BACKSPACE:
        return KeyboardKey_Backspace;
    case SDLK_TAB:
        return KeyboardKey_Tab;
    case SDLK_CLEAR:
        return KeyboardKey_Clear;
    case SDLK_RETURN:
        return KeyboardKey_Return;
    case SDLK_PAUSE:
        return KeyboardKey_Pause;
    case SDLK_ESCAPE:
        return KeyboardKey_Escape;
    case SDLK_SPACE:
        return KeyboardKey_Space;
    case SDLK_EXCLAIM:
        return KeyboardKey_EMark;
    case SDLK_QUOTEDBL:
        return KeyboardKey_DQuote;
    case SDLK_HASH:
        return KeyboardKey_Pound;
    case SDLK_DOLLAR:
        return KeyboardKey_Dollar;
    case SDLK_AMPERSAND:
        return KeyboardKey_Amp;
    case SDLK_QUOTE:
        return KeyboardKey_SQuote;
    case SDLK_LEFTPAREN:
        return KeyboardKey_LParen;
    case SDLK_RIGHTPAREN:
        return KeyboardKey_RParen;
    case SDLK_ASTERISK:
        return KeyboardKey_Star;
    case SDLK_PLUS:
        return KeyboardKey_Plus;
    case SDLK_COMMA:
        return KeyboardKey_Comma;
    case SDLK_MINUS:
        return KeyboardKey_Dash;
    case SDLK_PERIOD:
        return KeyboardKey_Period;
    case SDLK_SLASH:
        return KeyboardKey_FSlash;
    case SDLK_0:
        return KeyboardKey_Num0;
    case SDLK_1:
        return KeyboardKey_Num1;
    case SDLK_2:
        return KeyboardKey_Num2;
    case SDLK_3:
        return KeyboardKey_Num3;
    case SDLK_4:
        return KeyboardKey_Num4;
    case SDLK_5:
        return KeyboardKey_Num5;
    case SDLK_6:
        return KeyboardKey_Num6;
    case SDLK_7:
        return KeyboardKey_Num7;
    case SDLK_8:
        return KeyboardKey_Num8;
    case SDLK_9:
        return KeyboardKey_Num9;
    case SDLK_COLON:
        return KeyboardKey_Colon;
    case SDLK_SEMICOLON:
        return KeyboardKey_Semicolon;
    case SDLK_LESS:
        return KeyboardKey_LAngle;
    case SDLK_EQUALS:
        return KeyboardKey_Equals;
    case SDLK_GREATER:
        return KeyboardKey_RAngle;
    case SDLK_QUESTION:
        return KeyboardKey_QMark;
    case SDLK_AT:
        return KeyboardKey_AtSign;
    case SDLK_LEFTBRACKET:
        return KeyboardKey_LBracket;
    case SDLK_BACKSLASH:
        return KeyboardKey_BSlash;
    case SDLK_RIGHTBRACKET:
        return KeyboardKey_RBracket;
    case SDLK_CARET:
        return KeyboardKey_Caret;
    case SDLK_UNDERSCORE:
        return KeyboardKey_Underline;
    case SDLK_BACKQUOTE:
        return KeyboardKey_BQuote;
    case SDLK_a:
        return KeyboardKey_A;
    case SDLK_b:
        return KeyboardKey_B;
    case SDLK_c:
        return KeyboardKey_C;
    case SDLK_d:
        return KeyboardKey_D;
    case SDLK_e:
        return KeyboardKey_E;
    case SDLK_f:
        return KeyboardKey_F;
    case SDLK_g:
        return KeyboardKey_G;
    case SDLK_h:
        return KeyboardKey_H;
    case SDLK_i:
        return KeyboardKey_I;
    case SDLK_j:
        return KeyboardKey_J;
    case SDLK_k:
        return KeyboardKey_K;
    case SDLK_l:
        return KeyboardKey_L;
    case SDLK_m:
        return KeyboardKey_M;
    case SDLK_n:
        return KeyboardKey_N;
    case SDLK_o:
        return KeyboardKey_O;
    case SDLK_p:
        return KeyboardKey_P;
    case SDLK_q:
        return KeyboardKey_Q;
    case SDLK_r:
        return KeyboardKey_R;
    case SDLK_s:
        return KeyboardKey_S;
    case SDLK_t:
        return KeyboardKey_T;
    case SDLK_u:
        return KeyboardKey_U;
    case SDLK_v:
        return KeyboardKey_V;
    case SDLK_w:
        return KeyboardKey_W;
    case SDLK_x:
        return KeyboardKey_X;
    case SDLK_y:
        return KeyboardKey_Y;
    case SDLK_z:
        return KeyboardKey_Z;
    case SDLK_DELETE:
        return KeyboardKey_Delete;
    case SDLK_KP0:
        return KeyboardKey_KPad0;
    case SDLK_KP1:
        return KeyboardKey_KPad1;
    case SDLK_KP2:
        return KeyboardKey_KPad2;
    case SDLK_KP3:
        return KeyboardKey_KPad3;
    case SDLK_KP4:
        return KeyboardKey_KPad4;
    case SDLK_KP5:
        return KeyboardKey_KPad5;
    case SDLK_KP6:
        return KeyboardKey_KPad6;
    case SDLK_KP7:
        return KeyboardKey_KPad7;
    case SDLK_KP8:
        return KeyboardKey_KPad8;
    case SDLK_KP9:
        return KeyboardKey_KPad8;
    case SDLK_KP_PERIOD:
        return KeyboardKey_KPadPeriod;
    case SDLK_KP_DIVIDE:
        return KeyboardKey_KPadFSlash;
    case SDLK_KP_MULTIPLY:
        return KeyboardKey_KPadStar;
    case SDLK_KP_MINUS:
        return KeyboardKey_KPadDash;
    case SDLK_KP_PLUS:
        return KeyboardKey_KPadPlus;
    case SDLK_KP_ENTER:
        return KeyboardKey_KPadReturn;
    case SDLK_KP_EQUALS:
        return KeyboardKey_KPadEquals;
    case SDLK_UP:
        return KeyboardKey_Up;
    case SDLK_DOWN:
        return KeyboardKey_Down;
    case SDLK_RIGHT:
        return KeyboardKey_Right;
    case SDLK_LEFT:
        return KeyboardKey_Left;
    case SDLK_INSERT:
        return KeyboardKey_Insert;
    case SDLK_HOME:
        return KeyboardKey_Home;
    case SDLK_END:
        return KeyboardKey_End;
    case SDLK_PAGEUP:
        return KeyboardKey_PageUp;
    case SDLK_PAGEDOWN:
        return KeyboardKey_PageDown;
    case SDLK_F1:
        return KeyboardKey_F1;
    case SDLK_F2:
        return KeyboardKey_F2;
    case SDLK_F3:
        return KeyboardKey_F3;
    case SDLK_F4:
        return KeyboardKey_F4;
    case SDLK_F5:
        return KeyboardKey_F5;
    case SDLK_F6:
        return KeyboardKey_F6;
    case SDLK_F7:
        return KeyboardKey_F7;
    case SDLK_F8:
        return KeyboardKey_F8;
    case SDLK_F9:
        return KeyboardKey_F9;
    case SDLK_F10:
        return KeyboardKey_F10;
    case SDLK_F11:
        return KeyboardKey_F11;
    case SDLK_F12:
        return KeyboardKey_F12;
    case SDLK_F13:
    case SDLK_F14:
    case SDLK_F15:
        // TODO (jacob#): implement keys
        return KeyboardKey_Unknown;
    case SDLK_NUMLOCK:
        return KeyboardKey_NumLock;
    case SDLK_CAPSLOCK:
        return KeyboardKey_CapsLock;
    case SDLK_SCROLLOCK:
        return KeyboardKey_ScrollLock;
    case SDLK_RSHIFT:
        return KeyboardKey_RShift;
    case SDLK_LSHIFT:
        return KeyboardKey_LShift;
    case SDLK_RCTRL:
        return KeyboardKey_RCtrl;
    case SDLK_LCTRL:
        return KeyboardKey_LCtrl;
    case SDLK_RALT:
        return KeyboardKey_RAlt;
    case SDLK_LALT:
        return KeyboardKey_LAlt;
    case SDLK_RMETA:
        return KeyboardKey_RMeta;
    case SDLK_LMETA:
        return KeyboardKey_LMeta;
    case SDLK_LSUPER:
        return KeyboardKey_LSuper;
    case SDLK_RSUPER:
        return KeyboardKey_RSuper;
    case SDLK_MODE:
        return KeyboardKey_Mode;
    case SDLK_COMPOSE:
    case SDLK_HELP:
        // TODO (jacob#): implement keys
        return KeyboardKey_Unknown;
    case SDLK_PRINT:
        return KeyboardKey_PrintScreen;
    case SDLK_SYSREQ:
        return KeyboardKey_SysRequest;
    case SDLK_BREAK:
        return KeyboardKey_Break;
    case SDLK_MENU:
        return KeyboardKey_Menu;
    case SDLK_POWER:
    case SDLK_EURO:
    case SDLK_UNDO:
        // TODO (jacob#): implement keys
        return KeyboardKey_Unknown;
    default:
        return KeyboardKey_Unknown;
    }
}

static KeyboardModifiers translateModifiers(SDLMod input)
{
    int retval = KeyboardModifiers_None;
    if(input & KMOD_LSHIFT)
    {
        retval |= KeyboardModifiers_LShift;
    }
    if(input & KMOD_RSHIFT)
    {
        retval |= KeyboardModifiers_RShift;
    }
    if(input & KMOD_LALT)
    {
        retval |= KeyboardModifiers_LAlt;
    }
    if(input & KMOD_RALT)
    {
        retval |= KeyboardModifiers_RAlt;
    }
    if(input & KMOD_LCTRL)
    {
        retval |= KeyboardModifiers_LCtrl;
    }
    if(input & KMOD_RCTRL)
    {
        retval |= KeyboardModifiers_RCtrl;
    }
    if(input & KMOD_LMETA)
    {
        retval |= KeyboardModifiers_LMeta;
    }
    if(input & KMOD_RMETA)
    {
        retval |= KeyboardModifiers_RMeta;
    }
    if(input & KMOD_NUM)
    {
        retval |= KeyboardModifiers_NumLock;
    }
    if(input & KMOD_CAPS)
    {
        retval |= KeyboardModifiers_CapsLock;
    }
    if(input & KMOD_MODE)
    {
        retval |= KeyboardModifiers_Mode;
    }
    return static_cast<KeyboardModifiers>(retval);
}

static MouseButton translateButton(Uint8 button)
{
    switch(button)
    {
    case SDL_BUTTON_LEFT:
        return MouseButton_Left;
    case SDL_BUTTON_MIDDLE:
        return MouseButton_Middle;
    case SDL_BUTTON_RIGHT:
        return MouseButton_Right;
    case SDL_BUTTON_X1:
        return MouseButton_X1;
    case SDL_BUTTON_X2:
        return MouseButton_X2;
    default:
        return MouseButton_None;
    }
}

static bool &keyState(KeyboardKey key)
{
    static bool state[KeyboardKey_max + 1 - KeyboardKey_min];
    return state[static_cast<int>(key) + KeyboardKey_min];
}

static MouseButton buttonState = MouseButton_None;

static Event *makeEvent()
{
    while(true)
    {
        SDL_Event SDLEvent;
        if(SDL_PollEvent(&SDLEvent) == 0)
        {
            return nullptr;
        }
        switch(SDLEvent.type)
        {
        case SDL_ACTIVEEVENT:
            // TODO (jacob#): handle SDL_ACTIVEEVENT
            break;
        case SDL_KEYDOWN:
        {
            KeyboardKey key = translateKey(SDLEvent.key.keysym.sym);
            Event *retval = new KeyDownEvent(key, translateModifiers(SDLEvent.key.keysym.mod), keyState(key));
            keyState(key) = true;
            return retval;
        }
        case SDL_KEYUP:
        {
            KeyboardKey key = translateKey(SDLEvent.key.keysym.sym);
            Event *retval = new KeyUpEvent(key, translateModifiers(SDLEvent.key.keysym.mod));
            keyState(key) = false;
            return retval;
        }
        case SDL_MOUSEMOTION:
            return new MouseMoveEvent(SDLEvent.motion.x, SDLEvent.motion.y, SDLEvent.motion.xrel, SDLEvent.motion.yrel);
        case SDL_MOUSEBUTTONDOWN:
        {
            MouseButton button = translateButton(SDLEvent.button.button);
            buttonState = static_cast<MouseButton>(buttonState | button); // set bit
            return new MouseDownEvent(SDLEvent.button.x, SDLEvent.button.y, 0, 0, button);
        }
        case SDL_MOUSEBUTTONUP:
        {
            MouseButton button = translateButton(SDLEvent.button.button);
            buttonState = static_cast<MouseButton>(buttonState & ~button); // clear bit
            return new MouseUpEvent(SDLEvent.button.x, SDLEvent.button.y, 0, 0, button);
        }
        case SDL_JOYAXISMOTION:
        case SDL_JOYBALLMOTION:
        case SDL_JOYHATMOTION:
        case SDL_JOYBUTTONDOWN:
        case SDL_JOYBUTTONUP:
            //TODO (jacob#): handle joysticks
            break;
        case SDL_QUIT:
            return new QuitEvent();
        case SDL_SYSWMEVENT:
            //TODO (jacob#): handle SDL_SYSWMEVENT
            break;
        case SDL_VIDEORESIZE:
            //TODO (jacob#): handle SDL_VIDEORESIZE
            break;
        case SDL_VIDEOEXPOSE:
            //TODO (jacob#): handle SDL_VIDEOEXPOSE
            break;
        case SDL_EVENT_RESERVEDA:
        case SDL_EVENT_RESERVEDB:
        case SDL_EVENT_RESERVED2:
        case SDL_EVENT_RESERVED3:
        case SDL_EVENT_RESERVED4:
        case SDL_EVENT_RESERVED5:
        case SDL_EVENT_RESERVED6:
        case SDL_EVENT_RESERVED7:
        default:
            break;
        }
    }
}

namespace
{
struct DefaultEventHandler : public EventHandler
{
    virtual bool handleMouseUp(MouseUpEvent &) override
    {
        return true;
    }
    virtual bool handleMouseDown(MouseDownEvent &) override
    {
        return true;
    }
    virtual bool handleMouseMove(MouseMoveEvent &) override
    {
        return true;
    }
    virtual bool handleMouseScroll(MouseScrollEvent &) override
    {
        return true;
    }
    virtual bool handleKeyUp(KeyUpEvent &) override
    {
        return true;
    }
    virtual bool handleKeyDown(KeyDownEvent &event) override
    {
        if(event.key == KeyboardKey_F4 && (event.mods & KeyboardModifiers_Alt) != 0)
        {
            exit(0);
            return true;
        }
        return true;
    }
    virtual bool handleKeyPress(KeyPressEvent &) override
    {
        return true;
    }
    virtual bool handleQuit(QuitEvent &) override
    {
        exit(0);
        return true;
    }
};
shared_ptr<EventHandler> DefaultEventHandler_handler(new DefaultEventHandler);
}

static void handleEvents(shared_ptr<EventHandler> eventHandler)
{
    for(Event *e = makeEvent(); e != nullptr; e = makeEvent())
    {
        if(eventHandler == nullptr || !e->dispatch(eventHandler))
        {
            e->dispatch(DefaultEventHandler_handler);
        }
    }
}

void glLoadMatrix(Matrix mat)
{
    float matArray[16] =
    {
        mat.x00,
        mat.x01,
        mat.x02,
        0,

        mat.x10,
        mat.x11,
        mat.x12,
        0,

        mat.x20,
        mat.x21,
        mat.x22,
        0,

        mat.x30,
        mat.x31,
        mat.x32,
        1,
    };
    glLoadMatrixf(static_cast<const float *>(matArray));
}

wstring Display::title()
{
    char *title_, *icon;
    SDL_WM_GetCaption(&title_, &icon);
    return mbsrtowcs(title_);
}

void Display::title(wstring newTitle)
{
    string s = wcsrtombs(newTitle);
    SDL_WM_SetCaption(s.c_str(), nullptr);
}

void Display::handleEvents(shared_ptr<EventHandler> eventHandler)
{
    ::handleEvents(eventHandler);
}

static double timer_;

static void updateTimer()
{
    timer_ = Display::realtimeTimer();
}

initializer initializer4([]()
{
    updateTimer();
});

void Display::flip(float fps)
{
    flipDisplay(fps);
    updateTimer();
}

double Display::instantaneousFPS()
{
    return ::instantaneousFPS();
}

double Display::frameDeltaTime()
{
    return ::frameDeltaTime();
}

float Display::averageFPS()
{
    return ::averageFPS();
}

double Display::timer()
{
    return timer_;
}

static float scaleXInternal = 1.0, scaleYInternal = 1.0;

int Display::width()
{
    return xResInternal;
}

int Display::height()
{
    return yResInternal;
}

float Display::scaleX()
{
    return scaleXInternal;
}

float Display::scaleY()
{
    return scaleYInternal;
}

void Display::initFrame()
{
    if(width > height)
    {
        scaleXInternal = static_cast<float>(width()) / height();
        scaleYInternal = 1.0;
    }
    else
    {
        scaleXInternal = 1.0;
        scaleYInternal = static_cast<float>(height()) / width();
    }
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_ALPHA_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glDepthFunc(GL_LEQUAL);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glAlphaFunc(GL_GREATER, 0.0f);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    glViewport(0, 0, width(), height());
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    const float minDistance = 5e-2f, maxDistance = 100.0f;
    glFrustum(-minDistance * scaleX(), minDistance * scaleX(), -minDistance * scaleY(), minDistance * scaleY(), minDistance, maxDistance);
    glEnableClientState(GL_COLOR_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_VERTEX_ARRAY);
}

void Display::initOverlay()
{
    glDepthMask(GL_TRUE);
    glClear(GL_DEPTH_BUFFER_BIT);
}

static bool grabMouse_ = false;

bool Display::grabMouse()
{
    return grabMouse_;
}

void Display::grabMouse(bool g)
{
    grabMouse_ = g;
    SDL_ShowCursor(g ? 0 : 1);
    SDL_WM_GrabInput(g ? SDL_GRAB_ON : SDL_GRAB_OFF);
}

VectorF Display::transformMouseTo3D(float x, float y, float depth)
{
    return VectorF(depth * scaleX() * (2 * x / width() - 1), depth * scaleY() * (1 - 2 * y / height()), -depth);
}

