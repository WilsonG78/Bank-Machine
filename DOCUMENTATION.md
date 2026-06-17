# ATM Simulator — Documentation

## Overview

An ATM simulator built on TCP network communication between a client (ATM terminal) and a server (bank system). The server handles multiple connections in parallel — each in a separate child process created with `fork()`.

Account balances are persisted in `data/accounts.txt` and survive server restarts.

## Architecture

```
┌─────────────┐    TCP (port 8080)    ┌─────────────┐
│  client.c   │ ◄──────────────────► │  server.c   │
│    (ATM)    │   Request / Response │   (bank)    │
└─────────────┘                      └─────────────┘
                                           │
                                    fork() per client
                                           │
                              flock() + accounts.txt
```

## Course concepts (`examples/`)

### 1. TCP sockets — `klient.c`, `serwer.c`

| Function | Example | Usage in project |
|----------|---------|------------------|
| `socket(AF_INET, SOCK_STREAM, 0)` | `klient.c`, `serwer.c` | Create TCP socket |
| `connect()` | `klient.c` | Client connects to server |
| `bind()`, `listen()`, `accept()` | `serwer.c` | Server listens and accepts |
| `read()`, `write()` | `klient.c`, `serwer.c` | Send binary structures |
| `close()` | `klient.c`, `serwer.c` | Close connection |
| `inet_pton()` | `klient.c` | Server IP address |
| `INADDR_ANY` | `serwer.c` | Listen on all interfaces |

### 2. `fork()` — `procesy.c`

After `accept()`, the server creates a child process (`fork()`). The parent closes the client socket and returns to listening; the child handles the session and exits (`exit(0)`). `signal(SIGCHLD, SIG_IGN)` prevents zombie processes.

### 3. File locking — `flock()`

Because `fork()` gives each child its own memory copy, balances are stored in `data/accounts.txt`. `flock(LOCK_EX)` ensures only one process reads or writes at a time, keeping data consistent across child processes.

### 4. `pthread_mutex` — `blokada.c`

The mutex pattern from `blokada.c` is analogous to `flock()` — both protect a shared resource from concurrent access.

### 5. Pipes — `potoki.c`

Pipes (`pipe()`, `read()`, `write()`) illustrate IPC. This project uses TCP sockets and file I/O instead, but the idea of passing data between processes is the same.

### 6. Semaphores — `semafor.c`

Semaphores (`sem_open`, `sem_wait`, `sem_post`) synchronize processes. `flock()` plays a similar role here for the accounts file.

### 7. Message queues — `nadawca.c`, `odbiorca.c`

`msgget`, `msgsnd`, `msgrcv` — alternative IPC. This project uses TCP sockets to pass structures between processes.

### 8. Threads — `watki.c`

`pthread_create`, `pthread_join` — thread creation. The locking mechanism is the same as used for cross-process file access.

## Communication protocol (`src/common.h`)

- **Request** — command, account number, PIN, amount (client → server)
- **Response** — status, balance, text message (server → client)

| Command | Code | Description |
|---------|------|-------------|
| Balance | `CMD_BALANCE` (1) | Check account balance |
| Deposit | `CMD_DEPOSIT` (2) | Deposit funds |
| Withdraw | `CMD_WITHDRAW` (3) | Withdraw funds |
| Exit | `CMD_EXIT` (4) | End session |

## Persistent storage (`data/accounts.txt`)

Plain-text file, one account per line:

```
account_number pin balance
```

Example:

```
12345678 1234 1000
87654321 4321 2500
```

The server loads this file on startup (creates it with defaults if missing) and saves after every deposit or withdrawal. You can inspect balances directly:

```bash
cat data/accounts.txt
```

## Test accounts

| Account number | PIN | Initial balance |
|----------------|-----|-----------------|
| 12345678 | 1234 | 1000 |
| 87654321 | 4321 | 2500 |
| 11223344 | 1111 | 5000 |
| 44332211 | 2222 | 750 |
| 99887766 | 3333 | 12000 |
| 55443322 | 4444 | 300 |
| 66778899 | 5555 | 8900 |
| 22113344 | 6666 | 4200 |

## Running

```bash
make
./server          # terminal 1
./client          # terminal 2
```

After transactions, check persisted balances:

```bash
cat data/accounts.txt
```

Restart the server and balances will be restored from the file.

## Security (educational)

- PIN verification before every operation
- Insufficient funds block withdrawals
- Amounts must be positive
