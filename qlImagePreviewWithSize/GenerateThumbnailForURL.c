/* 
 
 GenerateThumnailForURL - generate a thumbnail for an image
 $Id: GenerateThumbnailForURL.c 1179 2012-02-23 09:35:40Z ranga $
 
 History:
 
 v. 0.1.0 (02/17/2012) - Initial Release
 v. 0.2.0 (09/10/2020) - add webp support
 
 Related links:
 
 http://www.cocoaintheshell.com/2012/02/quicklook-images-dimensions/
 https://github.com/emin/WebPQuickLook
 https://developers.google.com/speed/webp/

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

/* includes */

#import <CoreFoundation/CoreFoundation.h>
#import <CoreServices/CoreServices.h>
#import <QuickLook/QuickLook.h>
#import "Globals.h"

/* webp headers */

#include "src/webp/decode.h"
#include "src/webp/demux.h"

/* protoypes */

OSStatus GenerateThumbnailForURL(void *thisInterface, 
                                 QLThumbnailRequestRef thumbnail, 
                                 CFURLRef url, 
                                 CFStringRef contentTypeUTI, 
                                 CFDictionaryRef options, 
                                 CGSize maxSize);
void CancelThumbnailGeneration(void *thisInterface, 
                               QLThumbnailRequestRef thumbnail);

/* GenerateThumbnailForURL - generate a thumbnail for a given file */

OSStatus GenerateThumbnailForURL(void *thisInterface, 
                                 QLThumbnailRequestRef thumbnail, 
                                 CFURLRef url, 
                                 CFStringRef contentTypeUTI, 
                                 CFDictionaryRef options, 
                                 CGSize maxSize)
{
    CGSize imgSize;
    CGContextRef ctx = NULL;
    CGDataProviderRef provider = NULL;
    CGImageRef image = NULL;
    CFStringRef filePath = NULL;
    CFIndex fileLength;
    CFIndex fileMaxSize;
    Boolean err = false;
    FILE *imageF = NULL;
    WebPBitstreamFeatures features;
    uint8_t *rgbData = NULL;
    WebPData webpData;
    WebPDemuxer *demux = NULL;
    WebPIterator iter;
    WebPDecoderConfig config;
    uint8_t *data = NULL;
    size_t dataSize = 0;
    char *filePathStr = NULL;
    VP8StatusCode status;
    int width = 0;
    int height = 0;
    int thumbnailWidth = 0;
    int thumbnailHeight = 0;

    /* use default thumbnail generator for all non-webp images */
    
    if (!(CFEqual(contentTypeUTI, gPublicWebp) ||
          CFEqual(contentTypeUTI, gOrgWebmWebp))) {
        QLThumbnailRequestSetImageAtURL(thumbnail, url, NULL);
        return noErr;
    }
    
    /* generate thumbnails for webp images */
    
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

        if (dataSize <= 0) {
            err = true;
            break;
        }
        
        data = malloc(dataSize + 1);
        if (data == NULL) {
            err = true;
            break;
        }
                  
        if (fread(data, dataSize, 1, imageF) != 1) {
            err = true;
            break;
        }
        data[dataSize] = '\0';

        status = WebPGetFeatures(data, dataSize, &features);
        if (status != VP8_STATUS_OK) {
            err = true;
            break;
        }

        thumbnailWidth = (int)maxSize.width;
        thumbnailHeight = (int)maxSize.height;
        
        WebPGetInfo(&data[0], dataSize, &width, &height);

        /*
            scale the thumbnail's dimensions to match the original image's
            dimensions
         */
        
        if (width > height) {
            thumbnailHeight =
                (int)(thumbnailWidth * (float)((float)height/(float)width));
        } else {
            thumbnailWidth =
                (int)(thumbnailHeight * (float)((float)width/(float)height));
        }
        
        WebPInitDecoderConfig(&config);
        
        config.options.use_scaling = 1;
        config.options.scaled_width = thumbnailWidth;
        config.options.scaled_height = thumbnailHeight;
        config.output.colorspace = MODE_RGBA;
        
        if (features.has_animation) {

            /* animation - extract the first frame and use it as
               the basis for the thumbnail */

            webpData.bytes = data;
            webpData.size = dataSize;
            
            demux = WebPDemux(&webpData);
            if (demux == NULL) {
                err = true;
                break;
            }

            if (WebPDemuxGetFrame(demux, 1, &iter)) {
                WebPDecode(iter.fragment.bytes, dataSize, &config);
                WebPDemuxReleaseIterator(&iter);
            } else {
                err = true;
            }
            
            WebPDemuxDelete(demux);
            
            if (err == true) {
                break;
            }
            
        } else {
            
            /* still image */
            
            WebPDecode(&data[0], dataSize, &config);
        }
        
        rgbData = config.output.u.RGBA.rgba;
        if (rgbData == NULL) {
            err = true;
            break;
        }

        imgSize = CGSizeMake(thumbnailWidth, thumbnailHeight);

        ctx = QLThumbnailRequestCreateContext(thumbnail,
                                              imgSize,
                                              true,
                                              options);
        
        provider =
            CGDataProviderCreateWithData(NULL,
                                         rgbData,
                                         thumbnailWidth * thumbnailHeight * 4,
                                         NULL);
        if (provider == NULL) {
            err = true;
            break;
        }
        
        image =  CGImageCreate(thumbnailWidth,
                               thumbnailHeight,
                               8,
                               32,
                               thumbnailWidth * 4,
                               CGColorSpaceCreateDeviceRGB(),
                               kCGImageAlphaLast,
                               provider,
                               NULL,
                               false,
                               kCGRenderingIntentDefault);
                        
        if (ctx != NULL) {
            CGContextDrawImage(ctx,
                               CGRectMake(0,
                                          0,
                                          thumbnailWidth,
                                          thumbnailHeight),
                               image);
            QLThumbnailRequestFlushContext(thumbnail, ctx);
            CGContextFlush(ctx);
        }
        
        QLThumbnailRequestSetImage(thumbnail, image, NULL);
        
    } while (0);
 
    /* clean up */
   
    if (filePath != NULL) {
        CFRelease(filePath);
    }
    
    if (data != NULL) {
        free(data);
        data = NULL;
    }
           
    if (imageF != NULL) {
        fclose(imageF);
    }

    if (image != NULL) {
        CFRelease(image);
    }
           
    if (provider != NULL) {
        CFRelease(provider);
    }
                                 
    if (ctx != NULL) {
        CFRelease(ctx);
    }
           
    if (rgbData != NULL) {
        WebPFree((void *)rgbData);
    }
               
    return (err == true ? -1 : noErr);
}

void CancelThumbnailGeneration(void *thisInterface, 
                               QLThumbnailRequestRef thumbnail)
{
}
