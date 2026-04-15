#include <bits/stdc++.h>
int main(){
    std::string s;
    std::cin >> s;
    if(!strcasecmp(s.c_str(),"PASV")){
        std::cout << "1";
    }else{
        std::cout << "2";
    }
    return 0;
}