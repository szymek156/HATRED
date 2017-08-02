#pragma once
#include <stdio.h>
#include <windows.h>
#include <mmsystem.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <avrt.h>
#include <functiondiscoverykeys_devpkey.h>

class AudioCapture
{
public:
    AudioCapture();
    virtual ~AudioCapture();

    void run();

protected:
    bool findDefaultAudioDevice();
    bool initAudioClient();

    bool getDeviceFormat();

    void prepareFile();
    bool initCapture();

    IMMDevice *mDefaultAudioDevice;
    IAudioClient *mAudioClient;
    IAudioCaptureClient *mAudioCaptureClient;

    WAVEFORMATEX *mDeviceFormat;
};

