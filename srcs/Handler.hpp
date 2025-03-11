#pragma once

#include "Directive.hpp"
#include "HTTPRequest.hpp"
#include "HTTPResponse.hpp"

class Handler : public Directive, HTTPRequest, HTTPResponse {
 protected:
  Directive _conf;
  HTTPResponse _http_response;
  Handler* nextHandler;

 public:
  Handler();
  virtual ~Handler() {}

  void setNextHandler(Handler* handler) { nextHandler = handler; }

  virtual void handleRequest(const HTTPRequest& request) = 0;
};
