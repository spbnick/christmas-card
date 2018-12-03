Christmas-card
==============

Christmas-card is a firmware for a simple one-off blinking christmas card
board.

[![LEDs off, front][thumb_off_front]][photo_off_front][![Rotating][thumb_rotation]][photo_rotation][![LEDs on, front][thumb_on_front]][photo_on_front]
[![Demo video thumbnail][thumb_demo]][video_demo]

Building
--------
First you'll need the compiler. If you're using a Debian-based Linux you can
get it by installing `gcc-arm-none-eabi` package. Then you'll need the
[libstammer][libstammer] library. Build it first, then export environment
variables pointing to it:

    export CFLAGS=-ILIBSTAMMER_DIR LDFLAGS=-LLIBSTAMMER_DIR

`LIBSTAMMER_DIR` above is the directory where libstammer build is located.

After that you can build the program using `make`.

Hardware
--------

The hardware is very basic: just a cheap ["Blue Pill" board][BluePill] based
on [STM32F103][STM32F103], with a series of five [TLC5916][TLC5916] LED
drivers attached via SPI and controlling 40 lights (41 LEDs). Power is
supplied through the Micro USB cable attached to the Blue Pill.

The front of the card is made from black cardboard with silver-colored paper
on the front, and colored tissue paper on the back for the lights. Two layers
of black EVA foam below that provide spacing and light insulation for the
surface-mount LEDs. Holes were made with punches of various sizes. Round
pieces of white polyethylene foam are inserted into the light holes for
diffusion.

Four colored LEDs are used: white, red, yellow, and green. The tissue paper is
used for providing color when LEDs are off, and improving color richness when
LEDs are on. The card is held together by acrylic glass on the front, and the
prototype board with all the electronic at the back, attached with four 2mm
screws. Stands are made from thick copper wire insulated with heat-shrink
tubing.

Photos
------

[![off_front][thumb_off_front]][photo_off_front]
[![off_front_left][thumb_off_front_left]][photo_off_front_left]
[![off_right][thumb_off_right]][photo_off_right]
[![off_back_right][thumb_off_back_right]][photo_off_back_right]
[![off_back][thumb_off_back]][photo_off_back]
[![off_back_left][thumb_off_back_left]][photo_off_back_left]

[BluePill]: http://item.taobao.com/item.htm?spm=a1z10.1-c.w4004-9679183684.4.26lLDG&id=22097803050
[STM32F103]: http://www.st.com/en/microcontrollers/stm32f103.html
[TLC5916]: http://www.ti.com/product/TLC5916
[photo_off_back]: photos/off_back.jpg
[photo_off_back_left]: photos/off_back_left.jpg
[photo_off_back_right]: photos/off_back_right.jpg
[photo_off_front]: photos/off_front.jpg
[photo_off_front_left]: photos/off_front_left.jpg
[photo_off_right]: photos/off_right.jpg
[photo_on_front]: photos/on_front.jpg
[photo_rotation]: photos/rotation.gif
[thumb_off_back]: thumbs/off_back.jpg
[thumb_off_back_left]: thumbs/off_back_left.jpg
[thumb_off_back_right]: thumbs/off_back_right.jpg
[thumb_off_front]: thumbs/off_front.jpg
[thumb_off_front_left]: thumbs/off_front_left.jpg
[thumb_off_right]: thumbs/off_right.jpg
[thumb_on_front]: thumbs/on_front.jpg
[thumb_rotation]: thumbs/rotation.gif
[thumb_demo]: thumbs/demo.jpg
[video_demo]: https://youtu.be/78oc3YEZnP0
[libstammer]: https://github.com/spbnick/libstammer
