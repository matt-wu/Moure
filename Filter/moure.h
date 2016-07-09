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

#ifndef _MR_HEADER_H_
#define _MR_HEADER_H_

#include <ntddk.h>
#include <wdmsec.h>
#include <hidclass.h>
#include <ntddmou.h>
#include <common.h>

/*
 *
 * Global Defintions
 *
 */


/*
 * Debugging Message Display
 */

#define DEBUG(format, ...)                                                      \
    do {                                                                        \
        DbgPrint("%d:%d:%s:%d:" format, KeGetCurrentProcessorNumber(),          \
                  KeGetCurrentIrql(), __FUNCTION__, __LINE__, ## __VA_ARGS__);  \
    } while(0)



/*
 *
 * Core Structures
 *
 */


/*
 * global
 */

typedef struct mr_core {

    PDEVICE_OBJECT  mc_devobj;   /* control devobj for manager console */
    ERESOURCE       mc_lock;
    LIST_ENTRY      mc_dcb_list; /* list of mouse devices */
    PWCHAR          mc_dev_list; /* buffer for registry settings */
    ULONG           mc_dev_bytes;/* bytes of mc_dev_list */
    ULONG           mc_dcb_count;/* count of mouse devices */
    UNICODE_STRING  mc_reg;      /* registry path */

} MR_CORE, *PMR_CORE;

/*
 * device extension definition
 */

typedef struct mr_dcb {

    LIST_ENTRY      md_link;    /* to be linked to global list */
    PDEVICE_OBJECT  md_target;  /* target device */
    GUID            md_bus;     /* bus GUID */
    UNICODE_STRING  md_did;     /* vendor and device id */
    UNICODE_STRING  md_ven;     /* manufactor */
    UNICODE_STRING  md_desc;    /* device desc */
    UNICODE_STRING  md_name;    /* device desc */
    ULONG           md_state;   /* flags: swap or not */
    ULONG           md_ext;

} MR_DCB, *PMR_DCB;


/*
 *
 * Function Prototypes
 *
 */

NTSTATUS DriverEntry(
    IN PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING  RegistryPath 
    );

#endif /* _MR_HEADER_H_ */