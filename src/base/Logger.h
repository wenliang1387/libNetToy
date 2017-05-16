#pragma once

#include <boost/log/trivial.hpp>
#include <boost/function.hpp>
#include <boost/thread.hpp>

#include <string>
#include <iostream>
#include <sstream> 
using namespace std;

void consoleLog(const char *str);
void guiLog(const char *str);

class Logger
{
public:
	Logger();
	~Logger();

	static void init(const string &logName);
	static boost::log::sources::severity_logger_mt<boost::log::trivial::severity_level> &getLog();
	static void display(const string &msg);

	static void setDisplay(boost::function<void(const char*)> displayLog = consoleLog);
};

#define LOG_LINE "["<<__FILE__<<":"<<__LINE__<<"]"
#define THREAD_ID "(" << std::setw(4) << std::setfill('0') << boost::this_thread::get_id() << ") : "
#define LogTrace(show, msg) BOOST_LOG_SEV(Logger::getLog(), boost::log::trivial::trace) << THREAD_ID << msg; if (show){std::stringstream ss; ss << msg; Logger::display(ss.str());}
#define LogDebug(show, msg) BOOST_LOG_SEV(Logger::getLog(), boost::log::trivial::debug) << THREAD_ID << msg; if (show){std::stringstream ss; ss << msg; Logger::display(ss.str());}
#define LogInfo(show, msg) BOOST_LOG_SEV(Logger::getLog(), boost::log::trivial::info) << THREAD_ID << msg; if (show){std::stringstream ss; ss << msg; Logger::display(ss.str());}
#define LogWarn(show, msg) BOOST_LOG_SEV(Logger::getLog(), boost::log::trivial::warning) << THREAD_ID << msg; if (show){std::stringstream ss; ss << msg; Logger::display(ss.str());}
#define LogError(show, msg) BOOST_LOG_SEV(Logger::getLog(), boost::log::trivial::error) << THREAD_ID << LOG_LINE << msg; if (show){std::stringstream ss; ss << LOG_LINE << msg; Logger::display(ss.str());}
#define LogFatal(show, msg) BOOST_LOG_SEV(Logger::getLog(), boost::log::trivial::fatal) << THREAD_ID << msg; if (show){std::stringstream ss; ss << msg; Logger::display(ss.str());}

