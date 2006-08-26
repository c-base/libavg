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

void export_bitmap();
void export_raster();
void export_event();
#ifndef WIN32
void export_devices();
#endif

#include "../base/Logger.h"
#include "../base/Exception.h"
#include "../player/Player.h"
#include "../player/AVGNode.h"
#include "../player/DivNode.h"
#include "../player/PanoImage.h"

#include <boost/python.hpp>
#include <boost/version.hpp>
#include <boost/shared_ptr.hpp>

using namespace boost::python;
using namespace avg;

void exception_translator(Exception const & e) 
{
    PyErr_SetString(PyExc_RuntimeError, e.GetStr().c_str());
}

BOOST_PYTHON_MODULE(avg)
{
#if (BOOST_VERSION / 100000) > 1 || ((BOOST_VERSION / 100) % 1000) >= 33
    register_exception_translator<Exception>(exception_translator);
#endif
    register_ptr_to_python< NodePtr >();
    register_ptr_to_python< DivNodePtr >();
    register_ptr_to_python< AVGNodePtr >();

    class_<Logger>("Logger", 
            "Interface to the logger used by the avg player. Enables the setting\n"
            "of different logging categories and a destination file. Can be used\n"
            "to log application-specific messages as well.\n"
            "Each log entry contains the time the message was written, the category\n"
            "of the entry and the message itself.",
            no_init)
        .def("get", &Logger::get, 
                return_value_policy<reference_existing_object>(),
                "get() -> Logger\n\n"
                "There is only one logger. This method gives access to it.")
        .staticmethod("get")
        .def("setConsoleDest", &Logger::setConsoleDest,
                "setConsoleDest() -> None\n\n"
                "Sets the log to be output to the console (stderr, to be precise).\n"
                "This is the default.")
        .def("setFileDest", &Logger::setFileDest,
                "setFileDest(filename) -> None\n\n"
                "Sets a file that the log should be written to. If opening the file\n"
                "fails, the console is used instead.")
        .def("setSyslogDest", &Logger::setSyslogDest,
                "setSyslogDest(facility, logopt) -> None\n\n"
                "Causes log output to be written to the unix system log facility.\n"
                "facility and logopt are passed to the system log verbatim. See\n"
                "man 3 syslog for details of these parameters. Ident is set to\n"
                "'libavg'")
        .def("setCategories", &Logger::setCategories,
                "setCategories(categories) -> None\n\n"
                "Sets the types of messages that should be logged. Possible \n"
                "categories are:\n"
                "    NONE: No logging except for errors.\n"
                "    BLTS: Display subsystem logging. Useful for timing/performance\n"
                "          measurements.\n"
                "    PROFILE: Outputs performance statistics on player termination.\n"
                "    PROFILE_LATEFRAMES: Outputs performance statistics whenever a\n"
                "                        frame is displayed late.\n"
                "    EVENTS: Outputs basic event data.\n"
                "    EVENTS2: Outputs all event data available.\n"
                "    CONFIG: Outputs configuration data.\n"
                "    WARNING: Outputs warning messages. Default is on.\n"
                "    ERROR: Outputs error messages. Can't be shut off.\n"
                "    MEMORY: Currently unused.\n"
                "    APP: Reserved for application-level messages issued by python\n"
                "         code.\n"
                "Categories can be or'ed together.")
        .def("trace", &Logger::trace,
                "trace(category, message) -> None\n\n"
                "Logs message to the log if category is active. The category should\n"
                "in most cases be APP.")
        .def_readonly("NONE", &Logger::NONE)
        .def_readonly("BLTS", &Logger::BLTS)
        .def_readonly("PROFILE", &Logger::PROFILE)
        .def_readonly("PROFILE_LATEFRAMES", &Logger::PROFILE_LATEFRAMES)
        .def_readonly("EVENTS", &Logger::EVENTS)
        .def_readonly("EVENTS2", &Logger::EVENTS2)
        .def_readonly("CONFIG", &Logger::CONFIG)
        .def_readonly("WARNING", &Logger::WARNING)
        .def_readonly("ERROR", &Logger::ERROR)
        .def_readonly("MEMORY", &Logger::MEMORY)
        .def_readonly("APP", &Logger::APP)
    ;

#ifndef WIN32
     export_devices();
#endif
    export_event();

    class_<Node, boost::shared_ptr<Node>, boost::noncopyable>("Node",
            "Base class for all elements in the avg tree.\n"
            "Properties:\n"
            "    id: A unique identifier that can be used to reference the node (ro).\n"
            "    x: The position of the node's left edge relative to it's parent node.\n"
            "    y: The position of the node's top edge relative to it's parent node.\n"
            "    width\n"
            "    height\n"
            "    opacity: A measure of the node's transparency. 0.0 is completely\n"
            "             transparent, 1.0 is completely opaque. Opacity is relative to\n"
            "             the parent node's opacity.\n"
            "    active: If this attribute is true, the node behaves as usual. If not, it\n"
            "            is neither drawn nor does it react to events. Videos are paused.\n"
            "    sensitive: A node only reacts to events if sensitive is true.",
            no_init)
        .def("getParent", &Node::getParent,
                "getParent() -> Node\n\n"
                "Returns the container (AVGNode or DivNode) the node is in. For\n"
                "the root node, returns None.\n")
        .add_property("id", make_function(&Node::getID, 
                return_value_policy<copy_const_reference>()))
        .add_property("x", &Node::getX, &Node::setX)
        .add_property("y", &Node::getY, &Node::setY)
        .add_property("width", &Node::getWidth, &Node::setWidth)
        .add_property("height", &Node::getHeight, &Node::setHeight)
        .add_property("opacity", &Node::getOpacity, &Node::setOpacity)
        .add_property("active", &Node::getActive, &Node::setActive)
        .add_property("sensitive", &Node::getSensitive, &Node::setSensitive)
    ;

    export_bitmap();
    export_raster();
    
    class_<DivNode, bases<Node>, boost::noncopyable>("DivNode", 
            "A div node is a node that groups other nodes logically and visually.\n"
            "Its upper left corner is used as point of origin for the coordinates\n"
            "of its child nodes. Its extents are used to clip the children. Its\n"
            "opacity is used as base opacity for the child nodes' opacities.\n",
            no_init)
        .def("getNumChildren", &DivNode::getNumChildren,
                "getNumChildren() -> numChildren\n\n")
        .def("getChild", &DivNode::getChild, 
                "getChild(i) -> Node\n\n"
                "Returns the ith child in z-order.")
        .def("addChild", &DivNode::addChild,
                "addChild(Node) -> None\n\n"
                "Adds a new child to the container.")
        .def("removeChild", &DivNode::removeChild,
                "removeChild(i) -> None\n\n"
                "Removes the child at index i.")
        .def("indexOf", &DivNode::indexOf,
                "indexOf(childNode) -> i\n\n"
                "Returns the index of the child given or -1 if childNode isn't a\n"
                "child of the container.")
    ;
    
    class_<AVGNode, bases<DivNode> >("AVGNode",
            "Root node of any avg tree. Defines the properties of the display and\n"
            "handles key press events. The AVGNode's width and height define the\n"
            "coordinate system for the display and are the default for the window\n"
            "size used (i.e. by default, the coordinate system is pixel-based.)\n"
            "Properties:\n"
            "    onkeydown: The python code to execute when a key is pressed (ro).\n"
            "    onkeyup: The python code to execute when a key is released (ro).\n")
        .def("getCropSetting", &AVGNode::getCropSetting,
                "getCropSetting() -> isCropActive\n\n"
                "Returns true if cropping is active. Cropping can be turned off globally\n"
                "in the avg file. (Deprecated. This attribute is only nessesary because\n"
                "of certain buggy display drivers that don't work with cropping.)")
        .add_property("onkeydown", make_function(&AVGNode::getOnKeyDown,
                return_value_policy<copy_const_reference>()))
        .add_property("onkeyup", make_function(&AVGNode::getOnKeyUp,
                return_value_policy<copy_const_reference>()))
    ;

    class_<PanoImage, bases<Node> >("PanoImage",
            "A panorama image.\n"
            "Properties:\n"
            "    href: The source filename of the image.\n"
            "    sensorwidth: The width of the sensor used to make the image. This value\n"
            "                 is used together with sensorheight and focallength to\n"
            "                 determine the projection to use. (ro)\n"
            "    sensorheight: The height of the sensor used to make the image. (ro)\n"
            "    focallength: The focal length of the lens in millimeters. (ro)\n"
            "    hue: A hue to color the image in. (ro, deprecated)\n"
            "    saturation: The saturation the image should have. (ro, deprecated)\n"
            "    rotation: The current angle the viewer is looking at in radians.\n"
            "    maxrotation: The maximum angle the viewer can look at.\n")
        .def("getScreenPosFromPanoPos", &PanoImage::getScreenPosFromPanoPos,
                "getScreenPosFromPanoPos(panoPos) -> pos\n\n"
                "Converts a position in panorama image pixels to pixels in coordinates\n"
                "relative to the node, taking into account the current rotation angle.\n")
        .def("getScreenPosFromAngle", &PanoImage::getScreenPosFromAngle,
                "getScreenPosFromAngle(angle) -> pos\n\n"
                "Converts panorama angle to pixels in coordinates\n"
                "relative to the node, taking into account the current rotation angle.\n")
        .add_property("href", make_function(&PanoImage::getHRef, 
                return_value_policy<copy_const_reference>()), &PanoImage::setHRef)
        .add_property("sensorwidth", &PanoImage::getSensorWidth, 
                &PanoImage::setSensorWidth)
        .add_property("sensorheight", &PanoImage::getSensorHeight, 
                &PanoImage::setSensorHeight)
        .add_property("focallength", &PanoImage::getFocalLength, 
                &PanoImage::setFocalLength)
        .add_property("hue", &PanoImage::getHue)
        .add_property("saturation", &PanoImage::getSaturation)
        .add_property("rotation", &PanoImage::getRotation, &PanoImage::setRotation)
        .add_property("maxrotation", &PanoImage::getMaxRotation)
    ;

    enum_<Player::DisplayEngineType>("DisplayEngineType")
        .value("DFB", Player::DFB)
        .value("OGL", Player::OGL)
        .export_values()
    ;

    enum_<DisplayEngine::YCbCrMode>("YCbCrMode")
        .value("shader", DisplayEngine::OGL_SHADER)
        .value("mesa", DisplayEngine::OGL_MESA)
        .value("apple", DisplayEngine::OGL_APPLE)
        .value("none", DisplayEngine::NONE)
        .export_values()
    ;

    class_<TestHelper>("TestHelper", "", no_init)
        .def("getNumDifferentPixels", &TestHelper::getNumDifferentPixels, "")
        .def("fakeMouseEvent", &TestHelper::fakeMouseEvent, "")
    ;

    class_<Player>("Player", 
                "The class used to load and play avg files.")
        .def("setDisplayEngine", &Player::setDisplayEngine,
                "setDisplayEngine(engine) -> None\n\n"
                "Determines which display backend to use. Parameter can be either\n"
                "avg.DFB (for DirectFB rendering) or avg.OGL (for OpenGL rendering).\n"
                "Must be called before loadFile.")
        .def("setResolution", &Player::setResolution,
                "setResolution(fullscreen, width, height, bpp) -> None\n\n"
                "Sets display engine parameters. width and height set the window size\n"
                "(if fullscreen is false) or screen resolution (if fullscreen is true).\n"
                "bpp is the number of bits per pixel to use. Must be called before\n"
                "loadFile.")
        .def("setOGLOptions", &Player::setOGLOptions,
                "setOGLOptions(UsePOW2Textures, YCbCrMode, UseRGBOrder, UsePixelBuffers, MultiSampleSamples)\n"
                "       -> None\n\n"
                "Determines which OpenGL extensions to check for and use if possible.\n"
                "Mainly used for debugging purposes while developing libavg, but can\n"
                "also be used to work around buggy drivers.\n"
                "UsePOW2Textures=true restricts textures to power-of-two dimensions.\n"
                "YCbCrMode can be shader, mesa, apple or none and selects the preferred\n"
                "method of copying video textures to the screen.\n"
                "UseRGBOrder=true swaps the order of red and blue components in textures.\n"
                "UsePixelBuffers=false disables the use of OpenGL pixel buffer objects.\n"
                "MultiSampleSamples is the number of samples per pixel to compute. This\n"
                "costs performance and smoothes the edges of polygons. A value of 1 turns\n"
                "multisampling off. Good values are dependent on the graphics driver.")
        .def("loadFile", &Player::loadFile,
                "loadFile(fileName) -> None\n\n"
                "Loads the avg file specified in fileName.")
        .def("play", &Player::play,
                "play() -> None\n\n"
                "Opens a playback window or screen and starts playback. framerate is\n"
                "the number of frames per second that should be displayed. play returns\n"
                "when playback has ended.")
        .def("stop", &Player::stop,
                "stop() -> None\n\n"
                "Stops playback and resets the video mode if nessesary.")
        .def("isPlaying", &Player::isPlaying,
                "isPlaying() -> bool\n\n"
                "Returns true if play() is currently executing, false if not.")
        .def("setFramerate", &Player::setFramerate,
                "setFramerate(rate) -> None\n\n"
                "Sets the desired framerate for playback and turns off syncronization\n"
                "to the vertical blanking interval.")
        .def("setVBlankFramerate", &Player::setVBlankFramerate,
                "setVBlankFramerate(rate) -> bool\n\n"
                "Sets the desired number of vertical blanking intervals before the next\n"
                "frame is displayed. The resulting framerate is determined by the\n"
                "monitor refresh rate divided by the rate parameter. On Mac OS X, only 0\n"
                "are supported as rate.")
        .def("getTestHelper", &Player::getTestHelper,
                return_value_policy<reference_existing_object>(),
                "")
        .def("createNode", &Player::createNodeFromXmlString,
                "createNode(xml) -> Node\n\n"
                "Creates a new Node from an xml string. This node can be used as\n"
                "parameter to DivNode::addChild(). BROKEN!")
        .def("setInterval", &Player::setInterval,
                "setInterval(time, pyfunc) -> id\n\n"
                "Sets a python callable object that should be executed every time\n"
                "milliseconds. setInterval returns an id that can be used to\n"
                "call clearInterval() to stop the code from being called.")
        .def("setTimeout", &Player::setTimeout, 
                "setTimeout(time, pyfunc) -> id\n\n"
                "Sets a python callable object that should be executed after time\n"
                "milliseconds. setTimeout returns an id that can be used to\n"
                "call clearInterval() to stop the code from being called.")
        .def("clearInterval", &Player::clearInterval,
                "clearInterval(id) -> ok\n\n"
                "Stops a timeout or an interval from being called. Returns 1 if\n"
                "there was an interval with the given id, 0 if not.\n")
        .def("getCurEvent", &Player::getCurEvent,
                return_value_policy<reference_existing_object>(),
                "getCurEvent() -> Event\n\n"
                "Gets an interface to the current event. Only valid inside event\n"
                "handlers (onmouseup, onmousedown, etc.)")
        .def("getMouseState", &Player::getMouseState,
                return_value_policy<reference_existing_object>(),
                "getMouseState() -> Event\n\n"
                "Gets an interface to the last mouse event.")
        .def("screenshot", &Player::screenshot,
                return_value_policy<manage_new_object>(),
                "screenshot() -> ok\n\n"
                "Returns the contents of the current screen as a bitmap.\n")
        .def("showCursor", &Player::showCursor,
                "showCursor(show) -> None\n\n"
                "Shows or hides the mouse cursor. (Currently, this only works for\n"
                "OpenGL. Showing the DirectFB mouse cursor seems to expose some\n"
                "issue with DirectFB.)")
        .def("getElementByID", &Player::getElementByID,
                "getElementByID(id) -> Node\n\n"
                "Returns an element in the avg tree. The id corresponds to the id\n"
                "attribute of the node.")
        .def("getRootNode", &Player::getRootNode,
                "getRootNode() -> AVGNode\n\n"
                "Returns the outermost element in the avg tree.")
        .def("getFramerate", &Player::getFramerate,
                "getFramerate() -> framerate\n\n"
                "Returns the current framerate in frames per second.")
        .def("getVideoRefreshRate", &Player::getVideoRefreshRate,
                "getVideoRefreshRate() -> refreshrate\n\n"
                "Returns the current hardware video refresh rate in number of\n"
                "refreshes per second.")
        .def("setGamma", &Player::setGamma,
                "setGamma(red, green, blue) -> None\n\n"
                "Sets display gamma. This is a control for overall brightness and\n"
                "contrast that leaves black and white unchanged but adjusts greyscale\n"
                "values. 1.0 is identity, higher values give a brighter image, lower\n"
                "values a darker one.\n")
    ;

}
