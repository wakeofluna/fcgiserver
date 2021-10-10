#ifndef TEST_MOCK_CGI_DATA_H
#define TEST_MOCK_CGI_DATA_H

#include "i_cgi_data.h"
#include <string>

class MockCgiData : public fcgiserver::ICgiData
{
public:
	MockCgiData(std::string inbuf, const char **envp);
	~MockCgiData();

	int read(uint8_t * buffer, size_t bufsize) override;
	int write(const uint8_t * buffer, size_t bufsize) override;
	int error(const uint8_t * buffer, size_t bufsize) override;
	int flush_write() override;
	int flush_error() override;
	const char **env() const override;

	std::string m_readbuf;
	std::string m_writebuf;
	std::string m_errorbuf;
	const char **m_envp;
};


#endif // TEST_MOCK_CGI_DATA_H
