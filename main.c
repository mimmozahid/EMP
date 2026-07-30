#include <stdio.h>
#include "all_header.h"

int main()
{
    AdminManager loggedInUser;
    int loggedIn;
    int loggedInEmpId = 0;

    initializeSystem();

    printf("=========================================\n");
    printf("      EMPLOYEE MANAGEMENT SYSTEM\n");
    printf("=========================================\n");

    while (1)
    {
        loggedIn = login (&loggedInUser, &loggedInEmpId);

        if (!loggedIn)
        {
            continue;
        }

        if (loggedInUser.role == EXIT)
        {
            printf ("Exit the system successfully\n");
            break;
        }

        printf("\nLogin successful.\n");

        if (loggedInUser.role == ADMIN)
        {
            clearScreen();
            adminMenu(loggedInUser);
        }
        else if (loggedInUser.role == MANAGER)
        {
            clearScreen();
            managerMenu(loggedInUser);
        }
        else if (loggedInUser.role == EMPLOYEE)
        {
            clearScreen();
            employeeMenu(loggedInEmpId);
        }
        else 
            printf("Unknown role.\n");
    }
    
    return 0;
}