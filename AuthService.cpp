#define RAPIDJSON_HAS_STDSTRING 1

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "AuthService.h"
#include "StringUtils.h"
#include "ClientError.h"

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"

using namespace std;
using namespace rapidjson;

bool errorExistsForNewUser(User *create, HTTPResponse *response)
{
  if((create->username == "") || (create->password == "")) //checks if username or password is blank
  {
    throw ClientError::badRequest();
    return true;
  }

  string username = create->username;
  for(unsigned i = 0; i < username.size();i++) //loops through to make sure all characters are lowercase
  {
    if(isupper(username[i]))
    {
      throw ClientError::badRequest();
      return true;
    }
  }
  return false;
}
bool errorExistsForExistingUser(string username, string password, HTTPResponse *response, User *user)
{
  if((username == "") || (password == "")) //checks if username or password is blank
  {
    throw ClientError::badRequest();
    return true;
  }
  for(unsigned i = 0; i < username.size();i++) // checks if all lowercase
  {
    if(isupper(username[i]))
    {
      throw ClientError::badRequest();
      return true;
    }
  }
  if(user->password != password) //checks if the password entered matched previously stored password
  {
    throw ClientError::forbidden();
    return true;
  }
  return false;
}
void createResponseObject(string userId, string userToken, HTTPResponse *response)
{
  Document d;
  Document::AllocatorType& a = d.GetAllocator();
  Value o;
  o.SetObject();
  o.AddMember("auth_token", userToken, a);
  o.AddMember("user_id", userId, a);
  d.Swap(o);
  StringBuffer buffer;
  PrettyWriter<StringBuffer> writer(buffer);
  d.Accept(writer);
  response->setContentType("application/json");
  response->setBody(buffer.GetString() + string("\n"));
}
AuthService::AuthService() : HttpService("/auth-tokens") {
}

void AuthService::post(HTTPRequest *request, HTTPResponse *response) {
  WwwFormEncodedDict req = request->formEncodedBody();
  if(getAuthenticatedUser(request) == NULL)
  {
    //creating new user
    User *create = new User;
    create->user_id = StringUtils::createUserId();
    create->username = req.get("username");
    create->password = req.get("password");
    if(!errorExistsForNewUser(create, response))
    {
      string userAuthToken = StringUtils::createAuthToken();
      m_db->auth_tokens.insert(std::pair <string, User*>(userAuthToken, create));
      m_db->users.insert(std::pair <string, User*>(create->username, create));
      //now we create a response object that returns the auth_token and user_id
      createResponseObject(create->user_id, userAuthToken, response);
      response->setStatus(201);
    }
  }
  else{
    //user already exists
    User *user = getAuthenticatedUser(request);
    string username = req.get("username");
    string password = req.get("password");
    string userAuthToken = StringUtils::createAuthToken();
    if(!errorExistsForExistingUser(username, password, response, user))
    {
      m_db->auth_tokens.insert(std::pair <string, User*>(userAuthToken, user));
      //now we create a response object that returns the auth_token and user_id
      createResponseObject(user->user_id, userAuthToken, response);
      response->setStatus(200);
    }
  }
}

void AuthService::del(HTTPRequest *request, HTTPResponse *response) {
  if(request->hasAuthToken())
  {
    User *user;
    vector<string> path = request->getPathComponents();
    std::map<string,User*>::iterator iter =  m_db->auth_tokens.begin();
    while(iter !=  m_db->auth_tokens.end())
    {
      if(iter->first == path.back()) // loops through to find match for user token to delete
      {
        user = iter->second;
      }
      iter++;
    }
    if(getAuthenticatedUser(request) == user) // if user connected to auth token = the user from request, we erase the token
    {
      m_db->auth_tokens.erase(path.back());
    }
    else{
      throw ClientError::badRequest();
    }
  }
  else
  {
    throw ClientError::badRequest();
  }
}