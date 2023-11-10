#include <iostream>
#include "Tokenizer.h"
using namespace std;

Tokenizer::Tokenizer (const string _input) {
    error = false;
    input = trim(_input);
    // only want to split if we dont have sign expansion
    // if(hasSignExpansion(input)){
    //     markExpansionPipes(input);
    //     std::cout<<input<<std::endl;
    //     // replace all pipes in the sign expansion with special char ® to prevent spliting
    //     split("|");
    //     // replace the marked arguements in the command object with the pipes
    //     undoSignExpansionMark();
    split("|");

    
}

Tokenizer::~Tokenizer () {
    for (auto cmd : commands) {
        delete cmd;
    }
    commands.clear();
}

bool Tokenizer::hasError () {
    return error;
}

void Tokenizer::markExpansionPipes(std::string &input){
    for(long unsigned int i = 0; i < input.length()-1;i++){
        if(input[i] == '$' && input[i+1] == '('){
            // find the closing bracket
            std::vector<char> stack = {'('};
            long unsigned int j = i+2;
            while(stack.size() && j < input.length()){
                // search for a closing character
                if(input[j] == '|'){
                    input[j] = expansionPipe;
                }
                if(input[j] == '('){
                    stack.push_back('(');
                }else if(input[j] == ')'){
                    stack.pop_back();
                }
                j++;
            }
            
        }
    }
}

void Tokenizer::undoSignExpansionMark(){
    for(long unsigned int i = 0; i < commands.size(); i++){
        // replace marked characters in args with a pipe
        for(long unsigned int j = 0; j < commands[i]->args.size(); j++){
            for(long unsigned int k = 0; k < commands[i]->args[j].length(); k++){
                if(commands[i]->args[j][k] == expansionPipe){
                    commands[i]->args[j][k] = '|';
                }

            }

        }
    }
}

bool Tokenizer::hasSignExpansion (const std::string input){
    // do we have a sign expansion in the input string?
    for(long unsigned int i = 0; i < input.length()-1;i++){
        if(input[i] == '$' && input[i+1] == '('){
            // find the closing bracket
            std::vector<char> stack = {'('};
            long unsigned int j = i+2;
            while(stack.size() && j < input.length()){
                // search for a closing character
                if(input[j] == '('){
                    stack.push_back('(');
                }else if(input[j] == ')'){
                    stack.pop_back();
                }
                j++;
            }
            if(!stack.size()){
                return true;
            }
            error = true;
            return false;
        }
    }
    return false;
}

string Tokenizer::trim (const string in) {
    int i = in.find_first_not_of(" \n\r\t");
    int j = in.find_last_not_of(" \n\r\t");

    if (i >= 0 && j >= i) {
        return in.substr(i, j-i+1);
    }
    return in;
}

void Tokenizer::split (const string delim) {
    string temp = input;
    
    vector<string> inner_strings;
    int index = 0;
    while (temp.find("\"") != string::npos || temp.find("\'") != string::npos) {
        int start = 0;
        int end = 0;
        if (temp.find("\"") != string::npos
            && (temp.find("\'") == string::npos || temp.find("\"") < temp.find("\'"))) {
            start = temp.find("\"");
            end = temp.find("\"", start+1);
            if ((size_t) end == string::npos) {
                error = true;
                cerr << "Invalid command - Non-matching quotation mark on \"" << endl;
                return;
            }
        }
        else if (temp.find("\'") != string::npos) {
            start = temp.find("\'");
            end = temp.find("\'", start+1);
            if ((size_t) end == string::npos) {
                error = true;
                cerr << "Invalid command - Non-matching quotation mark on \'" << endl;
                return;
            }
        }
        
        inner_strings.push_back(temp.substr(start+1, end-start-1));
        
        string str_beg = temp.substr(0, start);
        string str_mid = "--str " + to_string(index);
        string str_end = temp.substr(end+1);
        temp = str_beg + str_mid + str_end;
        
        index++;
    }

	size_t i = 0;
	while ((i = temp.find(delim)) != string::npos) {
		commands.push_back(new Command(trim(temp.substr(0, i)), inner_strings));
		temp = trim(temp.substr(i+1));
	}
	commands.push_back(new Command(trim(temp), inner_strings));
}
