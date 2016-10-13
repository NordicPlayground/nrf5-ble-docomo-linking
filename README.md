nrf5-ble-docomo-linking
=======================

 This repository contains code examples that show how to implement DoCoMo Linking BLE device on nRF5. For more information about DoCoMo Linking IoT project, please visit https://linkingiot.com/en/.

Linking Spec versions
---------------------
- Linking Advertise Information Format Specification, v2.0.2, 2016-08-08

- PeripheralDeviceLinkingProfile on BLE Specification, v2.0, 2016-07-25
- PeripheralDevicePropertyInformationService on BLE Specification, v2.0, 2016-07-25
- PeripheralDeviceOperationService on BLE Specification, v2.0, 2016-07-25
- PeripheralDeviceNotificationService on BLE Specification, v2.0, 2016-07-25
- PeripheralDeviceSettingOperationService on BLE Specification, v2.0, 2016-07-25
- PeripheralDeviceSensorInformationService on BLE Specification, v2.0, 2016-07-25

- Device Setting Information Format Specification, v2.0, 2016-07-25
- Operation sequence with periphral devices in use cases, v1.3, 2016-02-17

Requirements
------------
The following toolchains/devices have been used for testing and verification:
- ARM: MDK-ARM version 5.18a
- GCC: GCC ARM Embedded 4.9 2015q3
- nRF5 SDK version 11.0.0
- nRF52 PCA10040 with S132 v2.0.1
- nRF51 PCA10028 with S130 v2.0.1

To compile it, clone the repository into the \nRF5_SDK_11.0.0_89a8197 folder. 

If you download the zip, please copy \components\ble\ble_services\experimental_ble_pdlp to "\nRF5_SDK_11.0.0_89a8197\components\ble\ble_services", and \examples\ble_peripheral\<all> to "\nRF5_SDK_11.0.0_89a8197\examples\ble_peripheral".

Documentation
-------------
References:
- For the NTT DoCoMo Linking project device specification, please refer to https://linkingiot.com/en/developer/.
- For details about the sample implementation of the Peripheral Device Link Profile (PDLP), please refer to the header file of ble_pdlp.h.


About the sample application projects, 
- experimental_ble_app_linking_button demonstrates the PDLP profile, Property Information Service (PIS) and Operation Service (OS). Tested with DoCoMo LinkIFDemo.apk in Android SDK.
- experimental_ble_app_linking_led demonstrates the PDLP profile, Property Information Service (PIS), Notification Service (NS) and Setting Operation Service (SOS). Tested with "天気予報通知" app in Google Play.
- experimental_ble_app_linking_temp demonstrates the PDLP profile, Property Information Service (PIS), Sensor Information Service (SIS). Tested with "温湿度アプリ" app in Google Play.
- experimental_ble_app_linking_beacon demonstrates Linking beacon format. The implementation is based on Nordic Solar Beacon project (https://github.com/NordicSemiconductor/solar_sensor_beacon), which does not use softdevice and requires no SDK. Tested with DoCoMo LinkIFDemo.apk in Android SDK.

NOTE the implementation is solely for demo purpose, so it is NOT optimized and does NOT have product quality. The sample project is NOT fully tested, due to lack of test applications on Smart Phones. The test is done with DoCoMo Linking app on Android.
The "LinkingIFDemo.apk" app can be found in Linking Android SDK, available on https://linkingiot.com/en/developer/

About this project
------------------
This application is one of several applications that has been built by the support team at Nordic Semiconductor, as a demo of some particular feature or use case. It has not necessarily been thoroughly tested, so there might be unknown issues. It is hence provided as-is, without any warranty. 

However, in the hope that it still may be useful also for others than the ones we initially wrote it for, we've chosen to distribute it here on GitHub. 

The application is built to be used with the official nRF5 SDK, that can be downloaded from http://developer.nordicsemi.com/
