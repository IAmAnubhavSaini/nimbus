#include "precomp.h"

//#include "ntifs.h"


#ifdef KERNEL_TABLE
CONN_HASH_ENTRY         ConnHashTable[CONN_TABLE_SIZE];
FT_FLOW_NOTIFY_TABLE   FTFlowNotifyTable;
#endif

FLOWTABLE               ft;
VELAN					GlobalVElan;

//ULONG					randomseed;

EthernetAddress         EthBcastAddr = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
EthernetAddress         EthMcastAddr = { 0x01, 0x00, 0x53, 0x00, 0x00, 0x00 };
//EthernetAddress         EthGatewayAddr = { 0x00, 0x04, 0x23, 0xb7, 0x17, 0xe6 };





int
FlowTableInit()
{
	
	int         i;
	LARGE_INTEGER                       Timeout;
	

	KeInitializeDpc(&GlobalVElan.TimeoutDpc, FTTimeout, NULL);
	KeInitializeTimerEx(&GlobalVElan.Timer, NotificationTimer);
	Timeout.QuadPart = -(LONGLONG)(SECOND);
	GlobalVElan.Timeout = KeQueryInterruptTime() - Timeout.QuadPart;
	KeSetTimerEx(&GlobalVElan.Timer, Timeout, SECOND/MILLISECOND, &GlobalVElan.TimeoutDpc);
	GlobalVElan.LastTimerUpdateTime = KeQueryInterruptTime();


#ifdef KERNEL_TABLE
	

	for (i = 0; i < CONN_TABLE_SIZE; ++i)
	{
		NdisAllocateSpinLock(&ConnHashTable[i].Lock);
		InitializeListHead(&ConnHashTable[i].ListHead);
	}

	NdisAllocateSpinLock(&FTFlowNotifyTable.SpinLock);
	FTFlowNotifyTable.head = 0;
	FTFlowNotifyTable.tail = 0;
#endif

	NdisAllocateSpinLock(&GlobalVElan.UserRecvPacketSetSpinLock);
	NdisAllocateSpinLock(&GlobalVElan.UserRecvPacketTableSpinLock);
	GlobalVElan.UserRecvPacketTableHead = 0;
	GlobalVElan.UserRecvPacketTableTail = 0;
	GlobalVElan.UserRecvPacketSetIrp = NULL;

	GlobalVElan.UserRecvPacketTableBuffer =
		ExAllocatePool(NonPagedPool, USER_RECV_PACKET_TABLE_SIZE * (sizeof(USER_PACKET)+2000));
	if (!GlobalVElan.UserRecvPacketTableBuffer)
	{
		//DBGPRINT(MUX_ERROR, ("[DriverEntry] Cannot allocate UserRecvPacketTableBuffer\n"));
		//Status = NDIS_STATUS_RESOURCES;
		return 0;
	}

	for (i = 0; i < USER_RECV_PACKET_TABLE_SIZE; ++i)
	{
		GlobalVElan.UserRecvPacketTable[i] =
			(PUSER_PACKET)(GlobalVElan.UserRecvPacketTableBuffer + i * (sizeof(USER_PACKET)+2000));
	}


	NdisZeroMemory(&ft, sizeof ft);

	ft.initialized = 0;
	//bft.mode = 0;
	//bft.kmacfwd = 0;
	ft.MaxActiveConn = FT_MAX_ACTIVE_FLOW_NUM;
	ft.pkt_samp_default = BASEPKTSAMPLERATE;
	ft.flow_samp_default = BASEFLOWSAMPLERATE;
	ft.MaxTokenSize = MAX_TOKEN_SIZE;
	//bft.MinNotifyPacket = BFT_FLOW_NOTIFY_MIN_PACKET;
	//bft.FlowTimeout = BFT_FLOW_TIMEOUT;
	//bft.FlowReportPeriod = BFT_FLOW_REPORT_PERIOD;


	//p = KeQueryPerformanceCounter(NULL);
	//randomseed = p.LowPart;

	return 1;
}

VOID
FlowTableDeInit()
{
#ifdef KERNEL_TABLE
	int         i;

	for (i = 0; i < CONN_TABLE_SIZE; ++i)
	{
		NdisFreeSpinLock(&ConnHashTable[i].Lock);
	}
#endif

}


VOID
FTTimeout(
PKDPC Dpc,
void *Context,
void *Unused1,
void *Unused2)
{

	Time  Now;

	UNREFERENCED_PARAMETER(Dpc);
	UNREFERENCED_PARAMETER(Context);
	UNREFERENCED_PARAMETER(Unused1);
	UNREFERENCED_PARAMETER(Unused2);

	Now = KeQueryInterruptTime();

	GlobalVElan.LastTimerUpdateTime = Now;

	ft.CurrentPPS = ft.CountFwdPackets;
	ft.CurrentMBS = (UINT)((ft.CountFwdBytes * 8) / 1000000);

	ft.CountFwdPackets = 0;
	ft.CountFwdBytes = 0;

	ft.CurrentInboundPPS = ft.CountTokenPackets;
	ft.CountTokenPackets = 0;

}





PCONN
FindConn(
IN CONN_HASH_ENTRY *pConnHash,
IN FLOW_LABEL      *flowkey)
{
	PCONN           pConn = NULL;
	PLIST_ENTRY     pEntry;

	pEntry = pConnHash->ListHead.Flink;
	while (pEntry != &pConnHash->ListHead)
	{
		pConn = CONTAINING_RECORD(pEntry, CONN, ListEntry);
		if (//pConn->key.saddr == flowkey->saddr &&
			pConn->key.daddr == flowkey->daddr &&
			//pConn->key.sport == flowkey->sport &&
			//pConn->key.dport == flowkey->dport &&
			pConn->key.protocol == flowkey->protocol
			//pConn->key.portnum == flowkey->portnum
			)
		{
			return pConn;
		}
		pEntry = pEntry->Flink;
	}

	return NULL;
}






INT
FTDeliverUserPacket(
IN  PNET_BUFFER     NB,
IN  INT             reason)
{
	//UINT                i = 0;
	INT                 rc = FT_FAIL;
	PUCHAR              buf;
	UINT                PacketSize = NET_BUFFER_DATA_LENGTH(NB);
	PIRP                Irp;
	PUSER_PACKET_SET    Info;
	PUSER_PACKET        upkt;

	//DBGPRINT(MUX_VERY_LOUD, ("<== SNetDeliverUserPacket\n"));


	NdisAcquireSpinLock(&GlobalVElan.UserRecvPacketSetSpinLock);

	if (GlobalVElan.UserRecvPacketSetIrp)
	{
		Irp = GlobalVElan.UserRecvPacketSetIrp;
		GlobalVElan.UserRecvPacketSetIrp = NULL;
		NdisReleaseSpinLock(&GlobalVElan.UserRecvPacketSetSpinLock);

		Info = (PUSER_PACKET_SET)Irp->AssociatedIrp.SystemBuffer;
		Info->npkt = 1;

		upkt = (PUSER_PACKET)Info->upkt;

		upkt->len = PacketSize;
		//upkt->inExtIf = inExtIf;
		//upkt->inIntIf = inIntIf;
		//upkt->vlanid = vlanid;
		//upkt->priority = 0;
		upkt->reason = reason;
		buf = NdisGetDataBuffer(NB, PacketSize, NULL, 1, 0);
		if (buf)
		{
			NdisMoveMemory(upkt->buf, buf, PacketSize);
		}
		else
		{
			buf = NdisGetDataBuffer(NB, PacketSize, upkt->buf, 1, 0);
		}

		IoSetCancelRoutine(Irp, NULL);

		Irp->IoStatus.Status = STATUS_SUCCESS;
		Irp->IoStatus.Information = sizeof(USER_PACKET_SET)+sizeof(USER_PACKET)+PacketSize;

		IoCompleteRequest(Irp, IO_NO_INCREMENT);

		InterlockedIncrement((PLONG)&ft.CountRecvDeliverUserPacket);

		rc = FT_SUCCESS;
	}
	else
	{
		NdisReleaseSpinLock(&GlobalVElan.UserRecvPacketSetSpinLock);

		NdisAcquireSpinLock(&GlobalVElan.UserRecvPacketTableSpinLock);

		if ((GlobalVElan.UserRecvPacketTableTail + 1) % USER_RECV_PACKET_TABLE_SIZE
			== GlobalVElan.UserRecvPacketTableHead)
		{
			//DBGPRINT(MUX_WARN, ("[SNetDeliverUserPacket] UserRecvPacketTable full, head=%d\n",
				//GlobalVElan.UserRecvPacketTableTail));

			InterlockedIncrement((PLONG)&ft.CountRecvDiscardUserPacket);
			NdisReleaseSpinLock(&GlobalVElan.UserRecvPacketTableSpinLock);
		}
		else
		{

			upkt = GlobalVElan.UserRecvPacketTable[GlobalVElan.UserRecvPacketTableTail];
			GlobalVElan.UserRecvPacketTableTail++;
			if (GlobalVElan.UserRecvPacketTableTail == USER_RECV_PACKET_TABLE_SIZE)
			{
				GlobalVElan.UserRecvPacketTableTail = 0;
			}
			NdisReleaseSpinLock(&GlobalVElan.UserRecvPacketTableSpinLock);

			upkt->len = PacketSize;
			//upkt->inExtIf = inExtIf;
			//upkt->inIntIf = inIntIf;
			//upkt->vlanid = vlanid;
			//upkt->priority = 0;
			upkt->reason = reason;
			buf = NdisGetDataBuffer(NB, PacketSize, NULL, 1, 0);
			if (buf)
			{
				NdisMoveMemory(upkt->buf, buf, PacketSize);
			}
			else
			{
				buf = NdisGetDataBuffer(NB, PacketSize, upkt->buf, 1, 0);
			}

			rc = FT_SUCCESS;
		}
	}

	//DBGPRINT(MUX_VERY_LOUD, ("==> SNetDeliverUserPacket %d\n", rc));

	return rc;
}



VOID
FTProcessNetPacket(
IN PNET_BUFFER RecvNB)
//IN PNET_BUFFER_LIST RecvNBL)
{
	UINT				PacketSize;
	ETH_HEADER         *ethdr;
	IP_HEADER          *iphdr;
	L4_HEADER          *l4hdr;
	FLOW_LABEL          flowkey;
	
	

	
	BOOLEAN				bFalse = FALSE;
	
	LARGE_INTEGER		p;
	ULONG				random;
	//double				samplerate = MAXSAMPLENUM;
	UINT				reason;
	UINT				flowkeyhash;
#ifdef KERNEL_TABLE
	INT                 newflow = 0;
	PCONN               pConn;
	UINT                hash;
#endif
	//UNREFERENCED_PARAMETER(RecvNBL);

	do
	{
		if (!ft.initialized)
		{
			break;
		}

		PacketSize = NET_BUFFER_DATA_LENGTH(RecvNB);

		ethdr = (ETH_HEADER *)NdisGetDataBuffer(RecvNB,
			sizeof(ETH_HEADER)+sizeof(IP_HEADER)+sizeof(L4_HEADER),
			NULL, 1, 0);
		if (ethdr == NULL)
		{
			//DBGPRINT(MUX_WARN, ("[SswSNetProcessNetPacket] from intport %u, extport %u: first block len too small\n", inIntIf, inExtIf));
			ft.CountDiscardSmallPacket++;
			break;
		}

		/* discard broadcast and mcast ether packets */
		if (NdisEqualMemory(ethdr->Dest, EthBcastAddr, 6) ||
			NdisEqualMemory(ethdr->Dest, EthMcastAddr, 3))
		{
			ft.CountDiscardBMcastPacket++;
			break;
		}

		//if (!NdisEqualMemory(ethdr->Dest, FTDeviceAddress, 6))
		//{
		//	break;
		//}

		iphdr = (IP_HEADER *)(ethdr + 1);
		if (iphdr->protocol == IP_TCP_PROTOCOL ||
			iphdr->protocol == IP_UDP_PROTOCOL)
		{
			l4hdr = (L4_HEADER *)(iphdr + 1);
			flowkey.saddr = ntohl(iphdr->src);
			flowkey.daddr = ntohl(iphdr->dest);
			flowkey.protocol = iphdr->protocol;
			//flowkey.portnum = portnum;
			flowkey.sport = ntohs(l4hdr->sport);
			flowkey.dport = ntohs(l4hdr->dport);

#ifdef KERNEL_TABLE

			hash = (((flowkey.daddr & 0xff) << 24) +
				(((flowkey.daddr >> 8) & 0xff)  << 18) +
				(((flowkey.daddr >> 16) & 0xff)  << 10) +
				(((flowkey.daddr >> 24) & 0xff)  << 2) +
				flowkey.protocol * 256 ) % CONN_TABLE_SIZE;
			//hash = (flowkey.daddr + flowkey.protocol) % CONN_TABLE_SIZE;

			NdisDprAcquireSpinLock(&ConnHashTable[hash].Lock);
			pConn = FindConn(&ConnHashTable[hash], &flowkey);
			if (!pConn)
			{
				if (ft.CountActiveConn < ft.MaxActiveConn)
				{
					pConn = (PCONN)ExAllocatePool(NonPagedPool, sizeof(CONN));
					if (pConn)
					{
						NdisZeroMemory(pConn, sizeof(CONN));
						//pConn->key.saddr = flowkey.saddr;
						pConn->key.daddr = flowkey.daddr;
						//pConn->key.sport = flowkey.sport;
						//pConn->key.dport = flowkey.dport;
						pConn->key.protocol = flowkey.protocol;
						//pConn->key.portnum = flowkey.portnum;
						//pConn->lasttime = KeQueryInterruptTime();
						//pConn->lastactivetime = pConn->lasttime;
						//if (bft.kmacfwd)
						//{
						//	pConn->outExtIf = BftMacLookup(ethdr->Dest);
						//}
						pConn->PktSampleRate = ft.pkt_samp_default;
						pConn->FlowSampleRate = ft.flow_samp_default;


						InsertHeadList(&ConnHashTable[hash].ListHead, &pConn->ListEntry);
						InterlockedIncrement((PLONG)&ft.CountActiveConn);
						ft.CountTotalConn++;
						if (ft.CountActiveConn > ft.CountMaxActiveConn)
						{
							ft.CountMaxActiveConn = ft.CountActiveConn;
						}
						newflow = 1;
					}
					else
					{
						ft.CountAllocateConnFail++;
					}
				}
				else
				{
					ft.CountReachMaxActiveConn++;
				}
			}
			NdisDprReleaseSpinLock(&ConnHashTable[hash].Lock);

			if (pConn)
			{
				pConn->bytes += PacketSize;
				pConn->pkts++;
				
				
				//pakcet sampling
				//random = RtlRandom(&randomseed) % MAXSAMPLENUM;
			
				reason = FT_WITHOUT_SAMPLE;

				if (ft.enable_pkt_samp)
				{

					p = KeQueryPerformanceCounter(NULL);
					random = p.LowPart % MAXPKTSAMPLENUM;
					if (random < pConn->PktSampleRate)
						reason |= FT_PACKET_SAMPLE;
				}

				if (ft.enable_flow_samp)
				{
					flowkeyhash = (flowkey.saddr + flowkey.daddr +
						flowkey.sport + flowkey.dport +
						flowkey.protocol) % MAXFLOWSAMPLENUM;
					if (flowkeyhash < pConn->FlowSampleRate)
						reason |= FT_FLOW_SAMPLE;
				}

				if (reason != FT_WITHOUT_SAMPLE)
				{
					InterlockedIncrement((PLONG)&ft.CountSamplePacket);
					if (FTDeliverUserPacket(RecvNB, reason) == FT_SUCCESS)
					{
						InterlockedIncrement((PLONG)&ft.CountDeliverUserPacket);
					}
				}
			}
#else
			reason = FT_WITHOUT_SAMPLE;

			if (ft.enable_pkt_samp)
			{
				p = KeQueryPerformanceCounter(NULL);
				random = p.LowPart % MAXPKTSAMPLENUM;
				if (random < ft.pkt_samp_default)
					reason |= FT_PACKET_SAMPLE;
			}

			if (ft.enable_flow_samp)
			{
				flowkeyhash = (flowkey.saddr + flowkey.daddr +
					flowkey.sport + flowkey.dport +
					flowkey.protocol) % MAXFLOWSAMPLENUM;
				if (flowkeyhash < ft.flow_samp_default)
					reason |= FT_FLOW_SAMPLE;
			}

			if (reason != FT_WITHOUT_SAMPLE)
			{
				InterlockedIncrement((PLONG)&ft.CountSamplePacket);
				if (FTDeliverUserPacket(RecvNB, reason) == FT_SUCCESS)
				{
					InterlockedIncrement((PLONG)&ft.CountDeliverUserPacket);
				}
			}

#endif
			ft.CountFwdPackets++;
			ft.CountFwdBytes += PacketSize;

		}
	} while (bFalse);
}


VOID
FTFlush()
{
#ifdef KERNEL_TABLE
	INT                     hash;
	CONN_HASH_ENTRY        *pConnHash;
	PLIST_ENTRY             pEntry;
	PLIST_ENTRY             pDelEntry;
	CONN                   *pConn;


	for (hash = 0; hash < CONN_TABLE_SIZE; ++hash)
	{
		pConnHash = &ConnHashTable[hash];
		NdisDprAcquireSpinLock(&pConnHash->Lock);
		pEntry = pConnHash->ListHead.Flink;
		while (pEntry != &pConnHash->ListHead)
		{
			pConn = CONTAINING_RECORD(pEntry, CONN, ListEntry);
			/* check if the flow timeout? */
			pDelEntry = pEntry;
			pEntry = pEntry->Flink;
			RemoveEntryList(pDelEntry);
			//DBGPRINT(MUX_INFO, ("[SswSNetTimeout] flow expire %u.%u.%u.%u.%u -> %u.%u.%u.%u.%u(%u), bytes %I64d, pkts %u\n",
			//NIPQUAD_HOSTORDER(pConn->key.saddr), pConn->key.sport,
			//NIPQUAD_HOSTORDER(pConn->key.daddr), pConn->key.dport, pConn->key.protocol,
			//pConn->bytes, pConn->pkts));
			ExFreePool(pConn);
			InterlockedDecrement((PLONG)&ft.CountActiveConn);
		}

		NdisDprReleaseSpinLock(&ConnHashTable[hash].Lock);
	}

#endif

	NdisAcquireSpinLock(&GlobalVElan.UserRecvPacketTableSpinLock);
	GlobalVElan.UserRecvPacketTableHead = 0;
	GlobalVElan.UserRecvPacketTableTail = 0;
	NdisReleaseSpinLock(&GlobalVElan.UserRecvPacketTableSpinLock);
}


#ifdef KERNEL_TABLE

INT
AddDeliverFlow(CONN *pConn)
{
	int             rc = 0;
	FT_INFO_FLOW  *pflow;
	BOOLEAN bFalse = FALSE;
	do
	{

		NdisAcquireSpinLock(&FTFlowNotifyTable.SpinLock);

		if ((FTFlowNotifyTable.tail + 1) % MAX_FLOW_NOTIFY_TABLE == FTFlowNotifyTable.head)
		{
			//DBGPRINT(MUX_WARN, ("[AddDeliverFlow] BftFlowNotifyTable full, head=%d, tail=%d\n",
				//BftFlowNotifyTable.head, BftFlowNotifyTable.tail));
			ft.CountFlowNotifyTableFull++;

			NdisReleaseSpinLock(&FTFlowNotifyTable.SpinLock);
			break;
		}

		pflow = &FTFlowNotifyTable.flows[FTFlowNotifyTable.tail];
		FTFlowNotifyTable.tail++;
		if (FTFlowNotifyTable.tail == MAX_FLOW_NOTIFY_TABLE)
		{
			FTFlowNotifyTable.tail = 0;
		}
		NdisReleaseSpinLock(&FTFlowNotifyTable.SpinLock);

		//ft.CountTotalNotifyFlowNum[type]++;

		//pflow->dport = pConn->key.dport;
		//pflow->sport = pConn->key.sport;
		pflow->protocol = pConn->key.protocol;
		//pflow->srcip = pConn->key.saddr;
		pflow->dstip = pConn->key.daddr;
		pflow->totalbytes = (ULONG)pConn->bytes;
		pflow->totalpkts = pConn->pkts;
		pflow->pktsamplerate = pConn->PktSampleRate;
		pflow->flowsamplerate = pConn->FlowSampleRate;
		//pflow->portnum = pConn->key.portnum;
		//pflow->incbytes = pConn->incbytes;
		//pflow->incpkts = pConn->incpkts;
		//pflow->inctime = pConn->inctime;
		//pflow->type = type;

		rc = 1;
	} while (bFalse);

	return rc;
}


VOID
FTGettop()
{
	INT                     hash;
	CONN_HASH_ENTRY        *pConnHash;
	PLIST_ENTRY             pEntry;
	//PLIST_ENTRY             pDelEntry;
	CONN                   *pConn;
	int                     i;
	CONN                   *ConnTable[FLOW_NOTIFY_NUM];
	int                     nConn = 0;

	for (i = 0; i < FLOW_NOTIFY_NUM; ++i)
	{
		ConnTable[i] = NULL;
	}

	for (hash = 0; hash < CONN_TABLE_SIZE; ++hash)
	{
		pConnHash = &ConnHashTable[hash];
		NdisDprAcquireSpinLock(&pConnHash->Lock);
		pEntry = pConnHash->ListHead.Flink;
		while (pEntry != &pConnHash->ListHead)
		{
			pConn = CONTAINING_RECORD(pEntry, CONN, ListEntry);

			/* find the largest 256 flows, insert flows from the tail */
			for (i = nConn - 1; i >= 0; --i)
			{
				if (pConn->pkts > ConnTable[i]->pkts)
				{
					if (i + 1 < FLOW_NOTIFY_NUM)
					{
						/* move entry one-step to the tail */
						ConnTable[i + 1] = ConnTable[i];
					}
					else
					{
						/* remove the last entry */
						nConn--;
					}
				}
				else
				{
					break;
				}
			}
			if (i + 1 < FLOW_NOTIFY_NUM)
			{
				ConnTable[i + 1] = pConn;
				nConn++;
			}
		
			pEntry = pEntry->Flink;
		}
		NdisDprReleaseSpinLock(&ConnHashTable[hash].Lock);
	}

	if (nConn > 0)
	{
		for (i = 0; i < nConn; ++i)
		{
			AddDeliverFlow(ConnTable[i]);
		}
		//DeliverFlowSet();
	}
}

#endif
