#include <windows.h>
//NO STDIO.H!!!!!!!!! LET'S GO!
#include <stddef.h>
// robus 🤖
int number = 42;
int *pointer = &number;
char kedo_memory[1024];
int memory_used = 0;
struct KedoConfig
{
    int reboot_on_crash;
    int shutdown_confirm;
    int sound_enabled;
};
struct MemoryBlock
{
    char *address;
    int size;
    int isFree;

};
struct KedoConfig config =
{
    1,
    1,
    1
};

struct MemoryBlock blocks[32];
int block_count = 0;

void kcoalesce(void)
{
    int i = 0;

    while (i < block_count)
    {
        int j = i + 1;

        while (j < block_count)
        {
            if (!blocks[i].isFree ||
                !blocks[j].isFree)
            {
                j++;
                continue;
            }

            /* Are the two blocks adjacent? */
            if (blocks[i].address + blocks[i].size
                    == blocks[j].address ||
                blocks[j].address + blocks[j].size
                    == blocks[i].address)
            {
                /* Keep the lower memory address. */
                if (blocks[j].address < blocks[i].address)
                {
                    blocks[i].address = blocks[j].address;
                }

                /* Combine both block sizes. */
                blocks[i].size += blocks[j].size;

                /* Remove block j from the block table. */
                for (int k = j; k < block_count - 1; k++)
                {
                    blocks[k] = blocks[k + 1];
                }

                block_count--;

                /* Check this block against the next entry. */
                continue;
            }

            j++;
        }

        i++;
    }
}

struct MemoryBlock kmalloc(int size)
{
    struct MemoryBlock invalid = {NULL, 0, 1};

    if (size <= 0)
    {
        return invalid;
    }

    
    // First, try to reuse freed memory.
    for (int i = 0; i < block_count; i++)
    {
        if (blocks[i].isFree && blocks[i].size >= size)
        {
            // Split the block if there is unused space.
            if (blocks[i].size > size && block_count < 32)
            {
                struct MemoryBlock remainder;

                remainder.address = blocks[i].address + size;
                remainder.size = blocks[i].size - size;
                remainder.isFree = 1;

                // Register the unused portion.
                blocks[block_count] = remainder;
                block_count++;

                // Keep only the requested size in this block.
                blocks[i].size = size;
            }

            // Mark the requested block as occupied.
            blocks[i].isFree = 0;

            return blocks[i];
        }
    }

    // Do not exceed the block table.
    if (block_count >= 32)
    {
        return invalid;
    }

    // Do not exceed the memory pool.
    if (size > (int)sizeof(kedo_memory) - memory_used)
    {
        return invalid;
    }

    struct MemoryBlock block;

    block.address = &kedo_memory[memory_used];
    block.size = size;
    block.isFree = 0;

    memory_used += size;

    blocks[block_count] = block;
    block_count++;

    return block;
}

void kfree(struct MemoryBlock *block)
{
    if (block == NULL || block->address == NULL)
    {
        return;
    }

    for (int i = 0; i < block_count; i++)
    {
        if (blocks[i].address == block->address &&
            blocks[i].size == block->size &&
            !blocks[i].isFree)
        {
            blocks[i].isFree = 1;
kcoalesce();
return;
        }
    }
}


void kinput(char buffer[], int size)
{
    HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
    DWORD bytes_read = 0;

    if (buffer == NULL || size <= 0)
    {
        return;
    }

    buffer[0] = '\0';

    if (input == NULL || input == INVALID_HANDLE_VALUE)
    {
        return;
    }

    if (!ReadConsoleA(
            input,
            buffer,
            (DWORD)(size - 1),
            &bytes_read,
            NULL))
    {
        return;
    }

    buffer[bytes_read] = '\0';

    /* Remove the line ending. */
    while (bytes_read > 0 &&
           (buffer[bytes_read - 1] == '\r' ||
            buffer[bytes_read - 1] == '\n'))
    {
        buffer[--bytes_read] = '\0';
    }
}

int koutput_write(const char *text)
{
    if (text == NULL)
    {
        return 1;
    }

    HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);

    if (output == NULL || output == INVALID_HANDLE_VALUE)
    {
        return 1;
    }

    size_t length = 0;

    while (text[length] != '\0')
    {
        length++;
    }

    size_t offset = 0;

    while (offset < length)
    {
        size_t remaining = length - offset;

        DWORD chunk = remaining > (size_t)MAXDWORD
                    ? MAXDWORD
                    : (DWORD)remaining;

        DWORD total_written = 0;

        while (total_written < chunk)
        {
            DWORD written = 0;

            if (!WriteFile(
                    output,
                    text + offset + total_written,
                    chunk - total_written,
                    &written,
                    NULL))
            {
                return 1;
            }

            if (written == 0)
            {
                return 1;
            }

            total_written += written;
        }

        offset += chunk;
    }

    return 0;
}



void kprint(const char *text)
{
    (void)koutput_write(text);
}

int starts_with(const char text[], const char prefix[])
{
    int i = 0;

    while (prefix[i] != '\0')
    {
        if (text[i] != prefix[i])
        {
            return 0;
        }

        i++;
    }

    return 1;
}

void command_write(char args[])
{
    char *filename = args;
    char *content;
    DWORD written = 0;
    DWORD length = 0;

    /* Find the filename/content separator. */
    while (*args != '\0' && *args != ' ')
    {
        args++;
    }

    if (*args == '\0')
    {
        kprint("Usage: write <filename> <text>\n");
        return;
    }

    *args = '\0';
    content = args + 1;

    while (*content == ' ')
    {
        content++;
    }

    if (*filename == '\0' || *content == '\0')
    {
        kprint("Usage: write <filename> <text>\n");
        return;
    }

    while (content[length] != '\0')
    {
        length++;
    }

    HANDLE file = CreateFileA(
        filename,
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (file == INVALID_HANDLE_VALUE)
    {
        kprint("Error: could not create file.\n");
        return;
    }

    BOOL success = WriteFile(
        file,
        content,
        length,
        &written,
        NULL
    );

    CloseHandle(file);

    if (!success || written != length)
    {
        kprint("Error: file write failed.\n");
        return;
    }

    kprint("File written successfully.\n");
}

void command_read(char filename[])
{
    char buffer[1024];
    DWORD bytes_read = 0;

    if (*filename == '\0')
    {
        kprint("Usage: read <filename>\n");
        return;
    }

    HANDLE file = CreateFileA(
        filename,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (file == INVALID_HANDLE_VALUE)
    {
        kprint("Error: could not open file.\n");
        return;
    }

    BOOL success = ReadFile(
        file,
        buffer,
        sizeof(buffer) - 1,
        &bytes_read,
        NULL
    );

    CloseHandle(file);

    if (!success)
    {
        kprint("Error: file read failed.\n");
        return;
    }

    buffer[bytes_read] = '\0';

    kprint(buffer);

    if (bytes_read == 0 ||
        buffer[bytes_read - 1] != '\n')
    {
        kprint("\n");
    }

    if (bytes_read == sizeof(buffer) - 1)
    {
        kprint("[Output limited to 1023 bytes.]\n");
    }
}

void command_dir(void)
{
    WIN32_FIND_DATAA data;
    HANDLE search = FindFirstFileA("*", &data);
    int found = 0;

    if (search == INVALID_HANDLE_VALUE)
    {
        kprint("Error: could not list files.\n");
        return;
    }

    kprint("Directory listing:\n");

    do
    {
        if (strings_equal(data.cFileName, ".") ||
            strings_equal(data.cFileName, ".."))
        {
            continue;
        }

        kprint("  ");
        kprint(data.cFileName);

        if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            kprint("/");
        }

        kprint("\n");
        found = 1;
    }
    while (FindNextFileA(search, &data));

    FindClose(search);

    if (!found)
    {
        kprint("  (empty)\n");
    }
}

void command_del(char filename[])
{
    if (*filename == '\0')
    {
        kprint("Usage: del <filename>\n");
        return;
    }

    /* Keep this command from deleting directories. */
    DWORD attributes = GetFileAttributesA(filename);

    if (attributes == INVALID_FILE_ATTRIBUTES)
    {
        kprint("Error: file not found.\n");
        return;
    }

    if (attributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        kprint("Error: del only deletes files.\n");
        return;
    }

    if (!DeleteFileA(filename))
    {
        kprint("Error: could not delete file.\n");
        return;
    }

    kprint("File deleted successfully.\n");
}

void kpanic(char error[])
{
    kprint("\n");
    kprint("KEDO KERNEL PANIC\n");
    kprint("Error: ");
    kprint(error);
    kprint("\n");
    kprint("System halted.\n");

    while (1)
    {
    }
}
void command_hello(void)
{
    kprint("please use a useful command\n");
}
int strings_equal(char a[], char b[])
{
    int i = 0;

    while (a[i] != '\0' && b[i] != '\0')
    {
        if (a[i] != b[i])
        {
            return 0;
        }

        i++;
    }

    return a[i] == b[i];
}

void kprint_number(int number)
{
    char buffer[12];
    int i = 0;
    unsigned int value;

    if (number < 0)
    {
        kprint("-");
        value = 0u - (unsigned int)number;
    }
    else
    {
        value = (unsigned int)number;
    }

    do
    {
        buffer[i] = '0' + (value % 10);
        value /= 10;
        i++;
    }
    while (value > 0);

    while (i > 0)
    {
        char character[2];

        i--;
        character[0] = buffer[i];
        character[1] = '\0';

        kprint(character);
    }
}

void kmeminfo(void)
{
    int free_memory = sizeof(kedo_memory) - memory_used;
    int active_blocks = 0;
    int free_blocks = 0;

    for (int i = 0; i < block_count; i++)
    {
        if (blocks[i].isFree)
        {
            free_blocks++;
        }
        else
        {
            active_blocks++;
        }
    }

    kprint("Memory pool: ");
    kprint_number(sizeof(kedo_memory));
    kprint(" bytes\n");

    kprint("Memory used: ");
    kprint_number(memory_used);
    kprint(" bytes\n");

    kprint("Memory available: ");
    kprint_number(free_memory);
    kprint(" bytes\n");

    kprint("Active blocks: ");
    kprint_number(active_blocks);
    kprint("\n");

    kprint("Free blocks: ");
    kprint_number(free_blocks);
    kprint("\n");
}

void kmemtest(void)
{
    struct MemoryBlock first;
    struct MemoryBlock second;
    struct MemoryBlock merged;
    
    kprint("Running memory test...\n");

    /* Test 1: Allocate 16 bytes. */
    first = kmalloc(16);

    if (first.address == NULL)
    {
        kprint("FAIL: allocation\n");
        return;
    }

    kprint("PASS: allocation\n");

    /* Test 2: Free the block. */
    kfree(&first);
    kprint("PASS: free\n");

    /* Test 3: Reuse the freed 16-byte block. */
    second = kmalloc(16);

    if (second.address == NULL ||
        second.address != first.address)
    {
        kprint("FAIL: memory reuse\n");

        if (second.address != NULL)
            kfree(&second);

        return;
    }

    kprint("PASS: memory reuse\n");

    /* Free it again so the next allocation can split it. */
    kfree(&second);

    /* Test 4: Allocate only 6 bytes from that 16-byte block. */
    first = kmalloc(6);

    if (first.address == NULL)
    {
        kprint("FAIL: split allocation\n");
        return;
    }

    if (first.size != 6)
    {
        kprint("FAIL: incorrect allocated block size\n");
        kfree(&first);
        return;
    }

    kprint("PASS: split allocation\n");

    /* Test 5: Check that a 10-byte remainder exists. */
    second = kmalloc(10);

    if (second.address == NULL ||
        second.address != first.address + 6)
    {
        kprint("FAIL: remainder reuse\n");
        kfree(&first);

        if (second.address != NULL)
            kfree(&second);

        return;
    }

    kprint("PASS: remainder reuse\n");
    
    /* Test 6: Free both adjacent blocks. */
    kfree(&first);
    kfree(&second);

    kprint("PASS: free split blocks\n");

    /* Test 7: The two blocks should now be merged. */
    merged = kmalloc(16);

    if (merged.address == NULL ||
        merged.address != first.address ||
        merged.size != 16)
    {
        kprint("FAIL: block coalescing\n");

        if (merged.address != NULL)
            kfree(&merged);

        return;
    }

    kprint("PASS: block coalescing\n");

    kfree(&merged);

    kprint("Memory test complete.\n");

    kfree(&first);
    kfree(&second);

   return;
}
char command[100];
char *command_pointer = command;
/********
    FUCKING MAIN BITCH
********/
int main(void)
{

kprint("\n");  

while (1)
{
    kprint("KEDO> ");
    kinput(command, sizeof(command));
    

if (command[0] == '\0')
{
    continue;
}
    if (strings_equal(command, "hello"))
    {
        command_hello();
        continue;
    }
    else if (strings_equal(command, "help"))
    {
      
        kprint("Commands:\n    hello: hi ig\n    help: the fuck you think it does?\n    ver: version T_T\n    advcom: Shows developer commands\n    write: makes a file\n    read: reads a fil- BITCH TF U THINK TS DOES!?\n    dir: shows all files\n    del: deletes a file\n");
    continue;
    }
        else if(strings_equal(command, "ver"))
        {
            kprint("kedo dos version 2.12132034+2.12132034i\n");
            continue;
        }
            else if(strings_equal(command, "advcom"))
            {
                kprint("Advanced commands:\n    fr: forces a return and ends the program\n    meminfo: displays memory information\n    memtest: performs a memory test\n    panic [SYS]: causes a panic\n    lcon: displays current system configuration\n");
            continue;
            }
                else if(strings_equal(command, "fr"))
                {
                    return 1;
                }
                    else if (strings_equal(command, "meminfo"))
{
    kmeminfo();
    continue;
}
                        else if(strings_equal(command, "memtest"))
                        {
                            kmemtest();
                            continue;
                        }
                            else if (strings_equal(command, "sys panic -bcd"))
{
    kpanic("BCD - Bad Configuration Data");
    continue;
}
                                else if(strings_equal(command, "lcon"))
                                {
                                    kprint("here, take a \\n.\n");
                                    continue;
                                }
                                    
    else if (starts_with(command, "write "))
    {
        command_write(command + 6);
        continue;
    }
    else if (starts_with(command, "read "))
    {
        command_read(command + 5);
        continue;
    }
        
    else if (strings_equal(command, "dir"))
    {
        command_dir();
        continue;
    }
    else if (starts_with(command, "del "))
    {
        command_del(command + 4);
        continue;
    }


    else
    {
        kprint("Unknown command. Try one that is in the designated dictionary [help]\n");
    }
}
}
