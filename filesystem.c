/*
 * filesystem.c
 *
 *  Created on: Jun 15, 2025
 *      Author: Ivars Renge
 */


#include "filesystem.h"


#ifdef USING_FILESYSTEM
#include "string.h"
#include "stm32f3xx.h"



//static uint32_t fsCurrentWriteAddr = FS_START_ADDR;



static int fsChunkIsFree(FsChunk* chunk) {
    return chunk->type == FS_TYPE_FREE || chunk->type == 0xFF;
}

void fsInit() {

    printf("filesystem loaded\n");

#ifdef USING_CONSOLE
    consoleRegister("fsdebug", &fsDebugChunks);
    consoleRegister("fstest", &fsTest);
    consoleRegister("fsformat", &fsFormat);
    consoleRegister("filesystem", &fsPrintUsage);
    consoleRegister("ls", &fsList);
    consoleRegister("set", &fsSet);
    consoleRegister("get", &fsGet);
    consoleRegister("del", &fsDelete);
#endif
}


void fsTest(char* param) {
    const char* filename = "testfile";
    size_t size = CHUNK_PAYLOAD_SIZE * 3;
    char* testData = malloc(size);
    if (!testData) {
        setTextColor(RED);
        printf("Error: Memory allocation failed\n");
        setTextColor(DEFAULT_COLOR);
        return;
    }
    for (size_t i = 0; i < size; ++i) {
        testData[i] = 'A' + (i % 26);
    }

    // 1) Write
    if (fsWriteBinary(filename, testData, size) != 0) {
        setTextColor(RED);
        printf("Error: Write failed\n");
        setTextColor(DEFAULT_COLOR);
        free(testData);
        return;
    }

    // 2) Read & verify
    char* readBuf = malloc(size + 1);
    if (!readBuf) {
        setTextColor(RED);
        printf("Error: Memory allocation failed\n");
        setTextColor(DEFAULT_COLOR);
        free(testData);
        return;
    }
    if (fsRead(filename, readBuf, size + 1) != 0) {
        setTextColor(RED);
        printf("Error: Read failed\n");
        setTextColor(DEFAULT_COLOR);
        free(testData);
        free(readBuf);
        return;
    }
    if (memcmp(readBuf, testData, size) != 0) {
        setTextColor(RED);
        printf("Error: Data mismatch\n");
        setTextColor(DEFAULT_COLOR);
        free(testData);
        free(readBuf);
        return;
    }
    free(readBuf);

    // 3) Delete
    if (fsDelete(filename) != 0) {
        setTextColor(RED);
        printf("Error: Delete failed\n");
        setTextColor(DEFAULT_COLOR);
        free(testData);
        return;
    }

    // 4) Ensure it's gone
    if (fsFind(filename) != 0) {
        setTextColor(RED);
        printf("Error: File still exists after delete\n");
        setTextColor(DEFAULT_COLOR);
        free(testData);
        return;
    }

    free(testData);

    // Success
    setTextColor(GREEN);
    printf("All Tests passed\n");
    setTextColor(DEFAULT_COLOR);
}


/**
 * @brief  Erase entire Flash memory.
 * @note   All application code and data will be lost.
 */

uint8_t fsPageIsEmpty(uint32_t pageIndex) {
    uint32_t startAddr = FLASH_BASE + pageIndex * FLASH_PAGE_SIZE;
    uint32_t endAddr   = startAddr + FLASH_PAGE_SIZE;

    // scan 32-bit words
    for (uint32_t addr = startAddr; addr < endAddr; addr += sizeof(uint32_t)) {
        if (*(uint32_t *)addr != 0xFFFFFFFFU) {
            return 0;   // found programmed bit
        }
    }
    return 1;
}


uint8_t fsPrintUsage(char* param) {
    const uint32_t pageSize      = FLASH_PAGE_SIZE;
    const uint32_t chunkSize     = sizeof(FsChunk);
    const uint32_t chunksPerPage = pageSize / chunkSize;

    uint32_t addr = FS_START_ADDR;
    uint32_t end  = FS_END_ADDR;

    int totalPages   = 0;
    int emptyPages   = 0;
    int goodPages    = 0;  // >0 & <80% used
    int badPages     = 0;  // ≥80% used

    int fileCount    = 0;
    int totalChunks  = 0;
    int usedChunks   = 0;
    int freeChunks   = 0;

    FsChunk chunk;
    while (addr + pageSize <= end) {
        int usedInPage = 0;
        // count used vs free chunks in this page
        for (uint32_t off = 0; off < pageSize; off += chunkSize) {
            memcpy(&chunk, (void*)(addr + off), chunkSize);
            uint8_t t = chunk.type & 0x0F;
            if ((t == FS_TYPE_FILE_HEAD || t == FS_TYPE_FILE_DATA)
                && chunk.notDeleted == 0xFF)
            {
                usedInPage++;
                if (t == FS_TYPE_FILE_HEAD) {
                    fileCount++;
                }
            }
        }

        int freeInPage = chunksPerPage - usedInPage;
        totalPages++;
        totalChunks  += chunksPerPage;
        usedChunks   += usedInPage;
        freeChunks   += freeInPage;

        // classify and color
        if (usedInPage == 0) {
            emptyPages++;
            setBackgroundColor(GREEN);
        }
        else if (usedInPage * 100 >= chunksPerPage * 80) {
            badPages++;
            setBackgroundColor(RED);
        }
        else {
            goodPages++;
            setBackgroundColor(YELLOW);
        }
        putchar(' ');
        kernel_process(1);

        addr += pageSize;
    }

    setBackgroundColor(DEFAULT_COLOR);
    putchar('\n');

    // summary
    printf("Filesystem usage:\n");
    printf("  Total Pages    : %d\n", totalPages);
    printf("    Empty Pages  : %d\n", emptyPages);
    printf("    Good Pages   : %d\n", goodPages);
    printf("    Bad Pages    : %d\n", badPages);
    printf("  Page Size      : %u bytes\n", (unsigned)pageSize);
    printf("  Region         : 0x%08lX - 0x%08lX\n",
           (unsigned long)FS_START_ADDR,
           (unsigned long)FS_END_ADDR);
    printf("  Total Size     : %u KB\n",
           (unsigned)((uint32_t)totalPages * pageSize / 1024));
    printf("  Used           : %u KB\n",
           (unsigned)(usedChunks * chunkSize / 1024));
    printf("  Free           : %u KB\n",
           (unsigned)(freeChunks * chunkSize / 1024));
    printf("  Files          : %d\n", fileCount);
    printf("  Total Chunks   : %d\n", totalChunks);
    printf("    Used Chunks  : %d\n", usedChunks);
    printf("    Free Chunks  : %d\n", freeChunks);

    return 1;
}


void fsFormat(char* param)
{
    FLASH_EraseInitTypeDef eraseInit = {0};
    uint32_t pageError = 0;

    // Start at the first page at or after FS_START_ADDR
    uint32_t addr = FS_START_ADDR & ~(FLASH_PAGE_SIZE - 1);

    HAL_FLASH_Unlock();

    // Erase pages up through FS_END_ADDR
    while (addr + FLASH_PAGE_SIZE <= FS_END_ADDR) {
        eraseInit.TypeErase   = FLASH_TYPEERASE_PAGES;
        eraseInit.PageAddress = addr;
        eraseInit.NbPages     = 1;
    #if defined(FLASH_BANK_1)
        eraseInit.Banks       = FLASH_BANK_1;
    #endif

        if (HAL_FLASHEx_Erase(&eraseInit, &pageError) != HAL_OK) {
            printf("fsFormat: Erase failed at 0x%08lX (err page 0x%08lX)\n",
                   (unsigned long)addr, (unsigned long)pageError);
            break;
        }

        addr += FLASH_PAGE_SIZE;
    }

    HAL_FLASH_Lock();
}

// long execution time
void fsDebugChunks(char* params) {
	/*
    printf("Chunk Debug:\n");
    uint32_t addr = FS_START_ADDR;
    FsChunk chunk;

    while (addr + sizeof(FsChunk) <= FS_END_ADDR) {
        memcpy(&chunk, (void*)addr, sizeof(FsChunk));

        if (((chunk.type & 0x0F) == FS_TYPE_FILE_HEAD) || ((chunk.type & 0x0F) == FS_TYPE_FILE_DATA)) {
            printf("@0x%06lX  %-12s  type=0x%02X  link=0x%06X\n",
                addr,
                chunk.data,
                chunk.type,
                FS_GET_ADDR(chunk));
        }

        addr += sizeof(FsChunk);
        kernel_process(1); // TODO! depth
    }*/
}


int fsFind(char* name) {
    uint32_t addr = FS_START_ADDR;
    FsChunk chunk;

    while (addr + sizeof(FsChunk) <= FS_END_ADDR) {
        memcpy(&chunk, (void*)addr, sizeof(FsChunk));
        // Only match live file heads whose name equals `name`
        if ((chunk.type & 0x0F) == FS_TYPE_FILE_HEAD
            && chunk.notDeleted == 0xFF
            && strncmp((char*)chunk.data, name, FS_NAME_LEN) == 0)
        {
            return addr;
        }
        addr += sizeof(FsChunk);
        kernel_process(1);
    }
    return 0;
}


int32_t fsFindFreeChunk(void) {
    FsChunk chunk;
    const uint32_t base       = FS_START_ADDR;
    const uint32_t end        = FS_END_ADDR;
    const uint32_t chunkSize  = sizeof(FsChunk);

    // how many whole chunks fit in [base..end]
    const uint32_t totalBytes = end - base + 1U;
    const uint32_t nChunks    = totalBytes / chunkSize;

    // scan from chunk index nChunks-1 down to 0
    for (uint32_t i = nChunks; i > 0U; --i) {
        uint32_t addr = base + (i - 1U) * chunkSize;
        memcpy(&chunk, (void*)addr, chunkSize);
        if (fsChunkIsFree(&chunk)) {
            return (int32_t)addr;
        }
        kernel_process(1);  // give time to other tasks
    }

    return -1;
}

int fsDelete(char *name) {
    if (!name || !*name)
        return 1;  // invalid name

    uint32_t addr = fsFind(name);
    if (addr == FS_INVALID_ADDR)
        return 2;  // not found

    FsChunk chk;
    while (addr != FS_INVALID_ADDR) {
        memcpy(&chk, (void*)addr, sizeof(chk));


        // Calculate half-word address for notDeleted (offsets 2–3)
        uint32_t hw_addr = addr + offsetof(FsChunk, notDeleted);
        // Read existing half-word
        uint16_t existingHW = *(__IO uint16_t*)hw_addr;
        // Desired: keep high byte (reserved2), clear low byte (notDeleted)
        uint16_t desiredHW  = existingHW & 0xFF00;

        // Debug each half-word

        // Only allow 1→0 transitions
        if ((desiredHW & existingHW) != desiredHW)
            return 3;

        // Clear stale flags, unlock, wait for BUSY to clear
        FLASH->SR = FLASH_SR_PGERR | FLASH_SR_WRPERR | FLASH_SR_EOP;
        HAL_FLASH_Unlock();
        while (__HAL_FLASH_GET_FLAG(FLASH_FLAG_BSY)) {}

        // Program the half-word
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, hw_addr, desiredHW) != HAL_OK) {
            HAL_FLASH_Lock();
            return 4;  // flash program error
        }
        HAL_FLASH_Lock();

        // Move to the next chunk
        addr = (chk.link == FS_INVALID_ADDR) ? FS_INVALID_ADDR : chk.link;
    }

    return 0;
}




int fsWrite(char* name,  char* data) {
    if (!name || !data) return -1;
    return fsWriteBinary(name, data, strlen(data)+1);
}

// Helper: find the first free chunk, but skip a specific address
static int fsFindFreeChunkSkip(uint32_t skipAddr) {
    uint32_t addr = FS_START_ADDR;
    FsChunk chunk;
    while (addr + sizeof(FsChunk) <= FS_END_ADDR) {
        if (addr == skipAddr) {
            addr += sizeof(FsChunk);
            continue;
        }
        memcpy(&chunk, (void*)addr, sizeof(chunk));
        if (fsChunkIsFree(&chunk)) {
            return addr;
        }
        addr += sizeof(FsChunk);
    }
    return -1;
}


/* 1) Perform basic checks and delete existing file if present */
static int _WritePreChecks(const char *name, size_t size) {
    if (!name || !*name) {
        printf("ERR: invalid file name\n");
        return -1;
    }
    if (strlen(name) >= FS_NAME_LEN) {
        printf("ERR: name too long (max %d)\n", FS_NAME_LEN - 1);
        return -1;
    }
    if (size == 0 || size > FS_MAX_FILE_SIZE) {
        printf("ERR: invalid file size %zu (0 < size <= %d)\n", size, FS_MAX_FILE_SIZE);
        return -1;
    }
    /* Remove any existing file by the same name */
    int existing = fsFind(name);
    if (existing >= 0) {
        if (fsDelete(name) != 0) {
            printf("WARN: failed to delete existing '%s'\n", name);
        }
    }
    return 0;
}

// adjust how many bytes we write per chunk
#define CHUNK_PAYLOAD_SIZE  FS_PAYLOAD_SIZE

/* 2) Write header chunk (32B) half-word at a time */
static uint32_t _WriteHeaderChunk(const char *name, size_t size) {
    int32_t addr = fsFindFreeChunk();         // returns a 32B-aligned flash address
    if (addr < 0) { printf("ERR no free chunk\n"); return 0; }

    FsChunk hdr;
    memset(&hdr, 0xFF, sizeof(hdr));
    hdr.type = FS_TYPE_FILE_HEAD;
    // link left 0xFFFFFFFF until first data chunk
    strncpy((char*)hdr.data, name, FS_PAYLOAD_SIZE);

    HAL_FLASH_Unlock();
    uint16_t *words = (uint16_t*)&hdr;
    for (size_t w = 0; w < sizeof(hdr)/2; ++w) {
        uint32_t a = addr + 2*w;
        uint16_t ex = *(__IO uint16_t*)a, de = words[w];
        if ((de & ex) != de) { printf("ERR 0→1 at hw %zu\n", w);
                              HAL_FLASH_Lock(); return 0; }
        if (ex != de)
            if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, a, de) != HAL_OK) {
                printf("ERR write hw %zu\n", w);
                HAL_FLASH_Lock(); return 0;
            }
    }
    HAL_FLASH_Lock();
    return addr;
}

/* 3) Write a data chunk (32B) half-word at a time */
static uint32_t _WriteChunk(const void *buf, size_t len) {
    int32_t addr = fsFindFreeChunk();
    if (addr < 0) { printf("ERR no free data chunk\n"); return 0; }

    FsChunk c;
    memset(&c, 0xFF, sizeof(c));
    c.type = FS_TYPE_FILE_DATA;
    c.link = 0xFFFFFFFF;
    memcpy(c.data, buf, len);

    HAL_FLASH_Unlock();
    uint16_t *words = (uint16_t*)&c;
    for (size_t w = 0; w < sizeof(c)/2; ++w) {
        uint32_t a = addr + 2*w;
        uint16_t ex = *(__IO uint16_t*)a, de = words[w];
        if ((de & ex) != de) { printf("ERR 0→1 at hw %zu\n", w);
                              HAL_FLASH_Lock(); return 0; }
        if (ex != de)
            if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, a, de) != HAL_OK) {
                printf("ERR write hw %zu\n", w);
                HAL_FLASH_Lock(); return 0;
            }
    }
    HAL_FLASH_Lock();
    return addr;
}

/* 4) Mark header.link in one 32-bit write */
static int _MarkHeader(uint32_t hdr_addr, uint32_t next_addr) {
    // read & debug
    FsChunk tmp;
    memcpy(&tmp, (void*)hdr_addr, sizeof(tmp));

    uint32_t meta = hdr_addr + offsetof(FsChunk, link);
    uint32_t existing = *(__IO uint32_t*)meta;
    uint32_t desired  = (uint32_t)next_addr;

    if ((desired & existing) != desired) {
        printf("ERR: 0→1 bit detected\n");
        return -1;
    }

    HAL_FLASH_Unlock();
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, meta, desired) != HAL_OK) {
        printf("ERR: word write failed\n");
        HAL_FLASH_Lock();
        return -1;
    }
    HAL_FLASH_Lock();
    return 0;
}



int fsWriteBinary(char *name, char *data, int size) {
    if (_WritePreChecks(name, size) < 0) return 1;

    uint32_t header_addr = _WriteHeaderChunk(name, size);
    if (!header_addr) return 1;

    const uint8_t *ptr      = (const uint8_t*)data;
    size_t        remaining = size;
    uint32_t      prev_addr = 0;      // <-- track last‐written chunk

    while (remaining > 0) {
        size_t chunk_len = remaining < CHUNK_PAYLOAD_SIZE
                           ? remaining
                           : CHUNK_PAYLOAD_SIZE;
        uint32_t chunk_addr = _WriteChunk(ptr, chunk_len);
        if (!chunk_addr) return 1;

        if (prev_addr == 0) {
            // first data chunk: link it from the header
            if (_MarkHeader(header_addr, chunk_addr) < 0)
                return 1;
        } else {
            // subsequent chunks: link from the previous chunk
            if (_MarkHeader(prev_addr, chunk_addr) < 0)
                return 1;
        }

        prev_addr  = chunk_addr;
        ptr       += chunk_len;
        remaining -= chunk_len;
    }

    return 0;
}


int fsRead(char* name, char* buffer, int maxLen) {
    if (!name || !buffer || maxLen < 1)
        return -1;

    // 1) Find the header chunk
    uint32_t addr = fsFind(name);
    if (addr == FS_INVALID_ADDR)
        return -1;

    FsChunk chunk;
    memcpy(&chunk, (void*)addr, sizeof(chunk));

    // 2) Confirm it’s a file head
    if ((chunk.type & 0x0F) != FS_TYPE_FILE_HEAD)
        return -1;

    // 3) Walk data chunks, copying payload
    int idx      = 0;
    uint32_t next = chunk.link;
    while (next != FS_INVALID_ADDR && idx < maxLen - 1) {
        memcpy(&chunk, (void*)next, sizeof(chunk));
        if ((chunk.type & 0x0F) != FS_TYPE_FILE_DATA)
            break;

        int toCopy = CHUNK_PAYLOAD_SIZE;
        if (toCopy > maxLen - 1 - idx)
            toCopy = maxLen - 1 - idx;

        memcpy(buffer + idx, chunk.data, toCopy);
        idx += toCopy;
        next = chunk.link;
    }

    // 4) Null-terminate
    buffer[idx] = '\0';
    return 0;
}


int fsList(const char* unused) {
    printf("Flat file list:\n");
    uint32_t addr = FS_START_ADDR;
    FsChunk chunk;

    while (addr + sizeof(FsChunk) <= FS_END_ADDR) {
        memcpy(&chunk, (void*)addr, sizeof(chunk));
        // Only list live file headers
        if ((chunk.type & 0x0F) == FS_TYPE_FILE_HEAD
            && chunk.notDeleted == 0xFF)
        {
            // Print up to CHUNK_PAYLOAD_SIZE chars from data[]
            printf(" - %.*s  @0x%06X\n",
                   CHUNK_PAYLOAD_SIZE,
                   (char*)chunk.data,
                   (unsigned int)addr);
        }
        addr += sizeof(FsChunk);
        kernel_process(1);
    }
    return 1;
}


// Simplified fsGet: uses fsRead with a 256-byte buffer and prints the retrieved value
char* fsGet(char* name) {
    static char buffer[256];
    if (!name) return NULL;

    // Read up to 255 chars (plus null) from the file into buffer
    if (fsRead(name, buffer, sizeof(buffer)) != 0) {
        return NULL;
    }

#ifdef USING_CONSOLE
    // Print "name>value"
    printf("%s>%s\n", name, buffer);
#endif

    return buffer;
}


int fsSet(char* msg) {
    if (!msg) return -1;

    const char* equalSign = strchr(msg, '=');
    if (!equalSign || equalSign == msg || *(equalSign + 1) == '\0') {
        return -2; // malformed input
    }

    char name[FS_NAME_LEN + 1] = {0};
    size_t paramLen = equalSign - msg;
    if (paramLen > FS_NAME_LEN) paramLen = FS_NAME_LEN;

    strncpy(name, msg, paramLen);
    const char* value = equalSign + 1;

    return fsWrite(name, value);
}

#endif
