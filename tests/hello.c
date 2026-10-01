#include <stdio.h>

int main(void)
{
    char command[100];

    while (1)
    {
        printf("KEDO> ");
        fgets(command, sizeof(command), stdin);

        printf("You entered: %s", command);
    }
}
