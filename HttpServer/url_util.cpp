#include <iostream>
#include <stdexcept>
#include "url_util.h"

bool UrlUtil::strCmpIgnCase(const std::string& s1,const std::string& s2)
{
	unsigned int sz = s1.size();
	if(s2.size() != sz)
	{
		return false;
	}
	for(unsigned int i = 0;i < sz; ++i)
	{
		if(tolower(s1[i]) != tolower(s2[i]))
		{
			return false;
		}
	}
	return true; 
}

std::vector<std::string> UrlUtil::parseUrlImpl(const std::string& urlStr)
{
	std::string::size_type pos = urlStr.find("://");
	if(pos == std::string::npos)
	{
		std::runtime_error ex("bad url");
		throw ex;
	}
	std::string scheme = urlStr.substr(0,pos);
	pos = urlStr.find_first_not_of("://",pos);
	if(pos == std::string::npos)
	{
		std::runtime_error ex("bad url");
		throw ex;
	}
	std::string::size_type pos1 = urlStr.find("/",pos);
	if(pos1 == std::string::npos)
	{
		pos1 = urlStr.size();
	}
	std::string hostAddr = urlStr.substr(pos,pos1 - pos);
	//resolve port number
	std::string portNum = "80";
	std::string::size_type portPos = hostAddr.find(":");
	if(portPos != std::string::npos)
	{
		if(portPos + 1 != hostAddr.size())
		{
			portNum = hostAddr.substr(portPos + 1,hostAddr.size() - portPos - 1);
		}
		hostAddr = hostAddr.substr(0,portPos);
	}

	std::string resource = "";
	pos1 = urlStr.find_first_not_of("/",pos1);
	if(pos1 != std::string::npos)
	{
		resource = urlStr.substr(pos1,urlStr.size() - pos1);
	}
	std::vector<std::string> result;
	result.push_back(scheme);
	result.push_back(hostAddr);
	result.push_back(portNum);
	result.push_back(resource);
	return result;
}

std::vector<std::string> UrlUtil::parseUrl(const std::string& urlStr)
{
	std::vector<std::string> result;
	try
	{
		result = parseUrlImpl(urlStr);
	}
	catch(const std::exception& ex)
	{
		std::cout<<ex.what()<<"\n";
	}
	return result;
}
