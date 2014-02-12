#ifndef _IOCONTROL_H
#define _IOCONTROL_H
#include "precomp.h"

#pragma warning(disable:28930) // Unused assignment of pointer, by design in samples
#pragma warning(disable:28931) // Unused assignment of variable, by design in samples


NTSTATUS
IoFtQueryProtocol(
IN PIRP Irp,
IN PIO_STACK_LOCATION IrpSp);


NTSTATUS
IoControlSampleRate(
IN PIRP Irp,
IN PIO_STACK_LOCATION IrpSp);

NTSTATUS
IoFtControlProtocol(
IN PIRP Irp,
IN PIO_STACK_LOCATION IrpSp);


NTSTATUS
IoFTQueryFlowSet(
IN PIRP Irp,
IN PIO_STACK_LOCATION IrpSp);



void
CancelReceivePacketSet(
IN PDEVICE_OBJECT DeviceObject,
IN PIRP Irp);


NTSTATUS
IoControlReceivePacketSet(
IN PIRP                 Irp,
IN PIO_STACK_LOCATION   IrpSp);


NTSTATUS
FlowTableIoControl(IN PIRP Irp);

#endif
