// vim: ts=4:sw=4
/*
 * screenresolution sets the screen resolution on Mac computers.
 * Copyright (C) 2011  John Ford <john@johnford.info>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <ApplicationServices/ApplicationServices.h>
#include "version.h"

// I have written an alternate list routine that spits out WAY more info
// #define LIST_DEBUG 1

struct config {
    size_t w; // width
    size_t h; // height
    size_t d; // colour depth
    double r; // refresh rate
};

unsigned int setDisplayToMode(CGDirectDisplayID display, CGDisplayModeRef mode);
unsigned int configureDisplay(CGDirectDisplayID display,
                              struct config *config,
                              int displayNum);
unsigned int listCurrentMode(CGDirectDisplayID display, int displayNum);
unsigned int listAvailableModes(CGDirectDisplayID display, int displayNum);
unsigned int parseStringConfig(const char *string, struct config *out);
size_t bitDepth(CGDisplayModeRef mode);

// http://stackoverflow.com/questions/3060121/core-foundation-equivalent-for-nslog/3062319#3062319
//void NSLog(CFStringRef format, ...);



int main(int argc, const char *argv[]) 
{
    
    unsigned int exitcode = 0;

    if (argc > 1) 
    {
        int d;
        int keepgoing = 1;
        CGError rc;
        uint32_t displayCount = 0;
        uint32_t activeDisplayCount = 0;
        CGDirectDisplayID *activeDisplays = NULL;

        rc = CGGetActiveDisplayList(0, NULL, &activeDisplayCount);
        if (rc != kCGErrorSuccess) 
        {
            fprintf(stderr, "Error: failed to get list of active displays.\n");
            return 1;
        }
        // Allocate storage for the next CGGetActiveDisplayList call
        activeDisplays = (CGDirectDisplayID *) malloc(activeDisplayCount * sizeof(CGDirectDisplayID));
        if (activeDisplays == NULL) 
        {
            fprintf(stderr, "Error: could not allocate memory for display list.\n");
            return 1;
        }
        rc = CGGetActiveDisplayList(activeDisplayCount, activeDisplays, &displayCount);
        if (rc != kCGErrorSuccess) 
        {
            fprintf(stderr, "Error: failed to get list of active displays.\n");
            return 1;
        }

        
        // only show results for first display, 'd' was in the original code the loop-variable of a 
        // for-loop over all displays. This is replaces with '0'.
	if (strcmp(argv[1], "get") == 0) 
        {
            if (!listCurrentMode(activeDisplays[0], 0)) 
            {
                exitcode++;
            }
        } 
        else if (strcmp(argv[1], "list") == 0) 
        {
            if (!listAvailableModes(activeDisplays[0], 0)) 
            {
                exitcode++;
            }
        } 
        else if (strcmp(argv[1], "set") == 0) 
        {
            if (argc == 3) 
            {
                struct config newConfig;
                if (parseStringConfig(argv[2], &newConfig)) 
                {
                    if (!configureDisplay(activeDisplays[0], &newConfig, 0)) 
                    {
                        exitcode++;
                    }
                } 
                else 
                {
                    exitcode++;
                }
            }
            else
            {
                fprintf(stderr, "wrong format for screen-resolution to set: '%s'\n", argv[2]);
                exitcode++;
            }
        } 
        else if (strcmp(argv[1], "-version") == 0) 
        {
            printf("screenresolution version %s\nLicensed under GPLv2\n", VERSION);
        } 
        else 
        {
            fprintf(stderr, "The first command-line-argument must be 'get', 'list' or 'set'.\n");
            exitcode++;
        }

        free(activeDisplays);
        activeDisplays = NULL;
    } 

    else 
    {
        fprintf(stderr, "Incorrect command line: Use 'get', 'list', or 'set' as command-line-argument.\n");
        return 1;
    }

    return exitcode > 0;
}



size_t bitDepth(CGDisplayModeRef mode) 
{
    size_t depth = 0;
    CFStringRef pixelEncoding = CGDisplayModeCopyPixelEncoding(mode);
    // my numerical representation for kIO16BitFloatPixels and kIO32bitFloatPixels
    // are made up and possibly non-sensical
    if (kCFCompareEqualTo == CFStringCompare(pixelEncoding, CFSTR(kIO32BitFloatPixels), kCFCompareCaseInsensitive)) 
    {
        depth = 96;
    } 
    else if (kCFCompareEqualTo == CFStringCompare(pixelEncoding, CFSTR(kIO64BitDirectPixels), kCFCompareCaseInsensitive)) 
    {
        depth = 64;
    } 
    else if (kCFCompareEqualTo == CFStringCompare(pixelEncoding, CFSTR(kIO16BitFloatPixels), kCFCompareCaseInsensitive)) 
    {
        depth = 48;
    } 
    else if (kCFCompareEqualTo == CFStringCompare(pixelEncoding, CFSTR(IO32BitDirectPixels), kCFCompareCaseInsensitive)) 
    {
        depth = 32;
    } 
    else if (kCFCompareEqualTo == CFStringCompare(pixelEncoding, CFSTR(kIO30BitDirectPixels), kCFCompareCaseInsensitive)) 
    {
        depth = 30;
    } 
    else if (kCFCompareEqualTo == CFStringCompare(pixelEncoding, CFSTR(IO16BitDirectPixels), kCFCompareCaseInsensitive)) 
    {
        depth = 16;
    } 
    else if (kCFCompareEqualTo == CFStringCompare(pixelEncoding, CFSTR(IO8BitIndexedPixels), kCFCompareCaseInsensitive)) 
    {
        depth = 8;
    }
    CFRelease(pixelEncoding);
    return depth;
}

unsigned int configureDisplay(CGDirectDisplayID display, struct config *config, int displayNum) 
{
    unsigned int returncode = 1;
    CFArrayRef allModes = CGDisplayCopyAllDisplayModes(display, NULL);
    if (allModes == NULL) 
    {
        fprintf(stderr, "Error: failed trying to look up modes for display %u.\n", displayNum);
    }

    CGDisplayModeRef newMode = NULL;
    CGDisplayModeRef possibleMode;
    size_t pw; // possible width.
    size_t ph; // possible height.
    size_t pd; // possible depth.
    double pr; // possible refresh rate
    int looking = 1; // used to decide whether to continue looking for modes.
    int i;
    for (i = 0 ; i < CFArrayGetCount(allModes) && looking; i++) 
    {
        possibleMode = (CGDisplayModeRef)CFArrayGetValueAtIndex(allModes, i);
        pw = CGDisplayModeGetWidth(possibleMode);
        ph = CGDisplayModeGetHeight(possibleMode);
        pd = bitDepth(possibleMode);
        pr = CGDisplayModeGetRefreshRate(possibleMode);
        if (pw == config->w &&
            ph == config->h &&
            pd == config->d &&
            pr == config->r) 
        {
            looking = 0; // Stop looking for more modes!
            newMode = possibleMode;
        }
    }
    CFRelease(allModes);
    if (newMode != NULL) 
    {
        printf("Set mode on display %u to %lux%lux%lu@%.0f\n", displayNum, pw, ph, pd, pr);
        setDisplayToMode(display,newMode);
    } 
    else 
    {
        fprintf(stderr,"Error: mode %lux%lux%lu@%f not available on display %u.\n", 
                config->w, config->h, config->d, config->r, displayNum);
        returncode = 0;
    }
    return returncode;
}

unsigned int setDisplayToMode(CGDirectDisplayID display, CGDisplayModeRef mode) 
{
    CGError rc;
    CGDisplayConfigRef config;
    rc = CGBeginDisplayConfiguration(&config);
    if (rc != kCGErrorSuccess) 
    {
        fprintf(stderr, "Error: failed CGBeginDisplayConfiguration err(%u)\n", rc);
        return 0;
    }
    rc = CGConfigureDisplayWithDisplayMode(config, display, mode, NULL);
    if (rc != kCGErrorSuccess) 
    {
        fprintf(stderr, "Error: failed CGConfigureDisplayWithDisplayMode err(%u)\n", rc);
        return 0;
    }
    rc = CGCompleteDisplayConfiguration(config, kCGConfigureForSession);
    if (rc != kCGErrorSuccess) 
    {
        fprintf(stderr, "Error: failed CGCompleteDisplayConfiguration err(%u)\n", rc);        
        return 0;
    }
    return 1;
}

unsigned int listCurrentMode(CGDirectDisplayID display, int displayNum) 
{
    unsigned int returncode = 1;
    CGDisplayModeRef currentMode = CGDisplayCopyDisplayMode(display);
    if (currentMode == NULL) 
    {
        fprintf(stderr, "Error: unable to copy current display mode.\n");
        returncode = 0;
    }
    printf("%lux%lux%lu@%.0f\n",
           CGDisplayModeGetWidth(currentMode),
           CGDisplayModeGetHeight(currentMode),
           bitDepth(currentMode),
           CGDisplayModeGetRefreshRate(currentMode));
    CGDisplayModeRelease(currentMode);
    return returncode;
}

unsigned int listAvailableModes(CGDirectDisplayID display, int displayNum) {
    unsigned int returncode = 1;
    int i;
    CFArrayRef allModes = CGDisplayCopyAllDisplayModes(display, NULL);
    if (allModes == NULL) 
    {
        returncode = 0;
    }
    CGDisplayModeRef mode;
    for (i = 0; i < CFArrayGetCount(allModes) && returncode; i++) 
    {
        mode = (CGDisplayModeRef) CFArrayGetValueAtIndex(allModes, i);
        // This formatting is functional but it ought to be done less poorly.
#ifndef LIST_DEBUG
        printf("%lux%lux%lu@%.0f\n",
               CGDisplayModeGetWidth(mode),
               CGDisplayModeGetHeight(mode),
               bitDepth(mode),
               CGDisplayModeGetRefreshRate(mode));
#else
        uint32_t ioflags = CGDisplayModeGetIOFlags(mode);
        printf("display: %d %4lux%4lux%2lu@%.0f usable:%u ioflags:%4x valid:%u safe:%u default:%u",
                displayNum,
                CGDisplayModeGetWidth(mode),
                CGDisplayModeGetHeight(mode),
                bitDepth(mode),
                CGDisplayModeGetRefreshRate(mode),
                CGDisplayModeIsUsableForDesktopGUI(mode),
                ioflags,
                ioflags & kDisplayModeValidFlag ?1:0,
                ioflags & kDisplayModeSafeFlag ?1:0,
                ioflags & kDisplayModeDefaultFlag ?1:0 );
        printf(" safety:%u alwaysshow:%u nevershow:%u notresize:%u requirepan:%u int:%u simul:%u",
                ioflags & kDisplayModeSafetyFlags ?1:0,
                ioflags & kDisplayModeAlwaysShowFlag ?1:0,
                ioflags & kDisplayModeNeverShowFlag ?1:0,
                ioflags & kDisplayModeNotResizeFlag ?1:0,
                ioflags & kDisplayModeRequiresPanFlag ?1:0,
                ioflags & kDisplayModeInterlacedFlag ?1:0,
                ioflags & kDisplayModeSimulscanFlag ?1:0 );
        printf(" builtin:%u notpreset:%u stretched:%u notgfxqual:%u valagnstdisp:%u tv:%u vldmirror:%u\n",
                ioflags & kDisplayModeBuiltInFlag ?1:0,
                ioflags & kDisplayModeNotPresetFlag ?1:0,
                ioflags & kDisplayModeStretchedFlag ?1:0,
                ioflags & kDisplayModeNotGraphicsQualityFlag ?1:0,
                ioflags & kDisplayModeValidateAgainstDisplay ?1:0,
                ioflags & kDisplayModeTelevisionFlag ?1:0,
                ioflags & kDisplayModeValidForMirroringFlag ?1:0 );
#endif
    }
    CFRelease(allModes);
    return returncode;
}

unsigned int parseStringConfig(const char *string, struct config *out) {
    unsigned int rc;
    size_t w;
    size_t h;
    size_t d;
    double r;
    int numConverted = sscanf(string, "%lux%lux%lu@%lf", &w, &h, &d, &r);
    if (numConverted != 4) 
    {
        numConverted = sscanf(string, "%lux%lux%lu", &w, &h, &d);
        if (numConverted != 3) 
        {
            numConverted = sscanf(string, "%lux%lu", &w, &h);
            if (numConverted != 2)
            {
                rc = 0;
                fprintf(stderr,"Error: the mode '%s' couldn't be parsed.\n", string);
            }
            else
            {
                out->w = w;
                out->h = h;
                out->d = 32;
                out->r = 60.0;
                printf("Warning: no pixel-depth and no refresh rate specified, assuming 32 bit and 60.0Hz.\n");
                rc=1;
            }
        } 
        else 
        {
            out->w = w;
            out->h = h;
            out->d = d;
            out->r=60.0;
            rc = 1;
            printf("Warning: no refresh rate specified, assuming 60.0Hz.\n");
        }
    }
    else 
    {
        out->w = w;
        out->h = h;
        out->d = d;
        out->r = r;
        rc = 1;
    }
    return rc;
}
