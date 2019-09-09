#include <iostream>
#include <vector>
#include <unistd.h>

#define buff_size sizeof(char)

using namespace std;

void take_ip(vector<string>& input_tokens)
{
    //input_tokens.push_back("manish");
    //input_tokens.push_back("chetwani");
    string input_command,temp,token;
    do
    {
        getline(cin,temp);
        input_command+=temp;
        if(input_command[input_command.size()-1]=='\\')
            input_command.erase(input_command.size()-1);
    }while(temp[temp.size()-1]=='\\');     //handling for multiple lines command
    //cout<<input_command<<endl;  //input command created
    /*tokenizing input command*/
    for(int i=0;i<input_command.size();i++)
    {
        if(input_command[i]==' ')  //generating token
        {
            //cout<<"space at "<<i<<endl;
            //cout<<"token "<<token<<endl;
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
    //cout<<"token="<<token<<endl;
    input_tokens.push_back(token);
    /*tokenizing input command*/
}

int main()
{
    /*vector<string> input_tokens;
    take_ip(input_tokens);
    for(int i=0;i<input_tokens.size();i++)
        cout<<input_tokens[i]<<endl;*/
    char c[1];
    while(read(STDIN_FILENO,c,buff_size))
    {
        cout<<c<<endl;
    }
    string s1;
    s1.
    return 0;
}