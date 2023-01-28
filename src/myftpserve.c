// Miniature FTP System - Server Component

#include "myftp.h"

int isDir(const char *path) {
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISDIR(path_stat.st_mode);
}

int listenConn(){
	int listenfd;
	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) { 
		perror("socket");
		exit(1);
	}
	if (DEBUG) { printf("Server socket created with fd %d\n", listenfd);}

	struct sockaddr_in servAddr;  

	memset(&servAddr, 0, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons(MY_PORT_NUMBER);  
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(listenfd, (struct sockaddr*) &servAddr, sizeof(servAddr)) < 0) {  
		perror("bind");
		exit(1);
	}
	if (DEBUG) {printf("Socket bind success\n"); }

	if ((listen(listenfd, 4)) < 0 ) {  // permit a queue of 4 connection requests simultaneously
		perror("listen");
		exit(1);
	}
	if(DEBUG) { printf("Listening...\n"); }
	return listenfd;
}

int main(int argc, char **argv) {
	int listenfd = listenConn();
	int connectfd;   
	int datafd, clientdata_fd, dataBinary = 0;  
	int status;  
	int length = sizeof(struct sockaddr_in);  
	struct sockaddr_in clientAddr;

	while (1) {
		if ((connectfd = accept(listenfd, (struct sockaddr *) &clientAddr, &length)) < 0) {
	        perror("accept");
	        exit (1);
        }
		if (DEBUG) { printf("Client connection found with fd %d\n", connectfd);}

		int cpid = fork();
		if (cpid) {  
			close(connectfd);  
			int newpid = waitpid(-1, &status, 0);  
			if(DEBUG) { printf("Forking process %d to manage new connection\n", newpid);}
			wait(NULL);
			exit(0);
		}	
		else {  
        if (cpid) {
		exit(0);
		}
		else {
			struct hostent* hostEntry;
    		char* hostName;
    		if( (hostEntry = gethostbyaddr(&(clientAddr.sin_addr), \
    			sizeof(struct in_addr), AF_INET)) == NULL) {
				herror("gethostbyaddr");
				exit(1);
			}
    		hostName = hostEntry->h_name;
    		printf("Control connection established with: %s\n", hostName); 
			char input[1];      
    		char buffer[MAX_BUF_SIZE];  
			int bytesRead, breakBinary = 0;
				
			while (1) {
				int i = 0;
				while((bytesRead = read(connectfd, input, 1)) != EOF) {  
					if (bytesRead == 0) {  
						breakBinary = 1;  
						break;
					}
					if (input[0] != '\n') {  
						buffer[i] = input[0];  
						i++;
					}
					else {
						break;
					}
				}
				if (breakBinary == 1) {  
					printf("Client exited\n");
					break;
				}
				buffer[i] = '\0';   
				printf("Server reads: %s\n", buffer);
				
				if (buffer[0] == 'Q') {
					if (DEBUG) { printf("'Q' exit command recieved\n\n");}
					write(connectfd, "A\n", 2);
					break;
				}
				if (buffer[0] == 'C') {
					if (DEBUG) {printf("'C' chdir command recieved\n");}
                	char path[100];
                	int i = 1;
                	while (buffer[i] != '\0') {  
                    	path[i-1] = buffer[i];
                    	i++;
                	}
                	path[i-1] = '\0';

                	char dir[100];
                	int errBinary = 0;
                	if (getcwd(dir, sizeof(dir)) == NULL ) {  
                    	perror("getcwd");
                	}
                	if (DEBUG) { 
                		printf("Server CWD: %s\n", dir);
        	            printf("Attempting chdir to '%s'\n", path); 
        	        }
                	if (access(path, F_OK) != 0) { 
                    	perror("access error");  
                    	errBinary = 1;
                	}
                	if (access(path, R_OK) != 0) { 
                    	perror("access readable"); 
                    	errBinary = 1;
                	}
                	if (chdir(path) == -1) { 
                    	perror("chdir");
                    	errBinary = 1;
                	}
                	if (errBinary == 1) {  
                    	fprintf(stderr, "Error in chdir attempt\n");
                    	char errMsg[] = "E No such file or directory\n";
                    	write(connectfd, errMsg, strlen(errMsg));
                	}
                	else {  
                    	printf("chdir to %s successful\n", path);
                    	write(connectfd, "A\n", 2);
                	}
                	fflush(stdout);
				}

            	if(buffer[0] == 'D') {
                	if (DEBUG) {printf("'D' establish data connection command recieved\n");}
                	int errBinary = 0;
                	if ((datafd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) { 
                    	perror("data socket");
                    	errBinary = 1;
                	}
                	if (DEBUG) { printf("Data socket created with fd %d\n", datafd);}
                	
                	struct sockaddr_in dataAddr;
                	memset(&dataAddr, 0, sizeof(dataAddr));
					dataAddr.sin_family = AF_INET;
                    dataAddr.sin_addr.s_addr = htonl(INADDR_ANY);
                    dataAddr.sin_port = htons(0);  

                	if (bind(datafd, (struct sockaddr*) &dataAddr, sizeof(dataAddr)) < 0) {  
                    	perror("bind");
                    	errBinary = 1;
                	}
                	if (DEBUG) { printf("Socket bind successful\n"); }

                	struct sockaddr_in sin;
                	memset(&sin, 0, sizeof(sin));
                	socklen_t len = sizeof(sin);
                	if (getsockname(datafd, (struct sockaddr *)&sin, &len) == -1) {
                        	perror("getsockname");
                        	errBinary = 1;
                	}
                	if (DEBUG) { printf("Port number %d\n", ntohs(sin.sin_port));}
                	int socketNum = ntohs(sin.sin_port);
                	if (errBinary != 1) {  
                    	char servResp[10];
                    	sprintf(servResp, "A%d\n", socketNum);
						printf("Writing: '%s' to client\n", servResp);
                    	write(connectfd, servResp, strlen(servResp)); 
                	}
                	else {  
						fprintf(stderr, "Socket error\n");
						char errMsg[] = "E Could not create data connection\n";
                    	write(connectfd, errMsg, strlen(errMsg));
                	}
                	if ((listen(datafd, 0)) < 0) {
                        	perror("listen");
                	}

					if (DEBUG) { printf("Listening...\n");}
                	int lenB = sizeof(struct sockaddr_in);
                	struct sockaddr_in clientAddrB;
					if ((clientdata_fd = accept(datafd, (struct sockaddr *) &clientAddrB, &lenB)) < 0) {
                    	perror("accept");
                	}
                	if (DEBUG) {printf("Client connected with fd %d\n", clientdata_fd);}
						dataBinary = 1;  
                    }
				
				if (buffer[0] == 'L') {
					if (DEBUG) { printf("'L' list dir command recieved\n");}
					if (dataBinary == 0) {   
						char errMsg[] = "E No data connection created\n";
						write(connectfd, errMsg, strlen(errMsg));
						fprintf(stderr, "No data connection created: %d, %s\n", errno, strerror(errno));
					}
					else {  
						write(connectfd, "A\n", 2);
						if (DEBUG) { printf("Forking child process to exec 'ls -l'\n");}
						int cpid = fork();
						if (cpid) {
							close(clientdata_fd);
							wait(NULL);
						}
						else {
							close(1); 
							dup2(clientdata_fd, 1);  
							execlp("ls", "ls", "-la", NULL);  
							perror("execlp ls failed");
						}
						close(clientdata_fd);  
						close(datafd);  
						dataBinary = 0;  
					}
				}
				
				if(buffer[0] == 'G') {
					if (DEBUG) { printf("'G' get a file command recieved\n");}
					if (dataBinary == 0) {  
						printf("Sending E response\n");
						char errMsg[] = "E No data connection created\n";
						write(connectfd, errMsg, strlen(errMsg));
						fprintf(stderr, "No data connection created: %d, %s\n", errno, strerror(errno));
					}
					else {  
						char path[1000];  
						int i = 1;
						while (1) {
							if(buffer[i] != '\n') {
								path[i-1] = buffer[i];
								i++;	
							}
							else {
								break;
							}
						}

						path[i-1] = '\0';
						printf("path read from client is '%s'\n", path);
						int fp;

						if (DEBUG) {printf("isDir = %d\n", isDir(path));}		
						if (isDir(path) == 1) {  
							printf("Sending E response\n");
							char errMsg[] = "E File is a dir\n";
                        	write(connectfd, errMsg, strlen(errMsg));
							fprintf(stderr, "File is a dir: %d, %s\n", errno, strerror(errno));
						}
						else {  
							if ((fp = open(path, O_RDONLY)) < 0) {  
								fprintf(stderr, "open error : %d %s\n", errno, strerror(errno));
								printf("Sending E response\n");
								char errMsg[] = "E File doesn't exist or isn't readable\n";
								write(connectfd, errMsg, strlen(errMsg));
							}
							else {  
								printf("Sending A response\n");
								write(connectfd, "A\n", 2);
								if (DEBUG) { printf("Writing file to client!\n");}
								char buffer[100];
                        		int i;
                        		while (1) {
                            		i = read(fp, buffer, 100);
                            		if (i < 1) {  
                                		break;
        	                		}
                	        		else {
                    	        		if (write(clientdata_fd, buffer, i) < 0) {  
											fprintf(stderr, "Write error: %d %s\n", errno, strerror(errno));
											break;
									}
									if (DEBUG) {printf("Wrote %d bytes to client!\n", i);}
                        			}
                    			}
								if (DEBUG) {printf("Finished transmitting file to client\n");}
							}
						}
						close(fp);  
						close(clientdata_fd);  
                    	close(datafd); 
						dataBinary = 0;
					}
				}
				if (buffer[0] == 'P') {
					if (DEBUG) { printf("'P' put file command recieved\n"); }
                        if (dataBinary == 0) {  
                                printf("Sending E response\n");
                                char errMsg[] = "E No data connection was created\n";
                                write(connectfd, errMsg, strlen(errMsg));
                                fprintf(stderr, "No data connection created: %d, %s\n", errno, strerror(errno));
                        }
                        else {  
						printf("'%s' read from client\n", buffer);
						char path[1000];  
						int i = 1;
                        while (1) {
                                if (buffer[i] != '\n') {
                                	path[i-1] = buffer[i];
									i++;
								}
                                else {
                                    break;
                                }
                        }
                        path[i] = '\0';
                        if (DEBUG) {printf("Path from client is '%s'\n", path);}

						i = 0;
						int k = 0;
						char parsedPath[1000];

                        while (1) {
                            if (path[k] != '\0') {
								if (path[k] == '/') {
									i = 0;
									k++;
									memset(parsedPath, 0, 1000);
								}
								else {
	                            	parsedPath[i] = path[k];
	                            	i++;
									k++;
								}
                            }
                            else {
                                break;
                            }
                        }
                        parsedPath[i] = '\0';
						printf("Parsed filename is '%s'\n", parsedPath);
						if (access(path, F_OK) == 0) { 
                            perror("access error");  
                            char errMsg[] = "E file already exists\n";
							write(connectfd, errMsg, strlen(errMsg));
                        }
						else {  
							int fp;
		                	if ((fp = open(parsedPath, O_WRONLY | O_CREAT | O_TRUNC, 0755)) < 0) { 
								fprintf(stderr, "Could not open %s to be created: %d, %s\n", parsedPath, errno, strerror(errno));
								char errMsg[] = "E file could not be opened\n";
                            	write(connectfd, errMsg, strlen(errMsg));
            				}
							else {  
								write(connectfd, "A\n", 2);
	        					i = 0;
	        					char buffer[100];
	        					while (1) {
	            					i = read(clientdata_fd, buffer, 100);
	            					if (i <= 0) {
	                					break;
	            					}
	            					else {
	                					if (write(fp, buffer, i) < 0) {  
											fprintf(stderr, "Write error: %d %s\n", errno, strerror(errno));
	                                        break;
										}
										if (DEBUG) { printf("Writing %d bytes to server\n", i);}
	            					}
	        					}
	        					close(fp);  
	        					if (DEBUG) { printf("Finished writing to file %s\n", parsedPath);}
							}
						}
						close(clientdata_fd);  
                        close(datafd);   
                        dataBinary = 0;	
					}
				}
			}	
			printf("Child process quitting...\n");
			close(connectfd); 
			exit(0);  
			}
		}
	}
	return 0;
}
