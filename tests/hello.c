#include <stdio.h>
#include <stddef.h>
// robus 🤖
int number = 42;
int *pointer = &number;
char kedo_memory[1024];
int memory_used = 0;
struct MemoryBlock
{
    char *address;
    int size;
    int isFree;

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
void kinput(char buffer[], int size)
{
    int i = 0;
    int character;

    while (i < size - 1)
    {
        character = getchar();
if (character == EOF)
{
    return;
}
        if (character == '\n')
        {
            break;
        }

        buffer[i] = character;
        i++;
    }

    buffer[i] = '\0';
}

void kprint(char text[])
{
    printf("%s", text);
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

    printf("Memory pool: %zu bytes\n", sizeof(kedo_memory));
    printf("Memory used: %d bytes\n", memory_used);
    printf("Memory available: %d bytes\n", free_memory);
    printf("Active blocks: %d\n", active_blocks);
    printf("Free blocks: %d\n", free_blocks);
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
    }
    else if (strings_equal(command, "help"))
    {
      
        kprint("Commands:\n    hello: hi ig\n    help: the fuck you think it does?\n    ver: version T_T\n    advcom: Shows developer commands\n    literally anything else: no :3\n");
    }
        else if(strings_equal(command, "ver"))
        {
            kprint("kedo dos version 2.12132034+2.12132034i\n");
        }
            else if(strings_equal(command, "advcom")
            {
                kprint("Advanced commands:\n    fr: forces a return and ends the program\n    meminfo: displays memory information\n");
            }
                else if(strings_equal(command, "fr"))
                {
                    return 1;
                }
                    else if (strings_equal(command, "meminfo"))
{
    kmeminfo();
}
    else
    {
        kprint("Unknown command. Try one that is in the designated dictionary [help]\n");
    }
}
}
