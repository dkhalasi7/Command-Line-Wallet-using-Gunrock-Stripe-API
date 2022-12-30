#include <iostream>

#include <stdlib.h>
#include <stdio.h>

#include "HttpService.h"
#include "ClientError.h"

using namespace std;

HttpService::HttpService(string pathPrefix) {
  this->m_pathPrefix = pathPrefix;
}

User *HttpService::getAuthenticatedUser(HTTPRequest *request)  {
  if(request->hasAuthToken()) //means there is a log in request with an authorization token
  {
  string authorizationToken = request->getAuthToken();
  std::map<string, User*>::iterator iter = m_db->auth_tokens.begin();
  while(iter != m_db->auth_tokens.end()) //loops to find match for auth token
  {
    if(iter->first == authorizationToken)
    {
    return iter->second;
    }
    iter++;
  }
  }
  else
  {
  WwwFormEncodedDict req = request->formEncodedBody();
  std::map<string, User*>::iterator iter = m_db->users.begin();
  while(iter != m_db->users.end()) //loops through checking if a user has the same username from request
  {
    if(iter->first == req.get("username")) 
    {
    return iter->second;
    }
    iter++;
  }

  }
  return NULL;
}

string HttpService::pathPrefix() {
  return m_pathPrefix;
}

void HttpService::head(HTTPRequest *request, HTTPResponse *response) {
  cout << "HEAD " << request->getPath() << endl;
  throw ClientError::methodNotAllowed();
}

void HttpService::get(HTTPRequest *request, HTTPResponse *response) {
  cout << "GET " << request->getPath() << endl;
  throw ClientError::methodNotAllowed();
}

void HttpService::put(HTTPRequest *request, HTTPResponse *response) {
  cout << "PUT " << request->getPath() << endl;
  throw ClientError::methodNotAllowed();
}

void HttpService::post(HTTPRequest *request, HTTPResponse *response) {
  cout << "POST " << request->getPath() << endl;
  throw ClientError::methodNotAllowed();
}

void HttpService::del(HTTPRequest *request, HTTPResponse *response) {
  cout << "DELETE " << request->getPath() << endl;
  throw ClientError::methodNotAllowed();
}

