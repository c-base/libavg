Player & Canvas
===============

.. automodule:: libavg.avg
    :no-members:

    This section describes the classes that provide a framework for rendering. The 
    :py:class:`Player` class is an interface to the avg renderer. The 
    :py:class:`Canvas` class and it's descendant :py:class:`OffscreenCanvas` provide 
    areas to draw on. 
    
    .. autoclass:: Canvas

        A Canvas is a tree of nodes. It is the place where a scenegraph is displayed. In
        a libavg session, there is one main canvas that corresponds to the screen (which
        is of class :py:class:`Canvas`) and zero or more canvases that are rendered 
        offscreen (which are of class :py:class:`OffscreenCanvas`). 

        .. py:method:: getElementByID(id) -> Node

            Returns the element in the canvas's tree that has the :py:attr:`id`
            given.
        
        .. py:method:: screenshot() -> Bitmap

            Returns the image the canvas has last rendered as :py:class:`Bitmap`. For
            the main canvas, this is a real screenshot. For offscreen canvases, this 
            is the image rendered offscreen.
        
        .. py:method:: getRootNode() -> CanvasNode

            Returns the root of the scenegraph. For the main canvas, this is an 
            :py:class:`AVGNode`. For an offscreen canvas, this is a 
            :py:class:`CanvasNode`.
        
    .. autoclass:: OffscreenCanvas

        An OffscreenCanvas is a Canvas that is rendered to a texture. It can be
        referenced in the href attribute of an image node. See 
        https://www.libavg.de/wiki/ProgrammersGuide/OffscreenRendering for an in-depth 
        explanation of using offscreen rendering. Offscreen canvases are created by
        calling :py:meth:`Player.loadCanvasFile` and :py:meth:`Player.loadCanvasString`.

        .. py:attribute:: autorender

            Turns autorendering on or off. Default is :py:const:`True`.

        .. py:attribute:: handleevents

            :py:const:`True` if events that arrive at an image node that is displaying 
            this canvas are routed to the offscreen canvas. Read-only.

        .. py:attribute:: mipmap

            :py:const:`True` if mipmaps are generated and used for the canvas. This is 
            used instead of RasterNode.mipmap for images that render the canvas. 
            Read-only.

        .. py:attribute:: multisamplesamples

            Number of samples per pixel to use for multisampling. Setting this to
            1 disables multisampling. Read-only.

        .. py:method:: getID() -> string

            Returns the id of the canvas. This is the same as
            calling :samp:`canvas.getRootNode().getID()`.

        .. py:method:: getNumDependentCanvases

            Returns the number of canvases that reference this canvas. Used mainly
            for unit tests.
                
        .. py:method:: registerCameraNode

        .. py:method:: render()

            Forces an immediate redraw of the offscreen canvas. This makes sure that
            following calls to screenshot() get a current version of the canvas and 
            is usually used in combination with :samp:`autorender=False`.

        .. py:method:: unregisterCameraNode

        .. py:classmethod:: isMultisampleSupported() -> bool

            :py:const:`True` if the machine's OpenGL implementation supports offscreen 
            multisampling.

    .. autoclass:: Player

        The class used to load and play avg files and the main interface to the avg
        renderer. Player is a singleton. There is only one instance, accessed by 
        :py:meth:`get`.

        .. py:attribute:: pluginPath

            A colon-separated list of directories where the player
            searches for plugins when :py:meth:`loadPlugin()` is called.

        .. py:attribute:: volume

            Total audio playback volume. 0 is silence, 1 passes media file
            volume through unchanged. Values higher than 1 can be used to
            amplify playback. A limiter prevents distortion when the volume
            is set to high.

        .. py:method:: addTracker()

            Adds a camera-based multitouch tracker to the avg player. The tracker can be
            configured using the :file:`avgtrackerrc` file and immediately starts
            reporting events.

            .. deprecated:: 1.5
                Use :func:`enableMultitouch()` instead. 

        .. py:method:: assumePhysicalScreenDimensions(sizeInMM)

            Tells the system to assume a size for the physical screen, overriding 
            operating system information. The parameter is the size in millimeters as a
            :py:class:`Point2D`. This function affects the values returned by 
            :py:meth:`getPhysicalScreenDimensions` and :py:meth:`getDPI`. It is useful for
            situations in which the OS cannot know the size (e.g. projectors) and when
            the automatic functions return wrong values (which happens, unfortunately).

        .. py:method:: clearInterval(id) -> bool

            Stops a timeout, an interval or an onFrameHandler from being called.
            Returns :py:const:`True` if there was an interval with the given 
            :py:attr:`id`, :py:const:`False` if not.
            
            :param int id: 
            
                An id returned by :py:meth:`setInterval`, :py:meth:`setTimeout` 
                or :py:meth:`setOnFrameHandler`.

        .. py:method:: createNode(xml) -> Node

            Creates a new Node. This node can be used as parameter to 
            :py:meth:`DivNode.appendChild()` and :py:meth:`DivNode.insertChild()`.
            This method will create any type of node, including :samp:`<div>` nodes
            with children.

            :param xml: 
            
                xml string conforming to the avg dtd that specifies the node to create.

        .. py:method:: createNode(type, args) -> Node

            Creates a new Node. This node can be used as parameter to 
            :py:meth:`DivNode.appendChild()` and :py:meth:`DivNode.insertChild()`.
            This method will only create one node at a time.

            :param string type: 
            
                Type string of the node to create (For example, :samp:`image` and 
                :samp:`words` are valid type strings).

            :param dict args: a dictionary specifying attributes of the node.

        .. py:method:: deleteCanvas(id)

            Removes the canvas given by id from the player's internal list of
            canvases. It is an error to delete a canvas that is still referenced by
            an image node.

        .. py:method:: enableMultitouch

            Enables multitouch event handling. Several drivers are available that 
            generate multitouch events. To choose a driver, set the environment
            variable :envvar:`AVG_MULTITOUCH_DRIVER` to the appropriate value:

            :samp:`TUIO`:
                Listens for TUIO events from a tracker that conforms to the TUIO 
                protocol (http://www.tuio.org), a de-facto standard for multitouch
                events. By default, it listens to events on the default TUIO UDP port
                3333, but this can be configured using the environment variable 
                :envvar:`AVG_TUIO_PORT`.

            :samp:`APPLETRACKPAD`:
                Uses the trackpad built into Mac Book Pros to generate events.

            :samp:`LINUXMTDEV`:
                Uses the linux mtdev library to interface to multitouch devices.
                The environment variable :envvar:`AVG_LINUX_MULTITOUCH_DEVICE` is used
                to determine which device file to open. Default is 
                :samp:`/dev/input/event3`.

            :samp:`TRACKER`:
                Enables the internal camera-based tracker. Configuring this tracker is
                described under https://www.libavg.de/wiki/ProgrammersGuide/Tracker.

            :samp:`XINPUT21`:
                Uses X11-based multitouch detection. This needs X11 with XInput 2.1
                support.

            If :envvar:`AVG_MULTITOUCH_DRIVER` is not set, the driver defaults to 
            a plattform-specific one. Under Linux, the default is :samp:`XINPUT21` if
            XInput 2.1 is available on the system, otherwise :samp:`LINUXMTDEV`. Other
            plattforms will follow.

            :py:meth:`enableMultitouch` throws an exception if the chosen driver is not
            available or no multitouch device could be found. (Exception: Since there is
            no way to determine if a TUIO device is available, :py:meth:`enableMultitouch`
            always appears to succeed in this case.)

        .. py:method:: getCanvas(id) -> OffscreenCanvas

            Returns the offscreen canvas with the :py:attr:`id` given.

        .. py:method:: getDPI() -> dpi

            Returns the number of dots per inch of the primary display as a 
            :py:class:`Point2D`. 

        .. py:method:: getEffectiveFramerate() -> float

            Returns the framerate that the player is actually achieving. The
            value returned is not averaged and reflects only the current frame.

        .. py:method:: getElementByID(id) -> Node

            Returns an element in the main avg tree.
            
            :param id: id attribute of the node to return.

        .. py:method:: getEventHook() -> pyfunc

            Returns the last event hook set using :py:meth:`setEventHook`.

        .. py:method:: getFrameDuration() -> int

            Returns the duration of the last frame in milliseconds.

        .. py:method:: getFramerate() -> float

            Returns the current target framerate in frames per second. To get the 
            actual framerate that the player is currently achieving, call 
            :py:meth:`getEffectiveFramerate`.

        .. py:method:: getFrameTime() -> int

            Returns the number of milliseconds that have elapsed since playback
            has started. Honors FakeFPS. The time returned stays constant for an
            entire frame; it is the time of the last display update.

        .. py:method:: getKeyModifierState() -> KeyModifier

            Returns the current modifier keys (:kbd:`shift`, :kbd:`ctrl`) pressed. 
            The return value is several KeyModifier values or'ed together.

        .. py:method:: getMainCanvas() -> Canvas

            Returns the main canvas. This is the canvas loaded using :py:meth:`loadFile`
            or :py:meth:`loadString` and displayed on screen.

        .. py:method:: addInputDevice(inputDevice)

            Registers an :py:class:`InputDevice` with the system.

        .. py:method:: getMouseState() -> MouseEvent

            Returns the last mouse event generated.

        .. py:method:: getPhysicalScreenDimensions() -> Point2D

            Returns the size of the primary screen in millimeters.

        .. py:method:: getRootNode() -> Node

            Returns the outermost element in the main avg tree.

        .. py:method:: getScreenResolution() -> Point2D

            Returns the size in pixels of the current screen.

        .. py:method:: getTestHelper

        .. py:method:: getTimeSinceLastFrame() -> int

            Returns the number of milliseconds that have elapsed since the last
            frame (i.e. the last display update).

        .. py:method:: getTracker() -> Tracker

            Returns a tracker previously created with :py:meth:`addTracker` or 
            :py:meth:`enableMultitouch` with the internal tracker configured.

        .. py:method:: getVideoRefreshRate() -> float

            Returns the current hardware video refresh rate in number of
            refreshes per second.

        .. py:method:: isMultitouchAvailable() -> bool

            Returns :py:const:`True` if a multitouch device has been configured and is
            active, :py:const:`False` if not. Must be called after :py:meth:`play()`.

        .. py:method:: isPlaying() -> bool

            Returns :py:const:`True` if :py:meth:`play()` is currently executing, 
            :py:const:`False` if not.

        .. py:method:: isUsingShaders() -> bool

            Returns :py:const:`True` if shader support is enabled and working,
            :py:const:`False` if not.
            May only be called after :py:meth:`play()` has been called.

        .. py:method:: loadCanvasFile(filename) -> Canvas

            Loads the canvas file specified in filename and adds it to the
            registered offscreen canvases.

        .. py:method:: loadCanvasString(avgString) -> Canvas

            Parses avgString, loads the nodes it contains and adds the hierarchy
            to the registered offscreen canvases.
            
            :param string avgString: An xml string containing an avg node hierarchy.

        .. py:method:: loadFile(filename) -> Canvas

            Loads the avg file specified in filename. Returns the canvas loaded.
            The canvas is the main canvas displayed onscreen.

        .. py:method:: loadPlugin(name)

            Load a Plugin.
        
            :param string name: filename of the plugin without directory and
                file extension.

        .. py:method:: loadString(avgString) -> Canvas

            Parses avgString and loads the nodes it contains. Returns the canvas
            loaded. The canvas is the main canvas displayed onscreen.

            :param string avgString: An xml string containing an avg node hierarchy.

        .. py:method:: play()

            Opens a playback window or screen and starts playback. play returns
            when playback has ended.

        .. py:method:: screenshot() -> Bitmap

            Returns the contents of the current screen as a bitmap.

        .. py:method:: setCursor(bitmap, hotspot)

            Sets the mouse cursor to the bitmap given. The bitmap must have a size
            divisible by 8 and an RGBA pixel format. The cursor generated is
            binary black and white with a binary transparency channel. hotspot is
            the relative position of the actual pointing coordinate in the
            bitmap.

        .. py:method:: setEventHook(pyfunc)

            Set a callable which will receive all events before the standard event 
            handlers receive them. If this callable returns :py:const:`True`,
            the event is not propagated to the standard event handlers.
        
            Generally, :py:meth:`setEventHook` should be used as a last resort. In most
            cases, standard event handlers are a lot cleaner. Also, setting several event
            hooks is not supported by libavg. To get around this limitation, you can
            use :py:meth:`getEventHook` to chain event hook functions.
            
            Note that :py:attr:`event.node` is not set in the callback, since the
            system hasn't determined the node to send the event to at that
            point. 
            
        .. py:method:: setFakeFPS(fps)

            Sets a fixed number of virtual frames per second that are used as
            clock source for video playback, animations and other time-based
            actions. If a value of :samp:`-1` is given as parameter, the real clock is
            used. :py:meth:`setFakeFPS` can be used to get reproducible results for 
            recordings or automated tests. Setting FakeFPS has the side-effect of
            disabling audio.

        .. py:method:: setFramerate(framerate)

            Sets the desired framerate for playback. Turns off syncronization
            to the vertical blanking interval.

        .. py:method:: setGamma(red, green, blue)

            Sets display gamma. This is a control for overall brightness and
            contrast that leaves black and white unchanged but adjusts greyscale
            values. :samp:`1.0` is identity, higher values give a brighter image, lower
            values a darker one.

        .. py:method:: setInterval(time, pyfunc) -> int

            Sets a python callable object that should be executed regularly.
            :py:meth:`setInterval` returns an id that can be used to
            call :py:meth:`clearInterval()` to stop the function from being called. The
            callback is called at most once per frame.

            :param int time: Number of milliseconds between two calls.

            :param pyfunc: Python callable to execute.

        .. py:method:: setMousePos(pos)

            Sets the position of the mouse cursor. Generates a mouse motion event.

        .. py:method:: setMultiSampleSamples(multiSampleSamples)

            Sets the number of samples per pixel to compute.
            This costs performance and smoothes the edges of polygons. A value of
            :samp:`1`  turns multisampling (also knowna as FSAA - Full-Screen
            Antialiasing) off. Good values are dependent on the graphics driver and 
            the performance of the graphics card.

        .. py:method:: setOGLOptions(usePOW2Textures, useShaders, usePixelBuffers, multiSampleSamples)

            Determines which OpenGL extensions to check for and use if possible.
            This method is mainly used for debugging purposes while developing libavg, 
            but can also be used to work around buggy drivers. The values set here
            override those in the :file:`avgrc` file. Note that with the exception of
            multiSampleSamples, fallbacks are always used - if a feature is
            specified that the system doesn't support, a less demanding one will
            be used.

            Must be called before :py:meth:`play`.

            :param bool usePOW2Textures: 
            
                If :py:const:`True`, restricts textures to power-of-two dimensions.

            :param bool useShaders: 
            
                If :py:const:`True`, shaders are used to render effects,
                do masking and do color space conversions. If :py:const:`False`, video
                color space conversion is done on the CPU and effects as well as masking
                are turned off.
                
            :param bool usePixelBuffers: 
                
                If :py:const:`False`, disables the use of OpenGL pixel buffer objects.

            :param int MultiSampleSamples: 
            
                The number of samples per pixel to compute.
                This costs performance and smoothes the edges of polygons. A value of
                :samp:`1` turns multisampling (also known as FSAA - Full-Screen 
                Antialiasing) off. Good values are dependent on the graphics driver and 
                the performance of the graphics card.

        .. py:method:: setOnFrameHandler(pyfunc) -> int

            Sets a python callable object that should be executed once per frame.
            This is the same as :samp:`setInterval(0, pyfunc)`. Returns an id that can 
            be used to call :py:meth:`clearInterval()` to stop the function from being 
            called.

            :param pyfunc: Python callable to execute.

        .. py:method:: setResolution(fullscreen, width, height, bpp)

            Sets display engine parameters. Must be called before :py:meth:`loadFile` or
            :py:meth:`loadString`.
                
            :param bool fullscreen: 
            
                :py:const:`True` if the avg file should be rendered fullscreen.

            :param int width, height: 
            
                The window size (if fullscreen is :py:const:`False`)
                or screen resolution (if fullscreen is :py:const:`True`).

            :param int bpp: 
            
                Number of bits per pixel to use. Valid values are :samp:`15`, :samp:`16`,
                :samp:`24` and :samp:`32`.

        .. py:method:: isFullscreen()

            Returns True when the player is running in fullscreen mode.
            
        .. py:method:: setTimeout(time, pyfunc) -> int

            Sets a python callable object that should be executed after a set
            amount of time. :py:meth:`setTimeout` returns an id that can be used to
            call :py:meth:`clearInterval()` to stop the function from being called.

            :param int time: Number of milliseconds before the call.

            :param pyfunc: Python callable to execute.

        .. py:method:: setVBlankFramerate(rate)

            Sets the desired number of monitor refreshes before the next
            frame is displayed. The resulting framerate is determined by the
            monitor refresh rate divided by the rate parameter.

            :param int rate: 
            
                Number of vertical blanking intervals to wait. On Mac OS X, only :samp:`1`
                is supported as rate.

        .. py:method:: setWindowFrame(hasWindowFrame)

            :py:attr:`hasWindowFrame` should be set to :py:const:`True` if a 
            non-fullscreen player should have a window frame. If set to 
            :py:const:`False`, the player runs with no title bar or window frame. Must
            be called before :py:meth:`play` is called.

        .. py:method:: setWindowPos(x, y)

            Sets the location of the player window. Must be called before loadFile
            or loadString.

        .. py:method:: showCursor(show)

            Shows or hides the mouse cursor.
            
            :param bool show: :py:const:`True` if the mouse cursor should be visible.

        .. py:method:: stop()

            Stops playback and resets the video mode if necessary.

        .. py:method:: stopOnEscape(stop)

            Toggles player stop upon escape keystroke. If stop is :py:const:`True` 
            (the default), if player will halt playback when :kbd:`Esc` is pressed.

        .. py:classmethod:: get() -> Player

            This method gives access to the player instance. If no player has been 
            created yet, a player is created.

    
