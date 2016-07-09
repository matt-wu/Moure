/*
 * Device Names
 */

#define MOURE_DEVICE    L"\\Device\\Mousre"        /* real device object name */
#define MOURE_SYMLNK    L"\\DosDevices\\Mousre"    /* user-visible symlink name */
#define MOURE_DOSDEV    L"\\\\.\\Mousre"           /* device for user mode app  */

/*
 * Registry Values
 */

#define MOURE_DEVLIST  L"MouseHWs"


/*
 * Data Exchange between Kernel and User modes
 */

typedef struct mr_io_element {
    USHORT          me_start;
    USHORT          me_size;
} MR_IO_ELEMENT, *PMR_IO_ELEMENT;

#define MR_IO_SLOT_MAGIC    'QIRM'

#define MR_DEV_STATE_ENABLED    (1 << 0)
#define MR_DEV_STATE_PERSIST    (1 << 1)

typedef struct mr_io_slot {

    USHORT          mi_size;    /* size of this slot */

    USHORT          mi_cmd;
    ULONG           mi_state;

    ULONG           mi_magic;
    ULONG           mi_flags;

    MR_IO_ELEMENT   mi_id;
    MR_IO_ELEMENT   mi_desc;
    MR_IO_ELEMENT   mi_name;
    MR_IO_ELEMENT   mi_ven;

    GUID            mi_bus;
    __int64         mi_dcb;

    UCHAR           mi_data[1];

} MR_IO_SLOT, *PMR_IO_SLOT;

#define MR_IO_SLOT_SIZE (FIELD_OFFSET(MR_IO_SLOT, mi_data))

#define MR_IO_CMD_BASE      '0M'
#define MR_IO_CMD_SET       MR_IO_CMD_BASE + 1

#define MR_IO_MAGIC         'RIRM'

typedef struct mr_io_record {

    ULONG           mi_magic;
    ULONG           mi_flags;

    ULONG           mi_nslots;
    ULONG           mi_length;

    MR_IO_SLOT      mi_slot[1];

} MR_IO_RECORD, *PMR_IO_RECORD;

#define MR_IO_REC_SIZE (FIELD_OFFSET(MR_IO_RECORD, mi_slot))

