#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

#define PORT 8080
#define MAX_ACCOUNTS 16
#define BUFFER_SIZE 1024
#define PIN_LENGTH 4
#define ACCOUNT_NO_LENGTH 8
#define ACCOUNTS_FILE "data/accounts.txt"

typedef enum {
    CMD_BALANCE = 1,
    CMD_DEPOSIT = 2,
    CMD_WITHDRAW = 3,
    CMD_EXIT = 4
} Command;

typedef struct {
    char account_no[ACCOUNT_NO_LENGTH + 1];
    char pin[PIN_LENGTH + 1];
    int64_t balance;
} Account;

typedef struct {
    Command command;
    char account_no[ACCOUNT_NO_LENGTH + 1];
    char pin[PIN_LENGTH + 1];
    int64_t amount;
} Request;

typedef struct {
    int status;
    int64_t balance;
    char message[256];
} Response;

#define STATUS_OK 0
#define STATUS_ERROR 1

#endif
