#include "test_mock_cgi_data.h"

MockCgiData::MockCgiData(std::string inbuf, const char **envp)
    : m_envp(envp)
{
	m_readbuf.swap(inbuf);
}

MockCgiData::~MockCgiData()
{
}

int MockCgiData::read(uint8_t * buffer, size_t bufsize)
{
	if (m_readbuf.empty())
		return -1;

	if (bufsize >= m_readbuf.size())
	{
		bufsize = m_readbuf.size();
		std::copy(m_readbuf.cbegin(), m_readbuf.cend(), buffer);
		m_readbuf.clear();
	}
	else
	{
		std::string::iterator split = m_readbuf.begin();
		std::advance(split, bufsize);
		std::copy(m_readbuf.begin(), split, buffer);
		m_readbuf.erase(m_readbuf.begin(), split);
	}

	return static_cast<int>(bufsize);
}

int MockCgiData::write(const uint8_t * buffer, size_t bufsize)
{
	m_writebuf.append(reinterpret_cast<const char *>(buffer), bufsize);
	return bufsize;
}

int MockCgiData::error(const uint8_t * buffer, size_t bufsize)
{
	m_errorbuf.append(reinterpret_cast<const char *>(buffer), bufsize);
	return bufsize;
}

int MockCgiData::flush_write()
{
	return 0;
}

int MockCgiData::flush_error()
{
	return 0;
}

const char **MockCgiData::env() const
{
	return m_envp;
}



#include <catch2/catch.hpp>
#include <cstring>

TEST_CASE("MockCgiData", "[request]")
{
	const char *envp[] = {
	    "THINGS=TRUE",
	    nullptr
	};

	MockCgiData cgidata("This is a line of input", envp);

	SECTION("Init")
	{
		REQUIRE(cgidata.m_readbuf == "This is a line of input");
		REQUIRE(cgidata.m_writebuf.empty());
		REQUIRE(cgidata.m_errorbuf.empty());
		REQUIRE(cgidata.m_envp == envp);
	}

	SECTION("Read")
	{
		int retval;
		uint8_t buf[256];

		buf[4] = 'X';
		retval = cgidata.read(buf, 4);
		REQUIRE(retval == 4);
		REQUIRE(std::memcmp(buf, "ThisX", 5) == 0);
		REQUIRE(cgidata.m_readbuf == " is a line of input");

		buf[8] = 'X';
		retval = cgidata.read(buf, 8);
		REQUIRE(retval == 8);
		REQUIRE(std::memcmp(buf, " is a liX", 9) == 0);
		REQUIRE(cgidata.m_readbuf == "ne of input");

		buf[11] = 'X';
		retval = cgidata.read(buf, 20);
		REQUIRE(retval == 11);
		REQUIRE(std::memcmp(buf, "ne of inputX", 12) == 0);
		REQUIRE(cgidata.m_readbuf.empty());

		buf[0] = 'X';
		retval = cgidata.read(buf, 20);
		REQUIRE(retval == -1);
		REQUIRE(buf[0] == 'X');
		REQUIRE(cgidata.m_readbuf.empty());

		// Post state checks
		REQUIRE(cgidata.m_writebuf.empty());
		REQUIRE(cgidata.m_errorbuf.empty());
		REQUIRE(cgidata.m_envp == envp);
	}

	SECTION("Write")
	{
		int retval;
		const char *line;

		line = "Some content";
		retval = cgidata.write(reinterpret_cast<const uint8_t*>(line), 12);
		REQUIRE(retval == 12);
		REQUIRE(cgidata.m_writebuf == "Some content");

		line = " is what we want!";
		retval = cgidata.write(reinterpret_cast<const uint8_t*>(line), 16);
		REQUIRE(retval == 16);
		REQUIRE(cgidata.m_writebuf == "Some content is what we want");

		line = " to achieve together with everyone";
		retval = cgidata.write(reinterpret_cast<const uint8_t*>(line), 11);
		REQUIRE(retval == 11);
		REQUIRE(cgidata.m_writebuf == "Some content is what we want to achieve");

		// Post state checks
		REQUIRE(cgidata.m_readbuf == "This is a line of input");
		REQUIRE(cgidata.m_errorbuf.empty());
		REQUIRE(cgidata.m_envp == envp);
	}

	SECTION("Error")
	{
		int retval;
		const char *line;

		line = "Some content";
		retval = cgidata.error(reinterpret_cast<const uint8_t*>(line), 12);
		REQUIRE(retval == 12);
		REQUIRE(cgidata.m_errorbuf == "Some content");

		line = " is what we want!";
		retval = cgidata.error(reinterpret_cast<const uint8_t*>(line), 16);
		REQUIRE(retval == 16);
		REQUIRE(cgidata.m_errorbuf == "Some content is what we want");

		line = " to achieve together with everyone";
		retval = cgidata.error(reinterpret_cast<const uint8_t*>(line), 11);
		REQUIRE(retval == 11);
		REQUIRE(cgidata.m_errorbuf == "Some content is what we want to achieve");

		// Post state checks
		REQUIRE(cgidata.m_readbuf == "This is a line of input");
		REQUIRE(cgidata.m_writebuf.empty());
		REQUIRE(cgidata.m_envp == envp);
	}
}
