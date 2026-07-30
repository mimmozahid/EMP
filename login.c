#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <argon2.h>
#include "all_header.h"


#define MAX_LOGIN_USERS 100

#define MAX_ADMIN_USERS 100

AdminAttempt adminAttempts[MAX_ADMIN_USERS];

int attempt;

EmployeeAttempt employeeAttempts[1000];

AdminAttempt adminAttempts[100];

int getAdminAttemptIndex(const char *username) {
    for (int i = 0; i < MAX_ADMIN_USERS; i++) {
        if (strcmp(adminAttempts[i].username, username) == 0) {
            return i;
        }
    }
    return -1;
}

int getAdminAttempt(const char *username) {
    int index = getAdminAttemptIndex(username);
    if (index == -1) return 0;
    return adminAttempts[index].attempts;
}

void increaseAdminAttempt(const char *username) {
    int index = getAdminAttemptIndex(username);

    if (index == -1) {
        for (int i = 0; i < MAX_ADMIN_USERS; i++) {
            if (adminAttempts[i].username[0] == '\0') {
                strncpy(adminAttempts[i].username, username, sizeof(adminAttempts[i].username) - 1);
                adminAttempts[i].attempts = 1;
                return;
            }
        }
    } else {
        adminAttempts[index].attempts++;
    }
}

void resetAdminAttempt(const char *username) {
    int index = getAdminAttemptIndex(username);
    if (index != -1) {
        adminAttempts[index].attempts = 0;
    }
}


void updateAdminRecord(AdminManager updatedUser) {
    FILE *fp = fopen(ADMINMANAGER, "r");
    FILE *temp = fopen("temp_admin.csv", "w");

    AdminManager user;
    char line[512];

    if (fp == NULL || temp == NULL) {
        printf("Error opening admin file.\n");
        if (fp != NULL) fclose(fp);
        if (temp != NULL) fclose(temp);
        return;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        line[strcspn(line, "\r\n")] = '\0';

        char *token;

        // 1. Username
        token = strtok(line, "|");
        if (token == NULL) continue;
        strcpy(user.username, token);

        // 2. Email
        token = strtok(NULL, "|");
        if (token == NULL) continue;
        strcpy(user.email, token);

        // 3. Password
        token = strtok(NULL, "|");
        if (token == NULL) continue;
        strcpy(user.password, token);

        // 4. Role
        token = strtok(NULL, "|");
        if (token == NULL) continue;
        user.role = atoi(token);

        // Replace entry if username matches
        if (strcmp(user.username, updatedUser.username) == 0)
        {
            fprintf(temp, "%s|%s|%s|%d\n", updatedUser.username, updatedUser.email, updatedUser.password, updatedUser.role);
        }
        else
        {
            fprintf(temp, "%s|%s|%s|%d\n", user.username, user.email, user.password, user.role);
        }
    }

    fclose(fp);
    fclose(temp);

    remove(ADMINMANAGER);
    rename("temp_admin.csv", ADMINMANAGER);
}

void resetAdminPassword(AdminManager user) {
    char newPassword[100];
    char confirmPassword[100];

    printf("Enter New Password: ");
    scanf("%99s", newPassword);

    if (create_strong_password (newPassword))
    {
        printf ("Strength: Strong\n");
    }
    else
    {
        printf ("Strength: Weak\n");
        return;
    }

    printf("Confirm Password: ");
    scanf("%99s", confirmPassword);

    if (strcmp(newPassword, confirmPassword) != 0) {
        printf("Passwords do not match.\n");
        return;
    }

    // Initialize RNG for Salt
    srand((unsigned int)time(NULL));
    uint8_t salt[16];
    for (int i = 0; i < 16; i++) {
        salt[i] = rand() % 256;
    }

    // Standard Argon2id configuration baselines
    uint32_t t_cost = 3;
    uint32_t m_cost = 65536;
    uint32_t parallelism = 4;

    char hash[200];
    int res = argon2id_hash_encoded(t_cost, m_cost, parallelism, confirmPassword, strlen(confirmPassword), salt, sizeof(salt), 32, hash, sizeof(hash));

    if (res != ARGON2_OK) {
        printf("Password hashing failed: %s\n", argon2_error_message(res));
        return;
    }

    strcpy(user.password, hash);
    updateAdminRecord(user);

    printf("Password reset successfully.\n");
}

int verifyAdminEmail(AdminManager user) {
    char email[50];

    printf("Verify Email : ");
    scanf("%49s", email);

    // FIXED: Properly comparing typed email with user structure's email field
    return (strcmp(user.email, email) == 0);
}
int getEmployeeAttempt(int empId)
{
    int index = getEmployeeAttemptIndex(empId);

    if(index != -1)
        return employeeAttempts[index].attempts;

    return 0;
}

int getEmployeeAttemptIndex(int empId)
{
    int i;

    for(i = 0; i < MAX_LOGIN_USERS; i++)
    {
        if(employeeAttempts[i].id == empId)
            return i;
    }

    for(i = 0; i < MAX_LOGIN_USERS; i++)
    {
        if(employeeAttempts[i].id == 0)
        {
            employeeAttempts[i].id = empId;
            employeeAttempts[i].attempts = 0;
            return i;
        }
    }

    return -1;
}

void increaseEmployeeAttempt(int empId)
{
    int index = getEmployeeAttemptIndex(empId);

    if(index != -1)
        employeeAttempts[index].attempts++;
}

void resetEmployeeAttempt(int empId)
{
    int index = getEmployeeAttemptIndex(empId);

    if(index != -1)
        employeeAttempts[index].attempts = 0;
}

int verifyDOB(Employee emp)
{
    char dob[11];

    printf("\nIdentity Verification\n");
    printf("Enter your Date of Birth (DD/MM/YYYY): ");
    scanf("%10s", dob);

    if(strcmp(emp.dateofbirth, dob) == 0)
        return 1;

    return 0;
}

void resetEmployeePassword(Employee emp)
{
    char newPass[100];
    char confirmPass[100];

    printf("\nCreate New Password: ");
    scanf("%99s", newPass);

    if (create_strong_password (newPass))
    {
        printf ("Strength: Strong\n");
    }
    else
    {
        printf ("Strength: Weak\n");
        return;
    }

    printf("Confirm Password: ");
    scanf("%99s", confirmPass);

    if(strcmp(newPass, confirmPass) != 0)
    {
        printf("Passwords do not match.\n");
        return;
    }

    uint8_t salt[16];
    for (int i = 0; i < 16; i++) {
        salt[i] = rand() % 256;
    }

    uint32_t t_cost = 3;
    uint32_t m_cost = 65536;
    uint32_t parallelism = 4;

    char hash[256];
    int res = argon2id_hash_encoded(t_cost, m_cost, parallelism, confirmPass, strlen(confirmPass), salt, 16, 32, hash, sizeof(hash));

    if (res != ARGON2_OK) {
        printf("Security Error: Failed to hash the reset password (%s).\n", argon2_error_message(res));
        return;
    }

    strcpy(emp.password, hash);

    updateEmployeeRecord(emp);

    printf("Password changed successfully.\n");
}

int login (AdminManager *loggedInUser, int *loggedInEmpId)
{
    int choice;
    
    printf("\n========== LOGIN ==========\n");
    printf("1. Admin Login\n");
    printf("2. Manager Login\n");
    printf("3. Employee Login\n");
    printf("4. EXIT system\n");
    
    printf("Enter choice: ");
    scanf("%d", &choice);
    
    if (choice == 1 || choice == 2)
    {
        FILE *fp = fopen(ADMINMANAGER, "r");
        char username[30], password[100];
        AdminManager user;

        if (!fp)
        {
            printf("Admin & Manager file not found.\n");
            return 0;
        }

        if (choice == 1)
            printf("--------- ADMIN ---------\n");
        else if (choice == 2)
            printf("--------- MANAGER ---------\n");

        printf("Username: ");
        scanf("%29s", username);
        printf("Password: ");
        scanf("%99s", password);

        char line[350];
        while (fgets(line, sizeof(line), fp) != NULL)
        {
            line[strcspn(line, "\r\n")] = '\0';

            char *token;

            // 1. Username
            token = strtok(line, "|");
            if (token == NULL) continue;
            strcpy(user.username, token);

            // 2. Email
            token = strtok(NULL, "|");
            if (token == NULL) continue;
            strcpy(user.email, token);

            // 3. Password (Argon2id Hash)
            token = strtok(NULL, "|");
            if (token == NULL) continue;
            strcpy(user.password, token);

            // 4. Role
            token = strtok(NULL, "|");
            if (token == NULL) continue;
            user.role = atoi(token);

            // FIXED: Enforce that the account's role matches the chosen menu option
            int requiredRole = (choice == 1) ? ADMIN : MANAGER;

            if (strcmp(user.username, username) == 0 && user.role == requiredRole)
            {
                int verify_result = argon2id_verify(user.password, password, strlen(password));

                if (verify_result == ARGON2_OK)
                {
                    resetAdminAttempt(username); // Reset attempts on successful login
                    *loggedInUser = user;
                    *loggedInEmpId = 0;
                    fclose(fp);
                    return 1;
                }
            }
        }

        fclose(fp); // Close file if match isn't found or password fails

        increaseAdminAttempt(username);
        int attempt = getAdminAttempt(username);

        if (attempt < 3)
        {
            clearScreen();
            printf("Invalid Username or Password.\n");
            printf("Remaining Attempts : %d\n", 3 - attempt);
            return 0;
        }

        // Maximum login attempts reached workflow
        int option;
        printf("\n====================================\n");
        printf("Maximum login attempts reached.\n");
        printf("1. Forgot Password\n");
        printf("2. Try Again\n");
        printf("Choose : ");
        scanf("%d", &option);

        if (option == 1) {
            FILE *fp2 = fopen(ADMINMANAGER, "r");

            if (fp2 == NULL) {
                printf("File not found.\n");
                return 0;
            }

            char emailInput[50];
            printf("Enter Email : ");
            scanf("%49s", emailInput);

            char lineBuffer[350];
            AdminManager resetUser;

            while (fgets(lineBuffer, sizeof(lineBuffer), fp2)) {
                lineBuffer[strcspn(lineBuffer, "\r\n")] = '\0';

                char *token;

                // 1. Username
                token = strtok(lineBuffer, "|");
                if (token == NULL) continue;
                strcpy(resetUser.username, token);

                // 2. Email (FIXED: Previously skipped in parsing)
                token = strtok(NULL, "|");
                if (token == NULL) continue;
                strcpy(resetUser.email, token);

                // 3. Password
                token = strtok(NULL, "|");
                if (token == NULL) continue;
                strcpy(resetUser.password, token);

                // 4. Role
                token = strtok(NULL, "|");
                if (token == NULL) continue;
                resetUser.role = atoi(token);

                // FIXED: Look up matching email instead of matching email against username
                if (strcmp(resetUser.email, emailInput) == 0)
                {
                    if (verifyAdminEmail(resetUser))
                    {
                        resetAdminPassword(resetUser);
                        resetAdminAttempt(resetUser.username);
                        
                    }
                    else
                    {
                        printf("Email verification failed.\n");
                    }

                    fclose(fp2);
                    clearScreen();
                    return 0;
                }
            }

            fclose(fp2);
            clearScreen();
            printf("Email not found.\n");
            return 0;
        } else {
            resetAdminAttempt(username);
            clearScreen();
            return 0;
        }
    }
    else if (choice == 3)
    {
        FILE *fp = fopen(EMP_FILE, "r");
        int empId;
        char password[100]; // Expanded to hold standard inputs safely
        Employee emp;

        int flg = 0;

        if (fp == NULL)
        {
            printf("Employee file not found.\n");
            return 0;
        }

        printf("Employee ID: ");
        scanf("%d", &empId);

        printf("Password: ");
        scanf("%99s", password);

        // Increased line array size since hashed text strings are significantly longer 
        char line[512]; 

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

            // Password (This token extracts the Argon2id string format from file)
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

            // --- SECURE ARGON2ID VERIFICATION VERDICT ---
            if (emp.id == empId)
            {
                // argon2id_verify returns ARGON2_OK (value 0) upon a successful password match
                int verify_result = argon2id_verify(emp.password, password, strlen(password));
                
                if (verify_result == ARGON2_OK)
                {
                    strcpy(loggedInUser->username, emp.name);
                    strcpy(loggedInUser->password, emp.password);
                    loggedInUser->role = EMPLOYEE;
                    *loggedInEmpId = emp.id;

                    flg = 1;

                    fclose(fp);
                    clearScreen();
                    return 1;
                }
            }
        }

        if(!flg)
        {
            increaseEmployeeAttempt(empId);

            int attempt = getEmployeeAttempt(empId);

            if(attempt < 3)
            {
                fclose(fp);

                clearScreen();

                printf("Invalid Employee ID or Password.\n");
                printf("Remaining Attempts : %d\n", 3 - attempt);

                return 0;
            }

            fclose(fp);

            int option;

            printf("\n====================================\n");
            printf("Maximum login attempts reached.\n");
            printf("1. Forgot Password\n");
            printf("2. Try Again\n");
            printf("Choose : ");
            scanf("%d", &option);

            if(option == 1)
            {
                FILE *fp2 = fopen(EMP_FILE, "r");

                if(fp2 == NULL)
                {
                    printf("Employee file not found.\n");
                    return 0;
                }

                printf("Enter Employee ID : ");
                scanf("%d", &empId);

                char line[512];

                while(fgets(line, sizeof(line), fp2))
                {
                    line[strcspn(line, "\n")] = '\0';

                    char *token;

                    token = strtok(line, "|");
                    if(token == NULL) continue;
                    emp.id = atoi(token);

                    token = strtok(NULL, "|");
                    if(token == NULL) continue;
                    strcpy(emp.name, token);

                    token = strtok(NULL, "|");
                    if(token == NULL) continue;
                    strcpy(emp.email, token);

                    token = strtok(NULL, "|");
                    if(token == NULL) continue;
                    strcpy(emp.dateofbirth, token);

                    token = strtok(NULL, "|");
                    if(token == NULL) continue;
                    strcpy(emp.position, token);

                    token = strtok(NULL, "|");
                    if(token == NULL) continue;
                    emp.baseSalary = atof(token);

                    token = strtok(NULL, "|");
                    if(token == NULL) continue;
                    strcpy(emp.password, token);

                    token = strtok(NULL, "|");
                    if(token ==NULL) continue;
                    emp.bonus = atof(token);

                    token = strtok(NULL, "|");
                    if(token == NULL) continue;
                    emp.deduction = atof(token);

                    if(emp.id == empId)
                    {
                        if(verifyDOB(emp))
                        {
                            resetEmployeePassword(emp);

                            // Password changed successfully
                            resetEmployeeAttempt(empId);
                        }
                        else
                        {
                            printf("Date of Birth does not match.\n");
                        }

                        fclose(fp2);
                        clearScreen();
                        return 0;
                    }
                }

                fclose(fp2);

                clearScreen();
                printf("Employee ID not found.\n");
                return 0;
            }
            else
            {
                resetEmployeeAttempt(empId);

                clearScreen();
                return 0;
            }
        }

        fclose(fp);
        clearScreen();
        // printf("Invalid Employee ID or Password.\n");
        return 0;
    }
    else if (choice == 4)
    {
        AdminManager user;
        char name[10] = "nothing";
        strcpy (user.username, name);
        char pass[10] = "nothing";
        strcpy (user.password, pass);

        user.role = EXIT;

        *loggedInUser = user;
        *loggedInEmpId = 0;
        clearScreen();
        
        return 3;
    }
    else
    {
        printf("Invalid login option.\n");
        return 0;
    }
}
