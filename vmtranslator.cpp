#include <iostream>
#include <vector>
#include <fstream>
#include <cxxabi.h>
#include <string.h>
#include <map>
#include <list>
using namespace std;

vector<string> split(string s, const char* delim) {
   char ss[s.length()+1];  //保留夠用byte數加1，準備補0用
   s.copy(ss, s.length(), 0);  //拷貝成char *,因為引數必需是char*
   ss[s.length()] = 0;     //補0，因copy沒加上0
   vector<string> tokens;
   char *p = strtok(ss, delim);
   while(p) {
      tokens.push_back(p);  //char* 可賦值給string
      p = strtok(NULL, delim); //再次呼叫，內部作用機制，語法有一點仔怪。
   }
   return tokens;
}
                        //所有命令總集合，無佇內底的愛噴出錯誤。
vector<string> command = {"return", "end", "call", "func", "push", "pop", "goto",
                "if-goto", "add", "sub", "mul", "div", "neg", "eq", "gt", "lt",
                "and", "or", "not", "label" };


bool isDigit(string token) {
   for (int j=0; j<token.length(); j++) {
      if ( !isdigit(token.at(j)) ) {
         return false;
      }
   }
   if (token.length() == 0) return false;
   return true;
}

void line_error(int line) {
    cout << "line " << line << " has error!" << endl;
}

template<typename T>
class Common {
    public:
    T command;
    Common(T command): command(command) { }
};

class Command {
    public:
    int line_number{}; string comm_name{}, code{};
    string label_name{};

    static map<int, Command*> allCommand;
    static bool isLabelExists(string lname) {
        for (auto it = allCommand.begin(); it != allCommand.end(); it++) {
            if (it->second->label_name == lname) return true;
        }
        return false;
    }
    static Command* searchLabel(string label) {
        for (auto it = allCommand.begin(); it != allCommand.end(); it++) {
            if (it->second->label_name == label) return it->second;
        }
        return nullptr;
    }
    static Command* findLastFunc() {
        for (auto it = allCommand.rbegin(); it != allCommand.rend(); it++) {
            Command* com = it->second;
            if (com->comm_name == "func") {
                return com;
            }
        }
        return nullptr;
    }
    Command(int line_number, string comm_name, string code) {
        this->line_number = line_number;
        this->comm_name = comm_name;
        this->code = code;
        allCommand.insert({line_number, this});
    }

    Command(int line_number, string comm_name, string code, string label):
                                        Command(line_number, comm_name, code) {
        this->label_name = label;
    }

    string info() {
        return to_string(line_number) + " " + comm_name + " " + code;
    }
    void writeCode() {
        cout << code << endl;
    }

};
map<int, Command*> Command::allCommand = { };

class Label: public Command {
    public:
    Label(int line, string comm_name, string code, string lname):
                                      Command(line, comm_name, code, lname) {
    }
};

class Goto: public Command {
    public:
    Goto(int line, string comm_name, string code, string lname):
                                     Command(line, comm_name, code, lname) {
        if (lname == "two-pass") {
            // do nothing , code is altered by second pass's argument
        }
        else this->code = "@" + lname + "\n0;JMP\n";  //first pass 修改 code
    }
};

class IfGoto: public Command {
    public:
    IfGoto(int line, string comm_name, string code, string lname):
                                     Command(line, comm_name, code, lname) {
        if (lname == "two-pass") { } // do nothing , code is as argument tokens[1]
        else this->code = "@SP\nA=M-1\nD=M\n@SP\nM=M-1\n@" + lname + "\nD;JMP\n";  //修改 code
    }
};

class Func: public Command {
    public:
    string ret_code = "";
    Func(int line, string comm_name, string code, string fname):
                                            Command(line, comm_name, code, fname) {
    }
};


class Call: public Command {
    public:
    static string KeepEnviron;
    Call(int line, string comm_name, string code, string fname):
                                     Command(line, comm_name, code, fname) {
        if (fname == "two-pass") { }
        else {              //reserve return address's code is in argement
            this->code = code + KeepEnviron + "@" + fname + "\n0;JMP\n"
                              + "(CALL_" + to_string(line) + ")\n";
        }
    }
};
string Call::KeepEnviron =     //only reserve sp to that
             string("@SP\nA=M\nM=A\n@SP\nM=M+1\n@LCL\nD=M\n@SP\nA=M\n")
           + string("M=D\n@SP\nM=M+1\n@ARG\nD=M\n@SP\nA=M\nM=D\n@SP\n")
           + string("M=M+1\n@THIS\nD=M\n@SP\nA=M\nM=D\n@SP\nM=M+1\n")
           + string("@THAT\nD=M\n@SP\nA=M\nM=D\n@SP\nM=M+1\n");

class Return: public Command {
    public:
    int parent_line = 0;
    static string RestoreEnviron;
    Return(int line, string comm_name, string code, string label):
                                       Command(line, comm_name, code, label) {
        this->code = RestoreEnviron + "(RETURN_" + to_string(line) + ")\n"; //label for return
    }
};
string Return::RestoreEnviron =
                string("@SP\nM=M-1\nA=M\nD=M\n@THAT\nM=D\n@SP\nM=M-1\n")
                + string("A=M\nD=M\n@THIS\nM=D\n@SP\nM=M-1\nA=M\nD=M\n")
                + string("@ARG\nM=D\n@SP\nM=M-1\nA=M\nD=M\n@LCL\nM=D\n")
                + string("@SP\nM=M-1\nA=M\nD=M\n@SP\nM=D\n")
                + string("@SP\nM=M-1\n@SP\nA=M\nD=M\nA=D\n0;JMP\n"); //after call's address

class NoArg: public Command {
    public:
    static map<string, string> Comm;
    NoArg(int line, string comm_name, string code): Command(line, comm_name, code) {
        size_t pos;  //無參數命令 若有label 改作行號
        while ((pos = this->code.find("END")) != string::npos) {
            this->code = this->code.replace(pos, 3, "LABEL_"+ to_string(line));
        }
    }
};
map<string, string>NoArg::Comm = {
    {"add", "@SP\nD=M\nA=D-1\nD=M\nA=A-1\nM=M+D\n@SP\nM=M-1" },
    {"sub", "@SP\nD=M\nA=D-1\nD=M\nA=A-1\nM=M-D\n@SP\nM=M-1" },
    {"neg", "@SP\nD=M\nA=D-1\nD=M\nD=!D\nM=D+1"},
    {"eq" , "@SP\nA=M-1\nD=M\nA=A-1\nD=M-D\n@SP\nM=M-1\nA=M\nA=A-1\nM=-1\n@END\nD;JEQ\n@SP\nA=M-1\nM=0\n(END)" },
    {"gt" , "@SP\nA=M-1\nD=M\nA=A-1\nD=M-D\n@SP\nM=M-1\nA=M\nA=A-1\nM=-1\n@END\nD;JGT\n@SP\nA=M-1\nM=0\n(END)" },
    {"lt" , "@SP\nA=M-1\nD=M\nA=A-1\nD=M-D\n@SP\nM=M-1\nA=M\nA=A-1\nM=-1\n@END\nD;JLT\n@SP\nA=M-1\nM=0\n(END)" },
    {"end", "end of program." },
};

class Push: public Command {
    public:
    static map<string, string> SubCommand;
    Push(int line, string comm_name, string code, string sub_comm):
                                     Command(line, comm_name, code, sub_comm) {
        //處理傳入來的數值，修改欲輸出的 code
        string out_code = "@" + code + "\n" + SubCommand[sub_comm];
        this->code = out_code;
    }
};
map<string, string> Push::SubCommand = {
        {"constant", "D=A\n@SP\nA=M\nM=D\n@SP\nM=M+1\n"},
        {"local",    "D=A\n@LCL\nA=M+D\nD=M\n@SP\nA=M\nM=D\n@SP\nM=M+1\n"},
        {"argument", "D=A\n@ARG\nA=M+D\nD=M\n@SP\nA=M\nM=D\n@SP\nM=M+1\n"},
        {"static",   "D=A\n@16\nA=A+D\nD=M\n@SP\nA=M\nM=D\n@SP\nM=M+1"},
        {"this",     "D=A\n@this\nA=M+D\nD=M\n@SP\nA=M\nM=D\n@SP\nM=M+1"},
        {"that",     "D=A\n@THAT\nA=M+D\nD=M\n@SP\nA=M\nM=D\n@SP\nM=M+1"},
        {"temp",     "D=A\n@R5\nA=A+D\nD=M\n@SP\nA=M\nM=D\n@SP\nM=M+1"},
};

class Pop: public Command {
    public:
    static map<string, string> SubCommand;
    Pop(int line, string comm_name, string code, string sub_comm):
                                           Command(line, comm_name, code, sub_comm) {
    }
};
map<string, string> Pop::SubCommand = {
    {"local", "D=A\n@LCL\nD=M+D\n@SP\nA=M\nM=D\n@SP\nM=M-1\nA=M\nD=M\n@SP\nA=M+1\nA=M\nM=D\n"},
    {"static", "@5\nD=A\n@256\nD=A+D\n@SP\nA=M\nM=D\n@SP\nM=M-1\nA=M\nD=M\n@SP\nA=M+1\nA=M\nM=D\n"},
};

void parse(int line, vector<string> tokens) {
    int args = tokens.size() - 1;
    Command *command = nullptr;
    if (tokens[0] == "label" && args == 1) {
        string code = "(" + tokens[1] + ")"; //輸出碼叫做 code
        if (Command::isLabelExists(tokens[1])) {
            cout << "line " << line << " label exists." << endl;
        }
        else {
            command = new Label(line, tokens[0], code, tokens[1]);
        }
    }
    else if (tokens[0] == "func" && args == 1) {
        if (Command::isLabelExists(tokens[1])) {
            cout << "line " << line << " func has label defined." << endl;
        }
        else {
            string code = "(" + tokens[1] + ")";
            command = new Func(line, tokens[0], code, tokens[1]);
        }
    }
    else if (tokens[0] == "return" && args == 0) {
        //Command* func = Command::findLastFunc();
        Command* func = Command::findLastFunc();
        if (func == nullptr) cout << "line " << line << " has no func defined" << endl;
        else {
            command = new Return(line, tokens[0], "", func->label_name); //完全交予 new Return
            func->code = "@RETURN_" + to_string(line) + "\n0;JMP\n" + func->code;
        }
    }
    else if (tokens[0] == "call" && args == 1) {
        Command* search_comm = Command::searchLabel(tokens[1]); //search label_name

        if (search_comm == nullptr) {  //label 無存在新定義label留予 second pass 處理
            command = new Call(line, tokens[0], tokens[1], "two-pass");
        }
        else if (search_comm->comm_name == "func") { //本逝處理第一pass
            string ret_addr = "@CALL_" + to_string(line)
                                       + "\nD=A\n@SP\nA=M\nM=D\n@SP\nM=M+1\n";
            command = new Call(line, tokens[0], ret_addr, tokens[1]);
        }
        else cout << "no such func exists." << endl;
    }
    else if (NoArg::Comm.find(tokens[0]) != NoArg::Comm.end() && args == 0) {
        command = new NoArg(line, tokens[0], NoArg::Comm[tokens[0]]);
    }
    else if (tokens[0] == "push" && args == 2) {  //subcommand exists
        if (Push::SubCommand.find(tokens[1]) != Push::SubCommand.end()) {
            int n = stoi(tokens[2]);
            assert(n >= 0);
            command = new Push(line, tokens[0], tokens[2], tokens[1]);
        }
        else { cout << "line " << line << " push has error!"; }  //subcommand not exists
    }
    else if (tokens[0] == "pop" && args == 2) {
        if (Pop::SubCommand.find(tokens[1]) != Pop::SubCommand.end()) {
            command = new Pop(line, tokens[0], Pop::SubCommand[tokens[1]], tokens[1]);
        }
        else { cout << "line " << line << " has error!"; }
    }
    else if (tokens[0] == "goto" && args == 1) {
        if (Command::isLabelExists(tokens[1])) {
            command = new Goto(line, tokens[0], tokens[1], tokens[1]);
        }
        else {
            // "two pass" laebl 暫存佇 code
            command = new Goto(line, tokens[0], tokens[1], "two-pass");
        }
    }
    else if (tokens[0] == "if-goto" && args == 1) {
        if (Command::isLabelExists(tokens[1])) {
            command = new Goto(line, tokens[0], tokens[1], tokens[1]);
        }
        else {
            // "two pass" laebl 暫存佇 code
            command = new Goto(line, tokens[0], tokens[1], "two-pass");
        }
    }
    else if (tokens[0] == "end" && args == 0) {
        cout << "Program end at this line" << endl;
    }
    else {
        cout << "line " << line << " has errors." << endl;
    }
}

int main(int argc, char* argv[]) {
    map<int, vector<string>> lines;  //記錄所有行，含空白及註解
    ifstream fin(argv[1]);
    if (!fin) {
        cout << "Can't open file!" << endl;
        return 0;
    }
    if (fin.is_open()) {
        string line=""; int line_count = 0;
        while (getline(fin, line)) {
            line_count++;
            if (line.length()==0) { continue; }  //完全空白列跳過
            if (line.substr(0, 2)=="//") continue;
            //將每一逝拆做tokens
            vector<string> tokens;
            tokens = split(line, " ");
            if (tokens.size()==0) continue;  //含空白space字元也跳過
                //每一逝攏是vector<string>
                lines.insert({line_count, tokens});
        }
        fin.close();
    }

    map<int, vector<string>>::iterator iter;
    for (iter = lines.begin(); iter != lines.end(); ++iter) {
        vector<string> tokens = iter->second;  //all tokens of single line
        if (std::find(command.begin(), command.end(), tokens[0]) != command.end()) {
            parse((*iter).first, tokens);
        }
        else {
            cout << "line " << (*iter).first << " :command error!" << endl;
            return 0;
        }
        cout << endl;
    }
//--------------------------------------------------------------------------
cout << "============================================" << endl;
for (map<int, Command*>::iterator it = Command::allCommand.begin(); it!=Command::allCommand.end(); it++) {
        Command* com = it->second;
        cout << com->line_number << " : comm: " << com->comm_name
             << " label: " << com->label_name << endl;
}
//--------------------------------------------------------------------------

//second pass
    for (map<int, Command*>::iterator it = Command::allCommand.begin(); it!=Command::allCommand.end(); it++) {
        Command* com = it->second;
        if (com->label_name == "two-pass") {
            if (com->comm_name == "goto") {
                com->label_name = com->code;
                com->code = "@" + com->code + "\n0;JMP\n";
            }
            if (com->comm_name == "if-goto") {
                com->label_name = com->code;
                com->code = "@SP\nA=M-1\nD=M\n@SP\nM=M-1\n@" + com->code + "\nD;JMP\n";
            }
            if (com->comm_name == "call") {  //code is label_name
                Command* touch = Command::searchLabel(com->code);
                //nullptr 是揣無 label， func 是有label 毋是 func label
                if (touch == nullptr || touch->comm_name != "func") {
                    cout << "line " << com->line_number;
                    cout << " no such func name be called" << endl;
                }
                else {
                    com->label_name = com->code;
                    com->code = "@CALL_" + to_string(com->line_number) + "\n"
                                + "D=A\n@SP\nA=M\nM=D\n@SP\nM=M+1\n" // push return address
                                + Call::KeepEnviron
                                + "@" + com->code + "\n0;JMP\n"
                                + "(CALL_" + to_string(com->line_number) + ")\n";
                }
            }
        }
    }
//===================================================================================
cout << "============================================" << endl;
for (map<int, Command*>::iterator it = Command::allCommand.begin(); it!=Command::allCommand.end(); it++) {
    Command* com = it->second;
    //cout << com->line_number << " : comm_name: " << com->comm_name
    //     << " code: " << com->code << " label: " << com->label_name << endl;
    cout << com->code << endl;
int status;
//cout << "real command: " << abi::__cxa_demangle(typeid(com).name(), 0, 0, &status) << endl;
}
//==================================================================================
    return 0;
}
