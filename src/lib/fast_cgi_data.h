#ifndef FCGISERVER_FASTCGIDATA_H
#define FCGISERVER_FASTCGIDATA_H

#include "fcgiserver_defs.h"
#include "i_cgi_data.h"

struct FCGX_Request;

namespace fcgiserver {

class DLL_PRIVATE FastCgiData : public ICgiData
{
public:
	FastCgiData(FCGX_Request & request);
	~FastCgiData();

	int read(uint8_t * buffer, size_t bufsize) override;
	int write(const uint8_t * buffer, size_t bufsize) override;
	int error(const uint8_t * buffer, size_t bufsize) override;
	int flush_write() override;
	int flush_error() override;
	const char **env() const override;

private:
	FCGX_Request * m_request;
};

} // namespace fcgiserver

#endif // FCGISERVER_FASTCGIDATA_H
