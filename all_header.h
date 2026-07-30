#ifndef all_header_h
#define all_header_h


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define ll long long

#define ADMIN 1
#define MANAGER 2
#define EMPLOYEE 3
#define EXIT 4

#define EMP_FILE "employees.csv"
#define ADMINMANAGER "AdminManager.csv"
#define ATT_FILE "attendance.csv"
#define LEAVE_FILE "leave.csv"




typedef struct {
    int id;
    char name[50];
    char email[50];
    char dateofbirth[20];
    char position[50];
    float baseSalary;
    char password[130];
    float bonus;
    float deduction;
} Employee;

typedef struct {
    char username[30];
    char email[30];
    char password[200];
    int role;
} AdminManager;

typedef struct {
    int empId;
    int day, month, year;
    int status;   //! 1 = Present, 0 = Absent
} Attendance;


typedef struct
{
    int empId;
    char applyDate[11];   // DD/MM/YYYY
    char fromDate[11];    // DD/MM/YYYY
    char toDate[11];      // DD/MM/YYYY
    char reason[200];
    int status;           // 0 = Pending, 1 = Approved, 2 = Rejected
} Leave;

typedef struct
{
    int id;
    int attempts;
} EmployeeAttempt;

typedef struct
{
    char username[30];
    int attempts;
} AdminAttempt;

typedef struct
{
    int id;
    int attempts;
} LoginAttempt;

extern EmployeeAttempt employeeAttempts[1000];

extern AdminAttempt adminAttempts[100];

void clearScreen(void);
void initializeSystem();
void clearInputBuffer();


int create_strong_password(char pass[]);
int generateEmployeeId();
int isValidEmail(const char *email);
void addEmployee();
int verifyDOB(Employee emp);
void updateEmployeeRecord(Employee updatedEmp);
void resetEmployeePassword(Employee emp);
void update_admin_manager_pass(AdminManager updateAd);
void change_admin_manager_pass (AdminManager user);
void viewEmployees();
int usernameExists(const char *username);
void addSystemUser();
Employee getEmployeeById(int id, int *found);
void updateEmployee();
int employeeExists(int id);
int attendanceAlreadyExists(int empId, int day, int month, int year);
void recordAttendance();
void deleteEmployeeAttendance(int empId);
void deleteEmployee();
int countPresentDays(int empId);
float calculateSalary(Employee emp);
void searchEmployee();
void manageJobRole();
void viewAttendance();
void payrollProcessing();
void addBonus();
void addDeduction();
void viewProfile(int empId);
void viewOwnAttendance(int empId);
void updateOwnProfile(int empId);
void applyLeave(int empId);
void viewLeaveStatus(int empId);
void approveLeaveRequest();

void changeEmployeePassword(int empId);

int login (AdminManager *loggedInUser, int *loggedInEmpId);
void employeeMenu(int empId);
void managerMenu(AdminManager user);
void adminMenu(AdminManager user);
int getEmployeeAttemptIndex(int empId);
void increaseEmployeeAttempt(int empId);
int getEmployeeAttempt(int empId);
void resetEmployeeAttempt(int empId);

int getAdminAttemptIndex(const char *username);

int getAdminAttempt(const char *username);

void increaseAdminAttempt(const char *username);

void resetAdminAttempt(const char *username);

void resetAdminPassword(AdminManager user);

void updateAdminRecord(AdminManager user);

#endif