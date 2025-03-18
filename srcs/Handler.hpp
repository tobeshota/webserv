#pragma once

#include "Directive.hpp"
#include "HTTPRequest.hpp"
#include "HTTPResponse.hpp"

class Handler {
 protected:
  Handler* _nextHandler;

 public:
  Handler() : _nextHandler(NULL){};
  virtual ~Handler() { ; }
  void setNextHandler(Handler* handler) { _nextHandler = handler; }
  virtual void handleRequest(HTTPResponse& httpResponse) = 0;
};
