README
------

qlImagePreviewWithSize v0.4
By Sriranga Veeraraghavan <ranga@calalum.org>

qlImagePreviewWithSize is a QuickLook generator for images that includes 
the dimensions and other information about the image in the header.  It 
is based on a similar plugin available here:

http://www.cocoaintheshell.com/2012/02/quicklook-images-dimensions/

Webp support is based on a similar plugin available here:

https://github.com/emin/WebPQuickLook

Install:

    1. Copy qlImagePreviewWithSize.qlgenerator to ~/Library/QuickLook

    2. Restart QuickLook:

       /usr/bin/qlmanage -r
       /usr/bin/qlmanage -r cache

History:

    v.0.4 - add webp support
    v.0.3 - add DPI and depth to header
    v.0.2 - update for Xcode 9.1
    v.0.1 - initial release

License:

    Please see LICENSE.txt

Known Issues:

    1. If Pixelmator Pro is installed, this quicklook generator will not
       produce a preview for a webp image because Quicklook always 
       prefers generators that are included in an application and there 
       is no way to override this behavior without editing Pixelmator 
       Pro.  See:

       https://stackoverflow.com/questions/11705425/prefer-my-quicklook-plugin

    2. If a webp image is over 20MB in size, a preview or thumbnail will 
       not be generated

    3. Playback of animated webp images is not supported.  The preview 
       and thumbnail for such images is the first frame of the animation.

    4. This quicklook generator will not work on Catalina (10.15.4) or 
       later because Apple no longer allows third party quicklook 
       generators to override the default quicklook generator for 
       public.image.

