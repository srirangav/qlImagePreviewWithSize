/* 

 GeneratePreviewForURL - generate image previews with image dimensions
                         and size displayed in title bar
 $Id: GeneratePreviewForURL.c 1435 2015-08-14 16:48:34Z ranga $
 
 History:
 
 v. 0.1.0 (02/17/2012) - Initial Release
 v. 0.1.1 (02/21/2012) - Add support of gigabyte size images
 
 Related links:
 
 http://www.cocoaintheshell.com/2012/02/quicklook-images-dimensions/
 http://www.carbondev.com/site/?page=GetFileSize
 
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

/* prototypes */

OSStatus GeneratePreviewForURL(void *thisInterface, 
                               QLPreviewRequestRef preview, 
                               CFURLRef url, 
                               CFStringRef contentTypeUTI, 
                               CFDictionaryRef options);
void CancelPreviewGeneration(void *thisInterface, 
                             QLPreviewRequestRef preview);

/* functions */

/* GeneratePreviewForURL - generates the quicklook preview for a given file */

OSStatus GeneratePreviewForURL(void *thisInterface, 
                               QLPreviewRequestRef preview, 
                               CFURLRef url, 
                               CFStringRef contentTypeUTI, 
                               CFDictionaryRef options)
{
    CGImageSourceRef imgSrc = NULL;
    CFDictionaryRef imgProperties = NULL;
    CFDictionaryRef properties = NULL;
    CFNumberRef widthRef = NULL;
    CFNumberRef heightRef = NULL;
    CFStringRef fileName = NULL;
    CFStringRef keys[1];
    CFStringRef values[1];
    CFStringRef fileSizeStr = NULL;
    CFStringRef fileSizeSpec = NULL;
    FSRef imgRef;
    FSCatalogInfoBitmap whichInfo;
    FSCatalogInfo catalogInfo;
    UInt64 fileSizeInBytes = 0;
    Float64 fileSize = 0.0;
    Boolean err = false;
    int width = 0;
    int height = 0;
    
    do {

        /* get the image's filename from the image's URL */
        
        fileName = CFURLCopyLastPathComponent(url);
        if (fileName == NULL) {
            err = true;
            break;
        }
        
        /* create an image source from the url and get its properties */
        
        imgSrc = CGImageSourceCreateWithURL(url, NULL);    
        if (imgSrc == NULL) {
            err = true;
            break;
        }

        imgProperties = CGImageSourceCopyPropertiesAtIndex(imgSrc, 0, NULL);
        if (imgProperties == NULL) {
            err = true;
            break;
        }
        
        /* get the image's width from its properties */
        
        widthRef = CFDictionaryGetValue(imgProperties, 
                                        kCGImagePropertyPixelWidth);
        if (widthRef == NULL) {
            err = true;
            break;
        }
        CFNumberGetValue(widthRef, kCFNumberIntType, &width);
        
        /* get the image's height from its properties */
        
        heightRef = CFDictionaryGetValue(imgProperties, 
                                         kCGImagePropertyPixelHeight);
        if (heightRef == NULL) {
            err = true;
            break;
        }
        CFNumberGetValue(heightRef, kCFNumberIntType, &height);
    
        /* determine the size of the image */
        
        if (CFURLGetFSRef(url, &imgRef) == TRUE) {

            /* figure out the file size by adding up the logical sizes
               of the file's data and resource forks */
            
             whichInfo = 
                kFSCatInfoNodeFlags | 
                kFSCatInfoRsrcSizes | 
                kFSCatInfoDataSizes;
            if (FSGetCatalogInfo(&imgRef, whichInfo, &catalogInfo, 
                                 NULL, NULL,NULL) == noErr &&
                !(catalogInfo.nodeFlags & kFSNodeIsDirectoryMask)) {
                fileSizeInBytes += catalogInfo.dataLogicalSize;
                fileSizeInBytes += catalogInfo.rsrcLogicalSize;
            }

            /* format the file size into GB, MB, KB, or Bytes, as 
               appropriate */
            
            if (fileSizeInBytes >= 100) {
                
                fileSize = fileSizeInBytes / 1000.0;
                if (fileSize >= 1000.0) {
                    fileSize /= 1000.0;                    
                    if (fileSize >= 1000.0) {
                        fileSize /= 1000.0;
                        fileSizeSpec = CFSTR("GB");
                    } else {                    
                        fileSizeSpec = CFSTR("MB");
                    }
                } else {
                    fileSizeSpec = CFSTR("KB");
                }
                
                fileSizeStr = 
                    CFStringCreateWithFormat(kCFAllocatorDefault, 
                                             NULL,
                                             CFSTR(" - %.1f %@"),
                                             fileSize,
                                             fileSizeSpec);
            } else {
                fileSizeStr = 
                    CFStringCreateWithFormat(kCFAllocatorDefault, 
                                             NULL,
                                             CFSTR(" - %f B"),
                                             fileSize);
            }
                        
        } else {
            fileSizeStr = 
                CFStringCreateWithFormat(kCFAllocatorDefault, 
                                         NULL,
                                         CFSTR("")); 

        }

        /* format the title string */
        
        keys[0] = kQLPreviewPropertyDisplayNameKey;
        
        values[0] = CFStringCreateWithFormat(kCFAllocatorDefault, 
                                             NULL, 
                                             CFSTR("%@ (%dx%d%@)"),
                                             fileName,
                                             width, 
                                             height,
                                             fileSizeStr);
                
        properties = CFDictionaryCreate(kCFAllocatorDefault, 
                                        (const void**)keys, 
                                        (const void**)values, 1, 
                                        &kCFTypeDictionaryKeyCallBacks, 
                                        &kCFTypeDictionaryValueCallBacks);
    } while (0);
    
    QLPreviewRequestSetURLRepresentation(preview, 
                                         url, 
                                         contentTypeUTI, 
                                         properties);

    /* clean up */
    
    if (properties != NULL) {
        CFRelease(fileSizeStr);
        CFRelease(values[0]);
        CFRelease(properties);
    }
    
    if (imgProperties != NULL) {
        CFRelease(imgProperties);
    }

    if (imgSrc != NULL) {
        CFRelease(imgSrc);
    }
    
    if (fileName != NULL) {
        CFRelease(fileName);
    }
    
    return (err == true ? noErr : -1);
}

void CancelPreviewGeneration(void *thisInterface, 
                             QLPreviewRequestRef preview)
{
}
