#include "fast_cgi_data.h"
#include <fcgiapp.h>

namespace fcgiserver {

FastCgiData::FastCgiData(FCGX_Request & FastCgiData)
    : m_request(&FastCgiData)
{
}

FastCgiData::~FastCgiData()
{
	FCGX_Finish_r(m_request);
}

int FastCgiData::read(uint8_t * buffer, size_t bufsize)
{
	return FCGX_GetStr(reinterpret_cast<char*>(buffer), bufsize, m_request->in);
}

int FastCgiData::write(const uint8_t * buffer, size_t bufsize)
{
	return FCGX_PutStr(reinterpret_cast<const char*>(buffer), bufsize, m_request->out);
}

int FastCgiData::error(const uint8_t * buffer, size_t bufsize)
{
	return FCGX_PutStr(reinterpret_cast<const char*>(buffer), bufsize, m_request->err);
}

int FastCgiData::flush_write()
{
	return FCGX_FFlush(m_request->out);
}

int FastCgiData::flush_error()
{
	return FCGX_FFlush(m_request->err);
}

const char **FastCgiData::env() const
{
	return const_cast<const char**>(m_request->envp);
}

} // namespace fcgiserver
