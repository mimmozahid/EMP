#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <argon2.h>
#include "all_header.h"

void approveLeaveRequest()
{
    FILE *fp = fopen(LEAVE_FILE, "r");
    FILE *temp = fopen("temp_leave.txt", "w");

    if (fp == NULL || temp == NULL)
    {
        printf("Error opening leave file.\n");

        if (fp)
            fclose(fp);

        if (temp)
            fclose(temp);

        return;
    }

    Leave leave;
    char line[300];
    int empId;
    int found = 0;
    int choice;

    printf("\n========== LEAVE REQUEST APPROVAL ==========\n");
    printf("Enter Employee ID: ");
    scanf("%d", &empId);

    while (fgets(line, sizeof(line), fp) != NULL)
    {
        line[strcspn(line, "\n")] = '\0';

        char *token;

        token = strtok(line, "|");
        if (!token) continue;
        leave.empId = atoi(token);

        token = strtok(NULL, "|");
        if (!token) continue;
        strcpy(leave.applyDate, token);

        token = strtok(NULL, "|");
        if (!token) continue;
        strcpy(leave.fromDate, token);

        token = strtok(NULL, "|");
        if (!token) continue;
        strcpy(leave.toDate, token);

        token = strtok(NULL, "|");
        if (!token) continue;
        strcpy(leave.reason, token);

        token = strtok(NULL, "|");
        if (!token) continue;
        leave.status = atoi(token);

        if (leave.empId == empId && leave.status == 0)
        {
            found = 1;

            printf("\nLeave Request Found\n");
            printf("------------------------------\n");
            printf("Employee ID : %d\n", leave.empId);
            printf("Apply Date  : %s\n", leave.applyDate);
            printf("From Date   : %s\n", leave.fromDate);
            printf("To Date     : %s\n", leave.toDate);
            printf("Reason      : %s\n", leave.reason);

            printf("\n1. Approve\n");
            printf("2. Reject\n");
            printf("Choice: ");
            scanf("%d", &choice);

            if (choice == 1)
                leave.status = 1;
            else if (choice == 2)
                leave.status = 2;
            else
            {
                printf("Invalid choice. Keeping Pending.\n");
                leave.status = 0;
            }
        }

        fprintf(temp,
                "%d|%s|%s|%s|%s|%d\n",
                leave.empId,
                leave.applyDate,
                leave.fromDate,
                leave.toDate,
                leave.reason,
                leave.status);
    }

    fclose(fp);
    fclose(temp);

    remove(LEAVE_FILE);
    rename("temp_leave.txt", LEAVE_FILE);

    if (found)
        printf("\nLeave request updated successfully.\n");
    else
        printf("\nNo pending leave request found for this employee.\n");
}

int countPresentDays(int empId)
{
    FILE *fp = fopen(ATT_FILE, "r");
    int count = 0;
    Attendance att;

    if (fp == NULL)
        return 0;

    char line[100];

    while (fgets(line, sizeof(line), fp) != NULL)
    {
        line[strcspn(line, "\n")] = '\0';

        char *token;

        // Employee ID
        token = strtok(line, "|");
        if (token == NULL)
            continue;
        att.empId = atoi(token);

        // Date (DD/MM/YYYY)
        token = strtok(NULL, "|");
        if (token == NULL)
            continue;

        sscanf(token, "%d/%d/%d",
               &att.day,
               &att.month,
               &att.year);

        // Status
        token = strtok(NULL, "|");
        if (token == NULL)
            continue;
        att.status = atoi(token);

        if (att.empId == empId && att.status == 1)
        {
            count++;
        }
    }

    fclose(fp);
    return count;
}

float calculateSalary(Employee emp)
{
    int presentDays = countPresentDays(emp.id);

    float dailySalary = emp.baseSalary / 30.0f;

    float finalSalary =
        (dailySalary * presentDays)
        + emp.bonus
        - emp.deduction;

    if (finalSalary < 0)
    {
        finalSalary = 0;
    }

    return finalSalary;
}


void managerMenu(AdminManager user) {
    int choice;

    do {
        printf("\n========== MANAGER MENU ==========\n");
        printf("1. View Employee Records\n");
        printf("2. Search Employee\n");
        printf("3. View Attendance\n");
        printf("4. Payroll Processing\n");
        printf("5. Approve Leave Request\n");
        printf("6. Change Manager Password\n");
        printf("7. Logout\n");
        printf("Enter choice: ");
        scanf("%d", &choice);

        clearScreen();

        switch (choice) {
            case 1: viewEmployees(); break;
            case 2: searchEmployee(); break;
            case 3: viewAttendance(); break;
            case 4: payrollProcessing(); break;
            case 5: approveLeaveRequest(); break;
            case 6: change_admin_manager_pass(user); break;
            case 7: printf("Logged out successfully.\n"); break;
            default: printf("Invalid choice.\n");
        }

    } while (choice != 7);
}