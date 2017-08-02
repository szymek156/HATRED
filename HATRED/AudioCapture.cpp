#include "stdafx.h"
#include "AudioCapture.h"
#include <mmdeviceapi.h>

AudioCapture::AudioCapture() : mDefaultAudioDevice(nullptr), mAudioClient(nullptr), mDeviceFormat(nullptr), mAudioCaptureClient(nullptr)
{
    CoInitialize(NULL);
}


AudioCapture::~AudioCapture()
{
    if (mAudioClient != nullptr)
    {
        mAudioClient->Release();
    }

    if (mDefaultAudioDevice != nullptr)
    {
        mDefaultAudioDevice->Release();
    }

    if (mAudioCaptureClient != nullptr)
    {
        mAudioCaptureClient->Release();
    }

    CoTaskMemFree(mDeviceFormat);

    CoUninitialize();
}


// This shit is only for debug purpouses, will be anihilated after adding codec lib.
HRESULT open_file(LPCWSTR szFileName, HMMIO *phFile) {
    MMIOINFO mi = { 0 };

    *phFile = mmioOpen(
        // some flags cause mmioOpen write to this buffer
        // but not any that we're using
        const_cast<LPWSTR>(szFileName),
        &mi,
        MMIO_WRITE | MMIO_CREATE
        );

    if (NULL == *phFile) {
        wprintf(L"mmioOpen(\"%ls\", ...) failed. wErrorRet == %u", szFileName, mi.wErrorRet);
        return E_FAIL;
    }

    return S_OK;
}

HRESULT WriteWaveHeader(HMMIO hFile, LPCWAVEFORMATEX pwfx, MMCKINFO *pckRIFF, MMCKINFO *pckData) {
    MMRESULT result;

    // make a RIFF/WAVE chunk
    pckRIFF->ckid = MAKEFOURCC('R', 'I', 'F', 'F');
    pckRIFF->fccType = MAKEFOURCC('W', 'A', 'V', 'E');

    result = mmioCreateChunk(hFile, pckRIFF, MMIO_CREATERIFF);
    if (MMSYSERR_NOERROR != result) {
        wprintf(L"mmioCreateChunk(\"RIFF/WAVE\") failed: MMRESULT = 0x%08x", result);
        return E_FAIL;
    }

    // make a 'fmt ' chunk (within the RIFF/WAVE chunk)
    MMCKINFO chunk;
    chunk.ckid = MAKEFOURCC('f', 'm', 't', ' ');
    result = mmioCreateChunk(hFile, &chunk, 0);
    if (MMSYSERR_NOERROR != result) {
        wprintf(L"mmioCreateChunk(\"fmt \") failed: MMRESULT = 0x%08x", result);
        return E_FAIL;
    }

    // write the WAVEFORMATEX data to it
    LONG lBytesInWfx = sizeof(WAVEFORMATEX) + pwfx->cbSize;
    LONG lBytesWritten =
        mmioWrite(
        hFile,
        reinterpret_cast<PCHAR>(const_cast<LPWAVEFORMATEX>(pwfx)),
        lBytesInWfx
        );
    if (lBytesWritten != lBytesInWfx) {
        wprintf(L"mmioWrite(fmt data) wrote %u bytes; expected %u bytes", lBytesWritten, lBytesInWfx);
        return E_FAIL;
    }

    // ascend from the 'fmt ' chunk
    result = mmioAscend(hFile, &chunk, 0);
    if (MMSYSERR_NOERROR != result) {
        wprintf(L"mmioAscend(\"fmt \" failed: MMRESULT = 0x%08x", result);
        return E_FAIL;
    }

    // make a 'fact' chunk whose data is (DWORD)0
    chunk.ckid = MAKEFOURCC('f', 'a', 'c', 't');
    result = mmioCreateChunk(hFile, &chunk, 0);
    if (MMSYSERR_NOERROR != result) {
        wprintf(L"mmioCreateChunk(\"fmt \") failed: MMRESULT = 0x%08x", result);
        return E_FAIL;
    }

    // write (DWORD)0 to it
    // this is cleaned up later
    DWORD frames = 0;
    lBytesWritten = mmioWrite(hFile, reinterpret_cast<PCHAR>(&frames), sizeof(frames));
    if (lBytesWritten != sizeof(frames)) {
        wprintf(L"mmioWrite(fact data) wrote %u bytes; expected %u bytes", lBytesWritten, (UINT32)sizeof(frames));
        return E_FAIL;
    }

    // ascend from the 'fact' chunk
    result = mmioAscend(hFile, &chunk, 0);
    if (MMSYSERR_NOERROR != result) {
        wprintf(L"mmioAscend(\"fact\" failed: MMRESULT = 0x%08x", result);
        return E_FAIL;
    }

    // make a 'data' chunk and leave the data pointer there
    pckData->ckid = MAKEFOURCC('d', 'a', 't', 'a');
    result = mmioCreateChunk(hFile, pckData, 0);
    if (MMSYSERR_NOERROR != result) {
        wprintf(L"mmioCreateChunk(\"data\") failed: MMRESULT = 0x%08x", result);
        return E_FAIL;
    }

    return S_OK;
}

HRESULT FinishWaveFile(HMMIO hFile, MMCKINFO *pckRIFF, MMCKINFO *pckData) {
    MMRESULT result;

    result = mmioAscend(hFile, pckData, 0);
    if (MMSYSERR_NOERROR != result) {
        wprintf(L"mmioAscend(\"data\" failed: MMRESULT = 0x%08x", result);
        return E_FAIL;
    }

    result = mmioAscend(hFile, pckRIFF, 0);
    if (MMSYSERR_NOERROR != result) {
        wprintf(L"mmioAscend(\"RIFF/WAVE\" failed: MMRESULT = 0x%08x", result);
        return E_FAIL;
    }

    return S_OK;
}

void AudioCapture::prepareFile()
{
#define DEFAULT_FILE L"loopback-capture.wav"

    HMMIO hFile;
    open_file(DEFAULT_FILE, &hFile);

    MMCKINFO ckRIFF = { 0 };
    MMCKINFO ckData = { 0 };
    WriteWaveHeader(hFile, mDeviceFormat, &ckRIFF, &ckData);
}

bool AudioCapture::getDeviceFormat()
{
    // get the default device format
    
    HRESULT result = mAudioClient->GetMixFormat(&mDeviceFormat);
    if (FAILED(result))
    {
        wprintf(L"IAudioClient::GetMixFormat failed: hr = 0x%08x", result);
        return false;
    }
    
    
    // coerce int-16 wave format
    // can do this in-place since we're not changing the size of the format
    // also, the engine will auto-convert from float to int for us
    switch (mDeviceFormat->wFormatTag)
    {
        case WAVE_FORMAT_IEEE_FLOAT:
        {
            mDeviceFormat->wFormatTag       = WAVE_FORMAT_PCM;
            mDeviceFormat->wBitsPerSample   = 16;
            mDeviceFormat->nBlockAlign      = mDeviceFormat->nChannels * mDeviceFormat->wBitsPerSample / 8;
            mDeviceFormat->nAvgBytesPerSec  = mDeviceFormat->nBlockAlign * mDeviceFormat->nSamplesPerSec;
            break;
        }
        case WAVE_FORMAT_EXTENSIBLE:
        {
            // naked scope for case-local variable
            PWAVEFORMATEXTENSIBLE pEx = reinterpret_cast<PWAVEFORMATEXTENSIBLE>(mDeviceFormat);
            if (IsEqualGUID(KSDATAFORMAT_SUBTYPE_IEEE_FLOAT, pEx->SubFormat)) 
            {
                pEx->SubFormat                      = KSDATAFORMAT_SUBTYPE_PCM;
                pEx->Samples.wValidBitsPerSample    = 16;
                mDeviceFormat->wBitsPerSample       = 16;
                mDeviceFormat->nBlockAlign          = mDeviceFormat->nChannels * mDeviceFormat->wBitsPerSample / 8;
                mDeviceFormat->nAvgBytesPerSec      = mDeviceFormat->nBlockAlign * mDeviceFormat->nSamplesPerSec;
            }
            else 
            {
                wprintf(L"%s", L"Don't know how to coerce mix format to int-16");
                return false;
            }

            break;
        }
        default:
            wprintf(L"Don't know how to coerce WAVEFORMATEX with wFormatTag = 0x%08x to int-16", mDeviceFormat->wFormatTag);
            return false;
    }

    return SUCCEEDED(result);
}

bool AudioCapture::findDefaultAudioDevice()
{   
    IMMDeviceEnumerator *deviceEnumerator = nullptr;

    // activate a device enumerator
    HRESULT result = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&deviceEnumerator);
    if (FAILED(result)) 
    {
        wprintf(L"CoCreateInstance(IMMDeviceEnumerator) failed: hr = 0x%08x", result);
        return false;
    }

    // get the default render endpoint
    result = deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &mDefaultAudioDevice);
    if (FAILED(result))
    {
        wprintf(L"IMMDeviceEnumerator::GetDefaultAudioEndpoint failed: hr = 0x%08x", result);
    }

    deviceEnumerator->Release();

    return SUCCEEDED(result);
}

bool AudioCapture::initAudioClient()
{
    
    HRESULT result = mDefaultAudioDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&mAudioClient);

    if (FAILED(result)) 
    {
        wprintf(L"IMMDevice::Activate(IAudioClient) failed: hr = 0x%08x", result);
    }

    return SUCCEEDED(result);
}

bool AudioCapture::initCapture()
{
    HRESULT result = mAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK, 0, 0, mDeviceFormat, 0);

    if (FAILED(result)) 
    {
        wprintf(L"IAudioClient::Initialize failed: hr = 0x%08x", result);
        return false;
    }

    // activate an IAudioCaptureClient
    result = mAudioClient->GetService(__uuidof(IAudioCaptureClient), (void**)&mAudioCaptureClient);

    if (FAILED(result))
    {
        wprintf(L"IAudioClient::GetService(IAudioCaptureClient) failed: hr = 0x%08x", result);
    }

    return SUCCEEDED(result);
}

void AudioCapture::run()
{
    if (findDefaultAudioDevice() && initAudioClient() && getDeviceFormat() && initCapture())
    {
        prepareFile();
    }
    
}