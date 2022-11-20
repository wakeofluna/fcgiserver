#ifndef FCGISERVER_SYMBOLS_H
#define FCGISERVER_SYMBOLS_H

namespace fcgiserver
{

class Symbol;

namespace symbols
{

// Common Header symbols
extern Symbol const Status;
extern Symbol const ContentLength;
extern Symbol const ContentType;

// Common Environment/Request symbols
extern Symbol const CONTENT_TYPE;
extern Symbol const CONTENT_LENGTH;
extern Symbol const REQUEST_METHOD;
extern Symbol const REQUEST_SCHEME;
extern Symbol const QUERY_STRING;
extern Symbol const SCRIPT_NAME;
extern Symbol const DOCUMENT_URI;
extern Symbol const PATH_INFO;
extern Symbol const REMOTE_ADDR;
extern Symbol const REMOTE_PORT;
extern Symbol const HTTP_USER_AGENT;
extern Symbol const HTTP_DNT;

// Common HTTP request methods
extern Symbol const GET;
extern Symbol const PUT;
extern Symbol const POST;
extern Symbol const HEAD;
extern Symbol const PATCH;
extern Symbol const TRACE;
extern Symbol const DELETE;
extern Symbol const CONNECT;
extern Symbol const OPTIONS;

}

}

#endif // FCGISERVER_SYMBOLS_H
