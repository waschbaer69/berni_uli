#include "Mmessage.h"

Mmessage::Mmessage(string text)
{
	fullmsg = text;
	string msg[4];
	istringstream ss(text);
    
    
    string temp;
    getline(ss, temp);
    sender = temp;
    getline(ss, temp);
    empf = temp;
    getline(ss, temp);
    betreff = temp;
    while(getline(ss,temp))
    {
      if(temp == "END") break;
      else
      {
        nachricht += temp + "\r\n";
      }
    }
}

string Mmessage::get_sender()
{
  return this->sender;
}
string Mmessage::get_empf()
{
  return this->empf;
}
string Mmessage::get_betreff()
{
  return this->betreff;
}
string Mmessage::get_nachricht()
{
  return this->nachricht;
}