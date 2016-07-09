/*
 *
 * Copyright 2016 Matt Wu <mattwu@163.com> under MIT license
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * PROJECT:
 *     Moure: Swap Mouse Left and Right Buttons
 *
 */

#include <moure.h>

/*
 * global defintions
 */

CHAR    g_drvVersion[] = "1.0";
CHAR    g_drvTime[]    = __TIME__;
CHAR    g_drvDate[]    = __DATE__;

struct mr_core  g_core;

/*
 * memory allocation wrapper
 */

PVOID MrAlloc(int bytes)
{
    PVOID ptr;
    ptr = ExAllocatePoolWithTag(NonPagedPool, bytes, 'PNRM');
    if (NULL == ptr) {
        return NULL;
    }
    memset(ptr, 0, bytes);
    return ptr;
}

VOID MrFree(PVOID ptr)
{
    ExFreePool(ptr);
}

/*
 * ERESOURCE wrapper routines
 */

BOOLEAN MrLockGlobal(BOOLEAN ex)
{
    KeEnterCriticalRegion();

    if (!ex)
        return ExAcquireResourceSharedLite(&g_core.mc_lock, TRUE);

    return ExAcquireResourceExclusiveLite(&g_core.mc_lock, TRUE);
}

VOID MrUnlockGlobal()
{
    ExReleaseResourceLite(&g_core.mc_lock);
    KeLeaveCriticalRegion();
}


PDEVICE_OBJECT MrCreateDevice(
    IN PDRIVER_OBJECT   drvobj,
    IN PWCHAR           devname,
    IN PWCHAR           symlink
    )
{
    UNICODE_STRING      dev_name;
    UNICODE_STRING      dos_name;
    PDEVICE_OBJECT      devobj;

    /* SDDL_DEVOBJ_SYS_ALL_ADM_RWX_WORLD_RWX_RES_RWX */
    UNICODE_STRING      sddl = RTL_CONSTANT_STRING
                           (L"D:P(A;;GA;;;SY)(A;;GRGWGX;;;BA)"
                            L"(A;;GRGWGX;;;WD)"
                            L"(A;;GRGWGX;;;RC)");
    NTSTATUS            status;

    /* create new device object with given name */
    RtlInitUnicodeString(&dev_name, devname);
    status = IoCreateDeviceSecure(drvobj, 0, &dev_name,
                                  FILE_DEVICE_UNKNOWN,
                                  0, FALSE, &sddl, NULL,
                                  &devobj);
    if (!NT_SUCCESS(status)) {
        return NULL;
    }

    /* create symlink, which is visible to user */
    RtlInitUnicodeString(&dos_name, symlink);
    status = IoCreateSymbolicLink(&dos_name, &dev_name);
    if (!NT_SUCCESS(status)) {
        IoDeleteDevice(devobj);
        return NULL;
    }
    devobj->Flags &= ~DO_DEVICE_INITIALIZING;

    return devobj;
}

VOID MrDestroyDevice(
    IN PDEVICE_OBJECT   devobj,
    IN PWCHAR           link
    )
{
    UNICODE_STRING  symlink;

    RtlInitUnicodeString(&symlink, link);
    IoDeleteSymbolicLink(&symlink);

    IoDeleteDevice(devobj);
}

NTSTATUS MrQueryCallback(
    IN PWSTR            ValueName,
    IN ULONG            ValueType,
    IN PVOID            ValueData,
    IN ULONG            ValueLength,
    IN PVOID            Context,
    IN PVOID            EntryContext
    )
{
    if (NULL == ValueName || NULL == ValueData || 0 == ValueLength)
        return STATUS_SUCCESS;

    if (ValueType == REG_MULTI_SZ &&
        wcslen(ValueName) == wcslen(MOURE_DEVLIST) &&
        !_wcsnicmp(ValueName, MOURE_DEVLIST, wcslen(MOURE_DEVLIST))) {

        if (1024 > ValueLength)
            g_core.mc_dev_bytes = 1024;
        else
            g_core.mc_dev_bytes = ValueLength + sizeof(WCHAR);
        g_core.mc_dev_list = MrAlloc(g_core.mc_dev_bytes);
        if (g_core.mc_dev_list)
            RtlCopyMemory(g_core.mc_dev_list, ValueData, ValueLength);
        else
            g_core.mc_dev_bytes = 0;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
MrQueryRegistry(IN PUNICODE_STRING RegistryPath)
{
    RTL_QUERY_REGISTRY_TABLE    QueryTable[2];
    int                         i = 0;
    NTSTATUS                    status;

    RtlZeroMemory(&QueryTable[0], sizeof(QueryTable));

    /*
     * 1 Device List (to be swapped)
     */
    QueryTable[i].Flags = RTL_QUERY_REGISTRY_NOEXPAND;
    QueryTable[i].Name = MOURE_DEVLIST;
    QueryTable[i].DefaultType = REG_MULTI_SZ;
    QueryTable[i].DefaultLength = 0;
    QueryTable[i].DefaultData = NULL;
    QueryTable[i].EntryContext = NULL;
    QueryTable[i].QueryRoutine = MrQueryCallback;
    i++;

    status = RtlQueryRegistryValues(
                 RTL_REGISTRY_ABSOLUTE,
                 RegistryPath->Buffer,
                 &QueryTable[0],
                 NULL,
                 NULL
            );


    /* failed to query registry settings */
    if (!NT_SUCCESS(status)) {
        if (!g_core.mc_dev_list) {
            g_core.mc_dev_bytes = 1024;
            g_core.mc_dev_list = MrAlloc(1024);
        }
        if (!g_core.mc_dev_list)
            g_core.mc_dev_bytes = 0;
    }

    return STATUS_SUCCESS;
}

VOID MrStopCore()
{
    ASSERT(IsListEmpty(&g_core.mc_dcb_list));

    if (g_core.mc_dev_list)
        MrFree(g_core.mc_dev_list);

    if (g_core.mc_reg.Buffer)
        MrFree(g_core.mc_reg.Buffer);

    /* stop and delete core deviceobj */
    if (g_core.mc_devobj)
        MrDestroyDevice(g_core.mc_devobj, MOURE_SYMLNK);

    /* delete resource lock */
    ExDeleteResourceLite(&g_core.mc_lock);
}

NTSTATUS MrStartCore(
    IN PDRIVER_OBJECT   drvobj,
    IN PUNICODE_STRING  regpath
    )
{
    NTSTATUS            status;

    __try {

        /* initialize core structure */
        ExInitializeResourceLite(&g_core.mc_lock);
        InitializeListHead(&g_core.mc_dcb_list);

        /* store registry path */
        g_core.mc_reg = *regpath;
        g_core.mc_reg.Buffer = MrAlloc(regpath->MaximumLength);
        if (g_core.mc_reg.Buffer)
            RtlCopyMemory(g_core.mc_reg.Buffer, regpath->Buffer,
                          regpath->MaximumLength);

        /* query resitry */
        status = MrQueryRegistry(regpath);
        if (!NT_SUCCESS(status)) {
            __leave;
        }

        /* create device object */
        g_core.mc_devobj = MrCreateDevice(drvobj,
                                          MOURE_DEVICE,
                                          MOURE_SYMLNK);
        if (NULL == g_core.mc_devobj) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            __leave;
        }

    } __finally {

        if (!NT_SUCCESS(status)) {
            /* do cleanup */
            MrStopCore();
        }
    }

    return status;
}

NTSTATUS MrPassthru(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp 
    )
{
    struct mr_dcb *dcb = DeviceObject->DeviceExtension;

    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(dcb->md_target, Irp);
}

NTSTATUS MrReadCompletion( 
    IN PDEVICE_OBJECT   DeviceObject, 
    IN PIRP             Irp,
    IN PVOID            Context 
    )
{
    struct mr_dcb      *dcb = Context;
    PIO_STACK_LOCATION  iosp;
    PMOUSE_INPUT_DATA   md;
    int                 n, i;

    __try {

        ASSERT(Context == DeviceObject->DeviceExtension);
        iosp = IoGetCurrentIrpStackLocation(Irp);

        if  (!NT_SUCCESS(Irp->IoStatus.Status)) {
            __leave;
        }

        if (0 == (dcb->md_state & MR_DEV_STATE_ENABLED)) {
            __leave;
        }

        md = Irp->AssociatedIrp.SystemBuffer;
        n = (int)(Irp->IoStatus.Information/sizeof(MOUSE_INPUT_DATA));

        /* swap left and right buttons */
        for( i = 0; i < n; i++ ) {
            if (md[i].Buttons & MOUSE_RIGHT_BUTTON_DOWN) {
                md[i].Buttons &= ~MOUSE_RIGHT_BUTTON_DOWN;
                md[i].Buttons |= MOUSE_LEFT_BUTTON_DOWN;
            } else if (md[i].Buttons & MOUSE_LEFT_BUTTON_DOWN) {
                md[i].Buttons &= ~MOUSE_LEFT_BUTTON_DOWN;
                md[i].Buttons |= MOUSE_RIGHT_BUTTON_DOWN;
            } else if (md[i].Buttons & MOUSE_RIGHT_BUTTON_UP) {
                md[i].Buttons &= ~MOUSE_RIGHT_BUTTON_UP;
                md[i].Buttons |= MOUSE_LEFT_BUTTON_UP;
            } else if (md[i].Buttons & MOUSE_LEFT_BUTTON_UP) {
                md[i].Buttons &= ~MOUSE_LEFT_BUTTON_UP;
                md[i].Buttons |= MOUSE_RIGHT_BUTTON_UP;
            }
        }

    } __finally {

        if (Irp->PendingReturned) {
            IoMarkIrpPending(Irp);
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS MrRead( 
    IN PDEVICE_OBJECT   DeviceObject, 
    IN PIRP             Irp
    )
{
    struct mr_dcb *dcb = DeviceObject->DeviceExtension;

    IoCopyCurrentIrpStackLocationToNext(Irp);
    IoSetCompletionRoutine(Irp, MrReadCompletion,
                           dcb, TRUE, TRUE, TRUE);

    return IoCallDriver(dcb->md_target, Irp);
}

NTSTATUS MrQueryString(
    IN PDEVICE_OBJECT           d,
    IN DEVICE_REGISTRY_PROPERTY c,
    IN OUT PUNICODE_STRING      n
    )
{
    PWCHAR data = NULL;
    ULONG len = 0;
    NTSTATUS status;

    data = MrAlloc(1024);
    if (NULL == data) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto errorout;
    }

    RtlZeroMemory(n, sizeof(UNICODE_STRING));
    status = IoGetDeviceProperty(d,
                                 c,
                                 1024,
                                 data,
                                 &len);

    if (!NT_SUCCESS(status)) {
        goto errorout;
    }

    len = wcslen(data) * sizeof(WCHAR);
    if (0 == len) {
        goto errorout;
    }
    n->MaximumLength = (USHORT)len + sizeof(WCHAR);
    n->Buffer = MrAlloc(n->MaximumLength);
    if (NULL == n->Buffer) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto errorout;
    }
    RtlCopyMemory(n->Buffer, data, len);
    n->Length = (USHORT)len;

errorout:

    if (data)
        MrFree(data);

    return status;
}

NTSTATUS MrQueryBinary(
    IN PDEVICE_OBJECT           d,
    IN DEVICE_REGISTRY_PROPERTY c,
    IN OUT PVOID                p,
    IN ULONG                    l
    )
{
    NTSTATUS status;

    RtlZeroMemory(p, l);
    status = IoGetDeviceProperty(d,
                                 c,
                                 l,
                                 p,
                                 &l);
    return status;
}

PWCHAR MrStrStr(PUNICODE_STRING s, PWCHAR t, ULONG len)
{
    USHORT i = 0;

    while ((i + len)* sizeof(WCHAR) <= s->Length) {
        if (0 == _wcsnicmp(&s->Buffer[i], t, len))
            return &s->Buffer[i];
        i++;
    }

    return NULL;
}


NTSTATUS MrInitDcb(struct mr_dcb* dcb, PDEVICE_OBJECT target)
{
    PWCHAR   p;
    NTSTATUS status;

    /* query hw information */
    status = MrQueryBinary(target,
                           DevicePropertyBusTypeGuid,
                           &dcb->md_bus,
                           sizeof(GUID));

    status = MrQueryString(target,
                           DevicePropertyDeviceDescription,
                           &dcb->md_desc);

    status = MrQueryString(target,
                           DevicePropertyManufacturer,
                           &dcb->md_ven);

    status = MrQueryString(target,
                           DevicePropertyPhysicalDeviceObjectName,
                           &dcb->md_name);

    status = MrQueryString(target,
                           DevicePropertyHardwareID,
                           &dcb->md_did);

    if (!NT_SUCCESS(status)) {
        goto errorout;
    }

    if (p = MrStrStr(&dcb->md_did, L"&MI_", 4)) {
        p[0] = 0;
        dcb->md_did.Length = wcslen(dcb->md_did.Buffer)
                             * sizeof(WCHAR);
    }
    if (p = MrStrStr(&dcb->md_did, L"&Col", 4)) {
        p[0] = 0;
        dcb->md_did.Length = wcslen(dcb->md_did.Buffer)
                             * sizeof(WCHAR);
    }

errorout:
    return status;
}


PWCHAR MrLookupId(struct mr_dcb *dcb)
{
    ULONG           l = 0;

    while (l < g_core.mc_dev_bytes) {

        PWCHAR s = g_core.mc_dev_list, p;
        s += l / sizeof(WCHAR);

        if (0 == wcslen(s))
            goto errorout;

        /* get filter rules provided by user in reg */
        if (p = MrStrStr(&dcb->md_did, s, wcslen(s))) {
            return s;
        }

        l += wcslen(s) * sizeof(WCHAR);

errorout:
        l += sizeof(WCHAR);
    }

    return NULL;
}

NTSTATUS MrAddDevice(
    IN PDRIVER_OBJECT       DriverObject,
    IN PDEVICE_OBJECT       DeviceObject
    )
{
    struct mr_dcb           *dcb;
    PDEVICE_OBJECT           devobj = NULL;
    NTSTATUS                 status;

    /* create filter object for target device */
    status = IoCreateDevice(DriverObject,
                            sizeof(struct mr_dcb),
                            NULL,                    
                            FILE_DEVICE_MOUSE,
                            0,
                            FALSE,
                            &devobj
                            );

    if (!NT_SUCCESS(status)) {
        goto errorout;
    }

    /* initialize device extension */
    dcb = (struct mr_dcb*)devobj->DeviceExtension;
    memset(dcb, 0, sizeof(struct mr_dcb));

    /* query device info (VID/DID ...) */
    status = MrInitDcb(dcb, DeviceObject);

    /* check whether this mouse is to be swapped */
    if (STATUS_SUCCESS == status) {
        if (MrLookupId(dcb)) {
            dcb->md_state |= MR_DEV_STATE_PERSIST |
                             MR_DEV_STATE_ENABLED;
        }
    }

    /* attach filter into target device stack */
    dcb->md_target = IoAttachDeviceToDeviceStack(
                            devobj, DeviceObject);
    devobj->Flags |= DO_BUFFERED_IO;
    devobj->Flags &= ~DO_DEVICE_INITIALIZING;

    /* insert dcb into global list */
    MrLockGlobal(TRUE);
    InsertTailList(&g_core.mc_dcb_list, &dcb->md_link);
    g_core.mc_dcb_count++;
    MrUnlockGlobal();

errorout:
    return STATUS_SUCCESS;
}

VOID MrCleanupString(PUNICODE_STRING n)
{
    if (n && n->Buffer) {
        MrFree(n->Buffer);
    }
}

NTSTATUS MrRemoveDevice(IN PDEVICE_OBJECT devobj)
{
    struct mr_dcb* dcb = devobj->DeviceExtension;

    /* remove dcb from global list */
    MrLockGlobal(TRUE);
    RemoveEntryList(&dcb->md_link);
    InitializeListHead(&dcb->md_link);
    g_core.mc_dcb_count--;
    MrUnlockGlobal();

    /* free nameing buffers */
    MrCleanupString(&dcb->md_desc);
    MrCleanupString(&dcb->md_did);
    MrCleanupString(&dcb->md_ven);
    MrCleanupString(&dcb->md_name);

    /* now delete the filter devobj */
    if (dcb->md_target)
        IoDetachDevice(dcb->md_target);
    IoDeleteDevice(devobj);

    return STATUS_SUCCESS;
}

NTSTATUS MrPnP(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp 
    )
{
    PIO_STACK_LOCATION  iosp;
    NTSTATUS            status;
    CHAR                minor;

    iosp = IoGetCurrentIrpStackLocation(Irp);
    minor = iosp->MinorFunction;

    /* just passthru the requst */
    status = MrPassthru(DeviceObject, Irp);

    /* do cleaup if device was just removed */
    if (IRP_MN_REMOVE_DEVICE == minor) {
        status = MrRemoveDevice(DeviceObject);
    }

    return status;
}

NTSTATUS MrPower(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp 
    )
{
    struct mr_dcb *dcb = DeviceObject->DeviceExtension;

    /* just passthru all power requests */    
    PoStartNextPowerIrp(Irp);
    IoSkipCurrentIrpStackLocation(Irp);
    
    return PoCallDriver(dcb->md_target, Irp);
}

PVOID
MrGetUserBuffer(IN PIRP Irp)
{
    ASSERT(Irp != NULL);

    if (Irp->MdlAddress) {
        return MmGetSystemAddressForMdlSafe(Irp->MdlAddress,
                                         NormalPagePriority);
    } else {
        return Irp->UserBuffer;
    }
}

ULONG MrSlotSize(struct mr_dcb *dcb)
{
    ULONG rc = MR_IO_SLOT_SIZE;

    if (dcb->md_desc.Length)
        rc += dcb->md_desc.Length + sizeof(WCHAR);
    if (dcb->md_name.Length)
        rc += dcb->md_name.Length + sizeof(WCHAR);
    if (dcb->md_ven.Length)
        rc += dcb->md_ven.Length + sizeof(WCHAR);
    rc += dcb->md_did.Length + sizeof(WCHAR);
    return rc;
}

BOOLEAN
MirFillIoElement(
    PMR_IO_SLOT     slot,
    PMR_IO_ELEMENT  ioe,
    PUNICODE_STRING n
    )
{
    if (!n || !n->Buffer || !n->Length)
        return FALSE;

    ioe->me_start = slot->mi_size;
    ioe->me_size = n->Length;
    slot->mi_size += ioe->me_size + sizeof(WCHAR);

    RtlCopyMemory((PUCHAR)slot + ioe->me_start,
                  n->Buffer, n->Length);
    return TRUE;
}

BOOLEAN MrFillIoSlot(
    PMR_IO_RECORD   mir,
    ULONG           len,
    struct mr_dcb  *dcb
    )
{
    PMR_IO_SLOT     slot;
    ULONG bytes;

    bytes = MrSlotSize(dcb);
    if (bytes + mir->mi_length > len)
        return FALSE;

    /* initialize slot structure */
    slot = (PMR_IO_SLOT)((PUCHAR)mir + mir->mi_length);
    slot->mi_magic =  MR_IO_SLOT_MAGIC;
    slot->mi_size = MR_IO_SLOT_SIZE;
    slot->mi_state = dcb->md_state;
    slot->mi_bus = dcb->md_bus;
    slot->mi_dcb = (__int64)dcb;

    /* process namings */
    MirFillIoElement(slot, &slot->mi_name, &dcb->md_name);
    MirFillIoElement(slot, &slot->mi_id,   &dcb->md_did);
    MirFillIoElement(slot, &slot->mi_desc, &dcb->md_desc);
    MirFillIoElement(slot, &slot->mi_ven,  &dcb->md_ven);

    /* add buffer consuming to mir */
    mir->mi_length += slot->mi_size;
    mir->mi_nslots +=1;

    return TRUE;
}

NTSTATUS MrUserRead(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp 
    )
{
    PIO_STACK_LOCATION  iosp;
    PLIST_ENTRY         link;
    struct mr_dcb      *dcb = NULL;
    PMR_IO_RECORD       mir;
    ULONG               len;
    ULONG               total = 0;
    
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    MrLockGlobal(FALSE);

    __try {

        iosp = IoGetCurrentIrpStackLocation(Irp);
        len = iosp->Parameters.Read.Length;
        mir = (PMR_IO_RECORD)MrGetUserBuffer(Irp);
        if (NULL == mir)
            __leave;
        memset(mir, 0, len);
        if (len <= MR_IO_REC_SIZE + MR_IO_SLOT_SIZE)
            __leave;

        link = g_core.mc_dcb_list.Flink;
        if (iosp->FileObject && iosp->FileObject->FsContext2) {
            while (link != &g_core.mc_dcb_list) {
                dcb = CONTAINING_RECORD(link, struct mr_dcb, md_link);
                if (dcb == iosp->FileObject->FsContext2) {
                    break;
                } else {
                    link = link->Flink;
                }
            }
            if (link == &g_core.mc_dcb_list) {
                dcb = NULL;
                link = g_core.mc_dcb_list.Flink;
            }
        }

        mir->mi_magic = MR_IO_MAGIC;
        mir->mi_length = MR_IO_REC_SIZE;

        while (link != &g_core.mc_dcb_list) {
            dcb = CONTAINING_RECORD(link, struct mr_dcb, md_link);
            link = link->Flink;
            if (!MrFillIoSlot(mir, len, dcb)) {
                break;
            }
            total = mir->mi_length;
            if (link == &g_core.mc_dcb_list) {
                dcb = NULL;
            }
        }

    } __finally {

        MrUnlockGlobal();
        if (total) {
            status = STATUS_SUCCESS;
            Irp->IoStatus.Information = total;
        }
        if (iosp->FileObject) {
            iosp->FileObject->FsContext2 =
                NT_SUCCESS(status) ? dcb : NULL;
        }
    }

    return status;
}

NTSTATUS MrSetRegValue(PWCHAR n, PVOID d, ULONG l)
{
    OBJECT_ATTRIBUTES oa = {0};
    HANDLE            hk = NULL;
    NTSTATUS          status;
    UNICODE_STRING    name;

    InitializeObjectAttributes(
        &oa, &g_core.mc_reg,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        NULL, NULL);

    status = ZwOpenKey(&hk, KEY_ALL_ACCESS, &oa);
    if (!NT_SUCCESS(status))
        goto errorout;

    RtlInitUnicodeString(&name, n);
    status = ZwSetValueKey(hk,
                           &name,
                           0,
                           REG_MULTI_SZ,
                           d,
                           l);
errorout:

    if (hk != 0)
        ZwClose(hk);

    return status;
}


NTSTATUS MrUpdateHWs(ULONG s1, ULONG s2, PMR_DCB dcb)
{
    PWCHAR              s, p;
    ULONG               b = 0, l, e, n;
    UNICODE_STRING      t;
    NTSTATUS            status = STATUS_SUCCESS;
    BOOLEAN             changed = FALSE;

    e = g_core.mc_dev_bytes / sizeof(WCHAR);

    /* remove old tag */
    if (s1 & MR_DEV_STATE_PERSIST) {

        while (s = MrLookupId(dcb)) {
            b = (ULONG)(s - g_core.mc_dev_list);
            l = wcslen(s);
            memset(s, 0, l * sizeof(WCHAR));
            if (b + l + 1 < e) {
                memmove(s, s + l + 1, (e - b - l - 1)
                                       * sizeof(WCHAR));
                memset(s + (e - b - l - 1), 0,
                       (l + 1) * sizeof(WCHAR));
            }
            changed = TRUE;
        }
        b = e - b - l;
    }

    /* add new tag */
    if (s2 & MR_DEV_STATE_PERSIST) {

        b = 0;

        n = dcb->md_did.Length / sizeof(WCHAR);
        if (p = MrStrStr(&dcb->md_did, L"&REV_", 5)) {
            if (p < dcb->md_did.Buffer + n)
                n = (ULONG)(p - dcb->md_did.Buffer);
        }
        if (p = MrStrStr(&dcb->md_did, L"&MI_", 4)) {
            if (p < dcb->md_did.Buffer + n)
                n = (ULONG)(p - dcb->md_did.Buffer);
        }
        if (p = MrStrStr(&dcb->md_did, L"&Col", 4)) {
            if (p < dcb->md_did.Buffer + n)
                n = (ULONG)(p - dcb->md_did.Buffer);
        }

        while (l < g_core.mc_dev_bytes/sizeof(WCHAR)) {

            s = g_core.mc_dev_list + b;
            if (l = wcslen(s)) {
                b += l + 1;
            } else {
                break;
            }
        }

        if (e < b + n + 2) {
            s = MrAlloc((b + n + 0x100) * sizeof(WCHAR));
            if (NULL == s) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto errorout;
            }
            RtlCopyMemory(s, g_core.mc_dev_list, b*sizeof(WCHAR));
            MrFree(g_core.mc_dev_list);
            g_core.mc_dev_list = s;
            e = b + n + 0x100;
            g_core.mc_dev_bytes = e * sizeof(WCHAR);
        }

        RtlCopyMemory(g_core.mc_dev_list + b, dcb->md_did.Buffer,
                      n * sizeof(WCHAR));
        b += n + 2;
        changed = TRUE;
    }

    if (changed) {
        /* set registry */
        while (b >= 3) {
            if (g_core.mc_dev_list[b - 3] == 0 &&
                g_core.mc_dev_list[b - 2] == 0 &&
                g_core.mc_dev_list[b - 1] == 0 )
                b = b - 1;
            else
                break;
        }
        status = MrSetRegValue(MOURE_DEVLIST, g_core.mc_dev_list,
                               b * sizeof(WCHAR));
    }

errorout:
    return status;
}

NTSTATUS MrUserWrite(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp 
    )
{
    PIO_STACK_LOCATION  iosp;
    PLIST_ENTRY         link;
    struct mr_dcb      *dcb;
    PMR_IO_RECORD       mir;
    PMR_IO_SLOT         slot;
    ULONG               len;
    ULONG               total = MR_IO_REC_SIZE;
    
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    MrLockGlobal(TRUE);

    __try {

        iosp = IoGetCurrentIrpStackLocation(Irp);
        len = iosp->Parameters.Write.Length;
        mir = (PMR_IO_RECORD)MrGetUserBuffer(Irp);
        if (NULL == mir)
            __leave;
        if (len <= MR_IO_REC_SIZE + MR_IO_SLOT_SIZE)
            __leave;

        if (mir->mi_magic  != MR_IO_MAGIC ||
            mir->mi_length < MR_IO_REC_SIZE + MR_IO_SLOT_SIZE) {
            __leave;
        }

        while (total + MR_IO_SLOT_SIZE < mir->mi_length) {

            slot = (PMR_IO_SLOT)((PUCHAR)mir + total);
            if (slot->mi_magic !=  MR_IO_SLOT_MAGIC ||
                slot->mi_cmd != MR_IO_CMD_SET ||
                slot->mi_size == 0 || slot->mi_dcb == 0) {
                __leave;
            }
            total += slot->mi_size;

            for (link  = g_core.mc_dcb_list.Flink;
                 link != &g_core.mc_dcb_list;
                 link  = link->Flink) {

                dcb = CONTAINING_RECORD(link, MR_DCB, md_link);
                if ((__int64)dcb == slot->mi_dcb)
                    break;
                else
                    dcb = NULL;
            }

            if (dcb) {
                MrUpdateHWs(dcb->md_state, slot->mi_state, dcb);
                dcb->md_state = slot->mi_state; 
            }
        }

    } __finally {

        MrUnlockGlobal();
        if (iosp->FileObject)
            iosp->FileObject->FsContext2 = NULL;
        if (total) {
            status = STATUS_SUCCESS;
            Irp->IoStatus.Information = total;
        }
    }

    return status;
}

NTSTATUS MrProcessUser(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             irp 
    )
{
    PIO_STACK_LOCATION  iosp;
    NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;

    __try {

        iosp = IoGetCurrentIrpStackLocation(irp);

        if (iosp->MajorFunction == IRP_MJ_CREATE) {
            irp->IoStatus.Information = FILE_OPENED;
            status = STATUS_SUCCESS;
        } else if (iosp->MajorFunction == IRP_MJ_CLEANUP) {
            status = STATUS_SUCCESS;
        } else if (iosp->MajorFunction == IRP_MJ_CLOSE) {
            status = STATUS_SUCCESS;
        } else if (iosp->MajorFunction == IRP_MJ_READ) {
            status = MrUserRead(DeviceObject, irp);
        } else if (iosp->MajorFunction == IRP_MJ_WRITE) {
            status = MrUserWrite(DeviceObject, irp);
        } else if (iosp->MajorFunction == IRP_MJ_SHUTDOWN) {
        }

    } __finally {

        irp->IoStatus.Status = status;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
    }

    return status;
}


NTSTATUS MrDispatch(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp 
    )
{
    PIO_STACK_LOCATION  iosp;

    iosp = IoGetCurrentIrpStackLocation(Irp);

    /* user mode manager requests */
    if (DeviceObject == g_core.mc_devobj) {
        return MrProcessUser(DeviceObject, Irp);
    }

    /* do swapping for reading */
    if (IRP_MJ_READ == iosp->MajorFunction)
        return MrRead(DeviceObject, Irp);

    /* do extra processing for pnp request */
    if (IRP_MJ_PNP == iosp->MajorFunction)
        return MrPnP(DeviceObject, Irp);

    /* do extra processing for power request */
    if (IRP_MJ_POWER == iosp->MajorFunction)
        return MrPower(DeviceObject, Irp);

    /* enable passthru mode */
    return MrPassthru(DeviceObject, Irp);
}

VOID MrUnload(
   IN PDRIVER_OBJECT Driver
   )
{
    MrStopCore();
}


NTSTATUS DriverEntry(
    IN PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING  RegistryPath 
    )
{
    NTSTATUS            status;
    ULONG               i;

    DEBUG("Moure (%s %s-%s) started.\n", g_drvVersion,
           g_drvDate, g_drvTime);

    /* initialize global defintions */
    status = MrStartCore(DriverObject, RegistryPath);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    /* initialize irp & callback entries */
    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {
        DriverObject->MajorFunction[i] = MrDispatch;
    }

    DriverObject->DriverExtension->AddDevice = MrAddDevice;
    DriverObject->DriverUnload = MrUnload;

    return STATUS_SUCCESS;
}
