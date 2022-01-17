#include "ntddk.h"

#define DELAY_ONE_MICROSEC (-10)
#define DELAY_ONE_MILLISEC (DELAY_ONE_MICROSEC*1000)

UNICODE_STRING SymbolicName = RTL_CONSTANT_STRING(L"\\??\\MySymbolicDriver");
ULONG PendingCounter = 0;

typedef struct _KEYBOARD_INPUT_DATA {
	USHORT UnitId;
	USHORT MakeCode;
	USHORT Flags;
	USHORT Reserved;
	ULONG  ExtraInformation;
} KEYBOARD_INPUT_DATA, * PKEYBOARD_INPUT_DATA;

typedef struct {
	PDEVICE_OBJECT lowerDevice;
}DeviceExtension, *PDEVICE_Extention;

VOID Unload(IN PDRIVER_OBJECT DriverObject)
{
	//IoDeleteSymbolicLink(&SymbolicName);
	PDEVICE_OBJECT KeyboardDevice = ((PDEVICE_Extention)DriverObject->DeviceObject->DeviceExtension)->lowerDevice;
	IoDetachDevice(KeyboardDevice);

	while (PendingCounter)
	{

	}
	IoDeleteDevice(DriverObject->DeviceObject);
	DbgPrint("driver unload \r\n");

}



NTSTATUS HandleRequest(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{


	IoCopyCurrentIrpStackLocationToNext(Irp);

	/*LARGE_INTEGER due = { 0 };
	due.QuadPart = 5000 * DELAY_ONE_MILLISEC;
	KeDelayExecutionThread(KernelMode, FALSE, &due);*/

	

	return IoCallDriver(((PDEVICE_Extention)DeviceObject->DeviceExtension)->lowerDevice,Irp);
}

NTSTATUS AttachDevice(PDRIVER_OBJECT DriverObject)
{
	NTSTATUS status;
	PDEVICE_OBJECT DeviceObject = NULL;

	//create Device Object of keyboard
	status = IoCreateDevice(DriverObject, sizeof(DeviceExtension), NULL, FILE_DEVICE_KEYBOARD, FILE_DEVICE_SECURE_OPEN, FALSE, &DeviceObject);
	if (!NT_SUCCESS(status))
	{
		KdPrint(("Failed to create Device\r\n"));
		return status;
	}

	UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(L"\\Device\\KeyboardClass0");
	DeviceObject->Flags |= DO_BUFFERED_IO;
	DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

	//Set the memory we allcoate to 0
	RtlZeroMemory(DeviceObject->DeviceExtension, sizeof(DeviceExtension));

	//attach the keyboardDevice to the Device we created
	
	status = IoAttachDevice(DeviceObject, &DeviceName, &((PDEVICE_Extention)DeviceObject->DeviceExtension)->lowerDevice);
	if (!NT_SUCCESS(status))
	{
		KdPrint(("Failed to attach device\r\n"));
		IoDeleteDevice(DeviceObject);
		return status;
	}
	return STATUS_SUCCESS;
}


NTSTATUS ReadComplete(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID Context)
{
	CHAR* keyflag[4] = { "KeyDown", "KeyUp", "E0", "E1" };
	PKEYBOARD_INPUT_DATA Keys = (PKEYBOARD_INPUT_DATA)Irp->AssociatedIrp.SystemBuffer;
	int structnum = Irp->IoStatus.Information / sizeof(KEYBOARD_INPUT_DATA);
	if (Irp->IoStatus.Status == STATUS_SUCCESS)
	{
		for (int i = 0; i < structnum; i++)
		{
			Keys[i].MakeCode = 0x24;
			KdPrint(("the scan code is %x (%s)\n", Keys[i].MakeCode, keyflag[(Keys[i]).Flags]));
		}
	}
	if (Irp->PendingReturned)
	{
		IoMarkIrpPending(Irp);
	}
	PendingCounter--;
	return Irp->IoStatus.Status;
}

NTSTATUS ReadRequest(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{


	IoCopyCurrentIrpStackLocationToNext(Irp);

	IoSetCompletionRoutine(Irp, ReadComplete, NULL, TRUE, FALSE, FALSE);
	PendingCounter++;
	return IoCallDriver(((PDEVICE_Extention)DeviceObject->DeviceExtension)->lowerDevice, Irp);
}


NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath)
{
	NTSTATUS status;
	KdPrint(("Driver start\r\n"));
	DriverObject->DriverUnload = Unload;



	

	/*status = IoCreateSymbolicLink(&SymbolicName, &DeviceName);
	if (!NT_SUCCESS(status))
	{
		KdPrint(("Failed to create Symbol\r\n"));
		IoDeleteDevice(DeviceObject);
		return status;
	}


	KdPrint(("create Symbol\r\n"));*/

	AttachDevice(DriverObject);
	
	
	

	for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
	{
		DriverObject->MajorFunction[i] = HandleRequest;
	}

	DriverObject->MajorFunction[IRP_MJ_READ] = ReadRequest;

	return STATUS_SUCCESS;
}
