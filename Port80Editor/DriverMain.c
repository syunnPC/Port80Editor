#include <ntddk.h>
#include <wdf.h>
#include "../Public.h"

#define DEVICE_NAME L"\\Device\\Port80"
#define SYMLINK_NAME L"\\DosDevices\\Port80"
#define PORT80_SDDL L"D:P(A;;GA;;;SY)(A;;GA;;;BA)"

#ifndef RETURN_IF_FAILED
#define RETURN_IF_FAILED(status) if(!NT_SUCCESS(status)){return status;}
#endif

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD Port80EvtDeviceAdd;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL Port80EvtIoDeviceControl;

static VOID WritePort80(_In_ UCHAR v)
{
	WRITE_PORT_UCHAR((PUCHAR)(ULONG_PTR)0x80, v);
}

NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
	WDF_DRIVER_CONFIG config;
	NTSTATUS status;

	WDF_DRIVER_CONFIG_INIT(&config, Port80EvtDeviceAdd);
	status = WdfDriverCreate(DriverObject, RegistryPath, WDF_NO_OBJECT_ATTRIBUTES, &config, WDF_NO_HANDLE);

	return status;
}

NTSTATUS Port80EvtDeviceAdd(_In_ WDFDRIVER Driver, _Inout_ PWDFDEVICE_INIT DeviceInit)
{
	UNREFERENCED_PARAMETER(Driver);

	NTSTATUS status;
	WDFDEVICE device;
	WDF_IO_QUEUE_CONFIG queueConfig;
	WDFQUEUE queue;
	UNICODE_STRING devName, symLink, sddl;

	//WdfDeviceInitSetSynchronizationScope(DeviceInit, WdfSynchronizationScopeNone);
	//WdfDeviceInitSetExecutionLevel(DeviceInit, WdfExecutionLevelPassive);

	RtlInitUnicodeString(&devName, DEVICE_NAME);
	status = WdfDeviceInitAssignName(DeviceInit, &devName);

	RETURN_IF_FAILED(status);

	RtlInitUnicodeString(&sddl, PORT80_SDDL);

	status = WdfDeviceInitAssignSDDLString(DeviceInit, &sddl);

	RETURN_IF_FAILED(status);

	status = WdfDeviceCreate(&DeviceInit, WDF_NO_OBJECT_ATTRIBUTES, &device);

	RETURN_IF_FAILED(status);

	RtlInitUnicodeString(&symLink, SYMLINK_NAME);
	
	status = WdfDeviceCreateSymbolicLink(device, &symLink);

	RETURN_IF_FAILED(status);

	WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queueConfig, WdfIoQueueDispatchSequential);
	queueConfig.EvtIoDeviceControl = Port80EvtIoDeviceControl;
	queueConfig.PowerManaged = WdfFalse;

	status = WdfIoQueueCreate(device, &queueConfig, WDF_NO_OBJECT_ATTRIBUTES, &queue);

	RETURN_IF_FAILED(status);

	return STATUS_SUCCESS;
}

VOID Port80EvtIoDeviceControl(_In_ WDFQUEUE Queue, _In_ WDFREQUEST Request, _In_ size_t OutputBufferLength, _In_ size_t InputBufferLength, _In_ ULONG IoControlCode)
{
	UNREFERENCED_PARAMETER(Queue);
	UNREFERENCED_PARAMETER(OutputBufferLength);

	NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;
	PVOID inBuf = NULL;
	size_t inLen = 0;

	if (IoControlCode == IOCTL_PORT80_WRITE_U8)
	{
		if (InputBufferLength != 1)
		{
			status = STATUS_INVALID_PARAMETER;
			WdfRequestComplete(Request, status);
			return;
		}

		status = WdfRequestRetrieveInputBuffer(Request, 1, &inBuf, &inLen);
		if (!NT_SUCCESS(status) || inLen < 1 || inBuf == NULL)
		{
			status = STATUS_INVALID_PARAMETER;
			WdfRequestComplete(Request, status);
			return;
		}

		UCHAR v = *(UCHAR*)inBuf;

		WritePort80(v);
		status = STATUS_SUCCESS;
	}

	WdfRequestComplete(Request, status);
}