#include "fcgiserver_defs.h"
#include "symbol.h"
#include <string_view>

using namespace std::literals::string_view_literals;

namespace fcgiserver
{
namespace symbols
{

// Common Header symbols
DLL_PUBLIC Symbol Status("Status");
DLL_PUBLIC Symbol ContentLength("Content-Length");
DLL_PUBLIC Symbol ContentType("Content-Type");

// Common Environment/Request symbols
DLL_PUBLIC Symbol CONTENT_TYPE("CONTENT_TYPE");
DLL_PUBLIC Symbol CONTENT_LENGTH("CONTENT_LENGTH");
DLL_PUBLIC Symbol REQUEST_METHOD("REQUEST_METHOD");
DLL_PUBLIC Symbol REQUEST_SCHEME("REQUEST_SCHEME");
DLL_PUBLIC Symbol QUERY_STRING("QUERY_STRING");
DLL_PUBLIC Symbol SCRIPT_NAME("SCRIPT_NAME");
DLL_PUBLIC Symbol DOCUMENT_URI("DOCUMENT_URI");
DLL_PUBLIC Symbol PATH_INFO("PATH_INFO");
DLL_PUBLIC Symbol REMOTE_ADDR("REMOTE_ADDR");
DLL_PUBLIC Symbol REMOTE_PORT("REMOTE_PORT");
DLL_PUBLIC Symbol HTTP_USER_AGENT("HTTP_USER_AGENT");
DLL_PUBLIC Symbol HTTP_DNT("HTTP_DNT");

// Common HTTP request methods
DLL_PUBLIC Symbol GET("GET");
DLL_PUBLIC Symbol PUT("PUT");
DLL_PUBLIC Symbol POST("POST");
DLL_PUBLIC Symbol HEAD("HEAD");
DLL_PUBLIC Symbol PATCH("PATCH");
DLL_PUBLIC Symbol TRACE("TRACE");
DLL_PUBLIC Symbol DELETE("DELETE");
DLL_PUBLIC Symbol CONNECT("CONNECT");
DLL_PUBLIC Symbol OPTIONS("OPTIONS");

}
}
