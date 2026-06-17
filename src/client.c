#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "common.h"

static int send_and_receive(int socket_fd, Request *request, Response *response)
{
    if (write(socket_fd, request, sizeof(*request)) != sizeof(*request))
        return -1;
    if (read(socket_fd, response, sizeof(*response)) != sizeof(*response))
        return -1;
    return 0;
}

int main()
{
    int socket_fd;
    struct sockaddr_in address;
    Request request;
    Response response;
    char account_no[ACCOUNT_NO_LENGTH + 1];
    char pin[PIN_LENGTH + 1];
    int choice;
    long long amount;

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    address.sin_family = AF_INET;
    address.sin_port = PORT;
    inet_pton(AF_INET, "127.0.0.1", &address.sin_addr);

    connect(socket_fd, (struct sockaddr *)&address, sizeof(address));

    printf("=== ATM ===\n");
    printf("Account number: ");
    scanf("%8s", account_no);
    printf("PIN: ");
    scanf("%4s", pin);

    while (1) {
        printf("\n1. Check balance\n");
        printf("2. Deposit\n");
        printf("3. Withdraw\n");
        printf("4. Exit\n");
        printf("Choice: ");
        if (scanf("%d", &choice) != 1)
            break;

        memset(&request, 0, sizeof(request));
        strcpy(request.account_no, account_no);
        strcpy(request.pin, pin);

        switch (choice) {
        case 1:
            request.command = CMD_BALANCE;
            break;
        case 2:
            request.command = CMD_DEPOSIT;
            printf("Deposit amount: ");
            scanf("%lld", &amount);
            request.amount = amount;
            break;
        case 3:
            request.command = CMD_WITHDRAW;
            printf("Withdrawal amount: ");
            scanf("%lld", &amount);
            request.amount = amount;
            break;
        case 4:
            request.command = CMD_EXIT;
            break;
        default:
            printf("Invalid choice.\n");
            continue;
        }

        if (send_and_receive(socket_fd, &request, &response) < 0)
            break;

        printf("Server: %s\n", response.message);
        if (response.status == STATUS_OK && request.command == CMD_BALANCE)
            printf("Balance: %lld\n", (long long)response.balance);

        if (choice == 4)
            break;
    }

    close(socket_fd);

    return 0;
}
