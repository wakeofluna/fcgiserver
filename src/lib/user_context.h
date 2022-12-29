#ifndef FCGISERVER_USER_CONTEXT_H
#define FCGISERVER_USER_CONTEXT_H

#include "fcgiserver_defs.h"

namespace fcgiserver
{

class DLL_PUBLIC UserContext
{
public:
	UserContext() = default;
	virtual ~UserContext() = 0;
	virtual int type() const;
	virtual void tick();
};

}

#endif // FCGISERVER_USER_CONTEXT_H
