#define RAPIDJSON_HAS_STDSTRING 1

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#include <iostream>
#include <map>
#include <string>
#include <sstream>

#include "DepositService.h"
#include "Database.h"
#include "ClientError.h"
#include "HTTPClientResponse.h"
#include "HttpClient.h"

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"

using namespace rapidjson;
using namespace std;

void createResponseObject(vector<Deposit*> depositList, HTTPResponse *response, User *user){
  Document document;
  Document::AllocatorType& a = document.GetAllocator();
  Value o;
  o.SetObject();
  o.AddMember("balance", user->balance, a);
  Value array;
  array.SetArray();
  for(unsigned i = 0; i < depositList.size(); i++){ 
    Value to;
    to.SetObject();
    to.AddMember("to", depositList[i]->to->username, a);
    to.AddMember("amount", depositList[i]->amount , a);
    to.AddMember("stripe_charge_id", depositList[i]->stripe_charge_id , a);
    array.PushBack(to, a);
  }
  o.AddMember("deposits", array, a);
  document.Swap(o);
  StringBuffer buffer;
  PrettyWriter<StringBuffer> writer(buffer);
  document.Accept(writer);
  response->setContentType("application/json");
  response->setBody(buffer.GetString() + string("\n"));
}

DepositService::DepositService() : HttpService("/deposits") { }

void DepositService::post(HTTPRequest *request, HTTPResponse *response) {
  WwwFormEncodedDict req = request->formEncodedBody();
  if(req.get("stripe_token") == "" || req.get("amount") == "" || !request->hasAuthToken()) // error check for empty stripe token, amount, and if no auth token is used
  {
    throw ClientError::badRequest();
  }
  else
  {
    int amount = stoi(req.get("amount"));
    if(amount < 50) // if less than 50 cents its a bad request
    {
      throw ClientError::badRequest();
    }
  }
  int amount = stoi(req.get("amount"));
  User *user = getAuthenticatedUser(request);
  string stripeTok = req.get("stripe_token");

  WwwFormEncodedDict body; // create body for client
  body.set("amount", amount);
  body.set("source", stripeTok);
  body.set("currency", "usd");
  string encoded_body = body.encode();

  HttpClient client("api.stripe.com", 443, true);
  client.set_basic_auth(m_db->stripe_secret_key, "");
  HTTPClientResponse *client_response = client.post("/v1/charges", encoded_body);

  if(client_response->success())
  {
    Document *d = client_response->jsonBody();
    Deposit *deposit = new Deposit();
    deposit->amount = (*d)["amount"].GetInt();
    user->balance = user->balance + deposit->amount;
    deposit->to = user;
    deposit->stripe_charge_id = (*d)["id"].GetString();
    delete d;
    m_db->deposits.push_back(deposit);
    vector<Deposit*> userDepositList;
    unsigned size = m_db->deposits.size();
    for(unsigned i = 0; i < size; i++)
    {
      if(m_db->deposits[i]->to == user)
      {
        userDepositList.push_back(m_db->deposits[i]); // gets history of deposits made
      }
    }
    createResponseObject(userDepositList, response, user); //returns to, amount, stripe id, and all deposits
  }
  else
  {
    throw ClientError::badRequest();
  }

}
