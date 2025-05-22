#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <syslog.h>
#include <signal.h> 
#include <fcntl.h>   
#include <arpa/inet.h>
#include <errno.h>




#define PORT 9000
int server_fd = -1;  //Sever_ID
int new_socket = -1; //New client ID
const char *filename = "/var/tmp/aesdsocketdata";       //File refrence
int stop_requested = 0;

void signalHandler(int sig)
{
    stop_requested = 1;
       // Log: Caught signal, exiting
    syslog(LOG_INFO, "Caught signal, exiting");

    if (server_fd != -1) {
        shutdown(server_fd, SHUT_RDWR);  // Gracefully disable communication
    }

    if (new_socket != -1) {
        shutdown(new_socket, SHUT_RDWR); // Optional: shutdown active client socket
    }
    if (new_socket != -1) {
        close(new_socket);
    }
    if (server_fd != -1) {
        close(server_fd);
    }
    

}


int main(int argc, char const* argv[])
{
    int daemon_mode = 0;
    if (argc == 2 && strcmp(argv[1], "-d") == 0) //look at the arguments pass through main, this is will look like "./aesdsocket -d"
    {
        daemon_mode = 1;
    }

    openlog("aesdsocket", LOG_PID, LOG_USER);       //For loging
    signal(SIGINT, signalHandler);                  //Allow for ctrl+c to exit code.
    signal(SIGTERM, signalHandler);
    
    struct sockaddr_in server_addr;
    int bytes_recv;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    char buffer[1024] = { 0 };

    // Zero out the structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // Bind to all interfaces
    server_addr.sin_port = htons(PORT);       // Port number
    // socklen_t server_addr_len = sizeof(server_addr);

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) 
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 9000
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) 
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) == -1) 
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    //Fork after listen & bind to ensure the desinated PORT is secured.
    if(daemon_mode)
    {
        FILE *fp = fopen(filename, "w");  // 'w' mode truncates file to zero length
        if (fp == NULL) {
            perror("fopen for truncating file");
            return EXIT_FAILURE;
        }
        fclose(fp);
        printf("AOV, File has been cleared\r\n");

        pid_t pid = fork();
        if (pid < 0) 
        {
            perror("fork failed");
            exit(EXIT_FAILURE);
        }
        if (pid > 0) 
        {
            // Parent process exits
            exit(0);
        }
        if (setsid() == -1) {           //This detaches the daemon from the controlling terminal and makes it the session leader.
            perror("setsid failed");    //In simple terms it stops the child from getting affected by CTR+C. severs ties to our main process.
            exit(EXIT_FAILURE);
        }
         // Redirect standard file descriptors to /dev/null
        fclose(stdin);
        fclose(stdout);
        fclose(stderr);

        open("/dev/null", O_RDONLY); // stdin
        open("/dev/null", O_WRONLY); // stdout
        open("/dev/null", O_RDWR);   // stderr
    }

    while(!stop_requested)//check if we pressed ctr+c
    {
        printf("Waiting for a new client to connect...\n");

        //Connects to a new client
        if ((new_socket = accept(server_fd, (struct sockaddr*)&client_addr, &client_addr_len)) == -1) 
        {
            if (stop_requested) 
            {
                // We're shutting down, don't log or exit
                break;
            }
            if (errno == EINTR && stop_requested) 
            {
                break;  // Clean exit on signal interrupt
            }
            perror("accept");
            continue;  // Try accept again if not stopping
        }
        syslog(LOG_INFO, "Accepted connection from %s", inet_ntoa(client_addr.sin_addr));
        printf("AOV, New connection\r\n");
        // subtract 1 for the null
        // terminator at the end
        int leave_loop = 0;
        while(leave_loop != 1)
        {
            bytes_recv = recv(new_socket, buffer, 1024 - 1, 0); 
            if (bytes_recv == -1) 
            {
                perror("recv");
                close(new_socket);
                continue;
            }
            
            // printf("AOV : %s******************************\n", buffer);
            
            //AOV, you need to make a while loop that will look at each buffer char and cut the buffer short when recieves a \n. After, you will send it back to the client.
            for(int i = 0; i < bytes_recv; i++) 
            {
                if(buffer[i] == '\n')
                {
                    bytes_recv = i + 1;
                    leave_loop = 1;
                    printf("AOV, Endline hit\r\n");
                }
                if(i == 1024)
                {
                    bytes_recv = i + 1;
                }
                
            }
            char Trim_buffer[bytes_recv + 1];
            memcpy(Trim_buffer, buffer, bytes_recv);
            Trim_buffer[bytes_recv] = '\0'; // null-terminate

            //File write!!
            

            // Open file in append mode; create if it doesn't exist
            FILE *fp = fopen(filename, "a");
            
            if (fp == NULL) {
                perror("fopen");
                return EXIT_FAILURE;
            }

            // Write string to file
            if (fputs(Trim_buffer, fp) == EOF) {     //wrote buffer to the file.
                perror("fputs");
                fclose(fp);
                return EXIT_FAILURE;
            }
            // send(new_socket, Trim_buffer, sizeof(Trim_buffer), 0);

            fclose(fp); // Done writing
        }
 

        FILE *read_fp = fopen(filename, "r");       //open to read and send to client.

        //send file back to client
        char send_buf[1024];
        size_t n;
        
        while ((n = fread(send_buf, 1, sizeof(send_buf), read_fp)) > 0) 
        {
            send(new_socket, send_buf, n, 0);
        }

        fclose(read_fp);
        //End of File Write

        //add close message to log.
        syslog(LOG_INFO, "Closed connection from %s", inet_ntoa(client_addr.sin_addr));
        printf("AOV, close connection\r\n");
        // remove(filename);
        // printf("AOV, File has been removed\r\n");
        // closing the connected socket
        close(new_socket);
    }
    // Close any open sockets and connections.
    if(new_socket != -1)
    {
        printf("AOV, new socket has been removed\r\n");
        close(new_socket);
    }
    if (server_fd != -1) {
        close(server_fd);
    }
    
        // Delete /var/tmp/aesdsocketdata
    remove(filename);
    printf("AOV, File has been removed\r\n");
        // closing the listening socket
    close(server_fd);
        //Close log
    closelog();
    return 0;

    // send(new_socket, hello, strlen(hello), 0);
    // printf("Hello message sent\n");

}