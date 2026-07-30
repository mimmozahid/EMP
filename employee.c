#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <argon2.h>
#include "all_header.h"

Employee getEmployeeById(int id, int *found)
{
    FILE *fp = fopen(EMP_FILE, "r");
    Employee emp;

    *found = 0;

    // Initialize employee
    emp.id = 0;
    strcpy(emp.name, "");
    strcpy(emp.email, "");
    strcpy(emp.dateofbirth, "");
    strcpy(emp.position, "");
    emp.baseSalary = 0;
    strcpy(emp.password, "");
    emp.bonus = 0;
    emp.deduction = 0;

    if (fp == NULL)
        return emp;

    char line[300];

    while (fgets(line, sizeof(line), fp) != NULL)
    {
        line[strcspn(line, "\n")] = '\0';

        char *token;

        // Employee ID
        token = strtok(line, "|");
        if (token == NULL) continue;
        emp.id = atoi(token);

        // Name
        token = strtok(NULL, "|");
        if (token == NULL) continue;
        strcpy(emp.name, token);

        // Email
        token = strtok(NULL, "|");
        if (token == NULL) continue;
        strcpy(emp.email, token);

        // Date of Birth
        token = strtok(NULL, "|");
        if (token == NULL) continue;
        strcpy(emp.dateofbirth, token);

        // Position
        token = strtok(NULL, "|");
        if (token == NULL) continue;
        strcpy(emp.position, token);

        // Base Salary
        token = strtok(NULL, "|");
        if (token == NULL) continue;
        emp.baseSalary = atof(token);

        // Password
        token = strtok(NULL, "|");
        if (token == NULL) continue;
        strcpy(emp.password, token);

        // Bonus
        token = strtok(NULL, "|");
        if (token == NULL) continue;
        emp.bonus = atof(token);

        // Deduction
        token = strtok(NULL, "|");
        if (token == NULL) continue;
        emp.deduction = atof(token);

        // Search Employee by ID
        if (emp.id == id)
        {
            *found = 1;
            fclose(fp);
            return emp;
        }
    }

    fclose(fp);
    return emp;
}

int employeeExists(int id)
{
    FILE *fp = fopen(EMP_FILE, "r");

    if (fp == NULL)
        return 0;

    char line[300];

    while (fgets(line, sizeof(line), fp) != NULL)
    {
        line[strcspn(line, "\n")] = '\0';

        char *token = strtok(line, "|");

        if (token == NULL)
            continue;

        int empId = atoi(token);

        if (empId == id)
        {
            fclose(fp);
            return 1;
        }
    }

    fclose(fp);
    return 0;
}

int create_strong_password(char pass[])
{
    int len = strlen(pass);

    if (len < 8)
        return 0;

    int upper = 0, lower = 0, digit = 0, special = 0;

    for (int i = 0; i < len; i++)
    {
        if (islower((unsigned char)pass[i]))
            lower = 1;
        else if (isupper((unsigned char)pass[i]))
            upper = 1;
        else if (isdigit((unsigned char)pass[i]))
            digit = 1;
        else if (ispunct((unsigned char)pass[i]))
            special = 1;
    }

    return upper && lower && digit && special;
}

void viewProfile(int empId)
{
    int found;
    Employee emp = getEmployeeById(empId, &found);

    if (!found)
    {
        printf("Profile not found.\n");
        return;
    }

    printf("\n========== EMPLOYEE PROFILE ==========\n");
    printf("Employee ID   : %d\n", emp.id);
    printf("Name          : %s\n", emp.name);
    printf("Email         : %s\n", emp.email);
    printf("Date of Birth : %s\n", emp.dateofbirth);
    printf("Position      : %s\n", emp.position);
    printf("Base Salary   : %.2f\n", emp.baseSalary);
    printf("Bonus         : %.2f\n", emp.bonus);
    printf("Deduction     : %.2f\n", emp.deduction);
    printf("Present Days  : %d\n", countPresentDays(emp.id));
    printf("Final Salary  : %.2f\n", calculateSalary(emp));
}

void viewOwnAttendance(int empId)
{
    FILE *fp = fopen(ATT_FILE, "r");
    Attendance att;
    int found = 0;

    if (fp == NULL)
    {
        printf("Attendance file not found.\n");
        return;
    }

    printf("\n========== MY ATTENDANCE ==========\n");
    printf("%-10s %-15s %-10s\n", "EmpID", "Date", "Status");
    printf("------------------------------------------\n");

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

        if (att.empId == empId)
        {
            printf("%-10d %02d/%02d/%04d %-10s\n",
                   att.empId,
                   att.day,
                   att.month,
                   att.year,
                   att.status == 1 ? "Present" : "Absent");

            found = 1;
        }
    }

    if (!found)
    {
        printf("No attendance records found.\n");
    }

    fclose(fp);
}

void applyLeave(int empId)
{
    FILE *fp = fopen(LEAVE_FILE, "a");

    if (fp == NULL)
    {
        printf("Error opening leave file.\n");
        return;
    }

    Leave leave;

    leave.empId = empId;
    leave.status = 0; // Pending

    // Get current date
    time_t now = time(NULL);
    struct tm *current = localtime(&now);

    strftime(leave.applyDate,
             sizeof(leave.applyDate),
             "%d/%m/%Y",
             current);

    printf("\n========== APPLY FOR LEAVE ==========\n");
    printf("Employee ID : %d\n", leave.empId);
    printf("Apply Date  : %s\n", leave.applyDate);

    // Leave From Date
    int fromDay, fromMonth, fromYear;

    printf("Leave From (DD/MM/YYYY): ");
    if (scanf("%d/%d/%d", &fromDay, &fromMonth, &fromYear) != 3)
    {
        printf("Invalid date format.\n");
        fclose(fp);
        clearInputBuffer();
        return;
    }

    if (fromYear < 100)
        fromYear += 2000;

    if (fromDay < 1 || fromDay > 31 ||
        fromMonth < 1 || fromMonth > 12)
    {
        printf("Invalid From Date.\n");
        fclose(fp);
        return;
    }

    sprintf(leave.fromDate, "%02d/%02d/%04d",fromDay,fromMonth,fromYear);

    // Leave To Date
    int toDay, toMonth, toYear;

    printf("Leave To (DD/MM/YYYY): ");
    if (scanf("%d/%d/%d", &toDay, &toMonth, &toYear) != 3)
    {
        printf("Invalid date format.\n");
        fclose(fp);
        clearInputBuffer();
        return;
    }

    if (toYear < 100)
        toYear += 2000;

    if (toDay < 1 || toDay > 31 ||
        toMonth < 1 || toMonth > 12)
    {
        printf("Invalid To Date.\n");
        fclose(fp);
        return;
    }

    sprintf(leave.toDate, "%02d/%02d/%04d", toDay, toMonth, toYear);

    // Check date order
    if ((toYear < fromYear) || (toYear == fromYear && toMonth < fromMonth) || (toYear == fromYear && toMonth == fromMonth && toDay < fromDay))
    {
        printf("Leave To date cannot be earlier than Leave From date.\n");
        fclose(fp);
        return;
    }

    clearInputBuffer();

    // Reason
    do
    {
        printf("Reason: ");
        fgets(leave.reason, sizeof(leave.reason), stdin);
        leave.reason[strcspn(leave.reason, "\n")] = '\0';

        if (strlen(leave.reason) == 0)
            printf("Reason cannot be empty.\n");

    } while (strlen(leave.reason) == 0);

    if (strchr(leave.reason, '|') != NULL)
    {
        printf("Reason cannot contain '|'.\n");
        fclose(fp);
        return;
    }

    // Save
    fprintf(fp, "%d|%s|%s|%s|%s|%d\n", leave.empId, leave.applyDate, leave.fromDate, leave.toDate, leave.reason, leave.status);

    fclose(fp);

    printf("\nLeave application submitted successfully.\n");
    printf("Status : Pending\n");
}

void viewLeaveStatus(int empId)
{
    FILE *fp = fopen(LEAVE_FILE, "r");

    if (fp == NULL)
    {
        printf("No leave records found.\n");
        return;
    }

    Leave leave;
    char line[300];
    int found = 0;

    printf("\n================ MY LEAVE STATUS ================\n");
    printf("%-12s %-12s %-12s %-25s %-10s\n",
           "Apply Date",
           "From",
           "To",
           "Reason",
           "Status");

    printf("--------------------------------------------------------------------------\n");

    while (fgets(line, sizeof(line), fp) != NULL)
    {
        line[strcspn(line, "\n")] = '\0';

        char *token;

        // Employee ID
        token = strtok(line, "|");
        if (token == NULL)
            continue;
        leave.empId = atoi(token);

        // Apply Date
        token = strtok(NULL, "|");
        if (token == NULL)
            continue;
        strcpy(leave.applyDate, token);

        // From Date
        token = strtok(NULL, "|");
        if (token == NULL)
            continue;
        strcpy(leave.fromDate, token);

        // To Date
        token = strtok(NULL, "|");
        if (token == NULL)
            continue;
        strcpy(leave.toDate, token);

        // Reason
        token = strtok(NULL, "|");
        if (token == NULL)
            continue;
        strcpy(leave.reason, token);

        // Status
        token = strtok(NULL, "|");
        if (token == NULL)
            continue;
        leave.status = atoi(token);

        if (leave.empId == empId)
        {
            char status[15];

            if (leave.status == 0)
                strcpy(status, "Pending");
            else if (leave.status == 1)
                strcpy(status, "Approved");
            else
                strcpy(status, "Rejected");

            printf("%-12s %-12s %-12s %-25s %-10s\n",
                   leave.applyDate,
                   leave.fromDate,
                   leave.toDate,
                   leave.reason,
                   status);

            found = 1;
        }
    }

    fclose(fp);

    if (!found)
    {
        printf("No leave applications found.\n");
    }
}

void updateOwnProfile(int empId)
{
    int found;
    Employee emp;

    emp = getEmployeeById(empId, &found);

    if (!found)
    {
        printf("Employee profile not found.\n");
        return;
    }

    printf("\n========== UPDATE MY PROFILE ==========\n");

    clearInputBuffer();

    // Name
    printf("Current Name : %s\n", emp.name);
    printf("Enter New Name: ");
    fgets(emp.name, sizeof(emp.name), stdin);
    emp.name[strcspn(emp.name, "\n")] = '\0';

    // Email
    printf("\nCurrent Email : %s\n", emp.email);
    printf("Enter New Email: ");
    fgets(emp.email, sizeof(emp.email), stdin);
    emp.email[strcspn(emp.email, "\n")] = '\0';

    // Date of Birth
    int day, month, year;

    printf("\nCurrent Date of Birth : %s\n", emp.dateofbirth);
    printf("Enter New Date of Birth (D/M/YYYY or D/M/YY): ");
    scanf("%d/%d/%d", &day, &month, &year);

    // Convert 2-digit year to 4-digit year
    if (year < 100)
        year += 2000;

    sprintf(emp.dateofbirth, "%02d/%02d/%04d", day, month, year);

    // Save changes
    updateEmployeeRecord(emp);

    printf("\nProfile updated successfully.\n");
}

void changeEmployeePassword(int empId)
{
    int found;
    Employee emp;
    char currentPass[30], newPass[30], confirmPass[30];

    emp = getEmployeeById(empId, &found);

    if (!found)
    {
        printf("Employee profile not found.\n");
        return;
    }

    printf("\n========== CHANGE PASSWORD ==========\n");

    printf("Enter Current Password: ");
    scanf("%29s", currentPass);

    int verify_result = argon2id_verify(emp.password, currentPass, strlen(currentPass));
    
    if (verify_result != ARGON2_OK)
    {
        printf("Current password is incorrect.\n");
        return;
    }
    // ----------------------------------------------

    printf("Enter New Password: ");
    scanf("%29s", newPass);

    if (create_strong_password (newPass))
    {
        printf ("Strength: Strong\n");
    }
    else
    {
        printf ("Strength: Weak\n");
        return;
    }

    printf("Confirm New Password: ");
    scanf("%29s", confirmPass);

    if (strcmp(newPass, confirmPass) != 0)
    {
        printf("New password and confirm password do not match.\n");
        return;
    }

    strcpy(emp.password, confirmPass);

    updateEmployeeRecord(emp);

    printf("Password changed successfully.\n");
}


void employeeMenu(int empId) {
    int choice;

    do {
        printf("\n========== EMPLOYEE MENU ==========\n");
        printf("1. View Profile\n");
        printf("2. View Own Attendance\n");
        printf("3. Apply For Leave\n");
        printf("4. View Leave Status\n");
        printf("5. Update My Profile\n");
        printf("6. Change Password\n");
        printf("7. Logout\n");
        printf("Enter choice: ");
        scanf("%d", &choice);

        clearScreen();

        switch (choice) {
            case 1: viewProfile(empId); break;
            case 2: viewOwnAttendance(empId); break;
            case 3: applyLeave(empId); break;
            case 4: viewLeaveStatus(empId); break;
            case 5: updateOwnProfile(empId); break;
            case 6: changeEmployeePassword(empId); break;
            case 7: printf("Logged out successfully.\n");  break;
            default: printf("Invalid choice.\n");
        }

    } while (choice != 7);
}