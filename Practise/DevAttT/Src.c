#include <ntddk.h>
#include <ntstrsafe.h>
#include <wdf.h>
NTSTATUS attachDev(_In_ PDRIVER_OBJECT DriObj ,_In_ PDEVICE_OBJECT DevObj, _Out_ PDEVICE_OBJECT *fltObj,_Out_ PDEVICE_OBJECT *attachedObj)
{
	NTSTATUS status = STATUS_SUCCESS;

	status = IoCreateDevice(DriObj, 0, NULL, DevObj->DeviceType //要与被attach的设备对象类型一致
		, 0, 0, fltObj);
	if (!NT_SUCCESS(status))
		return status;
	if (DevObj->Flags&DO_BUFFERED_IO)														 // 
		(*fltObj)->Flags |= DO_BUFFERED_IO;											 // 
	if (DevObj->Flags&DO_DIRECT_IO)															 // 
		(*fltObj)->Flags |= DO_DIRECT_IO;												 //  创建一个与目标设备属性一致的设备对象
	if (DevObj->Characteristics&FILE_DEVICE_SECURE_OPEN)									 //   至于为什么不完全复制属性，而只设置部分一样的属性，不清楚
		(*fltObj)->Characteristics |= FILE_DEVICE_SECURE_OPEN;							 // 
	(*fltObj)->Flags |= DO_POWER_PAGABLE;												 // 
	status = IoAttachDeviceToDeviceStackSafe(*fltObj, DevObj, attachedObj);
	if (!NT_SUCCESS(status))
	{
		IoDeleteDevice(*fltObj);
		return status;
	}
	return status;
}
NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT  DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
	NTSTATUS status = STATUS_SUCCESS;
	KdPrint(("Module DevAtt is loaded\n"));
	KdPrint(("DirvierName=%ws\nRegistryPath=%ws\n", DriverObject->DriverName, RegistryPath));
	UNICODE_STRING str;
	RtlInitUnicodeString(&str,L"\\Device\\00000069");//这个设备名是我虚拟机中的串口，如果是硬件设备可以从设备管理器中查到，也可以查到设备的驱动堆栈
	PFILE_OBJECT pFobj;
	PDEVICE_OBJECT pDevObj;
	status=IoGetDeviceObjectPointer(&str,FILE_ALL_ACCESS,&pFobj,&pDevObj);
	if (!NT_SUCCESS(status))
	{
		KdPrint(("Get Point Failed!\n"));
		KdBreakPoint();
		return status;
	}
		
	ObDereferenceObject(pFobj);//不知道文件对象有什么用，但是书上说需要释放该对象，不然会造成设备
	PDEVICE_OBJECT pObj;
	PDEVICE_OBJECT pAttachedObj;
	status = attachDev(DriverObject, pDevObj, &pObj, &pAttachedObj);
	if (!NT_SUCCESS(status))
	{
		KdPrint(("attach Failed!"));
		return status;
	}
	KdPrint(("attach SS!"));
	return status;
}