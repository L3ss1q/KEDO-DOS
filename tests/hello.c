
#include <windows.h>
// NO STDIO.H!!!!!!!!!! LET'S GO!
#include <stddef.h>

/*
    KEDO DOS
    Codename: Sofya Sobric

    Hosted development build.
    File commands use a custom KEDO disk image.
*/

/* =========================================================
   CONFIGURATION
   ========================================================= */

#define KEDO_MEMORY_SIZE 1024
#define KEDO_MAX_BLOCKS 32

#define KEDO_FILE_COUNT 16
#define KEDO_NAME_SIZE 32
#define KEDO_FILE_DATA_SIZE 1024

#define KEDO_DISK_FILENAME "kedo.disk"

/* =========================================================
   GENERAL VARIABLES
   ========================================================= */

int number = 42;
int *pointer = &number;

char kedo_memory[KEDO_MEMORY_SIZE];
int memory_used = 0;

char command[100];

/* =========================================================
   CONFIGURATION STRUCTURE
   ========================================================= */

struct KedoConfig
{
    int reboot_on_crash;
    int shutdown_confirm;
    int sound_enabled;
};

struct KedoConfig config =
{
    1,
    1,
    1
};

/* =========================================================
   MEMORY MANAGEMENT
   ========================================================= */

struct MemoryBlock
{
    char *address;
    int size;
    int isFree;
};

struct MemoryBlock blocks[KEDO_MAX_BLOCKS];
int block_count = 0;

/* Function declarations */

int strings_equal(char a[], char b[]);
int starts_with(const char text[], const char prefix[]);
void kcoalesce(void);

/* =========================================================
   MEMORY COALESCING
   ========================================================= */

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

            if (blocks[i].address + blocks[i].size
                    == blocks[j].address ||
                blocks[j].address + blocks[j].size
                    == blocks[i].address)
            {
                if (blocks[j].address < blocks[i].address)
                {
                    blocks[i].address = blocks[j].address;
                }

                blocks[i].size += blocks[j].size;

                for (int k = j; k < block_count - 1; k++)
                {
                    blocks[k] = blocks[k + 1];
                }

                block_count--;

                continue;
            }

            j++;
        }

        i++;
    }
}

/* =========================================================
   MEMORY ALLOCATION
   ========================================================= */

struct MemoryBlock kmalloc(int size)
{
    struct MemoryBlock invalid = {NULL, 0, 1};

    if (size <= 0)
    {
        return invalid;
    }

    /* Reuse a free block when possible. */
    for (int i = 0; i < block_count; i++)
    {
        if (blocks[i].isFree && blocks[i].size >= size)
        {
            /*
                Split the block if the metadata table
                has room for another block.
            */

            if (blocks[i].size > size &&
                block_count < KEDO_MAX_BLOCKS)
            {
                struct MemoryBlock remainder;

                remainder.address = blocks[i].address + size;
                remainder.size = blocks[i].size - size;
                remainder.isFree = 1;

                blocks[block_count] = remainder;
                block_count++;

                blocks[i].size = size;
            }

            blocks[i].isFree = 0;

            return blocks[i];
        }
    }

    /* Check the metadata table limit. */
    if (block_count >= KEDO_MAX_BLOCKS)
    {
        return invalid;
    }

    /* Check the memory pool limit. */
    if (size > KEDO_MEMORY_SIZE - memory_used)
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

/* =========================================================
   MEMORY FREEING
   ========================================================= */

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

/* =========================================================
   CONSOLE INPUT
   ========================================================= */

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

    while (bytes_read > 0 &&
           (buffer[bytes_read - 1] == '\r' ||
            buffer[bytes_read - 1] == '\n'))
    {
        buffer[--bytes_read] = '\0';
    }
}

/* =========================================================
   CONSOLE OUTPUT
   ========================================================= */

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

/* =========================================================
   BASIC COMMANDS
   ========================================================= */

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

/* =========================================================
   NUMBER OUTPUT
   ========================================================= */

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

/* =========================================================
   MEMORY INFORMATION
   ========================================================= */

void kmeminfo(void)
{
    int free_memory = KEDO_MEMORY_SIZE - memory_used;
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
    kprint_number(KEDO_MEMORY_SIZE);
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

/* =========================================================
   MEMORY TESTS
   ========================================================= */

void kmemtest(void)
{
    struct MemoryBlock first;
    struct MemoryBlock second;
    struct MemoryBlock merged;

    kprint("Running memory test...\n");

    /* Test 1: Allocation. */

    first = kmalloc(16);

    if (first.address == NULL)
    {
        kprint("FAIL: allocation\n");
        return;
    }

    kprint("PASS: allocation\n");

    /* Test 2: Freeing. */

    kfree(&first);

    kprint("PASS: free\n");

    /* Test 3: Reuse. */

    second = kmalloc(16);

    if (second.address == NULL ||
        second.address != first.address)
    {
        kprint("FAIL: memory reuse\n");

        if (second.address != NULL)
        {
            kfree(&second);
        }

        return;
    }

    kprint("PASS: memory reuse\n");

    /* Free the block for the splitting test. */

    kfree(&second);

    /* Test 4: Splitting. */

    first = kmalloc(6);

    if (first.address == NULL || first.size != 6)
    {
        kprint("FAIL: split allocation");

        if (first.address != NULL)
        {
            kfree(&first);
        }

        kprint("\n");
        return;
    }

    kprint("PASS: split allocation\n");

    /* Test 5: Reusing the remainder. */

    second = kmalloc(10);

    if (second.address == NULL ||
        second.address != first.address + 6 ||
        second.size != 10)
    {
        kprint("FAIL: remainder reuse\n");

        kfree(&first);

        if (second.address != NULL)
        {
            kfree(&second);
        }

        return;
    }

    kprint("PASS: remainder reuse\n");

    /*
        Freeing both adjacent blocks should coalesce
        them into one 16-byte block.
    */

    kfree(&first);
    kfree(&second);

    kprint("PASS: free split blocks\n");

    /* Test 6: Coalescing. */

    merged = kmalloc(16);

    if (merged.address == NULL ||
        merged.address != first.address ||
        merged.size != 16)
    {
        kprint("FAIL: block coalescing\n");

        if (merged.address != NULL)
        {
            kfree(&merged);
        }

        return;
    }

    kprint("PASS: block coalescing\n");

    kfree(&merged);

    kprint("Memory test complete.\n");
}

/* =========================================================
   KERNEL PANIC
   ========================================================= */

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

/* =========================================================
   KEDO DISK FORMAT
   ========================================================= */

struct KedoDiskFile
{
    unsigned int used;
    char name[KEDO_NAME_SIZE];
    unsigned int size;
    char data[KEDO_FILE_DATA_SIZE];
};

struct KedoDisk
{
    char magic[8];
    unsigned int version;
    struct KedoDiskFile files[KEDO_FILE_COUNT];
};

/* =========================================================
   DISK INITIALIZATION AND LOADING
   ========================================================= */

int kedo_disk_load(struct KedoDisk *disk)
{
    HANDLE file;
    LARGE_INTEGER file_size;
    DWORD bytes_read = 0;

    file = CreateFileA(
        KEDO_DISK_FILENAME,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ,
        NULL,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (file == INVALID_HANDLE_VALUE)
    {
        return 0;
    }

    if (!GetFileSizeEx(file, &file_size))
    {
        CloseHandle(file);
        return 0;
    }

    /* Initialize a new disk image if empty. */

    if (file_size.QuadPart == 0)
    {
        ZeroMemory(disk, sizeof(*disk));

        disk->magic[0] = 'K';
        disk->magic[1] = 'E';
        disk->magic[2] = 'D';
        disk->magic[3] = 'O';
        disk->magic[4] = 'D';
        disk->magic[5] = 'S';
        disk->magic[6] = 'K';
        disk->magic[7] = '1';

        disk->version = 1;

        DWORD bytes_written = 0;

        BOOL success = WriteFile(
            file,
            disk,
            (DWORD)sizeof(*disk),
            &bytes_written,
            NULL
        );

        CloseHandle(file);

        return success &&
               bytes_written == sizeof(*disk);
    }

    /* Refuse unexpected image sizes. */

    if (file_size.QuadPart != sizeof(*disk))
    {
        CloseHandle(file);
        return 0;
    }

    BOOL success = ReadFile(
        file,
        disk,
        (DWORD)sizeof(*disk),
        &bytes_read,
        NULL
    );

    CloseHandle(file);

    if (!success || bytes_read != sizeof(*disk))
    {
        return 0;
    }

    /* Verify the disk signature. */

    if (disk->magic[0] != 'K' ||
        disk->magic[1] != 'E' ||
        disk->magic[2] != 'D' ||
        disk->magic[3] != 'O' ||
        disk->magic[4] != 'D' ||
        disk->magic[5] != 'S' ||
        disk->magic[6] != 'K' ||
        disk->magic[7] != '1' ||
        disk->version != 1)
    {
        return 0;
    }

    /* Validate file metadata before using it. */

    for (int i = 0; i < KEDO_FILE_COUNT; i++)
    {
        if (disk->files[i].used > 1)
        {
            return 0;
        }

        if (disk->files[i].used)
        {
            int name_terminated = 0;

            for (int j = 0; j < KEDO_NAME_SIZE; j++)
            {
                if (disk->files[i].name[j] == '\0')
                {
                    name_terminated = 1;
                    break;
                }
            }

            if (!name_terminated ||
                disk->files[i].name[0] == '\0' ||
                disk->files[i].size >= KEDO_FILE_DATA_SIZE ||
                disk->files[i].data[disk->files[i].size] != '\0')
            {
                return 0;
            }
        }
    }

    return 1;
}

/* =========================================================
   DISK SAVING
   ========================================================= */

int kedo_disk_save(struct KedoDisk *disk)
{
    HANDLE file;
    DWORD bytes_written = 0;

    file = CreateFileA(
        KEDO_DISK_FILENAME,
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (file == INVALID_HANDLE_VALUE)
    {
        return 0;
    }

    BOOL success = WriteFile(
        file,
        disk,
        (DWORD)sizeof(*disk),
        &bytes_written,
        NULL
    );

    CloseHandle(file);

    return success &&
           bytes_written == sizeof(*disk);
}

/* =========================================================
   FILENAME VALIDATION
   ========================================================= */

int kedo_valid_filename(const char name[])
{
    int length = 0;

    if (name == NULL || name[0] == '\0')
    {
        return 0;
    }

    while (name[length] != '\0')
    {
        if (name[length] == '/' ||
            name[length] == '\\' ||
            name[length] == ':')
        {
            return 0;
        }

        length++;
    }

    if (length >= KEDO_NAME_SIZE ||
        strings_equal((char *)name, ".") ||
        strings_equal((char *)name, ".."))
    {
        return 0;
    }

    return 1;
}

/* =========================================================
   WRITE COMMAND
   ========================================================= */

void command_write(char args[])
{
    struct KedoDisk disk;

    char *filename = args;
    char *content;

    int length = 0;
    int slot = -1;

    /* Split filename and content. */

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

    if (!kedo_valid_filename(filename) ||
        *content == '\0')
    {
        kprint("Invalid filename or empty content.\n");
        return;
    }

    while (content[length] != '\0')
    {
        length++;
    }

    if (length >= KEDO_FILE_DATA_SIZE)
    {
        kprint("Error: file text is too large.\n");
        return;
    }

    if (!kedo_disk_load(&disk))
    {
        kprint("Error: could not load KEDO disk.\n");
        return;
    }

    /* Find an existing file or an empty slot. */

    for (int i = 0; i < KEDO_FILE_COUNT; i++)
    {
        if (disk.files[i].used &&
            strings_equal(disk.files[i].name, filename))
        {
            slot = i;
            break;
        }

        if (!disk.files[i].used && slot == -1)
        {
            slot = i;
        }
    }

    if (slot == -1)
    {
        kprint("Error: disk is full.\n");
        return;
    }

    disk.files[slot].used = 1;

    /* Clear the filename. */

    for (int i = 0; i < KEDO_NAME_SIZE; i++)
    {
        disk.files[slot].name[i] = '\0';
    }

    /* Copy the filename. */

    for (int i = 0; filename[i] != '\0'; i++)
    {
        disk.files[slot].name[i] = filename[i];
    }

    /* Clear the contents. */

    for (int i = 0; i < KEDO_FILE_DATA_SIZE; i++)
    {
        disk.files[slot].data[i] = '\0';
    }

    /* Copy the file contents. */

    for (int i = 0; i < length; i++)
    {
        disk.files[slot].data[i] = content[i];
    }

    disk.files[slot].size = length;

    if (!kedo_disk_save(&disk))
    {
        kprint("Error: could not save KEDO disk.\n");
        return;
    }

    kprint("File written to KEDO disk.\n");
}

/* =========================================================
   READ COMMAND
   ========================================================= */

void command_read(char filename[])
{
    struct KedoDisk disk;

    if (!kedo_valid_filename(filename))
    {
        kprint("Usage: read <filename>\n");
        return;
    }

    if (!kedo_disk_load(&disk))
    {
        kprint("Error: could not load KEDO disk.\n");
        return;
    }

    for (int i = 0; i < KEDO_FILE_COUNT; i++)
    {
        if (disk.files[i].used &&
            strings_equal(disk.files[i].name, filename))
        {
            kprint(disk.files[i].data);
            kprint("\n");
            return;
        }
    }

    kprint("Error: file not found on KEDO disk.\n");
}

/* =========================================================
   DIRECTORY COMMAND
   ========================================================= */

void command_dir(void)
{
    struct KedoDisk disk;
    int found = 0;

    if (!kedo_disk_load(&disk))
    {
        kprint("Error: could not load KEDO disk.\n");
        return;
    }

    kprint("KEDO disk directory:\n");

    for (int i = 0; i < KEDO_FILE_COUNT; i++)
    {
        if (disk.files[i].used)
        {
            kprint("  ");
            kprint(disk.files[i].name);
            kprint("\n");

            found = 1;
        }
    }

    if (!found)
    {
        kprint("  (empty)\n");
    }
}

/* =========================================================
   DELETE COMMAND
   ========================================================= */

void command_del(char filename[])
{
    struct KedoDisk disk;

    if (!kedo_valid_filename(filename))
    {
        kprint("Usage: del <filename>\n");
        return;
    }

    if (!kedo_disk_load(&disk))
    {
        kprint("Error: could not load KEDO disk.\n");
        return;
    }

    for (int i = 0; i < KEDO_FILE_COUNT; i++)
    {
        if (disk.files[i].used &&
            strings_equal(disk.files[i].name, filename))
        {
            disk.files[i].used = 0;

            if (!kedo_disk_save(&disk))
            {
                kprint("Error: could not save KEDO disk.\n");
                return;
            }

            kprint("KEDO file deleted.\n");
            return;
        }
    }

    kprint("Error: file not found on KEDO disk.\n");
}

/* =========================================================
   MAIN
   ========================================================= */

int main(void)
{
    kprint("\n");
    kprint("KEDO DOS - Sofya Sobric\n");
    kprint("Type help for commands.\n\n");

    while (1)
    {
        kprint("KEDO> ");

        /* Read the command exactly once. */

        kinput(command, sizeof(command));

        if (command[0] == '\0')
        {
            continue;
        }

        /* HELLO */

        if (strings_equal(command, "hello"))
        {
            command_hello();
            continue;
        }

        /* HELP */

        else if (strings_equal(command, "help"))
        {
            kprint(
                "Commands:\n"
                "    hello: hi ig\n"
                "    help: the fuck you think it does?\n"
                "    ver: version T_T\n"
                "    advcom: developer commands\n"
                "    write <name> <text>: write a KEDO file\n"
                "    read <name>: read a KEDO file\n"
                "    dir: list KEDO files\n"
                "    del <name>: delete a KEDO file\n"
            );

            continue;
        }

        /* VERSION */

        else if (strings_equal(command, "ver"))
        {
            kprint("kedo dos version 2.12132034+2.12132034i\n");
            continue;
        }

        /* ADVANCED COMMANDS */

        else if (strings_equal(command, "advcom"))
        {
            kprint(
                "Advanced commands:\n"
                "    fr: exit the program\n"
                "    meminfo: display memory information\n"
                "    memtest: run memory tests\n"
                "    sys panic -bcd: trigger a test panic\n"
                "    lcon: display system configuration\n"
            );

            continue;
        }

        /* EXIT */

        else if (strings_equal(command, "fr"))
        {
            return 0;
        }

        /* MEMORY INFORMATION */

        else if (strings_equal(command, "meminfo"))
        {
            kmeminfo();
            continue;
        }

        /* MEMORY TEST */

        else if (strings_equal(command, "memtest"))
        {
            kmemtest();
            continue;
        }

        /* PANIC TEST */

        else if (strings_equal(command, "sys panic -bcd"))
        {
            kpanic("BCD - Bad Configuration Data");
            continue;
        }

        /* CONFIGURATION */

        else if (strings_equal(command, "lcon"))
        {
            kprint("Current configuration:\n");

            kprint("Reboot on crash: ");
            kprint_number(config.reboot_on_crash);
            kprint("\n");

            kprint("Shutdown confirmation: ");
            kprint_number(config.shutdown_confirm);
            kprint("\n");

            kprint("Sound enabled: ");
            kprint_number(config.sound_enabled);
            kprint("\n");

            continue;
        }

        /* WRITE FILE */

        else if (starts_with(command, "write "))
        {
            command_write(command + 6);
            continue;
        }

        /* READ FILE */

        else if (starts_with(command, "read "))
        {
            command_read(command + 5);
            continue;
        }

        /* LIST FILES */

        else if (strings_equal(command, "dir"))
        {
            command_dir();
            continue;
        }

        /* DELETE FILE */

        else if (starts_with(command, "del "))
        {
            command_del(command + 4);
            continue;
        }

        /* UNKNOWN COMMAND */

        else
        {
            kprint(
                "Unknown command. "
                "Try one in the designated dictionary [help].\n"
            );
        }
    }
}
