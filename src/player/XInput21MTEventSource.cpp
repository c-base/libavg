//
//  libavg - Media Playback Engine. 
//  Copyright (C) 2003-2008 Ulrich von Zadow
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//  Current versions can be found at www.libavg.de
//

#include "XInput21MTEventSource.h"

#include "TouchEvent.h"
#include "Player.h"
#include "AVGNode.h"
#include "TouchStatus.h"
#include "SDLDisplayEngine.h"

#include "../base/Logger.h"
#include "../base/Point.h"
#include "../base/ObjectCounter.h"
#include "../base/Exception.h"
#include "../base/OSHelper.h"
#include "../base/StringHelper.h"

#include <SDL/SDL_syswm.h>
#include <SDL/SDL.h>

#include <X11/extensions/XInput.h>
#include <X11/extensions/XInput2.h>

using namespace std;

namespace avg {

Display* XInput21MTEventSource::s_pDisplay = 0;

const char* cookieTypeToName(int evtype);
string xEventTypeToName(int evtype);

XInput21MTEventSource::XInput21MTEventSource()
    : m_LastID(0)
{
}

XInput21MTEventSource::~XInput21MTEventSource()
{
}

void XInput21MTEventSource::start()
{
    Status status;
    SDLDisplayEngine * pEngine = dynamic_cast<SDLDisplayEngine *>(
            Player::get()->getDisplayEngine());
    pEngine->setXIMTEventSource(this);

    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);
    int rc = SDL_GetWMInfo(&info);
    AVG_ASSERT(rc != -1);
    s_pDisplay = info.info.x11.display;
    m_SDLLockFunc = info.info.x11.lock_func;
    m_SDLUnlockFunc = info.info.x11.unlock_func;

    m_SDLLockFunc();
    // XInput Extension available?
    int event, error;
    bool bOk = XQueryExtension(s_pDisplay, "XInputExtension", &m_XIOpcode, 
            &event, &error);
    if (!bOk) {
        throw Exception(AVG_ERR_MT_INIT, 
                "XInput 2.1 multitouch event source: X Input extension not available'");
    }

    // Which version of XI2? We need 2.1. 
    int major = 2, minor = 1;
    status = XIQueryVersion(s_pDisplay, &major, &minor);
    if (status == BadRequest) {
        throw Exception(AVG_ERR_MT_INIT, 
                "XInput 2.1 multitouch event source: Server does not support XI2");
    }
    if (major < 2 || minor < 1) {
        throw Exception(AVG_ERR_MT_INIT, 
                "XInput 2.1 multitouch event source: Supported version is "
                +toString(major)+"."+toString(minor)+". 2.1 is needed.");
    }
    if (pEngine->isFullscreen()) {
        m_Win = info.info.x11.fswindow;
    } else {
        m_Win = info.info.x11.wmwindow;
    }

    findMTDevice();

    XIEventMask mask;
    mask.deviceid = m_DeviceID;
    mask.mask_len = XIMaskLen(XI_LASTEVENT);
    mask.mask = (unsigned char *)calloc(mask.mask_len, sizeof(char));
    memset(mask.mask, 0, mask.mask_len);
    XISetMask(mask.mask, XI_TouchBegin);
    XISetMask(mask.mask, XI_TouchMotion);
    XISetMask(mask.mask, XI_TouchMotionUnowned);
    XISetMask(mask.mask, XI_TouchOwnership);
    XISetMask(mask.mask, XI_TouchEnd);

    status = XISelectEvents(s_pDisplay, m_Win, &mask, 1);
    AVG_ASSERT(status == Success);
/*
    {
        XIGrabModifiers mods = { XIAnyModifier, 0 };
        mask.deviceid = XIAllMasterDevices;
        memset(mask.mask, 0, mask.mask_len);
        XISetMask(mask.mask, XI_TouchBegin);
        XISetMask(mask.mask, XI_TouchMotion);
        XISetMask(mask.mask, XI_TouchMotionUnowned);
        XISetMask(mask.mask, XI_TouchOwnership);
        XISetMask(mask.mask, XI_TouchEnd);
        XIGrabTouchBegin(s_pDisplay, XIAllMasterDevices, m_Win, False, &mask,
                         1, &mods);
    }
*/
/*    status = XIGrabDevice(s_pDisplay, 11, info.info.x11.window, CurrentTime, 0, GrabModeAsync,
            GrabModeAsync, true, &mask);
    cerr << "XIGrabDevice: " << status << endl;
*/
    m_SDLUnlockFunc();

    SDL_SetEventFilter(XInput21MTEventSource::filterEvent);

    MultitouchEventSource::start();
    AVG_TRACE(Logger::CONFIG, "XInput 2.1 Multitouch event source created.");
}

void XInput21MTEventSource::handleXIEvent(const XEvent& xEvent)
{
    m_SDLLockFunc();
    XGenericEventCookie* pCookie = (XGenericEventCookie*)&xEvent.xcookie;
    if (pCookie->type == GenericEvent && pCookie->extension == m_XIOpcode) {
        XIDeviceEvent* pDevEvent = (XIDeviceEvent*)(pCookie->data);
        IntPoint pos(pDevEvent->event_x, pDevEvent->event_y);
        int xid = pDevEvent->detail;
        switch (pCookie->evtype) {
            case XI_TouchBegin:
                {
                    cerr << "TouchBegin " << xid << ", " << pos << endl;
                    m_LastID++;
                    TouchEventPtr pEvent = createEvent(m_LastID, Event::CURSORDOWN, pos); 
                    addTouchStatus(xid, pEvent);
                }
                break;
            case XI_TouchMotion:
                {
                    cerr << "TouchMotion " << xid << ", " << pos << endl;
                    TouchEventPtr pEvent = createEvent(0, Event::CURSORMOTION, pos); 
                    TouchStatusPtr pTouchStatus = getTouchStatus(xid);
                    AVG_ASSERT(pTouchStatus);
                    pTouchStatus->updateEvent(pEvent);
                }
                break;
            case XI_TouchEnd:
                {
                    cerr << "TouchEnd " << xid << ", " << pos << endl;
                    TouchStatusPtr pTouchStatus = getTouchStatus(xid);
                    AVG_ASSERT(pTouchStatus);
                    TouchEventPtr pEvent = createEvent(0, Event::CURSORUP, pos); 
                    pTouchStatus->updateEvent(pEvent);
                }
                break;
            default:
                cerr << "Unhandled XInput event, type: " 
                        << cookieTypeToName(pCookie->evtype) << endl;
        }
    } else {
        cerr << "Unhandled X11 Event: " << xEvent.type << endl;
    }

    XFreeEventData(s_pDisplay, pCookie);
    m_SDLUnlockFunc();
}

std::vector<EventPtr> XInput21MTEventSource::pollEvents()
{

    return MultitouchEventSource::pollEvents();
}

void XInput21MTEventSource::findMTDevice()
{
    int ndevices;
    XIDeviceInfo* pDevices;
    XIDeviceInfo* pDevice;

    pDevices = XIQueryDevice(s_pDisplay, XIAllDevices, &ndevices);

    XITouchClassInfo * pTouchClass = 0;
    int maxTouches;
    bool bDirectTouch;
    for (int i = 0; i < ndevices && !pTouchClass; ++i) {
        pDevice = &pDevices[i];
        if (pDevice->use == XISlavePointer) {
//            cerr << "Device " << pDevice->name << "(id: " << pDevice->deviceid << ")."
//                    << endl;
            for (int j = 0; j < pDevice->num_classes; ++j) {
                XIAnyClassInfo * pClass = pDevice->classes[j];
                if (pClass->type == XITouchClass) {
                    pTouchClass = (XITouchClassInfo *)pClass;
                    m_sDeviceName = pDevice->name;
                    m_DeviceID = pDevice->deviceid;
                    maxTouches = pTouchClass->num_touches;
                    bDirectTouch = pTouchClass->mode == XIDirectTouch;
                    break;
                }
            }
        }
    }
    if (pTouchClass) {
        AVG_TRACE(Logger::CONFIG, "Using multitouch input device " << m_sDeviceName 
                << ", max touches: " << maxTouches << ", direct touch: " << bDirectTouch);
    } else {
        throw Exception(AVG_ERR_MT_INIT, 
                "XInput 2.1 multitouch event source: No multitouch device found.");
    }
    XIFreeDeviceInfo(pDevices);
}

TouchEventPtr XInput21MTEventSource::createEvent(int id, Event::Type type, IntPoint pos)
{
    return TouchEventPtr(new TouchEvent(id, type, pos, Event::TOUCH, DPoint(0,0), 
            0, 20, 1, DPoint(5,0), DPoint(0,5)));
}

int XInput21MTEventSource::filterEvent(const SDL_Event * pEvent)
{
    // This is a hook into libsdl event processing. Since libsdl doesn't know about
    // XInput 2, it doesn't call XGetEventData either. By the time the event arrives
    // in handleXIEvent(), other events may have arrived and XGetEventData can't be 
    // called anymore. Hence this function, which calls XGetEventData for each event
    // that has a cookie.
    if (pEvent->type == SDL_SYSWMEVENT) {
        SDL_SysWMmsg* pMsg = pEvent->syswm.msg;
        AVG_ASSERT(pMsg->subsystem == SDL_SYSWM_X11);
        XEvent* pXEvent = &pMsg->event.xevent;
        XGenericEventCookie* pCookie = (XGenericEventCookie*)&(pXEvent->xcookie);
        cerr << "---- filter xinput event: " << xEventTypeToName(pXEvent->type) << ", "
                << cookieTypeToName(pCookie->evtype) << endl;
        XGetEventData(s_pDisplay, pCookie);
    } else {
        cerr << "---- filter: " << int(pEvent->type) << endl;
    }
    return 1;
}
          
// From xinput/test_xi2.c
const char* cookieTypeToName(int evtype)
{
    const char *name;
    switch(evtype) {
        case XI_DeviceChanged:    name = "DeviceChanged";       break;
        case XI_KeyPress:         name = "KeyPress";            break;
        case XI_KeyRelease:       name = "KeyRelease";          break;
        case XI_ButtonPress:      name = "ButtonPress";         break;
        case XI_ButtonRelease:    name = "ButtonRelease";       break;
        case XI_Motion:           name = "Motion";              break;
        case XI_Enter:            name = "Enter";               break;
        case XI_Leave:            name = "Leave";               break;
        case XI_FocusIn:          name = "FocusIn";             break;
        case XI_FocusOut:         name = "FocusOut";            break;
        case XI_HierarchyChanged: name = "HierarchyChanged";    break;
        case XI_PropertyEvent:    name = "PropertyEvent";       break;
        case XI_RawKeyPress:      name = "RawKeyPress";         break;
        case XI_RawKeyRelease:    name = "RawKeyRelease";       break;
        case XI_RawButtonPress:   name = "RawButtonPress";      break;
        case XI_RawButtonRelease: name = "RawButtonRelease";    break;
        case XI_RawMotion:        name = "RawMotion";           break;
        case XI_TouchBegin:       name = "TouchBegin";          break;
        case XI_TouchEnd:         name = "TouchEnd";            break;
        case XI_TouchMotion:      name = "TouchMotion";         break;
        case XI_TouchMotionUnowned:      name = "TouchMotionUnowned";         break;
        default:
                                  name = "unknown event type"; break;
    }
    return name;
}

string xEventTypeToName(int evtype)
{
    switch(evtype) {
        case KeyPress:
            return "KeyPress";
        case KeyRelease:
            return "KeyRelease";
        case ButtonPress:
            return "ButtonPress";
        case ButtonRelease:
            return "ButtonRelease";
        case MotionNotify:
            return "MotionNotify";
        case EnterNotify:
            return "EnterNotify";
        case LeaveNotify:
            return "LeaveNotify";
        case FocusIn:
            return "FocusIn";
        case FocusOut:
            return "FocusOut";
        case KeymapNotify:
            return "KeymapNotify";
        case Expose:
            return "Expose";
        case GraphicsExpose:
            return "GraphicsExpose";
        case NoExpose:
            return "NoExpose";
        case VisibilityNotify:
            return "VisibilityNotify";
        case CreateNotify:
            return "CreateNotify";
        case DestroyNotify:
            return "DestroyNotify";
        case UnmapNotify:
            return "UnmapNotify";
        case MapNotify:
            return "MapNotify";
        case MapRequest:
            return "MapRequest";
        case ReparentNotify:
            return "ReparentNotify";
        case ConfigureNotify:
            return "ConfigureNotify";
        case ConfigureRequest:
            return "ConfigureRequest";
        case GravityNotify:
            return "GravityNotify";
        case ResizeRequest:
            return "ResizeRequest";
        case CirculateNotify:
            return "CirculateNotify";
        case CirculateRequest:
            return "CirculateRequest";
        case PropertyNotify:
            return "PropertyNotify";
        case SelectionClear:
            return "SelectionClear";
        case SelectionRequest:
            return "SelectionRequest";
        case SelectionNotify:
            return "SelectionNotify";
        case ColormapNotify:
            return "ColormapNotify";
        case ClientMessage:
            return "ClientMessage";
        case MappingNotify:
            return "MappingNotify";
        case GenericEvent:
            return "GenericEvent";
        default:
            return "Unknown event type";
    }
}

}
