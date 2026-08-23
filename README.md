# Tcp From Scratch

Server TCP scritto da zero in C, senza librerie di rete: solo syscall POSIX.

## Perché esiste

È un progetto di studio, non una libreria da usare.

L'obiettivo è quello di capire come funziona il server:

- Cosa fa davvero `socket()`
- Perché serve `bind()`
- Differenze tra file descriptor e connessioni
- Passaggio da un flusso di byte a un protocollo

## Stato

**Stadio 0 completato.** Il server accetta una connessione alla volta, legge dal
socket finché il client non chiude, e risponde a ogni blocco di dati ricevuto.

## Requisiti

- **Linux.** Il progetto usa l'API socket POSIX, su Windows nativo non compila.
- `gcc` e `make`
- `nc` (netcat)

## Esecuzione

```bash
make        # compila
./server    # resta in ascolto sulla porta 1101
```

## Test

```bash
nc localhost 1101
```

## Struttura

|File               |Descrizione                                 |
|-------------------|--------------------------------------------|
|`server.c`         |tutto il codice, con i commenti di studio   |
|`Makefile`         |compilazione; produce anche `server.s`      |
|`.clang_format`    |stile del codice - LLVM, 4 spazi, 80 colonne|
|`compile_flags.txt`|i flag di compilazione letti da clangd      |
|`CLAUDE.md`        |regole di lavoro per l'assistente AI        |

## Licenza

Nessun licenza: tutti i diritti riservati.
Il codice è pubblico per essere letto, non per essere riusato.

