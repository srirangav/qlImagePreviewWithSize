/* 
 
 GenerateThumnailForURL - generate a thumbnail for an image
 $Id: GenerateThumbnailForURL.c 1179 2012-02-23 09:35:40Z ranga $
 
 History:
 
 v. 0.1.0 (02/17/2012) - Initial Release
 
 Related links:
 
 http://www.cocoaintheshell.com/2012/02/quicklook-images-dimensions/
 
 Copyright (c) 2012 Sriranga R. Veeraraghavan <ranga@calalum.org>
 
 Permission is hereby granted, free of charge, to any person obtaining 
 a copy of this software and associated documentation files (the "Software"),
 to deal in the Software without restriction, including without limitation
 the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 and/or sell copies of the Software, and to permit persons to whom the 
 Software is furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included
 in all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
 THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 DEALINGS IN THE SOFTWARE.
*/

#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>
#include <QuickLook/QuickLook.h>

OSStatus GenerateThumbnailForURL(void *thisInterface, 
                                 QLThumbnailRequestRef thumbnail, 
                                 CFURLRef url, 
                                 CFStringRef contentTypeUTI, 
                                 CFDictionaryRef options, 
                                 CGSize maxSize);
void CancelThumbnailGeneration(void *thisInterface, 
                               QLThumbnailRequestRef thumbnail);

/* GenerateThumbnailForURL - generate's a thumbnail for a given file */

OSStatus GenerateThumbnailForURL(void *thisInterface, 
                                 QLThumbnailRequestRef thumbnail, 
                                 CFURLRef url, 
                                 CFStringRef contentTypeUTI, 
                                 CFDictionaryRef options, 
                                 CGSize maxSize)
{
    QLThumbnailRequestSetImageAtURL(thumbnail, url, NULL);
    return noErr;
}

void CancelThumbnailGeneration(void *thisInterface, 
                               QLThumbnailRequestRef thumbnail)
{
}
