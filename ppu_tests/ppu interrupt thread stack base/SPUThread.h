/* // SPU Channels
enum
{
	SPU_RdEventStat	 = 0,  // Read event status with mask applied
	SPU_WrEventMask	 = 1,  // Write event mask
	SPU_WrEventAck	  = 2,  // Write end of event processing
	SPU_RdSigNotify1	= 3,  // Signal notification 1
	SPU_RdSigNotify2	= 4,  // Signal notification 2
	SPU_WrDec		   = 7,  // Write decrementer count
	SPU_RdDec		   = 8,  // Read decrementer count
	SPU_RdEventMask	 = 11, // Read event mask
	SPU_RdMachStat	  = 13, // Read SPU run status
	SPU_WrSRR0		  = 14, // Write SPU machine state save/restore register 0 (SRR0)
	SPU_RdSRR0		  = 15, // Read SPU machine state save/restore register 0 (SRR0)
	SPU_WrOutMbox	   = 28, // Write outbound mailbox contents
	SPU_RdInMbox		= 29, // Read inbound mailbox contents
	SPU_WrOutIntrMbox   = 30, // Write outbound interrupt mailbox contents (interrupting PPU)
};

// MFC Channels
enum
{
	MFC_WrMSSyncReq	 = 9,  // Write multisource synchronization request
	MFC_RdTagMask	   = 12, // Read tag mask
	MFC_LSA			 = 16, // Write local memory address command parameter
	MFC_EAH			 = 17, // Write high order DMA effective address command parameter
	MFC_EAL			 = 18, // Write low order DMA effective address command parameter
	MFC_Size			= 19, // Write DMA transfer size command parameter
	MFC_TagID		   = 20, // Write tag identifier command parameter
	MFC_Cmd			 = 21, // Write and enqueue DMA command with associated class ID
	MFC_WrTagMask	   = 22, // Write tag mask
	MFC_WrTagUpdate	 = 23, // Write request for conditional or unconditional tag status update
	MFC_RdTagStat	   = 24, // Read tag status with mask applied
	MFC_RdListStallStat = 25, // Read DMA list stall-and-notify status
	MFC_WrListStallAck  = 26, // Write DMA list stall-and-notify acknowledge
	MFC_RdAtomicStat	= 27, // Read completion status of last completed immediate MFC atomic update command
};*/

// SPU Events
enum
{
	SPU_EVENT_MS = 0x1000, // Multisource Synchronization event
	SPU_EVENT_A  = 0x800,  // Privileged Attention event
	SPU_EVENT_LR = 0x400,  // Lock Line Reservation Lost event
	SPU_EVENT_S1 = 0x200,  // Signal Notification Register 1 available
	SPU_EVENT_S2 = 0x100,  // Signal Notification Register 2 available
	SPU_EVENT_LE = 0x80,   // SPU Outbound Mailbox available
	SPU_EVENT_ME = 0x40,   // SPU Outbound Interrupt Mailbox available
	SPU_EVENT_TM = 0x20,   // SPU Decrementer became negative (?)
	SPU_EVENT_MB = 0x10,   // SPU Inbound mailbox available
	SPU_EVENT_QV = 0x8,	// MFC SPU Command Queue available
	SPU_EVENT_SN = 0x2,	// MFC List Command stall-and-notify event
	SPU_EVENT_TG = 0x1,	// MFC Tag Group status update event

	SPU_EVENT_IMPLEMENTED  = SPU_EVENT_LR | SPU_EVENT_TM | SPU_EVENT_SN, // Mask of implemented events
	SPU_EVENT_INTR_IMPLEMENTED = SPU_EVENT_SN,

	SPU_EVENT_WAITING	  = 0x80000000, // Originally unused, set when SPU thread starts waiting on ch_event_stat
	//SPU_EVENT_AVAILABLE  = 0x40000000, // Originally unused, channel count of the SPU_RdEventStat channel
	//SPU_EVENT_INTR_ENABLED = 0x20000000, // Originally unused, represents "SPU Interrupts Enabled" status

	SPU_EVENT_INTR_TEST = SPU_EVENT_INTR_IMPLEMENTED
};

// SPU Class 0 Interrupts
enum
{
	SPU_INT0_STAT_DMA_ALIGNMENT_INT   = (1ull << 0),
	SPU_INT0_STAT_INVALID_DMA_CMD_INT = (1ull << 1),
	SPU_INT0_STAT_SPU_ERROR_INT	   = (1ull << 2),
};

// SPU Class 2 Interrupts
enum
{
	SPU_INT2_STAT_MAILBOX_INT				  = (1ull << 0),
	SPU_INT2_STAT_SPU_STOP_AND_SIGNAL_INT	  = (1ull << 1),
	SPU_INT2_STAT_SPU_HALT_OR_STEP_INT		 = (1ull << 2),
	SPU_INT2_STAT_DMA_TAG_GROUP_COMPLETION_INT = (1ull << 3),
	SPU_INT2_STAT_SPU_MAILBOX_THRESHOLD_INT	= (1ull << 4),
};

enum
{
	SPU_RUNCNTL_STOP_REQUEST = 0,
	SPU_RUNCNTL_RUN_REQUEST  = 1,
};

// SPU Status Register bits (not accurate)
enum
{
	SPU_STATUS_STOPPED			 = 0x0,
	SPU_STATUS_RUNNING			 = 0x1,
	SPU_STATUS_STOPPED_BY_STOP	 = 0x2,
	SPU_STATUS_STOPPED_BY_HALT	 = 0x4,
	SPU_STATUS_WAITING_FOR_CHANNEL = 0x8,
	SPU_STATUS_SINGLE_STEP		 = 0x10,
};

enum
{
	SYS_SPU_THREAD_BASE_LOW  = 0xf0000000,
	SYS_SPU_THREAD_OFFSET	= 0x100000,
	SYS_SPU_THREAD_SNR1	  = 0x5400c,
	SYS_SPU_THREAD_SNR2	  = 0x5C00c,
};

enum
{
	MFC_LSA_offs = 0x3004,
	MFC_EAH_offs = 0x3008,
	MFC_EAL_offs = 0x300C,
	MFC_Size_Tag_offs = 0x3010,
	MFC_Class_CMD_offs = 0x3014,
	MFC_CMDStatus_offs = 0x3014,
	MFC_QStatus_offs = 0x3104,
	Prxy_QueryType_offs = 0x3204,
	Prxy_QueryMask_offs = 0x321C,
	Prxy_TagStatus_offs = 0x322C,
	SPU_Out_MBox_offs = 0x4004,
	SPU_In_MBox_offs = 0x400C,
	SPU_MBox_Status_offs = 0x4014,
	SPU_RunCntl_offs = 0x401C,
	SPU_Status_offs = 0x4024,
	SPU_NPC_offs = 0x4034,
	SPU_RdSigNotify1_offs = 0x1400C,
	SPU_RdSigNotify2_offs = 0x1C00C,
};

/*enum
{
	RAW_SPU_BASE_ADDR   = 0xe0000000,
	RAW_SPU_OFFSET	  = 0x00100000,
	RAW_SPU_LS_OFFSET   = 0x00000000,
	RAW_SPU_PROB_OFFSET = 0x00040000,
};*/

enum FPSCR_EX
{
	//Single-precision exceptions
	FPSCR_SOVF = 1 << 2,	//Overflow
	FPSCR_SUNF = 1 << 1,	//Underflow
	FPSCR_SDIFF = 1 << 0,   //Different (could be IEEE non-compliant)
	//Double-precision exceptions
	FPSCR_DOVF = 1 << 13,   //Overflow
	FPSCR_DUNF = 1 << 12,   //Underflow
	FPSCR_DINX = 1 << 11,   //Inexact
	FPSCR_DINV = 1 << 10,   //Invalid operation
	FPSCR_DNAN = 1 << 9,	//NaN
	FPSCR_DDENORM = 1 << 8, //Denormal
};

