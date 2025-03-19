#pragma once

#include "Directive.hpp"
#include "HTTPRequest.hpp"
#include "HTTPResponse.hpp"
#include "Handler.hpp"

class DeleteClientMethod : public Handler {
 protected:
  HTTPRequest _httpRequest;
  Directive _rootDirective;

 public:
  DeleteClientMethod(HTTPRequest& _httpRequest, Directive rootDirective) : Handler() {
    this->_httpRequest = _httpRequest;
    this->_rootDirective = rootDirective;
  }
  ~DeleteClientMethod() {}
  std::string getFullPath() const;
  void handleRequest(HTTPResponse& httpResponse);
};
