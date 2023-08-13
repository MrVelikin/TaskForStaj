#include <sys/types.h>
#include <chrono>
#include <string>
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

using namespace std;

struct server
{
    virtual ~server() { }

    virtual void config_server(int port,const char * address) = 0;
    virtual void sbind() = 0;
    virtual void slisten() = 0;
    virtual void saccept() = 0;
    virtual void sread() = 0;
};

template < typename T > 
struct rw_server : public virtual server
{
    ~rw_server() override{}
    
    virtual void swrite(const T& value) = 0;
};

class r_server : public virtual server
{
private:
    int sock, listener;
    struct sockaddr_in addr;
    char buf[128];
    int bytes_read;       

public:
    void config_server(int port,const char * address) override 
    {
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr=inet_addr(address);
    }

    void sbind() override 
    {
        if(bind(listener, (struct sockaddr *)&addr, sizeof(addr)) < 0) 
    {
        perror("bind");
        exit(2);
    }
    }
    void slisten() override 
    { 
        listener = socket(AF_INET, SOCK_STREAM, 0); 
        if(listener < 0)
        {
            perror("socket");
            exit(1);
        }    
    }
    void saccept() override 
    { 
        listen(listener, 1); 
        while(1)
        {
            sock = accept(listener, NULL, NULL); 
            if(sock < 0)
            {
                perror("accept");
                exit(3);
            }
            sread();
        }
        close(sock);
    }
    void sread() override 
    {   
        while(1)
        {
            memset(buf, '\0', sizeof(buf));
            bytes_read = recv(sock, buf, 2048, 0);
            if(bytes_read <= 0) break;
            if((bytes_read==2) and (atoi(buf)%32==0))
                cout<<"Получено cообщение: "<<buf<<endl;
            else
                cout<<"ОШИБКА"<<endl;
        }
    }

     void sclose() 
    {   
        close(sock);
    }

};

void correct_exit(r_server ourprog)
{
    while(1)
    {
        string str;
        getline(cin, str);
        if (str=="xxx")
        {
            ourprog.sclose();
            exit(0);
        }
    }
}

void work(r_server ourprog)
{
    ourprog.slisten();
    ourprog.config_server(1516,"127.0.0.1");
    ourprog.sbind();
    ourprog.saccept();
}

int main()
{
    r_server ourprog;
    cout<<"Tape xxx to exit"<<endl;
    thread th1(correct_exit, cref(ourprog));
    thread th2(work, cref(ourprog));
    th1.join();
    th2.join();
    return 0;
}
