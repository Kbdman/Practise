#include <ntddk.h>
#include <ntstrsafe.h>
#include <wdf.h>
#define SYMBLICOBJ L"\\??\\LH_TESTOBJ" //设备名称和符号名称都有一定的规范,

PDEVICE_OBJECT pobj = NULL;
VOID Unload(
	_In_  struct _DRIVER_OBJECT *DriverObject
	)
{
	if (pobj != NULL)
		IoDeleteDevice(pobj);
	UNICODE_STRING SymbolicName;
	RtlInitUnicodeString(&SymbolicName, SYMBLICOBJ);
	IoDeleteSymbolicLink(&SymbolicName);
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

NTSTATUS DispatchCreate(
	_Inout_  struct _DEVICE_OBJECT *DeviceObject,
	_Inout_  struct _IRP *Irp
	)
{
	KdPrint(("get Create IRP size %u to %u",Irp->Size,DeviceObject->Size));
	return STATUS_SUCCESS;  //CreateDispatch只有在返回STATUS_SUCCESS的时候  打开请求才会正确完成
}

NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT  DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
	NTSTATUS status=STATUS_SUCCESS;
	KdPrint(("Module Practise1 is loaded\n"));
	KdPrint(("DirvierName=%ws\nRegistryPath=%ws\n", DriverObject->DriverName,RegistryPath));
	DriverObject->DriverUnload = Unload;
	DriverObject->MajorFunction[IRP_MJ_WRITE] = P1DispatchWrite;  //为驱动内设备对象的读请求写请求分发函数。
	DriverObject->MajorFunction[IRP_MJ_CREATE] = DispatchCreate;  //IRP_MJ_CREATE  当应用层对象调用CreateFile打开设备对象时会调用该分发函数，如果CreateFile会产生错误1
	UNICODE_STRING DriverName;
	RtlInitUnicodeString(&DriverName, L"\\Device\\KernelObj_LH");

	status=IoCreateDevice(DriverObject, 0, &DriverName, FILE_DEVICE_UNKNOWN,FILE_DEVICE_SECURE_OPEN, FALSE, &pobj);
	if (!NT_SUCCESS(status))
	{
		KdPrint(("Create Device Failed\n"));
		return status;
	}
	UNICODE_STRING SymbolicName;
	RtlInitUnicodeString(&SymbolicName,SYMBLICOBJ);
	status =IoCreateSymbolicLink(&SymbolicName, &DriverName); //创建符号链接，应用层程序不可以通过设备名直接与设备进行交互，需要通过符号链接来与设备对象交互。
	if (!NT_SUCCESS(status))
	{
		KdPrint(("Create S Failed\n"));
		IoDeleteDevice(pobj);
		return status;
	}
	return status;
}