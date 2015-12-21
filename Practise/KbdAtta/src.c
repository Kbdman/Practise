#include <ntddk.h>
#define KBDDRV_NAME L"\\Driver\\Kbdclass"
DRIVER_INITIALIZE DriverEntry;
extern POBJECT_TYPE* IoDriverObjectType;
NTSTATUS  ObReferenceObjectByName(PUNICODE_STRING Name, ULONG attributes, PACCESS_STATE param3, ACCESS_MASK p4, POBJECT_TYPE objtype, MODE mode, PVOID, PVOID* Pobj);
_Use_decl_annotations_
NTSTATUS mIoCompletion(
	_In_      PDEVICE_OBJECT DeviceObject,
	_In_      PIRP Irp,
	_In_opt_  PVOID Context
	)
{
	KdPrint(("this is complet\n\r"));

	PVOID p_buf = Irp->AssociatedIrp.SystemBuffer;
	PIO_STACK_LOCATION irpsp = IoGetCurrentIrpStackLocation(Irp);
	if (irpsp->MajorFunction == IRP_MJ_READ)
	{
		ULONG length=Irp->IoStatus.Information;
		KdPrint(("CMPLETE   READ IRP：%d \r\n", length));
		for (ULONG i = 0; i < length; i++)
		{
			KdPrint(("%2x", ((char*)p_buf)[i]));
		}
		KdPrint(("\r\n"));
	}
	if (Irp->PendingReturned)
	{
		IoMarkIrpPending(Irp);
	}

	return  Irp->IoStatus.Status;
	//如果IoCompletion routine determines that additional processing is required for the IRP
	//it must return STATUS_MORE_PROCESSING_REQUIRED. For more information, see the following Remarks section. 
	//Otherwise, it should return STATUS_SUCCESS. (The I/O manager only checks for the presence or absence of STATUS_MORE_PROCESSING_REQUIRED.)
}


NTSTATUS dispachFunc(IN PDEVICE_OBJECT dev, IN PIRP irp)
{
	KdPrint(("Start DISPATCH\r\n"));
	PIO_STACK_LOCATION irpsp = IoGetCurrentIrpStackLocation(irp);
	if (irpsp->MajorFunction == IRP_MJ_POWER)
	{
		KdPrint(("POW\r\n"));
		PoStartNextPowerIrp(irp);
		IoSkipCurrentIrpStackLocation(irp);
		return PoCallDriver(*(PDEVICE_OBJECT*)(dev->DeviceExtension), irp);
	}
	if (irpsp->MajorFunction == IRP_MJ_WRITE)
	{

		PVOID p_buf = irp->AssociatedIrp.SystemBuffer;
		if (dev->Flags & DO_DIRECT_IO)
		{
			KdPrint((" ALC MDL\r\n"));
			p_buf = MmGetSystemAddressForMdlSafe(irp->MdlAddress, HighPagePriority);
		}
		ULONG length = irpsp->Parameters.Write.Length;
		KdPrint(("WRITE IRP：%d \r\n", length));
		for (ULONG i = 0; i < length; i++)
		{
			KdPrint(("%c", ((char*)p_buf)[i]));
		}
		KdPrint(("\r\n"));
	}
	else if (irpsp->MajorFunction == IRP_MJ_READ)
	{
		ULONG length = irpsp->Parameters.Read.Length;
		KdPrint(("READ IRP %d\r\n", length));
		IoCopyCurrentIrpStackLocationToNext(irp);
		IoSetCompletionRoutine(irp, mIoCompletion, NULL, TRUE, TRUE, TRUE);
		return IoCallDriver(*(PDEVICE_OBJECT*)(dev->DeviceExtension), irp);
	}
	IoSkipCurrentIrpStackLocation(irp);
	PDEVICE_OBJECT lowerdev = *(PDEVICE_OBJECT*)(dev->DeviceExtension);
	return IoCallDriver(lowerdev, irp);
}
DRIVER_UNLOAD unloadFunc;

VOID unloadFunc(
	_In_  struct _DRIVER_OBJECT *DriverObject
	)
{
	KdPrint(("MD unload\n"));
}
_Use_decl_annotations_
NTSTATUS DriverEntry(
_In_ struct _DRIVER_OBJECT *DriverObject,
_In_ PUNICODE_STRING       RegistryPath
)
{
	USHORT i = 0;
	DriverObject->DriverUnload = unloadFunc;
	for (; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
	{
		DriverObject->MajorFunction[i] = dispachFunc;
	}
	NTSTATUS status;
	UNICODE_STRING KbdDriverName = { 0 };
	RtlInitUnicodeString(&KbdDriverName, KBDDRV_NAME);
	PDRIVER_OBJECT kbd_DrvObj;
	//通过名字获取驱动对象
	status = ObReferenceObjectByName(&KbdDriverName, OBJ_CASE_INSENSITIVE, NULL, 0, *IoDriverObjectType, KernelMode, NULL, (PVOID*)&kbd_DrvObj);
	if (!NT_SUCCESS(status))
	{
		KdPrint(("Find kbdclass Faile\n"));
	}
	else
	{
		ObDereferenceObject(DriverObject);
	}
	PDEVICE_OBJECT pObject = kbd_DrvObj->DeviceObject;
	int d = 0;
	while (pObject != NULL)
	{
		PDEVICE_OBJECT atachObj;
		PDEVICE_OBJECT filterObj;

		status = IoCreateDevice(DriverObject, sizeof(PDEVICE_OBJECT), NULL, pObject->Type, 0, FALSE, &filterObj);//创建的对象，通过最后参数返回对象地址
		if (!NT_SUCCESS(status))
		{
			KdPrint(("Create Filter Dev Faile\n"));
			return STATUS_FAILED_DRIVER_ENTRY;
		}
		filterObj->Flags |= pObject->Flags&DO_DIRECT_IO;

		filterObj->Flags |= pObject->Flags&DO_BUFFERED_IO;
		if (filterObj->Flags & DO_DIRECT_IO)
			KdPrint(("DO_DRIECTIO\n"));
		if (filterObj->Flags & DO_BUFFERED_IO)
			KdPrint(("DO_BUFFERED\n"));
		atachObj = IoAttachDeviceToDeviceStack(filterObj, pObject);
		if (atachObj != NULL)
		{
			*((PDEVICE_OBJECT*)(filterObj->DeviceExtension)) = atachObj;
			KdPrint(("bind sucess\n\r"));
			d++;
		}
		else
		{
			KdPrint(("bind fail\n\r"));
		}
		pObject = pObject->NextDevice;

	}
	KdPrint(("bind sucess=%d\n\r", d));
	return STATUS_SUCCESS;
}