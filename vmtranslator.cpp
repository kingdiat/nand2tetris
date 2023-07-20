#include <iostream>
#include <vector>
#include <fstream>
#include <string>
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
    int line_number = 0; string comm_name = "";
    string code = "";
    static map<int, Command> allCommand;
    Command(int line_number, string comm_name, string code) {
        this->line_number = line_number;
        this->comm_name = comm_name;
        this->code = code;   //因為0是技巧安排用，非真正解析出來的
        if (line_number != 0) allCommand.insert({line_number, *this});
    }
    string info() {
        return to_string(line_number) + " " + comm_name + " " + code;
    }
    void writeCode() {
        cout << code << endl;
    }
};
map<int, Command> Command::allCommand = { };

class Label: public Command {
    public:
    string label_name = "";
    static map<int, Label> labels;
    static int count() {
        return labels.size();
    }
    static bool isExists(string lname) {
        for (auto it = labels.begin(); it != labels.end(); it++) {
            if (it->second.label_name == lname) return true;
        }
        return false;
    }

    Label(int line, string comm_name, string lname, string code): Command(line, comm_name, code) {
        if (isExists(lname)) {  //因為Command constructor執行就加入來矣，
            cout << "line " << line << " label duplicated." << endl;
            Command::allCommand.erase(this->line_number); //判斷的時袂赴了，刣掉
        }
        else { this->label_name = lname; labels.insert({line, *this}); }
    }
};
map<int, Label> Label::labels = {};

class Func: public Command {
    public:
    Label *p_label;
    string func_name = "";
    static map<int, Func> funcs;

    Func(int line, string comm_name, string lname, string code): Command(line, comm_name, code) {
    //Func(int line, string comm_name, string lname, string code) {
        if (Label::isExists(lname)) {
            cout << lname << " 已定義。" << endl;
        }
        else {         //下跤這逝若無 new 關鍵字，後續執行竟segment fault。
            p_label = new Label(line, comm_name, lname, code);
            func_name = comm_name + "_" + lname;
            funcs.insert({line, *this}); //另外記錄 func_name
            //cout << "func info: " << label->info() << " - " << decltype(label) << endl;
        }
    }
};
map<int, Func> Func::funcs = {};

class Return: public Func {
    public:
    Func *parent = nullptr;

};

class Call: public Command {
    public:
    string fname = "";
    static map<int, Call> calls;

    Call(int line, string comm_name, string fname, string code): Command(line, comm_name, code) {
        this->fname = fname;
        //this->code = "call code is fixed.";
        calls.insert({line, *this});
    }
};
map<int, Call> Call::calls = {};

class NoArg: public Command {
    public:
    string comm_name = "";
    static map<string, string> Comm;
    static map<int, string> commands;
    NoArg(int line, string comm_name, string code): Command(line, comm_name, code) {
        this->comm_name = comm_name;
        //this->code = code;
        commands.insert({line, comm_name});
    }
    void writeCode() {
        cout << Comm[comm_name];
    }
};
map<string, string>NoArg::Comm = {
    {"add", "@SP\nD=M\nA=D-1\nD=M\nA=A-1\nM=M+D\n@SP\nM=M-1" },
    {"sub", "@SP\nD=M\nA=D-1\nD=M\nA=A-1\nM=M-D\n@SP\nM=M-1" },
    {"neg", "@SP\nD=M\nA=D-1\nD=M\nD=!D\nM=D+1"},
    {"eq" , "@SP\nA=M-1\nD=M\nA=A-1\nD=M-D\n@SP\nM=M-1\nA=M\nA=A-1\nM=-1\n@END\nD;JEQ\n@SP\nA=M-1\nM=0\n(END)" },
    {"gt" , "@SP\nA=M-1\nD=M\nA=A-1\nD=M-D\n@SP\nM=M-1\nA=M\nA=A-1\nM=-1\n@END\nD;JGT\n@SP\nA=M-1\nM=0\n(END)" },
    {"lt" , "@SP\nA=M-1\nD=M\nA=A-1\nD=M-D\n@SP\nM=M-1\nA=M\nA=A-1\nM=-1\n@END\nD;JLT\n@SP\nA=M-1\nM=0\n(END)" },
    {"return", "func end." },
    {"end", "end of program." },
};
map<int, string>NoArg::commands = {};

class Push: public Command {
    public:
    string sub_comm = "";
    static map<string, string> sub_command;
    Push(int line, string comm_name, string sub_comm, string code): Command(line, comm_name, code) {
        if (sub_command.find(sub_comm) != sub_command.end()) {
            this->sub_comm = sub_comm;
            //this->code = Push::sub_command[sub_comm];
        }
        else { cout << "line " << this->line_number << " has error!"; }
    }
};
map<string, string> Push::sub_command = {
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
    string sub_comm = "";

    static map<string, string> sub_command;
    Pop(int line, string comm_name, string sub_comm, string code): Command(line, comm_name, code) {
        if (sub_command.find(sub_comm) != sub_command.end()) {
            this->sub_comm = sub_comm;
            //this->code = Pop::sub_command[sub_comm];
        }
        else { cout << "line " << this->line_number << " has error!"; }
    }
};
map<string, string> Pop::sub_command = {
    {"local", "D=A\n@LCL\nD=M+D\n@SP\nA=M\nM=D\n@SP\nM=M-1\nA=M\nD=M\n@SP\nA=M+1\nA=M\nM=D\n"},
    {"static", "@5\nD=A\n@256\nD=A+D\n@SP\nA=M\nM=D\n@SP\nM=M-1\nA=M\nD=M\n@SP\nA=M+1\nA=M\nM=D\n"},
};

void parse(int line, vector<string> tokens) {
    int args = tokens.size() - 1;
    Command command(0, "", "");// = Command(0, "", ""); //變數宣告公家用，初值 0

    if (tokens[0] == "label" && args == 1) {
        string code = "(" + tokens[1] + ")";
        command = Label(line, tokens[0], tokens[1], code);
    }
    else if (tokens[0] == "func" && args == 1) {
        string code = "(" + tokens[1] + ")";
        command = Func(line, tokens[0], tokens[1], code);
    }
    else if (tokens[0] == "call" && args == 1) {
        if (!Label::isExists(tokens[1]))
            cout << "line " << line << " no such func def." << endl;
        else command = Call(line, tokens[0], tokens[1], "call code temperate.");
    }
    else if (NoArg::Comm.find(tokens[0]) != NoArg::Comm.end() && args == 0) {
        command = NoArg(line, tokens[0], NoArg::Comm[tokens[0]]);
    }
    else if (tokens[0] == "push" && args == 2) {
        command = Push(line, tokens[0], tokens[1], Push::sub_command[tokens[1]]);
    }
    else if (tokens[0] == "pop" && args == 2) {
        command = Pop(line, tokens[0], tokens[1], Pop::sub_command[tokens[1]]);
    }
    else {
        cout << "line " << line << " has errors." << endl;
    }
    command.writeCode();
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
            //map<string, string> com = comm[tokens[0]];//解析出命令是佗一个
            parse((*iter).first, tokens);
        }
        else {
            cout << "line " << (*iter).first << " :command error!" << endl;
            return 0;
        }
        cout << endl;
    }

cout << "Label: " << Label::labels.size() << " Func: " << Func::funcs.size() << endl;
cout << "Command count: " << Command::allCommand.size() << endl;
for (map<int, Command>::iterator it = Command::allCommand.begin(); it!=Command::allCommand.end(); it++) {
    Command com = it->second;
    cout << com.line_number << " - " << com.comm_name << " - " << com.code << endl;
}
    return 0;
}
