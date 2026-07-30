#include <stdio.h>
#include <argon2.h>
#include "all_header.h"



void clearScreen(void) {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void clearInputBuffer() {
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF);
}

void initializeSystem() {
    FILE *fp;

    fp = fopen(ADMINMANAGER, "r");
    if (fp == NULL) {
        fp = fopen(ADMINMANAGER, "w");
        if (fp == NULL) {
            printf("Error creating users file.\n");
            return;
        }

        // Configuration baselines matching your employee setup
        uint32_t t_cost = 3;
        uint32_t m_cost = 65536;
        uint32_t parallelism = 4;

        // Unique salts for both default system roles
        uint8_t admin_salt[16] = "AdminInitSalt123";
        uint8_t manager_salt[16] = "MngrInitSalt1234";

        char hashed_admin_password[128];
        char hashed_manager_password[128];

        // 1. Hash the default admin password ("admin123")
        int res_admin = argon2id_hash_encoded(t_cost, m_cost, parallelism, "admin123", 8, admin_salt, 16, 32, hashed_admin_password, sizeof(hashed_admin_password));

        // 2. Hash the default manager password ("manager123")
        int res_manager = argon2id_hash_encoded(t_cost, m_cost, parallelism, "manager123", 10, manager_salt, 16, 32, hashed_manager_password, sizeof(hashed_manager_password));

        if (res_admin != ARGON2_OK || res_manager != ARGON2_OK) {
            printf("Security Error: Failed to initialize default credentials.\n");
            fclose(fp);
            return;
        }

        fprintf(fp, "admin|admin@gmail.com|%s|%d\n", hashed_admin_password, ADMIN);
        fprintf(fp, "manager|manager@gmail.com|%s|%d\n", hashed_manager_password, MANAGER);

        fclose(fp);
    }
    else
        fclose(fp);

    fp = fopen(EMP_FILE, "a");
    if (fp) fclose(fp);

    fp = fopen(ATT_FILE, "a");
    if (fp) fclose(fp);
}

