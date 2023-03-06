#define _FILE_OFFSET_BITS 64
#include <cstdio>
#include <cstdlib>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include <sstream>
#include <iostream>
#define BUFLEN 512
using std::cin;
using std::string;
typedef long long ll;
int main(int argc, char const *argv[])
{
    char ip[20]="";
    int svr_port;
    setbuf(stdout,NULL);
    for(int i=0;argv[1][i];i++)
    {
        if(argv[1][i]==':')
        {
            svr_port=atoi(argv[1]+i+1);
            break;
        }
        ip[i]=argv[1][i];
    }
    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        fprintf(stderr,"\n Socket creation error \n");
        return -1;
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(svr_port);
       
    // Convert IPv4 and IPv6 addresses from text to binary form
    if(inet_pton(AF_INET, ip, &serv_addr.sin_addr)<=0) 
    {
        fprintf(stderr,"\nInvalid address/ Address not supported \n");
        return -1;
    }
   
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        fprintf(stderr,"\nConnection Failed \n");
        return -1;
    }
    
    struct stat sta;
    if (stat("./client_dir", &sta) == -1) {
        mkdir("./client_dir", 0700);
    }
    char buf[BUFLEN+10]="";
    char buf1[BUFLEN+10]="";
    
    printf("input your username:\n");
    string username;
    while(getline(cin,username))
    {
        sprintf(buf,"4%s",username.c_str());
        send(sock,buf,strlen(buf),0);
        read(sock,buf,1);
        if(buf[0]=='+')
        {
            printf("connect successfully\n");
            break;
        }
        printf("username is in used, please try another:\n");
    }

    while(1)
    {
        string inA;
        string in;
        string _;
        getline(cin,in);
        std::stringstream ss;
        ss<<in;
        ss>>inA;
        if(inA=="ls")
        {
            if(ss>>_)
            {
                printf("Command format error\n");
                continue;
            }
            sprintf(buf,"1");
            send(sock,buf,strlen(buf),0);
            int mes_len;
            buf[read(sock,buf,10)]=0;
            mes_len=atoi(buf);
            for(int i=0;i<mes_len;)
            {
                int rlen=0;
                if(mes_len-i<BUFLEN)rlen=read(sock,buf,mes_len-i);
                else rlen=read(sock,buf,BUFLEN);
                buf[rlen]=0;
                fprintf(stdout,"%s",buf);
                i+=rlen;
            }
        }
        else if(inA=="put")
        {
            string filename;
            if(ss>>filename)
            {
                if(ss>>_)
                {
                    printf("Command format error\n");
                    continue;
                }
                string ofilename=filename;
                filename="./client_dir/"+filename;
                FILE *fp=fopen(filename.c_str(),"r");
                if(fp==NULL)
                {
                    printf("The %s doesn't exist\n",ofilename.c_str());
                    continue;
                }
                sprintf(buf,"2%s",ofilename.c_str());
                send(sock,buf,strlen(buf),0);
                read(sock,buf,1);
                
                fseeko(fp, 0L, SEEK_END);
                ll filesize = ftello(fp);
                fseeko(fp, 0L, SEEK_SET);
                sprintf(buf,"%20lld",filesize);
                send(sock,buf,20,0);
                
                while(1)
                {
                    int rlen=fread(buf,sizeof(char),BUFLEN,fp);
                    if(rlen==0)
                    {
                         fclose(fp);
                         break;
                    }
                    else
                    {
                        send(sock,buf,rlen,0);
                    }
                }
                read(sock,buf,1);
                printf("put %s successfully\n",ofilename.c_str());
            }
            else
            {
                printf("Command format error\n");
                continue;
            }
        }
        else if(inA=="get")
        {
            string filename;
            if(ss>>filename)
            {
                if(ss>>_)
                {
                    printf("Command format error\n");
                    continue;
                }
                sprintf(buf,"3%s",filename.c_str());
                send(sock,buf,strlen(buf),0);
                read(sock,buf,1);
                buf[1]=0;
                if(buf[0]=='-')
                {
                    printf("The %s doesn't exist\n",filename.c_str());
                }
                else 
                {
                    buf[read(sock,buf,20)]=0;
                    ll mes_len=atoll(buf);
                    string ofilename=filename;
                    filename="./client_dir/"+filename;
                    FILE *fp=fopen(filename.c_str(),"w");
                    for(ll i=0;i<mes_len;)
                    {
                        int rlen=0;
                        if(mes_len-i<BUFLEN)rlen=read(sock,buf,mes_len-i);
                        else rlen=read(sock,buf,BUFLEN);
                        buf[rlen]=0;
                        fwrite(buf,sizeof(char),rlen,fp);
                        i+=rlen;
                    }
                    fclose(fp);
                    read(sock,buf,1);
                    printf("get %s successfully\n",ofilename.c_str());
                }
            }
            else
            {
                printf("Command format error\n");
                continue;
            }
        }
        else
        {
            printf("Command not found\n");
            continue;
        }
    }

    return 0;
}
/*
send(sock,s,strlen(s),0);
*/
