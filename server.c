#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main(void) {
    /*
     * socket() crea un nuovo socket e restituisce un file descriptor.
     *
     * AF_INET: usa la famiglia di indirizzi IPv4.
     *
     * SOCK_STREAM: crea un socket orientato a uno stream di byte. Con AF_INET
     * il protocollo utilizzato è TCP.
     *
     * 0: lascia al kernel la scelta del protocollo appropriato per la
     * combinazione AF_INET + SOCK_STREAM.
     *
     * Il file descriptor restituito identifica il socket all'interno del
     * processo, analogamente a come un file descriptor identifica un file
     * aperto.
     */
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    if (fd == -1) {
        perror("socket");
        return 1;
    }

    /*
     * sockaddr_in rappresenta un indirizzo socket IPv4.
     *
     * I campi principali sono:
     *
     *  sin_family  ->  famiglia dell'indirizzo
     *  sin_port    ->  porta
     *  sin_addr    ->  indirizzo IPv4
     */
    struct sockaddr_in addr;

    /*
     * Azzera l'intera struttura prima di inizializzare i campi.
     *
     * Questo evita di lasciare valori indeterminati negli eventuali byte di
     * padding o nei campi che non impostiamo esplicitamente.
     */
    memset(&addr, 0, sizeof addr);

    addr.sin_family = AF_INET;

    /*
     * le porte nei protocolli di rete vengono memorizzate in network byte
     * order, convenzionalmente big-endian.
     *
     * htons = Host TO Network Short
     *
     * Converte un intero a 16 bit dal byte order della macchina al network byte
     * order.
     */
    addr.sin_port = htons(1101);

    /*
     * INADDR_ANY significa che il server accetta connessioni destinate a
     * qualsiasi indirizzo IPv4 locale della macchina.
     *
     * Corrisponde concettualmente a:
     *
     *   0.0.0.0:1101
     *
     * htonl = Host TO Network Long
     *
     * Converte un valore a 32 bit nel network byte order.
     */
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    const int yes = 1;

    /*
     * SO_REUSEADDR permette di riutilizzare l'indirizzo locale anche quando
     * esistono connessioni TCP precedenti che il sistema operativo non ha
     * ancora completamente eliminato.
     *
     * È particolarmente utile durante lo sviluppo, quando il server viene
     * terminato e riavviato frequentemente.
     */
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1) {
        perror("sockopt");
        return 1;
    }

    /*
     * bind() associa il socket all'indirizzo locale configurato:
     *
     *   0.0.0.0:1101
     *
     * bind() accetta un puntatore a struct sockaddr, che rappresenta il tipo
     * generico utilizzato dalle API socket.
     *
     * sockaddr_in è invece la struttura specifica per IPv4, quindi il suo
     * indirizzo viene convertito in struct sockaddr *.
     */
    if (bind(fd, (struct sockaddr *)&addr, sizeof addr) == -1) {
        perror("bind");
        return 1;
    }

    /*
     * listen() trasforma il socket in un listening socket: da questo momento fd
     * viene utilizzato per ricevere nuove richieste di connessione.
     *
     * 16 è il backlog richiesto: il numero di connessioni che il kernel può
     * tenere in attesa mentre il programma non ha ancora eseguito accept().
     */
    if (listen(fd, 16) == -1) {
        perror("listen");
        return 1;
    }

    printf("\n\nfd = %d\n", fd);
    printf("\nsizeof addr = %zu\n", sizeof addr);

    /*
     * Il listening socket continua a esistere per tutta la vita del server.
     * Dopo aver terminato la comunicazione con un client, torniamo quindi ad
     * accept() per aspettarne un altro.
     */
    while (1) {
        /*
         * accept() può restituirci anche l'indirizzo remoto del client che si è
         * appena connesso.
         */
        struct sockaddr_in client_addr;

        /*
         * client_addr_len è un parametro input/output.
         *
         * Prima di accept():
         *    indica quanto spazio è disponibile in client_addr.
         *
         * Dopo accept():
         *    contiene la dimensione dell'indirizzo scritto dal kernel.
         *
         * Viene quindi reinizializzato prima di ogni accept().
         */
        socklen_t client_addr_len = sizeof client_addr;

        /*
         * accept() attende una nuova connessione.
         *
         * fd rimane il listening socket.
         *
         * client_fd è invece un NUOVO file descriptor che rappresenta una
         * specifica connessione TCP.
         *
         * Quindi:
         *
         *    fd          -> riceve nuove connessioni
         *    client_fd   -> comunica con un singolo client
         */
        int client_fd =
            accept(fd, (struct sockaddr *)&client_addr, &client_addr_len);

        if (client_fd == -1) {
            /*
             * Il valore di ritorno segnala che la chiamata è fallita, errno ne
             * contiene il motivo. Non è una variabile globale ma una macro, che
             * dà accesso a una locazione propria di ogni thread.
             *
             * Il kernel la scrive al ritorno di una chiamata fallita, ma
             * nessuno la azzera mai. Qualunque chiamata successiva può
             * sovrascriverla, anche se non fallisce. Il valore viene quindi
             * copiato subito, prima di qualsiasi altra operazione.
             */
            int err = errno;

            switch (err) {
                /*
                 * Errori che riguardano una singola connessione.
                 *
                 * ECONNABORTED: il client ha completato l'handshake ed è stato
                 * messo nella coda del listening socket, poi se n'è andato
                 * prima che accept() lo prendesse in carico.
                 *
                 * EPERM: regole di firewall hanno vietato la connessione.
                 *
                 * EPROTO: errore di protocollo sulla connessione.
                 *
                 * In questi tre casi il server è sano e si continua ad
                 * accettare richieste.
                 */
                case ECONNABORTED:
                case EPERM:
                case EPROTO:
                    perror("accept");
                    continue;

                /*
                 * La sezione ERRORS di man 2 accept dichiara esplicitamente
                 * che possono arrivare errori di rete propri del nuovo socket,
                 * e che kernel diversi ne restituiscono altri ancora.
                 */
                default:
                    perror("accept");
                    return 1;
            }
        }

        /*
         * L'indirizzo IP dentro sockaddr_in è memorizzato in formato binario,
         * non come stringa "127.0.0.1".
         *
         * INET_ADDRSTRLEN indica lo spazio necessario per rappresentare un
         * indirizzo IPv4 in formato testuale, incluso il terminatore '\0'.
         */
        char client_ip[INET_ADDRSTRLEN];

        /*
         * inet_ntop = Network TO Presentation
         *
         * Converte l'indirizzo IPv4 binario contenuto in:
         *
         *    client_addr.sin_addr
         *
         * nella sua rappresentazione leggibile:
         *
         *    "127.0.0.1"
         */
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof client_ip);

        /*
         * sin_port è ancora in network byte order.
         *
         * ntohs = Network TO Host Short
         *
         * La porta mostrata qui è normalmente una ephemeral port, scelta
         * automaticamente dal sistema operativo del client.
         *
         * Una connessione TCP è identificata dalla cosiddetta 4-tuple:
         *
         *    client IP
         *    client port
         *    server IP
         *    server port
         *
         * Ad esempio:
         *
         *    127.0.0.1:31456 -> 127.0.0.1:1101
         */
        printf(
            "\nNuova connessione:\n\tclient_fd = %d\n\tport = %d\n\tip = %s\n",
            client_fd,
            ntohs(client_addr.sin_port),
            client_ip);

        const char res[] = "\n[SERVER]: Dati ricevuti\n\n";

        /*
         * Una singola connessione TCP può trasportare dati più volte,
         * quindi continuiamo a leggere dallo stesso client_fd finché il client
         * non chiude la connessione oppure si verifica un errore.
         */
        while (1) {
            char buf[1024];

            /*
             * read() prova a leggere al massimo sizeof buf byte dallo stream
             * TCP.
             *
             * TCP è uno STREAM DI BYTE, non un protocollo basato su messaggi.
             *
             * Questo significa che una read() non corrisponde necessariamente a
             * una write() eseguita dal client.
             *
             * Il valore restituito da read() significa:
             *
             *    > 0  numero di byte letti
             *      0  il peer ha chiuso la connessione
             *     -1  errore
             */
            ssize_t n_read = read(client_fd, buf, sizeof buf);

            if (n_read == -1) {
                perror("read");
                close(client_fd);
                break;
            }

            /*
             * read() == 0 indica EOF sul socket.
             *
             * In questo caso il client ha chiuso ordinatamente la propria
             * estremità della connessione.
             */
            if (n_read == 0) {
                close(client_fd);
                break;
            }

            /*
             * read() restituisce byte, non una stringa C.
             *
             * buf quindi non è necessariamente terminato dal carattere '\0'.
             *
             * %.*s permette di specificare esplicitamente quanti caratteri
             * printf deve leggere da buf.
             */
            printf("\n[CLIENT] %.*s", (int)n_read, buf);

            /*
             * write() prova a scrivere byte sul socket.
             *
             * sizeof res comprende anche il '\0' finale aggiunto
             * automaticamente alle stringhe C.
             *
             * Non vogliamo trasmettere quel terminatore, quindi utilizziamo:
             *
             *    sizeof res - 1
             *
             * IMPORTANTE:
             *
             * write() non garantisce in generale di scrivere tutti i byte
             * richiesti con una singola chiamata.
             *
             * Per ora controlliamo solamente gli errori.
             * Gestiremo successivamente le "partial writes".
             */
            ssize_t n_write = write(client_fd, res, sizeof res - 1);

            if (n_write == -1) {
                perror("write");
                close(client_fd);
                break;
            }
        }
    }

    return 0;
}
