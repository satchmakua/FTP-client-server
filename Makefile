all : myftp myftpserve

myftp : myftp.c
	cc -o myftp myftp.c
     
myftpserve : myftpserve.c
	cc -o myftpserve myftpserve.c



