/* 

 GeneratePreviewForURL - generate image previews with image details
                         displayed in title bar
 
 History:
 
 v. 0.1.0 (02/17/2012) - Initial Release
 v. 0.1.1 (02/21/2012) - Add support for gigabyte size images
 v. 0.1.2 (05/24/2018) - Add support for dpi and depth
 v. 0.1.3 (09/09/2020) - Add support for webp images
 
 Related links:
 
 http://www.cocoaintheshell.com/2012/02/quicklook-images-dimensions/
 http://www.carbondev.com/site/?page=GetFileSize
 https://github.com/emin/WebPQuickLook
 https://developers.google.com/speed/webp/
 
 Copyright (c) 2012, 2018, 2020 Sriranga Veeraraghavan <ranga@calalum.org>

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

/* includes */

#import <CoreFoundation/CoreFoundation.h>
#import <CoreServices/CoreServices.h>
#import <QuickLook/QuickLook.h>
#import "Globals.h"

/* webp headers */

#include "src/webp/decode.h"
#include "src/webp/demux.h"

/* globals */

const short gWebpLossless = 2;
const long  gMaxWebpSize = 20*1024*1024;

/* webp's UTIs */

const CFStringRef gPublicWebp = CFSTR("public.webp");
const CFStringRef gOrgWebmWebp = CFSTR("org.webmproject.webp");

/* prototypes */

CFStringRef GetFileSizeAsString(CFURLRef url);
OSStatus GeneratePreviewForWebpImage(void *thisInterface,
                                     QLPreviewRequestRef preview,
                                     CFURLRef url,
                                     CFStringRef contentTypeUTI,
                                     CFDictionaryRef options);
OSStatus GeneratePreviewForURL(void *thisInterface,
                               QLPreviewRequestRef preview, 
                               CFURLRef url, 
                               CFStringRef contentTypeUTI, 
                               CFDictionaryRef options);
void CancelPreviewGeneration(void *thisInterface,
                             QLPreviewRequestRef preview);

/* functions */

/* GetFileSizeAsString - returns the size of a given file as a string */

CFStringRef GetFileSizeAsString(CFURLRef url)
{
    FSRef imgRef;
    FSCatalogInfoBitmap whichInfo;
    FSCatalogInfo catalogInfo;
    UInt64 fileSizeInBytes = 0;
    Float64 fileSize = 0.0;
    CFStringRef fileSizeStr = NULL;
    CFStringRef fileSizeSpec = NULL;
    
    if (url == NULL) {
        return NULL;
    }
    
    if (CFURLGetFSRef(url, &imgRef) != TRUE) {
        fileSizeStr =
            CFStringCreateWithFormat(kCFAllocatorDefault,
                                     NULL,
                                     CFSTR(""));
    }

    /*
        figure out the file size by adding up the logical sizes
        of the file's data and resource forks
     */
               
    whichInfo =
        kFSCatInfoNodeFlags |
        kFSCatInfoRsrcSizes |
        kFSCatInfoDataSizes;
    
    if (FSGetCatalogInfo(&imgRef,
                         whichInfo,
                         &catalogInfo,
                         NULL,
                         NULL,
                         NULL) == noErr &&
        !(catalogInfo.nodeFlags & kFSNodeIsDirectoryMask)) {
            fileSizeInBytes += catalogInfo.dataLogicalSize;
            fileSizeInBytes += catalogInfo.rsrcLogicalSize;
    }

    /* format the file size into GB, MB, KB, or Bytes, as appropriate */
    
    fileSizeSpec = CFSTR("B");
    
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
    }
    
    fileSizeStr =
        CFStringCreateWithFormat(kCFAllocatorDefault,
                                 NULL,
                                 CFSTR(" - %.1f %@"),
                                 fileSize,
                                 fileSizeSpec);
    return fileSizeStr;
}

/* GeneratePreviewForWebpImage - generates the quicklook preview for a
   webp image */

OSStatus GeneratePreviewForWebpImage(void *thisInterface,
                                     QLPreviewRequestRef preview,
                                     CFURLRef url,
                                     CFStringRef contentTypeUTI,
                                     CFDictionaryRef options)
{
    CGBitmapInfo bitmapInfo = kCGBitmapByteOrderDefault;
    CGSize imgSize;
    CGContextRef ctx = NULL;
    CGDataProviderRef provider = NULL;
    CGImageRef image = NULL;
    CFDictionaryRef properties = NULL;
    CFStringRef fileName = NULL;
    CFStringRef filePath = NULL;
    CFStringRef keys[1];
    CFStringRef values[1];
    CFStringRef fileSizeStr = NULL;
    CFStringRef animationDetails = NULL;
    CFIndex fileLength;
    CFIndex fileMaxSize;
    Boolean err = false;
    FILE *imageF = NULL;
    WebPBitstreamFeatures features;
    uint8_t *rgbData = NULL;
    WebPData webpData;
    WebPDemuxer *demux = NULL;
    WebPIterator iter;
    uint8_t *data = NULL;
    size_t dataSize = 0;
    char *filePathStr = NULL;
    VP8StatusCode status;
    uint32_t frames = 0;
    int width = 0;
    int height = 0;
    int samples = 3;
    
    do {

        filePath =
            CFURLCreateStringByReplacingPercentEscapes(kCFAllocatorDefault,
                                                       CFURLCopyPath(url),
                                                       CFSTR(""));
        if (filePath == NULL) {
            err = true;
            break;
        }
            
        fileLength  = CFStringGetLength(filePath);
        fileMaxSize =
            CFStringGetMaximumSizeForEncoding(fileLength,
                                              kCFStringEncodingUTF8);

        filePathStr = malloc(fileMaxSize + 1);
        if (filePathStr == NULL) {
            err = true;
            break;
        }

        CFStringGetCString(filePath,
                           filePathStr,
                           fileMaxSize,
                           kCFStringEncodingUTF8);

        CFRelease(filePath);
                    
        /* load the image */
        
        imageF = fopen((const char *)filePathStr, "rb");
        if (imageF == NULL) {
            err = true;
            break;
        }
    
        if (filePathStr != NULL) {
            free(filePathStr);
            filePathStr = NULL;
        }

        fseek(imageF, 0, SEEK_END);
        dataSize = ftell(imageF);
        fseek(imageF, 0, SEEK_SET);

        if (QLPreviewRequestIsCancelled(preview)) {
            break;
        }

        if (dataSize <= 0) {
            err = true;
            break;
        }

        /* if the file is more than 20MB, don't generate a preview */
        
        if (dataSize > gMaxWebpSize) {
            QLPreviewRequestSetURLRepresentation(preview,
                                                 url,
                                                 contentTypeUTI,
                                                 properties);
            break;
        }
        
        data = malloc(dataSize + 1);
        if (data == NULL) {
            err = true;
            break;
        }
        
        if (QLPreviewRequestIsCancelled(preview)) {
            break;
        }

        if (fread(data, dataSize, 1, imageF) != 1) {
            err = true;
            break;
        }
        data[dataSize] = '\0';

        if (QLPreviewRequestIsCancelled(preview)) {
            break;
        }

        status = WebPGetFeatures(data, dataSize, &features);
        if (status != VP8_STATUS_OK) {
            err = true;
            break;
        }
        
        if (features.has_animation) {

            /* animation - extract the first frame and use it as
               the preview */

            webpData.bytes = data;
            webpData.size = dataSize;
            
            demux = WebPDemux(&webpData);
            if (demux == NULL) {
                err = true;
                break;
            }
            
            frames = WebPDemuxGetI(demux, WEBP_FF_FRAME_COUNT);
            if (frames >= 1) {

                if (WebPDemuxGetFrame(demux, 1, &iter)) {

                    if (features.has_alpha) {
                        rgbData = WebPDecodeRGBA(iter.fragment.bytes,
                                                 iter.fragment.size,
                                                 &iter.width,
                                                 &iter.height);
                        samples = 4;
                        bitmapInfo = kCGImageAlphaLast;
                    } else {
                        rgbData = WebPDecodeRGB(iter.fragment.bytes,
                                                iter.fragment.size,
                                                &iter.width,
                                                &iter.height);
                    }

                    width = iter.width;
                    height = iter.height;
                    
                } else {
                    err = true;
                }
                
                WebPDemuxReleaseIterator(&iter);
                
            } else {
                err = true;
            }

            WebPDemuxDelete(demux);
   
            if (err) {
                break;
            }
            
            animationDetails =
                CFStringCreateWithFormat(kCFAllocatorDefault,
                                         NULL,
                                         CFSTR(" - Animated (%d Frames)"),
                                         frames);
        } else {
            
            /* still image */
            
            if (features.has_alpha) {
                rgbData = WebPDecodeRGBA(data, dataSize, &width, &height);
                samples = 4;
                bitmapInfo = kCGImageAlphaLast;
            } else {
                rgbData = WebPDecodeRGB(data, dataSize, &width, &height);
            }

        }

        if (rgbData == NULL) {
            err = true;
            break;
        }

        /* create an image to display from the webp data */
        
        imgSize = CGSizeMake(width, height);
        
        keys[0] = kQLPreviewPropertyDisplayNameKey;

        /* get the image's filename from the image's URL */
        
        fileName = CFURLCopyLastPathComponent(url);

        fileSizeStr = GetFileSizeAsString(url);
        
        values[0] =
            CFStringCreateWithFormat(kCFAllocatorDefault,
                                     NULL,
                                     CFSTR("%@ (%dx%d %@%@%@)"),
                                     fileName == NULL ? CFSTR("(NULL)")
                                                      : fileName,
                                     width,
                                     height,
                                     fileSizeStr == NULL ? CFSTR("")
                                                         : fileSizeStr,
                                     features.format == gWebpLossless
                                     ? CFSTR(" - Lossless")
                                     : CFSTR(""),
                                     animationDetails != NULL
                                     ? animationDetails : CFSTR(""));
        if (values[0] == NULL) {
            err = true;
            break;
        }
        
        fprintf(stderr, "format is %d\n", features.format);
        
        properties = CFDictionaryCreate(kCFAllocatorDefault,
                                        (const void**)keys,
                                        (const void**)values,
                                        1,
                                        &kCFTypeDictionaryKeyCallBacks,
                                        &kCFTypeDictionaryValueCallBacks);

        /*
            we could change boolean isBitMap to false, to avoid debug
            errors, see:
         
            https://github.com/Marginal/QLVideo/commit/028b871abf1bb8bc2f0e29985735b0f3c3e49d4a
            
            However, we aren't doing this because this causes the preview
            images to be very large
         */
        
        ctx =
            QLPreviewRequestCreateContext(preview,
                                          imgSize,
                                          true,
                                          properties == NULL ? options
                                                             : properties);
        if (ctx == NULL) {
            err = true;
            break;
        }
        
        provider = CGDataProviderCreateWithData(NULL,
                                                rgbData,
                                                width*height*samples,
                                                NULL);
        if (provider == NULL) {
            err = true;
            break;
        }
        
        image = CGImageCreate(width,
                              height,
                              8,
                              8 * samples,
                              width * samples,
                              CGColorSpaceCreateDeviceRGB(),
                              bitmapInfo,
                              provider,
                              NULL,
                              false,
                              kCGRenderingIntentDefault);
        if (image == NULL) {
            err = true;
            break;
        }
        
        CGContextDrawImage(ctx, CGRectMake(0, 0, width, height), image);
        CGContextFlush(ctx);
        QLPreviewRequestFlushContext(preview, ctx);
        
    } while (0);
    
    /* clean up */

    if (animationDetails != NULL) {
        CFRelease(animationDetails);
    }
    
    if (data != NULL) {
        free(data);
        data = NULL;
    }
    
    if (imageF != NULL) {
        fclose(imageF);
    }

    if (fileName != NULL) {
        CFRelease(fileName);
    }

    if (fileSizeStr != NULL) {
        CFRelease(fileSizeStr);
    }

    if (image != NULL) {
        CFRelease(image);
    }
    
    if (provider != NULL) {
        CFRelease(provider);
    }
    
    if (properties != NULL) {
        CFRelease(properties);
    }
    
    if (values[0] != NULL) {
        CFRelease(values[0]);
    }
    
    if (ctx != NULL) {
        CFRelease(ctx);
    }
    
    if (rgbData != NULL) {
        WebPFree((void *)rgbData);
    }
    
    return (err == true ? -1 : noErr);
}

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
    CFNumberRef dpiRef = NULL;
    CFNumberRef depthRef = NULL;
    CFStringRef fileName = NULL;
    CFStringRef keys[1];
    CFStringRef values[1];
    CFStringRef fileSizeStr = NULL;
    CFStringRef dpiStr = NULL;
    CFStringRef depthStr = NULL;
    Boolean err = false;
    int width = 0;
    int height = 0;
    int dpi = 0;
    int depth = 0;
    
    /* handle webp images */
    
    if (CFEqual(contentTypeUTI, gPublicWebp) ||
        CFEqual(contentTypeUTI, gOrgWebmWebp)) {

        return GeneratePreviewForWebpImage(thisInterface,
                                           preview,
                                           url,
                                           contentTypeUTI,
                                           options);
    }
    
    /* handle all other image types */
    
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
    
        /* get the DPI of the image */
        
        dpiRef = CFDictionaryGetValue(imgProperties,
                                      kCGImagePropertyDPIHeight);
        if (dpiRef != NULL) {
            CFNumberGetValue(dpiRef, kCFNumberIntType, &dpi);
            dpiStr = CFStringCreateWithFormat(kCFAllocatorDefault,
                                              NULL,
                                              CFSTR(" - %d DPI"),
                                              dpi);
        } else {
            dpiStr = CFStringCreateWithFormat(kCFAllocatorDefault,
                                              NULL,
                                              CFSTR(""));
        }

        /* get the depth of the image */
        
        depthRef = CFDictionaryGetValue(imgProperties,
                                        kCGImagePropertyDepth);
        if (depthRef != NULL) {
            CFNumberGetValue(depthRef, kCFNumberIntType, &depth);
            depthStr = CFStringCreateWithFormat(kCFAllocatorDefault,
                                                NULL,
                                                CFSTR(" - %d BPP"),
                                                depth);
        } else {
            depthStr = CFStringCreateWithFormat(kCFAllocatorDefault,
                                                NULL,
                                                CFSTR(""));
        }

        /* get the image's size */

        fileSizeStr = GetFileSizeAsString(url);

        /* format the title string */
        
        keys[0] = kQLPreviewPropertyDisplayNameKey;
        
        values[0] = CFStringCreateWithFormat(kCFAllocatorDefault, 
                                             NULL, 
                                             CFSTR("%@ (%dx%d%@%@%@)"),
                                             fileName,
                                             width, 
                                             height,
                                             dpiStr,
                                             depthStr,
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

    if (fileSizeStr != NULL) {
        CFRelease(fileSizeStr);
    }

    if (dpiStr != NULL) {
        CFRelease(dpiStr);
    }
    
    if (depthStr != NULL) {
        CFRelease(depthStr);
    }
    
    if (values[0] != NULL) {
        CFRelease(values[0]);
    }
    
    if (properties != NULL) {
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
    
    return (err == true ? -1 : noErr);
}

void CancelPreviewGeneration(void *thisInterface, 
                             QLPreviewRequestRef preview)
{
}
