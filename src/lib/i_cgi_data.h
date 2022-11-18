#ifndef FCGISERVER_ICGIDATA_H
#define FCGISERVER_ICGIDATA_H

#include <cstddef>
#include <cstdint>
#include "fcgiserver_defs.h"

namespace fcgiserver
{

class DLL_PUBLIC ICgiData
{
public:
	virtual ~ICgiData() {}

	virtual int read(uint8_t * buffer, size_t bufsize) = 0;
	virtual int write(const uint8_t * buffer, size_t bufsize) = 0;
	virtual int error(const uint8_t * buffer, size_t bufsize) = 0;
	virtual int flush_write() = 0;
	virtual int flush_error() = 0;
	virtual const char **env() const = 0;
};

} // namespace fcgiserver

#endif // FCGISERVER_ICGIDATA_H
