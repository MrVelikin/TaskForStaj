#include <iostream>
#include <chrono>
#include <thread>
#include <string>
#include <algorithm>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h> 
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <mutex>

using namespace std;

mutex lock_;

struct client
{
    virtual ~client() { }

    virtual void config_client(int port,const char * address) = 0;
    virtual void csock() = 0;
    virtual bool cconent() = 0;
    virtual void csend() = 0;
};

struct rw_client : public virtual client
{
    ~rw_client() override{}
    
    virtual void crecv() = 0;
};

class our_client : public virtual client
{
private:
    struct sockaddr_in cli;
    int sock;
    vector <string> message;

public:
    void config_client(int port,const char * address) override 
    {
        cli.sin_addr.s_addr = inet_addr(address);
        cli.sin_port = htons(port);
        cli.sin_family = AF_INET;
    }

    void csock() override 
    {
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == -1)
        {
            perror("socket\n");
            exit(1);
        }
    }

    bool cconent() override 
    {
        close(sock);
        csock();
        if (connect(sock, (struct sockaddr *)&cli, sizeof(cli)) == -1)
            return false;
        else
            return true;
    }

    void csend() override 
    {
        send(sock, (message.back()).c_str(), (message.back()).size(), 0);
        message.pop_back();
        close(sock);
    }

    string get_message()
    {
         if(message.empty()!=true)
            return message.front();
        else 
            return "";
    }

    void set_str(string str_to_set)
    {
        message.insert(message.begin(),str_to_set);
    }

};

bool is_64number (string number)
{
    return number.size()==64 && all_of(number.begin(), number.end(), ::isdigit);
}

//замена в строчке четных на КВ
void to_KB(string &str)
{
    int i=0;
    while(i<=str.size())
    {
        if ((str[i] % 2 == 0) and isdigit(str[i]))
        {
            str.replace(i,1,"KB");
        }
        i++;
    }
}

//поток 1
void writeStr(int fd_fifo)
{
    cout<<"Enter 64 number string or xxx to exit"<<endl;
    while(1)
    {
        string str;
        getline(cin, str);
        if (str=="xxx")
        {
            unlink("fifo1.1");
            exit(0);
        }
        if(is_64number(str)==false)
        {
            cout<<"Wrong input, write 64 number string"<<endl;
            continue;
        }
        sort(str.begin(),str.end(),greater<int>());
        to_KB(str);
        write(fd_fifo,str.c_str(),str.size());
    }
}

//поток для отправки сообщений

void sendler(our_client &ourprog)
{
    while(1)
    {
        if( ourprog.get_message()!="")
        {
            if(ourprog.cconent()==true)
            {
                ourprog.csend();
                sleep(0.1);
            }
            else
            {
                sleep(0.5);
            }
        }
    }
}

//поток 2
void readStr(int fd_fifo,our_client &ourprog)
{
    int sum=0;
    while(1)
    {
        char buf[128]={}; 
        read(fd_fifo, &buf, sizeof(buf));
        cout<<"thread 2 get - "<<buf<<endl;
        for (int i=0;buf[i] != '\0';i++)
            if (isdigit(buf[i]))
                sum += buf[i] - '0';      
        ourprog.set_str(to_string(sum));
        sum=0;
    }
}

int main(int argc, char *argv[])
{
    our_client ourprog;
    ourprog.config_client(1516,"127.0.0.1");
    int fd_fifo,fd_fifo2;
    unlink("fifo1.1");
    if((mkfifo("fifo1.1", O_RDWR)) == -1)
    {
        fprintf(stderr, "Невозможно создать fifo\n");
        exit(0);
    } 
    if((fd_fifo=open("fifo1.1", O_RDWR)) == -1)
    {
        fprintf(stderr, "Невозможно открыть fifo, используйте 'sudo ./prog1' \n");
        exit(0);
    }  
	thread th1(writeStr,fd_fifo);
    thread th2(readStr,fd_fifo,ref(ourprog));
    thread th3(sendler,ref(ourprog));
    th1.join();
    th2.join();
    th3.join();
    return 0;
}