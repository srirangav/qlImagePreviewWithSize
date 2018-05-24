README
------

qlImagePreviewWithSize v0.3
By Sriranga Veeraraghavan <ranga@calalum.org>

qlImagePreviewWithSize is a QuickLook generator for images that
includes the dimensions, dpi, depth and size of images in the 
header.  It is based on a similar plugin available here:

http://www.cocoaintheshell.com/2012/02/quicklook-images-dimensions/

To install:

1. Copy qlImagePreviewWithSize.qlgenerator to ~/Library/QuickLook

2. Restart QuickLook: 

   /usr/bin/qlmanage -r 
   /usr/bin/qlmanage -r cache
   
History:

v.0.3 - add DPI and depth to header
v.0.2 - update for Xcode 9.1
v.0.1 - initial release

License:

Please see LICENSE.txt
