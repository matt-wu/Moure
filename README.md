
Introduction
------------

    Moure is a small utility to swap your mouse buttons. As we know Windows
    system already supports mouse buttons switching, but it has the following
    limitations:
    1) All mouses including touchpad are to be swapped.
    2) In remote desktop (mstsc), will turn back to what it's designed.

    Moure overcomes these 2 limitations. It enables you to only swap buttons
    for a particular one or any specified mouses. Other mouse devices or 
    your touchpad are left unchanged.

    Moure remembers your settings in registry, and it automatically reloads
    after system booting.


Supported OS
------------

    Windows XP and later
    
    
Story
-----

    Mostly I'm using my left hand to control the mouse (designed for right
    hand). Recently, trying to relieve my left hand from pains of carpal
    tunnel syndrome, I bought a new left-hand mouse (Minicute EZ2), then
    got troubles to get accustomed to the left & right confusions.

    I'm not a guy of IFIXIT-style, just cann't weld any jump wires. But
    as a software engineer, I can make software work as requested, even
    weirdly. Then moure was out, proving again the proverb:

    -- if all you have is a hammer, everything looks like a nail --


Development  Guides
-------------------

    Project website: https://github.com/matt-wu/Moure
    Requirements: VS2008 and WDK

    Moure is a kernel mouse class filter driver for Windows. So besides
    buttons swapping, it can do everything upon HID events for mouses.

    Thanks to Rolf Kristensen for his CGridListCtrlEx project:
    https://github.com/snakefoot/cgridlistctrlex


Bugs or issues
--------------

    Matt Wu <mattwu@163.com>
    http://www.ext2fsd.com
