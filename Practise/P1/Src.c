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
	PIO_STACK_LOCATION  pStackLoc = IoGetCurrentIrpStackLocation(Irp); //根据irp指针获取IRP在IRP栈上的位置，从该位置可以访问IRP的参数
	PVOID pData = Irp->AssociatedIrp.SystemBuffer;
	ULONG size = pStackLoc->Parameters.Write.Length; //不管缓冲区是userbuffer还是还是SystemBuffer，该值都是写入数据的长度
	if (pData == NULL&&Irp->MdlAddress!=NULL)// 当io类型是buffered I/O时使用AssociatedIrp.SystemBuffer存储缓冲大小
	{
		KdPrint(("this is a  DirectIo\n")); 
		//如果是Direct I/O那么 用户写的数据写在由MdlAddress秒速的一块内存上
		pData = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, HighPagePriority); //获得MDL对象指向的缓冲区在系统空间中的虚拟地址？意义不明
		size = MmGetMdlByteCount(Irp->MdlAddress);

	}
	if (pData==NULL)
	{
		pData = Irp->UserBuffer;
	}
	KdPrint(("Get a Write Irp to Dev %p ,size:%d\n context=%s\n", DeviceObject,size,pData ));
	
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