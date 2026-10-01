#include <stdio.h>

int main(void)
{
    char command[100];

    while (1)
    {
        printf("KEDO> ");
fgets(command, sizeof(command), stdin);

if (strcmp(command, "hello\n") == 0)
{
    printf("Hello!\n");
}
    }
}
