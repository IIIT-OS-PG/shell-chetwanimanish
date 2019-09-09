#include <iostream>
#include <vector>
#include <unistd.h>
#include <pwd.h>
#include <cstring>
#include <wait.h>
#include <sys/syscall.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unordered_map>
#include <fstream>

#define bin "/bin/"

using namespace std;

string home,user,ps1;
int last_proc;
unordered_map<string,string> aliases;

void tok_alias(basic_string<char> &basicString, string &basicString1, string &basicString2);
int make_tokens(vector<string>& input_tokens,string& input_command);

void clear_tokens(vector<string>& input_tokens)
{
    //cout<<"Vector size="<<input_tokens.size()<<endl;
    int i=0;
    while(i<input_tokens.size())
    {
        if(input_tokens[i].size()==0)
        {
            input_tokens.erase(input_tokens.begin()+i);
            i--;
        }
        i++;
    }
}

void clear_space(string& input_string)
{
    int i=0;
    while(i<input_string.size())
    {
        if((input_string[i]-' ')==0)
        {
            input_string.erase(input_string.begin()+i);
            i--;
        }
        i++;
    }
}

void set_env()  //deriving $HOME,$USER,$PS1
{
    uid_t uid=getuid();
    struct passwd *pwd=getpwuid(uid);
    char* ps;
    home=pwd->pw_dir;
    user=pwd->pw_name;
    setenv("HOME",home.c_str(),1);
    setenv("USER",user.c_str(),1);
    //cout<<"HOME="<<pwd->pw_dir<<endl;
    if(uid==0)
    {
        ps1.clear();
        ps1="#";
    }
    //setenv("PATH",bin, 1);
    //cout<<"PS1="<<ps1<<endl;
    //ofstream bashFile("."+pwd->pw_dir);
    //bashFile<<"PS1="<<endl;
}

void execute(vector<string>& input,int source_fd,int dest_fd,int source_2,int dest_2)
{
    if(input.size()>1 && strcmp(input[1].c_str(),"$$")==0)
    {
        input[1].clear();
        input.push_back(to_string(getpid()));
        //cout<<"input[1] size "<<input[input.size()-1].size()<<endl;
    }
    if(input.size()>1 && strcmp(input[1].c_str(),"$?")==0)
    {
        input[1].clear();
        input.push_back(to_string(last_proc));
        //cout<<"input[1] size "<<input[input.size()-1].size()<<endl;
    }
    if(aliases[input[0]].size()>0)
    {
        string temp=aliases[input[0]];
        input.erase(input.begin());
        vector<string> temps;
        make_tokens(temps,temp);
        for(int i=temps.size()-1;i>=0;i--)
        {
            input.insert(input.begin(),temps[i]);
        }
        /*cout<<"Alias exist"<<endl;
        cout<<"new command="<<input[0]<<endl;
        input[0].clear();
        cout<<"new command="<<aliases[input[0]]<<endl;
        for(int i=0;i<aliases[input[0]].size();i++)
        {
            input[0].push_back(aliases[input[0]][i]);
        }*/
        //cout<<"new command="<<input[0]<<endl;
    }
    if(strcmp(input[0].c_str(),"cd")==0)    //execute cd command
    {
        //cout<<"Cd Command"<<endl;
        if(strcmp(input[1].c_str(),"~")==0)
            input[1]=home;
        chdir(input[1].c_str());
    }
    else if(strcmp(input[0].c_str(),"sudo")==0)
    {
        //cout<<"Sudo Command"<<endl;
    }
    else
    {
        char* params[input.size()+1];
        for(int i=0;i<input.size();i++)
        {
            params[i]=(char *)malloc((input[i].size())* sizeof(char));
            strcpy(params[i], input[i].c_str());
        }
        params[input.size()]=NULL;
        /*for(int i=0;i<input.size();i++)
              cout<<params[i]<<endl;*/
        pid_t pid=fork();
        if(pid == -1)
        {
            cout<<"Cannot fork"<<endl;
            exit(0);
        }
        else if(pid==0)
        {
            //cout<<"Return of execvp="<<execvp(params[0],params)<<endl;
            dup2(source_fd,dest_fd);
            dup2(source_2,dest_2);
            if(execvp(params[0], params) < 0)
            {
                //cout<<"Size of params0= "<<sizeof(params[0])/ sizeof(char)<<endl;
                //cout<<"cannot execute command"<<endl;
            }
            //cout<<"exiting"<<endl;
            close(dest_fd);
            close(dest_2);
            exit(1);
        }
        else
        {
            waitpid(pid, &last_proc, 0);
        }
    }
}

int make_tokens(vector<string>& input_tokens,string& input_command) //return 0 - simple command
                                                                    //return 1 - pipes
                                                                    //return 2 - contains io redirection
{
    string token;
    int status=0;
    for(int i=0;i<input_command.size();i++)
    {
        if(input_command[i]=='|')
        {
            status=1;
            break;
        }
        if(input_command[i]=='>' || input_command[i]=='<')
        {
            status=2;
            break;
        }
    }
    if(status==0)
    {
        for(int i=0;i<input_command.size();i++)
        {
            if(input_command[i]==' ')  //generating token
            {
                input_tokens.push_back(token);
                token.clear();
                continue;
            }
            if(input_command[i]=='"')  //handling string literals
            {
                token.push_back('"');
                i++;
                while(input_command[i]!='"')
                {
                    token.push_back(input_command[i++]);
                }
                token.push_back('"');
                i++;
                continue;
            }
            token.push_back(input_command[i]);
        }
        input_tokens.push_back(token);
    }
    else if(status==1)
    {
        for(int i=0;i<input_command.size();i++)
        {
            if(input_command[i]=='|')  //generating token
            {
                input_tokens.push_back(token);
                token.clear();
                continue;
            }
            if(input_command[i]=='"')  //handling string literals
            {
                token.push_back('"');
                i++;
                while(input_command[i]!='"')
                {
                    token.push_back(input_command[i++]);
                }
                token.push_back('"');
                i++;
                continue;
            }
            token.push_back(input_command[i]);
        }
        input_tokens.push_back(token);
    }
    else if(status==2)
    {
        input_tokens.push_back(input_command);
    }
    return status;
    /*tokenizing input command*/
}

void take_ip(string& input_command)
{
    //input_tokens.push_back("manish");
    //input_tokens.push_back("chetwani");
    string temp,token;
    do
    {
        getline(cin,temp);
        input_command+=temp;
        if(input_command[input_command.size()-1]=='\\')
            input_command.erase(input_command.size()-1);
    }while(temp[temp.size()-1]=='\\');     //handling for multiple lines command
    //cout<<input_command<<endl;  //input command created
    /*tokenizing input command*/
}

void execute_normal_io(vector<string>& input_tokens)
{
    char command[input_tokens[0].size()];
    strcpy(command,input_tokens[0].c_str());
    vector<string> tokenize;
    string source,dest;
    if(input_tokens[0].find(">>") < input_tokens[0].size())     //append wala redirection
    {
        source=strtok(command,">>");
        dest=strtok(NULL,">>");
        clear_space(dest);
        //cout<<"File Name:"<<dest<<endl;
        int file_des=open(dest.c_str(),O_CREAT | O_WRONLY | O_APPEND,0777);
        make_tokens(tokenize,source);
        clear_tokens(tokenize);
        execute(tokenize,file_des,STDOUT_FILENO,STDIN_FILENO,STDIN_FILENO);
    }
    else if(input_tokens[0].find("<") < input_tokens[0].size())     //input redirection
    {
        dest=strtok(command,"<");
        source=strtok(command,"<");
        clear_space(dest);
        //cout<<"File Name:"<<dest<<endl;
        int file_des=open(dest.c_str(),O_CREAT | O_RDONLY,0777);
        make_tokens(tokenize,dest);
        clear_tokens(tokenize);
        execute(tokenize,file_des,STDIN_FILENO,STDOUT_FILENO,STDOUT_FILENO);
    }
    else        //output redirection
    {
        source=strtok(command,">");
        dest=strtok(NULL,">");
        clear_space(dest);
        //cout<<"File Name:"<<dest<<endl;
        int file_des=open(dest.c_str(),O_CREAT | O_WRONLY,0777);
        make_tokens(tokenize,source);
        clear_tokens(tokenize);
        execute(tokenize,file_des,STDOUT_FILENO,STDIN_FILENO,STDIN_FILENO);
    }
}

void piped_commands(vector<string>& input_tokens)
{
    clear_tokens(input_tokens);
    vector<string> ind_command;
    int pip[input_tokens.size()][2],status=0;
    for(int i=0;i<input_tokens.size();i++)
    {
        pipe(pip[i]);
    }
    for(int i=0;i<input_tokens.size();i++)
    {
        //cout<<input_tokens[i]<<endl;
        ind_command.clear();
        status=make_tokens(ind_command,input_tokens[i]);
        clear_tokens(ind_command);
        if(ind_command.size()>1 && strcmp(ind_command[1].c_str(),"$$")==0)
        {
            ind_command[1].clear();
            ind_command.push_back(to_string(getpid()));
            //cout<<"input[1] size "<<input[input.size()-1].size()<<endl;
        }
        if(ind_command.size()>1 && strcmp(ind_command[1].c_str(),"$?")==0)
        {
            ind_command[1].clear();
            ind_command.push_back(to_string(last_proc));
            //cout<<"input[1] size "<<input[input.size()-1].size()<<endl;
        }
        if(aliases[ind_command[0]].size()>0)
        {
            string temp=aliases[ind_command[0]];
            ind_command.erase(ind_command.begin());
            vector<string> temps;
            make_tokens(temps,temp);
            for(int i=temps.size()-1;i>=0;i--)
            {
                ind_command.insert(ind_command.begin(),temps[i]);
            }
        }
        if(status==2)
        {
            //cout<<"Pipe with IO redirection"<<endl;
            /*for(int k=0;k<ind_command.size();k++)
                cout<<ind_command[k]<<endl;*/
            //execute_normal_io(ind_command);

        }
        else
        {
            char* params[ind_command.size()+1];
            for(int j=0;j<ind_command.size();j++)
            {
                //cout<<ind_command[j]<<endl;
                params[j]=(char *)malloc((ind_command[j].size())* sizeof(char));
                strcpy(params[j], ind_command[j].c_str());
            }
            params[ind_command.size()]=NULL;
            /*for(int j=0;j<ind_command.size();j++)
                cout<<params[j]<<endl;*/
            //cerr<<"Error Here"<<endl;
            pid_t pid=fork();
            if(pid==-1)
            {
                cout<<"Cannot Fork Child in Pipe Execution"<<endl;
            }
            else if(pid==0)
            {
                if(i==0)
                {
                    close(STDOUT_FILENO);
                    close(pip[i][0]);
                    dup2(pip[i][1],STDOUT_FILENO);
                }
                else if(i==(input_tokens.size()-1))
                {
                    close(STDIN_FILENO);
                    close(pip[i-1][1]);
                    dup2(pip[i-1][0],STDIN_FILENO);
                }
                else
                {
                    close(STDIN_FILENO);
                    close(STDOUT_FILENO);
                    dup2(pip[i-1][0], STDIN_FILENO);
                    dup2(pip[i][1],STDOUT_FILENO);
                    close(pip[i][0]);
                }
                if(execvp(params[0], params)<0)
                    cout<<"cannot execute command"<<endl;
            }
            else
            {
                waitpid(pid, &last_proc, 0);
            }
        }
        close(pip[i][1]);
    }
}

void tok_alias(string& command, string& alias_name, string& alias_command) {
    alias_name.clear();
    alias_command.clear();
    int i=0;
    while(command[i]!='=')
    {
        alias_name.push_back(command[i++]);
    }
    //cout<<"Command is:"<<alias_name<<endl;
    i+=2;
    //cout<<"skip "<<i<<endl;
    for(int j=i;j<command.size()-1;j++)
    {
        alias_command.push_back(command[j]);
    }
}

int main() {
    //std::cout << "Hello, World!" << std::endl;
    vector<string> input_tokens;
    string input_given;
    int status=0;
    set_env();
    chdir(home.c_str());
    string bashName=home+"/.myrc";
    //cout<<bashName<<endl;
    ifstream bashRC(bashName);
    string ss;
    while(getline(bashRC,ss))
    {
        //cout<<ss<<endl;
        int i=0;
        string var_name,var_value;
        while(ss[i]!='=')
        {
            var_name.push_back(ss[i++]);
        }
        //cout<<"Var Name "<<var_name<<endl;
        i++;
        for(;i<ss.size();i++)
        {
            var_value.push_back(ss[i]);
        }
        //cout<<"Var Value "<<var_value<<endl;
        if(strcmp(var_name.c_str(),"PS1")==0)
            ps1=var_value;
        setenv(var_name.c_str(),var_value.c_str(), 1);
    }
    //cout<<"Home="<<home<<"\nps1="<<ps1<<"\nuser="<<user<<endl;
    do
    {
        status=0;
        input_tokens.clear();
        input_given.clear();
        set_env();
        cout<<user<<":";
        cout<<ps1<<"::";
        take_ip(input_given);
        status=make_tokens(input_tokens,input_given);
        clear_tokens(input_tokens);
        /*Alias Processing*/
        if(strcmp(input_tokens[0].c_str(),"alias")==0)
        {
            //cout<<"Alias"<<endl;
            if(input_tokens[1].find("=") < input_tokens[1].size())
            {
                //cout<<"Contains both params"<<endl;
                string alias_name,alias_command;
                tok_alias(input_tokens[1],alias_name,alias_command);
                aliases.insert(make_pair(alias_name,alias_command));
                //aliases[alias_name]=alias_command;
                //cout<<"alias added for "<<alias_name<<" as "<<aliases[alias_name]<<endl;
                //cout<<"alais for dash as "<<aliases["dash"].size()<<endl;
            }
            else
            {
                cerr<<"Enter Proper Command"<<endl;
            }
        }
        /*Alias Processing*/
        //for(int i=0;i<input_tokens.size();i++)
         //   cout<<input_tokens[i]<<" size="<<input_tokens[i].size()<<endl;
        if(status==0)   //Normal Execution
            execute(input_tokens,STDIN_FILENO,STDIN_FILENO,STDOUT_FILENO,STDOUT_FILENO);
        else if(status==1)  //Piped Execution
            piped_commands(input_tokens);
            //cout<<"piped execution"<<endl;
        else if(status==2)  //Normal IO Redirection
            execute_normal_io(input_tokens);
        //  cout<<"Last proc="<<last_proc<<endl;
    }while(strcmp(input_tokens[0].c_str(),"exit")!=0);
    return 0;
}
