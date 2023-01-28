// Miniature FTP System - Client Component

#include "myftp.h"

void getServResp(char *buffer, int socketfd) { 
	memset(buffer, 0, MAX_BUF_SIZE);  
    int i = 0;
    char input[1];
    while(1) {
    	read(socketfd,input,1); 
            if(DEBUG) { printf("Read %c from socket\n",input[0]);}
            if(input[0] != '\n') {
            	buffer[i] = input[0];
                    i++;
            }
            else {
            	break;
            }
    }
    buffer[i] = '\0';
}

void parsePath(char *path, char *file) { 
	int trace = 0; 
	int i = 0;
                       
        while(path[i] != '\0') {
        	if(path[i] == '/') {
                	trace = i+1;
                	if(DEBUG) { printf("trace at position %d\n",trace);}
                }
                i++;
        }
        i = 0;
        while(path[trace] != '\0') { 
        	if(DEBUG) { printf("Copying %c to file\n",path[trace]);}
                file[i] = path[trace];
                if(DEBUG) { printf("File is '%s'\n",file);}
                i++;
                trace++;
        }
        file[i] = '\0';
}

int getDataConn(char *buffer, char *argv[], int socketfd, int socketfd2) {  
	if(DEBUG) { printf("<MFTP> sending to server: D\n");}
        write(socketfd,"D\n",2);

	getServResp(buffer, socketfd);
        if(DEBUG) { printf("read %s from server\n", buffer);}
	
	if(buffer[0] == 'E') {
		fprintf(stderr,"Error getting port number : %d, %s\n",errno,strerror(errno));
		exit(1);
	}
	else {
        	
    	char prt[10];   
    	int i = 1, portNum; 
    	while(buffer[i] != '\0') {  
    		prt[i-1] = buffer[i];
            	i++;
    	}
    	prt[i-1] = '\0';
    	if(DEBUG) { printf("Port for data connection is : %s\n",prt);}
    	portNum = atoi(prt);
    	if(DEBUG) { printf("Creating socket with port '%d'\n",portNum);}

    	if( (socketfd2 = socket( AF_INET, SOCK_STREAM, 0)) < 0) { 
    		perror("data socket");
            	exit(1);
    	}
    	if(DEBUG) { printf("Data connection socket created\n");}

		struct sockaddr_in servAddr_B;
		struct hostent *hostEntry_B;
		struct in_addr **dbPtr_B;

		memset(&servAddr_B, 0,sizeof(servAddr_B));
    	servAddr_B.sin_family = AF_INET;
    	servAddr_B.sin_port = htons(portNum);

    	if( (hostEntry_B = gethostbyname(argv[1])) == NULL ) {
    		herror("gethostbyname");
            	exit(1);
    	}

    	dbPtr_B = (struct in_addr**) hostEntry_B->h_addr_list;
    	memcpy(&servAddr_B.sin_addr, *dbPtr_B,sizeof(struct in_addr));

    	if( (connect(socketfd2, (struct sockaddr *) &servAddr_B,sizeof(servAddr_B))) < 0 ) {
    		perror("connect");
            	exit(1);
    	}
    	if(DEBUG) { printf("Connected through socket file descriptor %d\n", socketfd2);}
			return(socketfd2);
		}
}

int isReg(const char *path) { 
    struct stat pathStat;  
    stat(path, &pathStat); 
    return S_ISREG(pathStat.st_mode); 
                                      
}

int main(int argc, char **argv) {
	if(argc < 2) { 
		printf("Invalid connection format, format is: <executable> <hostname/address>\n");
		exit(1);
	}
	
	int socketfd, socketfd2;                        

	if( (socketfd = socket( AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		exit(1);
	}

	struct sockaddr_in servAddr;
	struct hostent *hostEntry;
	struct in_addr **dbPtr;

	memset(&servAddr, 0,sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons(MY_PORT_NUMBER);
	

	if( (hostEntry = gethostbyname(argv[1])) == NULL ) {
		herror("gethostbyname");
		exit(1);
	}

	dbPtr = (struct in_addr**) hostEntry->h_addr_list;
	memcpy(&servAddr.sin_addr, *dbPtr,sizeof(struct in_addr));

	// Initialize server connection
	if( (connect(socketfd, (struct sockaddr *) &servAddr,sizeof(servAddr))) < 0 ) {
		perror("connect");
		exit(1);
	}
	if(DEBUG) { printf("Connected through socketfd %d\n", socketfd);}

	char buffer[4096];
    char arg1[10], arg2[1000], *tok;
    int bytesRead, tokCount = 0; 
	
	write(1, "<MFTP> ", 7);
	while( (bytesRead = read(0, buffer, 100)) > 0 ) { 
		buffer[bytesRead-1] = '\0'; 
		if(DEBUG) { printf("User entered: %s\n", buffer); }

   		tok = strtok(buffer, " \t\r\n\v\f"); 
   		while( tok != NULL && tokCount != 2) { 
      			if(tokCount == 0) {  
				strcpy(arg1, tok);
				if(DEBUG) { printf("Command: <%s> args: ", tok);}
				tokCount++;
			}
			else { 
				strcpy(arg2, tok);
				if(DEBUG) { printf("%s", tok);}
				tokCount++;
			}
      			tok = strtok(NULL, " ");
   		}
		tokCount = 0;
		fflush(stdout);

		if(strcmp(arg1, "exit") == 0) {
			if(DEBUG) { printf("'exit' command recieved, exiting server\n"); }
			write(socketfd,"Q\n",2);
			getServResp(buffer, socketfd);
        		printf("Read %s from server\n",buffer);
		        
			if(buffer[0] == 'E') {
				fprintf(stderr,"Server exit error\n");
			}	
			fflush(stdout);
			break;  
		}
		
		else if(strcmp(arg1, "cd") == 0) {
			char directory[100];
			int errFlag = 0;
                	if( getcwd(directory,sizeof(directory)) == NULL ) {
                        	perror("getcwd");
                	}
			if(DEBUG) { printf("'cd' command entered, CWD is  %s\n",directory);
			            printf("attempting to change CWD to %s\n",arg2); }
			if(access(arg2,F_OK) != 0) { 
                                perror("access error");
				errFlag = 1;
        		}
        		if(access(arg2,R_OK) != 0) { 
                                perror("access readable");
				errFlag = 1;
			}
			if(chdir(arg2) == -1) {
                        	perror("chdir");
				errFlag = 1;
	                }
			if(errFlag == 1) {
				if(DEBUG) { printf("Error failed to change directory\n"); }
				fprintf(stderr,"Error changing directories : %d, %s\n",errno,strerror(errno));
			}
			else {
				if(DEBUG) { printf("Directory changed to %s\n",arg2);}
			}
			printf("<MFTP> ");
			fflush(stdout);
		}

		else if(strcmp(arg1, "rcd") == 0) {
			if(DEBUG) { printf("'rcd' command intercepted %s\n",arg2); }
			char path[1024];
			sprintf(path,"C%s\n",arg2);
			if(DEBUG) { printf("Transmitting to server: %s",path);
			            printf("Transmitting %ld bytes to server\n",strlen(path));}
			write(socketfd,path,strlen(path));

			getServResp(buffer, socketfd);
			printf("Recieved %s from server\n",buffer);

			printf("<MFTP> ");
			fflush(stdout);
		}
		
		else if(strcmp(arg1, "ls") == 0) {
			if(DEBUG) { printf("'ls' command intercepted\n");}

			int fd[2]; 
	                int rdr,wtr;

			if(pipe(fd) == -1) { 
        		perror("pipe");
	         }
			rdr = fd[0]; 
            wtr = fd[1]; 
			
			int cpid = fork();

			if(cpid == 0) {
				int gpid = fork();  
				if(gpid == 0) {
					if(DEBUG) { 
						printf("execlp: ls -la: flags: show hidden, long form\n");
					}
					close(1);  
                    close(rdr);
                    if(dup2(wtr,1) == -1) { 
	                        perror("dup2");
        	        }
                	execlp("ls","ls","-la",NULL); 
                	perror("execlp failed");   
                	exit(1);
				}
				else {
					if(DEBUG) { printf("execlp: more: -d -20\n");}
					close(0);
       		        close(wtr);
                	if(dup2(rdr,0) == -1) {
                        	perror("dup2");
                	}
                	execlp("more","more","-d","-20",NULL); 
                	perror("execlp failed");
                	exit(1);
				}
			}
			else { 
				close(rdr);
				close(wtr);
				wait(NULL);
			}
			printf("<MFTP> ");
			fflush(stdout);
		}
		else if(strcmp(arg1, "rls") == 0) {
			if(DEBUG) { printf("'rls' command entered, attempting data connection\n");}
			socketfd2 = getDataConn(buffer, argv, socketfd, socketfd2); 
			write(socketfd,"L\n",2);
			if(DEBUG) { printf("Waiting on server response\n");}
			
			getServResp(buffer, socketfd);
			printf("Read %s from server\n", buffer);
			
			if(buffer[0] != 'E') { 
				if(fork()) {
					wait(NULL);
				}
				else {
					close(0); 
					dup2(socketfd2,0);
					execlp("more","more","-d","-20",NULL); 
                	perror("execlp more failed");
                	exit(1);
				}
			}
			if(DEBUG) { printf("Closing data connection\n");}
			close(socketfd2);
                        printf("<MFTP> ");
                        fflush(stdout);
		}
		else if(strcmp(arg1, "show") == 0) {
			if(DEBUG) { printf("'show' command entered, attempting command %s\n",arg2);
			            printf("Attempting to establish data connection\n");}
			socketfd2 = getDataConn(buffer, argv, socketfd, socketfd2);

            char path[1026];
            sprintf(path,"G%s\n",arg2);
            if(DEBUG) { printf("Transmitting to server: %s",path);
            printf("Transmitting  %ld bytes to server\n",strlen(path));}
            write(socketfd,path,strlen(path));
			if(DEBUG) { printf("Waiting on server response\n");}
			getServResp(buffer, socketfd);
			printf("Read %s from server\n", buffer);
			
			if(buffer[0] != 'E') {
				if(fork()) {
                	wait(NULL);
                        	}
                        	else {
                            	close(0);
                            	dup2(socketfd2,0);
                            	execlp("more","more","-d","-20",NULL); 
                            	perror("execlp more failed");  
                            	exit(1);
                        	}
			}
			else { 
				fprintf(stderr,"Error displaying pathname\n");
			}

            if(DEBUG) { printf("Closing data connection\n");}
            close(socketfd2);
            printf("<MFTP> ");
            fflush(stdout);
		}
		else if(arg1[0] == 'g' && arg1[1] == 'e' && arg1[2] == 't') {
			if(DEBUG) { printf("'get' command entered, attempting command %s\n",arg2);
            printf("Attempting to establish data connection\n");}
            socketfd2 = getDataConn(buffer, argv, socketfd, socketfd2);

            char path[1024];
            sprintf(path,"G%s\n",arg2);
            if(DEBUG) { printf("Transmitting to server: %s",path);
                        printf("Transmitting %ld bytes to server\n",strlen(path));}
            write(socketfd,path,strlen(path));
            if(DEBUG) { printf("Waiting on server response\n");}
                        
			getServResp(buffer, socketfd);
            printf("Read %s from server\n", buffer);

            if(buffer[0] != 'E') {
				if(DEBUG) { printf("Path is '%s'\n",arg2);}
				char file[1000];

				parsePath(arg2,file); 

				if(DEBUG) { printf("File is '%s'\n",file);}
				
				int fp, i = 0;
				if( (fp = open(file,O_WRONLY | O_CREAT | O_TRUNC,0755)) < 0 ) {
					fprintf(stderr,"Error opening file: %d, %s\n",errno,strerror(errno));
				}
				else {
					i = 0;
					memset(buffer, 0, 4096);
					while(1) {
						i = read(socketfd2,buffer,1);
						if(i <= 0) {
							break;
						}	
						else {
							if(write(fp,buffer,1) < 0) {
								fprintf(stderr,"Write Error: %d, %s\n",errno,strerror(errno));
								break;
							}
						}
					}
					close(fp);
					if(DEBUG) { printf("Done writing to file %s\n",file);}
				}
            }
			else { 
				fprintf(stderr,"Error retrieving pathname\n");
			}
			if(DEBUG) { printf("Closing data connection\n");}
            close(socketfd2);
            printf("<MFTP> ");
            fflush(stdout);
		}

		else if(arg1[0] == 'p' && arg1[1] == 'u' && arg1[2] == 't') {
			if(DEBUG) { printf("'put' command entered, attempting command with %s\n",arg2);}
			
			if(isReg(arg2) != 1) {  
				fprintf(stderr,"Error path is not a regular file or does not exist locally\n");
			}
			else {  
				int fp;
				if((fp = open(arg2,O_RDONLY)) < 0) {
                        		fprintf(stderr,"Error opening path: %d, %s\n",errno,strerror(errno));
            	}  
        		else { 
					if(DEBUG) { printf("Attempting data connection\n");}
                                        socketfd2 = getDataConn(buffer, argv, socketfd, socketfd2); 

					char path[1026];
					char file[1024];

					if(DEBUG) { printf("Path is %s\n",arg2);}
					parsePath(arg2,file); 
                	sprintf(path,"P%s\n",file); 
                	if(DEBUG) { printf("Transmitting to server: %s",path);
        		    printf("Transmitting %ld bytes to server\n",strlen(path));}
        			write(socketfd,path,strlen(path));  
        			if(DEBUG) { printf("Waiting on server response\n");}
                        		
					getServResp(buffer, socketfd); 
            		printf("Read %s from server\n", buffer); 

					if(buffer[0] != 'E') {
	            		if(DEBUG) { printf("Writing file to server\n");}
	            		char buffer[100];
	            		int b;
	            		while(1) {
	            			b = read(fp,buffer,100); 
	                		if(b <= 0) {
	                			break;
	                		}
	                		else {
	                			if( write(socketfd2,buffer,b) < 0) { 
								fprintf(stderr,"Write Error: %d, %s\n",errno,strerror(errno));
	                        	break;
								}
								if(DEBUG) { printf("Transmitted %d bytes to server\n",b);}
	                		}
	            		}
	            		if(DEBUG) { printf("Done writing file to server\n"); }
					}
					else {
						fprintf(stderr,"Error putting pathname, could not be opened or created\n");
					}
					close(fp);
					if(DEBUG) { printf("Closing data connection\n");}
                                        close(socketfd2);
                }
			}
            printf("<MFTP> ");
            fflush(stdout);
		}
		else {
			printf("Invalid input! \
			Recognized commands are: exit, cd <path>, \
			rcd <path>, ls, rls,\n show <path>, get <path>, put <path>\n");
			printf("<MFTP> ");
			fflush(stdout);
		}
	}
	printf("<MFTP> Client quitting...\n");
	close(socketfd);
	exit(0);
}
