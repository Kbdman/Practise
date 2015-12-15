#include <ntddk.h>
#include <ntstrsafe.h>
#include <wdf.h>
NTSTATUS attachDev(_In_ PDRIVER_OBJECT DriObj ,_In_ PDEVICE_OBJECT DevObj, _Out_ PDEVICE_OBJECT *fltObj,_Out_ PDEVICE_OBJECT *attachedObj)
{
	NTSTATUS status = STATUS_SUCCESS;

	status = IoCreateDevice(DriObj, 0, NULL, DevObj->DeviceType //Ҫ�뱻attach���豸��������һ��
		, 0, 0, fltObj);
	if (!NT_SUCCESS(status))
		return status;
	if (DevObj->Flags&DO_BUFFERED_IO)														 // 
		(*fltObj)->Flags |= DO_BUFFERED_IO;											 // 
	if (DevObj->Flags&DO_DIRECT_IO)															 // 
		(*fltObj)->Flags |= DO_DIRECT_IO;												 //  ����һ����Ŀ���豸����һ�µ��豸����
	if (DevObj->Characteristics&FILE_DEVICE_SECURE_OPEN)									 //   ����Ϊʲô����ȫ�������ԣ���ֻ���ò���һ�������ԣ������
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
	RtlInitUnicodeString(&str,L"\\Device\\00000069");//����豸������������еĴ��ڣ������Ӳ���豸���Դ��豸�������в鵽��Ҳ���Բ鵽�豸��������ջ
	PFILE_OBJECT pFobj;
	PDEVICE_OBJECT pDevObj;
	status=IoGetDeviceObjectPointer(&str,FILE_ALL_ACCESS,&pFobj,&pDevObj);
	if (!NT_SUCCESS(status))
	{
		KdPrint(("Get Point Failed!\n"));
		KdBreakPoint();
		return status;
	}
		
	ObDereferenceObject(pFobj);//��֪���ļ�������ʲô�ã���������˵��Ҫ�ͷŸö��󣬲�Ȼ������豸
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