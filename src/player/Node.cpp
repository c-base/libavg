//
//  libavg - Media Playback Engine. 
//  Copyright (C) 2003-2006 Ulrich von Zadow
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

#include "Node.h"
#include "DivNode.h"

#include "Event.h"
#include "MouseEvent.h"
#include "DivNode.h"
#include "Player.h"
#include "DisplayEngine.h"

#include "../base/Logger.h"
#include "../base/Exception.h"
#include "../base/XMLHelper.h"

#include <Python.h>
#include <object.h>
#include <compile.h>
#include <eval.h>
#include <boost/python.hpp>

#include <iostream>

using namespace boost::python;

using namespace std;

namespace avg {

Node::Node ()
    : m_pParent(0),
      m_pPlayer(0),
      m_ID(""),
      m_MouseMoveHandler(""),
      m_MouseButtonUpHandler(""),
      m_MouseButtonDownHandler(""),
      m_MouseOverHandler(""),
      m_MouseOutHandler(""),
      m_RelViewport(0,0,0,0),
      m_AbsViewport(0,0,0,0),
      m_Opacity(1.0),
      m_bActive(true),
      m_bSensitive(true),
      m_bInitialized(false),
      m_InitialWidth(0),
      m_InitialHeight(0)
{
}

Node::Node (const xmlNodePtr xmlNode, Player * pPlayer)
    : m_pParent(0),
      m_pPlayer(pPlayer),
      m_RelViewport(0,0,0,0),
      m_AbsViewport(0,0,0,0),
      m_bInitialized(false),
      m_InitialWidth(0),
      m_InitialHeight(0)
{
    m_ID = getDefaultedStringAttr (xmlNode, "id", "");
    m_MouseMoveHandler = getDefaultedStringAttr (xmlNode, "onmousemove", "");
    m_MouseButtonUpHandler = getDefaultedStringAttr (xmlNode, "onmouseup", "");
    m_MouseButtonDownHandler = getDefaultedStringAttr (xmlNode, "onmousedown", "");
    m_MouseOverHandler = getDefaultedStringAttr (xmlNode, "onmouseover", "");
    m_MouseOutHandler = getDefaultedStringAttr (xmlNode, "onmouseout", "");
    m_RelViewport.tl.x = getDefaultedDoubleAttr (xmlNode, "x", 0.0);
    m_RelViewport.tl.y = getDefaultedDoubleAttr (xmlNode, "y", 0.0);
    m_InitialWidth = getDefaultedDoubleAttr (xmlNode, "width", 0.0);
    m_InitialHeight = getDefaultedDoubleAttr (xmlNode, "height", 0.0);
    m_Opacity = getDefaultedDoubleAttr (xmlNode, "opacity", 1.0);
    m_bActive = getDefaultedBoolAttr (xmlNode, "active", true);
    m_bSensitive = getDefaultedBoolAttr (xmlNode, "sensitive", true);
    setState(NS_UNCONNECTED);
}

Node::~Node()
{
}

void Node::connect(DisplayEngine * pEngine, DivNode * pParent)
{
    m_pParent = pParent;
    m_pEngine = pEngine;
    
    DPoint PreferredSize = getPreferredMediaSize();
    
    if (m_InitialWidth == 0) {
        m_InitialWidth = (int)PreferredSize.x;
    }
    if (m_InitialHeight == 0) {
        m_InitialHeight = (int)PreferredSize.y;
    }
    m_RelViewport.SetWidth(m_InitialWidth);
    m_RelViewport.SetHeight(m_InitialHeight);
    DPoint pos(0,0);
    if (m_pParent) {
        pos = m_pParent->getAbsViewport().tl;
    } 
    m_AbsViewport = DRect (pos+getRelViewport().tl, pos+getRelViewport().br);
    m_State = NS_CONNECTED;
    m_bInitialized = true;
}

const string& Node::getID () const
{
    return m_ID;
}

double Node::getX() const {
    return m_RelViewport.tl.x;
}

void Node::setX(double x) {
    setViewport(x, -32767, -32767, -32767);
}

double Node::getY() const {
    return m_RelViewport.tl.y;
}

void Node::setY(double y) {
    setViewport(-32767, y, -32767, -32767);
}

double Node::getWidth() const {
    return getRelViewport().Width();
}

void Node::setWidth(double width) {
    setViewport(-32767, -32767, width, -32767);
}

double Node::getHeight() const {
    return getRelViewport().Height();
}

void Node::setHeight(double height) {
    setViewport(-32767, -32767, -32767, height);
}

double Node::getOpacity() const {
    return m_Opacity;
}

void Node::setOpacity(double opacity) {
    m_Opacity = opacity;
    if (m_Opacity < 0.0) {
        m_Opacity = 0.0;
    } else if (m_Opacity > 1.0) {
        m_Opacity = 1.0;
    }
    if (m_bActive) {
        invalidate();
    }
}

bool Node::getActive() const {
    return m_bActive;
}

void Node::setActive(bool bActive)
{
    if (bActive != m_bActive) {
        m_bActive = bActive;
        invalidate();
    }
}

bool Node::getSensitive() const {
    return m_bSensitive;
}

void Node::setSensitive(bool bSensitive)
{
    m_bSensitive = bSensitive;
}

DivNode* Node::getParent() const
{
    return m_pParent;
}

bool Node::isActive()
{
    return m_bActive;
}

bool Node::reactsToMouseEvents()
{
    return m_bActive && m_bSensitive;
}

Node * Node::getElementByPos (const DPoint & pos)
{
    if (getVisibleRect().Contains(pos) && reactsToMouseEvents()) {
        return this;
    } else {
        return 0;
    }
}

void Node::prepareRender (int time, const DRect& parent)
{
    calcAbsViewport();
}

void Node::maybeRender (const DRect& Rect)
{
    if (m_bActive) {
        bool bVisible;
        // FIXME
        if (dynamic_cast<DivNode*>(this) != 0) {
            bVisible = getEngine()->pushClipRect(getVisibleRect(), true);
        } else {
            bVisible = getEngine()->pushClipRect(getVisibleRect(), false);
        }
        if (bVisible) {
            if (getEffectiveOpacity() > 0.01) {
                if (!getParent() || 
                    !getParent()->obscures(getEngine()->getClipRect(), 
                            getParent()->indexOf(this)))
                {
                    if (m_ID != "") {
                        AVG_TRACE(Logger::BLTS, "Rendering " << getTypeStr() << 
                                " with ID " << m_ID);
                    } else {
                        AVG_TRACE(Logger::BLTS, "Rendering " << getTypeStr()); 
                    }
                    render(Rect);
                } 
            }
        }
        getEngine()->popClipRect();
    }
}

void Node::render (const DRect& Rect)
{
}

bool Node::obscures (const DRect& Rect, int Child)  
{
    return false;
}

void Node::addDirtyRect(const DRect& Rect)
{
    m_DirtyRegion.addRect(Rect);
}

void Node::getDirtyRegion (Region& Region)
{
    Region.addRegion(m_DirtyRegion);
    m_DirtyRegion.clear();
}

void Node::invalidate()
{
    if (m_bInitialized) {
        addDirtyRect(getVisibleRect());
    }
}

Node::NodeState Node::getState()
{
    return m_State;
}

Player * Node::getPlayer()
{
    return m_pPlayer;
}

void Node::setViewport (double x, double y, double width, double height)
{
    if (m_bActive) {
        invalidate();
    }
    if (x == -32767) {
        x = getRelViewport().tl.x;
    }
    if (y == -32767) {
        y = getRelViewport().tl.y;
    }
    if (width == -32767) {
        width = getRelViewport().Width();
    }
    if (height == -32767) {
        height = getRelViewport().Height();
    }
    m_RelViewport = DRect (x, y, x+width, y+height);
    calcAbsViewport();
    if (m_bActive) {
        invalidate();
    }
}

const DRect& Node::getRelViewport () const
{
//    cerr << "Node " << m_ID << ": (" << m_RelViewport.tl.x << ", " 
//            << m_RelViewport.tl.y << ")" << endl;
    return m_RelViewport;
}

const DRect& Node::getAbsViewport () const
{
    return m_AbsViewport;
}

DRect Node::getVisibleRect()
{
    Node * pParent = getParent();
    DRect visRect = getAbsViewport();
    if (pParent) {
        DRect parent = getParent()->getVisibleRect();
        visRect.Intersect(parent);
    }
    return visRect;
}

void Node::calcAbsViewport()
{
    Node * pParent = getParent();
    if (pParent) {
        DRect parent = pParent->getAbsViewport();
        m_AbsViewport = DRect(parent.tl+getRelViewport().tl, 
                parent.tl+getRelViewport().br);
    } else {
        m_AbsViewport = getRelViewport();
    }
    if (m_AbsViewport.Width() < 0 || m_AbsViewport.Height() < 0) {
        m_AbsViewport.br=m_AbsViewport.tl;
    }
}

double Node::getEffectiveOpacity()
{
    if (getParent()) {
        return m_Opacity*getParent()->getEffectiveOpacity();
    } else {
        return m_Opacity;
    }
}

DisplayEngine * Node::getEngine()
{
    return m_pEngine;
}

string Node::dump (int indent)
{
    string dumpStr = string(indent, ' ') + getTypeStr() + ": m_ID=" + m_ID;
    char sz[256];
    sprintf (sz, ", x=%.1f, y=%.1f, width=%.1f, height=%.1f, opacity=%.2f\n",
            m_RelViewport.tl.x, m_RelViewport.tl.y,
            m_RelViewport.Width(), m_RelViewport.Height(), m_Opacity);
    dumpStr += sz;
    sprintf (sz, "        Abs: (x=%.1f, y=%.1f, width=%.1f, height=%.1f)\n",
            m_AbsViewport.tl.x, m_AbsViewport.tl.y,  
            m_AbsViewport.Width(), m_AbsViewport.Height());
    dumpStr += sz;

    return dumpStr; 
}

string Node::getTypeStr ()
{
    return "Node";
}


void Node::setParent(DivNode * pParent)
{
    if (m_pParent) {
        throw(Exception(AVG_ERR_UNSUPPORTED, 
                "Can't change parent of node."));
    }
    m_pParent = pParent;
}

void Node::handleMouseEvent (MouseEvent* pEvent)
{
    string Code;
    pEvent->setElement(this);
    int EventType = pEvent->getType();
    switch (EventType) {
        case Event::MOUSEMOTION:
            Code = m_MouseMoveHandler;
            break;
        case Event::MOUSEBUTTONDOWN:
            Code = m_MouseButtonDownHandler;
            break;
        case Event::MOUSEBUTTONUP:
            Code = m_MouseButtonUpHandler;
            break;
        case Event::MOUSEOVER:
            Code = m_MouseOverHandler;
            break;
        case Event::MOUSEOUT:
            Code = m_MouseOutHandler;
            break;
         default:
            break;
    }
    if (!Code.empty()) {
        callPython(Code, *pEvent);
    }
    if (m_pParent) {
        m_pParent->handleMouseEvent (pEvent);
    }
}

void Node::callPython (const string& Code, const Event& Event)
{
    // TODO:
    //   - Pass Event to python.
    //   - Handle python return code/exception and pass to caller.
    PyObject * pModule = PyImport_AddModule("__main__");
    if (!pModule) {
        cerr << "Could not find module __main__." << endl;
        exit(-1);
    }
    PyObject * pDict = PyModule_GetDict(pModule);
    PyObject * pFunc = PyDict_GetItemString(pDict, Code.c_str());
    if (!pFunc) {
        AVG_TRACE(Logger::ERROR, "Function \"" << Code << 
                "\" not defined for node with id '"+m_ID+"'. Aborting.");
        exit(-1);
    }
    PyObject * pArgList = Py_BuildValue("()");
    PyObject * pResult = PyObject_CallObject(pFunc, pArgList);
    if (!pResult) {
        throw error_already_set();
    }
    Py_DECREF(pArgList);
}

void Node::initFilename(Player * pPlayer, string& sFilename)
{
    if (sFilename[0] != '/') {
        sFilename = pPlayer->getCurDirName() + sFilename;
    }
}

bool Node::isInitialized()
{
    return m_bInitialized;
}

void Node::setState(Node::NodeState State)
{
    m_State = State;
}

}

