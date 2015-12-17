/*
创建一个过滤器设备附加到一个串口上，截取一些对串口的访问信息
*/
#include <ntddk.h>
#include <ntstrsafe.h>
#include <wdf.h>
#define MEM_TAG 'MYTG'
struct attachPair* pPairs = NULL;
struct attachPair** pLastPtr = &pPairs;
//用于将内存置0的小函数，不太确定memset是否可用，不太想调用c库函数
void memzero(char* addr, int size)
{
	int i = 0;
	for (; i < size; i++)
		addr[i] = '\0';
}
//实现的一个小链表，在本例子里应该没什么卵用
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
	  系统会将tag与申请的内存做关联，一些工具比如WINDBG，会将与申请的缓冲区与对应的tag关联的显示在一起。
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
	DriverObject->MajorFunction[IRP_MJ_WRITE] = AttDispatchWrite; //设定写请求的分发函数
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