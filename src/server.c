#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/file.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "common.h"

static void create_default_accounts_file(void)
{
    FILE *file = fopen(ACCOUNTS_FILE, "w");

    if (!file) {
        perror("fopen");
        exit(1);
    }

    fprintf(file, "# account_number pin balance\n");
    fprintf(file, "12345678 1234 1000\n");
    fprintf(file, "87654321 4321 2500\n");
    fprintf(file, "11223344 1111 5000\n");
    fprintf(file, "44332211 2222 750\n");
    fprintf(file, "99887766 3333 12000\n");
    fprintf(file, "55443322 4444 300\n");
    fprintf(file, "66778899 5555 8900\n");
    fprintf(file, "22113344 6666 4200\n");

    fclose(file);
}

static int load_accounts(Account accounts[], int max_count)
{
    FILE *file;
    char line[128];
    int count = 0;

    file = fopen(ACCOUNTS_FILE, "r");
    if (!file)
        return -1;

    while (fgets(line, sizeof(line), file) && count < max_count) {
        if (line[0] == '#' || line[0] == '\n')
            continue;

        if (sscanf(line, "%8s %4s %lld",
                   accounts[count].account_no,
                   accounts[count].pin,
                   (long long *)&accounts[count].balance) == 3)
            count++;
    }

    fclose(file);
    return count;
}

static int save_accounts(const Account accounts[], int count)
{
    FILE *file;
    int i;

    file = fopen(ACCOUNTS_FILE, "w");
    if (!file)
        return -1;

    fprintf(file, "# account_number pin balance\n");
    for (i = 0; i < count; i++)
        fprintf(file, "%s %s %lld\n",
                accounts[i].account_no,
                accounts[i].pin,
                (long long)accounts[i].balance);

    fclose(file);
    return 0;
}

static Account *find_account(Account accounts[], int count, const char *account_no)
{
    int i;

    for (i = 0; i < count; i++) {
        if (strcmp(accounts[i].account_no, account_no) == 0)
            return &accounts[i];
    }

    return NULL;
}

static void handle_request(const Request *request, Response *response)
{
    Account accounts[MAX_ACCOUNTS];
    int count;
    Account *account;
    int lock_fd;
    int changed = 0;

    memset(response, 0, sizeof(*response));

    lock_fd = open(ACCOUNTS_FILE, O_RDONLY);
    if (lock_fd < 0) {
        response->status = STATUS_ERROR;
        snprintf(response->message, sizeof(response->message),
                 "Cannot open accounts file.");
        return;
    }

    flock(lock_fd, LOCK_EX);

    count = load_accounts(accounts, MAX_ACCOUNTS);
    if (count < 0) {
        response->status = STATUS_ERROR;
        snprintf(response->message, sizeof(response->message),
                 "Cannot read accounts file.");
        flock(lock_fd, LOCK_UN);
        close(lock_fd);
        return;
    }

    account = find_account(accounts, count, request->account_no);
    if (!account) {
        response->status = STATUS_ERROR;
        snprintf(response->message, sizeof(response->message),
                 "Account does not exist.");
        flock(lock_fd, LOCK_UN);
        close(lock_fd);
        return;
    }

    if (strcmp(account->pin, request->pin) != 0) {
        response->status = STATUS_ERROR;
        snprintf(response->message, sizeof(response->message), "Invalid PIN.");
        flock(lock_fd, LOCK_UN);
        close(lock_fd);
        return;
    }

    switch (request->command) {
    case CMD_BALANCE:
        response->status = STATUS_OK;
        response->balance = account->balance;
        snprintf(response->message, sizeof(response->message),
                 "Account balance: %lld", (long long)account->balance);
        break;

    case CMD_DEPOSIT:
        if (request->amount <= 0) {
            response->status = STATUS_ERROR;
            snprintf(response->message, sizeof(response->message),
                     "Amount must be positive.");
        } else {
            account->balance += request->amount;
            changed = 1;
            response->status = STATUS_OK;
            response->balance = account->balance;
            snprintf(response->message, sizeof(response->message),
                     "Deposit %lld OK. Balance: %lld",
                     (long long)request->amount, (long long)account->balance);
        }
        break;

    case CMD_WITHDRAW:
        if (request->amount <= 0) {
            response->status = STATUS_ERROR;
            snprintf(response->message, sizeof(response->message),
                     "Amount must be positive.");
        } else if (account->balance < request->amount) {
            response->status = STATUS_ERROR;
            snprintf(response->message, sizeof(response->message),
                     "Insufficient funds.");
        } else {
            account->balance -= request->amount;
            changed = 1;
            response->status = STATUS_OK;
            response->balance = account->balance;
            snprintf(response->message, sizeof(response->message),
                     "Withdrawal %lld OK. Balance: %lld",
                     (long long)request->amount, (long long)account->balance);
        }
        break;

    case CMD_EXIT:
        response->status = STATUS_OK;
        snprintf(response->message, sizeof(response->message), "Goodbye.");
        break;

    default:
        response->status = STATUS_ERROR;
        snprintf(response->message, sizeof(response->message), "Unknown command.");
        break;
    }

    if (changed && save_accounts(accounts, count) < 0) {
        response->status = STATUS_ERROR;
        snprintf(response->message, sizeof(response->message),
                 "Failed to save account balances.");
    }

    flock(lock_fd, LOCK_UN);
    close(lock_fd);
}

static void serve_client(int client_socket)
{
    Request request;
    Response response;

    while (1) {
        if (read(client_socket, &request, sizeof(request)) != sizeof(request))
            break;

        handle_request(&request, &response);
        write(client_socket, &response, sizeof(response));

        if (request.command == CMD_EXIT)
            break;
    }

    close(client_socket);
}

int main()
{
    struct sockaddr_in address;
    int server_socket, client_socket;
    pid_t pid;

    signal(SIGCHLD, SIG_IGN);

    if (access(ACCOUNTS_FILE, F_OK) != 0)
        create_default_accounts_file();

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = PORT;

    bind(server_socket, (struct sockaddr *)&address, sizeof(address));
    listen(server_socket, 5);

    printf("Bank server listening on port %d...\n", PORT);
    printf("Balances stored in %s\n", ACCOUNTS_FILE);

    while (1) {
        client_socket = accept(server_socket, (struct sockaddr *)NULL, NULL);
        printf("Server connected a client.\n");

        pid = fork();
        if (pid == 0) {
            close(server_socket);
            serve_client(client_socket);
            exit(0);
        }

        close(client_socket);
    }

    close(server_socket);

    return 0;
}
