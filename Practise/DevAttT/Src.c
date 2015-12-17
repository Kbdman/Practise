/*
����һ���������豸���ӵ�һ�������ϣ���ȡһЩ�Դ��ڵķ�����Ϣ
*/
#include <ntddk.h>
#include <ntstrsafe.h>
#include <wdf.h>
#define MEM_TAG 'MYTG'
struct attachPair* pPairs = NULL;
struct attachPair** pLastPtr = &pPairs;
//���ڽ��ڴ���0��С��������̫ȷ��memset�Ƿ���ã���̫�����c�⺯��
void memzero(char* addr, int size)
{
	int i = 0;
	for (; i < size; i++)
		addr[i] = '\0';
}
//ʵ�ֵ�һ��С�����ڱ�������Ӧ��ûʲô����
 struct attachPair
{
	PDEVICE_OBJECT Obj;
	PDEVICE_OBJECT AttachedObj;
	struct attachPair *next;
};

 PDEVICE_OBJECT getAttachedObjFromList(PDEVICE_OBJECT pObj)
 {
	 struct attachPair* pRead = pPairs;
	 while (pRead != NULL)
	 {
		 if (pRead->Obj == pObj)
			 return pRead->AttachedObj;
	 }
	 return NULL;
 }
 NTSTATUS allocAPairAndConnect(struct attachPair** pPtr)
 {
	 /*
	  ϵͳ�Ὣtag��������ڴ���������һЩ���߱���WINDBG���Ὣ������Ļ��������Ӧ��tag��������ʾ��һ��
	 */
	 struct attachPair* pPair= ExAllocatePoolWithTag(PagedPool,sizeof(struct attachPair),MEM_TAG);
	 if (pPair == NULL)
		 return STATUS_INSUFFICIENT_RESOURCES;
	 memzero((char*)pPair,sizeof(struct attachPair));
	 *pPtr = pPair;
	 return STATUS_SUCCESS;
 }

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
NTSTATUS AttDispatchWrite(
	_Inout_  struct _DEVICE_OBJECT *DeviceObject,
	_Inout_  struct _IRP *Irp
	)
{
	KdPrint(("In AttDispatchWrite\n"));
	PDEVICE_OBJECT pObj = getAttachedObjFromList(DeviceObject);
	if (pObj == NULL)
		return STATUS_SUCCESS;
	IoSkipCurrentIrpStackLocation(Irp);
	return IoCallDriver(pObj,Irp);

}

NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT  DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
	DriverObject->MajorFunction[IRP_MJ_WRITE] = AttDispatchWrite; //�趨д����ķַ�����
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

		IoDeleteDevice(pObj);
		KdPrint(("attach Failed!"));
		return status;
	}
	status= allocAPairAndConnect(&pPairs);
	if (NT_SUCCESS(status))
	{
		IoDeleteDevice(pObj);
		return status;
	}
	(*pLastPtr)->Obj = pObj;
	(*pLastPtr)->AttachedObj = pAttachedObj;
	pLastPtr = &(((*pLastPtr))->next);
	KdPrint(("attach SS!"));
	return status;
}