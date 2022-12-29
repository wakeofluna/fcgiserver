#include "user_context.h"

using namespace fcgiserver;

UserContext::~UserContext() = default;

int UserContext::type() const
{
	// If users don't want to use the type identifier then provide a sane default
	return 0;
}

void UserContext::tick()
{
	// Default implementation does nothing. How could it?
}
