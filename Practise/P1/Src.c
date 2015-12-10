#include <ntddk.h>
#include <ntstrsafe.h>
#include <wdf.h>

VOID Unload(
	_In_  struct _DRIVER_OBJECT *DriverObject
	)
{
	KdPrint(("Module Practise1 Name:%ws is unloaded!\n",DriverObject->DriverName));
}



DRIVER_DISPATCH P1DispatchWrite;

NTSTATUS P1DispatchWrite(
	_Inout_  struct _DEVICE_OBJECT *DeviceObject,
	_Inout_  struct _IRP *Irp
	)
{
	KdPrint(("Module Practise1 Name:%u is unloaded!\nIRP:%s\n", DeviceObject->Size,Irp));
	return STATUS_SUCCESS;
}




NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT  DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
	NTSTATUS status=STATUS_SUCCESS;
	KdPrint(("Module Practise1 is loaded\n"));
	KdPrint(("DirvierName=%ws\nRegistryPath=%ws\n", DriverObject->DriverName,RegistryPath));
	DriverObject->DriverUnload = Unload;
	DriverObject->MajorFunction[IRP_MJ_WRITE] = P1DispatchWrite;  //为驱动内设备对象的读请求写请求分发函数。
	return status;
}