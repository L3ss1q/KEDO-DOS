#include <stdio.h>

int number = 42;
int *pointer = &number;
char kedo_memory[1024];
int memory_used = 0;
char *kmalloc(int size)
{
    char *address;

    address = &kedo_memory[memory_used];
    memory_used += size;

    return address;
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
char command[100];
char *command_pointer = command;
int main(void)
{
  char *thing = kmalloc(10);

thing[0] = 'K';
thing[1] = 'E';
thing[2] = 'D';
thing[3] = 'O';
thing[4] = '\0';

kprint(thing);
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
      
        kprint("Commands:\n    hello: hi ig\n    help: the fuck you think it does?\n    ver: version T_T\n    literally anything else: no :3\n");
    }
        else if(strings_equal(command, "ver"))
        {
            kprint("kedo dos version 2.12132034+2.12132034i\n");
        }
    else
    {
        kprint("Unknown command. Try one that is in the designated dictionary [help]\n");
    }
}
}
