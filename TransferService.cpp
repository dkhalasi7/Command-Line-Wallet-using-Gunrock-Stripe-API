#define RAPIDJSON_HAS_STDSTRING 1

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#include <iostream>
#include <map>
#include <string>
#include <sstream>

#include "TransferService.h"
#include "ClientError.h"

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"

using namespace rapidjson;
using namespace std;

void createResponseObject(vector<Transfer*> transferList, HTTPResponse *response, User *user){
  Document document;
  Document::AllocatorType& a = document.GetAllocator();
  Value o;
  o.SetObject();
  o.AddMember("balance", user->balance, a);
  Value array;
  array.SetArray();
  for(unsigned i = 0; i < transferList.size(); i++){ 
    Value to;
    to.SetObject();
    to.AddMember("from", transferList[i]->from->username , a);
    to.AddMember("to", transferList[i]->to->username, a);
    to.AddMember("amount", transferList[i]->amount , a);
    array.PushBack(to, a);
  }
  o.AddMember("transfers", array, a);
  document.Swap(o);
  StringBuffer buffer;
  PrettyWriter<StringBuffer> writer(buffer);
  document.Accept(writer);
  response->setContentType("application/json");
  response->setBody(buffer.GetString() + string("\n"));
}

TransferService::TransferService() : HttpService("/transfers") { }


void TransferService::post(HTTPRequest *request, HTTPResponse *response) {
  WwwFormEncodedDict req = request->formEncodedBody();
  if(req.get("to") == "" || req.get("amount") == "")
  {
    throw ClientError::badRequest();
  }
  int amount = stoi(req.get("amount"));
  User *to = NULL;
  std::map<string, User*>::iterator iter = m_db->users.begin();
  while(iter != m_db->users.end())
  {
    if(iter->first == req.get("to"))
    {
      to = iter->second;
    }
    iter++;
  }

  User *from = getAuthenticatedUser(request);
  // if from balance is lower than the amount to send, if amount is < 0, to user DNE, to and from are same
  if(from->balance < amount || amount < 0)
  {
    throw ClientError::badRequest();
  }
  if(to == NULL || to == from)
  {
    throw ClientError::badRequest();
  }
  Transfer *transfer = new Transfer(); // creates a new transfer object
  transfer->amount = amount;
  transfer->to = to;
  transfer->from = from;

  from->balance = from->balance - amount; //editing balances
  to->balance = to->balance + amount;
  m_db->transfers.push_back(transfer);
  vector<Transfer*> transList;
  unsigned tranSize = m_db->transfers.size();
  for(unsigned i =0 ; i < tranSize; i++)
  {
    if(from->username == m_db->transfers[i]->from->username && to->username == m_db->transfers[i]->to->username) //matches the from username and to username
    {
      transList.push_back(m_db->transfers[i]); //creates list for all transfers
    }
  }
  createResponseObject(transList, response, from);

}
