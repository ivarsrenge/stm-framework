/*
 * filesystem.h
 *
 *  Created on: Jun 15, 2025
 *      Author: Ivars
 *
 *  This filesystem tries to aproach
 *  1. Simplicity
 *  2. Reliability
 *  3. Low overhead
 *  4. Basic read/write/list/delete
 *  5. Background process that levels out flash usage
 *
 *  Mainly the filesystem is meant for simple configuration, but is not limited to one chunk.
 *
 *
[dir: settings]
  ├──> "wifi" -> file address
  ├──> "ledCfg" -> file address

[header: wifi]
  ├──> data1
  └──> data2

[header: ledCfg]
  └──> data


	fs_background_maint(); // slowly frees dead space
 *
 */

#ifndef SYS_FILESYSTEM_H_
#define SYS_FILESYSTEM_H_

	#include "main.h"

#ifdef USING_FILESYSTEM
	#include "core.h"
	#include <stdint.h>

	// --- Internal State ---
	#define FS_FLASH_KB           (*(volatile uint16_t*)FLASHSIZE_BASE)           // Flash size in KB from system memory
	#define FS_FLASH_SIZE		  (FS_FLASH_KB * 1024)
	#define FS_FLASH_END          (FLASH_BASE + FS_FLASH_SIZE)
	#define FS_PAGES			  (FS_FLASH_SIZE / FLASH_PAGE_SIZE)

	//#define FS_START_ADDR         (FLASH_BASE + 0x20000)          Determine dynamicaly
	extern uint32_t _etext;
	#define FS_START_ADDR  		 (((uint32_t)&_etext + FLASH_PAGE_SIZE - 1U) & ~(FLASH_PAGE_SIZE - 1U))
	#define FS_END_ADDR          (FS_FLASH_END)
	#define FS_START_PAGE_INDEX  ( (FS_START_ADDR - FLASH_BASE) / FLASH_PAGE_SIZE )
	#define CHUNK_PAYLOAD_SIZE sizeof(((FsChunk*)0)->data)


	#define FS_REL(addr) 		((addr) - FLASH_BASE)


	// --- Flash FS Constants ---
	#define CHUNK_SIZE       	32
	#define FS_PAYLOAD_SIZE  	24    // 32 − 4(link) − 4(type+reserved)
	#define FS_NAME_LEN         24      // max filename length
	#define FS_MAX_FILE_SIZE    1024    // max file size in bytes (adjustable)
	#define FS_INVALID_ADDR     0xFFFFFFFF



	// --- File Types ---
	#define FS_TYPE_FREE        0xF
	#define FS_TYPE_DEAD        0x0
	#define FS_TYPE_FILE_HEAD   0x1
	#define FS_TYPE_FILE_DATA   0x2




	typedef struct {
	    uint8_t   type;        // file head/data/etc.
	    uint8_t   reserved1;   // padding
	    uint8_t   notDeleted;  // 0xFF = alive, 0x00 = deleted
	    uint8_t   reserved2;   // padding
	    uint32_t  link;        // next-chunk address
	    uint8_t   data[FS_PAYLOAD_SIZE];
	} __attribute__((packed)) FsChunk;


	// --- Core API ---


	void fsInit();                                     // Initialize the filesystem
	void fsMaintain();                                 // Background defragmentation

	int fsDelete(char* name);
	int fsWriteBinary(char* name, char* data, int len);   // Create or overwrite file
	int fsWrite(char* name, char* data);   // Create or overwrite file
	int fsSet(char* msg);					   // wifi=100
	char* fsGet(char* name);
	int fsRead(char* name, char* buffer, int maxLen);     // Read file data
	int fsDelete(char* name);                             // Mark file as deleted
	int fsList();
	void fsFormat(char* param);
	void fsTest(char*);
	int fsFind(char* name);

	// --- Internal Access ---

	// --- Maintenance ---
	void fsFormat();                                   // Full flash format
	uint8_t fsPrintUsage(char*);                         // Dump free/used status
	void fsDebugChunks(char* params);					// debug chunks


#endif

#endif /* SYS_FILESYSTEM_H_ */
