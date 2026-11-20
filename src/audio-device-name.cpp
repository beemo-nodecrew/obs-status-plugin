#include "audio-device-name.hpp"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <objbase.h>
#include <initguid.h>
#include <mmdeviceapi.h>
#include <functiondiscoverykeys_devpkey.h>

QString default_audio_device_name(bool input)
{
	IMMDeviceEnumerator *enumerator = nullptr;
	HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
				      reinterpret_cast<void **>(&enumerator));
	if (FAILED(hr) || !enumerator)
		return QString();

	QString name;
	IMMDevice *device = nullptr;
	// OBS win-wasapi와 같은 역할: 입력은 통신 기본 장치, 출력은 기본 장치
	hr = enumerator->GetDefaultAudioEndpoint(input ? eCapture : eRender, input ? eCommunications : eConsole,
						 &device);
	if (SUCCEEDED(hr) && device) {
		IPropertyStore *store = nullptr;
		if (SUCCEEDED(device->OpenPropertyStore(STGM_READ, &store)) && store) {
			PROPVARIANT value;
			PropVariantInit(&value);
			if (SUCCEEDED(store->GetValue(PKEY_Device_FriendlyName, &value)) && value.vt == VT_LPWSTR &&
			    value.pwszVal)
				name = QString::fromWCharArray(value.pwszVal);
			PropVariantClear(&value);
			store->Release();
		}
		device->Release();
	}
	enumerator->Release();
	return name;
}

#else

QString default_audio_device_name(bool)
{
	return QString();
}

#endif
