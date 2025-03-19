#pragma once

#include "Directive.hpp"
#include "HTTPRequest.hpp"
#include "HTTPResponse.hpp"
#include "Handler.hpp"

class DeleteClientMethod : public Handler {
 protected:
  HTTPRequest _http;

 public:
  DeleteClientMethod(HTTPRequest& http) : Handler() { _http = http; }
  ~DeleteClientMethod() {}
  void handleRequest(HTTPResponse& httpResponse);
};
