
//    SAPF - Sound As Pure Form
//    Copyright (C) 2019 James McCartney
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "DeviceSelection.hpp"
#include "Object.hpp"
#include <AudioToolbox/AudioToolbox.h>

void printAvailableAudioDevices(void)
{
    // 1. Ask the HAL how many devices it has
    AudioObjectPropertyAddress addr = {
        kAudioHardwarePropertyDevices,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMain
    };
    UInt32 dataSize = 0;
    OSStatus err = AudioObjectGetPropertyDataSize(
        kAudioObjectSystemObject,
        &addr,
        0, NULL,
        &dataSize
    );
    if (err != noErr) {
        post("Error getting device list size: %d\n", (int)err);
        return;
    }

    UInt32 deviceCount = dataSize / sizeof(AudioDeviceID);
    AudioDeviceID *devices = (AudioDeviceID*)malloc(dataSize);
    if (!devices) return;

    err = AudioObjectGetPropertyData(
        kAudioObjectSystemObject,
        &addr,
        0, NULL,
        &dataSize,
        devices
    );
    if (err != noErr) {
        post("Error getting device IDs: %d\n", (int)err);
        free(devices);
        return;
    }

    // 2. For each device, fetch its name and channel counts
    for (UInt32 i = 0; i < deviceCount; ++i) {
        AudioDeviceID dev = devices[i];

        // — Name
        CFStringRef cfName = NULL;
        AudioObjectPropertyAddress nameAddr = {
            kAudioDevicePropertyDeviceNameCFString,
            kAudioObjectPropertyScopeGlobal,
            kAudioObjectPropertyElementMain
        };
        UInt32 nameSize = sizeof(cfName);
        err = AudioObjectGetPropertyData(
            dev,
            &nameAddr,
            0, NULL,
            &nameSize,
            &cfName
        );

        char nameBuf[256] = "Unknown";
        if (err == noErr && cfName) {
            CFStringGetCString(cfName, nameBuf, sizeof(nameBuf),
                               kCFStringEncodingUTF8);
            CFRelease(cfName);
        }

        // — Helper to get total channels for a given scope
        auto getChannelCount = ^(AudioObjectPropertyScope scope) {
            AudioObjectPropertyAddress cfgAddr = {
                kAudioDevicePropertyStreamConfiguration,
                scope,
                kAudioObjectPropertyElementMain
            };
            UInt32 cfgSize = 0;
            AudioObjectGetPropertyDataSize(dev, &cfgAddr,
                                           0, NULL, &cfgSize);
            AudioBufferList *bufList = (AudioBufferList*)malloc(cfgSize);
            AudioObjectGetPropertyData(dev, &cfgAddr,
                                       0, NULL, &cfgSize, bufList);
            UInt32 totalCh = 0;
            for (UInt32 b = 0; b < bufList->mNumberBuffers; ++b)
                totalCh += bufList->mBuffers[b].mNumberChannels;
            free(bufList);
            return totalCh;
        };

        UInt32 inCh  = getChannelCount(kAudioObjectPropertyScopeInput);
        UInt32 outCh = getChannelCount(kAudioObjectPropertyScopeOutput);

        post("%s — input channels: %u, output channels: %u\n",
               nameBuf, (unsigned)inCh, (unsigned)outCh);
    }

    free(devices);
}

AudioDeviceID getDeviceIDByName(const char* deviceName)
{
    AudioObjectPropertyAddress addr = {
        kAudioHardwarePropertyDevices,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMain
    };
    UInt32 dataSize = 0;
    if (AudioObjectGetPropertyDataSize(kAudioObjectSystemObject,
                                       &addr, 0, NULL,
                                       &dataSize) != noErr)
        return kAudioObjectUnknown;

    UInt32 count = dataSize / sizeof(AudioDeviceID);
    AudioDeviceID *devices = (AudioDeviceID*)malloc(dataSize);
    if (!devices) return kAudioObjectUnknown;

    if (AudioObjectGetPropertyData(kAudioObjectSystemObject,
                                   &addr, 0, NULL,
                                   &dataSize, devices) != noErr) {
        free(devices);
        return kAudioObjectUnknown;
    }

    AudioDeviceID found = kAudioObjectUnknown;
    for (UInt32 i = 0; i < count; ++i) {
        CFStringRef cfName = NULL;
        AudioObjectPropertyAddress nameAddr = {
            kAudioDevicePropertyDeviceNameCFString,
            kAudioObjectPropertyScopeGlobal,
            kAudioObjectPropertyElementMain
        };
        UInt32 nameSize = sizeof(cfName);
        if (AudioObjectGetPropertyData(devices[i],
                                       &nameAddr, 0, NULL,
                                       &nameSize, &cfName) == noErr && cfName)
        {
            char buf[256] = {0};
            CFStringGetCString(cfName, buf, sizeof(buf), kCFStringEncodingUTF8);
            CFRelease(cfName);

            if (strcmp(buf, deviceName) == 0) {
                found = devices[i];
                break;
            }
        }
    }

    free(devices);
    return found;
}

bool isValidDevice(unsigned deviceID)
{
	if ((AudioDeviceID)deviceID != kAudioObjectUnknown)
		return true;
	else
		return false;
}

