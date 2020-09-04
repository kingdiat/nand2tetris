#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <string.h>
#include <bitset>
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

bool isUpper(string s) {
   for (int j=0; j<s.length(); j++) {
      if ( islower(s.at(j)) ) return false;
   }
   return true;
}

bool isLower(string s) {
   for (int j=0; j<s.length(); j++) {
      if ( isupper(s.at(j)) ) return false;
   }
   return true;
}

bool isLegal(string token) {  //does not start with digit, contains letters, _ , . , $ , :
   for (int j=0; j<token.length(); j++) {  //只要一个無合著袂使
      char c = token.at(j);
      if (!isalnum(c) && (c!='$') && (c!=':') && (c!='_') && (c!='.') ) return false;
   }
   if ( isdigit(token.at(0)) ) return false;
   return true;
}

bool isDigit(string token) {
   for (int j=0; j<token.length(); j++) {
      if ( !isdigit(token.at(j)) ) {
         return false;
      }
   }
   return true;
}

std::pair<string, string> parse(string s, string delim) {
   if ( s.find(delim)==string::npos ) {
      return make_pair(s, "");
   }
   else {
      int pos = s.find(delim);
      string sa = s.substr(0, pos);
      string sb = s.substr(pos+1, s.length()-1);
      return make_pair(sa, sb);
   }
}

string jBind(string a, string b) {  //jump condition b
   return "111" + a + "000" + b;
}

string cBind(string a, string b) {  //no jump 000
   return "111" + a + b + "000";
}

int main(int argc, char* argv[]) {
   int cmmd_order = 0;              //記錄機器指令數，目前為cout bitset次數
   map<int, string> command;
   list<pair<int, string>> pass2;   // <cmmd_order, Tag_name>
   map<string, string> l_cmmd = {{"", "000"}, {"M", "001"}, {"D", "010"}, {"MD", "011"},
      {"A", "100"}, {"AM", "101"}, {"AD", "110"}, {"AMD", "111"} };
   map<string, string> r_cmmd = { {"0", "0101010"}, {"1", "0111111"}, {"-1", "0111010"},
      {"D", "0001100"}, {"A", "0110000"}, {"!D", "0001101"}, {"!A", "0110001"},
      {"-D", "0001111"}, {"-A", "0110011"}, {"D+1", "0011111"}, {"A+1", "0110111"},
      {"D-1", "0001110"}, {"A-1", "0110010"}, {"D+A", "0000010"}, {"D-A", "0010011"},
      {"A-D", "0000111"}, {"D&A", "0000000"}, {"D|A", "0010101"}, {"M", "1110000"},
      {"!M", "1110001"}, {"-M", "1110011"},{"M+1", "1110111"}, {"M-1", "1110010"},
      {"D+M", "1000010"}, {"D-M", "1010011"}, {"M-D", "1000111"}, {"D&M", "1000000"},
      {"D|M", "1010101"} };
   map<string, string> jump = { {"", "000"}, {"JGT", "001"}, {"JEQ", "010"},
      {"JGE", "011"}, {"JLT", "100"}, {"JLE", "110"}, {"JNE", "101"}, {"JMP", "111"} };
   map<string, int> SYMBOL = {         //儲存變數位址--內建及自設
      {"SP", 0x0000}, {"LCL", 0x0001}, {"ARG", 0x0002}, {"THIS", 0x0003},
      {"THAT", 0x0004}, {"R0", 0x0000}, {"R1", 0x0001}, {"R2", 0x0002},
      {"R3", 0x0003}, {"R4", 0x0004}, {"R5", 0x0005}, {"R6", 0x0006},
      {"R7", 0x0007}, {"R8", 0x0008}, {"R9", 0x0009}, {"R10", 0x000A},
      {"R11", 0x000B}, {"R12", 0x000C}, {"R13", 0x000D}, {"R14", 0x000E},
      {"R15", 0x000F}, {"SCREEN", 0x4000}, 
      {"KBD", 0x6000}
   };

   for (map<string, int>::iterator it=SYMBOL.begin(); it!=SYMBOL.end(); it++) {
      //cout << it->first << "->" << it->second << endl;  本逝OK，暫時莫輸出
   }

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

   int var_addr = 0x0010;  //builtin first variable address
   for (map<int, vector<string>>::iterator it=lines.begin(); it!=lines.end(); it++) {
      vector<string>::iterator jt = it->second.begin();
      string token = *jt;

      if (token.substr(0,1)=="@") {  //@指令
         string tmp = token.substr(1, token.length()-1);
         if ( isDigit(tmp) ) {   //@數字
            int n = stoi(tmp);
            char p[tmp.length()+1];
            sprintf(p, "%d", n);
            if (n<0 || n>=16384) { cout << "number out of bound" << endl; return -1; }
            cout << cmmd_order << " ";
            cout << bitset<16>(n) << " " << it->first << endl;
            command.insert({cmmd_order, bitset<16>(n).to_string()});
            cmmd_order++;
         }
         else {    //@變數 and Tag
            if ( !isLegal(tmp) ) {
               cout << "string is illegal  " << it->first << endl;
               return -1;
            }
            else if (SYMBOL.find(tmp)==SYMBOL.end()) {  //not in symbol table
               if ( isLower(tmp) ) {  //check lower case = 變數
                  SYMBOL.insert({tmp, var_addr});
                  cout << cmmd_order << " ";
                  cout << bitset<16>(var_addr) << " " << it->first << endl;
                  command.insert({cmmd_order, bitset<16>(var_addr).to_string()});
                  var_addr++;
                  cmmd_order++;
               }
               else if ( isUpper(tmp) ) { //Tag
                  cout << cmmd_order << " ";
                  cout << bitset<16>(0) << " " << it->first << endl;
                  command.insert({cmmd_order, bitset<16>(0).to_string()});
                  pass2.push_back(make_pair(cmmd_order, tmp));
                  cmmd_order++;
               }
               else cout << "token invalid!" << endl;
            }
            else {  // variable and tag is in symbol table
              cout << cmmd_order << " ";
              cout << bitset<16>(SYMBOL[tmp]) << " " << it->first << endl;
              command.insert({cmmd_order, bitset<16>(SYMBOL[tmp]).to_string()});
              cmmd_order++;
            }
         }
      }
      //(addr tag)
      else if ( token.substr(0,1)=="(" && token.substr(token.length()-1, 1)==")" ) {
         if ( (++jt)!=it->second.end()) { //袂使有第2个token
            cout << "Tag must be a line alone\n";
            return -1;
         }
         string real_tok = token.substr(1, token.length()-2);  //去掉括號
         if (!isUpper(real_tok) ) {
            cout << "Tag must be upper case! " << it->first << endl;
            return -1;
         }
         if ( SYMBOL.count( real_tok ) ) {
            cout << real_tok << " repeat defined " << it->first << endl;
         }
         else {
            SYMBOL.insert({real_tok, cmmd_order});//袂使++,因為是虛指令
            cout << bitset<16>(cmmd_order) << " " << it->first << endl;
         }
      }
      //計算指令
      else {
         if (token.find("=")!=string::npos) {  //calculate
            std::pair<string, string> p = parse(token, "=");
            if (r_cmmd[p.second]=="" || l_cmmd[p.first]=="") {
               cout << "=command error  " << it->first << endl;
            }
            else  {
               cout << cmmd_order << " ";
               cout << cBind(r_cmmd[p.second], l_cmmd[p.first]) << " --" << it->first << endl;
               command.insert({cmmd_order, cBind(r_cmmd[p.second], l_cmmd[p.first])});
               cmmd_order++;
            }
         }
         else if (token.find(";")!=string::npos) {  //jump
            std::pair<string, string> q = parse(token, ";");
            if (r_cmmd[q.first]=="" || jump[q.second]=="") {
               cout << ";command error  " << it->first << endl;
            }
            else {
               cout << cmmd_order << " ";
               cout << jBind(r_cmmd[q.first], jump[q.second]) << " " << it->first << endl;
               command.insert({cmmd_order, jBind(r_cmmd[q.first], jump[q.second])});
               cmmd_order++;
            }
         }
         else cout << "other error " << it->first << endl;
      }
   }
   //show symbol in table , don't care!
   /*for (map<string, int>::iterator it=SYMBOL.begin(); it!=SYMBOL.end(); it++) {
      cout << (*it).first << " " << (*it).second << endl;
   }*/

   //from now on , second pass
   for (list<pair<int, string>>::iterator it=pass2.begin(); it!=pass2.end(); it++) {
      command[it->first] = bitset<16>(SYMBOL[it->second]).to_string();
   }
   //output command
   for (map<int, string>::iterator it=command.begin(); it!=command.end(); it++) {
      cout << it->first << " " << it->second << endl;
   }

   return 0;
}


