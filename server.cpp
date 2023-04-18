#define _FILE_OFFSET_BITS 64
#include <unistd.h>
#include <cstdio>
#include <sys/socket.h>
#include <cstdlib>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <cstring>
#include <iostream>
#include <vector>
#include <cerrno>
#include <set>
#define BUFLEN 512
using std::vector;
using std::max;
using std::min;
using std::swap;
using std::string;
typedef long long ll;
struct Client
{
    FILE *fp;
    int fd;
    int status;
    ll remain;
	string name="";
    Client(int _fd)
    {
        fd=_fd;
        fp=0;
        status=0;
        remain=0;
    }
};
int main(int argc, char const *argv[])
{
    int server_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
       
    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
       
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(atoi(argv[1]));
       
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    struct stat sta;
    if (stat("./server_dir", &sta) == -1) {
        fprintf(stderr,"success\n");
        mkdir("./server_dir", 0700);
    }

    char buf[BUFLEN+10]="";
    char buf1[BUFLEN+10]="";
    vector<Client> clients;
    std::set<std::string> username;
    while(1)
    {
        fd_set select_setr,select_setw;
        FD_ZERO(&select_setr);
        int maxfd=server_fd;
        FD_SET(server_fd,&select_setr);
        for(Client i:clients)
        {
            maxfd=max(i.fd,maxfd);
            FD_SET(i.fd,&select_setr);
        }
        select_setw=select_setr;
        FD_CLR(server_fd,&select_setw);
        if(select(maxfd+1,&select_setr,&select_setw,NULL,NULL)>0)
        {
            if(FD_ISSET(server_fd,&select_setr))
            {
                int conn_fd=accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
                if (conn_fd < 0) {
                    if (errno == EINTR || errno == EAGAIN) continue;  // try again
                    if (errno == ENFILE) {
                        fprintf(stderr, "out of file descriptor table ... (maxconn %d)\n", maxfd);
                        continue;
                    }
                    perror("listen");
                    exit(EXIT_FAILURE);
                }
                clients.emplace_back(Client(conn_fd));
            }
            for(int i=(int)clients.size()-1;i>=0;i--)
            {
		if(FD_ISSET(clients[i].fd,&select_setr))
		{
                    int rlen=recv(clients[i].fd,buf,1,MSG_PEEK);
                    if(rlen==0)
                    {    
                    	username.erase(clients[i].name);
                    	close(clients[i].fd);
                    	swap(clients[i],clients.back());
                    	clients.pop_back();
                    	continue;
                    }
		}
                if(clients[i].status==0)
                {
                    if(FD_ISSET(clients[i].fd,&select_setr))
                    {
                        int rlen=read(clients[i].fd,buf,1);
                        if(rlen==0)continue;
                        buf[1]=0;
                        fprintf(stderr,"read:[%s] from client %d(fd=%d)\n",buf,i,clients[i].fd);
                        if(buf[0]=='1')
                        {
                            // ls
                            struct dirent **namelist;
                            int dirsize=scandir("./server_dir", &namelist, 0, alphasort);
                            int mes_len=0;
                            
                            for(int j=0;j<dirsize;j++)
                            {
                                string tname(namelist[j]->d_name);
                                if(tname!="."&&tname!="..")
                                {
                                    sprintf(buf,"%s\n", namelist[j]->d_name);
                                    mes_len+=strlen(buf);
                                }
                            }
                            sprintf(buf,"%10d",mes_len);
                            send(clients[i].fd,buf,10,MSG_NOSIGNAL);
                            for(int j=0;j<dirsize;j++)
                            {
                                string tname(namelist[j]->d_name);
                                if(tname!="."&&tname!="..")
                                {
                                    sprintf(buf,"%s\n", namelist[j]->d_name);
                                    send(clients[i].fd,buf,strlen(buf),MSG_NOSIGNAL);
                                }
                                free(namelist[j]);
                            }
                            free(namelist);
                        }
                        else if(buf[0]=='2')
                        {
                            // put
                            rlen=read(clients[i].fd,buf,BUFLEN);
							if(rlen==0)continue;
							buf[rlen]=0;
                            send(clients[i].fd,"+",1,MSG_NOSIGNAL);
                            sprintf(buf1,"./server_dir/%s",buf);
                            clients[i].fp=fopen(buf1,"w");
                            clients[i].status=21;
                        }
                        else if(buf[0]=='3') 
                        {
                            // get 
                            rlen=read(clients[i].fd,buf,BUFLEN);
							if(rlen==0)continue;
							buf[rlen]=0;
                            sprintf(buf1,"./server_dir/%s",buf);
                            clients[i].fp=fopen(buf1,"r");
                            
                            if(clients[i].fp==NULL)
                            {
                                send(clients[i].fd,"-",1,MSG_NOSIGNAL);
                                clients[i].status=0;
                            }
                            else
                            {
                                send(clients[i].fd,"+",1,MSG_NOSIGNAL);
                                fseeko(clients[i].fp, 0L, SEEK_END);
                                ll filesize = ftello(clients[i].fp);
                                fseeko(clients[i].fp, 0L, SEEK_SET);
                                sprintf(buf,"%20lld",filesize);
                                send(clients[i].fd,buf,20,MSG_NOSIGNAL);
                                clients[i].status=3;
                            }
                        }
                        else if(buf[0]=='4')
                        {
							rlen=read(clients[i].fd,buf,BUFLEN);
							if(rlen==0)continue;
							buf[rlen]=0;
                            std::string newuser(buf);
                            if(username.find(newuser)==username.end())
                            {
                                send(clients[i].fd,"+",1,MSG_NOSIGNAL);
                                username.insert(newuser);
								clients[i].name=newuser;
                            }
                            else send(clients[i].fd,"-",1,MSG_NOSIGNAL);
                        }
                    }
                }
                else if(clients[i].status==21) // read put file length
                {
                    if(FD_ISSET(clients[i].fd,&select_setr))
                    {
                    	int rlen=read(clients[i].fd,buf,20);
                    	if(rlen==0)
                    	{
                    		fclose(clients[i].fp);
                            clients[i].fp=0;
                            continue;
						}
                        buf[rlen]=0;
                        clients[i].remain=atoll(buf);
                        clients[i].status=22;
                    }
                }
                else if(clients[i].status==22)
                {
                    if(FD_ISSET(clients[i].fd,&select_setr))
                    {
                        int rlen=read(clients[i].fd,buf,min((ll)BUFLEN,clients[i].remain));
                        if(rlen==0)
                        {
							fclose(clients[i].fp);
                            clients[i].fp=0;
                            continue;
						}
                        buf[rlen]=0;
                        clients[i].remain-=rlen;
                        fwrite(buf,sizeof(char),rlen,clients[i].fp);
                        if(clients[i].remain==0)
                        {
                            fclose(clients[i].fp);
                            clients[i].fp=0;
                            clients[i].status=0;
                            send(clients[i].fd,"+",1,MSG_NOSIGNAL);
                        }
                    }
                }
                else if(clients[i].status==3) // during get file
                {
                    if(FD_ISSET(clients[i].fd,&select_setw))
                    {
                        int rlen=fread(buf,sizeof(char),BUFLEN,clients[i].fp);
                        if(rlen==0)//EOF
                        {
                            clients[i].status=0;
                            fclose(clients[i].fp);
                            clients[i].fp=0;
                            send(clients[i].fd,"+",1,MSG_NOSIGNAL);
                        }
                        else
                        {
                            buf[rlen]=0;
                            send(clients[i].fd,buf,rlen,MSG_NOSIGNAL);
                        }
                    }
                }
            }
        }
    }
    //  accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))
    return 0;
}
