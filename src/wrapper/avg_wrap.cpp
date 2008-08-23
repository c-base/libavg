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

void export_node();
void export_event();
#ifndef WIN32
void export_devices();
#endif

#include "WrapHelper.h"

#include "../base/Logger.h"
#include "../base/Exception.h"
#include "../player/Player.h"
#include "../player/AVGNode.h"
#include "../player/DivNode.h"
#include "../player/PanoImage.h"
#include "../player/TrackerEventSource.h"

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
    docstring_options doc_options(true, false);

    scope().attr("__doc__") =
        "The main libavg module.\n"
        "G{classtree Node}\n"
        "G{classtree Event}\n"
        "G{classtree Bitmap}\n"
        "G{classtree Tracker TrackerCalibrator}\n"
        "G{classtree Logger}\n"
        "G{classtree ConradRelais ParPort}";

#if (BOOST_VERSION / 100000) > 1 || ((BOOST_VERSION / 100) % 1000) >= 33
    register_exception_translator<Exception>(exception_translator);
#endif
    register_ptr_to_python<DivNodePtr>();
    register_ptr_to_python<AVGNodePtr>();
    register_ptr_to_python<EventPtr>();
    register_ptr_to_python<MouseEventPtr>();

    to_python_converter<IntPoint, Point_to_python_tuple<int> >();
    DPoint_from_python_tuple();

    class_<Logger>("Logger", 
            "Interface to the logger used by the avg player. Enables the setting\n"
            "of different logging categories. Log output can be sent to the console, a\n"
            "file or unix syslog.\n"
            "Each log entry contains the time the message was written, the category\n"
            "of the entry and the message itself.\n",
            no_init)
        .def("get", &Logger::get, 
                return_value_policy<reference_existing_object>(),
                "This method gives access to the logger. There is only one instance.\n")
        .staticmethod("get")
        .def("setConsoleDest", &Logger::setConsoleDest,
                "Sets the log to be output to the console (stderr, to be precise)."
                "This is the default.")
        .def("setFileDest", &Logger::setFileDest,
                "Sets a file that the log should be written to. If opening the file\n"
                "fails, the console is used instead.\n"
                "@param filename: \n")
        .def("setSyslogDest", &Logger::setSyslogDest,
                "Causes log output to be written to the unix system log facility.\n"
                "syslog ident is set to 'libavg'.\n"
                "@param facility: Passed to the system log verbatim. See man 3 syslog.\n"
                "@param logopt: Passed to the system log verbatim. See man 3 syslog.")
        .def("setCategories", &Logger::setCategories,
                "Sets the types of messages that should be logged.\n" 
                "@param categories: Or'ed list of categories. Possible categories are:\n"
                "    - NONE: No logging except for errors.\n"
                "    - BLTS: Display subsystem logging. Useful for timing/performance"
                "            measurements.\n"
                "    - PROFILE: Outputs performance statistics on player termination.\n"
                "    - PROFILE_LATEFRAMES: Outputs performance statistics whenever a"
                "                          frame is displayed late.\n"
                "    - EVENTS: Outputs basic event data.\n"
                "    - EVENTS2: Outputs all event data available.\n"
                "    - CONFIG: Outputs configuration data.\n"
                "    - WARNING: Outputs warning messages. Default is on.\n"
                "    - ERROR: Outputs error messages. Can't be shut off.\n"
                "    - MEMORY: Outputs open/close information whenever a media file is accessed.\n"
                "    - APP: Reserved for application-level messages issued by python code.\n")
        .def("trace", &Logger::trace,
                "Logs message to the log if category is active.\n"
                "@param category: One of the categories listed for setCategories(). Should\n"
                "in most cases be APP.\n"
                "@param message: The log message.\n")
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
    export_node();

    enum_<YCbCrMode>("YCbCrMode")
        .value("shader", OGL_SHADER)
        .value("mesa", OGL_MESA)
        .value("apple", OGL_APPLE)
        .value("none", OGL_NONE)
        .export_values()
    ;

    class_<TestHelper>("TestHelper", "Miscelaneous routines used by tests.", no_init)
        .def("getNumDifferentPixels", &TestHelper::getNumDifferentPixels, "")
        .def("useFakeCamera", &TestHelper::useFakeCamera, "")
        .def("fakeMouseEvent", &TestHelper::fakeMouseEvent, "")
        .def("fakeKeyEvent", &TestHelper::fakeKeyEvent, "")
        .def("dumpObjects", &TestHelper::dumpObjects, "")
    ;

    class_<Player>("Player", 
                "The class used to load and play avg files.")
        .def("get", &Player::get, 
                return_value_policy<reference_existing_object>(),
                "This method gives access to the player, which must have been created.\n"
                "before by calling the constructor.\n")
        .staticmethod("get")
        .def("setResolution", &Player::setResolution,
                "setResolution(fullscreen, width, height, bpp)\n"
                "Sets display engine parameters. Must be called before loadFile or\n"
                "loadString.\n"
                "@param fullscreen: True if the avg file should be rendered fullscreen.\n"
                "@param width, height: Set the window size\n"
                "(if fullscreen is false) or screen resolution (if fullscreen is true).\n"
                "@param bpp: Number of bits per pixel to use.\n")
        .def("setWindowPos", &Player::setWindowPos,
                "setWindowPos(x, y)\n"
                "Sets the location of the player window. Must be called before loadFile\n"
                "or loadString.\n")
        .def("setOGLOptions", &Player::setOGLOptions,
                "setOGLOptions(UsePOW2Textures, YCbCrMode, UsePixelBuffers, MultiSampleSamples)\n"
                "Determines which OpenGL extensions to check for and use if possible.\n"
                "Mainly used for debugging purposes while developing libavg, but can\n"
                "also be used to work around buggy drivers. The values set here override\n"
                "those in avgrc. Note that with the exception of MultiSampleSamples,\n"
                "fallbacks are always used - if a feature is\n"
                "specified that the system doesn't support, a less demanding one will\n"
                "be used.\n"
                "@param UsePOW2Textures: If True, restricts textures to power-of-two dimensions.\n"
                "@param YCbCrMode: Selects the preferred method of copying video textures\n"
                "to the screen. Can be shader, mesa, apple or none.\n"
                "@param UsePixelBuffers: If False, disables the use of OpenGL pixel buffer\n"
                "objects.\n"
                "@param MultiSampleSamples: The number of samples per pixel to compute. This\n"
                "costs performance and smoothes the edges of polygons. A value of 1 turns\n"
                "multisampling (also knowna as FSAA - Full-Screen Antialiasing) off. Good\n"
                "values are dependent on the graphics driver.\n")
        .def("loadFile", &Player::loadFile,
                "loadFile(filename)\n"
                "Loads the avg file specified in filename.\n"
                "@param filename: ")
        .def("loadString", &Player::loadString,
                "loadString(avgString)\n"
                "Parses avgString and loads the nodes it contains.\n"
                "@param avgString: An xml string containing an avg node hierarchy.")
        .def("play", &Player::play,
                "play()\n"
                "Opens a playback window or screen and starts playback. play returns\n"
                "when playback has ended.\n")
        .def("stop", &Player::stop,
                "stop()\n"
                "Stops playback and resets the video mode if necessary.\n")
        .def("isPlaying", &Player::isPlaying,
                "isPlaying() -> bool\n"
                "Returns True if play() is currently executing, False if not.\n")
        .def("setFramerate", &Player::setFramerate,
                "setFramerate(framerate)\n"
                "Sets the desired framerate for playback. Turns off syncronization\n"
                "to the vertical blanking interval.\n"
                "@param framerate: ")
        .def("setVBlankFramerate", &Player::setVBlankFramerate,
                "setVBlankFramerate(rate)\n"
                "Sets the desired number of monitor refreshes before the next\n"
                "frame is displayed. The resulting framerate is determined by the\n"
                "monitor refresh rate divided by the rate parameter.\n"
                "@param rate: Number of vertical blanking intervals to wait.\n")
        .def("getEffectiveFramerate", &Player::getEffectiveFramerate,
                "getEffectiveFramerate() -> framerate\n"
                "Returns the framerate that the player is actually achieving. The\n"
                "value returned is not averaged and reflects only the current frame.\n")
        .def("getTestHelper", &Player::getTestHelper,
                return_value_policy<reference_existing_object>(),
                "")
        .def("setFakeFPS", &Player::setFakeFPS,
                "setFakeFPS(fps)\n"
                "Sets a fixed number of virtual frames per second that are used as clock\n"
                "source for video playback, animations and other time-based actions.\n"
                "If a value of -1 is given as parameter, the real clock is used.\n"
                "@param fps: \n")
        .def("getFrameTime", &Player::getFrameTime,
                "getFrameTime() -> time\n"
                "Returns the number of milliseconds that have elapsed since playback has\n"
                "started. Honors FakeFPS. The time returned stays constant for an entire\n"
                "frame; it is the time of the last display update.\n")
        .def("createNode", &Player::createNodeFromXmlString,
                "createNode(xml) -> node\n"
                "Creates a new Node. This node can be used as\n"
                "parameter to DivNode::appendChild() and insertChild().\n"
                "This method will create any type of node, including <div> nodes\n"
                "with children.\n"
                "@param xml: xml string in avg syntax that specifies the node to create.")
        .def("createNode", &Player::createNode,
                "createNode(type, args) -> node\n"
                "Creates a new Node. This node can be used as\n"
                "parameter to DivNode::appendChild() and insertChild().\n"
                "This method will only create one node at a time.\n"
                "@param type: type string of the node to create.\n"
                "@param args: a dictionary specifying attributes of the node.")
        .def("addTracker", &Player::addTracker,
                return_value_policy<reference_existing_object>(),
                "addTracker(configFilename)\n"
                "Adds a camera-based tracker to the avg player. The tracker can be\n"
                "configured using the config file given and immediately starts\n"
                "reporting events.")
        .def("setInterval", &Player::setInterval,
                "setInterval(time, pyfunc) -> id\n"
                "Sets a python callable object that should be executed regularly.\n"
                "setInterval returns an id that can be used to\n"
                "call clearInterval() to stop the function from being called. The\n"
                "callback is called at most once per frame.\n"
                "@param time: Number of milliseconds between two calls.\n"
                "@param pyfunc: Python callable to execute.\n")
        .def("setTimeout", &Player::setTimeout, 
                "setTimeout(time, pyfunc) -> id\n"
                "Sets a python callable object that should be executed after a set\n"
                "amount of time. setTimeout returns an id that can be used to\n"
                "call clearInterval() to stop the function from being called.\n"
                "@param time: Number of milliseconds before the call.\n"
                "@param pyfunc: Python callable to execute.\n")
        .def("setOnFrameHandler", &Player::setOnFrameHandler,
                "setOnFrameHandler(pyfunc) -> id\n"
                "Sets a python callable object that should be executed once per frame.\n"
                "Returns an id that can be used to call clearInterval() to stop the\n"
                "function from being called.\n"
                "@param pyfunc: Python callable to execute.\n")
        .def("clearInterval", &Player::clearInterval,
                "clearInterval(id) -> ok\n"
                "Stops a timeout, an interval or an onFrameHandler from being called.\n"
                "Returns True if there was an interval with the given id, False if not.\n"
                "@param id: An id returned by setInterval, setTimeout or setOnFrameHandler.\n")
        .def("getMouseState", &Player::getMouseState,
                "getMouseState() -> event\n"
                "Returns an interface to the last mouse event.\n")
        .def("screenshot", &Player::screenshot,
                return_value_policy<manage_new_object>(),
                "screenshot() -> bitmap\n"
                "Returns the contents of the current screen as a bitmap.\n")
        .def("showCursor", &Player::showCursor,
                "showCursor(show)\n"
                "Shows or hides the mouse cursor.\n"
                "@param show: True if the mouse cursor should be visible.\n")
        .def("getElementByID", &Player::getElementByID,
                "getElementByID(id) -> node\n"
                "Returns an element in the avg tree.\n"
                "@param id: id attribute of the node to return.\n")
        .def("getRootNode", &Player::getRootNode,
                "getRootNode() -> node\n"
                "Returns the outermost element in the avg tree.\n")
        .def("getFramerate", &Player::getFramerate,
                "getFramerate() -> rate\n"
                "Returns the current target framerate in frames per second.\n")
        .def("getVideoRefreshRate", &Player::getVideoRefreshRate,
                "getVideoRefreshRate() -> rate\n"
                "Returns the current hardware video refresh rate in number of\n"
                "refreshes per second.\n")
        .def("setGamma", &Player::setGamma,
                "setGamma(red, green, blue)\n"
                "Sets display gamma. This is a control for overall brightness and\n"
                "contrast that leaves black and white unchanged but adjusts greyscale\n"
                "values. 1.0 is identity, higher values give a brighter image, lower\n"
                "values a darker one.\n"
                "@param red, green, blue: \n")
        .def("getGPUMemoryUsage", &Player::getGPUMemoryUsage,
                "getGPUMemoryUsage() -> bytes\n"
                "Get the amount of memory currently allocated for textures storaging\n"
                "on the graphics adapter.\n")
        .add_property("volume", &Player::getVolume, &Player::setVolume,
                "Total audio playback volume. 0 is silence, 1 passes media file\n"
                "volume through unchanged. Values higher than 1 can be used to\n"
                "amplify playback. A limiter prevents distortion when the volume\n"
                "is set to high.\n")
    ;

}
