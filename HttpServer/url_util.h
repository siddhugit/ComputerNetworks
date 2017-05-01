#ifndef URL_UTIL_H__
#define URL_UTIL_H__

#include<string>
#include <vector>

class UrlUtil
{
	static std::vector<std::string> parseUrlImpl(const std::string& urlStr);
	public:
		static std::vector<std::string> parseUrl(const std::string& urlStr);
		static bool strCmpIgnCase(const std::string& s1,const std::string& s2);
};

#endif //URL_UTIL_H__
