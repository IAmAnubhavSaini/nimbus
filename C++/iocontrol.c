#include "precomp.h"






NTSTATUS
IoFtQueryProtocol(
IN PIRP Irp,
IN PIO_STACK_LOCATION IrpSp)
{
	FT_INFO_PROTOCOL          *Info;
	//INT                         i;
	NTSTATUS                    Status = STATUS_SUCCESS;
	BOOLEAN	bFalse = FALSE;
	//DBGPRINT(MUX_LOUD, ("==>IoBftQueryProtocol\n"));

	Irp->IoStatus.Information = 0;

	do
	{
		if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof *Info)
		{
			//DBGPRINT(MUX_WARN, ("[IoBftQueryProtocol] OutputBufferLength %d < sizeof(*Info) %d\n",
				//IrpSp->Parameters.DeviceIoControl.OutputBufferLength, sizeof *Info));
			Status = STATUS_INVALID_PARAMETER;
			break;
		}

		Info = (FT_INFO_PROTOCOL *)Irp->AssociatedIrp.SystemBuffer;

		//NdisMoveMemory(Info->DeviceAddress,
			//FTDeviceAddress,
			//NDIS_MAX_PHYS_ADDRESS_LENGTH);


		Info->initialized = ft.initialized;
		Info->enable_pkt_samp = ft.enable_pkt_samp;
		Info->enable_flow_samp = ft.enable_flow_samp;
		Info->pkt_default_samp = ft.pkt_samp_default;
		Info->flow_default_samp = ft.flow_samp_default;
		//Info->mode = bft.mode;
		//Info->MinNotifyPacket = bft.MinNotifyPacket;
		Info->MaxActiveConn = ft.MaxActiveConn;
		//Info->FlowTimeout = (INT)(bft.FlowTimeout / SECOND);
		//Info->FlowCheckPeriod = (INT)(bft.FlowReportPeriod / MILLISECOND);

		Info->CountActiveConn = ft.CountActiveConn;
		Info->CountMaxActiveConn = ft.CountMaxActiveConn;
		Info->CountTotalConn = ft.CountTotalConn;
		//for (i = 0; i < BFT_FLOW_TYPE_NUM; ++i)
		//{
		//	Info->CountTotalFlowNotifyNum[i] = bft.CountTotalNotifyFlowNum[i];
		//}
		//Info->CountDeliverUserPacket = bft.CountDeliverUserPacket;

		Info->CountAllocateConnFail = ft.CountAllocateConnFail;
		//Info->CountOtherFwdPackets = bft.CountOtherFwdPackets;
		//Info->CountOtherFwdBytes = bft.CountOtherFwdBytes;
		//Info->CountFwdPackets = bft.CountFwdPackets;
		//Info->CountFwdBytes = bft.CountFwdBytes;
		Info->MaxTokenSize = ft.MaxTokenSize;

		Info->CountReachMaxActiveConn = ft.CountReachMaxActiveConn;
		Info->CountDiscardSmallPacket = ft.CountDiscardSmallPacket;
		Info->CountDiscardBMcastPacket = ft.CountDiscardBMcastPacket;
		//Info->CountFlowNotifyTableFull = bft.CountFlowNotifyTableFull;
		Info->CountDeliverUserPacket = ft.CountDeliverUserPacket;
		Info->CountRecvDiscardUserPacket = ft.CountRecvDiscardUserPacket;
		Info->CountSamplePacket = ft.CountSamplePacket;
		Info->CountSetSampleRateFail = ft.CountSetSampleRateFail;

		Irp->IoStatus.Information = sizeof *Info;

	} while (bFalse);

	Irp->IoStatus.Status = Status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	//DBGPRINT(MUX_LOUD, ("<==IoBftQueryProtocol %x\n", Status));

	return Status;
}



NTSTATUS
IoControlSampleRate(
	IN PIRP Irp,
	IN PIO_STACK_LOCATION IrpSp)
{
	FT_CONTROL_SAMPLE_RATE *Control_Sample_Rate;
	NTSTATUS		Status = STATUS_SUCCESS;
	BOOLEAN		bFalse = FALSE;

#ifdef KERNEL_TABLE
	FLOW_LABEL  flowkey;
	UINT        psample_rate;
	UINT		fsample_rate;
	UINT		i;
	UINT		hash;
	PCONN		pConn;
#endif

	Irp->IoStatus.Information = 0;

	do
	{
		if (IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof *Control_Sample_Rate)
		{
			Status = STATUS_INVALID_PARAMETER;
			break;
		}

		Control_Sample_Rate = (FT_CONTROL_SAMPLE_RATE *)Irp->AssociatedIrp.SystemBuffer;
		
		if (Control_Sample_Rate->reason == 0)  //change default
		{
			if (Control_Sample_Rate->flow_samp_default != (UINT)-1)
			{
				
				ft.flow_samp_default = min(MAXPKTSAMPLENUM, Control_Sample_Rate->flow_samp_default);
			}
			if (Control_Sample_Rate->pkt_samp_default != (UINT)-1)
			{
				ft.pkt_samp_default = min(MAXFLOWSAMPLENUM, Control_Sample_Rate->pkt_samp_default);
			}
		}
#ifdef KERNEL_TABLE
		else
		{
			pConn = NULL;
			for (i = 0; i < Control_Sample_Rate->nflow; ++i)
			{
				flowkey.daddr = Control_Sample_Rate->sample_rate[i].key.daddr;
				flowkey.protocol = Control_Sample_Rate->sample_rate[i].key.protocol;
				psample_rate = Control_Sample_Rate->sample_rate[i].psrate;
				fsample_rate = Control_Sample_Rate->sample_rate[i].fsrate;

				hash = Control_Sample_Rate->sample_rate[i].hash;

				NdisDprAcquireSpinLock(&ConnHashTable[hash].Lock);
				pConn = FindConn(&ConnHashTable[hash], &flowkey);
				if (pConn)
				{
					if (psample_rate != (UINT)-1)
						pConn->PktSampleRate = psample_rate;
					if (fsample_rate != (UINT)-1)
						pConn->FlowSampleRate = fsample_rate;
				}
				else
				{
					InterlockedIncrement((PLONG)&ft.CountSetSampleRateFail);
				}
				NdisDprReleaseSpinLock(&ConnHashTable[hash].Lock);
			}
		}
#endif
	} while (bFalse);

	Irp->IoStatus.Status = Status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	//DBGPRINT(MUX_LOUD, ("<==IoBftControlProtocol %x\n", Status));

	return Status;
}

NTSTATUS
IoFtControlProtocol(
IN PIRP Irp,
IN PIO_STACK_LOCATION IrpSp)
{
	FT_CONTROL_PROTOCOL       *Control;
	//INT                         i;
	NTSTATUS                    Status = STATUS_SUCCESS;
	BOOLEAN bFalse = FALSE;
	//NDIS_CONFIGURATION_OBJECT           ConfigObject;
	//NDIS_HANDLE     ConfigurationHandle = NULL;
	//PVOID           NetworkAddress;
	//UINT            i;


	//DEBUGP(DL_TRACE, "===>IoBftControlProtocol\n");
	Irp->IoStatus.Information = 0;

	do
	{
		if (IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof *Control)
		{
			//DBGPRINT(MUX_WARN, ("[IoBftControlProtocol] InputBufferLength %d < sizeof(*Control) %d\n",
				//IrpSp->Parameters.DeviceIoControl.InputBufferLength, sizeof *Control));
			Status = STATUS_INVALID_PARAMETER;
			break;
		}

		Control = (FT_CONTROL_PROTOCOL *)Irp->AssociatedIrp.SystemBuffer;

		if (Control->initialized != (UINT)-1)
		{
			ft.initialized = Control->initialized;
			
			/*
			ConfigObject.Header.Type = NDIS_OBJECT_TYPE_CONFIGURATION_OBJECT;
			ConfigObject.Header.Revision = NDIS_CONFIGURATION_OBJECT_REVISION_1;
			ConfigObject.Header.Size = sizeof(NDIS_CONFIGURATION_OBJECT);
			ConfigObject.NdisHandle = FilterDriverHandle;
			ConfigObject.Flags = 0;

			Status = NdisOpenConfigurationEx(
				&ConfigObject,
				&ConfigurationHandle);

			if (Status != NDIS_STATUS_SUCCESS)
			{
				//DBGPRINT(MUX_ERROR, ("[MPInitialize] NdisOpenConfiguration failed %x\n", Status));
				DEBUGP(DL_ERROR, "NdisOpenConfiguration failed %x\n", Status);
				break;
			}

			NdisReadNetworkAddress(
				(PNDIS_STATUS)&Status,
				&NetworkAddress,
				&i,
				ConfigurationHandle);
			if (((Status == NDIS_STATUS_SUCCESS)
				&& (i == ETH_LENGTH_OF_ADDRESS))
				&& ((!ETH_IS_MULTICAST(NetworkAddress))
				&& (ETH_IS_LOCALLY_ADMINISTERED(NetworkAddress))))
			{
				ETH_COPY_NETWORK_ADDRESS(FTDeviceAddress, NetworkAddress);
			}
			
			*/
		}

		//if (Control->mode != (UINT)-1)
		//{
		//	bft.mode = Control->mode;
		//}

		//if (Control->MinNotifyPacket != (UINT)-1)
		//{
		//	bft.MinNotifyPacket = Control->MinNotifyPacket;
		//}

		//if (Control->FlowTimeout != (UINT)-1)
		//{
		//	bft.FlowTimeout = (ULONGLONG)Control->FlowTimeout * SECOND;
		//}
		if (Control->MaxTokenSize != (UINT)-1)
		{
			ft.MaxTokenSize = Control->MaxTokenSize;
		}
		if (Control->MaxActiveConn != (UINT)-1)
		{
			ft.MaxActiveConn = Control->MaxActiveConn;
		}

		//if (Control->FlowCheckPeriod != (UINT)-1)
		//{
		//	bft.FlowReportPeriod = (ULONGLONG)Control->FlowCheckPeriod * MILLISECOND;
		//}
		if (Control->enable_pkt_samp != (UINT)-1)
			ft.enable_pkt_samp = Control->enable_pkt_samp;

		if (Control->enable_flow_samp != (UINT)-1)
			ft.enable_flow_samp = Control->enable_flow_samp;


		if (Control->ResetCounter == 1)
		{
			ft.CountTotalConn = 0;
			ft.CountMaxActiveConn = 0;

			//ft.CountOtherFwdBytes = 0;
			//ft.CountOtherFwdPackets = 0;
			//bft.CountFwdBytes = 0;
			//bft.CountFwdPackets = 0;

			ft.CountAllocateConnFail = 0;
			ft.CountReachMaxActiveConn = 0;
			ft.CountDiscardSmallPacket = 0;
			ft.CountDiscardBMcastPacket = 0;
			//ft.CountFlowNotifyTableFull = 0;
			ft.CountDeliverUserPacket = 0;
			ft.CountRecvDiscardUserPacket = 0;
			ft.CountSamplePacket = 0;
			ft.CountSetSampleRateFail = 0;

			//for (i = 0; i < BFT_FLOW_TYPE_NUM; ++i)
			//{
			//	bft.CountTotalNotifyFlowNum[i] = 0;
			//}
			//bft.CountDeliverUserPacket = 0;

			FTFlush();

		}
	} while (bFalse);

	Irp->IoStatus.Status = Status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	//DBGPRINT(MUX_LOUD, ("<==IoBftControlProtocol %x\n", Status));

	return Status;
}




#ifdef KERNEL_TABLE
NTSTATUS
IoFTQueryFlowSet(
	IN PIRP Irp,
	IN PIO_STACK_LOCATION IrpSp)
{
	NTSTATUS            Status = STATUS_SUCCESS;
	UINT                i;
	//INT                 j;
	FT_QUERY_FLOWSET  *Query;
	FT_INFO_FLOWSET   *Info;
	BOOLEAN bFALSE = FALSE;


	//DBGPRINT(MUX_VERY_LOUD, ("==>IoBftQueryFlowSet\n"));

	do
	{
		if (IrpSp->Parameters.DeviceIoControl.InputBufferLength != sizeof (FT_QUERY_FLOWSET))
		{
			//DBGPRINT(MUX_WARN, ("[IoBftQueryFlowSet] InputBufferLength %d != %d\n",
			//	IrpSp->Parameters.DeviceIoControl.InputBufferLength,
			//	sizeof(BFT_QUERY_FLOWSET)));
			Status = STATUS_INVALID_PARAMETER;
			break;
		}
		Query = (FT_QUERY_FLOWSET *)Irp->AssociatedIrp.SystemBuffer;

		if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength != sizeof (FT_INFO_FLOWSET)+Query->nflow*sizeof(FT_INFO_FLOW))
		{
			//DBGPRINT(MUX_WARN, ("[IoBftQueryFlowSet] OutputBufferLength %d != %d\n",
			//	IrpSp->Parameters.DeviceIoControl.OutputBufferLength,
			//	sizeof(BFT_INFO_FLOWSET)+Query->nflow*sizeof(BFT_INFO_FLOW)));
			Status = STATUS_INVALID_PARAMETER;
		}

		Info = (FT_INFO_FLOWSET *)Irp->AssociatedIrp.SystemBuffer;
	

		FTGettop();

		i = 0;
		while (FTFlowNotifyTable.head != FTFlowNotifyTable.tail && i < Query->nflow)
		{
			Info->flow[i] = FTFlowNotifyTable.flows[FTFlowNotifyTable.head];
			FTFlowNotifyTable.head = (FTFlowNotifyTable.head + 1) % MAX_FLOW_NOTIFY_TABLE;
			++i;
		}

		Info->nflow = i;
		Irp->IoStatus.Status = Status;
		Irp->IoStatus.Information = sizeof(FT_INFO_FLOWSET)+Info->nflow * sizeof(FT_INFO_FLOW);

		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		
	} while (bFALSE);

	//DBGPRINT(MUX_VERY_LOUD, ("<==IoBftQueryFlowSet 0x%8x\n", Status));

	return Status;


}
#endif


void
CancelReceivePacketSet(
IN PDEVICE_OBJECT DeviceObject,
IN PIRP Irp
)
{
	KIRQL           cancelIrql = Irp->CancelIrql;
	PIRP            cancelIrp = NULL;

	UNREFERENCED_PARAMETER(DeviceObject);

	//DBGPRINT(MUX_LOUD, ("==>CancelReceivePacketSet\n"));

	IoReleaseCancelSpinLock(cancelIrql);

	NdisAcquireSpinLock(&GlobalVElan.UserRecvPacketSetSpinLock);

	cancelIrp = GlobalVElan.UserRecvPacketSetIrp;
	GlobalVElan.UserRecvPacketSetIrp = NULL;

	NdisReleaseSpinLock(&GlobalVElan.UserRecvPacketSetSpinLock);

	if (cancelIrp)
	{
		cancelIrp->IoStatus.Status = STATUS_CANCELLED;
		Irp->IoStatus.Information = 0;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
	}

	//DBGPRINT(MUX_LOUD, ("<==CancelReceivePacketSet\n"));
}



NTSTATUS
IoControlReceivePacketSet(
IN PIRP                 Irp,
IN PIO_STACK_LOCATION   IrpSp)
{
	NTSTATUS                Status = STATUS_SUCCESS;
	UINT                    i;
	//INT                     j;
	QUERY_USER_PACKET_SET  *Query;
	PUSER_PACKET_SET        Info;
	PUSER_PACKET            upkt;
	//UINT                    npktmax;
	//UINT                    buflen;
	BOOLEAN bFalse = FALSE;

	//DBGPRINT(MUX_VERY_LOUD, ("==>IoControlReceivePacketSet\n"));

	do
	{
		if (GlobalVElan.UserRecvPacketSetIrp)
		{
			//DBGPRINT(MUX_WARN, ("[IoControlReceivePacketSet] have a query already\n"));
			Status = STATUS_INVALID_PARAMETER;
			break;
		}

		if (IrpSp->Parameters.DeviceIoControl.InputBufferLength != sizeof (QUERY_USER_PACKET_SET))
		{
			//DBGPRINT(MUX_WARN, ("[IoControlReceivePacketSet] InputBufferLength %d != %d\n",
				//IrpSp->Parameters.DeviceIoControl.InputBufferLength,
				//sizeof(QUERY_USER_PACKET_SET)));
			Status = STATUS_INVALID_PARAMETER;
			break;
		}
		Query = (QUERY_USER_PACKET_SET *)Irp->AssociatedIrp.SystemBuffer;

		if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength !=
			sizeof (USER_PACKET_SET)+Query->npktmax * (sizeof(USER_PACKET)+Query->buflen))
		{
			//DBGPRINT(MUX_WARN, ("[IoControlReceivePacketSet] OutputBufferLength %d != %d\n",
				//IrpSp->Parameters.DeviceIoControl.OutputBufferLength,
				//sizeof (USER_PACKET_SET)+Query->npktmax * (sizeof(USER_PACKET)+Query->buflen)));
			Status = STATUS_INVALID_PARAMETER;
		}

		Info = (USER_PACKET_SET *)Irp->AssociatedIrp.SystemBuffer;

		NdisAcquireSpinLock(&GlobalVElan.UserRecvPacketTableSpinLock);

		if (GlobalVElan.UserRecvPacketTableHead != GlobalVElan.UserRecvPacketTableTail)
		{
			i = 0;
			do
			{
				upkt = (PUSER_PACKET)(Info->upkt + i * (sizeof(USER_PACKET)+Query->buflen));
				/* copy packet from user recv packet table */
				NdisMoveMemory(upkt,
					GlobalVElan.UserRecvPacketTable[GlobalVElan.UserRecvPacketTableHead],
					sizeof(USER_PACKET)+(GlobalVElan.UserRecvPacketTable[GlobalVElan.UserRecvPacketTableHead])->len);

				GlobalVElan.UserRecvPacketTableHead++;
				if (GlobalVElan.UserRecvPacketTableHead == USER_RECV_PACKET_TABLE_SIZE)
				{
					GlobalVElan.UserRecvPacketTableHead = 0;
				}
				++i;
			} while (GlobalVElan.UserRecvPacketTableHead != GlobalVElan.UserRecvPacketTableTail &&
				i < Query->npktmax);

			NdisReleaseSpinLock(&GlobalVElan.UserRecvPacketTableSpinLock);

			Info->npkt = i;
			Irp->IoStatus.Status = Status;
			Irp->IoStatus.Information = sizeof(USER_PACKET_SET)+Info->npkt * (sizeof(USER_PACKET)+Query->buflen);

			IoCompleteRequest(Irp, IO_NO_INCREMENT);
		}
		else
		{
			NdisReleaseSpinLock(&GlobalVElan.UserRecvPacketTableSpinLock);

			IoMarkIrpPending(Irp);

			IoSetCancelRoutine(Irp, CancelReceivePacketSet);

			NdisAcquireSpinLock(&GlobalVElan.UserRecvPacketSetSpinLock);
			GlobalVElan.UserRecvPacketSetIrp = Irp;
			NdisReleaseSpinLock(&GlobalVElan.UserRecvPacketSetSpinLock);
			Status = STATUS_PENDING;
		}


	} while (bFalse);

	//DBGPRINT(MUX_VERY_LOUD, ("<==IoControlReceivePacketSet 0x%8x\n", Status));

	return Status;
}



NTSTATUS
IoFtQueryPerformance(
IN PIRP Irp,
IN PIO_STACK_LOCATION IrpSp)
{
	FT_INFO_PERFORMANCE          *Info;
	//INT                         i;
	NTSTATUS                    Status = STATUS_SUCCESS;
	BOOLEAN	bFalse = FALSE;
	//DBGPRINT(MUX_LOUD, ("==>IoBftQueryProtocol\n"));

	Irp->IoStatus.Information = 0;

	do
	{
		if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof *Info)
		{
			//DBGPRINT(MUX_WARN, ("[IoBftQueryProtocol] OutputBufferLength %d < sizeof(*Info) %d\n",
			//IrpSp->Parameters.DeviceIoControl.OutputBufferLength, sizeof *Info));
			Status = STATUS_INVALID_PARAMETER;
			break;
		}

		Info = (FT_INFO_PERFORMANCE *)Irp->AssociatedIrp.SystemBuffer;

		//NdisMoveMemory(Info->DeviceAddress,
		//FTDeviceAddress,
		//NDIS_MAX_PHYS_ADDRESS_LENGTH);

		Info->LastTimerUpdateTime = (UINT)(GlobalVElan.LastTimerUpdateTime / MILLISECOND);
		Info->CurrentPPS = ft.CurrentPPS;
		Info->CurrentMBS = ft.CurrentMBS;
		Info->CurrentInboundPPS = ft.CurrentInboundPPS;

		Irp->IoStatus.Information = sizeof *Info;

	} while (bFalse);

	Irp->IoStatus.Status = Status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	//DBGPRINT(MUX_LOUD, ("<==IoBftQueryProtocol %x\n", Status));

	return Status;
}


NTSTATUS
FlowTableIoControl(
IN PIRP Irp)
{
	PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);

	switch (IrpSp->Parameters.DeviceIoControl.IoControlCode)
	{
	case IOCTL_FT_QUERY_PROTOCOL:
			return IoFtQueryProtocol(Irp, IrpSp);
		case IOCTL_FT_CONTROL_PROTOCOL:
			return IoFtControlProtocol(Irp, IrpSp);
#ifdef KERNEL_TABLE
		case IOCTL_FT_QUERY_FLOWSET:
			return IoFTQueryFlowSet(Irp, IrpSp);
#endif
		case IOCTL_FT_RECEIVE_PACKET_SET:
			return IoControlReceivePacketSet(Irp, IrpSp);
		case IOCTL_FT_CONTROL_SAMPLE_RATE:
			return IoControlSampleRate(Irp, IrpSp);
		case IOCTL_FT_QUERY_PERFORMANCE:
			return IoFtQueryPerformance(Irp, IrpSp);


	}

	Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
	return STATUS_NOT_IMPLEMENTED;
}