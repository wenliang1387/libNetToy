
#include <base/Logger.h>
#include <iostream>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>  
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/log/support/date_time.hpp>
#include <stdio.h>

boost::log::sources::severity_logger_mt<boost::log::trivial::severity_level> lg;

namespace logging = boost::log;  
namespace src = boost::log::sources;  
namespace expr = boost::log::expressions;  
namespace sinks = boost::log::sinks;  
namespace keywords = boost::log::keywords;

boost::function<void(const char*)> gdisplayLog;

Logger::Logger()
{

}
Logger::~Logger()
{
}

void Logger::init(const string &logName)
{
	logging::add_common_attributes();
	std::string fileName = logName + "_%Y%m%d%H%M%S.log";
	boost::log::add_file_log
		(
		keywords::file_name = fileName,
		keywords::rotation_size = 10 * 1024 * 1024,
		keywords::time_based_rotation = sinks::file::rotation_at_time_point(0, 0, 0),
		keywords::auto_flush = true,  
		keywords::format =
		(
		expr::stream
		<< expr::format_date_time< boost::posix_time::ptime >("TimeStamp", "%Y-%m-%d %H:%M:%S")
		<< ": (" << std::setw(7) << std::setfill(' ') 
		<< logging::trivial::severity
		<< ")"<< expr::message
		)
		);
	boost::log::core::get()->set_filter
		(
		boost::log::trivial::severity >= boost::log::trivial::debug
		);
}
boost::log::sources::severity_logger_mt<boost::log::trivial::severity_level> &Logger::getLog()
{
	return lg;
}


//通过display和setDisplay可以切换输出函数，这样就能让显示随时在控制台和gui之间切换了

void Logger::display(const string &msg)
{
	if(gdisplayLog){
		gdisplayLog(msg.c_str());
	}
}
void Logger::setDisplay(boost::function<void(const char*)> displayLog)
{
	gdisplayLog = displayLog;
}
void consoleLog(const char *str)
{
	printf("log:%s\n", str);
}
void guiLog(const char *str)
{
	//to do 实现gui显示
}