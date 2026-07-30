#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <argon2.h>
#include "all_header.h"


int generateEmployeeId()
{
    FILE *fp = fopen(EMP_FILE, "r");
    Employee emp;
    int lastId = 1000;

    if (fp == NULL)
        return 1001;

    char line[400];

    while (fgets(line, sizeof(line), fp) != NULL)
    {
        line[strcspn(line, "\n")] = '\0';

        char *token;

        // Employee ID
        token = strtok(line, "|");
        if (token == NULL)
            continue;
        emp.id = atoi(token);

        // Name
        token = strtok(NULL, "|");
        if (token == NULL)
            continue;
        strcpy(emp.name, token);

        // Email
        token = strtok(NULL, "|");
        if (token == NULL)
            continue;
        strcpy(emp.email, token);

        // Date of Birth
        token = strtok(NULL, "|");
        if (token == NULL)
            continue;
        strcpy(emp.dateofbirth, token);

        // Position
        token = strtok(NULL, "|");
        if (token == NULL)
            continue;
        strcpy(emp.position, token);

        // Base Salary
        token = strtok(NULL, "|");
        if (token == NULL)
            continue;
        emp.baseSalary = atof(token);

        // Password
        token = strtok(NULL, "|");
        if (token == NULL)
            continue;
        strcpy(emp.password, token);

        // Bonus
        token = strtok(NULL, "|");
        if (token == NULL)
            continue;
        emp.bonus = atof(token);

        // Deduction
        token = strtok(NULL, "|");
        if (token == NULL)
            continue;
        emp.deduction = atof(token);

        if (emp.id > lastId)
        {
            lastId = emp.id;
        }
    }

    fclose(fp);

    return lastId + 1;
}

int isValidEmail(const char *email)
{
    const char *gmail = "@gmail.com";
    const char *diu = "@diu.edu.bd";

    const char *at = strchr(email, '@');
    if (at == NULL)
        return 0;

    if (at == email)
        return 0;

    if (strcmp(at, gmail) == 0 || strcmp(at, diu) == 0)
        return 1;

    return 0;
}

void updateEmployeeRecord(Employee updatedEmp)
{
    FILE *fp = fopen(EMP_FILE, "r");
    FILE *tmp = fopen("tmp_emp.txt", "w");

    char line[512]; 
    Employee currentEmp;

    if (fp == NULL || tmp == NULL)
    {
        printf("Error opening employee data files.\n");
        if (fp) fclose(fp);
        if (tmp) fclose(tmp);
        return;
    }

    while (fgets(line, sizeof(line), fp))
    {
        line[strcspn(line, "\n")] = '\0';

        char lineCopy[512];
        strcpy(lineCopy, line);

        char *token = strtok(line, "|");
        if (token == NULL) continue;
        currentEmp.id = atoi(token);

        if (currentEmp.id == updatedEmp.id)
        {
            char hashed_password[128];
            
            if (strncmp(updatedEmp.password, "$argon2", 7) != 0)
            {
                uint8_t salt[16];
                for (int i = 0; i < 16; i++) {
                    salt[i] = rand() % 256;
                }

                uint32_t t_cost = 3;
                uint32_t m_cost = 65536;
                uint32_t parallelism = 4;

                int res = argon2id_hash_encoded(t_cost, m_cost, parallelism, updatedEmp.password, strlen(updatedEmp.password), salt, 16, 32, hashed_password, sizeof(hashed_password));

                if (res != ARGON2_OK) {
                    printf("Security Error: Failed to secure the updated password.\n");
                    fclose(fp);
                    fclose(tmp);
                    return;
                }
                
                strcpy(updatedEmp.password, hashed_password);
            }

            fprintf(tmp, "%d|%s|%s|%s|%s|%.2f|%s|%.2f|%.2f\n", updatedEmp.id, updatedEmp.name, updatedEmp.email, updatedEmp.dateofbirth, updatedEmp.position, updatedEmp.baseSalary, updatedEmp.password, updatedEmp.bonus, updatedEmp.deduction);
        }

        else fprintf(tmp, "%s\n", lineCopy);
    }

    fclose(fp);
    fclose(tmp);

    remove(EMP_FILE);
    rename("tmp_emp.txt", EMP_FILE);
}

void deleteEmployeeAttendance(int empId)
{
    FILE *fp = fopen(ATT_FILE, "r");
    FILE *temp = fopen("temp_attendance.csv", "w");

    Attendance att;

    if (fp == NULL || temp == NULL)
    {
        if (fp != NULL)
            fclose(fp);

        if (temp != NULL)
            fclose(temp);

        return;
    }

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

        // Skip deleted employee's attendance
        if (att.empId == empId)
            continue;

        fprintf(temp, "%d|%02d/%02d/%04d|%d\n", att.empId, att.day, att.month, att.year, att.status);
    }

    fclose(fp);
    fclose(temp);

    remove(ATT_FILE);
    rename("temp_attendance.csv", ATT_FILE);
}

int attendanceAlreadyExists(int empId, int day, int month, int year)
{
    FILE *fp = fopen(ATT_FILE, "r");
    Attendance att;

    if (fp == NULL)
        return 0;

    char line[300];

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

        sscanf(token, "%d/%d/%d", &att.day, &att.month, &att.year);

        // Status
        token = strtok(NULL, "|");
        if (token == NULL)
            continue;
        att.status = atoi(token);

        if (att.empId == empId && att.day == day &&att.month == month &&att.year == year)
        {
            fclose(fp);
            return 1;
        }
    }

    fclose(fp);
    return 0;
}

int usernameExists(const char *username)
{
    FILE *fp = fopen(ADMINMANAGER, "r");
    AdminManager user;

    if (fp == NULL)
        return 0;

    char line[300];

    while (fgets(line, sizeof(line), fp) != NULL)
    {
        line[strcspn(line, "\n")] = '\0';

        char *token;

        // Username
        token = strtok(line, "|");
        if (token == NULL)
            continue;
        strcpy(user.username, token);

        // Password
        token = strtok(NULL, "|");
        if (token == NULL)
            continue;
        strcpy(user.password, token);

        // Role
        token = strtok(NULL, "|");
        if (token == NULL)
            continue;
        user.role = atoi(token);

        if (strcmp(user.username, username) == 0)
        {
            fclose(fp);
            return 1;
        }
    }

    fclose(fp);
    return 0;
}

void update_admin_manager_pass(AdminManager updateAd)
{
    FILE *fp = fopen(ADMINMANAGER, "r");
    FILE *tmp = fopen("tmp.txt", "w");

    // Increased buffer size to safely capture the long Argon2 hash strings on read
    char line[300];
    AdminManager adm;

    if (fp == NULL || tmp == NULL)
    {
        printf("Error opening AdminManager file.\n");
        if (fp) fclose(fp);
        if (tmp) fclose(tmp);
        return;
    }

    while (fgets(line, sizeof(line), fp))
    {
        line[strcspn(line, "\n")] = '\0';   // Remove newline

        char *token = strtok(line, "|");
        if (token == NULL) continue;
        strcpy(adm.username, token);

        token = strtok(NULL, "|");
        if (token == NULL) continue;
        strcpy(adm.password, token);

        token = strtok(NULL, "|");
        if (token == NULL) continue;
        adm.role = atoi(token);

        if (strcmp(updateAd.username, adm.username) == 0)
        {
            // --- HASH THE NEW PASSWORD BEFORE SAVING ---
            // Setup a random salt for this specific user update
            uint8_t salt[16];
            for (int i = 0; i < 16; i++) {
                salt[i] = rand() % 256;
            }

            uint32_t t_cost = 3;
            uint32_t m_cost = 65536;
            uint32_t parallelism = 4;

            char hashed_password[128];

            // Hash updateAd.password (assuming it contains the new plaintext string from user input)
            int res = argon2id_hash_encoded(t_cost, m_cost, parallelism,
                                            updateAd.password, strlen(updateAd.password),
                                            salt, 16,
                                            32,
                                            hashed_password, sizeof(hashed_password));

            if (res != ARGON2_OK) {
                printf("Security Error: Failed to hash the updated password.\n");
                // Fallback: copy original matching line unmodified if hashing errors out
                fprintf(tmp, "%s|%s|%d\n", adm.username, adm.password, adm.role);
                continue;
            }

            // Write the newly secured password hash to the temporary file
            fprintf(tmp, "%s|%s|%d\n", updateAd.username, hashed_password, updateAd.role);
            // --------------------------------------------
        }
        else
        {
            // Preserve other existing users without changing their already-hashed passwords
            fprintf(tmp, "%s|%s|%d\n", adm.username, adm.password, adm.role);
        }
    }

    fclose(fp);
    fclose(tmp);

    remove(ADMINMANAGER);
    rename("tmp.txt", ADMINMANAGER);
}





void addEmployee()
{
    FILE *fp = fopen(EMP_FILE, "a");
    Employee emp;

    if (fp == NULL)
    {
        printf("Error opening employee file.\n");
        return;
    }

    printf("\n================== ADD EMPLOYEE ==================\n");

    emp.id = generateEmployeeId();
    printf("Employee ID: %d\n", emp.id);

    clearInputBuffer();

    // Name
    printf("Enter Name: ");
    fgets(emp.name, sizeof(emp.name), stdin);
    emp.name[strcspn(emp.name, "\n")] = '\0';

    // Email
    do
    {
        printf("Enter Email: ");
        fgets(emp.email, sizeof(emp.email), stdin);
        emp.email[strcspn(emp.email, "\n")] = '\0';

        if (!isValidEmail(emp.email))
        {
            printf("Invalid email!\n");
            printf("Email must end with @gmail.com or @diu.edu.bd\n");
        }

    } while (!isValidEmail(emp.email));
    

    // Date of Birth
    int day, month, year;
    printf("Enter Date of Birth (D/M/YYYY or D/M/YY): ");
    scanf("%d/%d/%d", &day, &month, &year);

    if (year < 100)
        year += 2000;

    sprintf(emp.dateofbirth, "%02d/%02d/%04d", day, month, year);

    clearInputBuffer();

    // Position
    printf("Enter Position: ");
    fgets(emp.position, sizeof(emp.position), stdin);
    emp.position[strcspn(emp.position, "\n")] = '\0';

    // Base Salary
    printf("Enter Base Salary: ");
    scanf("%f", &emp.baseSalary);

    // --- ARGON2 PASSWORD HASHING INTEGRATION ---
    char plainPassword[100];
    printf("Set Password: ");
    scanf("%99s", plainPassword); // Read into a temporary plaintext buffer

    // Generate a unique, random 16-byte salt
    uint8_t salt[16];
    for (int i = 0; i < 16; i++) {
        salt[i] = rand() % 256; 
    }

    // Configure safe baseline Argon2 parameters
    uint32_t t_cost = 3;           // 3 iterations
    uint32_t m_cost = 65536;       // 64 MB RAM consumption
    uint32_t parallelism = 4;      // 4 threads

    int res = argon2id_hash_encoded(t_cost, m_cost, parallelism,
                                    plainPassword, strlen(plainPassword),
                                    salt, 16,
                                    32,
                                    emp.password, sizeof(emp.password));

    if (res != ARGON2_OK) {
        printf("Security Error: Failed to safely hash the password (%s).\n", argon2_error_message(res));
        fclose(fp);
        return;
    }
    // --------------------------------------------

    // Default values
    emp.bonus = 0;
    emp.deduction = 0;

    // Save to file (emp.password now writes the safe encoded hash string)
    fprintf(fp, "%d|%s|%s|%s|%s|%.2f|%s|%.2f|%.2f\n", emp.id, emp.name, emp.email, emp.dateofbirth, emp.position, emp.baseSalary, emp.password, emp.bonus, emp.deduction);

    fclose(fp);

    printf("\nEmployee added successfully.\n");
    printf("Employee Login ID : %d\n", emp.id);
}

void viewEmployees()
{
    FILE *fp = fopen(EMP_FILE, "r");
    Employee emp;
    char line[300];
    int found = 0;

    if (fp == NULL)
    {
        printf("No employee file found.\n");
        return;
    }

    printf("\n====================================== EMPLOYEE LIST ======================================\n");
    printf("%-6s %-20s %-25s %-18s %-12s %-10s %-10s\n",
           "ID", "Name", "Email", "Position", "Salary", "Bonus", "Deduction");
    printf("-------------------------------------------------------------------------------------------------\n");

    while (fgets(line, sizeof(line), fp) != NULL)
    {
        line[strcspn(line, "\n")] = '\0';

        char *token;

        // ID
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

        // Date of Birth (Read but don't display)
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

        // Password (Read but don't display)
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

        printf("%-6d %-20s %-25s %-18s %-12.2f %-10.2f %-10.2f\n",
               emp.id,
               emp.name,
               emp.email,
               emp.position,
               emp.baseSalary,
               emp.bonus,
               emp.deduction);

        found = 1;
    }

    if (!found)
    {
        printf("No employee records found.\n");
    }

    fclose(fp);
}

void updateEmployee()
{
    int id, found;
    Employee emp;

    printf("\nEnter Employee ID to update: ");
    scanf("%d", &id);

    emp = getEmployeeById(id, &found);

    if (!found)
    {
        printf("Employee not found.\n");
        return;
    }

    clearInputBuffer();

    printf("\n========== UPDATE EMPLOYEE ==========\n");

    printf("Current Name: %s\n", emp.name);
    printf("Enter New Name: ");
    fgets(emp.name, sizeof(emp.name), stdin);
    emp.name[strcspn(emp.name, "\n")] = '\0';

    printf("Current Email: %s\n", emp.email);
    printf("Enter New Email: ");
    fgets(emp.email, sizeof(emp.email), stdin);
    emp.email[strcspn(emp.email, "\n")] = '\0';

    printf("Current Date of Birth: %s\n", emp.dateofbirth);
    printf("Enter New Date of Birth (DD/MM/YYYY): ");
    fgets(emp.dateofbirth, sizeof(emp.dateofbirth), stdin);
    emp.dateofbirth[strcspn(emp.dateofbirth, "\n")] = '\0';

    printf("Current Position: %s\n", emp.position);
    printf("Enter New Position: ");
    fgets(emp.position, sizeof(emp.position), stdin);
    emp.position[strcspn(emp.position, "\n")] = '\0';

    printf("Current Base Salary: %.2f\n", emp.baseSalary);
    printf("Enter New Base Salary: ");
    scanf("%f", &emp.baseSalary);

    //* Bonus, Deduction and Password remain unchanged...

    updateEmployeeRecord(emp);

    printf("\nEmployee updated successfully.\n");
}

void deleteEmployee()
{
    int id, found = 0;

    FILE *fp = fopen(EMP_FILE, "r");
    FILE *temp = fopen("temp_employees.txt", "w");

    Employee emp;

    if (fp == NULL || temp == NULL)
    {
        printf("Error opening employee file.\n");

        if (fp != NULL)
            fclose(fp);

        if (temp != NULL)
            fclose(temp);

        return;
    }

    printf("\nEnter Employee ID to delete: ");
    scanf("%d", &id);

    char line[300];

    while (fgets(line, sizeof(line), fp) != NULL)
    {
        line[strcspn(line, "\n")] = '\0';

        char *token;

        // ID
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
        emp.baseSalary = (float)atof(token);

        // Password
        token = strtok(NULL, "|");
        if (token == NULL) continue;
        strcpy(emp.password, token);

        // Bonus
        token = strtok(NULL, "|");
        if (token == NULL) continue;
        emp.bonus = (float)atof(token);

        // Deduction
        token = strtok(NULL, "|");
        if (token == NULL) continue;
        emp.deduction = (float)atof(token);

        // Skip employee to be deleted
        if (emp.id == id)
        {
            found = 1;
            continue;
        }

        // Write remaining employees
        fprintf(temp, "%d|%s|%s|%s|%s|%.2f|%s|%.2f|%.2f\n", emp.id, emp.name, emp.email, emp.dateofbirth, emp.position, emp.baseSalary, emp.password, emp.bonus, emp.deduction);
    }

    fclose(fp);
    fclose(temp);

    remove(EMP_FILE);
    rename("temp_employees.txt", EMP_FILE);

    if (found)
    {
        deleteEmployeeAttendance(id);
        printf("Employee deleted successfully.\n");
    }
    else
    {
        printf("Employee not found.\n");
    }
}

void searchEmployee()
{
    int id, found;
    Employee emp;

    printf("\nEnter Employee ID to search: ");
    scanf("%d", &id);

    emp = getEmployeeById(id, &found);

    if (!found)
    {
        printf("Employee not found.\n");
        return;
    }

    printf("\n========== EMPLOYEE DETAILS ==========\n");

    printf("Employee ID     : %d\n", emp.id);
    printf("Name            : %s\n", emp.name);
    printf("Email           : %s\n", emp.email);
    printf("Date of Birth   : %s\n", emp.dateofbirth);
    printf("Position        : %s\n", emp.position);
    printf("Base Salary     : %.2f\n", emp.baseSalary);
    printf("Bonus           : %.2f\n", emp.bonus);
    printf("Deduction       : %.2f\n", emp.deduction);
    printf("Present Days    : %d\n", countPresentDays(emp.id));
    printf("Final Salary    : %.2f\n", calculateSalary(emp));
}

void manageJobRole() {
    int id, found;
    char newRole[50];
    Employee emp;

    printf("\nEnter Employee ID to change role: ");
    scanf("%d", &id);

    emp = getEmployeeById(id, &found);
    if (!found) {
        printf("Employee not found.\n");
        return;
    }

    clearInputBuffer();

    printf("Current Position: %s\n", emp.position);
    printf("Enter new Position: ");
    fgets(newRole, sizeof(newRole), stdin);
    newRole[strcspn(newRole, "\n")] = '\0';

    strcpy(emp.position, newRole);
    updateEmployeeRecord(emp);

    printf("Job role updated successfully.\n");
}

void recordAttendance()
{
    FILE *empFile = fopen(EMP_FILE, "r");
    FILE *attFile = fopen(ATT_FILE, "a");

    Employee emp;

    if (empFile == NULL || attFile == NULL)
    {
        printf("Error opening file.\n");

        if (empFile) fclose(empFile);
        if (attFile) fclose(attFile);

        return;
    }

    // Today's date
    time_t now = time(NULL);
    struct tm *current = localtime(&now);

    char date[11];
    strftime(date, sizeof(date), "%d/%m/%Y", current);

    int day = current->tm_mday;
    int month = current->tm_mon + 1;
    int year = current->tm_year + 1900;

    printf("\n========== RECORD ATTENDANCE ==========\n");
    printf("Today's Date : %s\n\n", date);

    char line[300];

    while (fgets(line, sizeof(line), empFile))
    {
        line[strcspn(line, "\n")] = '\0';

        char *token;

        // ID
        token = strtok(line, "|");
        if (token == NULL)
            continue;
        emp.id = atoi(token);

        // Name
        token = strtok(NULL, "|");
        if (token == NULL)
            continue;
        strcpy(emp.name, token);

        // Email
        token = strtok(NULL, "|");
        if (token == NULL)
            continue;

        // DOB
        token = strtok(NULL, "|");
        if (token == NULL)
            continue;

        // Position
        token = strtok(NULL, "|");
        if (token == NULL)
            continue;

        // Salary
        token = strtok(NULL, "|");
        if (token == NULL)
            continue;

        // Password
        token = strtok(NULL, "|");
        if (token == NULL)
            continue;

        // Bonus
        token = strtok(NULL, "|");
        if (token == NULL)
            continue;

        // Deduction
        token = strtok(NULL, "|");
        if (token == NULL)
            continue;

        // Skip if today's attendance already exists
        if (attendanceAlreadyExists(emp.id, day, month, year))
        {
            printf("Employee %-5d (%s) -> Already Recorded\n",
                   emp.id, emp.name);
            continue;
        }

        int status;

        do
        {
            printf("Employee %-5d %-20s (1=Present, 0=Absent): ",
                   emp.id, emp.name);

            scanf("%d", &status);

            if (status != 0 && status != 1)
                printf("Invalid input. Enter only 1 or 0.\n");

        } while (status != 0 && status != 1);

        fprintf(attFile,
                "%d|%s|%d\n",
                emp.id,
                date,
                status);
    }

    fclose(empFile);
    fclose(attFile);

    printf("\nAttendance recording completed successfully.\n");
}

void viewAttendance()
{
    int id, found = 0;

    FILE *fp = fopen(ATT_FILE, "r");
    Attendance att;

    if (fp == NULL)
    {
        printf("Attendance file not found.\n");
        return;
    }

    printf("\nEnter Employee ID to view attendance: ");
    scanf("%d", &id);

    printf("\n========== ATTENDANCE LOG ==========\n");
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

        sscanf(token, "%d/%d/%d", &att.day, &att.month, &att.year);

        // Status
        token = strtok(NULL, "|");
        if (token == NULL)
            continue;
        att.status = atoi(token);

        if (att.empId == id)
        {
            printf("%-10d %02d/%02d/%04d %-10s\n", att.empId, att.day, att.month, att.year, (att.status == 1) ? "   Present" : "    Absent");

            found = 1;
        }
    }

    if (!found)
    {
        printf("No attendance records found.\n");
    }

    fclose(fp);
}

void payrollProcessing()
{
    int id, found;
    Employee emp;

    printf("\nEnter Employee ID for Payroll Processing: ");
    scanf("%d", &id);

    emp = getEmployeeById(id, &found);

    if (!found)
    {
        printf("Employee not found.\n");
        return;
    }

    printf("\n========== PAYROLL DETAILS ==========\n");

    printf("Employee ID     : %d\n", emp.id);
    printf("Name            : %s\n", emp.name);
    printf("Email           : %s\n", emp.email);
    printf("Position        : %s\n", emp.position);
    printf("Base Salary     : %.2f\n", emp.baseSalary);
    printf("Present Days    : %d\n", countPresentDays(emp.id));
    printf("Bonus           : %.2f\n", emp.bonus);
    printf("Deduction       : %.2f\n", emp.deduction);
    printf("-------------------------------------\n");
    printf("Final Salary    : %.2f\n", calculateSalary(emp));
}

void addBonus() {
    int id, found;
    float bonus;
    Employee emp;

    printf("\nEnter Employee ID to add bonus: ");
    scanf("%d", &id);

    emp = getEmployeeById(id, &found);
    if (!found) {
        printf("Employee not found.\n");
        return;
    }

    printf("Enter Bonus Amount: ");
    scanf("%f", &bonus);

    if (bonus < 0) {
        printf("Bonus cannot be negative.\n");
        return;
    }

    emp.bonus += bonus;
    updateEmployeeRecord(emp);

    printf("Bonus added successfully.\n");
}

void addDeduction() {
    int id, found;
    float deduction;
    Employee emp;

    printf("\nEnter Employee ID to add deduction: ");
    scanf("%d", &id);

    emp = getEmployeeById(id, &found);
    if (!found) {
        printf("Employee not found.\n");
        return;
    }

    printf("Enter Deduction Amount: ");
    scanf("%f", &deduction);

    if (deduction < 0) {
        printf("Deduction cannot be negative.\n");
        return;
    }

    emp.deduction += deduction;
    updateEmployeeRecord(emp);

    printf("Deduction added successfully.\n");
}

void addSystemUser()
{
    FILE *fp = fopen(ADMINMANAGER, "a");
    AdminManager newUser;
    int roleChoice;

    if (fp == NULL)
    {
        printf("Error opening users file.\n");
        return;
    }

    printf("\n========== ADD ADMIN / MANAGER ACCOUNT ==========\n");
    printf("1. Add Admin\n");
    printf("2. Add Manager\n");
    printf("Enter choice: ");
    scanf("%d", &roleChoice);

    if (roleChoice != 1 && roleChoice != 2)
    {
        printf("Invalid choice.\n");
        fclose(fp);
        return;
    }

    printf("Enter Username: ");
    scanf("%29s", newUser.username);

    if (usernameExists(newUser.username))
    {
        printf("Username already exists.\n");
        fclose(fp);
        return;
    }

    char plainPassword[30];
    printf("Enter Password: ");
    scanf("%29s", plainPassword);

    newUser.role = (roleChoice == 1) ? ADMIN : MANAGER;

    uint8_t salt[16];
    for (int i = 0; i < 16; i++) {
        salt[i] = rand() % 256;
    }

    uint32_t t_cost = 3;
    uint32_t m_cost = 65536;
    uint32_t parallelism = 4;

    int res = argon2id_hash_encoded(t_cost, m_cost, parallelism, plainPassword, strlen(plainPassword), salt, 16, 32, newUser.password, sizeof(newUser.password));

    if (res != ARGON2_OK) {
        printf("Security Error: Failed to hash the account password (%s).\n", argon2_error_message(res));
        fclose(fp);
        return;
    }
    // --------------------------------------------

    fprintf(fp, "%s|%s|%d\n", newUser.username, newUser.password, newUser.role);

    fclose(fp);

    printf("Account created successfully.\n");
}

void change_admin_manager_pass (AdminManager user)
{
    char currentPass[30], newPass[30], confirmPass[30];

    AdminManager ad;
    strcpy(ad.username, user.username);
    ad.role = user.role;

    printf ("\n========== CHANGE PASSWORD ==========\n");

    printf("Enter current password: ");
    scanf("%29s", currentPass);

    // --- SECURE ARGON2ID CURRENT PASSWORD CHECK ---
    // user.password holds the long hash string loaded from your file during login
    int verify_result = argon2id_verify(user.password, currentPass, strlen(currentPass));
    
    if (verify_result != ARGON2_OK)
    {
        printf("Current password is incorrect.\n");
        return;
    }
    // ----------------------------------------------

    printf("Enter new password: ");
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

    printf("Confirm new password: ");
    scanf("%29s", confirmPass);

    if (strcmp(newPass, confirmPass) != 0) {
        printf("New password and confirm password do not match.\n");
        return;
    }

    // Copy the raw, plaintext new password here. 
    // Your update_admin_manager_pass(ad) function will hash it automatically right before saving.
    strcpy (ad.password, confirmPass);

    update_admin_manager_pass (ad);

    printf ("Password changed successfully.\n");
}






void adminMenu(AdminManager user)
{
    int choice;

    do {
        printf("\n========== ADMIN MENU ==========\n");
        printf("1. Add Employee\n");
        printf("2. View Employee Records\n");
        printf("3. Update Employee\n");
        printf("4. Delete Employee\n");
        printf("5. Search Employee\n");
        printf("6. Manage Job Roles\n");
        printf("7. Record Attendance\n");
        printf("8. View Attendance\n");
        printf("9. Payroll Processing\n");
        printf("10. Bonus Management\n");
        printf("11. Deduction Management\n");
        printf("12. Add Admin / Manager Account\n");
        printf("13. Change Admin Password\n");
        printf("14. Logout\n");
        printf("Enter choice: ");
        scanf("%d", &choice);

        clearScreen();
        
        switch (choice) {
            case 1: addEmployee(); break;
            case 2: viewEmployees(); break;
            case 3: updateEmployee(); break;
            case 4: deleteEmployee(); break;
            case 5: searchEmployee(); break;
            case 6: manageJobRole(); break;
            case 7: recordAttendance(); break;
            case 8: viewAttendance(); break;
            case 9: payrollProcessing(); break;
            case 10: addBonus(); break;
            case 11: addDeduction(); break;
            case 12: addSystemUser(); break;
            case 13: change_admin_manager_pass(user);break;
            case 14: printf("Logged out successfully.\n"); break;
            default: printf("Invalid choice.\n");
        }

    } while (choice != 14);
}
