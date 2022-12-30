#define RAPIDJSON_HAS_STDSTRING 1

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "AccountService.h"
#include "ClientError.h"

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"

using namespace std;
using namespace rapidjson;

void createResponseObject(User *user, HTTPResponse *response)
{
  Document d;
  Document::AllocatorType& a = d.GetAllocator();
  Value o;
  o.SetObject();
  o.AddMember("balance", user->balance, a);
  o.AddMember("email", user->email, a);
  d.Swap(o);
  StringBuffer buffer;
  PrettyWriter<StringBuffer> writer(buffer);
  d.Accept(writer);
  response->setContentType("application/json");
  response->setBody(buffer.GetString() + string("\n"));
}

AccountService::AccountService() : HttpService("/users") {
  
}

void AccountService::get(HTTPRequest *request, HTTPResponse *response) {
  if(getAuthenticatedUser(request))
  {
    vector<string> path = request->getPathComponents();
    User *userToGetFrom = getAuthenticatedUser(request);
    if(userToGetFrom->user_id != path.back()) //error check
    {
      throw ClientError::forbidden(); 
    }
    else{
      createResponseObject(userToGetFrom,response); // prints balance and email
    }
  }
  else
  {
    throw ClientError::badRequest();
  }
}



void AccountService::put(HTTPRequest *request, HTTPResponse *response) {
  if(request->hasAuthToken())
  {
    vector<string> path = request->getPathComponents();
    WwwFormEncodedDict req = request->formEncodedBody();
    if(req.get("email") == "") //error check to see if email is blank
    {
      throw ClientError::badRequest();
    }
    else
    {
      User *userToUpdate = getAuthenticatedUser(request);
      if(userToUpdate->user_id != path.back()) //bad call
      {
        throw ClientError::unauthorized();
      }
      else
      {
        userToUpdate->email = req.get("email");
        createResponseObject(userToUpdate, response); // returns balance and email
      }
    }

  }
  else{
    throw ClientError::unauthorized(); // bad call
  }
}
