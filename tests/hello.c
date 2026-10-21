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
            return;
        }
    }
}

int kinput_read_char(char *character)
{
    HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
    DWORD bytes_read = 0;

    if (input == NULL || input == INVALID_HANDLE_VALUE)
    {
        return 0;
    }

    if (!ReadFile(input, character, 1, &bytes_read, NULL))
    {
        return 0;
    }

    return bytes_read == 1;
}


void kinput(char buffer[], int size)
{
    int i = 0;
    char character;
    DWORD bytes_read;
    HANDLE input = GetStdHandle(STD_INPUT_HANDLE);

    if (buffer == NULL || size <= 0)
    {
        return;
    }

    buffer[0] = '\0';

    if (input == NULL || input == INVALID_HANDLE_VALUE)
    {
        return;
    }

    while (i < size - 1)
    {
        bytes_read = 0;

        if (!ReadConsoleA(input, &character, 1, &bytes_read, NULL)
            || bytes_read == 0)
        {
            break;
        }

        if (character == '\r' || character == '\n')
        {
            break;
        }

        buffer[i++] = character;
    }

    buffer[i] = '\0';

    /* Consume the LF following CR, if one is waiting. */
    if (character == '\r')
    {
        DWORD available = 0;

        if (GetNumberOfConsoleInputEvents(input, &available)
            && available > 0)
        {
            /* No extra read needed: ReadConsoleA handles console input. */
        }
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
    struct MemoryBlock test;

    kprint("Running memory test...\n");

    test = kmalloc(16);

    if (test.address == NULL)
    {
        kprint("FAIL: allocation\n");
        return;
    }

    kprint("PASS: allocation\n");

    kfree(&test);

    kprint("PASS: free\n");

    test = kmalloc(16);

    if (test.address == NULL)
    {
        kprint("FAIL: reuse\n");
        return;
    }

    kprint("PASS: memory reuse\n");

    kfree(&test);

    kprint("Memory test complete.\n");
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
    if (strings_equal(command, "hello"))
    {
        command_hello();
        continue;
    }
    else if (strings_equal(command, "help"))
    {
      
        kprint("Commands:\n    hello: hi ig\n    help: the fuck you think it does?\n    ver: version T_T\n    advcom: Shows developer commands\n    literally anything else: no :3\n");
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
    else
    {
        kprint("Unknown command. Try one that is in the designated dictionary [help]\n");
    }
}
}
