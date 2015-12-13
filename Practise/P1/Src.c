#include <ntddk.h>
#include <ntstrsafe.h>
#include <wdf.h>
#define SYMBLICOBJ L"\\??\\LH_TESTOBJ" //�豸���ƺͷ������ƶ���һ���Ĺ淶,

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
	return STATUS_SUCCESS;  //CreateDispatchֻ���ڷ���STATUS_SUCCESS��ʱ��  ������Ż���ȷ���
}

NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT  DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
	NTSTATUS status=STATUS_SUCCESS;
	KdPrint(("Module Practise1 is loaded\n"));
	KdPrint(("DirvierName=%ws\nRegistryPath=%ws\n", DriverObject->DriverName,RegistryPath));
	DriverObject->DriverUnload = Unload;
	DriverObject->MajorFunction[IRP_MJ_WRITE] = P1DispatchWrite;  //Ϊ�������豸����Ķ�����д����ַ�������
	DriverObject->MajorFunction[IRP_MJ_CREATE] = DispatchCreate;  //IRP_MJ_CREATE  ��Ӧ�ò�������CreateFile���豸����ʱ����ø÷ַ����������CreateFile���������1
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
	status =IoCreateSymbolicLink(&SymbolicName, &DriverName); //�����������ӣ�Ӧ�ò���򲻿���ͨ���豸��ֱ�����豸���н�������Ҫͨ���������������豸���󽻻���
	if (!NT_SUCCESS(status))
	{
		KdPrint(("Create S Failed\n"));
		IoDeleteDevice(pobj);
		return status;
	}
	return status;
}